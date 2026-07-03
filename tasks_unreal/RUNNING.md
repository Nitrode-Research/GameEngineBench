# Running Unreal Tasks on Apple Silicon

This guide documents the working setup for running `unrealbench` Unreal tasks on an Apple Silicon Mac.

## Root Cause

The main failure mode is an architecture mismatch:

- Unreal project modules are compiled as `arm64`
- if the benchmark runner is launched from an Intel/Rosetta Python environment, UnrealEditor starts as `x64`
- Unreal shows the generic error:

```text
The game module 'HordeTemplateV2Native' could not be loaded.
```

The fix is to run the benchmark from a native `arm64` Python environment.

## Requirements

- Apple Silicon Mac
- Unreal Engine 5.7 installed
- Homebrew Python 3.10+ at `/opt/homebrew/bin/python3`
- repo root is the current working directory

## Create a Native Python Environment

Do not use an Intel/Rosetta conda env for Unreal runs.

Create a native venv:

```bash
rm -rf .venv
/opt/homebrew/bin/python3 -m venv .venv
```

Install dependencies:

```bash
./.venv/bin/python -m pip install --upgrade pip setuptools wheel
./.venv/bin/python -m pip install -e .
./.venv/bin/python -m pip install claude-code-sdk
```

Verify the interpreter is native:

```bash
./.venv/bin/python --version
./.venv/bin/python -c "import platform, sys; print(platform.machine()); print(sys.executable)"
```

Expected output includes:

- Python `>= 3.10`
- `arm64`

## Run the Unreal Benchmark

Clear Python environment variables so the venv is not redirected into another Python runtime:

```bash
env -u PYTHONHOME -u PYTHONPATH -u __PYVENV_LAUNCHER__ ./.venv/bin/python -m unrealbench.src.ue_benchmark_runner --tasks-dir tasks_unreal --output results/ue_results.json --agent claude-code --task-id ue_task_0001 --debug
```

## Audit Tasks Before Running

Run corpus health checks before evaluation:

```bash
# Validate spec.yaml schema for all tasks
python tasks_unreal/audit.py --schema

# Verify start/ stubs fail and solution/ passes static validation
python tasks_unreal/audit.py --static

# Compile all solution/ directories
python tasks_unreal/audit.py --compile
```

## Run UE Automation Tests Headless

Tests run in PIE listen server mode with one additional client. To run manually for a task:

1. Copy `tests/<SystemName>Tests.cpp` into `Source/HordeTemplateV2Native/Tests/` in the workspace
2. Add `"AutomationController"` to `PublicDependencyModuleNames` in `HordeTemplateV2Native.Build.cs`
3. Compile the project
4. Run headless:

```bash
"/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor" \
  "/absolute/path/to/HordeTemplateV2Native.uproject" \
  -ExecCmds="Automation RunTests HordeTemplate.Zombie" \
  -Unattended -NullRHI -NoSplash \
  -NumClients=1 -ListenServer
```

Replace `HordeTemplate.Zombie` with the filter string from the task's `spec.yaml`.

## Verify UnrealEditor Architecture

After a run, inspect the latest Unreal log:

```bash
latestlog=$(ls -1t "$HOME/Library/Logs/Unreal Engine/HordeTemplateV2NativeEditor"/*.log | head -1); echo "$latestlog"; grep -En "Architecture:|could not be loaded" "$latestlog"
```

Expected:

- `Architecture: arm64`
- no `could not be loaded` line

If the log shows `Architecture: x64`, the run is still going through Rosetta somewhere.

## Manual Sanity Check

To verify that a task project opens correctly, launch Unreal directly in `arm64`:

```bash
/usr/bin/arch -arm64 "/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor" "/absolute/path/to/HordeTemplateV2Native.uproject" -NoHotReload
```

This is useful for separating:

- project issues
- runner issues
- Python environment issues

## Common Problems

### `Bad CPU type in executable`

This usually means the current `python` is an Intel binary and cannot be forced to `arm64`.

Use the Homebrew Python venv above instead of that environment.

### `ModuleNotFoundError: No module named 'claude_code_sdk'`

Install into the native venv with:

```bash
./.venv/bin/python -m pip install claude-code-sdk
```

If imports still fail, verify with:

```bash
env -u PYTHONHOME -u PYTHONPATH -u __PYVENV_LAUNCHER__ ./.venv/bin/python -c "import sys, platform, claude_code_sdk; print(platform.machine()); print(sys.version); print(sys.executable); print(claude_code_sdk.__file__)"
```

### `Project file not found`

Do not reuse stale `test_result/...` paths from older runs. Use an existing current `.uproject` path.

## Notes

- The Unreal runner was updated to use a stable repo-local workspace under `tasks_unreal/_runner_workspace/`
- The runner strips copied Unreal-generated artifacts before compile/open
- Unreal `.uproject` files were updated to enable `EnhancedInput`
