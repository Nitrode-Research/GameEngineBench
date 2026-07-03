#!/usr/bin/env python3
"""Convert canonical trajectory JSONL to OpenAI chat fine-tuning JSONL."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, List


SYSTEM_PROMPT = (
    "You are an expert game-development coding assistant working in a repository. "
    "Respond with concise planning, targeted code changes, and explicit debugging/repair steps."
)


def _read_jsonl(path: Path) -> List[Dict[str, Any]]:
    items: List[Dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as f:
        for line_no, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                parsed = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(f"Invalid JSON on line {line_no}: {exc}") from exc
            if not isinstance(parsed, dict):
                raise ValueError(f"Line {line_no} must contain a JSON object")
            items.append(parsed)
    return items


def _task_to_user_prompt(item: Dict[str, Any]) -> str:
    task = item.get("task", {})
    context = item.get("context", {})

    title = task.get("title", "Untitled task")
    description = task.get("description", "")
    skills = task.get("skills", [])
    relevant_files = context.get("relevant_files", [])
    constraints = context.get("constraints", [])

    lines = [
        f"Task: {title}",
        f"Description: {description}",
    ]
    if skills:
        lines.append("Skills: " + ", ".join(str(s) for s in skills))
    if relevant_files:
        lines.append("Relevant files: " + ", ".join(str(p) for p in relevant_files))
    if constraints:
        lines.append("Constraints: " + "; ".join(str(c) for c in constraints))

    return "\n".join(lines)


def _trajectory_to_assistant_answer(item: Dict[str, Any]) -> str:
    trajectory = item.get("trajectory", [])
    outcome = item.get("outcome", {})

    plan_lines = ["Plan and execution:"]
    for step in trajectory:
        intent = step.get("intent", "")
        action = step.get("action", "")
        observation = step.get("observation", "")
        next_decision = step.get("next_decision", "")

        line = f"- [{intent}] {action}".strip()
        plan_lines.append(line)

        if observation:
            plan_lines.append(f"  Observation: {observation}")
        if next_decision:
            plan_lines.append(f"  Next: {next_decision}")

    plan_lines.append("")
    plan_lines.append("Outcome:")
    plan_lines.append(f"- status: {outcome.get('status', 'unknown')}")
    plan_lines.append(f"- tests_passed: {outcome.get('tests_passed', False)}")

    failure_mode = outcome.get("failure_mode")
    if failure_mode:
        plan_lines.append(f"- failure_mode: {failure_mode}")

    notes = outcome.get("notes")
    if notes:
        plan_lines.append(f"- notes: {notes}")

    return "\n".join(plan_lines)


def _convert(item: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "messages": [
            {"role": "system", "content": SYSTEM_PROMPT},
            {"role": "user", "content": _task_to_user_prompt(item)},
            {"role": "assistant", "content": _trajectory_to_assistant_answer(item)},
        ]
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Export canonical dataset to OpenAI chat JSONL")
    parser.add_argument("--input", required=True, help="Input canonical JSONL")
    parser.add_argument("--output", required=True, help="Output OpenAI JSONL")
    args = parser.parse_args()

    input_path = Path(args.input)
    output_path = Path(args.output)

    items = _read_jsonl(input_path)
    converted = [_convert(item) for item in items]

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as f:
        for row in converted:
            f.write(json.dumps(row, ensure_ascii=True) + "\n")

    print(f"Exported {len(converted)} rows to {output_path}")


if __name__ == "__main__":
    main()
