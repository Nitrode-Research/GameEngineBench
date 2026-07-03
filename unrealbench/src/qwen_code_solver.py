#!/usr/bin/env python3
"""
Qwen Code solver for UnrealBench tasks.
Uses Qwen Code in headless mode to edit the current workspace.
"""

import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time
from typing import Optional

from unrealbench.src.base_solver import BaseSolver
from unrealbench.src.utils.data_types import SolverResult, TokenUsage, pricing_for_model
from unrealbench.src.terminal_agent_solver import (
    parse_terminal_agent_steps,
    parse_terminal_cost,
    parse_terminal_token_usage,
)


QWEN_MODEL_ALIASES = {
    "qwen-3.7": "qwen3.7-plus",
    "qwen-3.7-plus": "qwen3.7-plus",
    "qwen-3.7-max": "qwen3.7-max",
    "qwen-3.6": "qwen3.6-plus",
    "qwen-3.6-plus": "qwen3.6-plus",
}

ZAI_CODING_BASE_URL = "https://api.z.ai/api/coding/paas/v4"
ALIBABA_COMPAT_BASE_URL = "https://dashscope-intl.aliyuncs.com/compatible-mode/v1"


class QwenCodeSolver(BaseSolver):
    """Solver that uses Qwen Code to complete benchmark tasks."""

    SUPPORTS_MCP = False
    SUPPORTS_SYSTEM_PROMPT = False

    def __init__(
        self,
        timeout_seconds: int = 3600,
        debug: bool = False,
        model: Optional[str] = None,
        use_mcp: bool = False,
        use_runtime_video: bool = False,
        reasoning_level: str = "default",
    ):
        super().__init__(
            timeout_seconds,
            debug,
            use_mcp,
            use_runtime_video,
            reasoning_level=reasoning_level,
        )
        self.model = model
        # Qwen Code is commonly installed as an npm shim on Windows. Resolve the
        # full path so subprocess can launch it reliably.
        self.qwen_cmd = shutil.which(os.environ.get("QWEN_CODE_CMD", "qwen")) or os.environ.get("QWEN_CODE_CMD", "qwen")

    def _uses_zai_glm(self) -> bool:
        model = (self.model or "").strip().lower()
        return bool(os.environ.get("ZAI_API_KEY")) and model.startswith(("glm", "zhipu"))

    def _zai_glm_args(self) -> list[str]:
        return [
            "--bare",
            "--approval-mode",
            "yolo",
            "--auth-type",
            "openai",
            "--openai-base-url",
            os.environ.get("ZAI_API_BASE_URL", ZAI_CODING_BASE_URL),
        ]

    def _uses_alibaba_openai_compat(self) -> bool:
        model = (self.model or "").strip().lower()
        return bool(os.environ.get("ALIBABA_API_KEY")) and model.startswith("deepseek")

    def _alibaba_openai_args(self) -> list[str]:
        return [
            "--bare",
            "--approval-mode",
            "yolo",
            "--auth-type",
            "openai",
            "--openai-base-url",
            os.environ.get("ALIBABA_API_BASE_URL", ALIBABA_COMPAT_BASE_URL),
        ]

    def _extra_args(self) -> list[str]:
        configured = os.environ.get("QWEN_CODE_ARGS")
        if self._uses_zai_glm() and (configured is None or configured.strip() in {"--yolo", "-y"}):
            return self._zai_glm_args()
        if self._uses_alibaba_openai_compat() and (configured is None or not configured.strip() or configured.strip() in {"--yolo", "-y"}):
            return self._alibaba_openai_args()
        if configured is None:
            return ["--yolo"]
        if not configured.strip():
            return []
        return shlex.split(configured, posix=(os.name != "nt"))

    @staticmethod
    def is_rate_limit_error(error_message: str) -> bool:
        error_lower = error_message.lower()
        rate_limit_keywords = [
            "rate limit",
            "rate_limit",
            "ratelimit",
            "quota exceeded",
            "quota_exceeded",
            "too many requests",
            "resource exhausted",
            "insufficient quota",
            "http 429",
            "status 429",
            "error 429",
            "throttlingexception",
            "request throttled",
        ]
        return any(keyword in error_lower for keyword in rate_limit_keywords)

    def _model_for_cli(self) -> Optional[str]:
        if not self.model:
            return None
        return QWEN_MODEL_ALIASES.get(self.model.strip().lower(), self.model)

    def _build_command(self, prompt: str) -> list[str]:
        cmd = [self.qwen_cmd]
        extra_args = self._extra_args()
        cmd.extend(extra_args)
        if not any(arg in {"--output-format", "-o", "--json-file", "--json-fd"} for arg in extra_args):
            cmd.extend(["--output-format", "json"])
        model_for_cli = self._model_for_cli()
        if model_for_cli:
            cmd.extend(["--model", model_for_cli])
        if os.environ.get("QWEN_CODE_PROMPT_ARG") == "1":
            cmd.extend(["-p", prompt])
        return cmd



    @staticmethod
    def _qwen_usage_records_path() -> Path:
        return Path.home() / ".qwen" / "usage_record.jsonl"

    @classmethod
    def _read_qwen_usage_records(cls) -> list[dict]:
        path = cls._qwen_usage_records_path()
        if not path.exists():
            return []
        records = []
        try:
            for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
                if not line.strip():
                    continue
                try:
                    record = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if isinstance(record, dict):
                    records.append(record)
        except OSError:
            return []
        return records

    @staticmethod
    def _usage_record_to_metrics(record: dict) -> tuple[TokenUsage | None, int | None, str | None, str | None]:
        models = record.get("models") if isinstance(record.get("models"), dict) else {}
        input_tokens = 0
        output_tokens = 0
        total_tokens = 0
        cached_tokens = 0
        actual_model = None
        for model_name, usage in models.items():
            if not isinstance(usage, dict):
                continue
            if actual_model is None:
                actual_model = str(model_name)
            input_tokens += int(usage.get("inputTokens") or 0)
            output_tokens += int(usage.get("outputTokens") or 0)
            total_tokens += int(usage.get("totalTokens") or 0)
            cached_tokens += int(usage.get("cachedTokens") or 0)
        token_usage = None
        if input_tokens or output_tokens or total_tokens:
            token_usage = TokenUsage(
                input_tokens=input_tokens,
                output_tokens=output_tokens,
                total_tokens=total_tokens or (input_tokens + output_tokens),
                cache_read_tokens=cached_tokens,
                cache_write_tokens=0,
            )
        tools = record.get("tools") if isinstance(record.get("tools"), dict) else {}
        tool_calls = tools.get("totalCalls")
        agent_steps = int(tool_calls) if isinstance(tool_calls, int) and tool_calls > 0 else None
        source = "qwen_usage_record" if record else None
        return token_usage, agent_steps, source, actual_model

    @staticmethod
    def _latest_matching_usage_record(records: list[dict], workspace: str, start_epoch_ms: int) -> dict | None:
        normalized_workspace = str(Path(workspace).resolve()).lower()
        candidates = []
        for record in records:
            project = str(record.get("project") or "")
            if not project:
                continue
            try:
                normalized_project = str(Path(project).resolve()).lower()
            except OSError:
                normalized_project = project.lower()
            if normalized_project != normalized_workspace:
                continue
            timestamp = int(record.get("timestamp") or 0)
            start_time = int(record.get("startTime") or 0)
            if timestamp and timestamp < start_epoch_ms:
                continue
            if start_time and start_time < start_epoch_ms - 5000:
                continue
            candidates.append(record)
        if not candidates:
            return None
        return max(candidates, key=lambda r: int(r.get("timestamp") or r.get("startTime") or 0))

    def solve_task(self) -> SolverResult:
        config = self.load_config()
        if not config:
            return SolverResult(
                success=False,
                message="Could not load task configuration",
                duration_seconds=0.0,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )

        start_time = time.time()
        qwen_start_epoch_ms = int(start_time * 1000)
        usage_records_before = self._read_qwen_usage_records()
        prompt = self.get_task_prompt(config)
        cmd = self._build_command(prompt)

        if self.debug:
            print("=" * 60)
            print("SENDING PROMPT TO QWEN CODE:")
            print("=" * 60)
            print(prompt)
            print("=" * 60)
            cmd_str = " ".join(c if " " not in c else f'"{c}"' for c in cmd[:-1])
            print(f"Running: {cmd_str} \"...\"")
            print("\nQWEN CODE TRAJECTORY:")
            print("=" * 60)

        try:
            env = os.environ.copy()
            if self._uses_zai_glm():
                env["OPENAI_API_KEY"] = env["ZAI_API_KEY"]
            elif self._uses_alibaba_openai_compat():
                env["OPENAI_API_KEY"] = env["ALIBABA_API_KEY"]
            result = subprocess.run(
                cmd,
                input=prompt if os.environ.get("QWEN_CODE_PROMPT_ARG") != "1" else None,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=self.timeout_seconds,
                cwd=os.getcwd(),
                env=env,
            )

            duration = time.time() - start_time
            stdout = result.stdout or ""
            stderr = result.stderr or ""
            combined = f"{stdout}\n{stderr}"
            is_rate_limited = self.is_rate_limit_error(combined)
            token_usage = parse_terminal_token_usage(combined)
            agent_steps, agent_steps_source = parse_terminal_agent_steps(combined)
            requested_model = self.model or "qwen"
            model_for_cli = self._model_for_cli() or requested_model
            actual_model = model_for_cli
            usage_record = self._latest_matching_usage_record(
                self._read_qwen_usage_records(),
                os.getcwd(),
                qwen_start_epoch_ms,
            )
            if usage_record:
                usage_from_record, steps_from_record, source_from_record, model_from_record = self._usage_record_to_metrics(usage_record)
                if usage_from_record:
                    token_usage = usage_from_record
                if steps_from_record is not None:
                    agent_steps = steps_from_record
                    agent_steps_source = source_from_record
                if model_from_record:
                    actual_model = model_from_record
            model_mismatch_note = None
            if actual_model and model_for_cli and actual_model != model_for_cli:
                model_mismatch_note = (
                    f"Qwen CLI usage record reported actual model {actual_model!r} "
                    f"after requesting {model_for_cli!r}; check Qwen model configuration."
                )
            parsed_cost = parse_terminal_cost(combined)
            cost_usd = parsed_cost if parsed_cost is not None else 0.0
            cost_usd_is_estimate = False
            cost_estimate_note = None
            if token_usage and parsed_cost is None:
                pricing_key, _ = pricing_for_model(actual_model)
                if pricing_key:
                    cost_usd = token_usage.calculate_cost(actual_model)
                    cost_usd_is_estimate = True
                    cost_estimate_note = (
                        f"estimated from Qwen usage record using {pricing_key} list pricing; "
                        "provider billing export is the source of truth"
                    )
                else:
                    cost_estimate_note = (
                        "Qwen usage record does not include billed cost and no explicit "
                        f"pricing is configured for {actual_model!r}; token usage and "
                        "tool-call steps were recorded"
                    )
            if model_mismatch_note:
                cost_estimate_note = (cost_estimate_note + "; " if cost_estimate_note else "") + model_mismatch_note

            if self.debug:
                if stdout:
                    print(stdout)
                if stderr:
                    print(f"\nStderr: {stderr[:1000]}")
                print(f"\n\nDuration: {duration:.2f} seconds")
                print(f"Exit code: {result.returncode}")
                print("=" * 60)

            if result.returncode != 0:
                message = f"Qwen Code failed with exit code {result.returncode}"
                if stderr.strip():
                    message += f"\nSTDERR: {stderr.strip()}"
                elif stdout.strip():
                    message += f"\nSTDOUT: {stdout.strip()[-2000:]}"
            else:
                message = stdout.strip() or "Task completed"

            return SolverResult(
                success=result.returncode == 0,
                message=message,
                duration_seconds=duration,
                stdout=stdout,
                stderr=stderr,
                is_rate_limited=is_rate_limited,
                token_usage=token_usage,
                model=actual_model,
                cost_usd=cost_usd,
                cost_usd_is_estimate=cost_usd_is_estimate,
                cost_estimate_note=cost_estimate_note,
                agent_steps=agent_steps,
                agent_steps_source=agent_steps_source,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )

        except subprocess.TimeoutExpired:
            duration = time.time() - start_time
            return SolverResult(
                success=False,
                message=f"Qwen Code timed out after {self.timeout_seconds}s",
                duration_seconds=duration,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )
        except FileNotFoundError:
            return SolverResult(
                success=False,
                message=(
                    "Qwen Code CLI not found. Install Qwen Code and ensure `qwen` "
                    "is on PATH, or set QWEN_CODE_CMD."
                ),
                duration_seconds=0.0,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )
        except Exception as exc:
            duration = time.time() - start_time
            error_msg = str(exc)
            return SolverResult(
                success=False,
                message=f"Error invoking Qwen Code: {error_msg}",
                duration_seconds=duration,
                is_rate_limited=self.is_rate_limit_error(error_msg),
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )


def main():
    solver = QwenCodeSolver(debug=True)
    result = solver.solve_task()
    print(result)


if __name__ == "__main__":
    main()
