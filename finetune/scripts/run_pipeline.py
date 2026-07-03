#!/usr/bin/env python3
"""Orchestrate dataset prep/export and optional fine-tune launch."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def _run(cmd: list[str]) -> None:
    print("Running:", " ".join(cmd))
    subprocess.run(cmd, check=True)


def _python_cmd(prefix: list[str], script: str, extra_args: list[str]) -> list[str]:
    return prefix + [script] + extra_args


def main() -> None:
    parser = argparse.ArgumentParser(description="Run fine-tuning data pipeline")
    parser.add_argument("--input", required=True, help="Canonical input JSONL")
    parser.add_argument(
        "--workdir",
        default="finetune/data/processed",
        help="Directory for processed outputs",
    )
    parser.add_argument("--manifest", default="finetune/artifacts/dataset_manifest.json")
    parser.add_argument("--val-ratio", type=float, default=0.1)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--base-model", help="If set with --launch, launch OpenAI fine-tune")
    parser.add_argument("--launch", action="store_true", help="Launch fine-tune job")
    parser.add_argument("--dry-run-train", action="store_true", help="Dry-run fine-tune launch")
    parser.add_argument(
        "--python-cmd",
        nargs="+",
        default=["uv", "run", "python"],
        help="Python command prefix for child scripts (default: uv run python)",
    )
    args = parser.parse_args()

    workdir = Path(args.workdir)
    train_canonical = workdir / "train.canonical.jsonl"
    val_canonical = workdir / "val.canonical.jsonl"
    train_openai = workdir / "train.openai.jsonl"
    val_openai = workdir / "val.openai.jsonl"

    _run(_python_cmd(args.python_cmd, "finetune/scripts/prepare_dataset.py", [
        "--input",
        args.input,
        "--train-out",
        str(train_canonical),
        "--val-out",
        str(val_canonical),
        "--manifest-out",
        args.manifest,
        "--val-ratio",
        str(args.val_ratio),
        "--seed",
        str(args.seed),
    ]))

    _run(_python_cmd(args.python_cmd, "finetune/scripts/export_openai_chat.py", [
        "--input",
        str(train_canonical),
        "--output",
        str(train_openai),
    ]))

    _run(_python_cmd(args.python_cmd, "finetune/scripts/export_openai_chat.py", [
        "--input",
        str(val_canonical),
        "--output",
        str(val_openai),
    ]))

    if args.launch:
        if not args.base_model:
            raise ValueError("--base-model is required when --launch is set")
        cmd = _python_cmd(args.python_cmd, "finetune/scripts/launch_openai_finetune.py", [
            "--train",
            str(train_openai),
            "--val",
            str(val_openai),
            "--base-model",
            args.base_model,
        ])
        if args.dry_run_train:
            cmd.append("--dry-run")
        _run(cmd)


if __name__ == "__main__":
    main()
