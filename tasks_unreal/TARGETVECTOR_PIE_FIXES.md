# TargetVector / ALSXT PIE Task Fixes

How the ALSXT/ALS PIE tasks (ue_task_0048–0055) were made to run and discriminate
in the headless benchmark, and the recipe for the remaining ones.

Reference outcome: **ue_task_0048 (ALSXT Vaulting) → stable `start 0/5, solution 5/5`.**

## Background

These tasks build on the frozen TargetVector plugin set (ALSXT, ALS-Refactored,
CommonGame, TargetVectorCommonUI, EOS, GameplayCameras). The frozen **content** was
authored against different plugin versions than the source the benchmark compiles
(symptoms in the UE log: `empty engine version`, `Invalid GameplayTag …`,
`Reference will be nulled`, BP compile failures such as `CDE_AlsxtCharacterPlayer`,
`ProxyFactoryClass null` in `TVEOS_GameInstance`). This content↔code skew is the root
of nearly everything below.

## Major fixes (root-cause, broad impact — apply to every ALSXT PIE task)

1. **PIE config override.** The project boots `GameInstanceClass=/TargetVectorCommonUI/
   TVEOS_GameInstance` + `GlobalDefaultGameMode=/ALSXT/ALS/Core/B_ALSXT_GameMode`,
   both broken content. In headless PIE this **tears the world down ~2 ms after
   "up for play"** (`Bringing World … up for play` immediately followed by
   `BeginTearingDown`), so `GetTestWorld()` returns null and no test can run.
   Fix — in `Config/DefaultEngine.ini` (start **and** solution):
   ```ini
   GameInstanceClass=/Script/Engine.GameInstance
   GlobalDefaultGameMode=/Script/Engine.GameModeBase
   ```
   After editing, **delete the warm base** (`Remove-Item -Recurse C:\ub\wb\<task>_*`):
   config edits do not currently invalidate the warm-cache key.

2. **Suppress engine log noise.** The (bare-spawned) ALSXT character emits many benign
   `Error`-level logs each tick (`IsMeshPaintingConfigured is false`, ability-system
   interface, struct-init, deprecated non-instanced ability ensure). The automation
   framework records captured log errors as test failures. Set, at the top of each
   `RunTest`:
   ```cpp
   FAutomationTestBase::bSuppressLogErrors = true;
   FAutomationTestBase::bSuppressLogWarnings = true;
   ```
   Explicit `TestTrue`/`TestFalse` failures still fail the test (they call `AddError`
   directly, not via the log capture), so discrimination is preserved.

3. **Spawn the character directly.** Don't rely on the game-mode boot to produce a
   pawn. A `GetOrSpawnCharacter()` helper finds an existing `AAlsxtCharacter` or spawns
   the project Blueprint character; keep a C++ `AAlsxtCharacter::StaticClass()` +
   injected-settings fallback for safety.

4. **`UnrealEd` Build.cs dependency.** PIE latent commands (`FStartPIECommand`,
   `FEditorLoadMap`, `FEndPlayMapCommand`, `FWaitForShadersToFinishCompiling`) link
   from the `UnrealEd` module. The task's `Source/TargetVector/TargetVector.Build.cs`
   must add it under `if (Target.bBuildEditor)`. (0048/0053–0055 already had this
   block; 0057 was missing it.)

5. **Runner: keep the injected test out of the warm-base compile** (infra, in
   `unrealbench/src/ue_benchmark_runner.py`). The warm base is built from `solution/`
   only (test deps patched but the test `.cpp` not injected); the test is compiled in
   the fast per-eval overlay, and a failed overlay compile is reported directly instead
   of triggering a cold rebuild. Turns broken-test iterations from ~313 s into ~16 s.

## Minor fixes (localized, mechanical, per-task tuning)

- **Approach velocity** before a vault/trace that scales reach with speed: the ALSXT
  vault forward-trace reach is `BaseReachDistance * clamp(GetVelocity().Length() * mult,
  1, 500)`, so a stationary character can't reach the obstacle. Apply forward velocity
  in the same frame as the attempt; reset velocity to 0 on a non-start to avoid drift.
- **Re-ground the character right before each positive vault** (fresh, settled grounded
  state) — fixes marginal/flaky vault detection.
- **Validate parameter-adaptation with a single vault** (solved `VaultingHeight` must
  reflect the obstacle) — sequential double-vaults on one character leave residual
  state (action tag, montage/root-motion) that makes the 2nd vault unreliable.
- **`GetTestWorld()` accepts `EWorldType::PIE || EWorldType::Game`** (listen-server
  world typing robustness).
- **Compile/link nits:** use `FGameplayTag()` not the non-linkable
  `FGameplayTag::EmptyTag`; set protected settings via reflection (`FindFProperty`)
  rather than `#define protected public`; `TObjectPtr` upcasts need an explicit cast;
  include `Animation/AnimInstance.h` when stopping montages.
- **Operational:** run with `PYTHONUTF8=1` (or set it in the shell) so the runner's
  `✓/✗` results table doesn't crash on Windows cp1252.

## Dead ends (done but NOT load-bearing for 0048)

- **Extracting the full `TargetVectorCommonUI/Content` (~156 MB).** Done, and it lets
  the Blueprint character spawn cleanly, but the **config override** is what actually
  fixed PIE. Keep the content if other tasks need the game-mode boot; it was not the
  fix for 0048.
- **C++ spawn + injected settings (reflection).** Built as a fallback for when the BP
  wouldn't spawn; once PIE stayed up the BP spawned fine, so this path is unused
  belt-and-suspenders.

## Recipe for the remaining ALSXT PIE tasks (0049–0055)

1. Apply the config override (#1) and delete the warm base.
2. Add `bSuppressLogErrors` at the top of each test (#2).
3. Acquire the character via direct spawn (#3); ensure `GetTestWorld()` accepts Game
   worlds (#4 minor).
4. Adapt the task's own interaction; only vault-style tests need the velocity /
   re-ground / single-attempt patterns.
5. Validate: `python -m unrealbench.src.ue_benchmark_runner --tasks-dir tasks_unreal
   --retest-baseline ue_task_00NN --warm-cache` — expect `start ✗ / solution ✓`.

## Suggested follow-up runner fixes

- Make a `Config/` edit invalidate the warm-cache key (or overlay it into the base) so
  config changes don't require a manual warm-base delete.
- Reconfigure stdout to UTF-8 in `main()` so the results table prints on Windows
  without `PYTHONUTF8`.
- Optionally inject the `UnrealEd` editor dependency the same way the runner injects
  `AutomationController`, so task scaffolds can't miss it.
