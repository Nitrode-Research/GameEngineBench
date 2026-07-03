#!/usr/bin/env python3
"""
Google Antigravity CLI solver for UnrealBench tasks.

Antigravity CLI (the `agy` binary, https://antigravity.google) is Google's
successor to the deprecated Gemini CLI. It runs the same Gemini-family agent
harness from the terminal and supports a non-interactive `--print` (`-p`) mode,
which is what the benchmark drives.

Flag mapping vs. the old gemini-cli solver:
    gemini --yolo                 -> agy --dangerously-skip-permissions
    gemini --model X              -> agy --model X
    gemini -p "<prompt>"          -> agy -p "<prompt>"
    gemini --skip-trust           -> (none; agy has no trust prompt)
    gemini --output-format json   -> (none; agy print mode emits plain text,
                                       so token usage/cost are estimated)
    (n/a)                         -> agy --print-timeout <dur>  (default 5m!)
    (n/a)                         -> agy --log-file <path>      (saved for audit)

The 5-minute default `--print-timeout` is the critical gotcha: without an
override, long solves are killed at 5 minutes. We set it to the solver's full
wall-clock budget.
"""

import asyncio
import os
import re
import sqlite3
import shutil
import subprocess
import time
from pathlib import Path
from typing import Optional

from unrealbench.src.base_solver import BaseSolver
from unrealbench.src.utils.data_types import SolverResult, TokenUsage
from unrealbench.src.terminal_agent_solver import parse_terminal_agent_steps


AGY_TOOL_STEP_TYPES = {5, 7, 8, 9, 21, 132}

# Antigravity does not expose measured token usage in --print output, --log-file,
# or the local conversation DB. These are deliberately rough benchmark estimates,
# calibrated from ue_task_0010 smoke runs with measured usage (Codex GPT-5.5,
# Qwen 3.7, DeepSeek v4 Pro, Claude Opus 4.8). Use the larger of tool-step and
# model-call based estimates and mark both token usage and cost as estimated.
AGY_EST_INPUT_TOKENS_PER_TOOL_STEP = 26_000
AGY_EST_OUTPUT_TOKENS_PER_TOOL_STEP = 285
AGY_EST_INPUT_TOKENS_PER_LLM_CALL = 18_000
AGY_EST_OUTPUT_TOKENS_PER_LLM_CALL = 150


def _printable_blob_text(value: bytes) -> str:
    return re.sub(
        r"\s+",
        " ",
        "".join(chr(b) if 32 <= b < 127 else " " for b in value),
    )


def _parse_agy_conversation_id(text: str) -> Optional[str]:
    match = re.search(
        r"\bconversation\s+([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\b",
        text or "",
        re.IGNORECASE,
    )
    if match:
        return match.group(1)
    match = re.search(
        r"\bANTIGRAVITY_CONVERSATION_ID\s+([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\b",
        text or "",
        re.IGNORECASE,
    )
    return match.group(1) if match else None


def _agy_conversation_db(conversation_id: Optional[str]) -> Optional[Path]:
    if not conversation_id:
        return None
    db_path = Path.home() / ".gemini" / "antigravity-cli" / "conversations" / f"{conversation_id}.db"
    return db_path if db_path.exists() else None


def _parse_agy_actual_model(text: str, conversation_id: Optional[str]) -> tuple[Optional[str], Optional[str]]:
    label = None
    label_matches = re.findall(
        r'Propagating selected model override to backend: label="([^"]+)"',
        text or "",
    )
    if label_matches:
        label = label_matches[-1]

    model_id = None
    db_path = _agy_conversation_db(conversation_id)
    if db_path:
        con = None
        try:
            con = sqlite3.connect(db_path)
            con.row_factory = sqlite3.Row
            for row in con.execute("select data from gen_metadata order by idx desc"):
                data = row["data"]
                if not isinstance(data, bytes):
                    continue
                printable = _printable_blob_text(data)
                model_matches = [
                    value
                    for value in re.findall(r"\b(gemini-[a-z0-9._-]+)\b", printable, re.IGNORECASE)
                    if not value.lower().startswith("gemini-coder")
                ]
                if model_matches:
                    model_id = model_matches[-1]
                    break
        except sqlite3.Error:
            pass
        finally:
            if con is not None:
                con.close()

    return model_id, label


def _parse_agy_llm_calls(text: str, conversation_id: Optional[str]) -> tuple[Optional[int], Optional[str]]:
    db_path = _agy_conversation_db(conversation_id)
    if db_path:
        con = None
        try:
            con = sqlite3.connect(db_path)
            count = con.execute("select count(*) from gen_metadata").fetchone()[0]
            if count:
                return int(count), "antigravity_conversation_db_gen_metadata"
        except sqlite3.Error:
            pass
        finally:
            if con is not None:
                con.close()

    calls = len(re.findall(r"streamGenerateContent", text or ""))
    if calls:
        return calls, "antigravity_log_stream_generate_content"
    return None, None


def _parse_agy_agent_steps(text: str, conversation_id: Optional[str]) -> tuple[Optional[int], Optional[str]]:
    db_path = _agy_conversation_db(conversation_id)
    if db_path:
        con = None
        try:
            con = sqlite3.connect(db_path)
            placeholders = ",".join("?" for _ in AGY_TOOL_STEP_TYPES)
            count = con.execute(
                f"select count(*) from steps where step_type in ({placeholders})",
                tuple(sorted(AGY_TOOL_STEP_TYPES)),
            ).fetchone()[0]
            if count:
                return int(count), "antigravity_conversation_db_tool_steps"
        except sqlite3.Error:
            pass
        finally:
            if con is not None:
                con.close()

    confirmations = len(re.findall(r"Auto-approving tool confirmation:", text or ""))
    if confirmations:
        return confirmations, "antigravity_log_tool_confirmations"

    return parse_terminal_agent_steps(text)


def _estimate_agy_token_usage(
    agent_steps: Optional[int],
    llm_calls: Optional[int],
) -> tuple[Optional[TokenUsage], Optional[str]]:
    if not agent_steps and not llm_calls:
        return None, None

    estimated_input = 0
    estimated_output = 0
    note_parts = []
    if agent_steps:
        estimated_input = max(estimated_input, agent_steps * AGY_EST_INPUT_TOKENS_PER_TOOL_STEP)
        estimated_output = max(estimated_output, agent_steps * AGY_EST_OUTPUT_TOKENS_PER_TOOL_STEP)
        note_parts.append(f"agent_steps={agent_steps}")
    if llm_calls:
        estimated_input = max(estimated_input, llm_calls * AGY_EST_INPUT_TOKENS_PER_LLM_CALL)
        estimated_output = max(estimated_output, llm_calls * AGY_EST_OUTPUT_TOKENS_PER_LLM_CALL)
        note_parts.append(f"llm_calls={llm_calls}")

    usage = TokenUsage(
        input_tokens=int(estimated_input),
        output_tokens=int(estimated_output),
        total_tokens=int(estimated_input + estimated_output),
        cache_read_tokens=0,
        cache_write_tokens=0,
    )
    note = (
        "estimated Antigravity token usage from "
        + ", ".join(note_parts)
        + "; calibrated from same-task smoke runs with measured accounting; no cache discount assumed"
    )
    return usage, note



def _resolve_agy_cmd() -> str:
    """Locate the agy binary.

    Prefer PATH (after `agy install` adds it), then fall back to the default
    Windows install location used by the official installer.
    """
    found = shutil.which("agy")
    if found:
        return found
    default_win = os.path.expandvars(r"%LOCALAPPDATA%\agy\bin\agy.exe")
    if os.path.isfile(default_win):
        return default_win
    return "agy"


class AntigravitySolver(BaseSolver):
    """Solver that uses Google's Antigravity CLI (`agy`) to complete tasks."""

    # Solver capabilities (required by BaseSolver).
    # MCP for agy is plugin-based (`agy plugin ...`), not a stable
    # non-interactive surface yet, and the code-only UE tasks don't need it.
    SUPPORTS_MCP = False
    SUPPORTS_SYSTEM_PROMPT = False

    def __init__(
        self,
        timeout_seconds: int = 3600,
        debug: bool = False,
        use_yolo: bool = True,            # auto-approve all tool actions
        model: Optional[str] = None,      # passed to `agy --model`
        use_runtime_video: bool = False,
        reasoning_level: str = "default",
    ):
        super().__init__(
            timeout_seconds,
            debug,
            use_mcp=False,
            use_runtime_video=use_runtime_video,
            reasoning_level=reasoning_level,
        )
        self.use_yolo = use_yolo
        self.model = model
        self.agy_cmd = _resolve_agy_cmd()

    @staticmethod
    def is_rate_limit_error(error_message: str) -> bool:
        error_lower = (error_message or "").lower()
        keywords = [
            "rate limit", "rate_limit", "ratelimit",
            "quota exceeded", "quota_exceeded", "429",
            "too many requests", "resource exhausted", "resource_exhausted",
        ]
        return any(k in error_lower for k in keywords)

    @staticmethod
    def is_auth_error(output: str) -> bool:
        low = (output or "").lower()
        return any(k in low for k in (
            "not authenticated", "unauthenticated", "please log in",
            "please login", "run `agy`", "authorization url", "sign in",
            "no credentials",
        ))

    async def solve_task_async(self) -> SolverResult:
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

        if self.debug:
            print("=" * 60)
            print("SENDING PROMPT TO ANTIGRAVITY CLI (agy):")
            print("=" * 60)
            print(prompt)
            print("=" * 60)

        # agy print-mode timeout is a Go duration string; give it the full
        # solver budget (its default is only 5m, which would truncate solves).
        print_timeout = f"{int(self.timeout_seconds)}s"
        agy_log_path = Path(os.getcwd()) / "agy_cli.log"
        try:
            agy_log_path.unlink(missing_ok=True)
        except OSError:
            pass

        cmd = [
            self.agy_cmd,
            "--print-timeout",
            print_timeout,
            "--log-file",
            str(agy_log_path),
        ]
        if self.use_yolo:
            cmd.append("--dangerously-skip-permissions")
        if self.model:
            cmd.extend(["--model", self.model])
        cmd.extend(["-p", prompt])

        if self.debug:
            print(f"\nRunning: {self.agy_cmd} --print-timeout {print_timeout} "
                  f"{'--dangerously-skip-permissions ' if self.use_yolo else ''}"
                  f"{('--model ' + self.model + ' ') if self.model else ''}-p \"...\"")
            print("\nAGY TRAJECTORY:")
            print("=" * 60)

        # agy renders its interactive trajectory directly to the parent console
        # (a TTY), bypassing the stdout/stderr pipes we capture below. Outside
        # debug mode, give the child a detached/invisible console on Windows so
        # that UI doesn't bleed into the benchmark output; the final response
        # still arrives on the captured stdout pipe. In debug we inherit the
        # console so the live trajectory stays visible.
        popen_kwargs = {}
        if not self.debug and os.name == "nt":
            popen_kwargs["creationflags"] = getattr(subprocess, "CREATE_NO_WINDOW", 0)

        try:
            proc = await asyncio.create_subprocess_exec(
                *cmd,
                cwd=os.getcwd(),
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
                **popen_kwargs,
            )

            try:
                stdout_bytes, stderr_bytes = await asyncio.wait_for(
                    proc.communicate(),
                    # small buffer over agy's own print-timeout
                    timeout=self.timeout_seconds + 60,
                )
            except asyncio.TimeoutError:
                proc.kill()
                await proc.wait()
                duration = time.time() - start_time
                return SolverResult(
                    success=False,
                    message=f"Antigravity CLI timed out after {self.timeout_seconds}s",
                    duration_seconds=duration,
                    reasoning_level_requested=self.reasoning_level,
                    reasoning_level_applied="default",
                )

            duration = time.time() - start_time
            stdout = (stdout_bytes or b"").decode(errors="ignore")
            stderr = (stderr_bytes or b"").decode(errors="ignore")
            if agy_log_path.exists():
                try:
                    agy_log = agy_log_path.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    agy_log = ""
                if agy_log:
                    stderr = f"{stderr}\n\nAGY LOG FILE ({agy_log_path}):\n{agy_log}"

            if self.debug:
                if stdout:
                    print(stdout)
                if stderr:
                    print(f"\nStderr: {stderr}")
                print(f"\n\nDuration: {duration:.2f} seconds")
                print(f"Exit code: {proc.returncode}")
                print("=" * 60)

            combined = f"{stdout}\n{stderr}"
            conversation_id = _parse_agy_conversation_id(combined)
            agent_steps, agent_steps_source = _parse_agy_agent_steps(combined, conversation_id)
            llm_calls, llm_calls_source = _parse_agy_llm_calls(combined, conversation_id)
            actual_model_id, actual_model_label = _parse_agy_actual_model(combined, conversation_id)

            # agy print mode emits plain text and its local protobuf/SQLite state
            # does not expose measured token usage/cost. Estimate API-equivalent
            # usage from model-call/tool-step counts and mark it explicitly.
            model_used = actual_model_id or self.model or "antigravity"
            token_usage, estimate_note = _estimate_agy_token_usage(agent_steps, llm_calls)
            cost_usd = token_usage.calculate_cost(model_used) if token_usage else 0.0
            note_parts = [
                "Antigravity CLI print mode does not expose measured input/output token usage or billable cost",
                "Gemini cost is an API-equivalent estimate, not measured Antigravity billing",
            ]
            if estimate_note:
                note_parts.append(estimate_note)
            if llm_calls is not None:
                note_parts.append(f"llm_calls_source={llm_calls_source}")
            if actual_model_label:
                note_parts.append(f"Antigravity label: {actual_model_label}")
            if self.model and model_used != self.model:
                note_parts.append(f"requested model {self.model!r} resolved to {model_used!r}")
            cost_note = "; ".join(note_parts)

            if proc.returncode != 0 and self.is_auth_error(combined):
                return SolverResult(
                    success=False,
                    message=(
                        "Antigravity CLI is not authenticated. Run `agy` once "
                        "interactively to sign in (Google account / device code), "
                        "then re-run the benchmark."
                    ),
                    duration_seconds=duration,
                    stdout=stdout,
                    stderr=stderr,
                    model=model_used,
                    agent_steps=agent_steps,
                    agent_steps_source=agent_steps_source,
                    reasoning_level_requested=self.reasoning_level,
                    reasoning_level_applied="default",
                )

            if self.model and model_used != self.model and proc.returncode == 0:
                message = (
                    f"Task completed; requested model {self.model!r} resolved to {model_used!r}"
                )
            else:
                message = (
                    "Task completed" if proc.returncode == 0
                    else f"Antigravity CLI exited with code {proc.returncode}"
                )

            return SolverResult(
                success=proc.returncode == 0,
                message=message,
                duration_seconds=duration,
                stdout=stdout,
                stderr=stderr,
                token_usage=token_usage,
                token_usage_is_estimate=token_usage is not None,
                model=model_used,
                cost_usd=cost_usd,
                cost_usd_is_estimate=token_usage is not None,
                cost_estimate_note=cost_note,
                agent_steps=agent_steps,
                agent_steps_source=agent_steps_source,
                is_rate_limited=self.is_rate_limit_error(combined),
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )

        except FileNotFoundError:
            return SolverResult(
                success=False,
                message=(
                    "Antigravity CLI (agy) not found. Install with: "
                    "irm https://antigravity.google/cli/install.ps1 | iex"
                ),
                duration_seconds=0.0,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )
        except Exception as e:  # noqa: BLE001
            duration = time.time() - start_time
            error_msg = str(e)
            if self.debug:
                print(f"\nERROR INVOKING ANTIGRAVITY CLI: {error_msg}")
                print("=" * 60)
            return SolverResult(
                success=False,
                message=f"Error invoking Antigravity CLI: {error_msg}",
                duration_seconds=duration,
                is_rate_limited=self.is_rate_limit_error(error_msg),
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )

    def solve_task(self) -> SolverResult:
        return asyncio.run(self.solve_task_async())


def main():
    solver = AntigravitySolver(debug=True)
    result = solver.solve_task()
    print("\n" + "=" * 60)
    print("RESULT:")
    print("=" * 60)
    print(f"Success: {result.success}")
    print(f"Message: {result.message}")
    print(f"Duration: {result.duration_seconds:.2f}s")


if __name__ == "__main__":
    main()
