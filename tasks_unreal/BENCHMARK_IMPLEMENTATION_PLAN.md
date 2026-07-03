# Unreal Benchmark — Implementation Plan

## Goal

Measure whether AI models can implement real Unreal Engine 5 C++ game systems. Tasks are drawn from real game projects and calibrated so that models producing plausible-looking code still fail.

**Pass @1** is the headline metric: given a real project codebase with a system removed and a behavioral description of what that system must do, does the model produce a correct working implementation?

Correctness is determined by compiling the model's code and running automated behavioral tests in PIE (Play In Editor) headless mode with a listen server and one client. Tests fail the task if required behaviors are absent, regardless of whether the code compiles.

---

## Task Format

Each task lives in `tasks_unreal/ue_task_XXXX_<name>/` and contains:

```
ue_task_XXXX_<name>/
├── spec.yaml          ← task definition (see schema below)
├── start/             ← what the model sees: project with system removed
│   ├── Source/
│   ├── Config/
│   └── <ProjectName>.uproject
├── tests/             ← hidden from model: UE automation test files
│   └── <SystemName>Tests.cpp
└── solution/          ← reference implementation for audit
    └── Source/
```

- `start/` contains `Source/` and `Config/` only. Binary outputs, `Content/`, `Saved/`, `Intermediate/`, and `DerivedDataCache/` are excluded.
- `tests/` is never exposed to the model. Test files are added to the project before building and running evaluation.
- `solution/` is used by `audit.py` to verify that tests pass on the reference implementation.

---

## spec.yaml Schema

`spec.yaml` is the canonical task definition. It is the only required file beyond the directory structure.

```yaml
task_id: ue_task_XXXX          # unique identifier, matches directory prefix
title: Human Readable Title    # short display name
tier: hard                     # easy | medium | hard

instruction: |
  Behavioral description of what the system must do.
  Written in terms of observable behavior — what the system does,
  not how to implement it. No UE API names, no class names, no hints
  about the implementation approach. The model receives this text plus
  the full contents of start/.

files_to_edit:
  - Source/<Project>/Public/...     # headers the model may extend
  - Source/<Project>/Private/...    # implementation files to write

tests:
  runner: ue_automation             # always ue_automation
  mode: listen_server               # always listen_server for multiplayer behavioral tests
  num_clients: 1                    # number of clients in addition to the listen server
  filter: "ProjectName.SystemName"  # UE automation test filter string
```

### Field notes

**`instruction`**: describes behavioral requirements only. Does not name classes, functions, or UE subsystems. The model infers the implementation from the surrounding codebase in `start/`.

**`files_to_edit`**: the complete list of files the model is expected to create or modify. Files not on this list are considered read-only context.

**`tests.filter`**: passed to the UE automation test runner as a prefix match. All test cases whose name begins with this string are executed. The filter corresponds to the namespace used in `IMPLEMENT_*_AUTOMATION_TEST` macros in the test file.

---

## Test File Contract

Test files in `tests/` own the full scoring contract. Each test case is a UE automation test that encodes whether it is required or partial:

- **Required**: failure hard-fails the task regardless of other results
- **Partial**: failure reduces the score but does not hard-fail the task

Required vs partial is communicated via comments in the test file (not in `spec.yaml`). The test runner reports pass/fail per case and derives the overall task result from the required set.

Tests run in PIE listen server mode (one server process acting as both server and first client, plus one additional client). This allows testing of server-authoritative logic, replication, and client-side feedback within a single headless process.

Tests use UE's latent automation framework (`ADD_LATENT_AUTOMATION_COMMAND`, `FFunctionLatentCommand`) for async behavioral assertions, and UE reflection (`FindFunctionByName`, `HasAnyFunctionFlags`) for architecture-level checks.

### Integration

To compile and run tests for a task:

1. Copy `tests/<SystemName>Tests.cpp` into `Source/<Project>/Tests/` in the workspace
2. Add `"AutomationController"` to the module's `PublicDependencyModuleNames` in `Build.cs`
3. Compile the project
4. Run headless:
   ```bash
   UnrealEditor <project>.uproject \
     -ExecCmds="Automation RunTests <filter>" \
     -Unattended -NullRHI -NoSplash \
     -NumClients=1 -ListenServer
   ```

---

## Execution Flow

1. Runner copies `start/` into a fresh workspace
2. Runner copies `tests/` files into `Source/<Project>/Tests/` in the workspace
3. Runner invokes the model with the `instruction` from `spec.yaml` and the `files_to_edit` list
4. Model edits files in the workspace
5. Runner compiles the project — compile failure = task failed (`compile_failed`)
6. Runner launches UE headless in listen server PIE mode and runs automation tests matching the filter
7. Runner collects per-test results and derives overall pass/fail
8. Artifacts saved to `test_result/<task_name>_<agent>_<timestamp>/`

---

## Result Schema

```json
{
  "task_id": "ue_task_XXXX",
  "task_name": "example_task_name",
  "agent": "claude-code",
  "model": "claude-sonnet-4-6",
  "timestamp": "2026-04-21T00:00:00Z",

  "compile_success": true,
  "tests_run": 9,
  "tests_passed": 7,
  "tests_failed": 2,
  "required_tests_passed": true,
  "task_passed": true,

  "test_results": [
    { "id": "interface_compiles",       "passed": true,  "required": true  },
    { "id": "behavior_a",               "passed": true,  "required": true  },
    { "id": "behavior_b",               "passed": false, "required": true  },
    { "id": "optional_behavior_c",      "passed": false, "required": false }
  ],

  "failure_reason": null,
  "files_edited": ["Source/.../Example.cpp"],
  "error": null,
  "token_usage": { "input_tokens": 0, "output_tokens": 0, "total_tokens": 0 },
  "cost_usd": 0.0,
  "duration_seconds": 0
}
```

`task_passed` is `true` only when `compile_success` is `true` AND all required test cases pass.

### Failure reasons

| Value | Meaning |
|---|---|
| `compile_failed` | Project did not compile after model edits |
| `tests_failed` | Compiled but one or more required tests failed |
| `test_runner_error` | UE automation runner crashed or timed out |
| `solver_error` | Model invocation failed before producing output |

---

## Audit

`audit.py` verifies corpus health before running evaluation:

```bash
python tasks_unreal/audit.py --static    # start/ must FAIL validation, solution/ must PASS
python tasks_unreal/audit.py --compile   # compile all solution/ dirs
python tasks_unreal/audit.py --static --compile
```

The compile audit locates the `.uproject` file in each `solution/` directory automatically.

---

## Adding a New Task

1. Create `tasks_unreal/ue_task_XXXX_<name>/`
2. Extract `start/` from the source project at the stripped commit (`Source/` + `Config/` only)
3. Extract `solution/Source/` from the source project at the full-implementation commit
4. Write `spec.yaml` following the schema above
5. Write `tests/<SystemName>Tests.cpp` with required and partial test cases
6. Run `python audit.py --static` — verify start/ tests fail, solution/ passes
7. Run `python audit.py --compile` — verify solution/ compiles cleanly
