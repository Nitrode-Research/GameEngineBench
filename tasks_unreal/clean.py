#!/usr/bin/env python3
"""Remove UE build artifacts (Binaries, Intermediate, Saved, Build, DerivedDataCache)
from one or more `tasks_unreal/ue_task_*` directories. Build artifacts are stripped
from inside matching `tasks_unreal/test_result/<short_id>_*` snapshots too, but
the snapshots themselves are kept (their Source/, result.json, and logs are the
actual evaluation record).

Usage:
    python tasks_unreal/clean.py 1                  # single task by number
    python tasks_unreal/clean.py 0001               # zero-padded works too
    python tasks_unreal/clean.py 1-2                # inclusive range by number
    python tasks_unreal/clean.py ue_task_0001_zombie_system_reimplementation
    python tasks_unreal/clean.py all                # every ue_task_* dir
    python tasks_unreal/clean.py 1 20 --dry-run     # multiple tokens, dry-run

Cleans both `start/` and `solution/` subtrees inside each matched task, plus
any `test_result/<short_task_id>_*` calibration snapshots that match.
Tokens may be mixed: `clean.py 1 ue_task_0020_action_system_reimplementation 35-40`.
"""
from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path

DEFAULT_DIRS = ("Binaries", "Intermediate", "Saved", "Build", "DerivedDataCache")
HERE = Path(__file__).resolve().parent  # tasks_unreal/
TEST_RESULT_DIR = HERE / "test_result"   # calibration-run snapshots
TASK_RE = re.compile(r"^ue_task_(\d{4})(_.*)?$")


def list_all_tasks() -> list[Path]:
    return sorted(p for p in HERE.glob("ue_task_*") if p.is_dir())


def tasks_by_number() -> dict[int, Path]:
    out: dict[int, Path] = {}
    for p in list_all_tasks():
        m = TASK_RE.match(p.name)
        if m:
            out[int(m.group(1))] = p
    return out


def resolve_token(token: str, by_num: dict[int, Path]) -> list[Path]:
    """Map one CLI token to zero or more task directories."""
    if token.lower() == "all":
        return list(by_num.values())

    if token.startswith("ue_task_"):
        target = HERE / token
        if not target.is_dir():
            sys.exit(f"[ERROR] task directory not found: {token}")
        return [target]

    # Range like '28-30'
    if "-" in token:
        lo_str, hi_str = token.split("-", 1)
        try:
            lo, hi = int(lo_str), int(hi_str)
        except ValueError:
            sys.exit(f"[ERROR] couldn't parse range {token!r} (expected 'N-M')")
        if lo > hi:
            lo, hi = hi, lo
        matched = [by_num[i] for i in range(lo, hi + 1) if i in by_num]
        if not matched:
            sys.exit(f"[ERROR] no tasks in range {lo}-{hi}")
        return matched

    # Bare number
    try:
        n = int(token)
    except ValueError:
        sys.exit(f"[ERROR] couldn't parse token {token!r} (expected number, range, task id, or 'all')")
    if n not in by_num:
        sys.exit(f"[ERROR] no task with number {n} (have: {sorted(by_num)[:5]}...)")
    return [by_num[n]]


def dir_size_bytes(path: Path) -> int:
    total = 0
    for f in path.rglob("*"):
        if f.is_file() and not f.is_symlink():
            try:
                total += f.stat().st_size
            except OSError:
                pass
    return total


def clean_task(task_dir: Path, dirs: tuple[str, ...], dry_run: bool) -> int:
    """Remove matching subdirs from start/ and solution/, plus matching
    test_result/ snapshots. Returns bytes freed."""
    freed = 0
    found_any = False
    for side in ("start", "solution"):
        side_root = task_dir / side
        if not side_root.is_dir():
            continue
        for sub in dirs:
            target = side_root / sub
            if not target.exists():
                continue
            found_any = True
            size = dir_size_bytes(target)
            freed += size
            tag = "[DRY]" if dry_run else "[RM] "
            print(f"  {tag} {target.relative_to(HERE)}  ({size / 1e6:,.1f} MB)")
            if not dry_run:
                shutil.rmtree(target)

    # Calibration-run snapshots at tasks_unreal/test_result/<short_id>_<agent>_<ts>/
    # are project-root copies the model agent worked in. We strip build artifacts
    # from inside each matching snapshot but keep the snapshot itself (Source/,
    # Content/, result.json, logs are the actual evaluation record).
    m = TASK_RE.match(task_dir.name)
    if m and TEST_RESULT_DIR.is_dir():
        short_id = f"ue_task_{m.group(1)}"
        for snap in TEST_RESULT_DIR.glob(f"{short_id}_*"):
            if not snap.is_dir():
                continue
            for sub in dirs:
                target = snap / sub
                if not target.exists():
                    continue
                found_any = True
                size = dir_size_bytes(target)
                freed += size
                tag = "[DRY]" if dry_run else "[RM] "
                print(f"  {tag} {target.relative_to(HERE)}  ({size / 1e6:,.1f} MB)")
                if not dry_run:
                    shutil.rmtree(target)

    if not found_any:
        print("  (nothing to remove)")
    return freed


def main() -> None:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "targets", nargs="+",
        help="one or more of: task number (1), zero-padded (0001), range (1-2), full task id (ue_task_0001_...), or 'all'",
    )
    parser.add_argument(
        "--dry-run", action="store_true",
        help="show what would be removed without deleting anything",
    )
    parser.add_argument(
        "--dirs", nargs="+", metavar="NAME", default=list(DEFAULT_DIRS),
        help=f"override default folder list (default: {' '.join(DEFAULT_DIRS)})",
    )
    args = parser.parse_args()

    by_num = tasks_by_number()
    if not by_num:
        sys.exit(f"[ERROR] no ue_task_* directories under {HERE}")

    selected: list[Path] = []
    seen: set[Path] = set()
    for token in args.targets:
        for p in resolve_token(token, by_num):
            if p not in seen:
                seen.add(p)
                selected.append(p)

    print(f"Cleaning {len(selected)} task(s){' [DRY RUN]' if args.dry_run else ''}")
    print(f"Removing dirs: {', '.join(args.dirs)}")

    total_freed = 0
    for task in selected:
        print(f"\n{task.name}")
        total_freed += clean_task(task, tuple(args.dirs), args.dry_run)

    verb = "Would free" if args.dry_run else "Freed"
    print(f"\n{verb}: {total_freed / 1e9:,.2f} GB across {len(selected)} task(s)")


if __name__ == "__main__":
    main()
