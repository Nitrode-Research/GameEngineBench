#!/usr/bin/env python3
"""Launch an OpenAI fine-tuning job from exported JSONL files."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path

from dotenv import load_dotenv


def _require_openai_client():
    try:
        from openai import OpenAI  # type: ignore
    except ImportError as exc:
        raise RuntimeError(
            "openai package is not installed. Install with: uv add openai"
        ) from exc
    return OpenAI


def main() -> None:
    parser = argparse.ArgumentParser(description="Launch OpenAI fine-tuning job")
    parser.add_argument(
        "--train", required=True, help="Train JSONL path (OpenAI chat format)"
    )
    parser.add_argument("--val", help="Validation JSONL path (OpenAI chat format)")
    parser.add_argument(
        "--base-model", required=True, help="Base model (e.g., gpt-4o-mini-2024-07-18)"
    )
    parser.add_argument("--suffix", default="unrealbench", help="Fine-tune suffix")
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print actions only; do not call OpenAI API",
    )
    args = parser.parse_args()

    # Load environment variables from repo .env (and current working dir .env if present).
    load_dotenv()

    train_path = Path(args.train)
    if not train_path.exists():
        raise FileNotFoundError(f"Train file not found: {train_path}")

    val_path = Path(args.val) if args.val else None
    if val_path and not val_path.exists():
        raise FileNotFoundError(f"Validation file not found: {val_path}")

    if args.dry_run:
        payload = {
            "training_file": str(train_path),
            "validation_file": str(val_path) if val_path else None,
            "model": args.base_model,
            "suffix": args.suffix,
        }
        print("[dry-run] Would launch fine-tune with:")
        print(json.dumps(payload, indent=2))
        return

    if not os.environ.get("OPENAI_API_KEY"):
        raise RuntimeError("OPENAI_API_KEY is not set")

    OpenAI = _require_openai_client()
    client = OpenAI()

    train_file = client.files.create(file=train_path.open("rb"), purpose="fine-tune")
    val_file_id = None

    if val_path:
        val_file = client.files.create(file=val_path.open("rb"), purpose="fine-tune")
        val_file_id = val_file.id

    request = {
        "training_file": train_file.id,
        "model": args.base_model,
        "suffix": args.suffix,
    }
    if val_file_id:
        request["validation_file"] = val_file_id

    job = client.fine_tuning.jobs.create(**request)

    print("Fine-tuning job created")
    print(
        json.dumps(
            {
                "job_id": job.id,
                "status": job.status,
                "base_model": args.base_model,
                "training_file_id": train_file.id,
                "validation_file_id": val_file_id,
            },
            indent=2,
        )
    )


if __name__ == "__main__":
    main()
