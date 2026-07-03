#!/usr/bin/env python3
"""Convert benchmark test_result trajectories into canonical fine-tune JSONL."""

from __future__ import annotations

import argparse
import ast
import json
import re
from pathlib import Path
from typing import Any, Dict, List, Optional


TOOL_USE_RE = re.compile(
    r"ToolUseBlock\(id='(?P<id>[^']+)', name='(?P<name>[^']+)', input=(?P<input>\{.*?\})\)",
    re.DOTALL,
)
TOOL_RESULT_RE = re.compile(
    r"ToolResultBlock\(tool_use_id='(?P<id>[^']+)', content='(?P<content>.*?)', is_error=(?P<is_error>[^\)]*)\)",
    re.DOTALL,
)
ASSISTANT_TEXT_RE = re.compile(
    r"AssistantMessage\(content=\[TextBlock\(text=\"(?P<text>.*?)\"\)\]",
    re.DOTALL,
)


def _load_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _clip(text: str, max_chars: int = 600) -> str:
    text = text.replace("\\n", "\n").strip()
    if len(text) <= max_chars:
        return text
    return text[:max_chars] + "..."


def _parse_tool_uses(log_text: str) -> List[Dict[str, Any]]:
    uses: List[Dict[str, Any]] = []
    for m in TOOL_USE_RE.finditer(log_text):
        block_input = m.group("input")
        parsed_input: Dict[str, Any] = {}
        try:
            parsed = ast.literal_eval(block_input)
            if isinstance(parsed, dict):
                parsed_input = parsed
        except Exception:
            parsed_input = {"raw": _clip(block_input, 300)}

        uses.append(
            {
                "id": m.group("id"),
                "name": m.group("name"),
                "input": parsed_input,
            }
        )
    return uses


def _parse_tool_results(log_text: str) -> Dict[str, str]:
    results: Dict[str, str] = {}
    for m in TOOL_RESULT_RE.finditer(log_text):
        results[m.group("id")] = _clip(m.group("content"), 500)
    return results


def _extract_initial_plan(log_text: str) -> Optional[str]:
    m = ASSISTANT_TEXT_RE.search(log_text)
    if not m:
        return None
    return _clip(m.group("text"), 400)


def _intent_from_tool(name: str, inp: Dict[str, Any]) -> str:
    if name in {"Edit", "Write", "NotebookEdit"}:
        return "edit"
    if name == "TodoWrite":
        return "plan"
    if name in {"Bash", "Read", "Glob", "Grep", "WebSearch", "WebFetch"}:
        cmd = str(inp.get("command", ""))
        if "godot" in cmd or "pytest" in cmd or "test" in cmd:
            return "run"
        return "inspect"
    return "inspect"


def _action_from_tool(name: str, inp: Dict[str, Any]) -> str:
    if "description" in inp and inp["description"]:
        return f"{name}: {inp['description']}"
    if "file_path" in inp and inp["file_path"]:
        return f"{name}: read {inp['file_path']}"
    if "command" in inp and inp["command"]:
        return f"{name}: {inp['command']}"
    return f"{name}"


def _extract_relevant_files(tool_uses: List[Dict[str, Any]], max_files: int = 20) -> List[str]:
    files: List[str] = []
    seen = set()
    for u in tool_uses:
        inp = u.get("input", {})
        fp = inp.get("file_path")
        if isinstance(fp, str) and fp and fp not in seen:
            seen.add(fp)
            files.append(fp)
            if len(files) >= max_files:
                break
    return files


def _to_example(run_dir: Path) -> Optional[Dict[str, Any]]:
    task_config_path = run_dir / "task_config.json"
    result_path = run_dir / "result.json"
    log_path = run_dir / "agent_trajectory.log"

    if not (task_config_path.exists() and result_path.exists() and log_path.exists()):
        return None

    task_cfg = _load_json(task_config_path)
    result = _load_json(result_path)
    log_text = log_path.read_text(encoding="utf-8", errors="replace")

    tool_uses = _parse_tool_uses(log_text)
    tool_results = _parse_tool_results(log_text)

    trajectory: List[Dict[str, Any]] = []
    plan_text = _extract_initial_plan(log_text)
    step_idx = 1

    if plan_text:
        trajectory.append(
            {
                "step": step_idx,
                "intent": "plan",
                "action": plan_text,
                "observation": "",
                "next_decision": "Inspect project files and implement task requirements.",
            }
        )
        step_idx += 1

    for use in tool_uses[:80]:
        inp = use.get("input", {})
        obs = tool_results.get(use["id"], "")
        trajectory.append(
            {
                "step": step_idx,
                "intent": _intent_from_tool(use["name"], inp),
                "action": _action_from_tool(use["name"], inp),
                "observation": obs,
                "next_decision": "Continue toward task completion based on the latest output.",
            }
        )
        step_idx += 1

    if not trajectory:
        trajectory = [
            {
                "step": 1,
                "intent": "inspect",
                "action": "Read task instructions and attempted solution logs.",
                "observation": "No structured tool trace was parsed from the trajectory log.",
                "next_decision": "Apply targeted edits and verify behavior with tests.",
            }
        ]

    val = result.get("validation", {})
    solver = result.get("solver", {})

    validation_success = bool(val.get("success", False))
    status = "success" if validation_success else "failure"

    example = {
        "example_id": f"test_result::{run_dir.name}",
        "source": "unrealbench_test_result",
        "task": {
            "title": result.get("task_name", run_dir.name),
            "description": task_cfg.get("instruction", ""),
            "skills": ["godot", "gameplay", "debugging"],
        },
        "context": {
            "repo_snapshot_id": run_dir.name,
            "relevant_files": _extract_relevant_files(tool_uses),
            "constraints": ["Derived from benchmark trajectory logs"],
        },
        "trajectory": trajectory,
        "outcome": {
            "status": status,
            "tests_passed": validation_success,
            "failure_mode": "validation_failed" if not validation_success else "",
            "notes": _clip(val.get("message", ""), 400),
        },
        "leakage": {
            "overlap_with_eval": True,
            "overlap_reason": "Derived from UnrealBench benchmark task trajectory.",
        },
        "metadata": {
            "agent": result.get("agent", ""),
            "model": result.get("model", ""),
            "solver_success": bool(solver.get("success", False)),
            "solver_duration_seconds": solver.get("duration_seconds", 0.0),
            "cost_usd": solver.get("cost_usd", 0.0),
            "source_dir": str(run_dir),
        },
    }
    return example


def main() -> None:
    parser = argparse.ArgumentParser(description="Convert tasks/test_result trajectories to canonical JSONL")
    parser.add_argument(
        "--input-dir",
        default="tasks/test_result",
        help="Directory containing benchmark run subdirectories",
    )
    parser.add_argument(
        "--output",
        default="finetune/data/canonical/from_test_results.jsonl",
        help="Output canonical JSONL path",
    )
    parser.add_argument("--limit", type=int, default=20, help="Max examples to export")
    parser.add_argument(
        "--require-validation-success",
        action="store_true",
        help="Only include runs with validation.success=true",
    )
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output_path = Path(args.output)

    run_dirs = sorted([p for p in input_dir.iterdir() if p.is_dir()])

    examples: List[Dict[str, Any]] = []
    for run_dir in run_dirs:
        if len(examples) >= args.limit:
            break
        example = _to_example(run_dir)
        if not example:
            continue
        if args.require_validation_success and not example["outcome"]["tests_passed"]:
            continue
        examples.append(example)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as f:
        for ex in examples:
            f.write(json.dumps(ex, ensure_ascii=True) + "\n")

    print(f"Wrote {len(examples)} examples to {output_path}")
    if examples:
        passed = sum(1 for e in examples if e["outcome"]["tests_passed"])
        print(f"Validation-passed examples: {passed}/{len(examples)}")


if __name__ == "__main__":
    main()
