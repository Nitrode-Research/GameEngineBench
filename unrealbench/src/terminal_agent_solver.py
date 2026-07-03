#!/usr/bin/env python3
"""
Shared helpers and configurable terminal-agent solvers.

These backends run provider CLIs in prompt mode and capture whatever accounting
the CLI emits. Token/cost metrics are intentionally left unset when the CLI does
not expose machine-readable usage.
"""

import json
import os
import re
import shlex
import shutil
import subprocess
import time
from typing import Optional

from unrealbench.src.base_solver import BaseSolver
from unrealbench.src.utils.data_types import SolverResult, TokenUsage


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


def _usage_from_mapping(data: dict) -> Optional[TokenUsage]:
    usage = data.get("usage") if isinstance(data.get("usage"), dict) else data
    input_tokens = (
        _int_value(usage.get("input_tokens"))
        or _int_value(usage.get("prompt_tokens"))
        or _int_value(usage.get("promptTokenCount"))
    )
    output_tokens = (
        _int_value(usage.get("output_tokens"))
        or _int_value(usage.get("completion_tokens"))
        or _int_value(usage.get("completionTokens"))
        or _int_value(usage.get("candidatesTokenCount"))
    )
    total_tokens = (
        _int_value(usage.get("total_tokens"))
        or _int_value(usage.get("totalTokens"))
        or _int_value(usage.get("totalTokenCount"))
    )
    cached_tokens = (
        _int_value(usage.get("cached_tokens"))
        or _int_value(usage.get("cache_read_input_tokens"))
    )
    cache_write_tokens = _int_value(usage.get("cache_creation_input_tokens"))

    if input_tokens or output_tokens:
        return TokenUsage(
            input_tokens=input_tokens,
            output_tokens=output_tokens,
            total_tokens=input_tokens + output_tokens,
            cache_read_tokens=cached_tokens,
            cache_write_tokens=cache_write_tokens,
        )
    if total_tokens:
        return TokenUsage(total_tokens=total_tokens, cache_read_tokens=cached_tokens)
    return None


def parse_terminal_token_usage(output: str) -> Optional[TokenUsage]:
    """Extract token usage from common JSONL and text summaries."""
    total = TokenUsage()
    saw_split_usage = False
    aggregate_total = 0

    for line in (output or "").splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        try:
            event = json.loads(stripped)
        except json.JSONDecodeError:
            continue
        if not isinstance(event, dict):
            continue
        parsed = _usage_from_mapping(event)
        if not parsed:
            continue
        if parsed.input_tokens or parsed.output_tokens:
            saw_split_usage = True
            total.input_tokens += parsed.input_tokens
            total.output_tokens += parsed.output_tokens
            total.cache_read_tokens += parsed.cache_read_tokens
            total.cache_write_tokens += parsed.cache_write_tokens
        else:
            aggregate_total += parsed.total_tokens

    if saw_split_usage:
        total.total_tokens = total.input_tokens + total.output_tokens
        return total
    if aggregate_total:
        return TokenUsage(total_tokens=aggregate_total)

    text = output or ""
    input_match = re.search(
        r"(?i)(?:input|prompt)\s+tokens?\D+([\d,]+)", text
    )
    output_match = re.search(
        r"(?i)(?:output|completion|generated)\s+tokens?\D+([\d,]+)", text
    )
    total_match = re.search(r"(?i)total\s+tokens?\D+([\d,]+)", text)

    input_tokens = _int_value(input_match.group(1)) if input_match else 0
    output_tokens = _int_value(output_match.group(1)) if output_match else 0
    total_tokens = _int_value(total_match.group(1)) if total_match else 0

    if input_tokens or output_tokens:
        return TokenUsage(
            input_tokens=input_tokens,
            output_tokens=output_tokens,
            total_tokens=input_tokens + output_tokens,
        )
    if total_tokens:
        return TokenUsage(total_tokens=total_tokens)
    return None


def parse_terminal_cost(output: str) -> Optional[float]:
    match = re.search(r"(?i)(?:cost|total\s+cost)\s*[:=]?\s*\$?\s*([0-9]+(?:\.[0-9]+)?)", output or "")
    if not match:
        return None
    try:
        return float(match.group(1))
    except ValueError:
        return None


def parse_terminal_agent_steps(output: str) -> tuple[Optional[int], Optional[str]]:
    """Extract a coarse action count from common CLI summaries or logs."""
    text = output or ""
    explicit = re.search(
        r"(?i)(?:agent\s+steps?|steps?|tool\s+calls?|actions?)\s*[:=]\s*([\d,]+)",
        text,
    )
    if explicit:
        return _int_value(explicit.group(1)), "cli_summary"

    count = 0
    for line in text.splitlines():
        low = line.lower().strip()
        if low.startswith("warning:"):
            continue
        if any(
            marker in low
            for marker in (
                "[tool call]",
                "calling tool",
                "run command:",
                "shell command:",
                "edit file:",
                "write file:",
                "apply patch",
            )
        ):
            count += 1
    if count:
        return count, "trajectory_action_lines"
    return None, None


class EnvTerminalAgentSolver(BaseSolver):
    """Generic prompt-mode terminal solver configured through environment vars."""

    SUPPORTS_MCP = False
    SUPPORTS_SYSTEM_PROMPT = False

    FAMILY = "terminal"
    CMD_ENV = "TERMINAL_AGENT_CMD"
    ARGS_ENV = "TERMINAL_AGENT_ARGS"
    DEFAULT_COMMANDS: tuple[str, ...] = ()
    INSTALL_HINT = "Install the provider CLI and configure the command environment variable."

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
        self.command = self._resolve_command()

    def _resolve_command(self) -> str:
        configured = os.environ.get(self.CMD_ENV, "").strip()
        candidates = [configured] if configured else list(self.DEFAULT_COMMANDS)
        for candidate in candidates:
            if not candidate:
                continue
            resolved = shutil.which(candidate)
            if resolved:
                return resolved
        return candidates[0] if candidates else self.FAMILY

    def _extra_args(self) -> list[str]:
        configured = os.environ.get(self.ARGS_ENV, "")
        if not configured.strip():
            return []
        return shlex.split(configured, posix=(os.name != "nt"))

    def _build_command(self, prompt: str) -> list[str]:
        cmd = [self.command]
        cmd.extend(self._extra_args())
        if self.model:
            cmd.extend(["--model", self.model])
        cmd.extend(["-p", prompt])
        return cmd

    @staticmethod
    def is_rate_limit_error(error_message: str) -> bool:
        error_lower = (error_message or "").lower()
        return any(
            keyword in error_lower
            for keyword in (
                "rate limit",
                "rate_limit",
                "ratelimit",
                "quota exceeded",
                "quota_exceeded",
                "429",
                "too many requests",
                "resource exhausted",
                "insufficient quota",
                "throttle",
                "throttled",
            )
        )

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
            print(f"SENDING PROMPT TO {self.FAMILY.upper()} CODE:")
            print("=" * 60)
            print(prompt)
            print("=" * 60)
            cmd_str = " ".join(c if " " not in c else f'"{c}"' for c in cmd[:-1])
            print(f"Running: {cmd_str} \"...\"")

        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=self.timeout_seconds,
                cwd=os.getcwd(),
            )
            duration = time.time() - start_time
            stdout = result.stdout or ""
            stderr = result.stderr or ""
            combined = f"{stdout}\n{stderr}"
            token_usage = parse_terminal_token_usage(combined)
            parsed_cost = parse_terminal_cost(combined)
            cost_usd = parsed_cost if parsed_cost is not None else 0.0
            cost_usd_is_estimate = False
            cost_note = None
            if token_usage and parsed_cost is None:
                cost_usd = token_usage.calculate_cost(self.model or self.FAMILY)
                cost_usd_is_estimate = token_usage.total_tokens > 0 and (
                    token_usage.input_tokens == 0 and token_usage.output_tokens == 0
                )
                if cost_usd_is_estimate:
                    cost_note = "estimated from aggregate terminal token count with blended pricing"
            steps, steps_source = parse_terminal_agent_steps(combined)

            if result.returncode != 0:
                message = f"{self.FAMILY} CLI failed with exit code {result.returncode}"
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
                is_rate_limited=self.is_rate_limit_error(combined),
                token_usage=token_usage,
                model=self.model or self.FAMILY,
                cost_usd=cost_usd,
                cost_usd_is_estimate=cost_usd_is_estimate,
                cost_estimate_note=cost_note,
                agent_steps=steps,
                agent_steps_source=steps_source,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )

        except subprocess.TimeoutExpired:
            duration = time.time() - start_time
            return SolverResult(
                success=False,
                message=f"{self.FAMILY} CLI timed out after {self.timeout_seconds}s",
                duration_seconds=duration,
                model=self.model or self.FAMILY,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )
        except FileNotFoundError:
            return SolverResult(
                success=False,
                message=f"{self.FAMILY} CLI not found. {self.INSTALL_HINT}",
                duration_seconds=0.0,
                model=self.model or self.FAMILY,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )
        except Exception as exc:
            duration = time.time() - start_time
            error_msg = str(exc)
            return SolverResult(
                success=False,
                message=f"Error invoking {self.FAMILY} CLI: {error_msg}",
                duration_seconds=duration,
                is_rate_limited=self.is_rate_limit_error(error_msg),
                model=self.model or self.FAMILY,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )


class DeepSeekCodeSolver(EnvTerminalAgentSolver):
    FAMILY = "deepseek"
    CMD_ENV = "DEEPSEEK_CODE_CMD"
    ARGS_ENV = "DEEPSEEK_CODE_ARGS"
    DEFAULT_COMMANDS = ("deepseek", "deepseek-code")
    INSTALL_HINT = "Install DeepSeek's coding-agent CLI, or set DEEPSEEK_CODE_CMD."


class GLMCodeSolver(EnvTerminalAgentSolver):
    FAMILY = "glm"
    CMD_ENV = "GLM_CODE_CMD"
    ARGS_ENV = "GLM_CODE_ARGS"
    DEFAULT_COMMANDS = ("glm", "glm-code")
    INSTALL_HINT = "Install GLM's coding-agent CLI, or set GLM_CODE_CMD."


class MuseCodeSolver(EnvTerminalAgentSolver):
    FAMILY = "muse"
    CMD_ENV = "MUSE_CODE_CMD"
    ARGS_ENV = "MUSE_CODE_ARGS"
    DEFAULT_COMMANDS = ("muse", "muse-code", "spark")
    INSTALL_HINT = "Install Muse Spark's coding-agent CLI, or set MUSE_CODE_CMD."
