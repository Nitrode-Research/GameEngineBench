# Fine-Tuning Pipeline

This folder contains a reproducible pipeline to evaluate whether training data improves UnrealBench performance while keeping workflow fixed.

## Goals

- Keep benchmark workflow constant across baseline and tuned runs.
- Parameterize only model ID as the experimental variable.
- Produce run manifests and comparison outputs suitable for reporting.

## Directory Layout

- `config/experiment.yaml`: Experiment settings (agent, baseline/tuned model IDs, task list, shared flags).
- `data/raw/`: Raw data from dataset builders.
- `data/canonical/`: Canonical trajectory JSONL.
- `data/processed/`: Exported provider-specific train/val files.
- `artifacts/`: Metadata and manifests.
- `reports/`: A/B comparison reports.
- `scripts/`: Pipeline scripts.

## Canonical Example Schema (JSONL)

Each line should contain:

```json
{
  "example_id": "traj_000001",
  "source": "tutorial",
  "task": {
    "title": "Fix jump buffering",
    "description": "...",
    "skills": ["movement", "physics"]
  },
  "context": {
    "repo_snapshot_id": "git:abc123",
    "relevant_files": ["scripts/player.gd"]
  },
  "trajectory": [
    {
      "step": 1,
      "intent": "plan",
      "action": "Inspect jump input path",
      "observation": "Input sampled late",
      "next_decision": "Move buffer read earlier"
    }
  ],
  "outcome": {
    "status": "success",
    "tests_passed": true,
    "failure_mode": ""
  },
  "leakage": {
    "overlap_with_eval": false,
    "overlap_reason": ""
  }
}
```

## Quick Start

1. Validate and split canonical dataset:

```bash
uv run python finetune/scripts/prepare_dataset.py \
  --input finetune/data/canonical/dataset.jsonl \
  --train-out finetune/data/processed/train.canonical.jsonl \
  --val-out finetune/data/processed/val.canonical.jsonl \
  --manifest-out finetune/artifacts/dataset_manifest.json
```

2. Export to OpenAI chat fine-tuning format:

```bash
uv run python finetune/scripts/export_openai_chat.py \
  --input finetune/data/processed/train.canonical.jsonl \
  --output finetune/data/processed/train.openai.jsonl

uv run python finetune/scripts/export_openai_chat.py \
  --input finetune/data/processed/val.canonical.jsonl \
  --output finetune/data/processed/val.openai.jsonl
```

3. Launch fine-tune job (optional helper):

```bash
uv run python finetune/scripts/launch_openai_finetune.py \
  --train finetune/data/processed/train.openai.jsonl \
  --val finetune/data/processed/val.openai.jsonl \
  --base-model gpt-4o-mini-2024-07-18
```

4. Run controlled A/B benchmark with identical flags:

```bash
uv run python finetune/scripts/run_ab_benchmark.py \
  --config finetune/config/experiment.yaml
```

5. Run the complete pipeline in one command:

```bash
uv run python finetune/scripts/run_complete_pipeline.py \
  --config finetune/config/full_pipeline.yaml
```

## Notes

- This repo does not run fine-tune jobs automatically unless `OPENAI_API_KEY` and network access are available.
- Keep `task-list`, runtime flags, and agent fixed between baseline and tuned models.

## Additional Utilities

- Watch fine-tune status:

```bash
uv run python finetune/scripts/watch_openai_finetune.py \
  --job-id ftjob_xxx
```

- Complete pipeline orchestrator:
  - `finetune/scripts/run_complete_pipeline.py`
  - Runs prepare/export, submits fine-tune, polls to completion, writes tuned model into a generated experiment config, then runs A/B benchmark.
