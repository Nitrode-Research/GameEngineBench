#!/usr/bin/env python3
"""Diagnose warm-cache compile failures: build ONLY the warm base for a task
(no cold fallback, no baselines). On success it confirms the warm cache works;
on failure the base + full compile log are preserved and the log path printed.

Usage:
    ./.venv/bin/python tasks_unreal/build_warm_base.py ue_task_0047
"""

import sys
from pathlib import Path

from unrealbench.src.ue_benchmark_runner import UnrealBenchmarkRunner


def main() -> int:
    task_ref = sys.argv[1] if len(sys.argv) > 1 else "ue_task_0047"
    runner = UnrealBenchmarkRunner(
        tasks_dir="tasks_unreal",
        output_file="results/_warm_diag_unused.json",
        agent="manual",
        warm_cache=True,
        debug=True,
    )
    matches = sorted(p for p in Path("tasks_unreal").glob(f"{task_ref}*")
                     if (p / "spec.yaml").exists())
    if not matches:
        print(f"No task matching {task_ref}")
        return 2
    task_dir = matches[0]
    files_to_edit = list((runner.load_task_config(task_dir) or {}).get("files_to_edit", []))

    print(f"Building warm base for {task_dir.name} ...")
    base = runner._get_or_build_warm_base(task_dir, files_to_edit)

    bar = "=" * 70
    print(f"\n{bar}")
    if base is not None:
        print("WARM BASE COMPILED OK - warm cache works now.")
        print(f"   {base}")
    else:
        print("WARM BASE FAILED TO COMPILE. Preserved base(s) + full log:")
        for c in sorted(runner._warm_cache_dir().glob(f"{task_dir.name}_*")):
            log = c / "compile_output.txt"
            print(f"   log: {log}  ({'exists' if log.exists() else 'MISSING'})")
    print(bar)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
