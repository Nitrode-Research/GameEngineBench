"""Impute missing benchmark token, cost, and step metrics.

This script is intentionally non-destructive: it does not overwrite raw
`token_usage`, `cost_usd`, or `agent_steps`. With `--apply`, it adds or replaces
an `imputed_metrics` object in each affected `result.json`.

Usage:
  python results/util/impute_result_metrics.py --tasks-dir tasks_unreal
  python results/util/impute_result_metrics.py --tasks-dir tasks_unreal --apply
"""
from __future__ import annotations

import argparse
import json
import math
import statistics
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


METRIC_FIELDS = ("input_tokens", "output_tokens", "total_tokens", "cost_usd", "agent_steps")


@dataclass
class Row:
    path: Path
    data: dict[str, Any]
    task_num: int
    agent: str
    model: str
    reasoning: str

    @property
    def config(self) -> tuple[str, str, str]:
        return (self.agent, self.model, self.reasoning)


def read_json(path: Path) -> dict[str, Any]:
    raw = path.read_bytes()
    for encoding in ("utf-8-sig", "cp1252"):
        try:
            return json.loads(raw.decode(encoding))
        except UnicodeDecodeError:
            continue
    return json.loads(raw.decode("utf-8", errors="replace"))


def reasoning_level(data: dict[str, Any]) -> str:
    requested = data.get("reasoning_level_requested")
    applied = data.get("reasoning_level_applied")
    if requested and requested != "default":
        return str(requested)
    if applied and applied != "default":
        return str(applied)
    return str(requested or applied or "default")


def task_num(data: dict[str, Any]) -> int | None:
    task_id = str(data.get("task_id", ""))
    if not task_id.startswith("ue_task_"):
        return None
    try:
        return int(task_id.rsplit("_", 1)[1])
    except ValueError:
        return None


def finite_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value)


def raw_metric(row: Row, metric: str) -> float | None:
    data = row.data
    token_usage = data.get("token_usage")
    if metric in {"input_tokens", "output_tokens", "total_tokens"}:
        if not isinstance(token_usage, dict):
            return None
        value = token_usage.get(metric)
        return float(value) if finite_number(value) else None
    if metric == "cost_usd":
        value = data.get("cost_usd")
        if not finite_number(value):
            return None
        if value == 0 and not raw_metric(row, "total_tokens"):
            return None
        return float(value)
    if metric == "agent_steps":
        value = data.get("agent_steps")
        return float(value) if finite_number(value) else None
    raise KeyError(metric)


def median(values: list[float]) -> float | None:
    values = [v for v in values if finite_number(v)]
    return statistics.median(values) if values else None


def rounded_metric(metric: str, value: float) -> int | float:
    if metric == "cost_usd":
        return round(float(value), 6)
    return int(round(value))


def confidence(method: str, overlap: int) -> str:
    if method.startswith("peer_task_scaled") and overlap >= 20:
        return "medium"
    if method.startswith("peer_task_scaled") and overlap >= 5:
        return "low"
    if method.startswith("config_median"):
        return "low"
    if method.startswith("cost_per_token"):
        return "medium"
    return "low"


def collect_rows(tasks_dir: Path) -> list[Row]:
    result_dir = tasks_dir / "test_result"
    rows: list[Row] = []
    for result_path in result_dir.glob("ue_task_*_*/result.json"):
        try:
            data = read_json(result_path)
        except Exception:
            continue
        num = task_num(data)
        if num is None:
            continue
        rows.append(
            Row(
                path=result_path,
                data=data,
                task_num=num,
                agent=str(data.get("agent") or ""),
                model=str(data.get("model") or ""),
                reasoning=reasoning_level(data),
            )
        )
    return rows


def build_indexes(rows: list[Row], metric: str) -> tuple[dict[tuple[str, str, str], dict[int, float]], dict[int, list[tuple[tuple[str, str, str], float]]]]:
    by_config: dict[tuple[str, str, str], dict[int, float]] = {}
    by_task: dict[int, list[tuple[tuple[str, str, str], float]]] = {}
    for row in rows:
        value = raw_metric(row, metric)
        if value is None:
            continue
        by_config.setdefault(row.config, {})[row.task_num] = value
        by_task.setdefault(row.task_num, []).append((row.config, value))
    return by_config, by_task


def estimate_from_peers(row: Row, metric: str, rows: list[Row]) -> dict[str, Any] | None:
    by_config, by_task = build_indexes(rows, metric)
    target_known = by_config.get(row.config, {})
    peer_values = by_task.get(row.task_num, [])
    candidates: list[float] = []
    overlaps: list[int] = []

    for peer_config, peer_task_value in peer_values:
        if peer_config == row.config:
            continue
        peer_known = by_config.get(peer_config, {})
        overlap_tasks = sorted(set(target_known) & set(peer_known))
        ratios = [
            target_known[t] / peer_known[t]
            for t in overlap_tasks
            if peer_known[t] > 0 and target_known[t] > 0
        ]
        ratio = median(ratios)
        if ratio is None:
            continue
        candidates.append(peer_task_value * ratio)
        overlaps.append(len(ratios))

    if candidates:
        max_overlap = max(overlaps)
        return {
            "value": rounded_metric(metric, median(candidates) or 0.0),
            "method": "peer_task_scaled_median",
            "peer_candidates": len(candidates),
            "max_overlap_tasks": max_overlap,
            "confidence": confidence("peer_task_scaled_median", max_overlap),
        }

    known_values = list(target_known.values())
    fallback = median(known_values)
    if fallback is None:
        return None
    return {
        "value": rounded_metric(metric, fallback),
        "method": "config_median",
        "known_config_rows": len(known_values),
        "confidence": confidence("config_median", 0),
    }


def estimate_cost(row: Row, rows: list[Row], imputed_total_tokens: int | None) -> dict[str, Any] | None:
    if imputed_total_tokens is not None:
        ratios = []
        for other in rows:
            if other.config != row.config:
                continue
            cost = raw_metric(other, "cost_usd")
            total = raw_metric(other, "total_tokens")
            if cost is not None and total and total > 0:
                ratios.append(cost / total)
        ratio = median(ratios)
        if ratio is not None:
            return {
                "value": rounded_metric("cost_usd", imputed_total_tokens * ratio),
                "method": "cost_per_token_config_median",
                "known_config_rows": len(ratios),
                "confidence": confidence("cost_per_token_config_median", len(ratios)),
            }
    return estimate_from_peers(row, "cost_usd", rows)


def build_imputation(row: Row, rows: list[Row]) -> dict[str, Any] | None:
    fields: dict[str, Any] = {}
    imputed_total_tokens: int | None = None

    token_fields: dict[str, Any] = {}
    for metric in ("input_tokens", "output_tokens", "total_tokens"):
        if raw_metric(row, metric) is not None:
            continue
        estimate = estimate_from_peers(row, metric, rows)
        if estimate:
            token_fields[metric] = estimate
            if metric == "total_tokens":
                imputed_total_tokens = int(estimate["value"])

    if token_fields:
        fields["token_usage"] = token_fields

    if raw_metric(row, "cost_usd") is None:
        if imputed_total_tokens is None and "total_tokens" in token_fields:
            imputed_total_tokens = int(token_fields["total_tokens"]["value"])
        estimate = estimate_cost(row, rows, imputed_total_tokens)
        if estimate:
            fields["cost_usd"] = estimate

    if raw_metric(row, "agent_steps") is None:
        estimate = estimate_from_peers(row, "agent_steps", rows)
        if estimate:
            fields["agent_steps"] = estimate

    if not fields:
        return None

    return {
        "version": 1,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "note": "Imputed values are estimates; raw token_usage, cost_usd, and agent_steps are unchanged.",
        "fields": fields,
    }


def config_label(config: tuple[str, str, str]) -> str:
    return " | ".join(config)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--tasks-dir", default="tasks_unreal")
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--report", default="results/imputed_metrics_report.json")
    args = parser.parse_args()

    rows = collect_rows(Path(args.tasks_dir))
    imputations: dict[Path, dict[str, Any]] = {}
    for row in rows:
        imputation = build_imputation(row, rows)
        if imputation:
            imputations[row.path] = imputation

    summary: dict[str, dict[str, int]] = {}
    for row in rows:
        label = config_label(row.config)
        item = summary.setdefault(label, {"entries": 0, "imputed_rows": 0})
        item["entries"] += 1
        if row.path in imputations:
            item["imputed_rows"] += 1

    report = {
        "applied": bool(args.apply),
        "rows_scanned": len(rows),
        "rows_with_imputations": len(imputations),
        "summary": summary,
        "rows": [
            {
                "path": str(path),
                "task_id": None,
                "imputed_metrics": imputation,
            }
            for path, imputation in sorted(imputations.items(), key=lambda item: str(item[0]))
        ],
    }

    # Fill task_id without keeping row objects in the report loop above.
    by_path = {row.path: row for row in rows}
    for item in report["rows"]:
        item["task_id"] = by_path[Path(item["path"])].data.get("task_id")

    report_path = Path(args.report)
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    if args.apply:
        for path, imputation in imputations.items():
            data = read_json(path)
            data["imputed_metrics"] = imputation
            path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")

    print(json.dumps({k: report[k] for k in ("applied", "rows_scanned", "rows_with_imputations")}, indent=2))
    print(f"Report: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
