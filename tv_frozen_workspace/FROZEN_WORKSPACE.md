# Frozen TargetVector Workspace

This directory is a point-in-time snapshot of the TargetVector project used as the base
environment for benchmark tasks 47-56. Every TargetVector benchmark run starts by copying
this directory, then overlaying the task's `start/Plugins/<plugin>/` on top.

## Frozen at

| Item | Value |
|---|---|
| TargetVector commit | `930ad3c` — "Convert CommonUser to submodule" |
| ALSXT submodule | `9bf0af9d` |
| ALS-Refactored submodule | `d3ef367b` |
| CommonUser submodule | `ecc0d4f6` (benchmark/commonuser-session-flow) |
| Frozen date | 2026-06-01 |

## What was excluded and why

### Plugin Content/ directories (all plugins)

Plugin `Content/` directories (animations, meshes, materials, textures) are excluded from
all plugins in this workspace. This was confirmed safe by observing that every existing
benchmark task's `start/Plugins/<plugin>/` contains only `Source/`, `Resources/`, `Config/`,
and the `.uplugin` file — no `Content/` — and the tests pass correctly. UE either loads
character assets from the cooked content path or tolerates soft-reference misses at test
time. Including plugin Content/ would add ~7 GB (ALSXT alone is 3.1 GB of animation assets).

If a future test ever requires a specific plugin asset to be present, add only that asset
to the relevant plugin's directory in this workspace, not the full Content/.

### Excluded plugins

Three plugins were also excluded entirely. Each was verified to have no compile-time or
runtime dependency in any code path exercised by the benchmark tests.

### EOSIntegrationKit (~450 MB source)

**Why excluded:** EIK is a marketplace plugin for Epic Online Services (sessions, voice, presence).
Benchmark tasks test character movement, combat, camera, and session URL construction — none of
which call EIK APIs at runtime. Verification:
- No `#include` of EIK headers in ALSXT, ALS-Refactored, or TargetVectorCode source
- No EIK module dependency in any Build.cs touched by benchmark tasks
- No EIK API calls in any C++ source reachable from character BeginPlay

**Config impact:** `DefaultEngine.ini` originally set `DefaultPlatformService=EIK`. Changed to
`DefaultPlatformService=Null` so the engine doesn't attempt to load the absent EIK OSS module.
The `NetDriverEIK` net driver entry is left in place — it has an explicit fallback to
`IpNetDriver`, which handles local PIE listen-server transport correctly.

### DataConfig (~174 MB source)

**Why excluded:** DataConfig is a data serialization utility plugin. Verified to have zero
references in: ALSXT/ALS-Refactored/CommonUser/TargetVectorCode source, all Build.cs files,
all Config files, and all Content assets. It auto-enables via `EnabledByDefault=true` in its
`.uplugin`, so it is explicitly disabled in the frozen `TargetVector.uproject`.

### Ricochet (~30 MB source)

**Why excluded:** Ricochet is a GAS-based projectile/ballistics plugin. No ALSXT or game source
references it, and it is not listed in the uproject's Plugins array (no `EnabledByDefault` set),
so omitting the directory is sufficient — UE won't discover it.

## Config patches

| File | Change | Reason |
|---|---|---|
| `Config/DefaultEngine.ini` | `DefaultPlatformService=Null` (was `EIK`) | EIK OSS module not present |
| `TargetVector.uproject` | EOSIntegrationKit → `Enabled: false` | Prevents module load attempt |
| `TargetVector.uproject` | DataConfig → `Enabled: false` | Overrides EnabledByDefault=true |

## How the runner uses this

```
rsync -a tv_frozen_workspace/ working_dir/
rsync -a task/start/Plugins/<edited-plugin>/ working_dir/Plugins/<edited-plugin>/
# build and run tests in working_dir/
```

The task's `start/Plugins/` contains only the plugin being edited (stripped). All peer plugins
come from this frozen workspace.

## How to update this workspace

If you need to advance the frozen baseline (e.g., after a major ALSXT or ALS-Refactored update):

1. Confirm the new TargetVector commit has working benchmark tests on the live project
2. Re-run the exclusion analysis (check EIK/DataConfig/Ricochet still have no C++ deps):
   ```
   grep -r "EIK" /path/to/TargetVector/Plugins/ALSXT/Source --include="*.cpp" --include="*.h" --include="*.cs"
   grep -r "DataConfig" /path/to/TargetVector/Plugins/ALSXT/Source --include="*.cpp"
   ```
3. Delete this directory and recreate from the new TargetVector commit
4. Apply the same patches (DefaultEngine.ini, uproject disable list)
5. Update the "Frozen at" table above
6. Update the frozen workspace commit SHA in memory (`~/.claude/projects/.../memory/project_commonuser_submodule.md`)

## Debugging guide

If benchmark tests fail unexpectedly after updating the frozen workspace:

**Character doesn't spawn / PIE crashes on startup:**
- Check that `DefaultPlatformService=Null` is set in `Config/DefaultEngine.ini`
- Check that EOSIntegrationKit is `Enabled: false` in `TargetVector.uproject`
- Run with `-log` and look for module load failures

**Character spawns but ALS locomotion state is wrong:**
- The issue is likely in the overlaid plugin (task's start/), not the frozen workspace
- Temporarily use the solution/ plugin instead of start/ to isolate

**Tests pass on solution/ but not start/ in unexpected ways:**
- Check if a test depends on behavior in a peer plugin that changed between the live
  TargetVector and the frozen baseline
- Compare the relevant peer plugin files: `diff frozen/Plugins/X/... live-TV/Plugins/X/...`

**Missing asset warnings in log (non-fatal):**
- Expected. Plugin Content/ directories are excluded from this workspace (see above).
- UE logs "Failed to find object" for soft references to plugin assets. These are harmless
  unless a test explicitly verifies a material, mesh, or animation asset is loaded.
- If a test fails because a required asset is missing, add only that asset to the
  relevant plugin directory here (do not add the entire plugin Content/).

**Online subsystem warnings in log (non-fatal):**
- Expected. The Null OSS logs warnings about missing EOS features. These are harmless
  for local PIE tests.

## Known issues and historical context

### macOS `._*` resource fork files break plugin/project discovery

macOS creates hidden `._*` resource fork files when copying directories (e.g. via Finder or
certain rsync invocations without `--exclude="._*"`). Unreal's plugin and project scanner treats
any file ending in `.uplugin` or `.uproject` as a valid descriptor — including `._MyPlugin.uplugin`
— causing duplicate plugin discovery errors or silent load failures. If a plugin or the project
stops loading after a workspace copy, run:

```bash
find tv_frozen_workspace/ -name "._*" -delete
```

before opening or building.

### ALSXT required UE 5.7 compile compatibility fixes

ALSXT was not natively compatible with UE 5.7 at the time it was frozen. Fixes were applied to
make it compile cleanly. If you update the ALSXT submodule pin to a newer commit, re-verify it
compiles against UE 5.7 before re-freezing.

### CommonUser and CommonGame were normalized into submodules for benchmark infrastructure

CommonUser and CommonGame were originally vendored directories inside TargetVector. They were
converted to proper git submodules to support benchmark task isolation — each plugin needs an
independent working tree so the runner can overlay a stripped `start/` version cleanly. This was
a benchmark infrastructure change, not a gameplay change. The plugins themselves are unmodified
from their vendored baseline.

### Future submodules

More plugins are likely to be converted to submodules as additional benchmark tasks are created.
When a new plugin is added, add it to the frozen workspace (Source/ only, no Content/) and update
the table below.

The full submodule list in TargetVector as of the frozen baseline (`930ad3c`):

| Submodule path | Pinned commit | In frozen workspace? | Notes |
|---|---|---|---|
| `Plugins/ALS-Refactored` | `d3ef367b` | ✅ (tasks 54–55) | |
| `Plugins/ALSXT` | `9bf0af9d` | ✅ (tasks 47–53) | UE 5.7 compile fixes applied |
| `Plugins/Aurora` | `bb385a5e` | ✅ | |
| `Plugins/Axos` | `ac50fa4b` | ✅ | |
| `Plugins/Chronos` | `7ba1bced` | ✅ | |
| `Plugins/Clima` | `e1338080` | ✅ | |
| `Plugins/CommonGame` | `f0d84abd` | ✅ | **Added as submodule post-freeze** — was vendored |
| `Plugins/CommonUser` | `ecc0d4f6` | ✅ (task 56) | **Added as submodule post-freeze** — was vendored |
| `Plugins/DataConfig` | `17e29bce` | ❌ excluded | Unused, 174 MB source |
| `Plugins/EOSIntegrationKit` | `d3f2b4c0` | ❌ excluded | No C++ deps, 450 MB source |
| `Plugins/Flora` | `ae34762c` | ❌ | No Source/ directory |
| `Plugins/GameFeatures/AlsxtDashAbility` | `6da2cb2d` | ❌ | Nested submodule, not copied |
| `Plugins/Ricochet` | `04539a99` | ❌ excluded | No build deps, 30 MB source |
| `Plugins/TTToolbox` | `bff9f97b` | ✅ | |
| `Plugins/TargetVectorCode` | `e7a02c29` | ✅ | |
| `Plugins/TargetVectorCommonUI` | `349c0c1e` | ✅ | |
| `Plugins/TargetVectorContent` | `96fba458` | ✅ | |
| `Plugins/TargetVectorEOS` | `b1101df2` | ✅ | |
| `Plugins/Travos` | `68dd609f` | ✅ | |
| `Plugins/UEModularFeatures_ExtraActions` | `7e3f302a` | ✅ | |
