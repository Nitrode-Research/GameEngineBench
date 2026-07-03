#!/usr/bin/env python3
"""Validate and merge S3-staged UnrealBench worker artifacts.

The runner and analysis code operate on local snapshot directories under
tasks_unreal/test_result. This script keeps S3 as the transport layer: sync the
run prefix to incoming/<run-id>/workers first, then use this script to validate
and copy worker snapshots into the canonical local test_result directory.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SNAPSHOT_RE = re.compile(r"^(ue_task_\d{4})_([^_]+)_([0-9]{8}_[0-9]{6})$")


@dataclass(frozen=True)
class Snapshot:
    worker: str
    path: Path
    result_path: Path
    data: dict[str, Any]

    @property
    def name(self) -> str:
        return self.path.name

    @property
    def task_id(self) -> str:
        return str(self.data.get("task_id") or "")

    @property
    def agent(self) -> str:
        return str(self.data.get("agent") or "")

    @property
    def model(self) -> str:
        return str(self.data.get("model") or "")

    @property
    def reasoning(self) -> str:
        return str(
            self.data.get("reasoning_level_applied")
            or self.data.get("reasoning_level_requested")
            or "default"
        )

    @property
    def config_key(self) -> tuple[str, str, str, str]:
        return (self.task_id, self.agent, self.model, self.reasoning)


def _load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def _hash_file(path: Path) -> str:
    import hashlib

    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def _expand_tasks(value: Any) -> set[str]:
    if value is None:
        return set()
    if isinstance(value, (str, int)):
        items = [value]
    else:
        items = list(value)

    out: list[int] = []
    for item in items:
        s = str(item).strip()
        if not s:
            continue
        if s.startswith("ue_task_"):
            out.append(int(s.rsplit("_", 1)[-1]))
        elif "-" in s:
            lo, hi = s.split("-", 1)
            out.extend(range(int(lo), int(hi) + 1))
        else:
            out.append(int(s))
    return {f"ue_task_{n:04d}" for n in dict.fromkeys(out)}


def load_shard_plan(path: Path | None) -> dict[str, dict[str, Any]]:
    if path is None:
        return {}
    raw = _load_json(path)
    if isinstance(raw, list):
        entries = raw
    elif isinstance(raw, dict) and isinstance(raw.get("shards"), list):
        entries = raw["shards"]
    elif isinstance(raw, dict) and isinstance(raw.get("workers"), dict):
        entries = []
        for worker, spec in raw["workers"].items():
            item = dict(spec or {})
            item.setdefault("worker", worker)
            entries.append(item)
    else:
        raise ValueError(
            "Shard plan must be a list, {'shards': [...]}, or {'workers': {...}}"
        )

    plan: dict[str, dict[str, Any]] = {}
    for entry in entries:
        worker = str(entry.get("worker") or entry.get("worker_id") or "").strip()
        if not worker:
            raise ValueError(f"Shard entry is missing worker: {entry}")
        tasks = _expand_tasks(entry.get("tasks", entry.get("task_ids")))
        plan[worker] = {
            "tasks": tasks,
            "agent": entry.get("agent"),
            "model": entry.get("model"),
            "reasoning": entry.get("reasoning", entry.get("reasoning_level")),
            "mode": entry.get("mode"),
        }
    return plan


def discover_snapshots(workers_root: Path) -> tuple[list[Snapshot], list[str]]:
    errors: list[str] = []
    snapshots: list[Snapshot] = []
    if not workers_root.exists():
        return [], [f"Workers root does not exist: {workers_root}"]

    worker_dirs = [p for p in sorted(workers_root.iterdir()) if p.is_dir()]
    for worker_dir in worker_dirs:
        test_result = worker_dir / "test_result"
        if not test_result.exists():
            errors.append(f"{worker_dir.name}: missing test_result directory")
            continue
        for snap_dir in sorted(p for p in test_result.iterdir() if p.is_dir()):
            result_path = snap_dir / "result.json"
            if not result_path.exists():
                errors.append(f"{worker_dir.name}/{snap_dir.name}: missing result.json")
                continue
            try:
                data = _load_json(result_path)
            except Exception as exc:
                errors.append(f"{worker_dir.name}/{snap_dir.name}: invalid result.json: {exc}")
                continue
            snapshots.append(
                Snapshot(
                    worker=worker_dir.name,
                    path=snap_dir,
                    result_path=result_path,
                    data=data,
                )
            )
    return snapshots, errors


def validate_snapshot_names(snapshots: list[Snapshot]) -> list[str]:
    errors: list[str] = []
    for snap in snapshots:
        m = SNAPSHOT_RE.match(snap.name)
        if not m:
            errors.append(f"{snap.worker}/{snap.name}: unexpected snapshot directory name")
            continue
        task_id, agent, _ = m.groups()
        if snap.task_id != task_id:
            errors.append(
                f"{snap.worker}/{snap.name}: task_id mismatch "
                f"name={task_id} result={snap.task_id}"
            )
        if snap.agent and snap.agent != agent:
            errors.append(
                f"{snap.worker}/{snap.name}: agent mismatch "
                f"name={agent} result={snap.agent}"
            )
    return errors


def validate_duplicates(snapshots: list[Snapshot], allow_duplicates: bool) -> list[str]:
    errors: list[str] = []
    by_name: dict[str, list[Snapshot]] = {}
    by_key: dict[tuple[str, str, str, str], list[Snapshot]] = {}
    for snap in snapshots:
        by_name.setdefault(snap.name, []).append(snap)
        by_key.setdefault(snap.config_key, []).append(snap)

    for name, group in sorted(by_name.items()):
        if len(group) > 1:
            owners = ", ".join(f"{s.worker}:{s.path}" for s in group)
            errors.append(f"duplicate snapshot directory name {name}: {owners}")

    if not allow_duplicates:
        for key, group in sorted(by_key.items()):
            if len(group) > 1:
                owners = ", ".join(f"{s.worker}:{s.name}" for s in group)
                errors.append(f"duplicate incoming config {key}: {owners}")
    return errors


def validate_shard_plan(
    snapshots: list[Snapshot], plan: dict[str, dict[str, Any]]
) -> tuple[list[str], dict[str, Any]]:
    errors: list[str] = []
    report: dict[str, Any] = {}
    if not plan:
        return errors, report

    seen_by_worker: dict[str, set[str]] = {worker: set() for worker in plan}
    for snap in snapshots:
        spec = plan.get(snap.worker)
        if spec is None:
            errors.append(f"{snap.worker}/{snap.name}: worker is not in shard plan")
            continue
        expected_tasks = spec.get("tasks") or set()
        if expected_tasks and snap.task_id not in expected_tasks:
            errors.append(
                f"{snap.worker}/{snap.name}: task {snap.task_id} is outside assigned shard"
            )
        seen_by_worker.setdefault(snap.worker, set()).add(snap.task_id)

        for field in ("agent", "model"):
            expected = spec.get(field)
            actual = getattr(snap, field)
            if expected and str(expected) != str(actual):
                errors.append(
                    f"{snap.worker}/{snap.name}: {field} mismatch "
                    f"expected={expected} actual={actual}"
                )
        expected_reasoning = spec.get("reasoning")
        if expected_reasoning and str(expected_reasoning) != snap.reasoning:
            errors.append(
                f"{snap.worker}/{snap.name}: reasoning mismatch "
                f"expected={expected_reasoning} actual={snap.reasoning}"
            )

    missing: dict[str, list[str]] = {}
    for worker, spec in plan.items():
        expected_tasks = spec.get("tasks") or set()
        if expected_tasks:
            missed = sorted(expected_tasks - seen_by_worker.get(worker, set()))
            if missed:
                missing[worker] = missed
    report["missing_tasks"] = missing
    return errors, report


def copy_snapshots(
    snapshots: list[Snapshot],
    dest_root: Path,
    apply: bool,
    update_existing: bool,
) -> tuple[list[str], list[dict[str, str]]]:
    errors: list[str] = []
    actions: list[dict[str, str]] = []
    dest_root.mkdir(parents=True, exist_ok=True) if apply else None

    for snap in snapshots:
        dest = dest_root / snap.name
        if dest.exists():
            dest_result = dest / "result.json"
            if dest_result.exists() and _hash_file(dest_result) == _hash_file(snap.result_path):
                actions.append(
                    {
                        "action": "skip-identical",
                        "worker": snap.worker,
                        "snapshot": snap.name,
                        "task_id": snap.task_id,
                        "agent": snap.agent,
                        "model": snap.model,
                        "reasoning": snap.reasoning,
                        "source_path": str(snap.path),
                        "dest_path": str(dest),
                    }
                )
                continue
            if not update_existing:
                errors.append(
                    f"{snap.worker}/{snap.name}: destination exists and differs; "
                    "use --update-existing for retest updates"
                )
                continue
            action = "update-existing"
            if apply:
                shutil.copytree(snap.path, dest, dirs_exist_ok=True)
        else:
            action = "copy-new"
            if apply:
                shutil.copytree(snap.path, dest)
        actions.append(
            {
                "action": action,
                "worker": snap.worker,
                "snapshot": snap.name,
                "task_id": snap.task_id,
                "agent": snap.agent,
                "model": snap.model,
                "reasoning": snap.reasoning,
                "source_path": str(snap.path),
                "dest_path": str(dest),
            }
        )
    return errors, actions


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Validate and merge S3-staged UnrealBench worker results."
    )
    parser.add_argument(
        "--incoming",
        required=True,
        type=Path,
        help="Path to synced workers directory, e.g. incoming/<run-id>/workers",
    )
    parser.add_argument(
        "--dest",
        type=Path,
        default=Path("tasks_unreal/test_result"),
        help="Canonical local test_result directory",
    )
    parser.add_argument(
        "--shard-plan",
        type=Path,
        default=None,
        help="Optional shard_plan.json to validate worker ownership",
    )
    parser.add_argument(
        "--report",
        type=Path,
        default=None,
        help="Optional path for JSON merge report",
    )
    parser.add_argument(
        "--run-id",
        default=None,
        help="Optional run identifier to record in the merge report.",
    )
    parser.add_argument(
        "--apply",
        action="store_true",
        help="Actually copy snapshots into --dest. Without this, validate only.",
    )
    parser.add_argument(
        "--update-existing",
        action="store_true",
        help="Allow an incoming snapshot to update an existing destination snapshot.",
    )
    parser.add_argument(
        "--allow-duplicates",
        action="store_true",
        help="Allow multiple incoming snapshots for the same task/agent/model/reasoning.",
    )
    args = parser.parse_args()

    snapshots, errors = discover_snapshots(args.incoming)
    errors.extend(validate_snapshot_names(snapshots))
    errors.extend(validate_duplicates(snapshots, args.allow_duplicates))

    plan = load_shard_plan(args.shard_plan) if args.shard_plan else {}
    plan_errors, plan_report = validate_shard_plan(snapshots, plan)
    errors.extend(plan_errors)

    if errors and args.apply:
        actions = []
    else:
        copy_errors, actions = copy_snapshots(
            snapshots=snapshots,
            dest_root=args.dest,
            apply=args.apply,
            update_existing=args.update_existing,
        )
        errors.extend(copy_errors)

    by_worker: dict[str, int] = {}
    by_config: dict[str, int] = {}
    for snap in snapshots:
        by_worker[snap.worker] = by_worker.get(snap.worker, 0) + 1
        key = " / ".join(snap.config_key)
        by_config[key] = by_config.get(key, 0) + 1

    report = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "run_id": args.run_id,
        "incoming": str(args.incoming),
        "dest": str(args.dest),
        "apply": args.apply,
        "update_existing": args.update_existing,
        "snapshot_count": len(snapshots),
        "by_worker": by_worker,
        "by_config": by_config,
        "actions": actions,
        "errors": errors,
        **plan_report,
    }

    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"Discovered {len(snapshots)} snapshot(s) from {args.incoming}")
    for worker, count in sorted(by_worker.items()):
        print(f"  {worker}: {count}")
    action_counts: dict[str, int] = {}
    for action in actions:
        action_counts[action["action"]] = action_counts.get(action["action"], 0) + 1
    if action_counts:
        print("Actions:")
        for action, count in sorted(action_counts.items()):
            print(f"  {action}: {count}")
    if plan_report.get("missing_tasks"):
        print("Missing tasks:")
        for worker, tasks in sorted(plan_report["missing_tasks"].items()):
            print(f"  {worker}: {', '.join(tasks)}")

    if errors:
        print("\nErrors:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    if not args.apply:
        print("Validation passed. Re-run with --apply to copy snapshots.")
    else:
        print("Merge completed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
