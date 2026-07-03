#!/usr/bin/env python3
"""Validate canonical dataset and create deterministic train/val splits."""

from __future__ import annotations

import argparse
import hashlib
import json
import random
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Tuple


REQUIRED_TOP_LEVEL = [
    "example_id",
    "source",
    "task",
    "context",
    "trajectory",
    "outcome",
    "leakage",
]


@dataclass
class ValidationIssue:
    line: int
    example_id: str
    message: str


def _read_jsonl(path: Path) -> List[Dict[str, Any]]:
    records: List[Dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as f:
        for i, raw in enumerate(f, start=1):
            raw = raw.strip()
            if not raw:
                continue
            try:
                item = json.loads(raw)
            except json.JSONDecodeError as exc:
                raise ValueError(f"Invalid JSON on line {i}: {exc}") from exc
            if not isinstance(item, dict):
                raise ValueError(f"Line {i} must be a JSON object")
            records.append(item)
    return records


def _sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _validate_example(example: Dict[str, Any], line: int) -> List[ValidationIssue]:
    issues: List[ValidationIssue] = []
    ex_id = str(example.get("example_id", "<missing>"))

    for key in REQUIRED_TOP_LEVEL:
        if key not in example:
            issues.append(ValidationIssue(line, ex_id, f"Missing top-level field: {key}"))

    task = example.get("task", {})
    if not isinstance(task, dict) or not task.get("description"):
        issues.append(ValidationIssue(line, ex_id, "task.description is required"))

    trajectory = example.get("trajectory")
    if not isinstance(trajectory, list) or not trajectory:
        issues.append(ValidationIssue(line, ex_id, "trajectory must be a non-empty list"))
    else:
        for idx, step in enumerate(trajectory, start=1):
            if not isinstance(step, dict):
                issues.append(ValidationIssue(line, ex_id, f"trajectory[{idx}] must be an object"))
                continue
            for required in ["intent", "action"]:
                if not step.get(required):
                    issues.append(ValidationIssue(line, ex_id, f"trajectory[{idx}].{required} is required"))

    outcome = example.get("outcome", {})
    if not isinstance(outcome, dict) or not outcome.get("status"):
        issues.append(ValidationIssue(line, ex_id, "outcome.status is required"))

    leakage = example.get("leakage", {})
    if not isinstance(leakage, dict):
        issues.append(ValidationIssue(line, ex_id, "leakage must be an object"))
    else:
        if leakage.get("overlap_with_eval") is True:
            issues.append(
                ValidationIssue(line, ex_id, "leakage.overlap_with_eval=true (example blocked)")
            )

    return issues


def _split_records(
    records: List[Dict[str, Any]],
    val_ratio: float,
    seed: int,
) -> Tuple[List[Dict[str, Any]], List[Dict[str, Any]]]:
    shuffled = list(records)
    rnd = random.Random(seed)
    rnd.shuffle(shuffled)

    val_count = int(len(shuffled) * val_ratio)
    if len(shuffled) > 1:
        val_count = max(1, min(val_count, len(shuffled) - 1))

    val = shuffled[:val_count]
    train = shuffled[val_count:]
    return train, val


def _write_jsonl(path: Path, records: List[Dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        for item in records:
            f.write(json.dumps(item, ensure_ascii=True) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description="Validate/split canonical fine-tune dataset")
    parser.add_argument("--input", required=True, help="Input canonical JSONL")
    parser.add_argument("--train-out", required=True, help="Output train canonical JSONL")
    parser.add_argument("--val-out", required=True, help="Output val canonical JSONL")
    parser.add_argument("--manifest-out", required=True, help="Output manifest JSON")
    parser.add_argument("--val-ratio", type=float, default=0.1, help="Validation split ratio")
    parser.add_argument("--seed", type=int, default=42, help="Split seed")
    args = parser.parse_args()

    input_path = Path(args.input)
    train_path = Path(args.train_out)
    val_path = Path(args.val_out)
    manifest_path = Path(args.manifest_out)

    records = _read_jsonl(input_path)
    if not records:
        raise ValueError(f"No records found in {input_path}")

    all_issues: List[ValidationIssue] = []
    id_seen = set()
    clean_records: List[Dict[str, Any]] = []

    for line, item in enumerate(records, start=1):
        issues = _validate_example(item, line)
        ex_id = str(item.get("example_id", "<missing>"))

        if ex_id in id_seen:
            issues.append(ValidationIssue(line, ex_id, "duplicate example_id"))
        else:
            id_seen.add(ex_id)

        all_issues.extend(issues)
        if not issues:
            clean_records.append(item)

    blocked = [issue for issue in all_issues if "blocked" in issue.message]
    if blocked:
        print(f"Blocked {len(blocked)} examples due to leakage flags")

    if not clean_records:
        raise ValueError("No valid records remain after validation")

    train_records, val_records = _split_records(clean_records, args.val_ratio, args.seed)

    _write_jsonl(train_path, train_records)
    _write_jsonl(val_path, val_records)

    manifest = {
        "input": str(input_path),
        "input_sha256": _sha256_file(input_path),
        "train": str(train_path),
        "val": str(val_path),
        "train_count": len(train_records),
        "val_count": len(val_records),
        "valid_count": len(clean_records),
        "invalid_count": len(records) - len(clean_records),
        "seed": args.seed,
        "val_ratio": args.val_ratio,
        "issues": [issue.__dict__ for issue in all_issues],
    }

    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    print(f"Input examples: {len(records)}")
    print(f"Valid examples: {len(clean_records)}")
    print(f"Train: {len(train_records)} -> {train_path}")
    print(f"Val: {len(val_records)} -> {val_path}")
    print(f"Manifest: {manifest_path}")


if __name__ == "__main__":
    main()
