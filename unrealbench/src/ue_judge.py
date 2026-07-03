#!/usr/bin/env python3
from __future__ import annotations

import asyncio
import importlib.util
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path
from typing import Optional, Sequence

from unrealbench.src.claude_code_sdk_compat import install as install_claude_code_sdk_compat
from unrealbench.src.utils.claude_auth import claude_sdk_auth_environment


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_AUDIT_SCRIPT = REPO_ROOT / "tasks_unreal" / "audit.py"
DEFAULT_CLAUDE_JUDGE_MODEL = "claude-opus-4-7"
DEFAULT_GPT_JUDGE_MODEL = "gpt-5.5"
DEFAULT_GPT_JUDGE_REASONING_EFFORT = os.environ.get("UNREALBENCH_GPT_JUDGE_REASONING_EFFORT", "high").strip() or "high"

_TEST_DECL_RE = re.compile(
    r"IMPLEMENT_SIMPLE_AUTOMATION_TEST\s*\(\s*F(\w+)Test\s*,\s*\"([^\"]+)\"",
    re.MULTILINE,
)


def _solver_family_text(model: str | None, agent: str | None = None) -> str:
    """Classify by model first; only fall back to the harness when model is absent."""
    model_text = (model or "").strip().lower()
    if model_text and model_text != "default":
        return model_text
    return (agent or "").strip().lower()


def _optional_bool(value: object) -> Optional[bool]:
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        lowered = value.strip().lower()
        if lowered in {"true", "yes", "1"}:
            return True
        if lowered in {"false", "no", "0"}:
            return False
    return None


def _normalize_hidden_requirements(value: object) -> dict[str, dict]:
    requirements: dict[str, dict] = {}
    if isinstance(value, dict):
        for req_id, raw_req in value.items():
            req_key = str(req_id)
            if isinstance(raw_req, dict):
                req = dict(raw_req)
                req.setdefault("text", str(raw_req.get("requirement") or raw_req.get("description") or ""))
            else:
                req = {"text": str(raw_req)}
            req["id"] = req_key
            requirements[req_key] = req
    elif isinstance(value, list):
        for idx, raw_req in enumerate(value, start=1):
            if isinstance(raw_req, dict):
                req_key = str(raw_req.get("id") or f"R{idx}")
                req = dict(raw_req)
                req.setdefault("text", str(raw_req.get("requirement") or raw_req.get("description") or ""))
            else:
                req_key = f"R{idx}"
                req = {"text": str(raw_req)}
            req["id"] = req_key
            requirements[req_key] = req
    return requirements


def _format_hidden_requirements_block(requirements: dict[str, dict]) -> str:
    if not requirements:
        return "(none provided)"
    lines = []
    for req_id, req in requirements.items():
        text = str(req.get("text") or req.get("requirement") or "").strip()
        category = str(req.get("category") or "").strip()
        suffix = f" [{category}]" if category else ""
        lines.append(f"- {req_id}{suffix}: {text}")
    return "\n".join(lines)


def _requirement_ids_for_test(
    test_requirements: object,
    *,
    full_id: str,
    short_id: str,
) -> list[str]:
    if not isinstance(test_requirements, dict):
        return []
    candidates = [full_id, short_id]
    if "." in full_id:
        candidates.append(full_id.rsplit(".", 1)[-1])
    raw_ids = None
    for candidate in candidates:
        if candidate in test_requirements:
            raw_ids = test_requirements[candidate]
            break
    if raw_ids is None:
        return []
    if isinstance(raw_ids, str):
        return [raw_ids]
    if isinstance(raw_ids, list):
        return [str(item) for item in raw_ids]
    return []


def _format_covered_requirements_block(
    covered_ids: Sequence[str],
    hidden_requirements: dict[str, dict],
) -> str:
    if not covered_ids:
        return "(none mapped for this test)"
    lines = []
    for req_id in covered_ids:
        req = hidden_requirements.get(req_id, {})
        text = str(req.get("text") or req.get("requirement") or "").strip()
        category = str(req.get("category") or "").strip()
        suffix = f" [{category}]" if category else ""
        if text:
            lines.append(f"- {req_id}{suffix}: {text}")
        else:
            lines.append(f"- {req_id}")
    return "\n".join(lines)


def _format_test_requirements_block(test_requirements: object) -> str:
    if not isinstance(test_requirements, dict) or not test_requirements:
        return "(none provided)"
    lines = []
    for test_id, req_ids in test_requirements.items():
        if isinstance(req_ids, str):
            req_text = req_ids
        elif isinstance(req_ids, list):
            req_text = ", ".join(str(req_id) for req_id in req_ids)
        else:
            req_text = str(req_ids)
        lines.append(f"- {test_id}: {req_text}")
    return "\n".join(lines)


def _format_requirement_coverage_block(
    hidden_requirements: dict[str, dict],
    test_requirements: object,
) -> str:
    if not hidden_requirements:
        return "(none provided)"
    covered_by: dict[str, list[str]] = {req_id: [] for req_id in hidden_requirements}
    if isinstance(test_requirements, dict):
        for test_id, req_ids in test_requirements.items():
            if isinstance(req_ids, str):
                req_iter = [req_ids]
            elif isinstance(req_ids, list):
                req_iter = [str(req_id) for req_id in req_ids]
            else:
                continue
            for req_id in req_iter:
                if req_id in covered_by:
                    covered_by[req_id].append(str(test_id))

    lines = []
    for req_id, req in hidden_requirements.items():
        text = str(req.get("text") or req.get("requirement") or "").strip()
        tests = covered_by.get(req_id) or []
        coverage = ", ".join(tests) if tests else "not directly covered by hidden tests"
        lines.append(f"- {req_id}: {coverage} | {text}")
    return "\n".join(lines)


def _format_test_outcomes_block(test_results: Sequence[dict]) -> str:
    if not test_results:
        return "(no parsed test results)"
    lines = []
    for tr in test_results:
        test_id = str(tr.get("id") or "?")
        outcome = "PASSED" if tr.get("passed") else "FAILED"
        errors = tr.get("errors") or []
        line = f"- {test_id}: {outcome}"
        if errors:
            line += " | " + " | ".join(str(err) for err in errors[:3])
        lines.append(line)
    return "\n".join(lines)


def _format_per_test_audit_block(classifications: Sequence[dict]) -> str:
    if not classifications:
        return "(per-test audit unavailable)"
    lines = []
    for c in classifications:
        covered = c.get("covered_requirement_ids") or []
        covered_text = ", ".join(str(req_id) for req_id in covered) if covered else "-"
        correct = c.get("model_code_correct_for_test")
        rationale = str(c.get("rationale") or "").replace("\n", " ").strip()
        lines.append(
            f"- {c.get('test_id', '?')}: outcome={c.get('outcome', '?')} "
            f"classification={c.get('classification', '?')} correct_for_test={correct} "
            f"covered={covered_text} rationale={rationale}"
        )
    return "\n".join(lines)


def _normalize_inferred_missing_requirements(value: object) -> list[dict]:
    if not isinstance(value, list):
        return []
    normalized: list[dict] = []
    for raw_req in value:
        if not isinstance(raw_req, dict):
            continue
        requirement = str(raw_req.get("requirement") or raw_req.get("text") or "").strip()
        evidence = str(raw_req.get("evidence") or "").strip()
        if not requirement:
            continue
        source = str(raw_req.get("source") or "task_description").strip()
        item = {
            "requirement": requirement,
            "evidence": evidence,
            "severity": "task_failing",
            "source": source,
        }
        requirement_id = str(raw_req.get("requirement_id") or raw_req.get("id") or "").strip()
        if requirement_id:
            item["requirement_id"] = requirement_id
        normalized.append(item)
    return normalized


def _ensure_full_task_failure_has_missing_requirement(
    inferred_missing_requirements: list[dict],
    *,
    full_task_satisfies_explicitly: Optional[bool],
    explicit_requirement_satisfaction: dict[str, dict],
    hidden_requirements: dict[str, dict],
    classifications: Sequence[dict],
    rationale: str,
) -> list[dict]:
    """Guarantee machine-readable failure evidence for full-task failures.

    The prompt asks the judge to emit inferred_missing_requirements whenever the
    full-task verdict is false, but LLMs sometimes put the reason only in prose.
    Downstream analysis needs a structured item for every calibrated failure.
    """
    if inferred_missing_requirements:
        return inferred_missing_requirements

    explicit_failures = [
        (req_id, data)
        for req_id, data in explicit_requirement_satisfaction.items()
        if isinstance(data, dict) and data.get("satisfied") is False
    ]
    if explicit_failures:
        req_id, data = explicit_failures[0]
        hidden = hidden_requirements.get(req_id) or {}
        requirement = str(hidden.get("text") or f"Explicit hidden requirement {req_id}").strip()
        evidence = str(data.get("evidence") or rationale or "The judge marked this explicit hidden requirement unsatisfied.").strip()
        return [{
            "id": "I1",
            "requirement_id": req_id,
            "requirement": requirement,
            "evidence": evidence,
            "severity": "task_failing",
            "source": "task_description",
        }]

    per_test_failures = [
        c for c in classifications
        if isinstance(c, dict) and c.get("model_code_correct_for_test") is False
    ]
    if per_test_failures:
        failure = per_test_failures[0]
        test_id = str(failure.get("test_id") or "the tested behavior")
        covered = failure.get("covered_requirement_ids") or []
        req_id = str(covered[0]) if covered else ""
        hidden = hidden_requirements.get(req_id) or {}
        requirement = str(hidden.get("text") or f"The model must satisfy the behavior tested by {test_id}.").strip()
        evidence = str(failure.get("rationale") or rationale or "The per-test audit marked the model incorrect for this tested behavior.").strip()
        item = {
            "id": "I1",
            "requirement": requirement,
            "evidence": evidence,
            "severity": "task_failing",
            "source": "test_source",
        }
        if req_id:
            item["requirement_id"] = req_id
        return [item]

    if full_task_satisfies_explicitly is False:
        return [{
            "id": "I1",
            "requirement": "The submitted implementation must satisfy the full task contract.",
            "evidence": str(rationale or "The full-task judge marked the submission incomplete but did not provide a structured missing requirement.").strip(),
            "severity": "task_failing",
            "source": "task_description",
        }]

    return inferred_missing_requirements


def _normalize_explicit_requirement_satisfaction(
    value: object,
    hidden_requirements: dict[str, dict],
) -> dict[str, dict]:
    normalized: dict[str, dict] = {}
    if isinstance(value, dict):
        for req_id, raw in value.items():
            key = str(req_id)
            if isinstance(raw, dict):
                satisfied = _optional_bool(raw.get("satisfied"))
                evidence = str(raw.get("evidence") or raw.get("rationale") or "").strip()
            else:
                satisfied = _optional_bool(raw)
                evidence = ""
            normalized[key] = {
                "satisfied": satisfied,
                "evidence": evidence,
            }
    for req_id in hidden_requirements:
        normalized.setdefault(req_id, {"satisfied": None, "evidence": ""})
    return normalized


def _is_claude_family(model: str | None, agent: str | None = None) -> bool:
    text = _solver_family_text(model, agent)
    return any(name in text for name in ("claude", "opus", "sonnet", "haiku"))


def _is_gpt_family(model: str | None, agent: str | None = None) -> bool:
    text = _solver_family_text(model, agent)
    return any(name in text for name in ("gpt", "openai", "codex", "o1", "o3", "o4"))


def select_cross_family_judge_model(
    solver_model: str | None = None,
    solver_agent: str | None = None,
) -> str:
    """Choose a judge from a different model family when possible."""
    if _is_claude_family(solver_model, solver_agent):
        return DEFAULT_GPT_JUDGE_MODEL
    return DEFAULT_CLAUDE_JUDGE_MODEL

_PHASE5_JUDGE_SYSTEM_PROMPT = """You are an experienced Unreal Engine C++ reviewer auditing an automated test result.

Your job is to judge whether the model satisfies the requirement that one specific automated test is intended to check. The runner already knows whether the test passed or failed. You should still return a TP/TN/FP/FN classification for readability, but model_code_correct_for_test is the primitive judgment:
- true means the model satisfies the tested requirement.
- false means the model does not satisfy the tested requirement.

The runner will use the observed test outcome and model_code_correct_for_test to verify the classification:
- TP: the test failed and the model is wrong for the tested requirement.
- TN: the test passed and the model is correct for the tested requirement.
- FP: the test failed even though the model is correct for the tested requirement.
- FN: the test passed even though the model is wrong for the tested requirement.

You will be given the spec, the hidden requirements mapped to this test when available, the test source, the test outcome with assertion errors, the model's edited code, and the reference solution. Judge against the spec and tested requirement, not literal solution equivalence. The reference solution is one valid implementation, not the only valid implementation.

Additional guidance:
- Treat failures in task-owned integration paths as real failures when that integration path is itself part of the requirement.
- Do not excuse a failure just because another system was involved if that system behavior is exactly what the task was supposed to integrate with.

Do not mark FN merely because the model has unrelated defects elsewhere in the task. If this specific tested behavior is correct, set model_code_correct_for_test=true and use TN/FP according to the observed test outcome. Put unrelated or broader task defects in concerns instead. A separate full-task audit will decide whether the whole submission solves the task."""


_FULL_TASK_JUDGE_SYSTEM_PROMPT = """You are an experienced Unreal Engine C++ reviewer auditing whether a model submission fully solves an Unreal Engine task.

Your job is different from per-test TP/TN/FP/FN classification. You are judging the whole submitted implementation against:
- the task instruction,
- all evaluator-authored hidden requirements,
- the observed test results and per-test audit,
- the model's edited code,
- and the reference solution as one valid implementation.

The tests are evidence, not the whole task contract. A submission can pass every current test and still fail the full task if it misses a central requirement from the task description, hidden requirements, Unreal Engine semantics, or surrounding project code.

Be strict about task-failing omissions, but do not require literal reference-solution equivalence. Valid alternate implementations are acceptable if they satisfy the same behavior.

Audit method:
- Check every explicit hidden requirement independently, including requirements not directly covered by hidden tests.
- Do not claim a requirement is satisfied unless you can point to concrete evidence in the submitted code, such as a function, field, call path, state transition, authority check, replication hook, or Unreal lifecycle callback.
- If the reference solution uses a mechanism that appears necessary for the requirement, and the model omits that mechanism without an equivalent alternative, mark that explicit requirement as unsatisfied.
- Passing tests are not enough to mark an uncovered or shallowly covered requirement as satisfied.
- Do not state that a mechanism exists unless it is actually present in the submitted code.

Only list inferred_missing_requirements for blocking omissions: issues that mean a professional Unreal developer would not consider the task solved. Put non-failing caveats, style issues, or nice-to-have robustness notes in concerns instead.

If submission_satisfies_full_task is false, inferred_missing_requirements must contain at least one item. When the miss is an explicit hidden requirement, include its requirement_id and restate the requirement with concrete evidence. When the miss is broader than the explicit list, omit requirement_id and explain the missing professional requirement."""


_JUDGE_USER_TEMPLATE = """{evaluator_notes_block}# Behavioral spec (canonical contract)
{spec_instruction}

# Test under review
File: {test_file_name}
Test name: {test_id}
Outcome: {outcome}
{errors_block}

# Hidden requirements authored by the evaluator
{hidden_requirements_block}

# Hidden requirements this test is intended to cover
{covered_requirements_block}

# Test source
```cpp
{test_body}
```

{model_files_block}
{solution_files_block}
# Your job
Return strict JSON with these keys:
{{
  "classification": "TP" | "TN" | "FP" | "FN",
  "model_code_correct_for_test": true | false,
  "rationale": "1-3 sentences explaining the call",
  "concerns": "any caveats - empty string if none"
}}
"""


_FULL_TASK_JUDGE_USER_TEMPLATE = """{evaluator_notes_block}# Behavioral spec (canonical contract)
{spec_instruction}

# Hidden requirements authored by the evaluator
{hidden_requirements_block}

# Test-to-requirement mapping
{test_requirements_block}

# Requirement coverage summary
{requirement_coverage_block}

# Observed test results
{test_outcomes_block}

# Per-test audit summary
{per_test_audit_block}

# Full test source
File: {test_file_name}
```cpp
{test_file_body}
```

{model_files_block}
{solution_files_block}
# Your job
Return strict JSON with these keys:
{{
  "submission_satisfies_full_task": true | false,
  "explicit_requirement_satisfaction": {{
    "R1": {{
      "satisfied": true | false,
      "evidence": "concrete code evidence; name the relevant function, field, call path, state transition, or missing mechanism"
    }}
  }},
  "inferred_missing_requirements": [
    {{
      "requirement_id": "R1 if this is an explicit hidden requirement miss; omit or use empty string for broader inferred misses",
      "requirement": "blocking requirement missed by the model",
      "evidence": "concrete evidence from spec, Unreal semantics, project code, tests, or edited code",
      "source": "task_description" | "unreal_semantics" | "project_code" | "test_source" | "reference_solution"
    }}
  ],
  "rationale": "2-4 sentences explaining whether the whole submission solves the task",
  "concerns": "non-failing caveats - empty string if none"
}}
"""


_DENSE_JUDGE_SYSTEM_PROMPT = """You are an experienced Unreal Engine C++ reviewer performing a second-layer audit over a low-count, dense test suite.

Your job:
1. Judge whether the model's edited code is substantively CORRECT, BUGGY, or UNCLEAR against the SPEC.
2. Judge whether the overall test outcome appears APPROPRIATE, TOO_BRITTLE, TOO_FORGIVING, or UNCLEAR.

Important:
- The SPEC is authoritative.
- The reference solution is one valid implementation, not the only acceptable implementation.
- The tests are evidence, not ground truth.
- Dense tests may bundle many requirements, so do not pretend one failing runtime test pinpoints one exact bug unless the evidence actually does.
- Focus on concrete evidence from the code and test output; do not rely on stylistic similarity to the reference solution.
- Treat failures in task-owned integration paths as real failures when that integration path is itself part of the requirement.
- Do not excuse a failure just because another system was involved if that system behavior is exactly what the task was supposed to integrate with.
"""


_DENSE_JUDGE_USER_TEMPLATE = """{evaluator_notes_block}# Behavioral spec (canonical contract)
{spec_instruction}

# Overall test outcome
Task outcome: {task_outcome}
Tests run: {tests_run}
Tests passed: {tests_passed}
Tests failed: {tests_failed}

# Per-test outcomes
{test_outcomes_block}

# Full test source
File: {test_file_name}
```cpp
{test_file_body}
```

{model_files_block}
{solution_files_block}
# Your job
Return strict JSON with these keys:
{{
  "submission_verdict": "CORRECT" | "BUGGY" | "UNCLEAR",
  "test_verdict": "APPROPRIATE" | "TOO_BRITTLE" | "TOO_FORGIVING" | "UNCLEAR",
  "confidence": "high" | "medium" | "low",
  "rationale": "2-4 sentences explaining the call",
  "test_breakdown": [
    {{
      "test_id": "CDOChecks",
      "outcome": "PASSED" | "FAILED",
      "assessment": "APPROPRIATE" | "TOO_BRITTLE" | "TOO_FORGIVING" | "UNCLEAR",
      "notes": "1-2 short sentences"
    }}
  ],
  "requirement_findings": [
    {{
      "requirement": "short requirement name",
      "status": "MET" | "NOT_MET" | "UNCLEAR",
      "evidence": "1 short evidence sentence"
    }}
  ],
  "evidence": [
    "short concrete evidence item"
  ]
}}
"""


_COMPILE_FALLBACK_SYSTEM_PROMPT = """You are an experienced Unreal Engine C++ reviewer performing a fallback capability assessment when calibration code does not compile.

Your job: judge the model's edited code directly against the SPEC, using the compiler output only as debugging context.

Important:
- Do NOT simply equate "doesn't compile" with "incapable".
- A small compile mistake in one file may coexist with otherwise strong task understanding.
- The reference solution is only one valid implementation; judge against the SPEC, not exact solution equivalence.
- Treat failures in task-owned integration paths as real failures when that integration path is itself part of the requirement.
- Do not excuse a failure just because another system was involved if that system behavior is exactly what the task was supposed to integrate with.

For each edited file, classify it as:
- CORRECT: behavior appears substantially correct for the spec slice this file owns
- BUGGY: behavior is wrong, missing, or materially incomplete for the spec slice this file owns
- UNCLEAR: cannot confidently judge from the provided context

Then provide:
- the likely root cause of the compile failure
- an overall capability score from 0.0 to 1.0 representing how much of the attempted implementation appears substantively correct despite the compile failure
"""


_COMPILE_FALLBACK_USER_TEMPLATE = """{evaluator_notes_block}# Behavioral spec (canonical contract)
{spec_instruction}

# Calibration outcome
Compile status: FAILED

# Compiler output
```text
{compile_output}
```

{model_files_block}
{solution_files_block}
# Your job
Return strict JSON with these keys:
{{
  "compile_failure_root_cause": "1-2 sentences identifying the most likely compile blocker",
  "overall_capability_score": 0.0,
  "overall_assessment": "1-3 sentences on how strong the implementation appears aside from the compile error",
  "file_judgments": [
    {{
      "path": "relative/path.cpp",
      "classification": "CORRECT" | "BUGGY" | "UNCLEAR",
      "rationale": "1-2 sentences"
    }}
  ]
}}
"""


def _iter_balanced_objects(text: str) -> list[str]:
    """Return every top-level {...} substring, honoring string literals/escapes.

    Used to recover the JSON verdict from judge output that leads with reasoning
    prose and ```cpp code fences (whose braces would otherwise derail a naive
    first-brace scan).
    """
    objects: list[str] = []
    depth = 0
    start: Optional[int] = None
    in_str = False
    esc = False
    for i, c in enumerate(text):
        if esc:
            esc = False
            continue
        if in_str:
            if c == "\\":
                esc = True
            elif c == '"':
                in_str = False
            continue
        if c == '"':
            in_str = True
        elif c == "{":
            if depth == 0:
                start = i
            depth += 1
        elif c == "}":
            if depth > 0:
                depth -= 1
                if depth == 0 and start is not None:
                    objects.append(text[start:i + 1])
                    start = None
    return objects


def _parse_json_loose(text: str) -> Optional[dict]:
    text = text.strip()

    # Fast path: the whole response is the JSON object.
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        pass

    # Primary: the verdict lives in a fenced code block (the prompt asks for a
    # final ```json fence). Try each fence body, last first â€” the answer is the
    # trailing fence; earlier ```cpp fences won't parse and are skipped.
    fence_bodies = re.findall(r"```[A-Za-z0-9]*\s*\n?(.*?)```", text, re.DOTALL)
    for body in reversed(fence_bodies):
        try:
            return json.loads(body.strip())
        except json.JSONDecodeError:
            continue

    # Fallback: scan every balanced {...} block and return the LAST that parses,
    # covering responses that emit the object without a fence.
    for candidate in reversed(_iter_balanced_objects(text)):
        try:
            return json.loads(candidate)
        except json.JSONDecodeError:
            continue
    return None


_JSON_REPAIR_MODEL = "claude-haiku-4-5-20251001"


def _repair_json_with_small_model(broken: str, model: str = _JSON_REPAIR_MODEL) -> Optional[dict]:
    """Last-resort recovery: ask a small, cheap model to repair malformed JSON.

    Reached only after structural extraction fails â€” handles truncation, trailing
    commas, unescaped quotes, etc. that no structural parse can recover. Returns
    the repaired dict, or None if repair also fails (caller then handles it).
    """
    install_claude_code_sdk_compat()
    system = (
        "You fix malformed JSON. Output ONLY the corrected JSON object inside a "
        "```json code fence, with nothing else."
    )
    user = (
        "The text below was meant to be a single JSON object but does not parse. "
        "Return the same content as valid JSON, fixing only syntax (quotes, commas, "
        "escaping, truncation). Do not change keys or meaning:\n\n" + broken
    )

    async def _run() -> str:
        from claude_code_sdk import ClaudeCodeOptions, query

        base_kwargs = {
            "system_prompt": system,
            "permission_mode": "bypassPermissions",
            "cwd": str(REPO_ROOT),
            "model": model,
            "extra_args": {"tools": ""},
        }
        options = None
        for extra in ({"max_turns": 1}, {}):
            try:
                options = ClaudeCodeOptions(**base_kwargs, **extra)
                break
            except TypeError:
                continue
        if options is None:
            return ""

        async def _prompt_stream():
            yield {"type": "user", "message": {"role": "user", "content": user}}

        parts: list[str] = []
        result_text = ""
        with claude_sdk_auth_environment():
            async for message in query(prompt=_prompt_stream(), options=options):
                if hasattr(message, "content") and isinstance(message.content, list):
                    for block in message.content:
                        block_text = getattr(block, "text", None)
                        if block_text:
                            parts.append(block_text)
                if hasattr(message, "result") and message.result:
                    result_text = message.result
        return result_text or "\n".join(parts)

    try:
        repaired = asyncio.run(_run())
    except Exception:
        return None
    return _parse_json_loose(repaired)


def _ledger_add(ledger: object | None, **kwargs) -> None:
    if ledger is not None and hasattr(ledger, "add"):
        ledger.add(**kwargs)


def _call_anthropic_json(
    *,
    system: str,
    user: str,
    max_tokens: int,
    model: str,
    ledger: object | None,
    phase: str,
    role: str,
) -> dict:
    schema_instruction = (
        "\n\nYou may reason briefly first, but you MUST end your reply with the result "
        "as a single JSON object inside a ```json code fence, and write nothing after "
        "the closing fence."
    )
    install_claude_code_sdk_compat()

    async def _run_sdk_query(token_limit: int) -> tuple[str, int, int, Optional[str]]:
        from claude_code_sdk import ClaudeCodeOptions, query

        stderr_file = tempfile.NamedTemporaryFile(
            mode="w+",
            encoding="utf-8",
            suffix="_claude_judge_stderr.log",
            delete=False,
        )
        stderr_path = Path(stderr_file.name)

        base_options_kwargs = {
            "system_prompt": system + schema_instruction,
            "permission_mode": "bypassPermissions",
            "cwd": str(REPO_ROOT),
            "model": model,
            "extra_args": {"debug-to-stderr": None, "tools": ""},
            "debug_stderr": stderr_file,
        }
        optional_options = [
            {"max_turns": 1, "settings_sources": ["user"], "max_output_tokens": token_limit},
            {"max_turns": 1, "settings_sources": ["user"]},
            {"max_turns": 1},
            {},
        ]
        options = None
        for extra_kwargs in optional_options:
            try:
                options = ClaudeCodeOptions(**base_options_kwargs, **extra_kwargs)
                break
            except TypeError:
                continue
        if options is None:
            raise RuntimeError("Could not construct ClaudeCodeOptions for judge")

        result_text = ""
        assistant_parts: list[str] = []
        input_tokens = 0
        output_tokens = 0
        stop_reason: Optional[str] = None

        async def _stream_user_prompt():
            # Pass the prompt over stdin (stream-json input) rather than as a
            # command-line argument. The SDK puts string prompts on argv, which
            # overflows Windows' ~32KB command-line limit for large judge
            # prompts (WinError 206). A streaming iterable routes it to stdin.
            yield {"type": "user", "message": {"role": "user", "content": user}}

        try:
            with claude_sdk_auth_environment():
                async for message in query(prompt=_stream_user_prompt(), options=options):
                    usage = getattr(message, "usage", None)
                    if usage:
                        input_tokens = usage.get("input_tokens", input_tokens)
                        output_tokens = usage.get("output_tokens", output_tokens)

                    if hasattr(message, "content") and isinstance(message.content, list):
                        for block in message.content:
                            text = getattr(block, "text", None)
                            if text:
                                assistant_parts.append(text)

                    if hasattr(message, "result") and message.result:
                        result_text = message.result
                    if hasattr(message, "structured_output") and message.structured_output is not None:
                        result_text = json.dumps(message.structured_output)
                    if hasattr(message, "stop_reason") and message.stop_reason:
                        stop_reason = message.stop_reason
                    if hasattr(message, "usage") and message.usage:
                        input_tokens = message.usage.get("input_tokens", input_tokens)
                        output_tokens = message.usage.get("output_tokens", output_tokens)
        except Exception as exc:
            stderr_file.flush()
            stderr_tail = stderr_path.read_text(encoding="utf-8", errors="ignore")[-4000:]
            detail = f"{exc}"
            if stderr_tail.strip():
                detail += f"\nClaude CLI stderr tail:\n{stderr_tail}"
            raise RuntimeError(detail) from exc
        finally:
            stderr_file.close()

        if not result_text:
            result_text = "\n".join(part for part in assistant_parts if part)
        return result_text, input_tokens, output_tokens, stop_reason

    raw, in_toks, out_toks, stop_reason = asyncio.run(_run_sdk_query(max_tokens))
    _ledger_add(
        ledger,
        phase=phase,
        provider="claude",
        role=role,
        input_tokens=in_toks,
        output_tokens=out_toks,
    )
    parsed = _parse_json_loose(raw)

    # If truncated by max_tokens, retry once with a higher ceiling.
    if parsed is None and stop_reason in {"max_tokens", "max_output_tokens"}:
        retry_limit = max_tokens * 2
        raw, in_toks2, out_toks2, stop_reason = asyncio.run(_run_sdk_query(retry_limit))
        _ledger_add(
            ledger,
            phase=phase,
            provider="claude",
            role=role,
            input_tokens=in_toks2,
            output_tokens=out_toks2,
        )
        parsed = _parse_json_loose(raw)
        out_toks = out_toks2

    # Last resort: have a small, cheap model repair the malformed JSON syntax.
    if parsed is None:
        parsed = _repair_json_with_small_model(raw)

    if parsed is None:
        raise RuntimeError(
            f"Claude ({role}) returned unparseable JSON "
            f"(output_tokens={out_toks}). "
            f"First 300 chars: {raw[:300]}... Last 200 chars: ...{raw[-200:]}"
        )
    return parsed


def _call_codex_json(
    *,
    system: str,
    user: str,
    max_tokens: int,
    model: str,
    ledger: object | None,
    phase: str,
    role: str,
) -> dict:
    schema_instruction = (
        "\n\nYou may reason briefly first, but you MUST end your reply with the "
        "result as a single JSON object inside a ```json code fence, and write "
        "nothing after the closing fence."
    )
    prompt = (
        "You are acting as an offline JSON judge. Do not run shell commands, "
        "open files, or inspect the repository; use only the material included "
        "in this prompt.\n\n"
        + system
        + schema_instruction
        + "\n\n"
        + user
    )

    with tempfile.NamedTemporaryFile(
        mode="w+",
        encoding="utf-8",
        suffix="_codex_judge_last_message.txt",
        delete=False,
    ) as out_file:
        out_path = Path(out_file.name)

    cmd = [
        "codex",
        "exec",
        "--skip-git-repo-check",
        "--ephemeral",
        "--ignore-rules",
        "-s",
        "read-only",
        "-C",
        str(REPO_ROOT),
        "-m",
        model,
        "-c",
        f"model_reasoning_effort=\"{DEFAULT_GPT_JUDGE_REASONING_EFFORT}\"",
        "-o",
        str(out_path),
        "-",
    ]
    try:
        result = subprocess.run(
            cmd,
            input=prompt,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=900,
            cwd=str(REPO_ROOT),
        )
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError(f"Codex judge timed out for model {model}") from exc

    _ledger_add(
        ledger,
        phase=phase,
        provider="openai",
        role=role,
        input_tokens=0,
        output_tokens=0,
    )

    raw = ""
    if out_path.exists():
        raw = out_path.read_text(encoding="utf-8", errors="ignore")
    if not raw:
        raw = result.stdout or ""
    if result.returncode != 0 and not raw.strip():
        raise RuntimeError(
            f"Codex judge failed for model {model} "
            f"(exit={result.returncode}). stderr: {result.stderr[-2000:]}"
        )

    parsed = _parse_json_loose(raw)
    if parsed is None:
        parsed = _repair_json_with_small_model(raw)
    if parsed is None:
        raise RuntimeError(
            f"Codex ({role}) returned unparseable JSON. "
            f"First 300 chars: {raw[:300]}... Last 200 chars: ...{raw[-200:]}"
        )
    return parsed


def _call_judge_json(
    *,
    system: str,
    user: str,
    max_tokens: int,
    model: str,
    ledger: object | None,
    phase: str,
    role: str,
) -> dict:
    if _is_gpt_family(model):
        return _call_codex_json(
            system=system,
            user=user,
            max_tokens=max_tokens,
            model=model,
            ledger=ledger,
            phase=phase,
            role=role,
        )
    return _call_anthropic_json(
        system=system,
        user=user,
        max_tokens=max_tokens,
        model=model,
        ledger=ledger,
        phase=phase,
        role=role,
    )


def _load_audit_parser(audit_script: Path):
    spec = importlib.util.spec_from_file_location("ue_audit_module", audit_script)
    mod = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(mod)
    return mod._parse_automation_results


def _read_result_record(snapshot_dir: Path) -> dict:
    result_path = snapshot_dir / "result.json"
    if not result_path.exists():
        return {}
    try:
        parsed = json.loads(result_path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError):
        return {}
    if isinstance(parsed, list):
        return parsed[0] if parsed else {}
    if isinstance(parsed, dict):
        return parsed
    return {}


def _read_files_for_judge(root: Path, files_to_edit: Sequence[str]) -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    for rel in files_to_edit:
        path = root / rel
        if not path.exists():
            continue
        try:
            content = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        out.append((rel, content))
    return out


def _format_files_block(label: str, files: list[tuple[str, str]]) -> str:
    parts = [f"# {label}\n"]
    for rel, content in files:
        parts.append(f"## {rel}\n```cpp\n{content}\n```\n")
    return "\n".join(parts)


def _format_dense_test_outcomes(parsed: Sequence[dict]) -> str:
    lines: list[str] = []
    for tr in parsed:
        status = "PASSED" if tr.get("passed") else "FAILED"
        lines.append(f"- {tr.get('id', '(unknown)')}: {status}")
        for err in (tr.get("errors") or [])[:3]:
            lines.append(f"  - {err}")
    return "\n".join(lines) if lines else "(no parsed test outcomes)"


def _select_judge_mode(parsed: Sequence[dict]) -> str:
    # UE tasks always use atomic mode: even 2-test suites like CDOChecks +
    # RuntimeBehaviors bundle many behavioral phases per test, so a per-test
    # TP/FP/TN/FN classification is more useful than a holistic dense verdict.
    return "atomic"


def _derive_dense_confusion_category(task_passed: Optional[bool], submission_verdict: str) -> str:
    if task_passed is None:
        return "UNCERTAIN"
    if task_passed:
        if submission_verdict == "CORRECT":
            return "TN"
        if submission_verdict == "BUGGY":
            return "FN"
    else:
        if submission_verdict == "BUGGY":
            return "TP"
        if submission_verdict == "CORRECT":
            return "FP"
    return "UNCERTAIN"


def _normalize_dense_test_breakdown(test_breakdown: object, parsed: Sequence[dict]) -> list[dict]:
    normalized: list[dict] = []
    if isinstance(test_breakdown, list):
        for item in test_breakdown:
            if not isinstance(item, dict):
                continue
            outcome = str(item.get("outcome", "FAILED")).upper().strip()
            if outcome not in ("PASSED", "FAILED"):
                outcome = "FAILED"
            assessment = str(item.get("assessment", "UNCLEAR")).upper().strip()
            if assessment not in ("APPROPRIATE", "TOO_BRITTLE", "TOO_FORGIVING", "UNCLEAR"):
                assessment = "UNCLEAR"
            normalized.append(
                {
                    "test_id": str(item.get("test_id", "")),
                    "outcome": outcome,
                    "assessment": assessment,
                    "notes": str(item.get("notes", "")),
                }
            )
    if normalized:
        return normalized
    return [
        {
            "test_id": str(tr.get("id", "")),
            "outcome": "PASSED" if tr.get("passed") else "FAILED",
            "assessment": "UNCLEAR",
            "notes": "",
        }
        for tr in parsed
    ]


def _normalize_requirement_findings(requirement_findings: object) -> list[dict]:
    normalized: list[dict] = []
    if not isinstance(requirement_findings, list):
        return normalized
    for item in requirement_findings:
        if not isinstance(item, dict):
            continue
        status = str(item.get("status", "UNCLEAR")).upper().strip()
        if status not in ("MET", "NOT_MET", "UNCLEAR"):
            status = "UNCLEAR"
        normalized.append(
            {
                "requirement": str(item.get("requirement", "")),
                "status": status,
                "evidence": str(item.get("evidence", "")),
            }
        )
    return normalized


def _extract_test_body(test_id: str, test_file_path: Path) -> Optional[str]:
    text = test_file_path.read_text(encoding="utf-8", errors="ignore")
    matches = list(_TEST_DECL_RE.finditer(text))
    for i, match in enumerate(matches):
        if match.group(2).endswith(test_id):
            start = match.start()
            end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
            return text[start:end].rstrip() + "\n"
    return None


def _merge_judge_block(snapshot_dir: Path, judge_block: dict) -> Path:
    result_path = snapshot_dir / "result.json"
    if result_path.exists():
        try:
            existing = json.loads(result_path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            existing = {}
        if isinstance(existing, list):
            if existing:
                existing[0]["judge"] = judge_block
            else:
                existing = [{"judge": judge_block}]
            result_path.write_text(json.dumps(existing, indent=2) + "\n", encoding="utf-8")
        else:
            existing["judge"] = judge_block
            result_path.write_text(json.dumps(existing, indent=2) + "\n", encoding="utf-8")
    return result_path


def normalize_persisted_judge(judge_block: Optional[dict]) -> dict:
    if not judge_block:
        return {"skipped": True, "reason": "no_judge_result"}
    if judge_block.get("mode") == "compile_fallback":
        return {
            "skipped": False,
            "mode": "compile_fallback",
            "counts": judge_block.get("counts", {"CORRECT": 0, "BUGGY": 0, "UNCLEAR": 0}),
            "capability_score": judge_block.get("overall_capability_score", 0.0),
            "compile_failure_root_cause": judge_block.get("compile_failure_root_cause", ""),
            "overall_assessment": judge_block.get("overall_assessment", ""),
            "file_judgments": judge_block.get("file_judgments", []),
            "persisted_judge": judge_block,
        }
    if judge_block.get("mode") == "dense":
        return {
            "skipped": False,
            "mode": "dense",
            "judge_mode": "dense",
            "submission_verdict": judge_block.get("submission_verdict", "UNCLEAR"),
            "test_verdict": judge_block.get("test_verdict", "UNCLEAR"),
            "confidence": judge_block.get("confidence", "low"),
            "confusion_category": judge_block.get("confusion_category", "UNCERTAIN"),
            "rationale": judge_block.get("rationale", ""),
            "test_breakdown": judge_block.get("test_breakdown", []),
            "requirement_findings": judge_block.get("requirement_findings", []),
            "evidence": judge_block.get("evidence", []),
            "persisted_judge": judge_block,
        }
    full_task_audit = judge_block.get("full_task_audit", {})
    if not isinstance(full_task_audit, dict):
        full_task_audit = {}
    inferred_missing_requirements = judge_block.get("inferred_missing_requirements")
    if inferred_missing_requirements is None:
        inferred_missing_requirements = full_task_audit.get("inferred_missing_requirements", [])
    return {
        "skipped": False,
        "mode": "atomic",
        "judge_mode": "atomic",
        "classifications": judge_block.get("classifications", []),
        "counts": judge_block.get("counts", {"TP": 0, "TN": 0, "FP": 0, "FN": 0}),
        "trust_score": judge_block.get("trust_score", 0.0),
        "hidden_requirements": judge_block.get("hidden_requirements", {}),
        "test_requirements": judge_block.get("test_requirements", {}),
        "per_test_audit": judge_block.get("per_test_audit", judge_block.get("classifications", [])),
        "full_task_audit": full_task_audit,
        "submission_satisfies_full_task": judge_block.get("submission_satisfies_full_task"),
        "inferred_missing_requirements": inferred_missing_requirements or [],
        "persisted_judge": judge_block,
    }


def read_result_record(snapshot_dir: Path) -> dict:
    return _read_result_record(snapshot_dir)


def extract_test_body(test_id: str, test_file_path: Path) -> Optional[str]:
    return _extract_test_body(test_id, test_file_path)


def run_unreal_judge(
    *,
    snapshot_dir: Path,
    task_instruction: str,
    files_to_edit: Sequence[str],
    solution_dir: Path,
    test_file: Optional[Path],
    hidden_requirements: object | None = None,
    test_requirements: object | None = None,
    evaluator_notes: str = "",
    audit_script: Optional[Path] = None,
    ledger: object | None = None,
    phase: str = "phase5_5_judge",
    judge_model: str = DEFAULT_CLAUDE_JUDGE_MODEL,
) -> dict:
    if not snapshot_dir.exists():
        return {"skipped": True, "reason": "no_snapshot"}
    if not files_to_edit:
        return {"skipped": True, "reason": "no_files_to_edit"}

    model_files = _read_files_for_judge(snapshot_dir, files_to_edit)
    solution_files = _read_files_for_judge(solution_dir, files_to_edit)
    if not model_files:
        return {"skipped": True, "reason": "no_model_files"}

    normalized_hidden_requirements = _normalize_hidden_requirements(hidden_requirements)

    result_record = _read_result_record(snapshot_dir)
    compile_output_path = snapshot_dir / "compile_output.txt"
    compile_output = ""
    if compile_output_path.exists():
        compile_output = compile_output_path.read_text(encoding="utf-8", errors="ignore")

    parsed: list[dict] = []
    unreal_log = snapshot_dir / "unreal_log.txt"
    if unreal_log.exists() and unreal_log.stat().st_size > 0:
        parser = _load_audit_parser(audit_script or DEFAULT_AUDIT_SCRIPT)
        log_text = unreal_log.read_text(encoding="utf-8", errors="ignore")
        parsed = parser(log_text)

    evaluator_block = ""
    if evaluator_notes:
        evaluator_block = (
            "# Private evaluator notes (oracle â€” do not copy into rationale)\n"
            f"{evaluator_notes}\n\n"
        )

    if not parsed:
        user_msg = _COMPILE_FALLBACK_USER_TEMPLATE.format(
            evaluator_notes_block=evaluator_block,
            spec_instruction=task_instruction or "(none)",
            compile_output=compile_output or "(no compile output captured)",
            model_files_block=_format_files_block("Model's implementation", model_files),
            solution_files_block=_format_files_block("Reference solution", solution_files),
        )
        result = _call_judge_json(
            system=_COMPILE_FALLBACK_SYSTEM_PROMPT,
            user=user_msg,
            max_tokens=4096,
            model=judge_model,
            ledger=ledger,
            phase=phase,
            role="judge",
        )

        file_judgments = result.get("file_judgments") or []
        counts = {"CORRECT": 0, "BUGGY": 0, "UNCLEAR": 0}
        judged = 0
        correct = 0
        for file_judgment in file_judgments:
            cls = str(file_judgment.get("classification", "UNCLEAR")).upper().strip()
            if cls not in counts:
                cls = "UNCLEAR"
            counts[cls] += 1
            judged += 1
            if cls == "CORRECT":
                correct += 1
        capability_score = result.get("overall_capability_score")
        if not isinstance(capability_score, (int, float)):
            capability_score = (correct / judged) if judged else 0.0

        judge_block = {
            "model": judge_model,
            "mode": "compile_fallback",
            "counts": counts,
            "compile_failure_root_cause": result.get("compile_failure_root_cause", ""),
            "overall_capability_score": capability_score,
            "overall_assessment": result.get("overall_assessment", ""),
            "file_judgments": file_judgments,
            "runner_failure_reason": result_record.get("failure_reason"),
            "compile_success": result_record.get("compile_success"),
        }
        result_path = _merge_judge_block(snapshot_dir, judge_block)
        normalized = normalize_persisted_judge(judge_block)
        normalized["report_path"] = result_path
        return normalized

    if test_file is None or not test_file.exists():
        return {"skipped": True, "reason": "no_test_file"}

    test_results = result_record.get("test_results") or []
    judge_mode = _select_judge_mode(parsed)

    if judge_mode == "dense":
        test_file_body = test_file.read_text(encoding="utf-8", errors="ignore")
        user_msg = _DENSE_JUDGE_USER_TEMPLATE.format(
            evaluator_notes_block=evaluator_block,
            spec_instruction=task_instruction or "(none)",
            task_outcome="PASSED" if result_record.get("task_passed") else "FAILED",
            tests_run=result_record.get("tests_run", len(parsed)),
            tests_passed=result_record.get("tests_passed", sum(1 for t in parsed if t.get("passed"))),
            tests_failed=result_record.get("tests_failed", sum(1 for t in parsed if not t.get("passed"))),
            test_outcomes_block=_format_dense_test_outcomes(parsed),
            test_file_name=test_file.name,
            test_file_body=test_file_body,
            model_files_block=_format_files_block("Model's implementation", model_files),
            solution_files_block=_format_files_block("Reference solution", solution_files),
        )
        result = _call_judge_json(
            system=_DENSE_JUDGE_SYSTEM_PROMPT,
            user=user_msg,
            max_tokens=1536,
            model=judge_model,
            ledger=ledger,
            phase=phase,
            role="judge",
        )

        submission_verdict = str(result.get("submission_verdict", "UNCLEAR")).upper().strip()
        if submission_verdict not in ("CORRECT", "BUGGY", "UNCLEAR"):
            submission_verdict = "UNCLEAR"
        test_verdict = str(result.get("test_verdict", "UNCLEAR")).upper().strip()
        if test_verdict not in ("APPROPRIATE", "TOO_BRITTLE", "TOO_FORGIVING", "UNCLEAR"):
            test_verdict = "UNCLEAR"
        confidence = str(result.get("confidence", "low")).lower().strip()
        if confidence not in ("high", "medium", "low"):
            confidence = "low"
        test_breakdown = _normalize_dense_test_breakdown(result.get("test_breakdown"), parsed)
        requirement_findings = _normalize_requirement_findings(result.get("requirement_findings"))

        judge_block = {
            "model": judge_model,
            "mode": "dense",
            "submission_verdict": submission_verdict,
            "test_verdict": test_verdict,
            "confidence": confidence,
            "confusion_category": _derive_dense_confusion_category(
                result_record.get("task_passed"),
                submission_verdict,
            ),
            "rationale": result.get("rationale", ""),
            "test_breakdown": test_breakdown,
            "requirement_findings": requirement_findings,
            "evidence": result.get("evidence") or [],
            "tests_run": result_record.get("tests_run", len(parsed)),
            "tests_passed": result_record.get("tests_passed", sum(1 for t in parsed if t.get("passed"))),
            "tests_failed": result_record.get("tests_failed", sum(1 for t in parsed if not t.get("passed"))),
        }
        result_path = _merge_judge_block(snapshot_dir, judge_block)
        normalized = normalize_persisted_judge(judge_block)
        normalized["report_path"] = result_path
        return normalized

    test_filter = ""
    if test_results:
        first_id = test_results[0].get("id", "")
        if "." in first_id:
            test_filter = first_id.rsplit(".", 1)[0]

    classifications: list[dict] = []
    for tr in parsed:
        full_id = tr["id"]
        short_id = full_id[len(test_filter) + 1:] if test_filter and full_id.startswith(test_filter + ".") else full_id
        covered_requirement_ids = _requirement_ids_for_test(
            test_requirements,
            full_id=full_id,
            short_id=short_id,
        )
        errors = tr.get("errors") or []
        errors_block = ""
        if errors:
            errors_block = "Assertion errors:\n" + "\n".join(f"  - {e}" for e in errors[:10])

        user_msg = _JUDGE_USER_TEMPLATE.format(
            evaluator_notes_block=evaluator_block,
            spec_instruction=task_instruction or "(none)",
            test_file_name=test_file.name,
            test_id=short_id,
            outcome="PASSED" if tr.get("passed") else "FAILED",
            errors_block=errors_block,
            hidden_requirements_block=_format_hidden_requirements_block(normalized_hidden_requirements),
            covered_requirements_block=_format_covered_requirements_block(
                covered_requirement_ids,
                normalized_hidden_requirements,
            ),
            test_body=_extract_test_body(short_id, test_file) or "(could not extract test body)",
            model_files_block=_format_files_block("Model's implementation", model_files),
            solution_files_block=_format_files_block("Reference solution", solution_files),
        )
        result = _call_judge_json(
            system=_PHASE5_JUDGE_SYSTEM_PROMPT,
            user=user_msg,
            max_tokens=1024,
            model=judge_model,
            ledger=ledger,
            phase=phase,
            role="judge",
        )
        cls = str(result.get("classification", "ERROR")).upper().strip()
        if cls not in ("TP", "TN", "FP", "FN"):
            cls = "ERROR"
        model_code_correct_for_test = _optional_bool(result.get("model_code_correct_for_test"))
        if model_code_correct_for_test is None:
            model_code_correct_for_test = _optional_bool(result.get("model_code_correct"))
        if model_code_correct_for_test is None and cls in ("TP", "TN", "FP", "FN"):
            model_code_correct_for_test = cls in ("TN", "FP")
        if model_code_correct_for_test is not None:
            if tr.get("passed"):
                cls = "TN" if model_code_correct_for_test else "FN"
            else:
                cls = "FP" if model_code_correct_for_test else "TP"

        classifications.append(
            {
                "test_id": short_id,
                "outcome": "PASSED" if tr.get("passed") else "FAILED",
                "classification": cls,
                "covered_requirement_ids": covered_requirement_ids,
                "model_code_correct_for_test": model_code_correct_for_test,
                "rationale": result.get("rationale", ""),
                "concerns": result.get("concerns", ""),
            }
        )

    counts = {"TP": 0, "TN": 0, "FP": 0, "FN": 0}
    for classification in classifications:
        if classification["classification"] in counts:
            counts[classification["classification"]] += 1
    judged = sum(counts.values())
    trust_score = ((counts["TP"] + counts["TN"]) / judged) if judged else 0.0

    full_task_user_msg = _FULL_TASK_JUDGE_USER_TEMPLATE.format(
        evaluator_notes_block=evaluator_block,
        spec_instruction=task_instruction or "(none)",
        hidden_requirements_block=_format_hidden_requirements_block(normalized_hidden_requirements),
        test_requirements_block=_format_test_requirements_block(test_requirements),
        requirement_coverage_block=_format_requirement_coverage_block(
            normalized_hidden_requirements,
            test_requirements,
        ),
        test_outcomes_block=_format_test_outcomes_block(parsed),
        per_test_audit_block=_format_per_test_audit_block(classifications),
        test_file_name=test_file.name,
        test_file_body=test_file.read_text(encoding="utf-8", errors="ignore"),
        model_files_block=_format_files_block("Model's implementation", model_files),
        solution_files_block=_format_files_block("Reference solution", solution_files),
    )
    full_task_raw = _call_judge_json(
        system=_FULL_TASK_JUDGE_SYSTEM_PROMPT,
        user=full_task_user_msg,
        max_tokens=2048,
        model=judge_model,
        ledger=ledger,
        phase=phase,
        role="judge",
    )
    explicit_requirement_satisfaction = _normalize_explicit_requirement_satisfaction(
        full_task_raw.get("explicit_requirement_satisfaction"),
        normalized_hidden_requirements,
    )
    inferred_missing_requirements = _normalize_inferred_missing_requirements(
        full_task_raw.get("inferred_missing_requirements")
    )
    full_task_rationale = str(full_task_raw.get("rationale") or "")

    full_task_satisfies_explicitly = _optional_bool(
        full_task_raw.get("submission_satisfies_full_task")
    )
    per_test_has_failure = any(
        c.get("model_code_correct_for_test") is False for c in classifications
    )
    per_test_has_unknown = any(
        c.get("model_code_correct_for_test") is None for c in classifications
    )
    explicit_has_failure = any(
        req.get("satisfied") is False for req in explicit_requirement_satisfaction.values()
    )
    explicit_has_unknown = any(
        req.get("satisfied") is None for req in explicit_requirement_satisfaction.values()
    )

    if (
        per_test_has_failure
        or full_task_satisfies_explicitly is False
        or explicit_has_failure
        or inferred_missing_requirements
    ):
        submission_satisfies_full_task = False
    elif per_test_has_unknown or full_task_satisfies_explicitly is None or explicit_has_unknown:
        submission_satisfies_full_task = None
    else:
        submission_satisfies_full_task = True

    if submission_satisfies_full_task is False:
        inferred_missing_requirements = _ensure_full_task_failure_has_missing_requirement(
            inferred_missing_requirements,
            full_task_satisfies_explicitly=full_task_satisfies_explicitly,
            explicit_requirement_satisfaction=explicit_requirement_satisfaction,
            hidden_requirements=normalized_hidden_requirements,
            classifications=classifications,
            rationale=full_task_rationale,
        )
    for idx, inferred in enumerate(inferred_missing_requirements, start=1):
        inferred["id"] = f"I{idx}"

    full_task_audit = {
        "submission_satisfies_full_task": full_task_satisfies_explicitly,
        "explicit_requirement_satisfaction": explicit_requirement_satisfaction,
        "inferred_missing_requirements": inferred_missing_requirements,
        "rationale": full_task_rationale,
        "concerns": full_task_raw.get("concerns", ""),
    }

    judge_block = {
        "model": judge_model,
        "mode": "atomic",
        "counts": counts,
        "trust_score": trust_score,
        "hidden_requirements": normalized_hidden_requirements,
        "test_requirements": test_requirements if isinstance(test_requirements, dict) else {},
        "per_test_audit": classifications,
        "full_task_audit": full_task_audit,
        "submission_satisfies_full_task": submission_satisfies_full_task,
        "classifications": classifications,
    }
    result_path = _merge_judge_block(snapshot_dir, judge_block)
    normalized = normalize_persisted_judge(judge_block)
    normalized["report_path"] = result_path
    return normalized

