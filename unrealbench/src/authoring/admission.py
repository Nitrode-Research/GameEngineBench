"""
Admission gate CLI for benchmark tasks.

Subcommands:
  check        Run admission check for a single task
  check-all    Run admission check for all ue_task_* tasks in a directory
  approve      Promote a task to 'stable' (sets readiness flags + lifecycle status)
  status       Print current lifecycle status and readiness for one or all tasks

Usage:
    python -m unrealbench.src.authoring.admission check \
        --task-dir tasks_unreal/ue_task_0001_day_night_cycle --mode code_only

    python -m unrealbench.src.authoring.admission check-all \
        --tasks-dir tasks_unreal/ --mode code_only

    python -m unrealbench.src.authoring.admission approve \
        --task-dir tasks_unreal/ue_task_0001_day_night_cycle --track code_only

    python -m unrealbench.src.authoring.admission status \
        --tasks-dir tasks_unreal/

Approve semantics:
  --approve sets readiness.<track> = true for the given track.
  If all applicable tracks become ready, lifecycle status is promoted to 'stable'.
  'stable' → 'official' is a separate manual step (version freeze).
  Approve is blocked if:
    - schema has validation errors
    - deprecated tasks
    - requires_alt_solutions is true but alt_good_solutions/ is missing/empty

Exit codes:
  0  all checks passed (or approval succeeded)
  1  one or more checks failed
  2  argument / configuration error
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path
from typing import Optional

from .schema import load_task_schema, validate_schema, dump_task_schema
from .validators.registry import run_full_validation, overall_passed


# ── Admission check helpers ───────────────────────────────────────────────────

def _check_schema_validity(task_dir: Path) -> tuple[bool, list[str]]:
    """Load and validate the schema. Returns (ok, errors)."""
    yaml_path = task_dir / "task.yaml"
    if not yaml_path.exists():
        return False, [f"task.yaml not found in {task_dir}"]
    try:
        schema = load_task_schema(task_dir)
    except Exception as exc:
        return False, [f"Failed to load task.yaml: {exc}"]

    errors = validate_schema(schema.to_dict())
    return len(errors) == 0, errors


def _check_calibration_assets(task_dir: Path) -> list[str]:
    """Check presence of required calibration solution sets."""
    issues: list[str] = []

    yaml_path = task_dir / "task.yaml"
    if not yaml_path.exists():
        return issues

    try:
        schema = load_task_schema(task_dir)
    except Exception:
        return issues

    # Bad solutions required
    bad_dir = task_dir / "calibration" / "bad_solutions"
    if not bad_dir.exists() or not any(bad_dir.iterdir()):
        issues.append("calibration/bad_solutions/ missing or empty (require ≥ 1 variant)")

    # Alt-good solutions required if flag set
    if schema.requires_alt_solutions:
        alt_dir = task_dir / "calibration" / "alt_good_solutions"
        if not alt_dir.exists() or not any(alt_dir.iterdir()):
            issues.append(
                "requires_alt_solutions is true but calibration/alt_good_solutions/ "
                "missing or empty"
            )

    # solution/ must exist
    if not (task_dir / "solution").exists():
        issues.append("solution/ directory missing")

    return issues


def _run_static_validation(task_dir: Path) -> tuple[bool, list[str]]:
    """Run public (static) validators only. Returns (passed, messages)."""
    try:
        schema = load_task_schema(task_dir)
    except Exception as exc:
        return False, [f"Schema load failed: {exc}"]

    pub_reports, priv_reports = run_full_validation(
        schema, task_dir, adapter=None, mode="code_only"
    )

    messages: list[str] = []
    for r in pub_reports:
        status = "PASS" if r.passed else ("SKIP" if r.skipped else "FAIL")
        messages.append(f"  [{status}] {r.validator_type}: {r.summary or r.skip_reason}")
        if not r.passed and not r.skipped and r.raw_output:
            for line in r.raw_output.strip().splitlines()[-10:]:  # last 10 lines
                messages.append(f"         {line}")

    passed = overall_passed(pub_reports, priv_reports)
    return passed, messages


# ── check command ─────────────────────────────────────────────────────────────

def cmd_check(task_dir: Path, mode: str = "code_only", verbose: bool = False) -> int:
    """Run admission check for a single task. Returns exit code."""
    print(f"\n{'═'*60}")
    print(f"ADMISSION CHECK: {task_dir.name}  [mode={mode}]")
    print(f"{'═'*60}")

    all_ok = True

    # 1. Schema validity
    schema_ok, schema_errors = _check_schema_validity(task_dir)
    if schema_ok:
        print("  [PASS] Schema validation")
    else:
        print("  [FAIL] Schema validation")
        for e in schema_errors:
            print(f"         {e}")
        all_ok = False

    # 2. Calibration assets
    asset_issues = _check_calibration_assets(task_dir)
    if not asset_issues:
        print("  [PASS] Calibration assets")
    else:
        all_ok = False
        for issue in asset_issues:
            print(f"  [FAIL] {issue}")

    # 3. Static validators (public, code_only)
    val_ok, val_messages = _run_static_validation(task_dir)
    for msg in val_messages:
        print(msg)
    if not val_ok:
        all_ok = False

    # 4. Migration warnings
    try:
        schema = load_task_schema(task_dir)
        mw = schema.calibration.get("migration_warnings", [])
        if mw:
            print(f"  [WARN] {len(mw)} migration warnings (review before approving):")
            if verbose:
                for w in mw:
                    print(f"         • {w}")
    except Exception:
        pass

    # Overall
    print()
    if all_ok:
        print("  RESULT: READY FOR ADMISSION")
    else:
        print("  RESULT: NOT READY — fix issues above before approving")

    return 0 if all_ok else 1


def cmd_check_all(tasks_dir: Path, mode: str = "code_only", verbose: bool = False) -> int:
    """Run admission check for all ue_task_* directories."""
    task_dirs = sorted(tasks_dir.glob("ue_task_*"))
    if not task_dirs:
        print(f"No ue_task_* directories found in {tasks_dir}")
        return 2

    results: dict[str, bool] = {}
    for td in task_dirs:
        if not td.is_dir():
            continue
        rc = cmd_check(td, mode=mode, verbose=verbose)
        results[td.name] = rc == 0

    n_pass = sum(results.values())
    n_total = len(results)
    print(f"\n{'═'*60}")
    print(f"SUMMARY: {n_pass}/{n_total} tasks ready")
    not_ready = [k for k, v in results.items() if not v]
    if not_ready:
        print("  Not ready:")
        for name in not_ready:
            print(f"    • {name}")

    return 0 if n_pass == n_total else 1


# ── approve command ───────────────────────────────────────────────────────────

def cmd_approve(task_dir: Path, track: str) -> int:
    """Approve a task for a given track (sets readiness + promotes lifecycle status)."""
    yaml_path = task_dir / "task.yaml"
    if not yaml_path.exists():
        print(f"ERROR: task.yaml not found in {task_dir}")
        return 2

    try:
        schema = load_task_schema(task_dir)
    except Exception as exc:
        print(f"ERROR: Failed to load task.yaml: {exc}")
        return 2

    # Block approve on deprecated
    if schema.lifecycle_status == "deprecated":
        print(f"ERROR: Task is deprecated — cannot approve")
        return 1

    # Block approve if schema has errors
    errors = validate_schema(schema.to_dict())
    if errors:
        print(f"ERROR: Schema has {len(errors)} validation error(s) — fix before approving:")
        for e in errors:
            print(f"  • {e}")
        return 1

    # Block approve if requires_alt_solutions but alt_good_solutions missing
    if schema.requires_alt_solutions:
        alt_dir = task_dir / "calibration" / "alt_good_solutions"
        if not alt_dir.exists() or not any(alt_dir.iterdir()):
            print(
                "ERROR: requires_alt_solutions is true but calibration/alt_good_solutions/ "
                "is missing or empty"
            )
            return 1

    # Block approve if track not applicable
    if track not in schema.applicable_tracks:
        print(
            f"ERROR: Track '{track}' is not applicable for this task "
            f"(mode={schema.capability_mode}, applicable={schema.applicable_tracks})"
        )
        return 1

    # Set readiness
    calibration = dict(schema.calibration)
    readiness = dict(calibration.get("readiness", {}))
    readiness[track] = True
    calibration["readiness"] = readiness

    # Promote lifecycle status
    current_status = calibration.get("status", "draft")
    all_applicable_ready = all(readiness.get(t, False) for t in schema.applicable_tracks)

    if all_applicable_ready and current_status not in ("stable", "official", "deprecated"):
        calibration["status"] = "stable"
        print(f"  Lifecycle status promoted: {current_status} → stable")
    elif not all_applicable_ready and current_status == "draft":
        calibration["status"] = "calibrating"
        print(f"  Lifecycle status updated: draft → calibrating")

    schema.calibration = calibration
    dump_task_schema(schema, yaml_path)
    print(f"  Approved track '{track}' for {task_dir.name}")
    print(f"  Readiness: {readiness}")
    return 0


# ── status command ────────────────────────────────────────────────────────────

def cmd_status(task_dir: Optional[Path] = None, tasks_dir: Optional[Path] = None) -> int:
    """Print lifecycle status and readiness for one or all tasks."""
    if task_dir is not None:
        _print_status_one(task_dir)
        return 0

    if tasks_dir is not None:
        task_dirs = sorted(tasks_dir.glob("ue_task_*"))
        if not task_dirs:
            print(f"No ue_task_* directories found in {tasks_dir}")
            return 2
        print(f"{'Task':<45} {'Status':<14} {'code_only':<12} {'hybrid':<10}")
        print("─" * 85)
        for td in task_dirs:
            if not td.is_dir():
                continue
            _print_status_row(td)
        return 0

    print("ERROR: --task-dir or --tasks-dir required")
    return 2


def _print_status_one(task_dir: Path) -> None:
    yaml_path = task_dir / "task.yaml"
    if not yaml_path.exists():
        print(f"{task_dir.name}: no task.yaml")
        return
    try:
        schema = load_task_schema(task_dir)
    except Exception as exc:
        print(f"{task_dir.name}: ERROR loading task.yaml — {exc}")
        return

    readiness = schema.calibration.get("readiness", {})
    print(f"\nTask:          {schema.task_id}")
    print(f"Name:          {schema.task_name}")
    print(f"Mode:          {schema.capability_mode}")
    print(f"Family:        {schema.task_family}")
    print(f"Status:        {schema.lifecycle_status}")
    print(f"Readiness:")
    for track in schema.applicable_tracks:
        ready = readiness.get(track, False)
        print(f"  {track:<16} {'✓ ready' if ready else '✗ not ready'}")

    mw = schema.calibration.get("migration_warnings", [])
    if mw:
        print(f"Warnings:      {len(mw)} migration warnings")


def _print_status_row(task_dir: Path) -> None:
    yaml_path = task_dir / "task.yaml"
    if not yaml_path.exists():
        print(f"{task_dir.name:<45} {'(no task.yaml)'}")
        return
    try:
        schema = load_task_schema(task_dir)
    except Exception:
        print(f"{task_dir.name:<45} {'ERROR':<14}")
        return

    readiness = schema.calibration.get("readiness", {})
    co = "Y" if readiness.get("code_only", False) else "N"
    hyb = "Y" if readiness.get("hybrid", False) else (
        "n/a" if schema.capability_mode == "code_only" else "N"
    )
    print(f"{task_dir.name:<45} {schema.lifecycle_status:<14} {co:<12} {hyb:<10}")


# ── CLI ───────────────────────────────────────────────────────────────────────

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Admission gate for benchmark tasks",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    # check
    p_check = subparsers.add_parser("check", help="Check a single task")
    p_check.add_argument("--task-dir", required=True, help="Task directory")
    p_check.add_argument("--mode", default="code_only", choices=["code_only", "hybrid"])
    p_check.add_argument("--verbose", action="store_true")

    # check-all
    p_check_all = subparsers.add_parser("check-all", help="Check all tasks in a directory")
    p_check_all.add_argument("--tasks-dir", required=True, help="Directory with ue_task_* dirs")
    p_check_all.add_argument("--mode", default="code_only", choices=["code_only", "hybrid"])
    p_check_all.add_argument("--verbose", action="store_true")

    # approve
    p_approve = subparsers.add_parser("approve", help="Approve a task for a given track")
    p_approve.add_argument("--task-dir", required=True, help="Task directory")
    p_approve.add_argument(
        "--track", required=True, choices=["code_only", "hybrid"],
        help="Track to approve"
    )

    # status
    p_status = subparsers.add_parser("status", help="Print lifecycle status")
    status_group = p_status.add_mutually_exclusive_group(required=True)
    status_group.add_argument("--task-dir", help="Single task directory")
    status_group.add_argument("--tasks-dir", help="Directory with ue_task_* dirs")

    args = parser.parse_args()

    if args.command == "check":
        return cmd_check(Path(args.task_dir), mode=args.mode, verbose=args.verbose)

    if args.command == "check-all":
        return cmd_check_all(Path(args.tasks_dir), mode=args.mode, verbose=args.verbose)

    if args.command == "approve":
        return cmd_approve(Path(args.task_dir), track=args.track)

    if args.command == "status":
        return cmd_status(
            task_dir=Path(args.task_dir) if getattr(args, "task_dir", None) else None,
            tasks_dir=Path(args.tasks_dir) if getattr(args, "tasks_dir", None) else None,
        )

    return 2


if __name__ == "__main__":
    sys.exit(main())
