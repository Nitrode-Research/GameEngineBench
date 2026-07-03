#!/usr/bin/env python3
"""Create a ready-to-build UE workspace for a task baseline WITHOUT compiling.

Reuses the benchmark runner's exact setup logic (shared-plugin allowlist copy,
hidden-test injection, DDC path patch) so the workspace is byte-identical to what
`--retest-baseline` would build — then stops and prints the precise compile and
test commands so you can iterate manually on the terminal.

Usage:
    ./.venv/bin/python tasks_unreal/make_workspace.py ue_task_0047 solution
    ./.venv/bin/python tasks_unreal/make_workspace.py ue_task_0047 start

The second arg is the variant: "solution" (default) or "start".
"""

import shlex
import sys
from pathlib import Path

from unrealbench.src.ue_benchmark_runner import UnrealBenchmarkRunner


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: make_workspace.py <task_ref> [solution|start]")
        return 2
    task_ref = sys.argv[1]
    variant = sys.argv[2] if len(sys.argv) > 2 else "solution"
    if variant not in ("solution", "start"):
        print(f"variant must be 'solution' or 'start', got {variant!r}")
        return 2

    runner = UnrealBenchmarkRunner(
        tasks_dir="tasks_unreal",
        output_file="results/_make_workspace_unused.json",
        agent="manual",
        debug=True,
    )

    ctx = runner._load_baseline_context(task_ref)
    task_dir = ctx["task_dir"]
    test_filter = ctx["test_filter"]
    num_clients = ctx["num_clients"]
    source_dir = task_dir / variant

    print(f"\nPreparing {variant} workspace for {ctx['task_id']} ...")
    workspace = runner._prepare_retest_workspace(source_dir, task_dir, variant)
    runner._inject_tests_for_evaluation(workspace, task_dir)

    uproject = runner._find_uproject(workspace)
    if uproject is None:
        print("ERROR: no .uproject in workspace")
        return 1
    uproject = uproject.resolve()

    # Reuse the runner's own command builders so the printed commands stay
    # byte-identical to what the runner would execute (no drift).
    compile_cmd = runner._build_compile_cmd(uproject)
    test_cmd = runner._build_test_cmd(uproject, test_filter, num_clients)

    bar = "=" * 78
    print(f"\n{bar}")
    print(f"Workspace ready ({variant}):")
    print(f"  {workspace}")
    print(f"\nPlugins on disk:")
    plugins = sorted(p.name for p in (workspace / "Plugins").iterdir() if p.is_dir())
    print(f"  {', '.join(plugins)}")
    print(f"\nTest filter: {test_filter}")
    print(bar)
    print("\n# 1) COMPILE:")
    print(" ".join(shlex.quote(c) for c in compile_cmd))
    print("\n# 2) RUN TESTS (after a successful compile):")
    print(" ".join(shlex.quote(c) for c in test_cmd))
    print(f"\n# UE automation log (after the test run):")
    print('ls -t "$HOME/Library/Logs/Unreal Engine"/*/*.log | head -1')
    print(f"\n# When done, delete the workspace:")
    print(f"rm -rf {shlex.quote(str(workspace))}")
    print(bar)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
