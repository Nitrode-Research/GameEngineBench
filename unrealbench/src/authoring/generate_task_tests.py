#!/usr/bin/env python3
"""Automated test generation + calibration for Unreal benchmark tasks.

Takes a pre-scaffolded task directory (with start/, solution/, spec.yaml) and:
  Phase 1 — validate inputs (schema, compile, API keys)
  Phase 2 — read solution source, produce API Surface summary
  Phase 3 — multi-model test generation (Claude → GPT → Gemini → Claude)
  Phase 4a — solution-pass loop (patch until solution tests pass; 3-strikes bail)
  Phase 4b — start-fail tightening loop (tighten tests that trivially pass; cap 3)
  Phase 5 — calibration run (ue_benchmark_runner with frontier model)
  Phase 6 — structured summary + LLM query report

Usage:
    python -m unrealbench.src.authoring.generate_task_tests \
        --task-id ue_task_0020_inventory_system \
        [--calibration-model claude-sonnet-4-6] \
        [--dry-run]

The human scaffolds the task directory (extraction, Target.cs patches, spec.yaml)
before invoking this script. Query count and per-phase breakdown are reported
at the end.
"""

from __future__ import annotations

import argparse
import asyncio
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, cast

from unrealbench.src.claude_code_sdk_compat import install as install_claude_code_sdk_compat
try:
    import yaml
except ImportError:
    print("ERROR: PyYAML not installed. Run: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

# Load .env if present — matches multi_model.py convention
try:
    from dotenv import load_dotenv
    load_dotenv()
except ImportError:
    pass

from unrealbench.src.ue_judge import (
    extract_test_body,
    normalize_persisted_judge,
    read_result_record,
    run_unreal_judge,
)

REPO_ROOT = Path(__file__).resolve().parents[3]
TASKS_DIR = REPO_ROOT / "tasks_unreal"
AUDIT_SCRIPT = TASKS_DIR / "audit.py"
CACHE_ROOT = Path.home() / ".cache" / "unrealbench" / "import_log"

# Approximate per-1M-token prices in USD as of 2026-Q1.
# Used for cost estimation only; exact invoicing varies by provider.
_PRICE_PER_1M_INPUT = {
    "claude": 3.00,       # Anthropic claude-opus-4-6 / sonnet tier
    "openai": 2.50,       # OpenAI gpt-4o
    "gemini": 1.25,       # Google gemini-1.5-pro
}
_PRICE_PER_1M_OUTPUT = {
    "claude": 15.00,
    "openai": 10.00,
    "gemini": 5.00,
}


# ── Query ledger ──────────────────────────────────────────────────────────────

@dataclass
class QueryRecord:
    phase: str              # "phase3_draft", "phase4a", etc.
    provider: str           # "claude" | "openai" | "gemini"
    input_tokens: int = 0
    output_tokens: int = 0
    role: str = ""          # "drafter" | "judge" | "improver" | "editor" | "patcher"


@dataclass
class QueryLedger:
    records: list[QueryRecord] = field(default_factory=list)

    def add(self, **kwargs) -> None:
        self.records.append(QueryRecord(**kwargs))

    def total_calls(self) -> int:
        return len(self.records)

    def count_by_phase(self) -> dict[str, int]:
        counts: dict[str, int] = {}
        for r in self.records:
            counts[r.phase] = counts.get(r.phase, 0) + 1
        return counts

    def count_by_phase_and_provider(self) -> dict[tuple[str, str], int]:
        counts: dict[tuple[str, str], int] = {}
        for r in self.records:
            key = (r.phase, r.provider)
            counts[key] = counts.get(key, 0) + 1
        return counts

    def estimated_cost_usd(self) -> float:
        total = 0.0
        for r in self.records:
            pin = _PRICE_PER_1M_INPUT.get(r.provider, 0) / 1_000_000
            pout = _PRICE_PER_1M_OUTPUT.get(r.provider, 0) / 1_000_000
            total += r.input_tokens * pin + r.output_tokens * pout
        return total


# ── Phase 1: Validation ───────────────────────────────────────────────────────

@dataclass
class TaskContext:
    task_id: str
    task_dir: Path
    spec: dict
    log_dir: Path
    evaluator_notes: str = ""  # contents of --evaluator-notes file, if supplied

    @property
    def start_dir(self) -> Path:
        return self.task_dir / "start"

    @property
    def solution_dir(self) -> Path:
        return self.task_dir / "solution"

    @property
    def tests_dir(self) -> Path:
        return self.task_dir / "tests"

    @property
    def project_name(self) -> Optional[str]:
        """Derived from the .uproject file in solution/."""
        uprojects = list(self.solution_dir.rglob("*.uproject"))
        return uprojects[0].stem if uprojects else None


@dataclass
class AuthoringConfig:
    mode: str = "llm_json"   # llm_json | cli_agent
    agent: str = "codex"     # codex | claude-code
    model: Optional[str] = None


def _phase_header(title: str) -> None:
    print()
    print("═" * 70)
    print(title)
    print("═" * 70)


def _resolve_task_dir(task_ref: str) -> tuple[str, Path]:
    """Resolve a task reference to exactly one task directory.

    Accepts either the full task directory name or an unambiguous prefix such
    as ``ue_task_0001``.
    """
    exact = TASKS_DIR / task_ref
    if exact.exists() and exact.is_dir():
        return task_ref, exact

    matches = sorted(
        p for p in TASKS_DIR.glob(f"{task_ref}*")
        if p.is_dir()
    )
    if not matches:
        sys.exit(f"  [FAIL] Task not found: {task_ref}")
    if len(matches) > 1:
        names = "\n".join(f"    - {p.name}" for p in matches)
        sys.exit(
            f"  [FAIL] Task reference '{task_ref}' is ambiguous. Matches:\n{names}"
        )
    return matches[0].name, matches[0]


def _run_audit_subprocess(
    args: list[str], capture: bool = True, timeout: Optional[int] = None
) -> subprocess.CompletedProcess:
    """Invoke tasks_unreal/audit.py with the given args. Returns the CompletedProcess."""
    cmd = [sys.executable, str(AUDIT_SCRIPT), *args]
    return subprocess.run(
        cmd,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
        text=True,
        timeout=timeout,
    )


def _run_audit_subprocess_with_heartbeat(
    args: list[str], timeout: Optional[int] = None, heartbeat_seconds: float = 10.0
) -> subprocess.CompletedProcess:
    """Invoke audit.py while printing elapsed-time heartbeats for long-running calls."""
    cmd = [sys.executable, str(AUDIT_SCRIPT), *args]
    with tempfile.NamedTemporaryFile(mode="w+", encoding="utf-8", delete=False) as tmp:
        tmp_path = Path(tmp.name)
        proc = subprocess.Popen(
            cmd,
            stdout=tmp,
            stderr=tmp,
            text=True,
        )

    t0 = time.time()
    next_heartbeat = t0 + heartbeat_seconds
    try:
        while True:
            ret = proc.poll()
            now = time.time()
            if ret is not None:
                break
            if now >= next_heartbeat:
                print(f"…{int(now - t0)}s", end="", flush=True)
                next_heartbeat = now + heartbeat_seconds
            if timeout is not None and now - t0 > timeout:
                proc.kill()
                proc.wait()
                raise subprocess.TimeoutExpired(cmd, timeout)
            time.sleep(0.5)

        stdout = tmp_path.read_text(encoding="utf-8", errors="ignore")
        return subprocess.CompletedProcess(
            args=cmd,
            returncode=proc.returncode,
            stdout=stdout,
            stderr=None,
        )
    finally:
        try:
            tmp_path.unlink(missing_ok=True)
        except OSError:
            pass


def phase1_validate(task_id: str, evaluator_notes_path: Optional[Path] = None) -> TaskContext:
    resolved_task_id, task_dir = _resolve_task_dir(task_id)
    _phase_header(f"Phase 1 — Validate inputs for {resolved_task_id}")
    if resolved_task_id != task_id:
        print(f"  Resolved task reference: {task_id} -> {resolved_task_id}")

    if not task_dir.exists():
        sys.exit(f"  [FAIL] Task directory not found: {task_dir}")

    for sub in ("start", "solution", "tests"):
        if not (task_dir / sub).exists():
            sys.exit(f"  [FAIL] Missing {sub}/ in task directory")

    spec_path = task_dir / "spec.yaml"
    if not spec_path.exists():
        sys.exit(f"  [FAIL] Missing spec.yaml")

    with spec_path.open() as f:
        spec = yaml.safe_load(f)

    print(f"  ✓ Task directory present: {task_dir.relative_to(REPO_ROOT)}")
    print(f"  ✓ spec.yaml loaded (title={spec.get('title', '?')})")

    # Schema check via audit.py
    print("  Running audit.py --schema...")
    result = _run_audit_subprocess(["--schema", "--task-id", resolved_task_id])
    if result.returncode != 0:
        print(result.stdout)
        sys.exit("  [FAIL] Schema audit failed; fix spec.yaml and retry")
    print("  ✓ spec.yaml valid per schema")

    # API keys. Google accepts GOOGLE_API_KEY or GEMINI_API_KEY (multi_model.py convention).
    have_anthropic = bool(os.environ.get("ANTHROPIC_API_KEY"))
    have_openai = bool(os.environ.get("OPENAI_API_KEY"))
    have_google = bool(os.environ.get("GOOGLE_API_KEY") or os.environ.get("GEMINI_API_KEY"))
    missing: list[str] = []
    if not have_anthropic: missing.append("ANTHROPIC_API_KEY")
    if not have_openai:    missing.append("OPENAI_API_KEY")
    if not have_google:    missing.append("GOOGLE_API_KEY (or GEMINI_API_KEY)")
    if missing:
        sys.exit(f"  [FAIL] Missing API keys for multi-model loop: {', '.join(missing)}")
    print("  ✓ API keys present (anthropic, openai, google)")

    # Compile check via audit.py (both start/ and solution/)
    print("  Running audit.py --compile (solution/ only — start/ verification is user's responsibility)... ", end="", flush=True)
    t0 = time.time()
    result = _run_audit_subprocess_with_heartbeat(["--compile", "--task-id", resolved_task_id], timeout=900)
    compile_elapsed = time.time() - t0
    if result.returncode != 0:
        print()
        # Show compile tail for debugging
        for line in (result.stdout or "").strip().splitlines()[-15:]:
            print(f"    {line}")
        sys.exit(f"  [FAIL] solution/ doesn't compile — fix before running this script")
    print(f"✓ solution/ compiles ({compile_elapsed:.1f}s)")

    log_dir = CACHE_ROOT / resolved_task_id
    log_dir.mkdir(parents=True, exist_ok=True)
    print(f"  ✓ Log directory: {log_dir}  (outside repo)")

    # Evaluator notes — optional, loaded into memory only, never persisted.
    # Intentionally kept outside the unrealbench repo; author passes an
    # absolute path so the file never gets committed or copied to the workspace.
    evaluator_notes = ""
    if evaluator_notes_path is not None:
        if not evaluator_notes_path.exists():
            sys.exit(f"  [FAIL] --evaluator-notes path not found: {evaluator_notes_path}")
        evaluator_notes = evaluator_notes_path.read_text(encoding="utf-8")
        print(f"  ✓ Evaluator notes loaded: {evaluator_notes_path}  ({len(evaluator_notes)} chars, in-memory only)")

    return TaskContext(
        task_id=resolved_task_id, task_dir=task_dir, spec=spec, log_dir=log_dir,
        evaluator_notes=evaluator_notes,
    )


# ── Phase 2: API discovery ────────────────────────────────────────────────────

_RE_UCLASS_DECL = re.compile(r"UCLASS\([^)]*\)\s*\n\s*class\s+(?:\w+_API\s+)?(\w+)\s*:\s*public\s+(\w+)")
_RE_USTRUCT_DECL = re.compile(r"USTRUCT\([^)]*\)\s*\n\s*struct\s+(?:\w+_API\s+)?(\w+)")
_RE_UFUNCTION = re.compile(
    r"UFUNCTION\(([^)]*)\)\s*\n\s*(?:virtual\s+)?([^;{]+?)(?=;|\{|/\*)",
    re.DOTALL,
)
_RE_UPROPERTY = re.compile(
    r"UPROPERTY\(([^)]*)\)\s*\n\s*([^;]+);",
    re.DOTALL,
)
# Catch the leading guard clauses of a cpp method to surface preconditions.
_RE_METHOD_BODY = re.compile(
    r"^(?:\w+\s+)+(?:\w+::)(\w+)\([^)]*\)[^{]*\{\s*\n((?:[^{}]|\{[^{}]*\})*?)\}",
    re.MULTILINE | re.DOTALL,
)


def _summarize_header(header_path: Path) -> str:
    """Extract classes, UFUNCTIONs, UPROPERTYs from a UE header."""
    if not header_path.exists():
        return f"_(header not found: {header_path.name})_\n"
    text = header_path.read_text(encoding="utf-8", errors="ignore")
    out: list[str] = []

    for m in _RE_UCLASS_DECL.finditer(text):
        cls, base = m.group(1), m.group(2)
        out.append(f"**class `{cls}` : public `{base}`**")

    for m in _RE_USTRUCT_DECL.finditer(text):
        out.append(f"**struct `{m.group(1)}`**")

    funcs = []
    for m in _RE_UFUNCTION.finditer(text):
        meta = m.group(1).strip()
        sig = m.group(2).strip().replace("\n", " ")
        sig = re.sub(r"\s+", " ", sig)
        funcs.append(f"- `UFUNCTION({meta})` {sig}")
    if funcs:
        out.append("\n**UFUNCTIONs:**")
        out.extend(funcs)

    props = []
    for m in _RE_UPROPERTY.finditer(text):
        meta = m.group(1).strip()
        decl = m.group(2).strip().replace("\n", " ")
        decl = re.sub(r"\s+", " ", decl)
        props.append(f"- `UPROPERTY({meta})` {decl}")
    if props:
        out.append("\n**UPROPERTYs:**")
        out.extend(props)

    return "\n".join(out) + "\n"


def _summarize_cpp(cpp_path: Path) -> str:
    """Extract method preconditions (the first ~8 lines of each method body)
    from a UE cpp. Surfaces guard clauses like `if (!HasAuthority()) return;`
    that tests must satisfy."""
    if not cpp_path.exists():
        return f"_(cpp not found: {cpp_path.name})_\n"
    text = cpp_path.read_text(encoding="utf-8", errors="ignore")
    out: list[str] = []
    for m in _RE_METHOD_BODY.finditer(text):
        name = m.group(1)
        body = m.group(2).strip()
        lines = [ln.rstrip() for ln in body.splitlines()[:8] if ln.strip()]
        if not lines:
            continue
        out.append(f"- `{name}(...)` first lines:")
        for ln in lines:
            out.append(f"    {ln}")
    return "\n".join(out) + "\n"


def phase2_api_discovery(ctx: TaskContext, ledger: QueryLedger) -> str:
    _phase_header("Phase 2 — API discovery")
    files_to_edit = ctx.spec.get("files_to_edit", [])
    if not files_to_edit:
        print("  [WARN] spec.yaml has no files_to_edit — API summary will be empty")
        return ""

    print(f"  Reading {len(files_to_edit)} files from solution/...")
    sections: list[str] = [
        f"# API Surface — {ctx.task_id}",
        "",
        f"Generated from solution/ at {time.strftime('%Y-%m-%d %H:%M:%S')}.",
        "",
        "## Behavioral contract (from spec.yaml)",
        "",
        (ctx.spec.get("instruction") or "_(no instruction)_").strip(),
        "",
        "---",
        "",
        "## Files to implement",
        "",
    ]
    for f in files_to_edit:
        sections.append(f"- `{f}`")
    sections.append("")

    # Group by class (pair .h + .cpp where both exist)
    by_stem: dict[str, list[Path]] = {}
    for rel in files_to_edit:
        path = ctx.solution_dir / rel
        stem = path.stem
        by_stem.setdefault(stem, []).append(path)

    for stem, paths in sorted(by_stem.items()):
        sections.append(f"## {stem}")
        sections.append("")
        for p in sorted(paths, key=lambda x: (x.suffix != ".h", x.name)):
            sections.append(f"### `{p.relative_to(ctx.solution_dir)}`")
            sections.append("")
            if p.suffix == ".h":
                sections.append(_summarize_header(p))
            else:
                sections.append(_summarize_cpp(p))
            sections.append("")

    api_surface = "\n".join(sections)

    out_path = ctx.log_dir / "api_surface.md"
    out_path.write_text(api_surface, encoding="utf-8")
    print(f"  ✓ Wrote {out_path}  ({len(api_surface)} chars)")
    print(f"  ✓ Covered {len(by_stem)} class(es) across {len(files_to_edit)} files")
    return api_surface


# ── Phase 3: Multi-model test generation ──────────────────────────────────────

_DRAFTER_SYSTEM_PROMPT = """You are an expert Unreal Engine 5.7 C++ developer specializing in automation testing.

Your task: given a behavioral spec and the target solution's API surface, produce a complete
`<TaskName>Tests.cpp` file following the unrealbench UE automation test template.

HARD REQUIREMENTS (violating these causes compile or runtime failure):
1. All game-specific `#include` statements at the TOP of the file. Never inside function bodies.
2. Use UE5.7 flag constants: `EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`
   (underscore on the mask; scoped enum for the filter).
3. Guard editor-only code with `#if WITH_EDITOR` and include `Tests/AutomationEditorCommon.h` there.
4. For tests needing a live world, use the PIE bootstrap pattern:
   - FEditorLoadMap(map path) — NOT FLoadGameMapCommand (asserts EWorldType::Game)
   - FWaitForShadersToFinishCompiling, FStartPIECommand(false), FWaitLatentCommand, FEndPlayMapCommand
5. Use `UGameplayStatics::ApplyPointDamage` — never call Actor::TakeDamage directly (may be protected).
6. When applying damage, provide a valid DamageCauser (ACharacter) and a non-empty HitResult.BoneName
   (e.g. "spine_01" or "head") — the solution's TakeDamage may gate on these.
7. For replication checks, spawn on server world and verify on client world. Never match on function
   names (that's implementation-coupled). Count NetMulticast functions via reflection instead.
8. Each test must compile to a separate IMPLEMENT_SIMPLE_AUTOMATION_TEST macro.
9. The prompt will include a REQUIRED BEHAVIOR CHECKLIST with stable IDs (`B1`, `B2`, ...).
   Every behavior ID must appear in your coverage_map and be honestly classified based on the tests
   you actually wrote.
10. Start with a small core suite of the strongest directly testable runtime behaviors, usually about
   6-10 tests. Prefer one strong direct behavioral test per major behavior category over broad shallow coverage.
11. DIRECT coverage means runtime behavioral coverage: drive a gameplay path and assert an
    observable outcome in the world or on replicated state. Reflection/schema/asset-wiring checks
    are not DIRECT coverage of gameplay behavior.
12. Do NOT count exact property presence, exact component existence, exact delegate binding,
    exact asset assignment, or exact RPC/flag presence as DIRECT coverage of a gameplay behavior.
    Use those only as PARTIAL coverage when a direct behavioral test is genuinely infeasible.
13. Prefer fewer strong runtime behavioral tests over many weak structural tests.
14. Use behavior categories, usually matching the spec headings, to choose coverage. Do not try to
    force every bullet into its own test in the first draft.

OUTPUT FORMAT:
Return a JSON object with exactly this schema:
{
  "test_file_content": "<complete C++ file contents as a string, including copyright/header comments>",
  "test_names": ["Module.Group.test_one", "Module.Group.test_two", ...],
  "rationale": "<one paragraph: which behaviors are tested and why>",
  "coverage_map": [
    {"behavior_id": "B1", "test_names": ["Module.Group.test_one"], "coverage": "direct|partial"}
  ]
}
"""

_PHASE3_JUDGE_SYSTEM_PROMPT = """You are a senior test-review engineer. You receive a draft Unreal automation
test file and the target solution's API surface. Your job: flag issues that will cause false negatives
(tests pass on empty stubs) or false positives (tests fail against valid implementations).

Focus on:
- Implementation coupling (exact method names, field names, substring matches on function names)
- Tests that can trivially pass on empty stubs (OR conditions that let them pass vacuously,
  assertions that don't gate on actual behavior)
- Missing preconditions that the solution requires (DamageCauser types, BoneName non-empty,
  HasAuthority checks, etc.)
- Non-PIE-friendly patterns (FLoadGameMapCommand, EAutomationTestFlags:: old form, etc.)
- Missing behavior coverage versus the REQUIRED BEHAVIOR CHECKLIST
- Behaviors that are only partially covered or only indirectly inferred

OUTPUT JSON:
{
  "issues": [
    {"severity": "critical|warning", "test_name": "...", "issue": "...", "suggestion": "..."},
    ...
  ],
  "overall_quality": "<one sentence>",
  "coverage_gaps": [
    {"behavior_id": "B1", "gap": "missing|partial", "reason": "..."}
  ]
}
"""

_IMPROVER_SYSTEM_PROMPT = """You are an expert test author. You receive a draft Unreal automation test
file, the API surface, and review feedback from another model. Your job: propose CONCRETE test
replacements for flagged issues, optionally adding tests for behaviors the drafter missed.

Focus on BEHAVIORAL checks (spawn X, call Y, assert observable state change), not structural ones
(does this field exist by exact name).

OUTPUT JSON:
{
  "replacements": [
    {"test_name": "existing or new", "new_code": "<full IMPLEMENT_*_AUTOMATION_TEST block>"},
    ...
  ],
  "new_tests": [ {"name": "...", "code": "..."} ],
  "notes": "<one paragraph>",
  "coverage_map": [
    {"behavior_id": "B1", "test_names": ["Module.Group.test_one"], "coverage": "direct|partial"}
  ]
}
"""

_EDITOR_SYSTEM_PROMPT = """You are the final editor. You receive:
- The current draft of the test file
- Judge feedback (issues identified)
- Improver suggestions (concrete replacements and new tests)

Integrate the feedback into a revised full test file. Preserve all the HARD REQUIREMENTS from the
drafter role (includes at top, UE5.7 flags, PIE bootstrap, UGameplayStatics, no name-match coupling).

Build around a stable core direct behavioral suite first. Prefer a smaller number of strong runtime
tests to a broad shallow suite. Only add more tests when they materially improve coverage of a major
behavior category. Consolidate genuinely duplicate tests.
Prefer DIRECT behavioral coverage over structural stand-ins. Do not upgrade a schema/reflection/
asset-wiring test to DIRECT coverage unless it truly drives runtime behavior and asserts an
observable outcome.
When a weak structural test and a stronger runtime test cover the same behavior, prefer replacing
the structural test rather than keeping both.

OUTPUT JSON:
{
  "test_file_content": "<complete revised C++ file>",
  "test_names": [...],
  "changes_applied": "<short summary of what you changed>",
  "coverage_map": [
    {"behavior_id": "B1", "test_names": ["Module.Group.test"], "coverage": "direct|partial"}
  ]
}
"""

_COVERAGE_PATCHER_SYSTEM_PROMPT = """You are an expert Unreal Engine 5.7 C++ test author.

The current test file is behaviorally incomplete at the behavior-group level. Edit the file in place
to improve coverage so:
- every behavior group has at least PARTIAL coverage
- a majority of behavior groups have DIRECT coverage

CONSTRAINTS:
- Preserve existing passing harness structure unless a tiny targeted extension is required.
- Make additive, selective changes. Add at most 1-2 tests in this pass unless replacing an existing weak test.
- Prefer replacing weak structural tests with stronger runtime tests over keeping both.
- Do not replace behavioral checks with reflection or implementation-coupled assertions.
- DIRECT coverage must come from runtime behavioral tests, not from exact property/component/
  delegate/asset/RPC checks.
- Preserve the exact UE5.7 automation flags:
  `EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`
- Preserve includes-at-top, PIE bootstrap patterns, and `UGameplayStatics::ApplyPointDamage`.
- Prefer direct coverage, but if a behavior truly cannot be made direct without brittle setup,
  partial coverage is acceptable as a last resort.

OUTPUT JSON:
{
  "test_file_content": "<complete revised C++ file>",
  "changes_summary": "<short summary>",
  "coverage_map": [
    {"behavior_id": "B1", "test_names": ["Module.Group.test"], "coverage": "direct|partial"}
  ]
}
"""

_COVERAGE_AUDITOR_SYSTEM_PROMPT = """You are a senior Unreal automation test reviewer.

Given a REQUIRED BEHAVIOR CHECKLIST with stable IDs (`B1`, `B2`, ...), an API surface summary,
and the current test file, decide which behaviors have direct test coverage, only partial coverage,
or no coverage.

DIRECT means a test explicitly drives the relevant gameplay path and asserts the observable outcome.
PARTIAL means the behavior is only indirectly implied, weakly approximated, or bundled into another
test without clearly proving the full requirement.
Structural tests such as exact property presence, exact component existence, exact delegate binding,
exact asset assignment, or exact RPC/flag presence should normally be classified as PARTIAL, not
DIRECT, unless they are only a small part of a larger runtime behavioral test.

OUTPUT JSON:
{
  "coverage_map": [
    {"behavior_id": "B1", "test_names": ["Module.Group.test"], "coverage": "direct|partial|missing", "reason": "..."}
  ],
  "summary": "<one sentence>"
}
"""


def _call_anthropic(
    system: str,
    user: str,
    max_tokens: int,
    ledger: QueryLedger,
    phase: str,
    role: str,
    model: str = "claude-sonnet-4-6",
) -> dict:
    """Call Claude with JSON response (streaming for high token budgets). Returns parsed JSON or raises."""
    import anthropic
    client = anthropic.Anthropic(api_key=os.environ["ANTHROPIC_API_KEY"])
    schema_instruction = "\n\nRespond with ONLY a valid JSON object. No prose, no markdown fences."

    # Streaming is required for max_tokens high enough that the response might
    # exceed 10 min. Always stream to be safe.
    raw_parts: list[str] = []
    in_toks = 0
    out_toks = 0
    stop_reason: Optional[str] = None
    with client.messages.stream(
        model=model,
        max_tokens=max_tokens,
        system=system + schema_instruction,
        messages=[{"role": "user", "content": user}],
    ) as stream:
        for text in stream.text_stream:
            raw_parts.append(text)
        final_msg = stream.get_final_message()
        if final_msg.usage:
            in_toks = final_msg.usage.input_tokens
            out_toks = final_msg.usage.output_tokens
        stop_reason = getattr(final_msg, "stop_reason", None)

    ledger.add(
        phase=phase, provider="claude", role=role,
        input_tokens=in_toks, output_tokens=out_toks,
    )
    raw = "".join(raw_parts)
    parsed = _parse_json_loose(raw)
    if parsed is None:
        hint = ""
        if stop_reason == "max_tokens":
            hint = (
                f"\n\nHINT: Claude stopped at max_tokens={max_tokens} "
                f"({out_toks} output tokens). Response was truncated. "
                "Bump max_tokens or split into smaller chunks."
            )
        raise RuntimeError(
            f"Claude ({role}) returned unparseable JSON "
            f"(stop_reason={stop_reason}, output_tokens={out_toks}). "
            f"First 300 chars: {raw[:300]}... Last 200 chars: ...{raw[-200:]}{hint}"
        )
    return parsed


def _call_openai(system: str, user: str, max_tokens: int, ledger: QueryLedger, phase: str, role: str) -> dict:
    """Call GPT-4o with JSON response."""
    import openai
    client = openai.OpenAI(api_key=os.environ["OPENAI_API_KEY"])
    schema_instruction = "\n\nRespond with ONLY a valid JSON object."
    resp = client.chat.completions.create(
        model="gpt-4o",
        response_format={"type": "json_object"},
        max_tokens=max_tokens,
        messages=[
            {"role": "system", "content": system + schema_instruction},
            {"role": "user", "content": user},
        ],
    )
    usage = resp.usage
    ledger.add(
        phase=phase, provider="openai", role=role,
        input_tokens=usage.prompt_tokens,
        output_tokens=usage.completion_tokens,
    )
    raw = resp.choices[0].message.content or ""
    parsed = _parse_json_loose(raw)
    if parsed is None:
        raise RuntimeError(f"OpenAI ({role}) returned unparseable JSON: {raw[:200]}")
    return parsed


def _call_gemini(system: str, user: str, max_tokens: int, ledger: QueryLedger, phase: str, role: str) -> dict:
    """Call Gemini via google-genai SDK (new interface, same as multi_model.py)."""
    from google import genai  # type: ignore
    api_key = os.environ.get("GOOGLE_API_KEY") or os.environ.get("GEMINI_API_KEY")
    client = genai.Client(api_key=api_key)
    full_prompt = f"{system}\n\nRespond with ONLY a valid JSON object.\n\n{user}"
    resp = client.models.generate_content(
        model="gemini-2.5-pro",
        contents=full_prompt,
        config=genai.types.GenerateContentConfig(
            max_output_tokens=max_tokens,
            response_mime_type="application/json",
        ),
    )
    usage = getattr(resp, "usage_metadata", None)
    ledger.add(
        phase=phase, provider="gemini", role=role,
        input_tokens=(usage.prompt_token_count if usage else 0),
        output_tokens=(usage.candidates_token_count if usage else 0),
    )
    raw = resp.text
    if raw is None or not raw.strip():
        # Gemini returned no text — usually means internal reasoning (`thoughts`)
        # consumed the whole output budget. Surface the usage so we can tell.
        thoughts = getattr(usage, "thoughts_token_count", 0) if usage else 0
        candidates = getattr(usage, "candidates_token_count", 0) if usage else 0
        raise RuntimeError(
            f"Gemini ({role}) returned empty text. "
            f"usage: thoughts={thoughts} candidates={candidates} max_tokens={max_tokens}. "
            "Bump max_tokens or simplify the prompt."
        )
    parsed = _parse_json_loose(raw)
    if parsed is None:
        raise RuntimeError(f"Gemini ({role}) returned unparseable JSON: {raw[:300]}")
    return parsed


def _parse_json_loose(text: str) -> Optional[dict]:
    """Parse JSON, stripping markdown fences and finding the first balanced
    {...} block if the outer shape isn't clean JSON."""
    text = text.strip()

    # Strip ```json ... ``` (or any language tag) fences, including trailing noise.
    fence_match = re.search(
        r"```(?:\w+)?\s*\n(.*?)\n```",
        text,
        re.DOTALL,
    )
    if fence_match:
        text = fence_match.group(1).strip()

    try:
        return json.loads(text)
    except json.JSONDecodeError:
        pass

    # Fallback: walk the string finding a balanced {...} that respects
    # JSON string literals (so braces inside C++ code don't confuse us).
    start = text.find("{")
    if start == -1:
        return None
    depth = 0
    in_str = False
    esc = False
    for i in range(start, len(text)):
        c = text[i]
        if esc:
            esc = False
        elif in_str and c == "\\":
            esc = True
        elif c == '"':
            in_str = not in_str
        elif not in_str:
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    try:
                        return json.loads(text[start:i + 1])
                    except json.JSONDecodeError:
                        return None
    return None


def _short_task_name(task_id: str) -> str:
    """Convert 'ue_task_0001_zombie_system_reimplementation' → 'ZombieSystemReimplementation'."""
    parts = task_id.split("_")[3:]  # skip 'ue', 'task', NNNN
    return "".join(p.capitalize() for p in parts)


def _default_test_file_path(ctx: TaskContext) -> Path:
    """Pick the canonical test file path for generation.

    If tests/ already contains exactly one .cpp file, reuse it. Otherwise fall back
    to the generated <TaskName>Tests.cpp naming convention.
    """
    existing = sorted(ctx.tests_dir.glob("*.cpp"))
    if len(existing) == 1:
        return existing[0]
    return ctx.tests_dir / f"{_short_task_name(ctx.task_id)}Tests.cpp"


def _extract_behavior_bullets(spec_instruction: str) -> list[dict[str, str]]:
    """Extract the required behavioral bullets from spec.yaml instruction text.

    Returns a list of {"id", "category", "text"} entries. Category headings are inferred
    from markdown headings or from lines ending in ':' immediately before bullet lists.
    """
    bullets: list[dict[str, str]] = []
    category = ""
    for raw_line in spec_instruction.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("#"):
            heading = line.lstrip("#").strip()
            if heading:
                category = heading
            continue
        if line.endswith(":") and not line.startswith("-"):
            category = line[:-1].strip()
            continue
        if line.startswith("- "):
            bullet_text = line[2:].strip()
            behavior_id = f"B{len(bullets) + 1}"
            bullets.append({
                "id": behavior_id,
                "category": category,
                "text": bullet_text,
            })
    return bullets


def _format_behavior_checklist(bullets: list[dict[str, str]]) -> str:
    if not bullets:
        return "_(no explicit behavioral bullets parsed from spec)_"
    lines = []
    for bullet in bullets:
        prefix = f"{bullet['category']} — " if bullet["category"] else ""
        lines.append(f"- `{bullet['id']}` {prefix}{bullet['text']}")
    return "\n".join(lines)


_MIN_DIRECT_GROUP_RATIO = 0.60
_PARTIAL_GROUP_WEIGHT = 0.50


def _normalize_coverage_map(raw_coverage_map: object) -> list[dict[str, object]]:
    if not isinstance(raw_coverage_map, list):
        return []
    normalized: list[dict[str, object]] = []
    for row in raw_coverage_map:
        if not isinstance(row, dict):
            continue
        behavior_id = str(row.get("behavior_id", "")).strip()
        coverage = str(row.get("coverage", "")).strip().lower()
        test_names = row.get("test_names", [])
        if not isinstance(test_names, list):
            test_names = []
        normalized.append({
            "behavior_id": behavior_id,
            "coverage": coverage,
            "test_names": [str(name).strip() for name in test_names if str(name).strip()],
        })
    return normalized


def _coverage_gaps(
    bullets: list[dict[str, str]],
    raw_coverage_map: object,
) -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    """Return (missing_direct, missing_any) gaps against the expected behavior bullets."""
    coverage_map = _normalize_coverage_map(raw_coverage_map)
    by_id: dict[str, list[dict[str, object]]] = {}
    for row in coverage_map:
        behavior_id = str(row.get("behavior_id", "")).strip()
        if behavior_id:
            by_id.setdefault(behavior_id, []).append(row)

    missing_direct: list[dict[str, str]] = []
    missing_any: list[dict[str, str]] = []
    for bullet in bullets:
        rows = by_id.get(bullet["id"], [])
        if not rows:
            missing_any.append(bullet)
            missing_direct.append(bullet)
            continue
        has_any = any(row.get("test_names") for row in rows)
        has_direct = any(
            row.get("coverage") == "direct" and row.get("test_names")
            for row in rows
        )
        if not has_any:
            missing_any.append(bullet)
        if not has_direct:
            missing_direct.append(bullet)
    return missing_direct, missing_any


def _extract_behavior_groups(bullets: list[dict[str, str]]) -> list[dict[str, object]]:
    groups: list[dict[str, object]] = []
    by_category: dict[str, dict[str, object]] = {}
    for bullet in bullets:
        category = bullet["category"].strip()
        if category:
            group = by_category.get(category)
            if group is None:
                group = {"id": f"G{len(groups) + 1}", "label": category, "bullets": []}
                groups.append(group)
                by_category[category] = group
            cast(list, group["bullets"]).append(bullet)
        else:
            groups.append({
                "id": f"G{len(groups) + 1}",
                "label": bullet["text"],
                "bullets": [bullet],
            })
    return groups


def _normalize_group_label(label: str) -> str:
    return re.sub(r"\s+", " ", label.strip().lower())


def _normalize_coverage_policy(raw_policy: object) -> dict[str, object]:
    if not isinstance(raw_policy, dict):
        return {}

    def _normalize_label_list(key: str) -> set[str]:
        value = raw_policy.get(key, [])
        if not isinstance(value, list):
            return set()
        return {
            _normalize_group_label(str(item))
            for item in value
            if str(item).strip()
        }

    min_direct_group_ratio = raw_policy.get("min_direct_group_ratio", _MIN_DIRECT_GROUP_RATIO)
    partial_group_weight = raw_policy.get("partial_group_weight", _PARTIAL_GROUP_WEIGHT)

    try:
        min_direct_group_ratio = float(min_direct_group_ratio)
    except (TypeError, ValueError):
        min_direct_group_ratio = _MIN_DIRECT_GROUP_RATIO
    try:
        partial_group_weight = float(partial_group_weight)
    except (TypeError, ValueError):
        partial_group_weight = _PARTIAL_GROUP_WEIGHT

    return {
        "harness_limited_groups": _normalize_label_list("harness_limited_groups"),
        "min_direct_group_ratio": max(0.0, min(1.0, min_direct_group_ratio)),
        "partial_group_weight": max(0.0, min(1.0, partial_group_weight)),
        "allow_none_in_harness_limited": bool(raw_policy.get("allow_none_in_harness_limited", False)),
    }


def _summarize_group_coverage(
    bullets: list[dict[str, str]],
    raw_coverage_map: object,
    raw_coverage_policy: object = None,
) -> dict[str, object]:
    coverage_map = _normalize_coverage_map(raw_coverage_map)
    coverage_policy = _normalize_coverage_policy(raw_coverage_policy)
    by_id: dict[str, list[dict[str, object]]] = {}
    for row in coverage_map:
        behavior_id = str(row.get("behavior_id", "")).strip()
        if behavior_id:
            by_id.setdefault(behavior_id, []).append(row)

    bullet_status: dict[str, str] = {}
    for bullet in bullets:
        rows = by_id.get(bullet["id"], [])
        has_direct = any(
            row.get("coverage") == "direct" and row.get("test_names")
            for row in rows
        )
        has_any = any(
            row.get("coverage") in {"direct", "partial"} and row.get("test_names")
            for row in rows
        )
        if has_direct:
            bullet_status[bullet["id"]] = "direct"
        elif has_any:
            bullet_status[bullet["id"]] = "partial"
        else:
            bullet_status[bullet["id"]] = "none"

    groups = _extract_behavior_groups(bullets)
    group_summaries: list[dict[str, object]] = []
    direct_groups = 0
    partial_groups = 0
    none_groups = 0
    counted_direct_groups = 0
    counted_partial_groups = 0
    counted_none_groups = 0
    exempt_none_groups = 0
    counted_total_groups = 0

    harness_limited_groups = cast(set, coverage_policy.get("harness_limited_groups", set()))
    min_direct_group_ratio = cast(float, coverage_policy.get("min_direct_group_ratio", _MIN_DIRECT_GROUP_RATIO))
    partial_group_weight = cast(float, coverage_policy.get("partial_group_weight", _PARTIAL_GROUP_WEIGHT))
    allow_none_in_harness_limited = cast(
        bool, coverage_policy.get("allow_none_in_harness_limited", False)
    )

    for group in groups:
        group_bullets = cast(list, group["bullets"])
        statuses = [bullet_status[bullet["id"]] for bullet in group_bullets]
        direct_count = sum(1 for status in statuses if status == "direct")
        partial_count = sum(1 for status in statuses if status == "partial")
        covered_count = direct_count + partial_count
        coverage_score = (
            (direct_count + (partial_group_weight * partial_count)) / len(statuses)
            if statuses else 0.0
        )

        if covered_count == 0:
            status = "none"
            none_groups += 1
        elif coverage_score >= min_direct_group_ratio and direct_count > 0:
            status = "direct"
            direct_groups += 1
        else:
            status = "partial"
            partial_groups += 1

        normalized_label = _normalize_group_label(str(group["label"]))
        is_harness_limited = normalized_label in harness_limited_groups
        counted = not is_harness_limited
        if counted:
            counted_total_groups += 1
            if status == "none":
                counted_none_groups += 1
            elif status == "direct":
                counted_direct_groups += 1
            else:
                counted_partial_groups += 1
        elif status == "none" and allow_none_in_harness_limited:
            exempt_none_groups += 1

        group_summaries.append({
            "group_id": group["id"],
            "label": group["label"],
            "status": status,
            "counts_toward_threshold": counted,
            "harness_limited": is_harness_limited,
            "bullet_ids": [bullet["id"] for bullet in group_bullets],
            "coverage_score": coverage_score,
        })

    total_groups = len(group_summaries)
    threshold_groups = counted_total_groups if harness_limited_groups else total_groups
    required_direct_groups = math.ceil(threshold_groups * min_direct_group_ratio) if threshold_groups else 0
    effective_direct_groups = counted_direct_groups + (partial_group_weight * counted_partial_groups)
    meets_threshold = (
        threshold_groups == 0
        or (counted_none_groups == 0 and effective_direct_groups >= required_direct_groups)
    )
    return {
        "groups": group_summaries,
        "direct_groups": direct_groups,
        "partial_groups": partial_groups,
        "none_groups": none_groups,
        "total_groups": total_groups,
        "counted_direct_groups": counted_direct_groups,
        "counted_partial_groups": counted_partial_groups,
        "counted_none_groups": counted_none_groups,
        "counted_total_groups": counted_total_groups,
        "exempt_none_groups": exempt_none_groups,
        "required_direct_groups": required_direct_groups,
        "effective_direct_groups": effective_direct_groups,
        "meets_threshold": meets_threshold,
    }


def _run_cli_authoring_agent(
    ctx: TaskContext,
    authoring: AuthoringConfig,
    prompt: str,
    ledger: QueryLedger,
    phase: str,
    role: str,
    timeout_seconds: int = 1200,
) -> None:
    """Run the CLI authoring agent against the current repo and let it edit files in place."""
    print(f"         cli-agent ({authoring.agent}): ", end="", flush=True)
    t0 = time.time()
    if authoring.agent == "codex":
        cmd = [
            "codex",
            "exec",
            "--skip-git-repo-check",
            "--yolo",
            "-s", "danger-full-access",
            "-C", str(REPO_ROOT),
            prompt,
        ]
        if authoring.model:
            cmd[1:1] = ["--model", authoring.model]

        try:
            with tempfile.NamedTemporaryFile(mode="w+", encoding="utf-8", delete=False) as tmp:
                tmp_path = Path(tmp.name)
                proc = subprocess.Popen(
                    cmd,
                    stdout=tmp,
                    stderr=tmp,
                    text=True,
                    cwd=str(REPO_ROOT),
                )

            heartbeat_interval = 10.0
            next_heartbeat = time.time() + heartbeat_interval
            while True:
                ret = proc.poll()
                now = time.time()
                if ret is not None:
                    break
                if now >= next_heartbeat:
                    elapsed = now - t0
                    print(f"…{int(elapsed)}s", end="", flush=True)
                    next_heartbeat = now + heartbeat_interval
                if now - t0 > timeout_seconds:
                    proc.kill()
                    proc.wait()
                    raise subprocess.TimeoutExpired(cmd, timeout_seconds)
                time.sleep(0.5)
            output_text = tmp_path.read_text(encoding="utf-8", errors="ignore")
        except FileNotFoundError as exc:
            raise RuntimeError("Codex CLI not found. Install with: npm i -g @openai/codex") from exc
        elapsed = time.time() - t0
        status = "✓" if proc.returncode == 0 else "✗"
        print(f"{status} ({elapsed:.1f}s)")

        log_path = ctx.log_dir / f"{phase}_{role}_{authoring.agent}.log"
        log_path.write_text(
            f"$ {' '.join(cmd[:-1])} <prompt>\n\nPROMPT:\n{prompt}\n\nOUTPUT:\n{output_text}\n",
            encoding="utf-8",
        )
        ledger.add(
            phase=phase,
            provider=authoring.agent,
            role=role,
            input_tokens=0,
            output_tokens=0,
        )
        try:
            tmp_path.unlink(missing_ok=True)
        except OSError:
            pass
        if proc.returncode != 0:
            tail = output_text.strip()[-1200:]
            raise RuntimeError(f"CLI authoring agent failed:\n{tail}")
        return

    if authoring.agent == "claude-code":
        install_claude_code_sdk_compat()
        from claude_code_sdk import query, ClaudeCodeOptions

        async def _run_claude() -> tuple[bool, str, int, int]:
            options_kwargs = {
                "permission_mode": "bypassPermissions",
                "cwd": str(REPO_ROOT),
            }
            if authoring.model:
                options_kwargs["model"] = authoring.model
            options = ClaudeCodeOptions(**options_kwargs)

            output_parts: list[str] = []
            in_toks = 0
            out_toks = 0
            next_heartbeat = time.time() + 10.0
            async for message in query(prompt=prompt, options=options):
                output_parts.append(str(message))
                usage = getattr(message, "usage", None)
                if usage:
                    in_toks += usage.get("input_tokens", 0)
                    out_toks += usage.get("output_tokens", 0)
                now = time.time()
                if now >= next_heartbeat:
                    print(f"…{int(now - t0)}s", end="", flush=True)
                    next_heartbeat = now + 10.0
            return True, "".join(output_parts), in_toks, out_toks

        try:
            success, output_text, in_toks, out_toks = asyncio.run(
                asyncio.wait_for(_run_claude(), timeout=timeout_seconds)
            )
        except ModuleNotFoundError as exc:
            raise RuntimeError("claude_code_sdk not installed; cannot use authoring-agent claude-code") from exc
        except asyncio.TimeoutError as exc:
            raise RuntimeError(f"Claude Code authoring timed out after {timeout_seconds}s") from exc
        except Exception as exc:
            raise RuntimeError(f"Claude Code authoring failed: {exc}") from exc

        elapsed = time.time() - t0
        print(f"✓ ({elapsed:.1f}s)")
        log_path = ctx.log_dir / f"{phase}_{role}_{authoring.agent}.log"
        log_path.write_text(
            f"PROMPT:\n{prompt}\n\nOUTPUT:\n{output_text}\n",
            encoding="utf-8",
        )
        ledger.add(
            phase=phase,
            provider="claude",
            role=role,
            input_tokens=in_toks,
            output_tokens=out_toks,
        )
        if not success:
            tail = output_text.strip()[-1200:]
            raise RuntimeError(f"CLI authoring agent failed:\n{tail}")
        return

    raise RuntimeError(
        f"Unsupported cli authoring agent: {authoring.agent}. "
        "Currently supported: codex, claude-code"
    )


def phase3_test_generation(
    ctx: TaskContext,
    api_surface: str,
    ledger: QueryLedger,
    authoring: AuthoringConfig,
) -> Path:
    if authoring.mode == "cli_agent":
        _phase_header("Phase 3 — CLI agent test generation")
    else:
        _phase_header("Phase 3 — Multi-model test generation")

    test_filter = (ctx.spec.get("tests") or {}).get("filter", "")
    task_title = ctx.spec.get("title", ctx.task_id)
    test_file_path = _default_test_file_path(ctx)
    test_file_name = test_file_path.name
    spec_instruction = ctx.spec.get("instruction") or ""
    behavior_bullets = _extract_behavior_bullets(spec_instruction)
    behavior_checklist = _format_behavior_checklist(behavior_bullets)

    if authoring.mode == "cli_agent":
        ctx.tests_dir.mkdir(exist_ok=True)
        phase3_prompt = (
            f"You are authoring the canonical Unreal automation test file for benchmark task `{ctx.task_id}`.\n\n"
            f"Working repo root: `{REPO_ROOT}`\n"
            f"Task directory: `{ctx.task_dir}`\n"
            f"Target test file: `{test_file_path}`\n"
            f"API surface summary: `{ctx.log_dir / 'api_surface.md'}`\n\n"
            f"Requirements:\n"
            f"- Read `spec.yaml`, inspect the real repo as needed, and write the test file directly.\n"
            f"- Test behavior, not implementation details.\n"
            f"- Start with a small core suite of the strongest directly testable runtime behaviors, usually about 6-10 tests.\n"
            f"- Use major behavior categories from the spec headings to choose the strongest tests first.\n"
            f"- Cover remaining behaviors partially only when direct runtime testing is genuinely infeasible.\n"
            f"- Do not count exact property/component/delegate/asset/RPC checks as direct gameplay coverage.\n"
            f"- Preserve or create a stable Unreal PIE harness; do not over-engineer the framework.\n"
            f"- Use exact UE automation flags: `EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`\n"
            f"- Use `UGameplayStatics::ApplyPointDamage` instead of calling protected damage paths directly.\n"
            f"- Keep game-specific includes at the top of the file.\n"
            f"- For replication behaviors, use a targeted multiplayer bootstrap rather than rewriting the whole harness.\n"
            f"- Do not modify files outside the task test file unless absolutely required for the test to compile.\n\n"
            f"Required behavior checklist:\n{behavior_checklist}\n\n"
        )
        if ctx.evaluator_notes:
            phase3_prompt += (
                "Private evaluator notes are available below. Use them as authoring guidance, "
                "but do not copy them verbatim into the test file.\n\n"
                f"{ctx.evaluator_notes}\n\n"
            )
        phase3_prompt += (
            "Write or rewrite the full test file now. When done, stop."
        )
        _run_cli_authoring_agent(
            ctx,
            authoring,
            phase3_prompt,
            ledger,
            phase="phase3_cli",
            role="author",
            timeout_seconds=1800,
        )
        if not test_file_path.exists():
            raise RuntimeError(f"CLI agent did not create the expected test file: {test_file_path}")
        print(f"        Wrote {test_file_path.relative_to(REPO_ROOT)}  ({test_file_path.stat().st_size} bytes)")
        return test_file_path

    # Evaluator oracle block — prepended to every Phase 3 user prompt when
    # --evaluator-notes was supplied. This is the task author's private
    # "answer key" — known edge cases, passing criteria, common wrong
    # solutions. Used ONLY as LLM context; never persisted anywhere in the
    # unrealbench repo or written to log files.
    evaluator_block = ""
    if ctx.evaluator_notes:
        evaluator_block = (
            "# Evaluator oracle (private — do not echo verbatim in tests)\n"
            "The task author's private evaluator notes describing known edge cases, "
            "passing criteria, and common wrong solutions. Use this to inform WHICH "
            "behaviors to cover and WHERE the hard cases are. Do not literally copy "
            "the acceptance text into test comments.\n\n"
            f"{ctx.evaluator_notes}\n\n"
        )

    drafter_user = (
        f"# Task\n{task_title}\n\n"
        f"# Test filter\n{test_filter}\n\n"
        f"# Target test filename\n{test_file_name}\n\n"
        f"# Required behavior checklist\n{behavior_checklist}\n\n"
        f"{evaluator_block}"
        f"# API Surface\n{api_surface}\n\n"
        "Draft the complete test file now."
    )

    # Round 1: Claude drafts (16K tokens — test files routinely run 500-800 lines)
    print("  [1/4] Claude (drafter)...")
    drafter_out = _call_anthropic(
        _DRAFTER_SYSTEM_PROMPT, drafter_user,
        max_tokens=32000, ledger=ledger,
        phase="phase3_draft", role="drafter",
    )
    (ctx.log_dir / "phase3_1_drafter.md").write_text(
        f"# Claude Drafter\n\n## Rationale\n{drafter_out.get('rationale', '')}\n\n"
        f"## Test names\n{drafter_out.get('test_names', [])}\n\n"
        f"## Coverage map\n{json.dumps(drafter_out.get('coverage_map', []), indent=2)}\n\n"
        f"## File content\n```cpp\n{drafter_out.get('test_file_content', '')}\n```\n",
        encoding="utf-8",
    )
    draft_content = drafter_out["test_file_content"]
    print(f"        Wrote draft ({len(draft_content)} chars, {len(drafter_out.get('test_names', []))} tests)")

    # Round 2: GPT judges
    print("  [2/4] GPT-4o (judge)...")
    judge_user = (
        f"# Required behavior checklist\n{behavior_checklist}\n\n"
        f"{evaluator_block}"
        f"# API Surface\n{api_surface}\n\n"
        f"# Draft test file\n```cpp\n{draft_content}\n```\n\n"
        "Review and list issues."
    )
    judge_out = _call_openai(
        _PHASE3_JUDGE_SYSTEM_PROMPT, judge_user,
        max_tokens=4096, ledger=ledger,
        phase="phase3_judge", role="judge",
    )
    (ctx.log_dir / "phase3_2_judge.md").write_text(
        f"# GPT-4o Judge\n\n{json.dumps(judge_out, indent=2)}\n",
        encoding="utf-8",
    )
    issues = judge_out.get("issues", [])
    print(f"        Judge flagged {len(issues)} issue(s)")

    # Round 3: Gemini improves
    print("  [3/4] Gemini (improver)...")
    improver_user = (
        f"# Required behavior checklist\n{behavior_checklist}\n\n"
        f"{evaluator_block}"
        f"# API Surface\n{api_surface}\n\n"
        f"# Draft file\n```cpp\n{draft_content}\n```\n\n"
        f"# Judge feedback\n{json.dumps(judge_out, indent=2)}\n\n"
        "Propose concrete replacements for flagged tests."
    )
    improver_out = _call_gemini(
        _IMPROVER_SYSTEM_PROMPT, improver_user,
        max_tokens=16384, ledger=ledger,
        phase="phase3_improve", role="improver",
    )
    (ctx.log_dir / "phase3_3_improver.md").write_text(
        f"# Gemini Improver\n\n{json.dumps(improver_out, indent=2)}\n",
        encoding="utf-8",
    )
    replacements = improver_out.get("replacements", [])
    new_tests = improver_out.get("new_tests", [])
    print(f"        Proposed {len(replacements)} replacement(s) + {len(new_tests)} new test(s)")

    # Round 4: Claude integrates
    print("  [4/4] Claude (editor)...")
    editor_user = (
        f"# Required behavior checklist\n{behavior_checklist}\n\n"
        f"{evaluator_block}"
        f"# API Surface\n{api_surface}\n\n"
        f"# Current draft\n```cpp\n{draft_content}\n```\n\n"
        f"# Judge feedback\n{json.dumps(judge_out, indent=2)}\n\n"
        f"# Improver suggestions\n{json.dumps(improver_out, indent=2)}\n\n"
        "Integrate all feedback and return the revised full file."
    )
    editor_out = _call_anthropic(
        _EDITOR_SYSTEM_PROMPT, editor_user,
        max_tokens=32000, ledger=ledger,
        phase="phase3_edit", role="editor",
    )
    (ctx.log_dir / "phase3_4_editor.md").write_text(
        f"# Claude Editor\n\n## Changes applied\n{editor_out.get('changes_applied', '')}\n\n"
        f"## Coverage map\n{json.dumps(editor_out.get('coverage_map', []), indent=2)}\n\n"
        f"## Final file\n```cpp\n{editor_out.get('test_file_content', '')}\n```\n",
        encoding="utf-8",
    )
    final_content = editor_out["test_file_content"]
    final_coverage_map = editor_out.get("coverage_map", [])
    missing_direct, missing_any = _coverage_gaps(behavior_bullets, final_coverage_map)
    group_coverage = _summarize_group_coverage(
        behavior_bullets,
        final_coverage_map,
        ctx.spec.get("coverage_policy"),
    )

    if not group_coverage["meets_threshold"]:
        print(
            "        Coverage threshold not met: "
            f"{group_coverage['direct_groups']}/{group_coverage['total_groups']} groups direct, "
            f"{group_coverage['none_groups']} uncovered"
        )
        group_lines = []
        for group in cast(list, group_coverage["groups"]):
            status = str(group["status"])
            if status == "direct":
                continue
            bullet_ids = ", ".join(cast(list, group["bullet_ids"]))
            group_lines.append(
                f"- `{group['group_id']}` {group['label']} — {status.upper()} "
                f"(bullets: {bullet_ids})"
            )
        coverage_user = (
            f"# Required behavior checklist\n{behavior_checklist}\n\n"
            f"{evaluator_block}"
            f"# API Surface\n{api_surface}\n\n"
            f"# Current test file\n```cpp\n{final_content}\n```\n\n"
            f"# Undercovered behavior groups\n" + "\n".join(group_lines) + "\n\n"
            "Revise the full file by making the smallest useful change set. Add at most 1-2 tests in this pass "
            "unless you are replacing a weak structural test with a stronger runtime test. Every behavior group "
            "must have at least PARTIAL coverage and a majority of groups must have DIRECT coverage."
        )
        coverage_out = _call_anthropic(
            _COVERAGE_PATCHER_SYSTEM_PROMPT,
            coverage_user,
            max_tokens=32000,
            ledger=ledger,
            phase="phase3_coverage",
            role="coverage_patcher",
        )
        (ctx.log_dir / "phase3_5_coverage.md").write_text(
            f"# Claude Coverage Patcher\n\n## Summary\n{coverage_out.get('changes_summary', '')}\n\n"
            f"## Coverage map\n{json.dumps(coverage_out.get('coverage_map', []), indent=2)}\n\n"
            f"## Final file\n```cpp\n{coverage_out.get('test_file_content', '')}\n```\n",
            encoding="utf-8",
        )
        final_content = coverage_out["test_file_content"]
        final_coverage_map = coverage_out.get("coverage_map", [])
        missing_direct, missing_any = _coverage_gaps(behavior_bullets, final_coverage_map)
        group_coverage = _summarize_group_coverage(
            behavior_bullets,
            final_coverage_map,
            ctx.spec.get("coverage_policy"),
        )

    ctx.tests_dir.mkdir(exist_ok=True)
    test_file_path.write_text(final_content, encoding="utf-8")
    print(f"        Wrote {test_file_path.relative_to(REPO_ROOT)}  ({len(final_content)} chars)")
    if behavior_bullets:
        direct_count = len(behavior_bullets) - len(missing_direct)
        print(
            f"        Coverage summary: {direct_count}/{len(behavior_bullets)} required behaviors directly covered; "
            f"{group_coverage['direct_groups']}/{group_coverage['total_groups']} groups direct"
        )
    if missing_any:
        out = ctx.log_dir / "phase3_coverage_gaps.json"
        out.write_text(json.dumps({
            "missing_direct": missing_direct,
            "missing_any": missing_any,
            "coverage_map": final_coverage_map,
            "group_coverage": group_coverage,
        }, indent=2), encoding="utf-8")

    return test_file_path


# ── Phase 4: Audit loops ──────────────────────────────────────────────────────

@dataclass
class AuditResult:
    compile_ok: bool
    compile_tail: str = ""          # last 20 lines of compile output if failed
    tests_parsed: bool = False
    total: int = 0
    passed: int = 0
    failed_tests: list[dict] = field(default_factory=list)  # [{id, errors:[...]}]
    passing_tests: list[str] = field(default_factory=list)
    raw_output: str = ""
    crash_summary: str = ""         # set if audit.py reported [CRASH] (UE editor crashed mid-run)


def _run_audit_tests(task_id: str, target: str) -> AuditResult:
    """Run `audit.py --run-tests --target <solution|start> --task-id <id>` and parse output."""
    proc = _run_audit_subprocess(
        ["--run-tests", "--target", target, "--task-id", task_id],
        timeout=1500,
    )
    raw = proc.stdout or ""
    result = AuditResult(compile_ok=False, raw_output=raw)

    if "Compile OK" in raw:
        result.compile_ok = True
    elif "compile failed" in raw.lower():
        lines = raw.strip().splitlines()
        # Find the compile-failed block and capture tail
        tail: list[str] = []
        for i, line in enumerate(lines):
            if "[FAIL] compile failed" in line:
                tail = lines[i:i+25]
                break
        result.compile_tail = "\n".join(tail)
        return result

    # Parse test results: blocks of "[PASS|FAIL] <name>" followed optionally
    # by "→ <error>" indent lines.
    lines = raw.splitlines()
    current_test: Optional[dict] = None
    for line in lines:
        m_pass = re.match(r"\s+\[PASS\]\s+(\S+)", line)
        m_fail = re.match(r"\s+\[FAIL\]\s+(\S+)", line)
        m_err = re.match(r"\s+→\s+(.+)", line)
        m_summary = re.match(r"\s+Summary:\s+(\d+)/(\d+)\s+passed", line)
        m_crash = re.match(r"\s+\[CRASH\]\s+UE editor crashed mid-run:\s+(.+)", line)
        if m_pass:
            result.passing_tests.append(m_pass.group(1))
            current_test = None
        elif m_fail:
            current_test = {"id": m_fail.group(1), "errors": []}
            result.failed_tests.append(current_test)
        elif m_err and current_test is not None:
            current_test["errors"].append(m_err.group(1).strip())
        elif m_summary:
            result.passed = int(m_summary.group(1))
            result.total = int(m_summary.group(2))
            result.tests_parsed = True
        elif m_crash:
            result.crash_summary = m_crash.group(1).strip()

    return result


def _error_signature(result: AuditResult) -> str:
    """Compact fingerprint of the current failure state for same-error detection."""
    if not result.compile_ok:
        # Take the first error line from the compile tail
        for line in result.compile_tail.splitlines():
            if "error:" in line.lower():
                return f"compile:{line.strip()[:120]}"
        return f"compile:{result.compile_tail[:120]}"
    # Test-assertion signature: join test IDs with their first error message
    parts = []
    for t in result.failed_tests:
        first = t["errors"][0] if t["errors"] else "(no message)"
        parts.append(f"{t['id']}:{first[:80]}")
    return "tests:" + "|".join(sorted(parts))[:500]


def _build_patcher_context(
    ctx: TaskContext,
    api_surface: str,
    test_file: Path,
    failure_names: Optional[list[str]] = None,
) -> str:
    """Build the standard context block passed to every Phase 4 patcher call.

    Includes spec instruction, full API surface, source snippets for methods
    referenced by the current failures, and the current test file content.
    This is identical across 4a patch, 4b tighten, and 4b repair so Claude
    always has the same backdrop — only the task-specific prompt varies.
    """
    current_content = test_file.read_text(encoding="utf-8") if test_file.exists() else ""
    spec_instruction = (ctx.spec.get("instruction") or "").strip()
    behavior_bullets = _extract_behavior_bullets(spec_instruction)
    snippets = _extract_method_snippets(failure_names or [], ctx.solution_dir)
    start_header_snippets = _extract_start_header_snippets(ctx, test_file)

    parts: list[str] = []
    parts.append(f"## Behavioral contract (from spec.yaml)\n{spec_instruction}")
    if behavior_bullets:
        parts.append(f"## Required behavior checklist\n{_format_behavior_checklist(behavior_bullets)}")
    if ctx.evaluator_notes:
        parts.append(
            "## Evaluator oracle (private — authoritative guidance on what tests must check)\n"
            f"{ctx.evaluator_notes}"
        )
    parts.append(f"## API Surface (complete)\n{api_surface}")
    if snippets:
        parts.append(snippets)
    if start_header_snippets:
        parts.append(start_header_snippets)
    parts.append(f"## Current test file\n```cpp\n{current_content}\n```")
    return "\n\n".join(parts)


def _invoke_claude_code_patch(
    ctx: TaskContext,
    test_file: Path,
    system_prompt: str,
    user_prompt: str,
    ledger: QueryLedger,
    phase: str,
    timeout_seconds: int = 150,
) -> None:
    """Rewrite the test file via a single Anthropic API call.

    Callers must have already baked the full context (spec, api surface,
    source snippets, current file) into `user_prompt` via _build_patcher_context.
    This function just adds the tail instruction + schema and makes the call.
    """
    import anthropic

    full_user = (
        f"{user_prompt}\n\n"
        "Rewrite the entire file to address the issue above. If a test can't be "
        "made correct without testing private implementation state, DELETE IT "
        "ENTIRELY — don't ship an unfixable test."
    )

    schema_instruction = (
        "\n\nRespond with ONLY a valid JSON object with this shape:\n"
        '{"test_file_content": "<full rewritten file as a string>", '
        '"changes_summary": "<one sentence describing what you changed>"}'
    )

    client = anthropic.Anthropic(
        api_key=os.environ["ANTHROPIC_API_KEY"],
        timeout=timeout_seconds,
    )
    print("         patcher: ", end="", flush=True)
    t0 = time.time()
    try:
        raw_parts: list[str] = []
        in_toks = 0
        out_toks = 0
        with client.messages.stream(
            model="claude-sonnet-4-6",
            max_tokens=32000,
            system=system_prompt + schema_instruction,
            messages=[{"role": "user", "content": full_user}],
        ) as stream:
            for text in stream.text_stream:
                raw_parts.append(text)
                # Progress dot every ~500 chars of streaming output
                if sum(len(p) for p in raw_parts) // 500 > len(raw_parts) // 500:
                    print(".", end="", flush=True)
            final = stream.get_final_message()
            if final.usage:
                in_toks = final.usage.input_tokens
                out_toks = final.usage.output_tokens
        elapsed = time.time() - t0
        print(f" ✓ ({elapsed:.1f}s, {out_toks} out toks)", flush=True)
    except Exception as exc:
        elapsed = time.time() - t0
        print(f" [FAIL] ({elapsed:.1f}s) {exc}", flush=True)
        return

    raw = "".join(raw_parts)
    parsed = _parse_json_loose(raw)
    if parsed is None or "test_file_content" not in parsed:
        print("         [FAIL] patcher returned unparseable response")
        return

    test_file.write_text(parsed["test_file_content"], encoding="utf-8")
    summary = parsed.get("changes_summary", "(no summary)")
    print(f"         patch applied: {summary}")
    ledger.add(
        phase=phase, provider="claude", role="patcher",
        input_tokens=in_toks, output_tokens=out_toks,
    )


def _assess_test_coverage(
    ctx: TaskContext,
    api_surface: str,
    test_file: Path,
    ledger: QueryLedger,
    phase: str,
) -> dict:
    spec_instruction = (ctx.spec.get("instruction") or "").strip()
    behavior_bullets = _extract_behavior_bullets(spec_instruction)
    behavior_checklist = _format_behavior_checklist(behavior_bullets)
    current_content = test_file.read_text(encoding="utf-8") if test_file.exists() else ""
    evaluator_block = ""
    if ctx.evaluator_notes:
        evaluator_block = (
            "# Evaluator oracle (private — do not echo verbatim)\n"
            f"{ctx.evaluator_notes}\n\n"
        )

    user_prompt = (
        f"# Required behavior checklist\n{behavior_checklist}\n\n"
        f"{evaluator_block}"
        f"# API Surface\n{api_surface}\n\n"
        f"# Current test file\n```cpp\n{current_content}\n```\n\n"
        "Assess coverage now."
    )
    return _call_anthropic(
        _COVERAGE_AUDITOR_SYSTEM_PROMPT,
        user_prompt,
        max_tokens=12000,
        ledger=ledger,
        phase=phase,
        role="coverage_auditor",
    )


def _assess_start_passing_tests(
    ctx: TaskContext,
    api_surface: str,
    test_file: Path,
    passing_tests: list[str],
    ledger: QueryLedger,
    phase: str,
) -> dict:
    spec_instruction = (ctx.spec.get("instruction") or "").strip()
    behavior_bullets = _extract_behavior_bullets(spec_instruction)
    behavior_checklist = _format_behavior_checklist(behavior_bullets)
    start_header_snippets = _extract_start_header_snippets(ctx, test_file)

    test_blocks: list[str] = []
    for test_name in passing_tests:
        body = extract_test_body(test_name, test_file) or "(could not extract test body)"
        test_blocks.append(f"## {test_name}\n```cpp\n{body}\n```")

    evaluator_block = ""
    if ctx.evaluator_notes:
        evaluator_block = (
            "# Evaluator oracle (private — do not echo verbatim)\n"
            f"{ctx.evaluator_notes}\n\n"
        )

    user_prompt = (
        f"# Behavioral contract\n{spec_instruction}\n\n"
        f"# Required behavior checklist\n{behavior_checklist}\n\n"
        f"{evaluator_block}"
        f"# API Surface\n{api_surface}\n\n"
        f"{start_header_snippets}\n\n"
        f"# Tests that passed on start/\n" + "\n\n".join(test_blocks) + "\n\n"
        "Classify each passing test now."
    )
    return _call_anthropic(
        _START_PASS_AUDITOR_SYSTEM_PROMPT,
        user_prompt,
        max_tokens=8000,
        ledger=ledger,
        phase=phase,
        role="start_pass_auditor",
    )


def _invoke_authoring_repair(
    ctx: TaskContext,
    test_file: Path,
    authoring: AuthoringConfig,
    system_prompt: str,
    user_prompt: str,
    ledger: QueryLedger,
    phase: str,
) -> None:
    if authoring.mode == "cli_agent":
        prompt = (
            f"{system_prompt}\n\n"
            f"{user_prompt}\n\n"
            f"Edit the file `{test_file}` directly in the repo. Preserve the working harness when possible. "
            f"When finished, stop."
        )
        _run_cli_authoring_agent(
            ctx,
            authoring,
            prompt,
            ledger,
            phase=phase,
            role="repair",
            timeout_seconds=1800,
        )
        return

    _invoke_claude_code_patch(
        ctx,
        test_file,
        system_prompt=system_prompt,
        user_prompt=user_prompt,
        ledger=ledger,
        phase=phase,
    )


_PATCHER_SYSTEM_PROMPT = """You are an expert Unreal Engine 5.7 C++ test author.

A test file you authored is failing. Read the file, understand the failures, and Edit the file in
place to fix them.

CONSTRAINTS:
- Use the Edit or Write tool to modify the test file.
- Do NOT run the audit yourself — the caller controls that.
- Preserve the structural conventions: includes at top, EXACT UE5.7 flag constants
  (`EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`),
  PIE bootstrap via FEditorLoadMap+FStartPIECommand, UGameplayStatics::ApplyPointDamage for damage,
  valid BoneName, DamageCauser-as-character.
- NEVER change the automation flag spelling or style unless the compile error explicitly points to it.
  The required form is exactly:
  `EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`
- If the failure is a compile error, fix the cause (wrong type, missing include, etc.).
- If the failure is a compile error, make the SMALLEST POSSIBLE edit that fixes that exact error.
  Do not rewrite unrelated tests, macros, helper names, or conventions.
- If the failure is an assertion failure against solution/, the test's expectation likely doesn't
  match what the solution actually implements. Read the API surface file to see the solution's
  actual behavior and adjust the test.
- The prompt may include a REQUIRED BEHAVIOR CHECKLIST with IDs (`B1`, `B2`, ...). Preserve direct
  coverage for those behaviors while fixing failures. Do not drop coverage unless the behavior is
  genuinely impossible to test from public observable behavior.
- Every game-specific symbol called from the test file must be declared in start/ headers or
  standard UE headers. If a symbol exists only in solution/ or only in solution/.cpp, do not call it.
- Tests must rely on OBSERVABLE BEHAVIOR or clearly public declarations only.
- Do NOT directly access protected/private members, even if they appear in the API summary or
  solution snippets. Solution snippets show implementation details and preconditions, not
  necessarily public test API.
- Do NOT assume any symbol seen only in a `.cpp` snippet is callable or accessible from tests.
- Prefer gameplay-observable checks over structural or reflection checks when there is any doubt
  about member accessibility.
"""


_TIGHTENER_SYSTEM_PROMPT = """You are an expert Unreal Engine 5.7 C++ test author.

The tests in this file pass even when run against an empty stub (start/). That means they're
testing structure instead of behavior, or using OR conditions that let them pass vacuously.

Read the file and tighten the flagged tests so they actually gate on observable behavior. Use
the Edit or Write tool.

CONSTRAINTS:
- Preserve all structural conventions, especially the EXACT UE5.7 automation flags:
  `EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`
  Do not rewrite or modernize this spelling.
- Preserve all structural conventions (PIE bootstrap, UGameplayStatics, etc.).
- Don't break tests that currently pass on solution/ — just make them fail on start/.
- Preserve direct coverage of the REQUIRED BEHAVIOR CHECKLIST when present. Tightening should not
  silently delete coverage of spec bullets unless that coverage is impossible to make meaningful.
- Every game-specific symbol called from the test file must be declared in start/ headers or
  standard UE headers. Do not tighten a test by introducing solution-only API calls.
- Prefer behavioral assertions (observable state changes after explicit actions) over
  reflection/name-matching tests.
- Do NOT tighten tests by accessing protected/private members or by depending on symbols visible
  only in solution `.cpp` implementations.
"""

_START_COMPAT_PATCHER_SYSTEM_PROMPT = """You are an expert Unreal Engine 5.7 C++ test author.

The current test file compiles against solution/ but fails to compile against start/. This means
the tests are calling symbols that are not declared in the stripped start/ headers, or otherwise
depend on implementation-specific APIs that the model will not have while iterating.

Your job: make the SMALLEST POSSIBLE edit that restores start/ compile compatibility while
preserving the behavioral intent of the tests.

CONSTRAINTS:
- NEVER modify any game source files (start/Source/ or solution/Source/). You may ONLY edit the
  test .cpp file (and any companion .h test helper files). If a test references symbols that do not
  exist in start/ headers, remove or rewrite those test assertions — do NOT add those symbols to
  start/ headers or .cpp source files.
- Treat declarations visible in start/ headers as the only callable game-specific API surface.
- Do NOT call methods or access members that are missing from start/ headers, even if they exist
  in solution/ headers or solution/.cpp snippets.
- If a behavior currently depends on internal/controller/helper methods, rewrite the test to drive
  that behavior through public gameplay setup instead.
- Prefer preserving the existing harness and test names.
- Preserve the exact UE5.7 automation flags:
  `EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`
- Preserve includes-at-top, PIE bootstrap patterns, and `UGameplayStatics::ApplyPointDamage`.
- Do not broaden the edit beyond what is needed to make start/ compile and keep the test behavioral.
"""

_START_PASS_AUDITOR_SYSTEM_PROMPT = """You are a senior Unreal automation test reviewer.

The current test suite is being run against start/, the stripped starter workspace. Some tests passed.
Your job: classify each passing test as either:

- ACCEPTABLE_PASS: this test is legitimately expected to pass on start/ because it is a regression,
  API-surface, harness, or other behavior that the starter implementation should already satisfy.
- SHOULD_FAIL: this test is intended to distinguish start/ from solution/ and should therefore fail
  on start/. If it passed, it needs tightening.

Rules:
- Judge against the behavioral spec first.
- Use start/ header snippets only to understand what public callable API exists; do not require exact
  internal implementation details.
- Be conservative. Only label ACCEPTABLE_PASS when there is a strong reason the test truly should pass.

OUTPUT JSON:
{
  "classifications": [
    {"test_name": "Module.Group.test", "classification": "ACCEPTABLE_PASS|SHOULD_FAIL", "reason": "..."}
  ],
  "summary": "<one sentence>"
}
"""


def _extract_method_snippets(failure_names: list[str], solution_dir: Path, max_methods: int = 6) -> str:
    """For each failed test, try to identify referenced class::method names and
    extract a short snippet from the solution's cpp implementation so the
    patcher can see preconditions / guard clauses."""
    # Pull candidate method tokens from failure test IDs like
    # "BaseFirearm_Fire_ConsumesAmmo" or "HordeTemplate.Weapon.projectile_spawns_on_fire"
    tokens: set[str] = set()
    for name in failure_names:
        for part in re.split(r"[._]", name):
            if part and part[0].isalpha() and len(part) > 2:
                tokens.add(part)

    if not tokens:
        return ""

    # Walk cpp files and collect matching method bodies.
    snippets: list[str] = []
    for cpp in sorted(solution_dir.rglob("*.cpp")):
        if len(snippets) >= max_methods:
            break
        text = cpp.read_text(encoding="utf-8", errors="ignore")
        # Match method definitions whose name matches any token (case-insensitive substring)
        for m in re.finditer(
            r"^(?:\w[\w\s:*&<>]*)::(\w+)\([^)]*\)[^{]*\{",
            text,
            re.MULTILINE,
        ):
            if len(snippets) >= max_methods:
                break
            method_name = m.group(1)
            if not any(tok.lower() in method_name.lower() or method_name.lower() in tok.lower() for tok in tokens):
                continue
            # Grab the first ~20 lines of the method body
            start = m.end()
            depth = 1
            end = start
            for i, c in enumerate(text[start:], start=start):
                if c == "{": depth += 1
                elif c == "}":
                    depth -= 1
                    if depth == 0:
                        end = i
                        break
            body_lines = text[start:end].strip().splitlines()[:20]
            snippet = "\n".join(body_lines)
            snippets.append(
                f"### `{cpp.name}::{method_name}(...)`\n```cpp\n{snippet}\n```"
            )

    if not snippets:
        return ""
    header = (
        "## Relevant solution source snippets\n\n"
        "First lines of methods referenced by failing tests. Look for preconditions "
        "(HasAuthority, null checks, field guards) that tests must satisfy.\n\n"
    )
    return header + "\n\n".join(snippets)


def _extract_start_header_snippets(ctx: TaskContext, test_file: Path, max_headers: int = 10) -> str:
    """Read game-specific headers included by the current test file from start/.

    This helps Phase 4b compatibility repair stay anchored to callable symbols that the
    stripped starter workspace actually exposes.
    """
    if not test_file.exists():
        return ""

    text = test_file.read_text(encoding="utf-8", errors="ignore")
    include_paths = re.findall(r'^\s*#include\s+"([^"]+)"', text, re.MULTILINE)
    snippets: list[str] = []
    seen: set[Path] = set()

    for include in include_paths:
        if include.startswith(("Core", "Engine/", "GameFramework/", "Kismet/", "Misc/", "Tests/", "Settings/")):
            continue

        candidates = list(ctx.start_dir.rglob(include))
        if not candidates:
            continue
        header = candidates[0]
        if header in seen:
            continue
        seen.add(header)
        body = header.read_text(encoding="utf-8", errors="ignore")
        body_lines = body.splitlines()[:120]
        snippets.append(
            f"### `{header.relative_to(ctx.start_dir)}`\n```cpp\n" + "\n".join(body_lines) + "\n```"
        )
        if len(snippets) >= max_headers:
            break

    if not snippets:
        return ""

    header = (
        "## Relevant start/ header snippets\n\n"
        "These are the callable declarations the stripped starter workspace exposes. "
        "Tests must compile against these headers.\n\n"
    )
    return header + "\n\n".join(snippets)


def _phase4a_solution_loop(
    ctx: TaskContext,
    test_file: Path,
    api_surface: str,
    ledger: QueryLedger,
    authoring: AuthoringConfig,
) -> bool:
    """Iterate until solution/ tests all pass. Bail if same error 3x in a row."""
    last_sig: Optional[str] = None
    same_count = 0
    iteration = 0

    while True:
        iteration += 1
        print(f"  [4a.{iteration}] Running audit against solution/...")
        result = _run_audit_tests(ctx.task_id, "solution")

        if not result.compile_ok:
            print(f"         compile failed")
        elif result.crash_summary:
            print(f"         ✗ UE crashed mid-run: {result.crash_summary}")
            if result.total > 0:
                print(f"         (only {result.passed}/{result.total} tests reported before crash; rest never ran)")
        elif result.tests_parsed and result.passed == result.total and result.total > 0:
            print(f"         ✓ solution/ passes all {result.total} tests")
            return True
        else:
            print(f"         {result.passed}/{result.total} passed, {len(result.failed_tests)} failed")

        sig = _error_signature(result)
        if sig == last_sig:
            same_count += 1
            print(f"         same error as last iteration (strike {same_count}/3)")
            if same_count >= 3:
                print("  [BAIL] Same error 3 iterations in a row — handing off to human.")
                return False
        else:
            last_sig = sig
            same_count = 1

        # Build prompt for patcher (uses unified context)
        failures_block = ""
        failure_names: list[str] = []
        if not result.compile_ok:
            failures_block = f"## Compile failure (tail)\n```\n{result.compile_tail}\n```\n"
        elif result.crash_summary:
            failures_block = (
                "## UE crash during solution/ run\n"
                f"- Crash summary: {result.crash_summary}\n"
                f"- Tests reported before crash: {result.passed}/{result.total}\n\n"
            )
        else:
            for t in result.failed_tests:
                failures_block += f"- **{t['id']}**\n"
                failure_names.append(t["id"])
                for e in t["errors"]:
                    failures_block += f"  - {e}\n"

        context = _build_patcher_context(ctx, api_surface, test_file, failure_names)
        user_prompt = (
            f"The test file at `{test_file.relative_to(ctx.task_dir)}` is failing against solution/.\n\n"
            f"## Failures\n{failures_block}\n\n"
            f"{context}\n\n"
            f"Fix or DELETE failing tests as appropriate. If the run crashes Unreal, prioritize removing or "
            f"simplifying the crash-inducing test/harness path before chasing broader coverage. If a test gates on private "
            f"implementation state (internal counters, protected methods, component "
            f"activation flags), it cannot be fixed from the test side — delete it."
        )
        _invoke_authoring_repair(
            ctx,
            test_file,
            authoring,
            system_prompt=_PATCHER_SYSTEM_PROMPT,
            user_prompt=user_prompt,
            ledger=ledger,
            phase="phase4a_patch",
        )


def _phase4b_tightening_loop(
    ctx: TaskContext,
    test_file: Path,
    api_surface: str,
    ledger: QueryLedger,
    authoring: AuthoringConfig,
) -> dict:
    """Iterate tightening tests that pass trivially on start/.

    Unbounded rounds with same-error-3× bail, same pattern as 4a. The fingerprint
    here is the set of tests that still pass trivially — if the same tests are
    stuck passing after three patches, the tightener is spinning and we hand off.
    Returns a dict describing the final start/ pass status.
    """
    last_sig: Optional[str] = None
    same_count = 0
    iteration = 0
    compat_repair_attempts = 0
    last_compat_sig: Optional[str] = None

    while True:
        iteration += 1
        print(f"  [4b.{iteration}] Running audit against start/...")
        result = _run_audit_tests(ctx.task_id, "start")

        # Compile failure on start/ is a broken setup, NOT convergence.
        # The model needs start/ to compile so it can iterate on its code.
        # If compile fails, the test file is likely referencing symbols that
        # exist only in solution/'s stripped cpp files. Try a targeted
        # compatibility repair before escalating to a human.
        if not result.compile_ok:
            print(f"         ✗ start/ doesn't compile — the model needs a compilable starting point")
            print(f"         compile tail:")
            for line in result.compile_tail.splitlines()[-10:]:
                print(f"           {line}")
            compat_sig = result.compile_tail.strip()[-1200:]
            if compat_sig == last_compat_sig:
                compat_repair_attempts += 1
            else:
                last_compat_sig = compat_sig
                compat_repair_attempts = 1

            if compat_repair_attempts > 2:
                print(f"  [BAIL] Phase 4b can't proceed — tests reference symbols the stub doesn't provide.")
                print(f"         Fix the test file to only reference declarations present in start/ headers.")
                return {"status": "start_compile_failed", "unexpected_passes": None, "acceptable_passes": None}

            context = _build_patcher_context(ctx, api_surface, test_file, [])
            user_prompt = (
                f"The test file compiles against solution/ but fails to compile against start/.\n\n"
                f"## start/ compile failure (tail)\n```\n{result.compile_tail}\n```\n\n"
                f"{context}\n\n"
                f"Make the smallest possible edit so the tests compile against start/ headers. "
                f"Remove or rewrite any solution-only API calls, but preserve behavioral intent."
            )
            _invoke_authoring_repair(
                ctx,
                test_file,
                authoring,
                system_prompt=_START_COMPAT_PATCHER_SYSTEM_PROMPT,
                user_prompt=user_prompt,
                ledger=ledger,
                phase="phase4b_start_compat",
            )
            continue

        if result.crash_summary:
            print(f"         ✗ UE crashed mid-run: {result.crash_summary}")
            print(f"         (only {result.passed}/{result.total} tests reported before crash; rest never ran)")
            print(f"  [BAIL] Phase 4b can't measure start/ failure rate when the editor crashes.")
            print(f"         Fix the test infrastructure (likely a CreateWorld / GC issue) and retry.")
            return {"status": "crash", "unexpected_passes": None, "acceptable_passes": None}

        if not result.tests_parsed:
            print("         ✗ couldn't parse test results")
            print("  [BAIL] Phase 4b is inconclusive — unparseable start/ results are not evidence of non-triviality.")
            print("         Inspect compile_output.txt and unreal_log.txt for the start/ run, then retry.")
            return {"status": "unparseable", "unexpected_passes": None, "acceptable_passes": None}

        print(f"         {result.passed}/{result.total} pass on start/")
        if result.passed == 0:
            print(f"         ✓ Converged (0 tests pass on start/ — all tests discriminate)")
            return {
                "status": "ok",
                "unexpected_passes": 0,
                "acceptable_passes": 0,
                "raw_passes": 0,
            }
        start_pass_audit = _assess_start_passing_tests(
            ctx,
            api_surface,
            test_file,
            result.passing_tests,
            ledger,
            phase="phase4b_start_pass_audit",
        )
        (ctx.log_dir / "phase4b_start_pass_audit.json").write_text(
            json.dumps(start_pass_audit, indent=2),
            encoding="utf-8",
        )
        classifications = start_pass_audit.get("classifications", []) or []
        acceptable_passes = [
            c["test_name"] for c in classifications
            if c.get("classification") == "ACCEPTABLE_PASS"
        ]
        should_fail_passes = [
            c["test_name"] for c in classifications
            if c.get("classification") == "SHOULD_FAIL"
        ]
        if not should_fail_passes:
            print(f"         ✓ Converged (all tests that should fail do fail)")
            if acceptable_passes:
                print(f"         acceptable start/ passes: {len(acceptable_passes)}")
            return {
                "status": "ok",
                "unexpected_passes": 0,
                "acceptable_passes": len(acceptable_passes),
                "raw_passes": result.passed,
            }

        # Same-error-3× bail: if the same set of tests keeps passing trivially
        # after patches, the tightener is spinning and can't make progress.
        sig = "|".join(sorted(should_fail_passes))
        if sig == last_sig:
            same_count += 1
            print(f"         same tests trivially passing as last iteration (strike {same_count}/3)")
            if same_count >= 3:
                print("  [BAIL] Same start/ failure 3 iterations in a row — handing off to human.")
                return {
                    "status": "stuck",
                    "unexpected_passes": len(should_fail_passes),
                    "acceptable_passes": len(acceptable_passes),
                    "raw_passes": result.passed,
                }
        else:
            last_sig = sig
            same_count = 1

        # Tighten passing tests — and preserve solution/ correctness.
        passing_names = ", ".join(should_fail_passes)
        context = _build_patcher_context(ctx, api_surface, test_file, should_fail_passes)
        user_prompt = (
            f"The following tests passed against start/ but should fail because they are intended "
            f"to distinguish the stub from solution/.\n\n"
            f"## Tests that should fail on start/ but currently pass\n{passing_names}\n\n"
            f"{context}\n\n"
            f"CRITICAL: any changes must preserve behavior against solution/. If you "
            f"can't tighten a test without breaking its solution-pass behavior, delete it. "
            f"It is fine for regression-style tests that are legitimately expected to pass on start/ "
            f"to remain passing."
        )
        _invoke_authoring_repair(
            ctx,
            test_file,
            authoring,
            system_prompt=_TIGHTENER_SYSTEM_PROMPT,
            user_prompt=user_prompt,
            ledger=ledger,
            phase="phase4b_tighten",
        )


def phase4_audit_loops(
    ctx: TaskContext,
    test_file: Path,
    api_surface: str,
    ledger: QueryLedger,
    authoring: AuthoringConfig,
    max_attempts: int = 3,
) -> dict:
    """Outer loop: run 4a then 4b, ping-pong up to max_attempts times.

    After each [4a → 4b] attempt, re-verify 4a still passes. If 4b's tightening
    regressed solution/, start another attempt (re-run 4a, which will patch it
    back, then re-run 4b, which may re-introduce the regression on different
    tests). Exit when both invariants hold simultaneously on the same file, or
    when max_attempts is reached.
    """
    _phase_header("Phase 4 — Audit loops")

    for attempt in range(1, max_attempts + 1):
        print(f"\n  ═══ Attempt {attempt}/{max_attempts} ═══")

        # 4a: make solution pass
        print(f"\n  Phase 4a (attempt {attempt}) — Solution-pass loop")
        solution_ok = _phase4a_solution_loop(ctx, test_file, api_surface, ledger, authoring)
        if not solution_ok:
            return {
                "solution_pass": False, "start_trivial_pass": None,
                "attempts_used": attempt,
            }

        # 4b: tighten against start
        print(f"\n  Phase 4b (attempt {attempt}) — Start-fail tightening loop")
        pre_phase4b_content = test_file.read_text(encoding="utf-8") if test_file.exists() else ""
        start_result = _phase4b_tightening_loop(ctx, test_file, api_surface, ledger, authoring)
        post_phase4b_content = test_file.read_text(encoding="utf-8") if test_file.exists() else ""

        if start_result.get("status") != "ok":
            return {
                "solution_pass": False, "start_trivial_pass": -1,
                "attempts_used": attempt,
                "bail_reason": start_result.get("status"),
                "acceptable_start_passes": start_result.get("acceptable_passes"),
                "unexpected_start_passes": start_result.get("unexpected_passes"),
            }

        if post_phase4b_content == pre_phase4b_content:
            print("\n  [verify] Skipping solution/ re-audit after Phase 4b (no test changes)")
            both_ok = True
            sol_result = None
        else:
        # Regression check: does solution/ still pass after 4b's edits?
            print(f"\n  [verify] Re-running audit against solution/ after Phase 4b...")
            sol_result = _run_audit_tests(ctx.task_id, "solution")
            both_ok = (
                sol_result.compile_ok and sol_result.tests_parsed
                and sol_result.passed == sol_result.total and sol_result.total > 0
                and not sol_result.crash_summary
            )
            if sol_result.crash_summary:
                print(f"         ✗ UE crashed mid-run: {sol_result.crash_summary}")
        if both_ok:
            if sol_result is not None:
                print(f"         ✓ solution/ still passes all {sol_result.total} tests")

            coverage_bullets = _extract_behavior_bullets((ctx.spec.get("instruction") or "").strip())
            if coverage_bullets:
                coverage_assessment = _assess_test_coverage(
                    ctx,
                    api_surface,
                    test_file,
                    ledger,
                    phase="phase4_coverage_audit",
                )
                coverage_map = coverage_assessment.get("coverage_map", [])
                missing_direct, missing_any = _coverage_gaps(coverage_bullets, coverage_map)
                group_coverage = _summarize_group_coverage(
                    coverage_bullets,
                    coverage_map,
                    ctx.spec.get("coverage_policy"),
                )
                (ctx.log_dir / "phase4_coverage_audit.json").write_text(
                    json.dumps({
                        **coverage_assessment,
                        "group_coverage": group_coverage,
                    }, indent=2),
                    encoding="utf-8",
                )
                covered = len(coverage_bullets) - len(missing_direct)
                print(f"         coverage audit: {covered}/{len(coverage_bullets)} required behaviors directly covered")
                print(
                    "         coverage groups: "
                    f"{group_coverage['direct_groups']}/{group_coverage['total_groups']} direct, "
                    f"{group_coverage['partial_groups']}/{group_coverage['total_groups']} partial, "
                    f"{group_coverage['none_groups']}/{group_coverage['total_groups']} none"
                )
                if group_coverage.get("counted_total_groups", group_coverage["total_groups"]) != group_coverage["total_groups"]:
                    print(
                        "         threshold groups: "
                        f"{group_coverage.get('counted_direct_groups', 0)}/"
                        f"{group_coverage.get('counted_total_groups', 0)} direct, "
                        f"{group_coverage.get('counted_partial_groups', 0)}/"
                        f"{group_coverage.get('counted_total_groups', 0)} partial, "
                        f"{group_coverage.get('counted_none_groups', 0)}/"
                        f"{group_coverage.get('counted_total_groups', 0)} blocking none, "
                        f"{group_coverage.get('exempt_none_groups', 0)} exempt none"
                    )
                print(
                    "         effective direct groups: "
                    f"{group_coverage.get('effective_direct_groups', 0):.1f}/"
                    f"{group_coverage['required_direct_groups']} required"
                )
                if not group_coverage["meets_threshold"]:
                    print("         ✗ coverage threshold not met")
                    if attempt >= max_attempts:
                        return {
                            "solution_pass": False,
                            "start_trivial_pass": start_result.get("unexpected_passes", 0),
                            "attempts_used": attempt,
                            "bail_reason": "coverage_threshold_not_met",
                            "coverage_direct": covered,
                            "coverage_total": len(coverage_bullets),
                            "coverage_groups_direct": group_coverage["direct_groups"],
                            "coverage_groups_partial": group_coverage["partial_groups"],
                            "coverage_groups_none": group_coverage["none_groups"],
                            "coverage_groups_total": group_coverage["total_groups"],
                            "coverage_groups_counted_direct": group_coverage.get("counted_direct_groups", 0),
                            "coverage_groups_counted_partial": group_coverage.get("counted_partial_groups", 0),
                            "coverage_groups_counted_none": group_coverage.get("counted_none_groups", 0),
                            "coverage_groups_counted_total": group_coverage.get("counted_total_groups", 0),
                            "coverage_groups_exempt_none": group_coverage.get("exempt_none_groups", 0),
                            "coverage_required_direct_groups": group_coverage["required_direct_groups"],
                            "coverage_effective_direct_groups": group_coverage["effective_direct_groups"],
                            "acceptable_start_passes": start_result.get("acceptable_passes"),
                            "unexpected_start_passes": start_result.get("unexpected_passes"),
                        }

                    group_lines = []
                    for group in cast(list, group_coverage["groups"]):
                        status = str(group["status"])
                        if status == "direct":
                            continue
                        bullet_ids = ", ".join(cast(list, group["bullet_ids"]))
                        group_lines.append(
                            f"- `{group['group_id']}` {group['label']} — {status.upper()} "
                            f"(bullets: {bullet_ids})"
                        )
                    context = _build_patcher_context(ctx, api_surface, test_file, [])
                    user_prompt = (
                        "The current test file passes solution/ and tightens start/, but it still "
                        "misses the required behavior-group coverage threshold.\n\n"
                        "## Undercovered behavior groups\n" + "\n".join(group_lines) + "\n\n"
                        f"{context}\n\n"
                        "Make the smallest useful additive coverage change. Add at most 1-2 tests in this pass "
                        "unless you are replacing a weak structural test with a stronger runtime test. Every "
                        "behavior group must have at least PARTIAL coverage and a majority of groups must have "
                        "DIRECT coverage, while preserving the current passing behavior against solution/ and "
                        "non-triviality against start/."
                    )
                    _invoke_authoring_repair(
                        ctx,
                        test_file,
                        authoring,
                        system_prompt=_COVERAGE_PATCHER_SYSTEM_PROMPT,
                        user_prompt=user_prompt,
                        ledger=ledger,
                        phase="phase4_coverage_patch",
                    )
                    print(f"         → looping back to 4a for another attempt")
                    continue

            print(f"  ✓ Both invariants satisfied on attempt {attempt}")
            if coverage_bullets:
                return {
                    "solution_pass": True,
                    "start_trivial_pass": start_result.get("unexpected_passes", 0),
                    "attempts_used": attempt,
                    "acceptable_start_passes": start_result.get("acceptable_passes", 0),
                    "unexpected_start_passes": start_result.get("unexpected_passes", 0),
                    "raw_start_passes": start_result.get("raw_passes", 0),
                    "coverage_direct": len(coverage_bullets) - len(missing_direct),
                    "coverage_total": len(coverage_bullets),
                    "coverage_groups_direct": group_coverage["direct_groups"],
                    "coverage_groups_partial": group_coverage["partial_groups"],
                    "coverage_groups_none": group_coverage["none_groups"],
                    "coverage_groups_total": group_coverage["total_groups"],
                    "coverage_groups_counted_direct": group_coverage.get("counted_direct_groups", 0),
                    "coverage_groups_counted_partial": group_coverage.get("counted_partial_groups", 0),
                    "coverage_groups_counted_none": group_coverage.get("counted_none_groups", 0),
                    "coverage_groups_counted_total": group_coverage.get("counted_total_groups", 0),
                    "coverage_groups_exempt_none": group_coverage.get("exempt_none_groups", 0),
                    "coverage_required_direct_groups": group_coverage["required_direct_groups"],
                    "coverage_effective_direct_groups": group_coverage["effective_direct_groups"],
                }
            return {
                "solution_pass": True,
                "start_trivial_pass": start_result.get("unexpected_passes", 0),
                "attempts_used": attempt,
                "acceptable_start_passes": start_result.get("acceptable_passes", 0),
                "unexpected_start_passes": start_result.get("unexpected_passes", 0),
                "raw_start_passes": start_result.get("raw_passes", 0),
                "coverage_direct": 0,
                "coverage_total": 0,
                "coverage_groups_direct": 0,
                "coverage_groups_partial": 0,
                "coverage_groups_none": 0,
                "coverage_groups_total": 0,
                "coverage_required_direct_groups": 0,
                "coverage_effective_direct_groups": 0,
            }
        else:
            regressed = [t["id"] for t in sol_result.failed_tests]
            print(f"         ✗ solution/ regressed after 4b: {len(regressed)} tests now failing")
            if attempt < max_attempts:
                print(f"         → looping back to 4a for another attempt")
            else:
                print(f"         → out of attempts")

    print(f"  [BAIL] Could not satisfy both invariants in {max_attempts} attempts — handing off")
    return {
        "solution_pass": False, "start_trivial_pass": None,
        "attempts_used": max_attempts,
    }


# ── Phase 5: Calibration ──────────────────────────────────────────────────────

def phase5_calibration(ctx: TaskContext, model: str, ledger: QueryLedger, run_judge: bool) -> dict:
    _phase_header(f"Phase 5 — Calibration run with {model}")

    result_path = Path(f"/tmp/{ctx.task_id}_calibration.json")
    timeout_seconds = 3600 if run_judge else 1800
    cmd = [
        sys.executable, "-m", "unrealbench.src.ue_benchmark_runner",
        "--tasks-dir", str(TASKS_DIR),
        "--task-id", ctx.task_id,
        "--agent", "claude-code",
        "--model", model,
        "--output", str(result_path),
    ]
    if not run_judge:
        cmd.append("--skip-judge")
    print(f"  Running: {' '.join(cmd)}")
    if run_judge:
        print(f"  (This takes ~5-30 min with judge enabled; streaming output below)")
    else:
        print(f"  (This takes ~5-15 min; streaming output below)")
    print()
    t0 = time.time()
    proc = subprocess.run(cmd, cwd=str(REPO_ROOT), timeout=timeout_seconds)
    elapsed = time.time() - t0
    print(f"\n  Runner exit code: {proc.returncode}  ({elapsed/60:.1f} min)")

    # The runner invokes claude-code internally; we can't cleanly pull its
    # usage metrics into our ledger from here. Record a placeholder so the
    # report shows "calibration used 1 model invocation".
    ledger.add(
        phase="phase5_calibration", provider="claude", role="calibrator",
        input_tokens=0, output_tokens=0,
    )

    if not result_path.exists():
        print(f"  [WARN] Runner did not produce result JSON at {result_path}")
        return {"solver_success": False, "task_passed": False, "workspace": None}

    record = json.loads(result_path.read_text())
    # Runner writes a list of records; take the first (single task)
    rec = record[0] if isinstance(record, list) and record else record
    if run_judge and isinstance(rec, dict):
        judge_block = rec.get("judge") or {}
        if judge_block.get("mode") == "compile_fallback":
            ledger.add(
                phase="phase5_5_judge", provider="claude", role="judge",
                input_tokens=0, output_tokens=0,
            )
        elif judge_block.get("mode") == "dense":
            ledger.add(
                phase="phase5_5_judge", provider="claude", role="judge",
                input_tokens=0, output_tokens=0,
            )
        else:
            for _ in judge_block.get("classifications", []):
                ledger.add(
                    phase="phase5_5_judge", provider="claude", role="judge",
                    input_tokens=0, output_tokens=0,
                )

    # The runner record never includes the snapshot path. Reconstruct by globbing
    # the most-recent dir matching <short_task_id>_<agent>_*. The runner wrote
    # this dir synchronously above, so the most recent match by mtime is ours.
    short_task_id = rec.get("task_id", ctx.task_id)
    agent = rec.get("agent", "claude-code")
    snapshot_dir: Optional[Path] = None
    candidates = sorted(
        (TASKS_DIR / "test_result").glob(f"{short_task_id}_{agent}_*"),
        key=lambda p: p.stat().st_mtime,
    )
    if candidates:
        snapshot_dir = candidates[-1]

    return {
        "solver_success": rec.get("solver_success", False),
        "task_passed": rec.get("task_passed", False),
        "workspace": str(snapshot_dir) if snapshot_dir else None,
        "snapshot_dir": snapshot_dir,
        "runner_record": rec,
        "duration_seconds": rec.get("duration_seconds"),
        "elapsed_minutes": elapsed / 60,
    }


def _find_latest_snapshot_dir(ctx: TaskContext) -> Optional[Path]:
    short_task_id = (ctx.spec.get("task_id") or ctx.task_id)
    candidates = sorted(
        (TASKS_DIR / "test_result").glob(f"{short_task_id}_*"),
        key=lambda p: p.stat().st_mtime,
    )
    return candidates[-1] if candidates else None


def phase5_replay_current_tests_against_snapshot(
    ctx: TaskContext,
    test_file: Path,
    ledger: QueryLedger,
) -> dict:
    _phase_header("Phase 5 — Replaying current tests against existing snapshot")

    snapshot_dir = _find_latest_snapshot_dir(ctx)
    if snapshot_dir is None:
        print("  [SKIP] no existing snapshot found for this task")
        return {"solver_success": None, "task_passed": None, "workspace": None, "snapshot_dir": None}

    print(f"  Using snapshot: {snapshot_dir.name}")
    replay_dir = ctx.log_dir / "phase5_replay_workspace"
    if replay_dir.exists():
        shutil.rmtree(replay_dir)
    shutil.copytree(snapshot_dir, replay_dir)

    from unrealbench.src.ue_benchmark_runner import UnrealBenchmarkRunner

    runner = UnrealBenchmarkRunner(
        tasks_dir=str(TASKS_DIR),
        output_file="/tmp/unused_phase5_replay.json",
        agent="claude-code",
        model="replay",
        debug=False,
    )

    uproject_path = runner._find_uproject(replay_dir)
    if uproject_path is None:
        print("  [SKIP] replay snapshot has no .uproject")
        return {"solver_success": None, "task_passed": None, "workspace": str(replay_dir), "snapshot_dir": replay_dir}

    project_name = uproject_path.stem
    test_dest = replay_dir / "Source" / project_name / "Tests"
    if test_dest.exists():
        shutil.rmtree(test_dest)
    test_dest.mkdir(parents=True, exist_ok=True)
    copied = []
    for src in sorted(ctx.tests_dir.glob("*.cpp")):
        shutil.copy2(src, test_dest / src.name)
        copied.append(src.name)
    print(f"  Injected current test file(s): {', '.join(copied)}")
    runner._patch_build_cs_for_tests(replay_dir, project_name)

    compile_success, compile_output = runner._compile_project(uproject_path)
    (replay_dir / "compile_output.txt").write_text(compile_output, encoding="utf-8")
    if not compile_success:
        print("  Compile FAILED — replay test run skipped")
        record = {
            "task_id": ctx.spec.get("task_id", ctx.task_id),
            "agent": "snapshot-replay",
            "solver_success": True,
            "compile_success": False,
            "tests_run": 0,
            "tests_passed": 0,
            "tests_failed": 0,
            "task_passed": False,
            "test_results": [],
            "failure_reason": "compile_failed",
            "replay_of_new_tests": True,
            "source_snapshot": str(snapshot_dir),
        }
        (replay_dir / "result.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
        ledger.add(
            phase="phase5_replay", provider="claude", role="calibrator",
            input_tokens=0, output_tokens=0,
        )
        return {
            "solver_success": True,
            "task_passed": False,
            "workspace": str(replay_dir),
            "snapshot_dir": replay_dir,
            "runner_record": record,
            "mode": "snapshot_replay",
        }

    tests_cfg = ctx.spec.get("tests", {}) or {}
    test_filter = tests_cfg.get("filter", "")
    num_clients = tests_cfg.get("num_clients", 1)
    print(f"  Running tests: {test_filter} ...")
    ran, test_results, unreal_log = runner._run_headless_tests(uproject_path, test_filter, num_clients)
    (replay_dir / "unreal_log.txt").write_text(unreal_log, encoding="utf-8")

    tests_run = len(test_results)
    tests_passed = sum(1 for t in test_results if t["passed"])
    tests_failed = tests_run - tests_passed
    task_passed = ran and tests_failed == 0 and tests_run > 0
    failure_reason = None
    if not ran:
        failure_reason = "test_runner_error"
    elif not task_passed:
        failure_reason = "tests_failed"

    print(f"  Replay results: {tests_passed}/{tests_run} passed")

    base_record = read_result_record(snapshot_dir)
    record = {
        "task_id": ctx.spec.get("task_id", ctx.task_id),
        "agent": base_record.get("agent", "snapshot-replay"),
        "model": base_record.get("model", ""),
        "solver_success": base_record.get("solver_success", True),
        "compile_success": True,
        "tests_run": tests_run,
        "tests_passed": tests_passed,
        "tests_failed": tests_failed,
        "task_passed": task_passed,
        "test_results": test_results,
        "failure_reason": failure_reason,
        "replay_of_new_tests": True,
        "source_snapshot": str(snapshot_dir),
        "replayed_test_file": str(test_file),
    }
    (replay_dir / "result.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")

    ledger.add(
        phase="phase5_replay", provider="claude", role="calibrator",
        input_tokens=0, output_tokens=0,
    )

    return {
        "solver_success": base_record.get("solver_success", True),
        "task_passed": task_passed,
        "workspace": str(replay_dir),
        "snapshot_dir": replay_dir,
        "runner_record": record,
        "mode": "snapshot_replay",
    }


# ── Phase 5.5: LLM-as-Judge over calibration test outcomes ───────────────────

def phase5_5_judge_calibration(
    ctx: TaskContext, phase5_result: dict, ledger: QueryLedger, test_file: Path
) -> dict:
    _phase_header("Phase 5.5 — LLM-as-judge: test outcome classification")

    snapshot_dir: Optional[Path] = phase5_result.get("snapshot_dir")
    if snapshot_dir is None or not snapshot_dir.exists():
        # Calibration was skipped (or didn't write a snapshot). Try to find the
        # most recent snapshot for this task on disk so the judge can still run.
        short_task_id = (ctx.spec.get("task_id") or ctx.task_id)
        candidates = sorted(
            (TASKS_DIR / "test_result").glob(f"{short_task_id}_*"),
            key=lambda p: p.stat().st_mtime,
        )
        if candidates:
            snapshot_dir = candidates[-1]
            print(f"  Calibration was skipped — using most recent snapshot: {snapshot_dir.name}")
        else:
            print(f"  [SKIP] no snapshot available — pass --skip-judge or run calibration first")
            return {"skipped": True, "reason": "no_snapshot"}

    persisted = read_result_record(snapshot_dir).get("judge")
    if persisted:
        print("  Using judge results already written by benchmark runner")
        normalized = normalize_persisted_judge(persisted)
        normalized["report_path"] = snapshot_dir / "result.json"
        return normalized

    try:
        result = run_unreal_judge(
            snapshot_dir=snapshot_dir,
            task_instruction=ctx.spec.get("instruction", "(none)"),
            files_to_edit=list(ctx.spec.get("files_to_edit") or []),
            solution_dir=ctx.solution_dir,
            test_file=test_file,
            evaluator_notes=ctx.evaluator_notes,
            audit_script=AUDIT_SCRIPT,
            ledger=ledger,
            phase="phase5_5_judge",
        )
    except Exception as exc:
        print(f"  [SKIP] judge call failed: {exc}")
        return {"skipped": True, "reason": "judge_failed"}

    if result.get("report_path"):
        print(f"  Judge results merged into {result['report_path']}")
    return result


# ── Phase 6: Summary ──────────────────────────────────────────────────────────

def phase6_summary(ctx: TaskContext, phase4_result: dict, phase5_result: dict, ledger: QueryLedger, judge_result: Optional[dict] = None) -> None:
    _phase_header("Phase 6 — Summary")

    lines: list[str] = []
    lines.append(f"Task:   {ctx.task_id}")
    lines.append(f"Title:  {ctx.spec.get('title', '?')}")
    lines.append("")

    # Phase 4 status
    if phase4_result.get("solution_pass") or phase4_result.get("bail_reason") == "coverage_threshold_not_met":
        lines.append("✓ Solution passes all tests")
        if phase4_result.get("bail_reason") == "coverage_threshold_not_met":
            lines.append("✗ Coverage threshold not met — major behavior coverage is too weak")
    else:
        lines.append("✗ Solution does NOT pass all tests — human intervention needed")

    trivial = phase4_result.get("start_trivial_pass")
    if trivial is None:
        lines.append("  (start/ tightening skipped — solution did not pass)")
    elif trivial == -1:
        bail_reason = phase4_result.get("bail_reason")
        if bail_reason == "start_compile_failed":
            lines.append("✗ start/ does NOT compile — the model can't iterate on a broken stub")
            lines.append("  Fix the test file to only reference declarations the stripped start/ still provides")
        elif bail_reason == "crash":
            lines.append("✗ start/ crashed during tightening — Phase 4b is inconclusive")
        elif bail_reason == "unparseable":
            lines.append("✗ start/ test results were unparseable — Phase 4b is inconclusive")
        elif bail_reason == "stuck":
            lines.append("✗ start/ still has unexpected passing tests after tightening attempts")
        else:
            lines.append("✗ start/ tightening did not converge")
    elif trivial == 0:
        acceptable = phase4_result.get("acceptable_start_passes", 0) or 0
        if acceptable:
            lines.append(f"✓ start/ compiles; all should-fail tests fail (acceptable passes: {acceptable})")
        else:
            lines.append("✓ start/ compiles and fails every test that should fail")
    else:
        acceptable = phase4_result.get("acceptable_start_passes", 0) or 0
        lines.append(f"✗ start/ still has {trivial} unexpected passing test(s)")
        if acceptable:
            lines.append(f"  Acceptable regression-style passes: {acceptable}")
    if phase4_result.get("coverage_total"):
        lines.append(
            f"ℹ Spec coverage: {phase4_result.get('coverage_direct', 0)}/"
            f"{phase4_result['coverage_total']} required behaviors directly covered"
        )
    if phase4_result.get("coverage_groups_total"):
        counted_total = phase4_result.get("coverage_groups_counted_total", phase4_result["coverage_groups_total"])
        if counted_total != phase4_result["coverage_groups_total"]:
            lines.append(
                f"ℹ Behavior-group coverage: {phase4_result.get('coverage_groups_direct', 0)}/"
                f"{phase4_result['coverage_groups_total']} direct, "
                f"{phase4_result.get('coverage_groups_partial', 0)}/"
                f"{phase4_result['coverage_groups_total']} partial, "
                f"{phase4_result.get('coverage_groups_none', 0)}/"
                f"{phase4_result['coverage_groups_total']} none "
                f"(threshold groups: {phase4_result.get('coverage_groups_counted_direct', 0)}/"
                f"{counted_total} direct, "
                f"{phase4_result.get('coverage_groups_counted_partial', 0)}/"
                f"{counted_total} partial, "
                f"{phase4_result.get('coverage_groups_counted_none', 0)}/"
                f"{counted_total} blocking none, "
                f"{phase4_result.get('coverage_groups_exempt_none', 0)}/"
                f"{phase4_result['coverage_groups_total']} exempt none, "
                f"effective {phase4_result.get('coverage_effective_direct_groups', 0):.1f}/"
                f"{phase4_result.get('coverage_required_direct_groups', 0)} required)"
            )
        else:
            lines.append(
                f"ℹ Behavior-group coverage: {phase4_result.get('coverage_groups_direct', 0)}/"
                f"{phase4_result['coverage_groups_total']} direct, "
                f"{phase4_result.get('coverage_groups_partial', 0)}/"
                f"{phase4_result['coverage_groups_total']} partial, "
                f"{phase4_result.get('coverage_groups_none', 0)}/"
                f"{phase4_result['coverage_groups_total']} none "
                f"(effective {phase4_result.get('coverage_effective_direct_groups', 0):.1f}/"
                f"{phase4_result.get('coverage_required_direct_groups', 0)} required, "
                f"{phase4_result.get('coverage_groups_counted_none', phase4_result.get('coverage_groups_none', 0))} blocking none)"
            )
    lines.append("")

    # Phase 5 status
    if phase5_result.get("solver_success") is None:
        # Skipped (either by flag or because Phase 4 failed to converge)
        if phase4_result.get("solution_pass"):
            lines.append("— Calibration: skipped (--skip-calibration)")
        else:
            lines.append("— Calibration: SKIPPED because Phase 4 did not converge")
            lines.append("  Fix the test file manually, then re-run with --skip-gen to calibrate.")
    elif phase5_result.get("mode") == "snapshot_replay":
        if phase5_result.get("task_passed"):
            lines.append("✓ Snapshot replay: current tests pass against an existing model output")
        else:
            lines.append("✓ Snapshot replay: current tests ran against an existing model output and found failures")
    elif phase5_result.get("solver_success"):
        provisional = phase4_result.get("bail_reason") == "coverage_threshold_not_met"
        if phase5_result.get("task_passed"):
            if provisional:
                lines.append("⚠ Calibration: model PASSED the task — provisional because coverage threshold was not met")
            else:
                lines.append("⚠ Calibration: model PASSED the task — consider tightening")
        else:
            if provisional:
                lines.append("✓ Calibration: model attempted but did not pass (provisional — coverage threshold was not met)")
            else:
                lines.append("✓ Calibration: model attempted but did not pass (good — task is non-trivial)")
    else:
        lines.append("✗ Calibration: solver_success=false (infra error or rate limit — rerun)")
    if phase5_result.get("workspace"):
        lines.append(f"  Workspace: {phase5_result['workspace']}")
    lines.append("")

    # Phase 5.5 — Test trust report
    current_test_count: Optional[int] = None
    try:
        current_test_count = len(
            re.findall(
                r"IMPLEMENT_SIMPLE_AUTOMATION_TEST\s*\(",
                _read_text(ctx.tests_file),
            )
        )
    except Exception:
        current_test_count = None

    judged_test_count: Optional[int] = None
    if judge_result and not judge_result.get("skipped"):
        if isinstance(judge_result.get("classifications"), list):
            judged_test_count = len(judge_result["classifications"])
        elif isinstance(judge_result.get("test_breakdown"), list):
            judged_test_count = len(judge_result["test_breakdown"])

    if (
        judge_result
        and not judge_result.get("skipped")
        and current_test_count is not None
        and judged_test_count is not None
        and current_test_count != judged_test_count
    ):
        lines.append(
            "Test trust report: SKIPPED "
            f"(stale judge snapshot: current suite has {current_test_count} tests, "
            f"cached judge result covers {judged_test_count})"
        )
        lines.append("")
        judge_result = None

    if judge_result and not judge_result.get("skipped"):
        if judge_result.get("mode") == "compile_fallback":
            counts = judge_result["counts"]
            lines.append("Capability fallback report (Claude Opus 4.7 judge):")
            lines.append(f"  CORRECT files: {counts['CORRECT']}")
            lines.append(f"  BUGGY files:   {counts['BUGGY']}")
            lines.append(f"  UNCLEAR files: {counts['UNCLEAR']}")
            lines.append(f"  Capability score: {judge_result.get('capability_score', 0.0):.0%}")
            if judge_result.get("compile_failure_root_cause"):
                lines.append(f"  Compile blocker: {judge_result['compile_failure_root_cause']}")
            if judge_result.get("overall_assessment"):
                lines.append(f"  Assessment: {judge_result['overall_assessment']}")
            lines.append("")
        elif judge_result.get("mode") == "dense":
            lines.append("Dense test audit (Claude Opus 4.7 judge):")
            lines.append(f"  Submission verdict: {judge_result.get('submission_verdict', 'UNCLEAR')}")
            lines.append(f"  Test verdict:       {judge_result.get('test_verdict', 'UNCLEAR')}")
            lines.append(f"  Confidence:         {judge_result.get('confidence', 'low')}")
            lines.append(f"  Confusion category: {judge_result.get('confusion_category', 'UNCERTAIN')}")
            if judge_result.get("rationale"):
                lines.append(f"  Rationale: {judge_result['rationale']}")
            test_breakdown = judge_result.get("test_breakdown") or []
            if test_breakdown:
                lines.append("  Test breakdown:")
                for item in test_breakdown[:4]:
                    lines.append(
                        f"    - {item.get('test_id', '?')}: {item.get('outcome', 'FAILED')} / "
                        f"{item.get('assessment', 'UNCLEAR')} — {item.get('notes', '')[:120]}"
                    )
            requirement_findings = judge_result.get("requirement_findings") or []
            if requirement_findings:
                lines.append("  Requirement findings:")
                for item in requirement_findings[:4]:
                    lines.append(
                        f"    - [{item.get('status', 'UNCLEAR')}] {item.get('requirement', '?')} — "
                        f"{item.get('evidence', '')[:120]}"
                    )
            evidence = judge_result.get("evidence") or []
            if evidence:
                lines.append("  Evidence:")
                for item in evidence[:4]:
                    lines.append(f"    - {item}")
            lines.append("")
        else:
            counts = judge_result["counts"]
            trust = judge_result["trust_score"]
            judged = counts["TP"] + counts["TN"] + counts["FP"] + counts["FN"]
            lines.append("Test trust report (Claude Opus 4.7 judge):")
            lines.append(f"  TP (test caught real bug):     {counts['TP']}")
            lines.append(f"  TN (test let good code pass):  {counts['TN']}")
            lines.append(f"  FP (test over-strict):         {counts['FP']}")
            lines.append(f"  FN (test missed real bug):     {counts['FN']}")
            lines.append(f"  Trust score: {counts['TP'] + counts['TN']}/{judged} = {trust:.0%}")

            flagged = [c for c in judge_result["classifications"] if c["classification"] in ("FP", "FN")]
            if flagged:
                lines.append("  Flagged tests (review these):")
                for c in flagged:
                    lines.append(f"    - [{c['classification']}] {c['test_id']} — {c.get('rationale', '')[:120]}")
            lines.append("")
    elif judge_result and judge_result.get("skipped"):
        lines.append(f"Test trust report: SKIPPED ({judge_result.get('reason', 'unknown')})")
        lines.append("")

    # Query report
    lines.append(f"LLM queries: {ledger.total_calls()} total")
    phase_counts = ledger.count_by_phase()
    per_phase_provider = ledger.count_by_phase_and_provider()
    for phase in sorted(phase_counts.keys()):
        provider_strs = [
            f"{prov}: {cnt}"
            for (ph, prov), cnt in per_phase_provider.items() if ph == phase
        ]
        lines.append(f"  {phase}: {phase_counts[phase]}  ({', '.join(provider_strs)})")
    lines.append(f"Estimated cost: ${ledger.estimated_cost_usd():.2f}")
    lines.append("")

    # Next steps
    lines.append("Next steps:")
    if phase5_result.get("workspace"):
        lines.append(f"  - Manually review model output at {phase5_result['workspace']}")
    if judge_result and judge_result.get("report_path"):
        lines.append(f"  - Test trust report: {judge_result['report_path']}")
    lines.append(f"  - Per-phase logs: {ctx.log_dir}/")

    summary = "\n".join(lines)
    print(summary)

    (ctx.log_dir / "summary.md").write_text(
        f"# Import Summary — {ctx.task_id}\n\n```\n{summary}\n```\n",
        encoding="utf-8",
    )
    print(f"\n  Written to {ctx.log_dir}/summary.md")


# ── Entry ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Automated test generation + calibration for Unreal benchmark tasks",
    )
    parser.add_argument(
        "--task-id", required=True,
        help="Task ID, e.g. ue_task_0020_inventory_system",
    )
    parser.add_argument(
        "--calibration-model", default="claude-sonnet-4-6",
        help="Frontier model for Phase 5 calibration (default: claude-sonnet-4-6)",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Run Phases 1-2 only (validate + API discovery); skip LLM generation and audits",
    )
    parser.add_argument(
        "--skip-gen", action="store_true",
        help="Skip Phase 3 (test generation) — use the existing test file at "
             "tests/<TaskName>Tests.cpp. Starts directly at Phase 4 audit loop.",
    )
    parser.add_argument(
        "--skip-audit", action="store_true",
        help="Skip Phase 4 (audit loops) — go straight to Phase 5 calibration using "
             "the existing test file. Only safe when tests are already validated "
             "(solution passes, start fails). Implies --skip-gen.",
    )
    parser.add_argument(
        "--skip-calibration", action="store_true",
        help="Skip Phase 5 (model calibration run). Useful when you just want to "
             "iterate on tests and save the 10-15 min calibration cost.",
    )
    parser.add_argument(
        "--skip-judge", action="store_true",
        help="Skip Phase 5.5 (LLM-as-judge over calibration test outcomes). "
             "Default: judge runs after calibration. When --skip-calibration is "
             "set, the judge runs against the most recent snapshot in "
             "tasks_unreal/test_result/ matching this task.",
    )
    parser.add_argument(
        "--evaluator-notes", type=Path, default=None, metavar="PATH",
        help="Path to a private evaluator notes file (e.g. benchmarks_private/<system>_evaluator.md). "
             "Contents are fed to Phase 3 and Phase 4 LLM prompts as oracle context, "
             "NEVER persisted in the unrealbench repo. Keep this file outside the repo "
             "to avoid answer-key contamination.",
    )
    parser.add_argument(
        "--authoring-mode",
        choices=["llm_json", "cli_agent"],
        default="llm_json",
        help="How to generate and repair tests in Phases 3-4. "
             "`llm_json` uses the existing multi-model JSON pipeline. "
             "`cli_agent` uses Codex CLI to author and repair the test file directly.",
    )
    parser.add_argument(
        "--authoring-agent",
        choices=["codex", "claude-code"],
        default="codex",
        help="CLI agent backend for --authoring-mode cli_agent.",
    )
    parser.add_argument(
        "--authoring-model",
        default=None,
        help="Model name for the selected authoring backend. "
             "For `llm_json`, this is ignored. For `cli_agent`, this is passed to Codex CLI.",
    )
    args = parser.parse_args()

    ledger = QueryLedger()
    authoring = AuthoringConfig(
        mode=args.authoring_mode,
        agent=args.authoring_agent,
        model=args.authoring_model,
    )

    # Phase 1: validation
    ctx = phase1_validate(args.task_id, evaluator_notes_path=args.evaluator_notes)

    # Phase 2: API discovery (always runs, cheap)
    api_surface = phase2_api_discovery(ctx, ledger)

    if args.dry_run:
        print("\n[DRY RUN] Stopping before LLM generation.")
        return

    # Phase 3: test generation (skippable)
    candidates = list(ctx.tests_dir.glob("*.cpp"))
    if len(candidates) > 1:
        sys.exit(
            f"  [FAIL] {ctx.tests_dir} contains multiple .cpp files: {[c.name for c in candidates]}\n"
            f"         The audit pipeline injects ALL of them, which causes duplicate-class compile\n"
            f"         errors. Keep exactly one .cpp file in tests/ (delete or rename the others)."
        )

    if args.skip_gen or args.skip_audit:
        if not candidates:
            sys.exit(f"  [FAIL] --skip-gen/--skip-audit but no test file in {ctx.tests_dir}")
        test_file = candidates[0]
        _phase_header(f"Phase 3 — SKIPPED (using existing {test_file.name})")
        print(f"  Existing test file: {test_file}  ({test_file.stat().st_size} bytes)")
    else:
        test_file = phase3_test_generation(ctx, api_surface, ledger, authoring)

    # Phase 4: audit loops (skippable when tests are already validated)
    if args.skip_audit:
        _phase_header("Phase 4 — SKIPPED (--skip-audit)")
        print("  Assuming existing test file is already validated.")
        print("  WARNING: only use --skip-audit when solution/ passes and start/ fails correctly.")
        phase4_result = {"solution_pass": True, "start_trivial_pass": 0, "acceptable_passes": 0}
    else:
        phase4_result = phase4_audit_loops(ctx, test_file, api_surface, ledger, authoring)

    # Phase 5: calibration (skippable, or auto-skipped if Phase 4 didn't converge)
    coverage_only_miss = (
        phase4_result.get("bail_reason") == "coverage_threshold_not_met"
        and phase4_result.get("start_trivial_pass") == 0
    )
    if args.skip_calibration:
        if phase4_result.get("solution_pass") or coverage_only_miss:
            phase5_result = phase5_replay_current_tests_against_snapshot(ctx, test_file, ledger)
        else:
            _phase_header("Phase 5 — SKIPPED (--skip-calibration)")
            phase5_result = {"solver_success": None, "task_passed": None, "workspace": None}
    elif not phase4_result.get("solution_pass") and not coverage_only_miss:
        _phase_header("Phase 5 — SKIPPED (Phase 4 did not converge)")
        print("  The test file is not validated — running the calibration model against")
        print("  unvalidated tests wastes tokens and produces meaningless results.")
        print("  Fix Phase 4 output manually, then re-run with --skip-gen to calibrate.")
        phase5_result = {"solver_success": None, "task_passed": None, "workspace": None}
    elif coverage_only_miss:
        _phase_header(f"Phase 5 — Calibration run with {args.calibration_model} (coverage threshold not met)")
        print("  Running calibration anyway because solution/ passed and start/ discrimination converged.")
        print("  Treat the result as provisional until coverage is improved.")
        phase5_result = phase5_calibration(ctx, args.calibration_model, ledger, run_judge=not args.skip_judge)
    else:
        phase5_result = phase5_calibration(ctx, args.calibration_model, ledger, run_judge=not args.skip_judge)

    # Phase 5.5: LLM-as-judge over calibration outcomes. Default ON; the judge
    # itself decides to skip if no snapshot is available.
    judge_result: Optional[dict] = None
    if args.skip_judge:
        _phase_header("Phase 5.5 — SKIPPED (--skip-judge)")
    else:
        judge_result = phase5_5_judge_calibration(ctx, phase5_result, ledger, test_file)

    # Phase 6: summary
    phase6_summary(ctx, phase4_result, phase5_result, ledger, judge_result=judge_result)


if __name__ == "__main__":
    main()
