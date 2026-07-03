"""
Conservative migration from legacy task_config.json to canonical task.yaml.

Usage:
    python -m unrealbench.src.authoring.migrate --tasks-dir tasks_unreal/ [--dry-run]
    python -m unrealbench.src.authoring.migrate --task-dir tasks_unreal/ue_task_0001_day_night_cycle

What this does:
  - Reads task_config.json for each task that does NOT yet have task.yaml
  - Calls synthesize_from_legacy() to build a TaskSchema
  - Writes task.yaml with calibration.status='migrated' and exhaustive migration_warnings
  - Does NOT touch task_config.json, validate.py, start/, or solution/
  - Does NOT set any readiness flag — every migrated task starts as NOT benchmark-ready

What it does NOT do:
  - Auto-admit tasks
  - Populate behavioral validator configs (checkpoints, actor_checks, etc.)
  - Populate public_contract.required_classes/callables
  - Infer correct PIE behavior

After running this, authors must:
  1. Fill in public_contract.required_classes, required_callables, required_properties
  2. Remove behavior_only: true once the above is populated
  3. Fill in starting_state.map
  4. Author validation.private.validators[*] configs
  5. Run `admission check` to see what's still missing
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from .schema import synthesize_from_legacy, dump_task_schema, validate_schema


def migrate_task(task_dir: Path, dry_run: bool = False) -> tuple[bool, list[str]]:
    """Migrate one task. Returns (success, warnings)."""
    task_dir = Path(task_dir)
    yaml_path = task_dir / "task.yaml"

    if yaml_path.exists():
        return True, [f"SKIP {task_dir.name}: task.yaml already exists"]

    config_path = task_dir / "task_config.json"
    if not config_path.exists():
        return False, [f"ERROR {task_dir.name}: no task_config.json found"]

    try:
        schema = synthesize_from_legacy(task_dir)
    except Exception as exc:
        return False, [f"ERROR {task_dir.name}: synthesis failed — {exc}"]

    # Validate the synthesized schema (some errors expected for migrated tasks)
    errors = validate_schema(schema.to_dict())
    schema_warnings = [f"schema error (will need fixing): {e}" for e in errors]

    migration_warnings = schema.calibration.get("migration_warnings", [])
    all_warnings = migration_warnings + schema_warnings

    if dry_run:
        print(f"\n{'─'*60}")
        print(f"DRY RUN: {task_dir.name}")
        print(f"  Migration warnings ({len(all_warnings)}):")
        for w in all_warnings:
            print(f"    • {w}")
        import yaml
        print(f"\n  Generated task.yaml preview:")
        print(yaml.dump(schema.to_dict(), default_flow_style=False, allow_unicode=True, sort_keys=False))
        return True, all_warnings

    dump_task_schema(schema, yaml_path)
    print(f"  Wrote {yaml_path.relative_to(task_dir.parent)}")
    if all_warnings:
        print(f"  {len(all_warnings)} migration warnings — review before admission")

    return True, all_warnings


def migrate_all(tasks_dir: Path, dry_run: bool = False) -> dict[str, list[str]]:
    """Migrate all ue_task_* directories that don't have task.yaml yet."""
    tasks_dir = Path(tasks_dir)
    task_dirs = sorted(tasks_dir.glob("ue_task_*"))
    if not task_dirs:
        print(f"No ue_task_* directories found in {tasks_dir}")
        return {}

    results: dict[str, list[str]] = {}
    for td in task_dirs:
        if not td.is_dir():
            continue
        success, warnings = migrate_task(td, dry_run=dry_run)
        results[td.name] = warnings

    print(f"\nMigrated {sum(1 for v in results.values() if not any('ERROR' in w for w in v))}"
          f"/{len(results)} tasks")
    return results


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Migrate legacy task_config.json to canonical task.yaml"
    )
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--tasks-dir", help="Directory containing ue_task_* subdirs")
    group.add_argument("--task-dir", help="Single task directory to migrate")
    parser.add_argument(
        "--dry-run", action="store_true",
        help="Print generated YAML without writing files"
    )
    args = parser.parse_args()

    if args.task_dir:
        success, warnings = migrate_task(Path(args.task_dir), dry_run=args.dry_run)
        return 0 if success else 1

    migrate_all(Path(args.tasks_dir), dry_run=args.dry_run)
    return 0


if __name__ == "__main__":
    sys.exit(main())
