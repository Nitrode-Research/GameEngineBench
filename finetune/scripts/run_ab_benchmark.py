#!/usr/bin/env python3
"""Run baseline vs tuned benchmark with a fixed workflow and produce comparison report."""

from __future__ import annotations

import argparse
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List

import yaml


def _load_yaml(path: Path) -> Dict[str, Any]:
    data = yaml.safe_load(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise ValueError("Config file must contain a mapping")
    return data


def _safe_model_name(model: str) -> str:
    return model.replace("/", "_") if model else "default"


def _build_cmd(config: Dict[str, Any], model: str) -> List[str]:
    shared = config.get("shared_flags", {})
    benchmark_runner = config.get("benchmark_runner", "unrealbench/src/benchmark_runner.py")

    cmd: List[str] = [
        "uv",
        "run",
        "python",
        benchmark_runner,
        "--agent",
        str(config["agent"]),
        "--model",
        model,
    ]

    if shared.get("use_runtime_video"):
        cmd.append("--use-runtime-video")
    if shared.get("enable_mcp"):
        cmd.append("--enable-mcp")
    if shared.get("skip_display"):
        cmd.append("--skip-display")
    if shared.get("debug"):
        cmd.append("--debug")

    cmd.extend(["run", "--task-list", str(config["task_list"])])
    return cmd


def _run_once(config: Dict[str, Any], model: str) -> Dict[str, Any]:
    cmd = _build_cmd(config, model)
    print("Running:", " ".join(cmd))

    proc = subprocess.run(cmd, text=True)
    if proc.returncode != 0:
        raise RuntimeError(f"Benchmark run failed for model={model} with code {proc.returncode}")

    result_file = Path("results") / f"{config['agent']}_{_safe_model_name(model)}_final_results.json"
    if not result_file.exists():
        raise FileNotFoundError(
            f"Expected results file not found: {result_file}. Benchmark may have failed early."
        )

    return json.loads(result_file.read_text(encoding="utf-8"))


def _extract_metrics(results: Dict[str, Any]) -> Dict[str, Any]:
    return {
        "task_success_rate": results.get("task_success_rate", 0.0),
        "tasks_attempted": results.get("tasks_attempted", 0),
        "tasks_passed": results.get("tasks_passed", 0),
        "tasks_failed": results.get("tasks_failed", 0),
        "token_statistics": results.get("token_statistics", {}),
        "cost_statistics": results.get("cost_statistics", {}),
        "duration_statistics": results.get("duration_statistics", {}),
    }


def _write_summary(summary_path: Path, report: Dict[str, Any]) -> None:
    baseline = report["baseline"]
    tuned = report["tuned"]
    delta = report["delta"]

    content = [
        "# A/B Benchmark Summary",
        "",
        f"- Timestamp (UTC): {report['timestamp_utc']}",
        f"- Agent: {report['agent']}",
        f"- Task list: {report['task_list']}",
        f"- Baseline model: {report['baseline_model']}",
        f"- Tuned model: {report['tuned_model']}",
        "",
        "## Primary Metric",
        f"- Baseline success rate: {baseline['task_success_rate']}%",
        f"- Tuned success rate: {tuned['task_success_rate']}%",
        f"- Delta: {delta['task_success_rate']} pp",
        "",
        "## Counts",
        f"- Baseline passed/attempted: {baseline['tasks_passed']}/{baseline['tasks_attempted']}",
        f"- Tuned passed/attempted: {tuned['tasks_passed']}/{tuned['tasks_attempted']}",
        f"- Delta passed: {delta['tasks_passed']}",
    ]

    summary_path.parent.mkdir(parents=True, exist_ok=True)
    summary_path.write_text("\n".join(content) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Run controlled baseline vs tuned benchmark")
    parser.add_argument("--config", required=True, help="Path to experiment YAML")
    args = parser.parse_args()

    cfg_path = Path(args.config)
    cfg = _load_yaml(cfg_path)

    for key in ["agent", "task_list", "baseline_model", "tuned_model"]:
        if key not in cfg:
            raise ValueError(f"Missing required config key: {key}")

    baseline_results = _run_once(cfg, str(cfg["baseline_model"]))
    tuned_results = _run_once(cfg, str(cfg["tuned_model"]))

    baseline = _extract_metrics(baseline_results)
    tuned = _extract_metrics(tuned_results)

    delta = {
        "task_success_rate": round(tuned["task_success_rate"] - baseline["task_success_rate"], 3),
        "tasks_passed": tuned["tasks_passed"] - baseline["tasks_passed"],
        "tasks_failed": tuned["tasks_failed"] - baseline["tasks_failed"],
        "total_cost_usd": round(
            tuned.get("cost_statistics", {}).get("total_cost_usd", 0.0)
            - baseline.get("cost_statistics", {}).get("total_cost_usd", 0.0),
            6,
        ),
        "total_duration_seconds": round(
            tuned.get("duration_statistics", {}).get("total_duration_seconds", 0.0)
            - baseline.get("duration_statistics", {}).get("total_duration_seconds", 0.0),
            6,
        ),
    }

    report = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "agent": cfg["agent"],
        "task_list": cfg["task_list"],
        "baseline_model": cfg["baseline_model"],
        "tuned_model": cfg["tuned_model"],
        "shared_flags": cfg.get("shared_flags", {}),
        "baseline": baseline,
        "tuned": tuned,
        "delta": delta,
    }

    outputs = cfg.get("outputs", {})
    report_path = Path(outputs.get("report_path", "finetune/reports/latest_ab_report.json"))
    summary_path = Path(outputs.get("summary_path", "finetune/reports/latest_ab_summary.md"))

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")
    _write_summary(summary_path, report)

    print(f"A/B report written: {report_path}")
    print(f"Summary written: {summary_path}")


if __name__ == "__main__":
    main()
