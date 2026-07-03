"""
Multi-model enrichment for migrated task.yaml files.

Fills placeholder fields left by migrate.py using LLM advisory proposals.
Never modifies reviewed content and never rewrites the instruction field.

Usage:
    # Propose only (no writes to task.yaml):
    python -m unrealbench.src.authoring.enrich \
        --task-dir tasks_unreal/ue_task_0001_day_night_cycle \
        --field public_contract --field validator_config

    # Apply high-confidence proposals to task.yaml:
    python -m unrealbench.src.authoring.enrich \
        --task-dir tasks_unreal/ue_task_0001_day_night_cycle \
        --field public_contract --apply

    # Batch enrichment:
    python -m unrealbench.src.authoring.enrich \
        --tasks-dir tasks_unreal/ --field public_contract

Fields:
    public_contract   — propose-and-merge (all available models in parallel)
    validator_config  — draft-critique-revise (draft → critique → synthesize)
    instruction       — ambiguity critique only; NEVER rewritten
    solution_classify — advisory labels for bad/near-miss solutions

Output:
    _enrichment/enrich_proposals.yaml  — committed evidence (merged output + conflicts)
    _enrichment_cache/enrich_raw_*.json — local cache of full responses (not committed)

Idempotency:
    Only fills fields that are empty lists, null, or have type-only placeholder validators.
    Fields that already have non-empty content are skipped unless --force is passed.

Provider failure policy:
    - Missing API key: provider skipped with warning; run continues
    - Invalid schema after 3 retries: provider treated as unavailable
    - Two providers fail: all results are singleton tier; --apply not allowed
    - All providers fail: exits non-fatally; no proposals written; task.yaml unchanged
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional, TYPE_CHECKING

import yaml

from .schema import load_task_schema, dump_task_schema
from .multi_model import build_available_clients, run_parallel, ModelResponse

if TYPE_CHECKING:
    from .schema import TaskSchema

_ENRICHMENT_DIR = "_enrichment"
_CACHE_DIR = "_enrichment_cache"
_PROPOSALS_FILE = "enrich_proposals.yaml"

ALL_FIELDS = ("public_contract", "validator_config", "instruction", "solution_classify")


# ── Idempotency guards ────────────────────────────────────────────────────────

def _public_contract_needs_enrichment(schema: "TaskSchema") -> bool:
    pc = schema.public_contract
    return (
        not pc.get("required_classes")
        or not pc.get("required_callables")
    )


def _validator_config_needs_enrichment(schema: "TaskSchema") -> bool:
    for vcfg in schema.private_validators:
        # Placeholder: only has 'type', no substantive config
        if len(vcfg) <= 1:
            return True
        if not vcfg.get("checkpoints") and not vcfg.get("actor_checks"):
            return True
    return False


def _solution_dirs(task_dir: Path) -> tuple[list[Path], list[Path]]:
    bad = sorted((task_dir / "calibration" / "bad_solutions").glob("*")) if (
        task_dir / "calibration" / "bad_solutions"
    ).exists() else []
    near = sorted((task_dir / "calibration" / "near_miss_solutions").glob("*")) if (
        task_dir / "calibration" / "near_miss_solutions"
    ).exists() else []
    return (
        [d for d in bad if d.is_dir()],
        [d for d in near if d.is_dir()],
    )


# ── Consensus helpers ─────────────────────────────────────────────────────────

def _build_run_metadata(requested: list[str], responses: list[ModelResponse]) -> dict:
    available = [r.provider for r in responses if r.parse_ok]
    all_requested = requested
    consensus_mode = "full" if set(available) == set(all_requested) else "degraded"
    return {
        "requested_models": all_requested,
        "available_models": available,
        "consensus_mode": consensus_mode,
        "timestamp": datetime.now(timezone.utc).isoformat(),
    }


def _merge_list_proposals(
    responses: list[ModelResponse],
    field_key: str,
) -> dict:
    """Merge list-typed proposals into confidence tiers.

    Returns dict with keys: high_confidence, majority_confidence, conflicted, singleton, votes
    """
    available = [r for r in responses if r.parse_ok and r.parsed]
    n = len(available)

    if n == 0:
        return {
            "high_confidence": [],
            "majority_confidence": [],
            "conflicted": [],
            "singleton": [],
        }

    # Collect proposals per provider
    proposals: dict[str, list] = {}
    for r in available:
        items = r.parsed.get(field_key, []) if r.parsed else []
        proposals[r.provider] = items if isinstance(items, list) else []

    # Normalize items to comparable form (serialize to JSON for hashing)
    def _key(item) -> str:
        return json.dumps(item, sort_keys=True)

    all_keys: set[str] = set()
    for items in proposals.values():
        for item in items:
            all_keys.add(_key(item))

    high: list = []
    majority: list = []
    conflicted_providers: dict[str, list] = {}
    singleton: list = []

    # Track votes per serialized item
    for k in all_keys:
        voters = [p for p, items in proposals.items() if any(_key(i) == k for i in items)]
        item = json.loads(k)  # restore original form
        vote_count = len(voters)

        if vote_count == n:
            high.append(item)
        elif vote_count >= max(2, (n + 1) // 2):  # majority
            majority.append(item)
        elif vote_count == 1:
            singleton.append(item)
        else:
            # Disagreement without clear majority — classify as conflicted
            for p, items in proposals.items():
                if p not in conflicted_providers:
                    conflicted_providers[p] = []

    # Items proposed differently per provider go to conflicted
    # (handled by examining divergent proposals)
    all_proposal_sets = {p: set(_key(i) for i in items) for p, items in proposals.items()}

    # High and majority already extracted; remaining divergences are conflicted
    classified = set(_key(i) for i in high + majority + singleton)
    for k in all_keys:
        if _key(json.loads(k)) in classified:
            continue
        item = json.loads(k)
        for p, items in proposals.items():
            if p not in conflicted_providers:
                conflicted_providers[p] = []
            if any(_key(i) == k for i in items):
                conflicted_providers[p].append(item)

    return {
        "high_confidence": high,
        "majority_confidence": majority,
        "conflicted": conflicted_providers if conflicted_providers else {},
        "singleton": singleton,
        "votes": {p: len(items) for p, items in proposals.items()},
    }


def _can_auto_apply(metadata: dict, responses: list[ModelResponse]) -> bool:
    """Returns False if provider state makes auto-apply unsafe (all-singleton or all failed)."""
    available = [r for r in responses if r.parse_ok]
    if len(available) <= 1:
        return False  # singleton mode — no auto-apply
    return True


# ── Rationale summaries (committed) ──────────────────────────────────────────

def _rationale_summary(responses: list[ModelResponse], field_key: str) -> dict[str, str]:
    """Extract a single-sentence rationale per provider from parsed output."""
    out: dict[str, str] = {}
    for r in responses:
        if r.parse_ok and r.parsed:
            rationale = r.parsed.get("rationale") or r.parsed.get("reason") or ""
            if isinstance(rationale, str):
                out[r.provider] = rationale[:200]
            elif isinstance(rationale, dict):
                out[r.provider] = str(rationale)[:200]
    return out


# ── Cache writer ──────────────────────────────────────────────────────────────

def _write_cache(task_dir: Path, field: str, responses: list[ModelResponse]) -> None:
    cache_dir = task_dir / _CACHE_DIR
    cache_dir.mkdir(exist_ok=True)
    for r in responses:
        path = cache_dir / f"enrich_{field}_{r.provider}.json"
        with path.open("w", encoding="utf-8") as f:
            json.dump({
                "provider": r.provider,
                "model_id": r.model_id,
                "parse_ok": r.parse_ok,
                "retries_used": r.retries_used,
                "error": r.error,
                "raw_text": r.raw_text,
            }, f, indent=2)


# ── Field-specific enrichers ──────────────────────────────────────────────────

PUBLIC_CONTRACT_SCHEMA = {
    "type": "object",
    "properties": {
        "required_classes": {
            "type": "array",
            "items": {"type": "string"},
            "description": "C++ class names the agent must create (e.g. ADayNightCycleActor)",
        },
        "required_properties": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "class": {"type": "string"},
                    "name": {"type": "string"},
                    "type": {"type": "string"},
                },
                "required": ["class", "name", "type"],
            },
            "description": "UPROPERTY fields that must exist",
        },
        "required_callables": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "class": {"type": "string"},
                    "name": {"type": "string"},
                },
                "required": ["class", "name"],
            },
            "description": "UFUNCTION callables that must exist",
        },
        "rationale": {
            "type": "string",
            "description": "One sentence explaining your inference (for evidence file)",
        },
    },
    "required": ["required_classes", "required_properties", "required_callables", "rationale"],
}


def enrich_public_contract(
    schema: "TaskSchema",
    task_dir: Path,
    clients: list,
    force: bool = False,
) -> dict:
    """Run propose-and-merge for public_contract fields."""
    if not force and not _public_contract_needs_enrichment(schema):
        return {"skipped": True, "reason": "public_contract already populated"}

    system = (
        "You are an expert Unreal Engine 5 C++ architect. "
        "Your job is to infer the public API contract that an agent must implement "
        "for a given benchmark task. Focus on what is externally observable and testable."
    )

    # Build context from task files
    context_parts = [f"task_id: {schema.task_id}", f"task_family: {schema.task_family}"]
    context_parts.append(f"\ninstruction:\n{schema.instruction}")
    context_parts.append(f"\nallowed_files: {json.dumps(schema.allowed_files)}")

    # Include source file stubs if available
    for rel in schema.allowed_files[:4]:  # limit context size
        src = task_dir / "start" / rel
        if src.exists():
            content = src.read_text(encoding="utf-8", errors="ignore")[:2000]
            context_parts.append(f"\n--- {rel} (start stub) ---\n{content}")

    user = "\n".join(context_parts)

    requested = [c.provider for c in clients]
    responses = run_parallel(clients, system, user, PUBLIC_CONTRACT_SCHEMA)
    _write_cache(task_dir, "public_contract", responses)

    metadata = _build_run_metadata(requested, responses)

    # Merge per sub-field
    classes_merge = _merge_list_proposals(responses, "required_classes")
    props_merge = _merge_list_proposals(responses, "required_properties")
    callables_merge = _merge_list_proposals(responses, "required_callables")

    rationale = _rationale_summary(responses, "required_classes")
    can_apply = _can_auto_apply(metadata, responses)

    return {
        "run_metadata": metadata,
        "can_auto_apply": can_apply,
        "required_classes": classes_merge,
        "required_properties": props_merge,
        "required_callables": callables_merge,
        "rationale_summary": rationale,
    }


VALIDATOR_DRAFT_SCHEMA = {
    "type": "object",
    "properties": {
        "type": {"type": "string", "enum": [
            "runtime_behavioral", "editor_inspection", "scene_graph",
            "config_plus_scene", "pipeline_and_frame",
        ]},
        "pie_wait_seconds": {"type": "number"},
        "pie_start_timeout": {"type": "number"},
        "checkpoints": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "time_s": {"type": "number"},
                    "checks": {
                        "type": "array",
                        "items": {
                            "type": "object",
                            "properties": {
                                "actor": {"type": "string"},
                                "property": {"type": "string"},
                                "op": {"type": "string", "enum": ["gt", "lt", "gte", "lte", "eq", "neq"]},
                                "value": {},
                            },
                            "required": ["actor", "property", "op", "value"],
                        },
                    },
                },
                "required": ["time_s", "checks"],
            },
        },
        "actor_checks": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "actor_name": {"type": "string"},
                    "property_path": {"type": "string"},
                    "op": {"type": "string"},
                    "value": {},
                },
                "required": ["actor_name", "property_path", "op", "value"],
            },
        },
    },
    "required": ["type"],
}

VALIDATOR_CRITIQUE_SCHEMA = {
    "type": "object",
    "properties": {
        "risks": {"type": "array", "items": {"type": "string"}},
        "open_questions": {"type": "array", "items": {"type": "string"}},
        "missing_checks": {"type": "array", "items": {"type": "string"}},
    },
    "required": ["risks", "open_questions"],
}


def enrich_validator_config(
    schema: "TaskSchema",
    task_dir: Path,
    clients: list,
    force: bool = False,
) -> dict:
    """Run draft-critique-revise for private validator configs."""
    if not force and not _validator_config_needs_enrichment(schema):
        return {"skipped": True, "reason": "validator config already populated"}

    if not clients:
        return {"skipped": True, "reason": "no model clients available"}

    # Use first available client (Claude preferred) for draft
    drafter = clients[0]
    critics = clients[1:]

    system_draft = (
        "You are an expert Unreal Engine 5 benchmark author. "
        "Draft a precise, implementation-agnostic validator config for this task. "
        "Focus on externally observable state — actor properties, scene graph, runtime behavior. "
        "Do not assume any specific internal implementation."
    )

    context = (
        f"task_id: {schema.task_id}\n"
        f"task_family: {schema.task_family}\n"
        f"instruction:\n{schema.instruction}\n"
        f"public_contract: {json.dumps(schema.public_contract, indent=2)}\n"
        f"starting_state: {json.dumps(schema.starting_state, indent=2)}"
    )

    draft_response = drafter.complete(system_draft, context, VALIDATOR_DRAFT_SCHEMA)
    _write_cache(task_dir, "validator_draft", [draft_response])

    if not draft_response.parse_ok or not draft_response.parsed:
        return {
            "skipped": False,
            "error": f"Draft failed: {draft_response.error}",
            "draft": None,
        }

    draft = draft_response.parsed

    # Critique phase — critics evaluate independently
    system_critique = (
        "You are reviewing a benchmark validator config for an Unreal Engine 5 task. "
        "Identify: (1) underspecification — missing required checks, "
        "(2) overfitting — checks that assume a specific implementation detail, "
        "(3) false negative risk — thresholds too loose, "
        "(4) missing edge cases. "
        "Be specific and concise."
    )
    critique_prompt = (
        f"Task context:\n{context}\n\n"
        f"Proposed validator config:\n{json.dumps(draft, indent=2)}"
    )

    critique_responses: list[ModelResponse] = []
    if critics:
        critique_responses = run_parallel(critics, system_critique, critique_prompt, VALIDATOR_CRITIQUE_SCHEMA)
        _write_cache(task_dir, "validator_critique", critique_responses)

    # Aggregate critique findings
    all_risks: list[str] = []
    all_open_questions: list[str] = []
    for cr in critique_responses:
        if cr.parse_ok and cr.parsed:
            all_risks.extend(cr.parsed.get("risks", []))
            all_open_questions.extend(cr.parsed.get("open_questions", []))

    return {
        "config_proposal": draft,
        "risks": list(dict.fromkeys(all_risks)),       # deduplicated, order-preserved
        "open_questions": list(dict.fromkeys(all_open_questions)),
        "draft_provider": drafter.provider,
        "critique_providers": [c.provider for c in critics if any(
            r.provider == c.provider and r.parse_ok for r in critique_responses
        )],
    }


INSTRUCTION_CRITIQUE_SCHEMA = {
    "type": "object",
    "properties": {
        "ambiguities": {"type": "array", "items": {"type": "string"}},
        "missing_alignment": {"type": "array", "items": {"type": "string"}},
        "suggested_clarifications": {"type": "array", "items": {"type": "string"}},
    },
    "required": ["ambiguities", "missing_alignment", "suggested_clarifications"],
}


def enrich_instruction(
    schema: "TaskSchema",
    task_dir: Path,
    clients: list,
    force: bool = False,
) -> dict:
    """Critique instruction for ambiguity. NEVER rewrites instruction."""
    if not clients:
        return {"skipped": True, "reason": "no model clients available"}

    # Single strong model for critique
    critic = clients[0]

    system = (
        "You are reviewing a benchmark task instruction for an AI agent. "
        "Identify ambiguities, hidden assumptions, and misalignments with the public contract. "
        "Do NOT rewrite the instruction. Only report issues and suggest clarifications."
    )

    user = (
        f"instruction:\n{schema.instruction}\n\n"
        f"public_contract: {json.dumps(schema.public_contract, indent=2)}\n"
        f"allowed_files: {json.dumps(schema.allowed_files)}"
    )

    response = critic.complete(system, user, INSTRUCTION_CRITIQUE_SCHEMA)
    _write_cache(task_dir, "instruction", [response])

    if not response.parse_ok or not response.parsed:
        return {"skipped": False, "error": f"Critique failed: {response.error}"}

    return {
        "advisory_only": True,
        "never_auto_applied": True,
        "ambiguities": response.parsed.get("ambiguities", []),
        "missing_alignment": response.parsed.get("missing_alignment", []),
        "suggested_clarifications": response.parsed.get("suggested_clarifications", []),
    }


SOLUTION_CLASSIFY_SCHEMA = {
    "type": "object",
    "properties": {
        "label": {
            "type": "string",
            "enum": ["likely_bad", "possibly_valid_alt", "near_miss_confirmed", "needs_human_review"],
        },
        "reason": {"type": "string"},
        "confidence": {"type": "string", "enum": ["high", "medium", "low"]},
        "needs_human_review": {"type": "boolean"},
    },
    "required": ["label", "reason", "confidence"],
}


def enrich_solution_classify(
    schema: "TaskSchema",
    task_dir: Path,
    clients: list,
    force: bool = False,
) -> dict:
    """Advisory classification of bad/near-miss solutions."""
    bad_dirs, near_dirs = _solution_dirs(task_dir)

    if not bad_dirs and not near_dirs:
        return {"skipped": True, "reason": "no calibration solutions found"}

    if not clients:
        return {"skipped": True, "reason": "no model clients available"}

    classifier = clients[0]
    system = (
        "You are reviewing a benchmark calibration solution. "
        "Classify whether this is genuinely incorrect (likely_bad), "
        "a valid alternate implementation (possibly_valid_alt), "
        "a near-miss that partially satisfies the task (near_miss_confirmed), "
        "or ambiguous (needs_human_review). "
        "This classification is advisory only — it does not affect benchmark scoring."
    )

    def _classify_dir(sol_dir: Path, expected_set: str) -> dict:
        files_text = ""
        for rel in schema.writable_files[:4]:
            src = sol_dir / rel
            if src.exists():
                content = src.read_text(encoding="utf-8", errors="ignore")[:1500]
                files_text += f"\n--- {rel} ---\n{content}"
            else:
                fname = Path(rel).name
                candidate = sol_dir / fname
                if candidate.exists():
                    content = candidate.read_text(encoding="utf-8", errors="ignore")[:1500]
                    files_text += f"\n--- {fname} ---\n{content}"

        user = (
            f"Task instruction:\n{schema.instruction}\n\n"
            f"Public contract: {json.dumps(schema.public_contract, indent=2)}\n\n"
            f"Solution files (expected to be '{expected_set}'):\n{files_text}"
        )
        r = classifier.complete(system, user, SOLUTION_CLASSIFY_SCHEMA)
        if r.parse_ok and r.parsed:
            return {
                "label": r.parsed.get("label", "needs_human_review"),
                "reason": r.parsed.get("reason", ""),
                "confidence": r.parsed.get("confidence", "low"),
                "needs_human_review": r.parsed.get("needs_human_review", False),
            }
        return {"label": "needs_human_review", "reason": f"classification failed: {r.error}", "confidence": "low", "needs_human_review": True}

    result: dict = {"advisory_only": True}
    if bad_dirs:
        result["bad_solutions"] = {d.name: _classify_dir(d, "bad") for d in bad_dirs}
    if near_dirs:
        result["near_miss_solutions"] = {d.name: _classify_dir(d, "near_miss") for d in near_dirs}

    return result


# ── Proposal writer ───────────────────────────────────────────────────────────

def _write_proposals(task_dir: Path, proposals: dict) -> Path:
    enrichment_dir = task_dir / _ENRICHMENT_DIR
    enrichment_dir.mkdir(exist_ok=True)
    out_path = enrichment_dir / _PROPOSALS_FILE

    # Merge with existing proposals if present
    existing: dict = {}
    if out_path.exists():
        with out_path.open() as f:
            existing = yaml.safe_load(f) or {}

    existing.update(proposals)
    with out_path.open("w", encoding="utf-8") as f:
        yaml.dump(existing, f, default_flow_style=False, allow_unicode=True, sort_keys=False)

    return out_path


# ── Apply high-confidence proposals to task.yaml ──────────────────────────────

def _find_map_path(task_dir: Path) -> Optional[str]:
    """Scan start/ for .umap files and return the first one as a Content-relative path."""
    start_dir = task_dir / "start"
    if not start_dir.exists():
        return None
    for umap in sorted(start_dir.rglob("*.umap")):
        try:
            rel = umap.relative_to(start_dir)
            return str(rel).replace("\\", "/")
        except ValueError:
            continue
    return None


def _apply_proposals(task_dir: Path, proposals: dict, auto: bool = False) -> list[str]:
    """Apply proposals to task.yaml.

    --apply (auto=False): applies only high_confidence public_contract fields.
    --auto  (auto=True):  additionally applies majority_confidence fields,
                          validator_config_proposal, and auto-detected starting_state.map.
    Instruction is NEVER applied in either mode.

    Returns list of field names that were written.
    """
    applied: list[str] = []

    pc_proposal = proposals.get("public_contract_proposal", {})
    can_apply = pc_proposal.get("can_auto_apply", True) if pc_proposal else True

    if pc_proposal and not can_apply and auto:
        print("  WARN: singleton mode — majority_confidence fields will not be auto-applied")

    schema = load_task_schema(task_dir, strict=False)
    pc = dict(schema.public_contract)
    ss = dict(schema.starting_state)
    changed = False

    # ── public_contract sub-fields ────────────────────────────────────────────
    if pc_proposal:
        tiers = ["high_confidence"] + (["majority_confidence"] if auto and can_apply else [])
        for sub_field in ("required_classes", "required_properties", "required_callables"):
            if pc.get(sub_field):
                continue  # already populated — never overwrite
            merge = pc_proposal.get(sub_field, {})
            combined: list = []
            seen_keys: set[str] = set()
            for tier in tiers:
                for item in merge.get(tier, []):
                    k = json.dumps(item, sort_keys=True)
                    if k not in seen_keys:
                        seen_keys.add(k)
                        combined.append(item)
            if combined:
                pc[sub_field] = combined
                applied.append(f"public_contract.{sub_field}")
                changed = True

    # Remove behavior_only placeholder once required output surfaces are populated
    if pc.get("behavior_only") and (pc.get("required_classes") or pc.get("required_callables")):
        pc.pop("behavior_only")
        applied.append("public_contract.behavior_only (removed)")
        changed = True

    schema.public_contract = pc

    # ── starting_state.map (auto only) ───────────────────────────────────────
    if auto and not ss.get("map"):
        map_path = _find_map_path(task_dir)
        if map_path:
            ss["map"] = map_path
            schema.starting_state = ss
            applied.append("starting_state.map")
            changed = True

    # ── validator_config (auto only) ─────────────────────────────────────────
    if auto:
        vc_proposal = proposals.get("validator_config_proposal", {})
        config = vc_proposal.get("config_proposal") if vc_proposal else None
        if config and isinstance(config, dict) and config.get("type"):
            # Replace placeholder private validators (type-only entries)
            private_validators = schema.private_validators
            has_placeholder = any(len(v) <= 1 for v in private_validators)
            if has_placeholder or not private_validators:
                # private_validators is a read-only property — mutate validation dict directly
                if isinstance(schema.validation, dict):
                    priv = schema.validation.setdefault("private", {})
                    priv["validators"] = [config]
                    applied.append("validation.private.validators")
                    changed = True

    if changed:
        yaml_path = task_dir / "task.yaml"
        dump_task_schema(schema, yaml_path)

    return applied


# ── Per-task runner ───────────────────────────────────────────────────────────

def enrich_task(
    task_dir: Path,
    fields: tuple[str, ...],
    apply: bool = False,
    auto: bool = False,
    force: bool = False,
) -> dict:
    task_dir = Path(task_dir)
    yaml_path = task_dir / "task.yaml"

    if not yaml_path.exists():
        return {"error": f"no task.yaml in {task_dir}"}

    try:
        schema = load_task_schema(task_dir, strict=False)
    except Exception as exc:
        return {"error": f"schema load failed: {exc}"}

    proposals: dict = {}
    clients: list = []

    # Only build clients if we actually need to call LLMs
    needs_llm = any(f in fields for f in ("public_contract", "validator_config", "instruction", "solution_classify"))
    if needs_llm:
        clients = build_available_clients()
        if not clients:
            if not (apply or auto):
                print("  ERROR: no model clients available — check API keys")
                return {"error": "no_clients"}
            # If apply/auto is set and no clients, we can still apply existing proposals
            print("  WARN: no model clients available — skipping enrichment, applying existing proposals only")

    requested_providers = [c.provider for c in clients]

    if clients:
        if "public_contract" in fields:
            print(f"  Enriching public_contract ({', '.join(requested_providers)})...")
            result = enrich_public_contract(schema, task_dir, clients, force=force)
            if result.get("skipped"):
                print(f"    Skipped: {result.get('reason')}")
            else:
                proposals["public_contract_proposal"] = result

        if "validator_config" in fields:
            print(f"  Enriching validator_config (draft-critique-revise)...")
            result = enrich_validator_config(schema, task_dir, clients, force=force)
            if result.get("skipped"):
                print(f"    Skipped: {result.get('reason')}")
            else:
                proposals["validator_config_proposal"] = result

        if "instruction" in fields:
            print(f"  Critiquing instruction (advisory only — will not be rewritten)...")
            result = enrich_instruction(schema, task_dir, clients, force=force)
            if result.get("skipped"):
                print(f"    Skipped: {result.get('reason')}")
            else:
                proposals["instruction_audit"] = result

        if "solution_classify" in fields:
            print(f"  Classifying solutions (advisory)...")
            result = enrich_solution_classify(schema, task_dir, clients, force=force)
            if result.get("skipped"):
                print(f"    Skipped: {result.get('reason')}")
            else:
                sol_path = task_dir / _ENRICHMENT_DIR / "solution_classification.yaml"
                (task_dir / _ENRICHMENT_DIR).mkdir(exist_ok=True)
                with sol_path.open("w", encoding="utf-8") as f:
                    yaml.dump(result, f, default_flow_style=False, allow_unicode=True, sort_keys=False)
                print(f"    Wrote {sol_path.relative_to(task_dir.parent)}")

    if proposals:
        out = _write_proposals(task_dir, proposals)
        print(f"  Wrote {out.relative_to(task_dir.parent)}")

    if apply or auto:
        # Merge new proposals with any existing ones on disk
        existing_proposals: dict = {}
        proposals_path = task_dir / _ENRICHMENT_DIR / _PROPOSALS_FILE
        if proposals_path.exists():
            with proposals_path.open() as f:
                existing_proposals = yaml.safe_load(f) or {}
        merged = {**existing_proposals, **proposals}  # new proposals win on conflict

        if merged:
            applied = _apply_proposals(task_dir, merged, auto=auto)
            if applied:
                mode = "--auto" if auto else "--apply"
                print(f"  Applied to task.yaml ({mode}): {applied}")
            else:
                print(f"  Nothing applied (no proposals or already populated)")
        else:
            print(f"  Nothing to apply — run enrichment first to generate proposals")

    return proposals


def enrich_all(
    tasks_dir: Path,
    fields: tuple[str, ...],
    apply: bool = False,
    auto: bool = False,
    force: bool = False,
) -> None:
    tasks_dir = Path(tasks_dir)
    task_dirs = sorted(tasks_dir.glob("ue_task_*"))
    if not task_dirs:
        print(f"No ue_task_* directories found in {tasks_dir}")
        return

    for td in task_dirs:
        if not td.is_dir():
            continue
        print(f"\n{'─'*60}")
        print(f"Enriching: {td.name}")
        result = enrich_task(td, fields=fields, apply=apply, auto=auto, force=force)
        if isinstance(result, dict) and "error" in result:
            print(f"  ERROR: {result['error']}")


# ── CLI ───────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Multi-model enrichment for migrated task.yaml files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--task-dir", help="Single task directory")
    group.add_argument("--tasks-dir", help="Directory containing ue_task_* subdirs")

    parser.add_argument(
        "--field",
        action="append",
        dest="fields",
        choices=list(ALL_FIELDS),
        metavar="FIELD",
        help=(
            "Field to enrich; may be repeated. "
            f"Choices: {', '.join(ALL_FIELDS)}"
        ),
    )
    parser.add_argument(
        "--all-fields",
        action="store_true",
        help="Enrich all fields",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Apply high-confidence public_contract proposals to task.yaml",
    )
    parser.add_argument(
        "--auto",
        action="store_true",
        help=(
            "Aggressively apply all proposals: high+majority public_contract, "
            "validator_config, and auto-detected starting_state.map. "
            "Instruction is never auto-applied."
        ),
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-run enrichment even if field already populated",
    )

    args = parser.parse_args()

    if args.all_fields:
        fields = ALL_FIELDS
    elif args.fields:
        fields = tuple(args.fields)
    else:
        parser.error("Specify --field FIELD (one or more) or --all-fields")
        return 2

    if args.task_dir:
        result = enrich_task(
            Path(args.task_dir), fields=fields,
            apply=args.apply, auto=args.auto, force=args.force,
        )
        return 0 if "error" not in result else 1

    enrich_all(
        Path(args.tasks_dir), fields=fields,
        apply=args.apply, auto=args.auto, force=args.force,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
