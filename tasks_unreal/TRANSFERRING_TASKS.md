# Transferring Unreal Tasks into UnrealBench

How to import a new C++ reimplementation task from an external Unreal Engine project.

Advice tagged `[Game-specific]` comes from a previous HordeTemplate import and may not apply to your source project. Everything else is general.

---

## For AI Agents Reading This Guide

Ask clarifying questions before making assumptions. Transferring a task involves decisions that depend on project-specific context: which commits to use, which files may be edited, which behaviors are required versus partial, and how the game's spawn, replication, and lifecycle flow actually works. Many of those decisions are not visible in the code alone.

Stop and ask when:

- You are guessing which commits to use for `start/` and `solution/`. Ask the user to confirm hashes.
- A `[Game-specific]` callout looks relevant. Verify by reading the source project, or ask.
- A test fails and you are about to change either the test or the expected solution behavior. Confirm which side is wrong.
- You are considering deleting a test. Surface the behavior coverage being lost before deleting it.
- The behavioral contract in `spec.yaml` is ambiguous. Ask for clarification; do not infer intent from implementation details.

Do not silently modify `spec.yaml`, rename solution methods, commit changes, or restart long-running processes without explicit consent. Prefer one concrete question over shipping a plausible but wrong benchmark.

---

## TL;DR

```bash
# 1. Extract start/ and solution/ from the source repo.
# 2. Author spec.yaml, including hidden_requirements and test_requirements.
# 3. Ask Chat/Codex/Claude to write tests/<TaskName>Tests.cpp.
# 4. Audit start/ and solution/ with the benchmark runner:

python -m unrealbench.src.ue_benchmark_runner \
  --tasks-dir tasks_unreal \
  --retest-baseline ue_task_NNNN \
  --warm-cache

# 5. Iterate on the test file until solution passes and start fails meaningful tests.
# 6. Run calibration:

python -m unrealbench.src.ue_benchmark_runner \
  --tasks-dir tasks_unreal \
  --output results/ue_task_NNNN_calibration.json \
  --agent claude-code \
  --model <model> \
  --task-id ue_task_NNNN \
  --warm-cache
```

`generate_task_tests.py` is legacy tooling. Do not use it for new task imports. Use chat to author and repair the test file, and use `ue_benchmark_runner --retest-baseline` as the source of truth for baseline validation.

---

## 1. Task Model

The core question each task poses: given a real UE5 game with one system removed or one bug reintroduced, can a model restore the intended behavior?

| Directory/File | Contents | Model sees it? |
| --- | --- | --- |
| `start/` | Project state the solver receives. It compiles but is behaviorally incomplete or buggy. | Yes |
| `solution/` | Reference implementation. | No |
| `tests/` | Hidden UE automation tests encoding the behavioral contract. | No, injected after solving |
| `spec.yaml` | Public task metadata and instruction. | Instruction only |

For benchmark runs, `ue_benchmark_runner` creates a solver workspace outside the repo, copies only `start/` into that workspace, runs the solver, then injects `tests/*.cpp` afterward for evaluation. On Windows the workspace root is short (`C:/ub/ws`) to avoid UnrealBuildTool path length failures; on macOS/Linux it lives under the system temp directory. Warm compile bases live under `C:/ub/wb` or the temp directory's `wb`.

Snapshots are saved under `tasks_unreal/test_result/<task-id>_<agent>_<timestamp>/` by default. The default results root can be overridden with `GAMEDEVBENCH_RESULTS_DIR`.

---

## 2. Task Types

### Type A: Reimplementation

Use this when the target system has been stripped and the model must reconstruct it.

- `solution/` = last commit where the target system works
- `start/` = later commit where the implementation was emptied or removed
- `instruction:` describes observable behaviors to restore, not how the reference implementation works

Most of this guide assumes Type A.

### Type B: Bug Fix

Use this when a real bug existed and a commit or PR fixed it.

Do not use a raw broken commit as `start/`. Raw broken commits often contain unrelated issues or crash-state ambiguity.

Recommended workflow:

- `solution/` = fixed, stable commit
- `start/` = copy of `solution/` with exactly one bug manually reintroduced
- The `start/` to `solution/` diff should be small, ideally 1-5 lines changing one logical thing
- `instruction:` describes the observable symptom, not the fix

Good:

```text
Players are never disconnected when DisconnectFromServer is called.
```

Bad:

```text
Replace PS->GettingKicked() with UKismetSystemLibrary::QuitGame.
```

Tests should check the correct behavior, not grep for implementation patterns, unless the pattern itself is the bug and no runtime behavior can reliably distinguish it.

---

## 3. Pick Commits

For Type A:

- **Solution commit**: last commit where the target system works
- **Stripped commit**: later commit where the relevant implementation is removed

Both trees must compile. If the upstream project has no stripped commit, create one yourself.

For Type B:

- **Solution commit**: fixed, stable commit
- **Start state**: solution tree with one bug manually reintroduced

Do not pick commits from commit messages alone. Confirm hashes with the human task owner.

---

## 4. Extract Trees

Create the task directory:

```bash
TASK_DIR=/path/to/unrealbench/tasks_unreal/ue_task_NNNN_<name>
mkdir -p "$TASK_DIR/start" "$TASK_DIR/solution" "$TASK_DIR/tests"
```

From the source repo, archive the project files into `start/` and `solution/`:

```bash
cd /path/to/source_repo

git archive <stripped_commit> Source Config Content Plugins <Project>.uproject \
  | tar -x -C "$TASK_DIR/start"

git archive <solution_commit> Source Config Content Plugins <Project>.uproject \
  | tar -x -C "$TASK_DIR/solution"
```

Include:

- `Source/`
- `Config/`
- `Content/` when maps/assets are needed
- `Plugins/` when task-owned code or required project plugins live there
- `<Project>.uproject`

Exclude:

- `Binaries/`
- `Intermediate/`
- `Saved/`
- `Build/`
- `DerivedDataCache/`
- unrelated docs and local IDE files

If the source repo uses Git LFS for `Content/`, pull LFS objects first. Text-pointer `.uasset` files will usually compile but fail at runtime.

For bug-fix tasks, copy `solution/` to `start/`, then manually reintroduce the single bug in `start/` only.

---

## 5. Installed UE Target Fix

Installed Unreal builds reject `TargetBuildEnvironment.Unique` in many imported projects. In each copied `*.Target.cs`, use:

```csharp
BuildEnvironment = TargetBuildEnvironment.Shared;
bOverrideBuildEnvironment = true;
```

This is safe to apply consistently to both `start/` and `solution/`.

---

## 6. Write `spec.yaml`

Use `spec.yaml` for UE automation tasks. The current live task corpus uses `spec.yaml` plus `hidden_requirements` and `test_requirements`; do not migrate new UE automation tasks to `task.yaml`.

### Public Instruction

When available, copy the behavior-only prompt from the source repo:

```text
benchmarks/public/<system-name>.md
```

That file should be the canonical public description of what was removed and what behavior must be restored. Do not paraphrase from memory.

If the source repo does not have a public prompt yet, write one before importing the task. It should describe observable behavior, not reference-solution function names.

Good:

```text
Zombies detect and chase living players within range.
```

Bad:

```text
Implement AZedAIController::EnemyInSight using UAIPerceptionComponent.
```

### Template

```yaml
task_id: ue_task_NNNN
title: <Short Title>
tier: hard            # easy | medium | hard

instruction: |
  Paste the public behavior-only prompt here.

files_to_edit:
  - Source/<Project>/Public/X.h
  - Source/<Project>/Private/X.cpp
  # Plugin-owned task files are valid too:
  # - Plugins/<Plugin>/Source/<Module>/Private/Y.cpp

tests:
  runner: ue_automation
  mode: listen_server
  num_clients: 1
  filter: "<Group.Subgroup>"

hidden_requirements:
  R1:
    text: One precise behavior or semantic requirement.
    category: core_logic
  R2:
    text: Another behavior, edge case, authority rule, or lifecycle requirement.
    category: replication

test_requirements:
  <Group.Subgroup.TestName>:
    - R1
    - R2
```

`hidden_requirements` are the evaluator-authored contract used by manual review and LLM-as-judge. They should be specific enough to catch partial, shortcut, or implementation-shaped solutions.

`test_requirements` maps each automation test to the requirements it is intended to cover. Keep this map current as tests are renamed, deleted, or added.

Tier guidance:

- `easy`: one file, roughly 100 LOC or less
- `medium`: one coherent subsystem
- `hard`: cross-cutting behavior, replication, async/lifecycle issues, or several subsystems

---

## 7. Ask Chat to Author Tests

Ask Chat/Codex/Claude to write the test file directly in:

```text
tasks_unreal/<task>/tests/<TaskName>Tests.cpp
```

Give the chat:

- `spec.yaml`
- `files_to_edit` from both `start/` and `solution/`
- relevant public headers, C++ implementation files, and module Build.cs files
- relevant map/asset paths when tests need PIE
- private evaluator notes only as pasted context, not as a file path that should be persisted in logs

Good prompt shape:

```text
Read tasks_unreal/<task>/spec.yaml and the task-owned files in start/ and solution/.
Write tests/<TaskName>Tests.cpp.

Test observable behavior, not literal reference implementation details.
Use UE automation tests with:
EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter

Prefer a small direct behavioral suite first. Include REQUIRED or PARTIAL comments
above tests, and keep game-specific includes at the top.
```

Keep the first suite small and strong. Six direct behavioral tests are usually better than twelve brittle structural checks.

---

## 8. Audit with `--retest-baseline`

Run baseline validation through the benchmark runner:

```bash
python -m unrealbench.src.ue_benchmark_runner \
  --tasks-dir tasks_unreal \
  --retest-baseline ue_task_NNNN \
  --warm-cache
```

The runner compiles and tests both baselines:

- `solution/` should compile and pass all tests.
- `start/` should compile and fail meaningful hidden tests.
- `STUB_PASS` warnings must be reviewed. They are acceptable only for regression-style or static/partial coverage, not for core behavior.
- `SOL_FAIL` is a test bug or reference-solution issue. Fix it before calibration.

Iterate with chat:

1. Run `--retest-baseline`.
2. Paste the relevant failure output into chat.
3. Ask for a targeted fix to `tests/<TaskName>Tests.cpp`.
4. Review the edit.
5. Repeat until the baselines are credible.

Do not ask the chat agent to run a long audit loop unless the human has explicitly delegated that. The human should control audit cadence and decide when a test is too brittle, too indirect, or not worth keeping.

---

## 9. Test Authoring Conventions

Compile and UE harness issues are common. Keep these rules in the prompt and in review:

- Put all game-specific `#include` statements at the top of the file.
- Use `EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter`.
- Guard editor-only helpers with `#if WITH_EDITOR`.
- Prefer existing PIE bootstrap patterns in nearby task tests. Do not invent a new framework unless necessary.
- Use `UGameplayStatics::ApplyPointDamage` instead of calling protected damage paths directly.
- For replication behavior, spawn or mutate state on the server world and verify on the client world where feasible.
- Avoid AI path-following tests in old maps; headless navmesh can be flaky.
- Reflection/static checks should usually be `PARTIAL`, not direct behavioral coverage.
- Keep exactly one `.cpp` test file in `tests/` unless there is a concrete reason to split files.

`[Game-specific]` HordeTemplate gotchas, only if the source code confirms they apply:

- `TakeDamage` may skip kill paths if `DamageCauser` is not a real character.
- `HitResult.BoneName` may need to be non-empty, e.g. `FName("spine_01")` or `FName("head")`.
- Raw C++ character classes may be unsafe to spawn if `PostInitializeComponents` uses illegal `ConstructorHelpers` calls. Prefer an existing BP subclass fixture.
- Lobby countdowns may need to be bypassed through an explicit game-state start call.

---

## 10. Calibrate the Task

After `--retest-baseline` is credible, run a real solver:

```bash
python -m unrealbench.src.ue_benchmark_runner \
  --tasks-dir tasks_unreal \
  --output results/ue_task_NNNN_calibration.json \
  --agent claude-code \
  --model <model> \
  --task-id ue_task_NNNN \
  --warm-cache
```

Only `solver_success` tells you whether the solver produced an output. It is not the same as task correctness.

- `solver_success: false`: infra error, solver crash, timeout, or setup issue. Rerun or debug infrastructure.
- `solver_success: true`: model produced a workspace. Manually review the snapshot.

Do not rely only on `task_passed`. Tests can be too weak or too brittle while the task is still being authored.

Review the saved snapshot:

```text
tasks_unreal/test_result/<task>_<agent>_<timestamp>/
```

Compare the model's edited files against:

- the public `instruction`
- `hidden_requirements`
- private evaluator notes from the source repo, if they exist
- the reference solution as one valid implementation, not the only valid implementation

A good benchmark task often yields code that looks plausible but still misses important semantics: authority checks, replication, lifecycle ordering, async completion, asset routing, edge cases, or cleanup.

---

## 11. Retest Existing Snapshots

When you change tests after a model snapshot already exists, retest that snapshot without invoking the solver again:

```bash
python -m unrealbench.src.ue_benchmark_runner \
  --tasks-dir tasks_unreal \
  --retest tasks_unreal/test_result/<snapshot_dir> \
  --warm-cache
```

For several snapshots of the same task:

```bash
python -m unrealbench.src.ue_benchmark_runner \
  --tasks-dir tasks_unreal \
  --retest-batch \
    tasks_unreal/test_result/<snapshot_a> \
    tasks_unreal/test_result/<snapshot_b> \
  --warm-cache
```

Use `--skip-retest-baselines` only after you have already validated the current baseline state. Otherwise the retest can hide a broken test suite.

The runner's LLM-as-judge runs by default after benchmark snapshots and retests unless `--skip-judge` is passed. It chooses a cross-family judge when possible: Claude-family solver outputs are judged by the configured GPT judge, and non-Claude outputs are judged by the configured Claude judge. The judge is a secondary signal; manual review remains required.

---

## 12. Checklist

Authoring:

- [ ] Task owner confirmed the source commits or the single manually reintroduced bug.
- [ ] `start/`, `solution/`, and `tests/` directories exist.
- [ ] Both `start/` and `solution/` contain the required `.uproject`.
- [ ] `Source/`, required `Config/`, required `Content/`, and required `Plugins/` are present.
- [ ] No `Binaries/`, `Intermediate/`, `Saved/`, `Build/`, or `DerivedDataCache/` directories are committed.
- [ ] Git LFS assets are real assets, not text pointers.
- [ ] Installed-UE `*.Target.cs` files use `Shared` plus `bOverrideBuildEnvironment = true` where needed.
- [ ] `spec.yaml` passes schema audit.
- [ ] `instruction:` is copied or derived from a public behavior-only prompt.
- [ ] `files_to_edit` contains only files the solver may edit.
- [ ] `hidden_requirements` captures the evaluator's real contract.
- [ ] `test_requirements` maps each test to hidden requirement IDs.

Test validation:

- [ ] `tests/<TaskName>Tests.cpp` exists and is the only `.cpp` test file unless a split is intentional.
- [ ] `python -m unrealbench.src.ue_benchmark_runner --tasks-dir tasks_unreal --retest-baseline <task> --warm-cache` runs.
- [ ] `solution/` compiles and all tests pass.
- [ ] `start/` compiles and fails meaningful hidden tests.
- [ ] Every `STUB_PASS` is reviewed and justified.
- [ ] Every `SOL_FAIL` is fixed or explained before calibration.
- [ ] Test names in `test_requirements` match the actual automation test IDs.

Calibration:

- [ ] A solver run produced `solver_success: true`.
- [ ] The saved snapshot under `tasks_unreal/test_result/` was manually reviewed.
- [ ] The review was checked against private evaluator notes when available.
- [ ] Judge output in `result.json` was reviewed, especially false-positive or false-negative concerns.
- [ ] The task is not trivially solved by the baseline or by a shallow plausible implementation.

Security and repo hygiene:

- [ ] Private evaluator notes stay outside `unrealbench`.
- [ ] Private oracle text is not copied into `spec.yaml`, tests, transcripts, or committed files.
- [ ] No git commit is made unless the human explicitly asked for it.
- [ ] `git status` is reviewed before handing off.

---

## Manual Cleanup

The runner and audit helpers normally clean injected tests and temporary Build.cs changes. If a failed local run leaves injected tests behind, remove them and restore the touched Build.cs file deliberately:

```bash
rm -rf tasks_unreal/<task>/solution/Source/<Project>/Tests/
git checkout -- tasks_unreal/<task>/solution/Source/<Project>/<Project>.Build.cs
```

Do not run destructive cleanup commands against computed paths until you have checked the resolved path is inside the intended task directory.
