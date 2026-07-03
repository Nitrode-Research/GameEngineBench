"""
Two-stage consistency audit for task.yaml files.

Stage A: Deterministic structural checks (rule-based, no LLM).
Stage B: LLM semantic audit for natural language contradictions (Claude Opus by default).

Stage B runs only if Stage A is clean, unless --force-llm is passed.

Usage:
    python -m unrealbench.src.authoring.audit \
        --task-dir tasks_unreal/ue_task_0001_day_night_cycle

    python -m unrealbench.src.authoring.audit \
        --tasks-dir tasks_unreal/

    python -m unrealbench.src.authoring.audit \
        --task-dir tasks_unreal/ue_task_0001_day_night_cycle --thorough
        # runs Stage B with all 3 providers

Exit codes:
    0 = clean or warnings only
    1 = any critical issue found in Stage A or B
    2 = configuration error (no task.yaml, schema load failure)
"""

from __future__ import annotations

import argparse
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Optional, TYPE_CHECKING

import yaml

from .schema import load_task_schema
from .multi_model import build_available_clients, run_parallel, ModelResponse

if TYPE_CHECKING:
    from .schema import TaskSchema

_ENRICHMENT_DIR = "_enrichment"
_AUDIT_FILE = "audit_report.yaml"


# ── Issue container ───────────────────────────────────────────────────────────

def _issue(severity: str, check: str, detail: str, fix: str = "") -> dict:
    return {"severity": severity, "check": check, "detail": detail, "fix_suggestion": fix}


# ── Stage A: Deterministic structural checks ──────────────────────────────────

def _actor_names_in_starting_state(schema: "TaskSchema") -> set[str]:
    ss = schema.starting_state
    names: set[str] = set()
    for entry in ss.get("agent_must_place", []):
        if isinstance(entry, dict) and "name" in entry:
            names.add(entry["name"])
    for entry in ss.get("starter_map_provides", []):
        if isinstance(entry, dict) and "name" in entry:
            names.add(entry["name"])
    return names


def _classes_in_starting_state(schema: "TaskSchema") -> set[str]:
    ss = schema.starting_state
    classes: set[str] = set()
    for entry in ss.get("agent_must_place", []):
        if isinstance(entry, dict) and "class" in entry:
            classes.add(entry["class"])
    for entry in ss.get("starter_map_provides", []):
        if isinstance(entry, dict) and "class" in entry:
            classes.add(entry["class"])
    return classes


def _validator_actor_refs(vcfg: dict) -> set[str]:
    refs: set[str] = set()
    for cp in vcfg.get("checkpoints", []):
        for chk in cp.get("checks", []):
            if "actor" in chk:
                refs.add(chk["actor"])
    for ac in vcfg.get("actor_checks", []):
        if "actor_name" in ac:
            refs.add(ac["actor_name"])
    return refs


def _validator_class_refs(vcfg: dict) -> set[str]:
    refs: set[str] = set()
    for ac in vcfg.get("actor_checks", []):
        if "class" in ac:
            refs.add(ac["class"])
    for cp in vcfg.get("checkpoints", []):
        for chk in cp.get("checks", []):
            if "class" in chk:
                refs.add(chk["class"])
    return refs


def run_stage_a(schema: "TaskSchema") -> list[dict]:
    """Deterministic structural checks. Returns list of issue dicts."""
    issues: list[dict] = []
    pc = schema.public_contract
    ss = schema.starting_state

    # 1. allowed_files vs writable_files consistency
    allowed = set(pc.get("allowed_files", []))
    writable = set(ss.get("writable_files", []))
    if writable and allowed:
        extras = writable - allowed
        if extras:
            issues.append(_issue(
                "warning",
                "writable_files_not_in_allowed_files",
                f"writable_files contains paths not in allowed_files: {sorted(extras)}",
                "Add these paths to public_contract.allowed_files or remove from writable_files",
            ))

    # 2. required_callables classes exist in required_classes
    req_classes = set(pc.get("required_classes", []))
    for callable_entry in pc.get("required_callables", []):
        if isinstance(callable_entry, dict):
            cls = callable_entry.get("class", "")
            if cls and req_classes and cls not in req_classes:
                issues.append(_issue(
                    "critical",
                    "callable_class_not_in_required_classes",
                    f"required_callables references class '{cls}' which is not in required_classes",
                    f"Add '{cls}' to public_contract.required_classes",
                ))

    # 3. required_properties classes exist in required_classes
    for prop_entry in pc.get("required_properties", []):
        if isinstance(prop_entry, dict):
            cls = prop_entry.get("class", "")
            if cls and req_classes and cls not in req_classes:
                issues.append(_issue(
                    "critical",
                    "property_class_not_in_required_classes",
                    f"required_properties references class '{cls}' which is not in required_classes",
                    f"Add '{cls}' to public_contract.required_classes",
                ))

    # 4. Validator actor references exist in starting_state
    known_actors = _actor_names_in_starting_state(schema)
    for vcfg in schema.private_validators:
        actor_refs = _validator_actor_refs(vcfg)
        unknown = actor_refs - known_actors
        if unknown and known_actors:  # only flag if starting_state is populated
            for actor in unknown:
                issues.append(_issue(
                    "critical",
                    "validator_actor_not_in_starting_state",
                    f"Validator references actor '{actor}' but it is not in agent_must_place or starter_map_provides",
                    f"Add '{actor}' to starting_state.agent_must_place",
                ))

    # 5. Validator class references vs required_classes
    for vcfg in schema.private_validators:
        cls_refs = _validator_class_refs(vcfg)
        unknown_cls = cls_refs - req_classes
        if unknown_cls and req_classes:
            for cls in unknown_cls:
                issues.append(_issue(
                    "warning",
                    "validator_class_not_in_required_classes",
                    f"Validator references class '{cls}' which is not in required_classes",
                    f"Add '{cls}' to public_contract.required_classes or remove from validator",
                ))

    # 6. If schema_static_sanity is used, public_contract must provide output-shape expectations
    uses_schema_static = any(v.get("type") == "schema_static_sanity" for v in schema.public_validators)
    if uses_schema_static and not req_classes and not pc.get("required_callables"):
        issues.append(_issue(
            "critical",
            "schema_static_without_public_contract_shape",
            "schema_static_sanity is configured but public_contract has no "
            "required_classes or required_callables",
            "Populate public_contract or switch to a validator that does not depend on shape checks",
        ))

    # 7. Hybrid tasks must have configured (non-placeholder) private validators
    if schema.capability_mode == "hybrid":
        for vcfg in schema.private_validators:
            if len(vcfg) <= 1:
                issues.append(_issue(
                    "critical",
                    "hybrid_task_placeholder_validator",
                    f"Private validator has only 'type' field — no substantive config",
                    "Author validator config or run enrich.py --field validator_config",
                ))

    return issues


# ── Stage B: LLM semantic audit ───────────────────────────────────────────────

SEMANTIC_AUDIT_SCHEMA = {
    "type": "object",
    "properties": {
        "issues": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {
                    "severity": {"type": "string", "enum": ["critical", "warning"]},
                    "check": {"type": "string"},
                    "detail": {"type": "string"},
                    "fix_suggestion": {"type": "string"},
                },
                "required": ["severity", "check", "detail"],
            },
        },
    },
    "required": ["issues"],
}


def run_stage_b(schema: "TaskSchema", clients: list, thorough: bool = False) -> list[dict]:
    """LLM semantic audit. Returns list of issue dicts."""
    if not clients:
        return []

    active_clients = clients if thorough else clients[:1]

    import json
    system = (
        "You are auditing a benchmark task specification for self-consistency. "
        "Find contradictions between: instruction vs public_contract, "
        "public_contract vs starting_state, validator configs vs instruction. "
        "Also flag: underspecified behavior, hidden assumptions, "
        "or missing success criteria in the instruction. "
        "Return ONLY issues you are confident about. Do not invent problems."
    )

    user = (
        f"task_id: {schema.task_id}\n\n"
        f"instruction:\n{schema.instruction}\n\n"
        f"public_contract:\n{json.dumps(schema.public_contract, indent=2)}\n\n"
        f"starting_state:\n{json.dumps(schema.starting_state, indent=2)}\n\n"
        f"validation:\n{json.dumps(schema.validation, indent=2)}"
    )

    responses = run_parallel(active_clients, system, user, SEMANTIC_AUDIT_SCHEMA)

    all_issues: list[dict] = []
    for r in responses:
        if r.parse_ok and r.parsed:
            for issue in r.parsed.get("issues", []):
                # Tag with provider for --thorough mode
                tagged = dict(issue)
                if thorough:
                    tagged["flagged_by"] = r.provider
                all_issues.append(tagged)

    if thorough and len(active_clients) > 1:
        # Deduplicate by (check, severity) — keep all unique issues
        seen: set[str] = set()
        deduped: list[dict] = []
        for issue in all_issues:
            key = f"{issue.get('check', '')}:{issue.get('detail', '')[:80]}"
            if key not in seen:
                seen.add(key)
                deduped.append(issue)
        return deduped

    return all_issues


# ── Report writer ─────────────────────────────────────────────────────────────

def _write_audit_report(task_dir: Path, stage_a: list[dict], stage_b: list[dict]) -> Path:
    enrichment_dir = task_dir / _ENRICHMENT_DIR
    enrichment_dir.mkdir(exist_ok=True)
    report_path = enrichment_dir / _AUDIT_FILE

    a_critical = sum(1 for i in stage_a if i["severity"] == "critical")
    a_warnings = sum(1 for i in stage_a if i["severity"] == "warning")
    b_critical = sum(1 for i in stage_b if i["severity"] == "critical")
    b_warnings = sum(1 for i in stage_b if i["severity"] == "warning")
    total_critical = a_critical + b_critical

    report = {
        "task_id": _try_task_id(task_dir),
        "audit_timestamp": datetime.now(timezone.utc).isoformat(),
        "stage_a_issues": stage_a,
        "stage_b_issues": stage_b,
        "summary": {
            "stage_a": {"critical": a_critical, "warnings": a_warnings},
            "stage_b": {"critical": b_critical, "warnings": b_warnings},
            "clean": total_critical == 0,
        },
    }

    with report_path.open("w", encoding="utf-8") as f:
        yaml.dump(report, f, default_flow_style=False, allow_unicode=True, sort_keys=False)

    return report_path


def _try_task_id(task_dir: Path) -> str:
    try:
        schema = load_task_schema(task_dir)
        return schema.task_id
    except Exception:
        return task_dir.name


# ── Per-task runner ───────────────────────────────────────────────────────────

def audit_task(task_dir: Path, force_llm: bool = False, thorough: bool = False) -> int:
    """Audit one task. Returns exit code (0=clean/warn, 1=critical, 2=config error)."""
    task_dir = Path(task_dir)
    print(f"\n{'═'*60}")
    print(f"AUDIT: {task_dir.name}")

    yaml_path = task_dir / "task.yaml"
    if not yaml_path.exists():
        print(f"  ERROR: no task.yaml")
        return 2

    try:
        schema = load_task_schema(task_dir, strict=False)
    except Exception as exc:
        print(f"  ERROR: schema load failed — {exc}")
        return 2

    # Stage A
    print("  Stage A: deterministic structural checks...")
    stage_a_issues = run_stage_a(schema)
    _print_issues("A", stage_a_issues)

    a_critical = sum(1 for i in stage_a_issues if i["severity"] == "critical")

    # Stage B
    stage_b_issues: list[dict] = []
    if a_critical > 0 and not force_llm:
        print("  Stage B: skipped (Stage A has critical issues; use --force-llm to override)")
    else:
        print(f"  Stage B: LLM semantic audit{'  [--thorough]' if thorough else ''}...")
        clients = build_available_clients()
        if not clients:
            print("  Stage B: skipped (no model clients available)")
        else:
            stage_b_issues = run_stage_b(schema, clients, thorough=thorough)
            _print_issues("B", stage_b_issues)

    report_path = _write_audit_report(task_dir, stage_a_issues, stage_b_issues)
    print(f"\n  Report: {report_path.relative_to(task_dir.parent)}")

    total_critical = a_critical + sum(1 for i in stage_b_issues if i["severity"] == "critical")
    if total_critical == 0:
        print("  RESULT: CLEAN")
        return 0
    else:
        print(f"  RESULT: {total_critical} critical issue(s) — fix before proceeding")
        return 1


def _print_issues(stage: str, issues: list[dict]) -> None:
    if not issues:
        print(f"    [Stage {stage}] No issues")
        return
    for issue in issues:
        sev = issue.get("severity", "?").upper()
        check = issue.get("check", "?")
        detail = issue.get("detail", "")
        fix = issue.get("fix_suggestion", "")
        print(f"    [{sev}] {check}")
        print(f"           {detail}")
        if fix:
            print(f"           Fix: {fix}")


def audit_all(tasks_dir: Path, force_llm: bool = False, thorough: bool = False) -> int:
    tasks_dir = Path(tasks_dir)
    task_dirs = sorted(tasks_dir.glob("ue_task_*"))
    if not task_dirs:
        print(f"No ue_task_* directories found in {tasks_dir}")
        return 2

    results: dict[str, int] = {}
    for td in task_dirs:
        if not td.is_dir():
            continue
        rc = audit_task(td, force_llm=force_llm, thorough=thorough)
        results[td.name] = rc

    clean = sum(1 for rc in results.values() if rc == 0)
    critical = sum(1 for rc in results.values() if rc == 1)
    error = sum(1 for rc in results.values() if rc == 2)

    print(f"\n{'═'*60}")
    print(f"SUMMARY: {clean}/{len(results)} clean, {critical} with critical issues, {error} errors")

    return 0 if critical == 0 and error == 0 else 1


# ── CLI ───────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Two-stage consistency audit for task.yaml files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--task-dir", help="Single task directory")
    group.add_argument("--tasks-dir", help="Directory containing ue_task_* subdirs")

    parser.add_argument(
        "--force-llm",
        action="store_true",
        help="Run Stage B even if Stage A has critical issues",
    )
    parser.add_argument(
        "--thorough",
        action="store_true",
        help="Run Stage B with all available providers and deduplicate findings",
    )

    args = parser.parse_args()

    if args.task_dir:
        return audit_task(Path(args.task_dir), force_llm=args.force_llm, thorough=args.thorough)

    return audit_all(Path(args.tasks_dir), force_llm=args.force_llm, thorough=args.thorough)


if __name__ == "__main__":
    sys.exit(main())
