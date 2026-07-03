#!/usr/bin/env python3
"""Run full fine-tuning pipeline: prepare/export -> train -> poll -> A/B benchmark."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict

import yaml
from dotenv import load_dotenv


def _require_openai_client():
    try:
        from openai import OpenAI  # type: ignore
    except ImportError as exc:
        raise RuntimeError("openai package is not installed. Install with: uv add openai") from exc
    return OpenAI


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def _run(cmd: list[str]) -> None:
    print("Running:", " ".join(cmd))
    subprocess.run(cmd, check=True)


def _load_yaml(path: Path) -> Dict[str, Any]:
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError(f"Config must be a mapping: {path}")
    return data


def _write_yaml(path: Path, data: Dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(yaml.safe_dump(data, sort_keys=False), encoding="utf-8")


def _launch_finetune(train_path: Path, val_path: Path | None, base_model: str, suffix: str) -> Dict[str, Any]:
    load_dotenv()
    if not os.environ.get("OPENAI_API_KEY"):
        raise RuntimeError("OPENAI_API_KEY is not set")

    OpenAI = _require_openai_client()
    client = OpenAI()

    train_file = client.files.create(file=train_path.open("rb"), purpose="fine-tune")
    val_file_id = None
    if val_path:
        val_file = client.files.create(file=val_path.open("rb"), purpose="fine-tune")
        val_file_id = val_file.id

    req: Dict[str, Any] = {
        "training_file": train_file.id,
        "model": base_model,
        "suffix": suffix,
    }
    if val_file_id:
        req["validation_file"] = val_file_id

    job = client.fine_tuning.jobs.create(**req)
    payload = {
        "job_id": job.id,
        "status": str(job.status),
        "base_model": base_model,
        "training_file_id": train_file.id,
        "validation_file_id": val_file_id,
        "created_at": _now_iso(),
    }
    print("Fine-tuning job submitted:")
    print(json.dumps(payload, indent=2))
    return payload


def _poll_finetune(job_id: str, poll_seconds: int, timeout_seconds: int) -> Dict[str, Any]:
    OpenAI = _require_openai_client()
    client = OpenAI()

    start = time.time()
    terminal = {"succeeded", "failed", "cancelled"}

    while True:
        job = client.fine_tuning.jobs.retrieve(job_id)
        status = str(job.status)
        model_id = getattr(job, "fine_tuned_model", None)
        elapsed = int(time.time() - start)
        print(f"[{_now_iso()}] job={job_id} status={status} elapsed={elapsed}s model={model_id}")

        if status in terminal:
            result = {
                "job_id": job_id,
                "status": status,
                "fine_tuned_model": model_id,
                "elapsed_seconds": elapsed,
                "finished_at": _now_iso(),
            }
            if status != "succeeded":
                raise RuntimeError(f"Fine-tune job ended with status={status}")
            return result

        if elapsed > timeout_seconds:
            raise TimeoutError(f"Timed out waiting for job {job_id}")

        time.sleep(poll_seconds)


def main() -> None:
    parser = argparse.ArgumentParser(description="Run complete fine-tuning + benchmark pipeline")
    parser.add_argument(
        "--config",
        default="finetune/config/full_pipeline.yaml",
        help="YAML config path",
    )
    parser.add_argument(
        "--skip-train",
        action="store_true",
        help="Skip fine-tune submission/poll and only run benchmark using config tuned_model",
    )
    parser.add_argument(
        "--skip-benchmark",
        action="store_true",
        help="Run through fine-tune and stop before benchmark",
    )
    args = parser.parse_args()

    cfg = _load_yaml(Path(args.config))

    required = [
        "dataset_input",
        "base_model",
        "baseline_model",
        "agent",
        "task_list",
    ]
    for key in required:
        if key not in cfg:
            raise ValueError(f"Missing required key in config: {key}")

    python_cmd = cfg.get("python_cmd", ["uv", "run", "python"])
    if not isinstance(python_cmd, list) or not python_cmd:
        raise ValueError("python_cmd must be a non-empty list")

    processed_dir = Path(cfg.get("processed_dir", "finetune/data/processed"))
    manifest = Path(cfg.get("manifest_path", "finetune/artifacts/dataset_manifest.json"))
    val_ratio = float(cfg.get("val_ratio", 0.1))
    seed = int(cfg.get("seed", 42))

    # 1) Prepare + export
    _run(
        python_cmd
        + [
            "finetune/scripts/run_pipeline.py",
            "--input",
            str(cfg["dataset_input"]),
            "--workdir",
            str(processed_dir),
            "--manifest",
            str(manifest),
            "--val-ratio",
            str(val_ratio),
            "--seed",
            str(seed),
            "--python-cmd",
        ]
        + python_cmd
    )

    tuned_model = cfg.get("tuned_model")
    train_info: Dict[str, Any] | None = None
    final_job: Dict[str, Any] | None = None

    # 2) Fine-tune submit + poll
    if not args.skip_train:
        train_file = processed_dir / "train.openai.jsonl"
        val_file = processed_dir / "val.openai.jsonl"
        if not train_file.exists():
            raise FileNotFoundError(f"Missing train file: {train_file}")

        train_info = _launch_finetune(
            train_path=train_file,
            val_path=val_file if val_file.exists() else None,
            base_model=str(cfg["base_model"]),
            suffix=str(cfg.get("suffix", "unrealbench")),
        )
        final_job = _poll_finetune(
            job_id=train_info["job_id"],
            poll_seconds=int(cfg.get("poll_seconds", 30)),
            timeout_seconds=int(cfg.get("timeout_seconds", 7200)),
        )
        tuned_model = final_job.get("fine_tuned_model")

    if not tuned_model:
        raise ValueError("No tuned_model available. Provide tuned_model in config or run without --skip-train")

    artifact = {
        "timestamp_utc": _now_iso(),
        "config": str(args.config),
        "train_submission": train_info,
        "train_final": final_job,
        "tuned_model": tuned_model,
        "baseline_model": cfg["baseline_model"],
    }
    artifact_path = Path(cfg.get("run_artifact_path", "finetune/artifacts/complete_pipeline_run.json"))
    artifact_path.parent.mkdir(parents=True, exist_ok=True)
    artifact_path.write_text(json.dumps(artifact, indent=2), encoding="utf-8")
    print(f"Wrote run artifact: {artifact_path}")

    # 3) Generate experiment config and run A/B
    exp_cfg = {
        "agent": cfg["agent"],
        "task_list": cfg["task_list"],
        "benchmark_runner": cfg.get("benchmark_runner", "unrealbench/src/benchmark_runner.py"),
        "shared_flags": cfg.get("shared_flags", {}),
        "baseline_model": cfg["baseline_model"],
        "tuned_model": tuned_model,
        "outputs": cfg.get(
            "outputs",
            {
                "report_path": "finetune/reports/latest_ab_report.json",
                "summary_path": "finetune/reports/latest_ab_summary.md",
            },
        ),
    }
    exp_cfg_path = Path(cfg.get("generated_experiment_config", "finetune/config/experiment.generated.yaml"))
    _write_yaml(exp_cfg_path, exp_cfg)
    print(f"Wrote experiment config: {exp_cfg_path}")

    if not args.skip_benchmark:
        _run(python_cmd + ["finetune/scripts/run_ab_benchmark.py", "--config", str(exp_cfg_path)])


if __name__ == "__main__":
    main()
