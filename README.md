# GameEngineBench

GameEngineBench is a benchmark for evaluating coding agents on native C++ changes inside functioning Unreal Engine 5 projects. The current release contains 120 tasks drawn from nine real Unreal repositories. Each task gives the agent a buildable start state, scoped editable C++ files, and a behavior specification; after the solve phase, hidden tests are injected and run through Unreal automation, then judge auditing checks whether the result satisfies the requested behavior.

This repository is the Unreal Engine branch of the work described in the paper. It is not a Godot benchmark and does not require Godot. Compared with GameDevBench-style game-development tasks that emphasize tutorial-derived projects, visual scene construction, shaders, sprites, or full project generation, GameEngineBench focuses on runtime-integrated Unreal C++ edits in existing projects. The hard cases are usually engine-semantic failures: server/client authority, replication, object lifecycle, subsystem initialization, persistence, UI/session flow, ability-system integration, and interactions across existing gameplay systems.

## What Is Included

- 120 Unreal Engine C++ tasks under `tasks_unreal/ue_task_0001_*` through `tasks_unreal/ue_task_0120_*`
- A benchmark runner at `unrealbench/src/ue_benchmark_runner.py`
- Solver wrappers for Claude Code, Codex, Gemini CLI, Qwen Code, Kimi Code, OpenHands, and configurable terminal agents
- Task authoring and validation helpers under `unrealbench/src/authoring/`
- Paper assets, result summaries, and analysis utilities under `paper/` and `results/`
- A frozen Unreal workspace fixture under `tv_frozen_workspace/`

Old Kaggle/export workspaces, checked-in dependency folders, and archived unusable task snapshots are intentionally excluded from the clean launch branch.

## Benchmark Design

Each task package contains a start project, reference solution material, public task specification, editable-file scope, and hidden test assets. The runner copies the task into an isolated workspace, invokes a solver, compiles the resulting Unreal project, injects tests after the solver finishes, and records execution artifacts.

The benchmark measures behavioral correctness rather than reference similarity. A run can compile and still fail if it violates an Unreal runtime contract, such as doing authoritative gameplay work on a client, missing replicated state needed by UI, cleaning up actors at the wrong lifecycle point, or initializing a subsystem after dependent code expects it to exist.

The paper reports results over the 120-task set across ten evaluated model configurations. Pass@1 remains below 30%, while requirement-level partial progress is much higher, showing that many agents write plausible local C++ but fail to integrate it correctly with the surrounding runtime system.

## Repository Layout

- `unrealbench/src/ue_benchmark_runner.py` - main Unreal execution, solver orchestration, compilation, test injection, and artifact collection runner
- `unrealbench/src/` - solver wrappers, judge code, prompt utilities, and shared data types
- `unrealbench/src/authoring/` - task schema, task migration, validation, enrichment, admission, and calibration utilities
- `tasks_unreal/` - current Unreal benchmark task packages
- `results/` - benchmark progress notes and result-analysis utilities
- `paper/` - paper draft, figures, model notes, and bibliography
- `tv_frozen_workspace/` - frozen Unreal fixture used by parts of the benchmark tooling

## Installation

### Prerequisites

1. Python 3.10+ for the benchmark package; Python 3.12+ is required for OpenHands integrations.
2. Unreal Engine 5 installed locally, with `UE_ENGINE_ROOT` pointing to the engine root.
3. Optional solver CLIs for whichever agents you plan to run.
4. EOS SDK for the EOSIntegrationKit tasks. The SDK itself is gitignored because it is large and licensed. Download it from the Epic Dev Portal, then run `python setup_eos_sdk.py` or pass `--sdk <path>`.

### Python Environment

```bash
python -m venv .venv
. .venv/bin/activate
pip install -e .
```

On Windows PowerShell:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
pip install -e .
```

Install Node dependencies only if you use tooling that requires the checked package dependency:

```bash
npm install
```

`node_modules/` is generated locally and is not tracked.

## Configuration

Copy `.env.example` to `.env` and set only the credentials needed for the agents you run. Key variables include:

- `UE_ENGINE_ROOT` - path to the local Unreal Engine installation
- `OPENAI_API_KEY` - required for Codex runs
- `CLAUDE_CODE_OAUTH_TOKEN` or `claude /login` - preferred for Claude Code benchmark runs
- `ANTHROPIC_API_KEY` - used by some authoring/pipeline utilities, not the benchmark Claude subscription path
- `QWEN_CODE_CMD` and `QWEN_CODE_ARGS` - optional Qwen Code command and non-interactive flags
- `KIMI_CODE_CMD` and `KIMI_CODE_ARGS` - optional Kimi Code command and flags
- `DEEPSEEK_CODE_CMD`, `GLM_CODE_CMD`, `MUSE_CODE_CMD` - optional terminal-agent wrapper commands

Benchmark Claude paths prefer Claude subscription authentication and temporarily ignore `ANTHROPIC_API_KEY` while the SDK is running. This applies to both the `claude-code` solver and the Unreal judge path.

## Running A Task

```bash
python -m unrealbench.src.ue_benchmark_runner \
  --tasks-dir tasks_unreal \
  --output results/ue_results.json \
  --agent codex \
  --model gpt-5.5 \
  --task-id ue_task_0020
```

Common options:

- `--agent` - `claude-code`, `codex`, `gemini-cli`, `qwen-code`, `kimi-code`, `openhands`, or a configured terminal-agent wrapper
- `--model` - model name passed through to the selected solver
- `--task-id` - one or more task IDs to run
- `--solver-timeout` - solver wall-clock timeout in seconds; default is 3600
- `--skip-judge` - skip LLM-as-judge after the snapshot is saved
- `--resume-from` - skip tasks already solved in a previous results JSON
- `--reasoning-level` - requested effort level for supported wrappers: `default`, `low`, `medium`, `high`, `xhigh`, or `max`

Run the configured model matrix from `run_manifest.yaml`:

```bash
python -m unrealbench.src.ue_benchmark_runner --manifest run_manifest.yaml
```

## Results And Artifacts

Snapshots are written under `tasks_unreal/test_result/<task-id>_<agent>_<timestamp>/` unless `GAMEDEVBENCH_RESULTS_DIR` overrides the root. Each snapshot includes the solver workspace, compile/test output, judge verdict, token/cost metadata when available, and the final `result.json`.

Inspect a snapshot with:

```bash
unrealbench-ue-show --snapshot <snapshot-path>
```

Provider CLIs that do not expose token usage leave token and cost fields unset rather than estimating silently.

## Paper Alignment

The current paper frames GameEngineBench as an executable benchmark for runtime-integrated Unreal C++ tasks. The evaluated set spans gameplay mechanics, multiplayer behavior, AI/world orchestration, animation and movement, UI/session code, loading behavior, online-service integration, persistence, serialization, XR behavior, and rendering-oriented plugin code.

The repository keeps that focus: current tasks are scoped C++ edit tasks inside existing Unreal projects, not Godot tasks, visual-only tasks, or full game generation tasks. The public release should be interpreted as a gameplay-facing Unreal C++ systems benchmark with thinner coverage of audio, performance, platform support, security, and editor tooling.

## Citation

```bibtex
@misc{la2026gameenginebench,
      title={GameEngineBench: Evaluating Coding Models on Real Runtime Environments},
      author={Brian La and Sejoon Chang and Ben Kim},
      year={2026},
      note={Preprint},
}
```

This repository originated as a fork of GameDevBench. The Unreal task format, runner, solvers, judge pipeline, authoring tooling, and benchmark methodology are independent work built on top of that foundation.

## License
