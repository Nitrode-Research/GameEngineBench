#!/usr/bin/env python3
"""
Kimi Code solver for UnrealBench tasks.
Uses Kimi Code in headless mode to edit the current workspace.
"""

import json
import os
import re
import shlex
import shutil
import subprocess
import time
from pathlib import Path
from typing import Optional

from unrealbench.src.base_solver import BaseSolver
from unrealbench.src.utils.data_types import SolverResult, TokenUsage
from unrealbench.src.terminal_agent_solver import (
    parse_terminal_agent_steps,
    parse_terminal_cost,
    parse_terminal_token_usage,
)


KIMI_MODEL_ALIASES = {
    "kimi-k2.7-code": "kimi-code/kimi-for-coding",
    "kimi-k2.7": "kimi-code/kimi-for-coding",
    "kimi-2.7": "kimi-code/kimi-for-coding",
    "kimi-for-coding": "kimi-code/kimi-for-coding",
}


def _int_value(value) -> int:
    if isinstance(value, int):
        return value
    if isinstance(value, float):
        return int(value)
    if isinstance(value, str):
        text = value.strip().replace(",", "")
        if text.isdigit():
            return int(text)
    return 0


def _kimi_cli_model(model: Optional[str]) -> Optional[str]:
    if not model:
        return None
    return KIMI_MODEL_ALIASES.get(model.lower(), model)


def _has_option(args: list[str], *names: str) -> bool:
    return any(
        any(arg == name or arg.startswith(f"{name}=") for name in names)
        for arg in args
    )


def _parse_session_id(output: str) -> Optional[str]:
    for line in (output or "").splitlines():
        try:
            event = json.loads(line)
        except json.JSONDecodeError:
            continue
        if isinstance(event, dict) and event.get("session_id"):
            return str(event["session_id"])
    match = re.search(r"session_[0-9a-f-]+", output or "", re.IGNORECASE)
    return match.group(0) if match else None


def _kimi_wire_path(session_id: Optional[str]) -> Optional[Path]:
    if not session_id:
        return None
    root = Path.home() / ".kimi-code" / "sessions"
    if not root.exists():
        return None
    matches = sorted(
        root.glob(f"*/{session_id}/agents/main/wire.jsonl"),
        key=lambda p: p.stat().st_mtime,
        reverse=True,
    )
    return matches[0] if matches else None


def _usage_from_kimi_mapping(usage: dict) -> TokenUsage:
    input_other = _int_value(usage.get("inputOther"))
    output = _int_value(usage.get("output"))
    cache_read = _int_value(usage.get("inputCacheRead"))
    cache_write = _int_value(usage.get("inputCacheCreation"))
    input_total = input_other + cache_read + cache_write
    return TokenUsage(
        input_tokens=input_total,
        output_tokens=output,
        total_tokens=input_total + output,
        cache_read_tokens=cache_read,
        cache_write_tokens=cache_write,
    )


def _parse_kimi_wire_usage(session_id: Optional[str]) -> tuple[Optional[TokenUsage], Optional[int], Optional[str], Optional[str]]:
    wire_path = _kimi_wire_path(session_id)
    if not wire_path:
        return None, None, None, None

    total = TokenUsage()
    saw_usage_record = False
    fallback_total = TokenUsage()
    saw_fallback = False
    model = None
    agent_steps = 0

    for line in wire_path.read_text(encoding="utf-8", errors="replace").splitlines():
        try:
            event = json.loads(line)
        except json.JSONDecodeError:
            continue
        if not isinstance(event, dict):
            continue

        event_type = event.get("type")
        if event_type == "usage.record" and isinstance(event.get("usage"), dict):
            usage = _usage_from_kimi_mapping(event["usage"])
            total.input_tokens += usage.input_tokens
            total.output_tokens += usage.output_tokens
            total.cache_read_tokens += usage.cache_read_tokens
            total.cache_write_tokens += usage.cache_write_tokens
            saw_usage_record = True
            model = event.get("model") or model
            continue

        nested = event.get("event")
        if isinstance(nested, dict):
            nested_type = str(nested.get("type", "")).lower()
            # Count agent actions, not tool results. Kimi records both
            # tool.call and tool.result events for each tool invocation.
            if nested_type == "tool.call":
                agent_steps += 1
            if isinstance(nested.get("usage"), dict):
                usage = _usage_from_kimi_mapping(nested["usage"])
                fallback_total.input_tokens += usage.input_tokens
                fallback_total.output_tokens += usage.output_tokens
                fallback_total.cache_read_tokens += usage.cache_read_tokens
                fallback_total.cache_write_tokens += usage.cache_write_tokens
                saw_fallback = True

    if saw_usage_record:
        total.total_tokens = total.input_tokens + total.output_tokens
        return total, (agent_steps or None), "kimi_session_usage_record", model
    if saw_fallback:
        fallback_total.total_tokens = fallback_total.input_tokens + fallback_total.output_tokens
        return fallback_total, (agent_steps or None), "kimi_session_loop_event_usage", model
    return None, (agent_steps or None), None, model


class KimiCodeSolver(BaseSolver):
    """Solver that uses Kimi Code to complete benchmark tasks."""

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
        self.kimi_cmd = self._resolve_command()

    def _resolve_command(self) -> str:
        configured = os.environ.get("KIMI_CODE_CMD", "").strip()
        candidates = [configured] if configured else ["kimi", "kimi-code"]
        for candidate in candidates:
            if not candidate:
                continue
            resolved = shutil.which(candidate)
            if resolved:
                return resolved
        return candidates[0] if candidates else "kimi"

    def _extra_args(self) -> list[str]:
        configured = os.environ.get("KIMI_CODE_ARGS")
        if configured is None:
            # Kimi Code rejects --yolo together with --prompt. In prompt mode,
            # rely on its non-interactive defaults unless the caller supplies
            # a compatible flag such as --auto through KIMI_CODE_ARGS.
            return []
        if not configured.strip():
            return []
        return shlex.split(configured, posix=(os.name != "nt"))

    def _subprocess_env(self) -> dict:
        env = os.environ.copy()
        if os.name == "nt" and not env.get("KIMI_SHELL_PATH"):
            for candidate in (
                r"C:\Program Files\Git\bin\bash.exe",
                r"C:\Program Files\Git\usr\bin\bash.exe",
            ):
                if os.path.exists(candidate):
                    env["KIMI_SHELL_PATH"] = candidate
                    break
        return env

    @staticmethod
    def is_rate_limit_error(error_message: str) -> bool:
        error_lower = (error_message or "").lower()
        rate_limit_keywords = [
            "rate limit",
            "rate_limit",
            "ratelimit",
            "quota exceeded",
            "quota_exceeded",
            "too many requests",
            "resource exhausted",
            "insufficient quota",
            "throttle",
            "throttled",
        ]
        return any(keyword in error_lower for keyword in rate_limit_keywords)

    def _build_command(self, prompt: str) -> list[str]:
        cmd = [self.kimi_cmd]
        extra_args = self._extra_args()
        cmd.extend(extra_args)
        if not _has_option(extra_args, "--output-format"):
            cmd.extend(["--output-format", "stream-json"])
        cli_model = _kimi_cli_model(self.model)
        if cli_model:
            cmd.extend(["--model", cli_model])
        cmd.extend(["-p", prompt])
        return cmd

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
        prompt = self.get_task_prompt(config)
        cmd = self._build_command(prompt)

        if self.debug:
            print("=" * 60)
            print("SENDING PROMPT TO KIMI CODE:")
            print("=" * 60)
            print(prompt)
            print("=" * 60)
            cmd_str = " ".join(c if " " not in c else f'"{c}"' for c in cmd[:-1])
            print(f"Running: {cmd_str} \"...\"")
            print("\nKIMI CODE TRAJECTORY:")
            print("=" * 60)

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=self.timeout_seconds,
                cwd=os.getcwd(),
                env=self._subprocess_env(),
            )

            duration = time.time() - start_time
            stdout = result.stdout or ""
            stderr = result.stderr or ""
            combined = f"{stdout}\n{stderr}"
            is_rate_limited = self.is_rate_limit_error(combined)
            session_id = _parse_session_id(combined)
            token_usage, wire_steps, wire_source, wire_model = _parse_kimi_wire_usage(session_id)
            if token_usage is None:
                token_usage = parse_terminal_token_usage(combined)
            parsed_cost = parse_terminal_cost(combined)
            model_used = wire_model or _kimi_cli_model(self.model) or "kimi-code/kimi-for-coding"
            cost_usd = parsed_cost if parsed_cost is not None else 0.0
            cost_usd_is_estimate = False
            cost_estimate_note = None
            if token_usage and parsed_cost is None:
                cost_usd = token_usage.calculate_cost(model_used)
                cost_usd_is_estimate = True
                cost_estimate_note = (
                    f"estimated from {wire_source or 'terminal output'} using Kimi list pricing; "
                    "provider billing export is the source of truth"
                )
            agent_steps, agent_steps_source = parse_terminal_agent_steps(combined)
            if wire_steps is not None:
                agent_steps = wire_steps
                agent_steps_source = "kimi_session_tool_events"

            if self.debug:
                if stdout:
                    print(stdout)
                if stderr:
                    print(f"\nStderr: {stderr[:1000]}")
                print(f"\n\nDuration: {duration:.2f} seconds")
                print(f"Exit code: {result.returncode}")
                print("=" * 60)

            if result.returncode != 0:
                message = f"Kimi Code failed with exit code {result.returncode}"
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
                model=model_used,
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
                message=f"Kimi Code timed out after {self.timeout_seconds}s",
                duration_seconds=duration,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )
        except FileNotFoundError:
            return SolverResult(
                success=False,
                message=(
                    "Kimi Code CLI not found. Install Kimi Code and ensure `kimi` "
                    "or `kimi-code` is on PATH, or set KIMI_CODE_CMD."
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
                message=f"Error invoking Kimi Code: {error_msg}",
                duration_seconds=duration,
                is_rate_limited=self.is_rate_limit_error(error_msg),
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )


def main():
    solver = KimiCodeSolver(debug=True)
    result = solver.solve_task()
    print(result)


if __name__ == "__main__":
    main()
