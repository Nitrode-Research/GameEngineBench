# Parallel S3 Runs

This workflow keeps the existing runner behavior intact:

- Workers run against local `tasks_unreal/` checkouts.
- The runner writes snapshots under `tasks_unreal/test_result/<task>_<agent>_<timestamp>/`.
- The runner writes a per-run summary to the worker-specific `--output` JSON.
- S3 is only the finished-artifact transport and archive.
- The coordinator syncs worker artifacts locally, validates them, then merges into `tasks_unreal/test_result`.

## S3 Layout

Use one run ID and one prefix per worker:

```text
s3://unrealbench-runs/runs/<run-id>/
  manifest/
    shard_plan.json
    run_manifest.yaml
    git_commit.txt
  workers/
    vm-01/
      results/
      test_result/
      logs/
    vm-02/
      results/
      test_result/
      logs/
  merged/
```

Workers should only write under their own `workers/<worker-id>/` prefix.

## Preflight Run Record

Before launching worker VMs, choose one shared run ID and one unique worker ID per VM. Record these values in your notes or shard plan:

```powershell
$S3Root = "s3://unrealbench-runs-583372808219"
$RunId = "overnight-20260624-full-4agents"
$WorkerId = "vm-01"
$OutputFile = "results/vm-01_claude-opus-4-8_high_tasks_1-18.json"
```

For each worker, document:

```text
RunId:
WorkerId:
S3Root:
repo commit:
mode: solve or retest
agent:
model:
reasoning level:
assigned task IDs or retest snapshot dirs:
output file, for solve runs:
exact runner command:
exact upload command:
UE version / UE_ENGINE_ROOT:
worker machine name:
```

All workers in the same campaign use the same `RunId`. Each worker must use a different `WorkerId` and, for solve runs, a different `--output` file.

## Shard Plan

A shard is one owned slice of the run matrix: `agent/model/reasoning/tasks`.

Example `shard_plan.json`:

```json
{
  "shards": [
    {
      "worker": "vm-01",
      "mode": "solve",
      "agent": "claude-code",
      "model": "claude-opus-4-8",
      "reasoning": "high",
      "tasks": ["1-18"]
    },
    {
      "worker": "vm-02",
      "mode": "solve",
      "agent": "codex",
      "model": "gpt-5.5",
      "reasoning": "high",
      "tasks": ["1-18"]
    }
  ]
}
```

Do not assign the same `task_id/agent/model/reasoning` to multiple workers unless duplicate trials are intentional.

## Worker Solve Run

Use a unique output file per worker:

```powershell
python -m unrealbench.src.ue_benchmark_runner `
  --tasks-dir tasks_unreal `
  --output results/vm-01_claude-opus-4-8_high_tasks_1-18.json `
  --agent claude-code `
  --model claude-opus-4-8 `
  --reasoning-level high `
  --warm-cache `
  --task-id ue_task_0001 ue_task_0002 ue_task_0003
```

Then upload only the snapshots referenced by that output file:

```powershell
.\results\util\upload_worker_artifacts.ps1 `
  -RunId 2026-06-23-overnight-001 `
  -WorkerId vm-01 `
  -S3Root s3://unrealbench-runs `
  -OutputFile results/vm-01_claude-opus-4-8_high_tasks_1-18.json `
  -LogFiles results/vm-01_runner.log
```

Use `-DryRun` first to inspect the AWS commands without transferring files.

## Worker Retest Run

Retest needs the assigned snapshot directories present locally before running.
After `--retest` or `--retest-batch`, upload explicit snapshot dirs:

```powershell
.\results\util\upload_worker_artifacts.ps1 `
  -RunId 2026-06-23-retest-001 `
  -WorkerId vm-03 `
  -S3Root s3://unrealbench-runs `
  -SnapshotDirs @(
    "tasks_unreal/test_result/ue_task_0051_claude-code_20260621_004400",
    "tasks_unreal/test_result/ue_task_0052_claude-code_20260621_010000"
  )
```

Each original snapshot should be retested by only one worker. Retest updates `result.json`, so duplicate retest ownership creates competing versions.

## Coordinator Sync And Merge

Sync is incremental; unchanged local files are skipped:

```powershell
aws s3 sync `
  s3://unrealbench-runs/runs/2026-06-23-overnight-001/workers/ `
  incoming/2026-06-23-overnight-001/workers/
```

Validate first:

```powershell
python results/util/merge_worker_results.py `
  --incoming incoming/2026-06-23-overnight-001/workers `
  --run-id 2026-06-23-overnight-001 `
  --shard-plan incoming/2026-06-23-overnight-001/manifest/shard_plan.json `
  --report results/merge_report_2026-06-23-overnight-001.json
```

Apply after validation passes:

```powershell
python results/util/merge_worker_results.py `
  --incoming incoming/2026-06-23-overnight-001/workers `
  --run-id 2026-06-23-overnight-001 `
  --shard-plan incoming/2026-06-23-overnight-001/manifest/shard_plan.json `
  --report results/merge_report_2026-06-23-overnight-001.json `
  --apply
```

For retest updates where destination snapshot directories already exist, add `--update-existing`:

```powershell
python results/util/merge_worker_results.py `
  --incoming incoming/2026-06-23-retest-001/workers `
  --run-id 2026-06-23-retest-001 `
  --shard-plan incoming/2026-06-23-retest-001/manifest/shard_plan.json `
  --apply `
  --update-existing
```

Then run analysis per config:

```powershell
python results/util/_analyze.py claude-opus-4-8 claude-code high
python results/util/_analyze.py gpt-5.5 codex high
python results/util/_analyze.py <gemini-model> gemini-cli medium
python results/util/_analyze.py <kimi-model> kimi-code
```
