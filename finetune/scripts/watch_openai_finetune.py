#!/usr/bin/env python3
"""Poll an OpenAI fine-tuning job until completion."""

from __future__ import annotations

import argparse
import json
import os
import time
from datetime import datetime, timezone
from pathlib import Path

from dotenv import load_dotenv


def _require_openai_client():
    try:
        from openai import OpenAI  # type: ignore
    except ImportError as exc:
        raise RuntimeError("openai package is not installed. Install with: uv add openai") from exc
    return OpenAI


def _now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def main() -> None:
    parser = argparse.ArgumentParser(description="Watch OpenAI fine-tuning job")
    parser.add_argument("--job-id", required=True, help="Fine-tuning job id (ftjob_...)")
    parser.add_argument("--poll-seconds", type=int, default=20, help="Polling interval in seconds")
    parser.add_argument("--timeout-seconds", type=int, default=7200, help="Maximum wait time")
    parser.add_argument("--output", help="Optional JSON output file")
    args = parser.parse_args()

    load_dotenv()
    if not os.environ.get("OPENAI_API_KEY"):
        raise RuntimeError("OPENAI_API_KEY is not set")

    OpenAI = _require_openai_client()
    client = OpenAI()

    start = time.time()
    terminal = {"succeeded", "failed", "cancelled"}

    print(f"Watching job: {args.job_id}")
    while True:
        job = client.fine_tuning.jobs.retrieve(args.job_id)
        status = str(job.status)
        model_id = getattr(job, "fine_tuned_model", None)
        elapsed = int(time.time() - start)
        print(f"[{_now_iso()}] status={status} elapsed={elapsed}s fine_tuned_model={model_id}")

        if status in terminal:
            result = {
                "job_id": args.job_id,
                "status": status,
                "fine_tuned_model": model_id,
                "finished_at": _now_iso(),
                "elapsed_seconds": elapsed,
            }
            if args.output:
                out = Path(args.output)
                out.parent.mkdir(parents=True, exist_ok=True)
                out.write_text(json.dumps(result, indent=2), encoding="utf-8")
                print(f"Wrote job result: {out}")
            print(json.dumps(result, indent=2))
            if status != "succeeded":
                raise RuntimeError(f"Fine-tuning job ended with status: {status}")
            return

        if elapsed > args.timeout_seconds:
            raise TimeoutError(
                f"Timed out waiting for fine-tuning job after {args.timeout_seconds}s"
            )

        time.sleep(args.poll_seconds)


if __name__ == "__main__":
    main()
