# Final Unreal Task Reordering Plan

Executed remap record. The active task corpus has been reordered according to this document; removed tasks and removed active result snapshots were archived rather than deleted.

## Final Active Set

- Final active task IDs should be continuous: `ue_task_0001` through `ue_task_0110`.
- Move old `ue_task_0008` through `ue_task_0019` out of the active corpus into the archived task area.
- Preserve old `ue_task_0001` through `ue_task_0007`.
- Relocate old `ue_task_0110` through `ue_task_0127` into their original repository groups instead of leaving them as a tail block.
- Remove the sorting repository prefix from relocated task names, for example `bomber_pool_manager_runtime` becomes `pool_manager_runtime`.

## Range Map

| Final range | Source tasks |
| --- | --- |
| `0001-0007` | old `0001-0007` |
| removed | old `0008-0019` |
| `0008-0018` | old `0020-0030` |
| `0019-0022` | old `0031-0034` |
| `0023-0031` | relocated Bomber tasks from old `0110, 0111, 0112, 0113, 0116, 0119, 0125, 0126, 0127` |
| `0032-0043` | old `0035-0046` |
| `0044-0050` | old `0047-0053` |
| `0051-0053` | relocated ALSXT tasks from old `0114, 0120, 0121` |
| `0054-0069` | old `0054-0069` |
| `0070` | old `0070` |
| `0071` | relocated Ricochet task from old `0122` |
| `0072` | old `0071` |
| `0073-0075` | relocated DataConfig tasks from old `0115, 0117, 0118` |
| `0076-0081` | old `0072-0077` |
| `0082-0087` | old `0079-0084` |
| `0088-0089` | relocated GASShooter tasks from old `0123, 0124` |
| `0090-0108` | old `0085-0103` |
| `0109-0110` | old `0128-0129` |

Notes:

- There is no active `ue_task_0078` directory in the current filesystem.
- There are no active `ue_task_0104` through `ue_task_0109` directories in the current filesystem.
- `tasks.yaml` currently lists phantom IDs `0078`, `0104-0109`, and `0130-0132`; it should be regenerated from the active task directories after reordering.

## Relocated Task Details

| Old | New | New directory suffix |
| --- | --- | --- |
| `0110` | `0023` | `pool_manager_runtime` |
| `0111` | `0024` | `data_assets_loader_runtime` |
| `0112` | `0025` | `sounds_subsystem` |
| `0113` | `0026` | `pool_manager_editor_k2_nodes` |
| `0116` | `0027` | `data_assets_loader_editor_k2_nodes` |
| `0119` | `0028` | `instanced_static_mesh_converter` |
| `0125` | `0029` | `settings_widget_constructor_runtime` |
| `0126` | `0030` | `progression_system_runtime` |
| `0127` | `0031` | `function_picker_editor_customization` |
| `0114` | `0051` | `alsxt_recoil_curve_animation_modifier` |
| `0120` | `0052` | `alsxt_gas_ability_async_tasks` |
| `0121` | `0053` | `alsxt_movement_effect_notifies` |
| `0122` | `0071` | `ricochet_weapon_part_components` |
| `0115` | `0073` | `dataconfig_msgpack` |
| `0117` | `0074` | `dataconfig_anystruct_serde` |
| `0118` | `0075` | `dataconfig_property_path_json_converter` |
| `0123` | `0088` | `gameplay_ability_effect_containers` |
| `0124` | `0089` | `character_base_lifecycle` |

## Things Changed During Execution

1. Active task directories under `tasks_unreal/`.
   - Old `ue_task_0008` through `ue_task_0019` were moved into the archived task area instead of deleted.
   - Every kept task whose ID changed was renamed.
   - A temporary rename phase was used to avoid collisions where destination names already existed.

2. Root task metadata inside each active task.
   - Each active task's `spec.yaml` `task_id` was updated.
   - For relocated `0110-0127`, title and folder suffix were updated to remove only the sorting repo prefix (`Bomber`, `TargetVector`, `GASShooter`, etc.).
   - Project/module names were kept in instructions, file paths, and automation test filters.

3. JSON task metadata.
   - `task_config.json` was updated in tasks that have it, including the final LASAA tasks `0109-0110`.
   - Both `task_id` and `task_name` fields were updated where present.

4. Active task list files.
   - `tasks.yaml` was regenerated as `task_0001` through `task_0110`.
   - The previous stale `tasks.yaml` should not be used as authority for interpreting old runs.

5. Run manifests and active benchmark configs.
   - `run_manifest.yaml` was updated to use final task IDs. The outstanding old `51-65,69-71` range is now final `48-50,54-65,69-70,72`, with the old range retained only as explanatory provenance in comments.

6. Historical result artifacts.
   - `tasks_unreal/test_result/` previously contained old IDs in snapshot directory names and metadata files.
   - `tasks_unreal/test_result/` snapshot directories for kept tasks were migrated to their final task IDs, with embedded `task_config.json` and `result.json` files updated.
   - `tasks_unreal/test_result/` snapshots for removed old `0008-0019` were moved into the archived result area instead of deleted.
   - `tasks_unreal/test_result_archive/` was intentionally left untouched.
   - `results/*.json`, `results/*.log`, and `results/BENCHMARK_PROGRESS.md` contain old task numbers.
   - This `FINAL_TASK_REORDERING_PLAN.md` is the durable old-to-new mapping record; no separate remap file was created.

7. Documentation and reports.
   - Any current progress report that claims a corpus range such as `0001-0071`, `0085-0127`, or `0112-0127` should be considered old-numbered after the reorder.
   - Future reports should use the final `0001-0110` numbering and mention this remap.

## Execution Safety Checks

Before applying the reorder:

- Build an old-to-new mapping from the active directory list, not from stale `tasks.yaml`.
- Confirm the final destination IDs are exactly `0001-0110` with no gaps.
- Confirm no two old tasks map to the same new ID.
- Confirm all active tasks have a root `spec.yaml`.
- Confirm `spec.yaml task_id` matches the containing directory after the rename.
- Confirm `tasks.yaml` matches the final active directory set.
- Confirm removed old `0008-0019` task directories were moved to the archived task area.
- Confirm `tasks_unreal/test_result/` snapshots were either migrated to final IDs or moved to the archived result area if their source task was removed.
- Confirm `tasks_unreal/test_result_archive/` was left untouched.
