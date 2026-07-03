#!/usr/bin/env python3
"""
OpenAI Codex solver for gamedev benchmark tasks.
Uses Codex CLI with MCP server for Godot screenshots.
"""

import json
import re
import time
import os
import subprocess
from pathlib import Path
from typing import Optional

from unrealbench.src.base_solver import BaseSolver
from unrealbench.src.utils.data_types import SolverResult, TokenUsage


class CodexSolver(BaseSolver):
    """Solver that uses OpenAI Codex CLI to complete game development tasks."""

    # Solver capabilities (required by BaseSolver)
    SUPPORTS_MCP = True
    SUPPORTS_SYSTEM_PROMPT = False  # Codex embeds context in main prompt

    def __init__(
        self,
        timeout_seconds: int = 3600,
        debug: bool = False,
        use_mcp: bool = False,
        model: Optional[str] = None,
        approval_policy: str = "never",      # never | auto-edit | full-auto
        sandbox: str = "workspace-write",    # read-only | workspace-write | danger-full-access
        use_runtime_video: bool = False,
        reasoning_level: str = "default",
    ):
        # Call parent constructor (handles MCP validation)
        super().__init__(
            timeout_seconds,
            debug,
            use_mcp,
            use_runtime_video,
            reasoning_level=reasoning_level,
        )

        # Codex-specific parameters
        self.model = model
        self.approval_policy = approval_policy
        self.sandbox = sandbox

        # Only configure MCP if enabled
        if use_mcp:
            self._ensure_mcp_config()

    def _configured_reasoning_level(self) -> str:
        config_file = Path.home() / ".codex" / "config.toml"
        if not config_file.exists():
            return "default"
        try:
            text = config_file.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            return "default"
        for line in text.splitlines():
            stripped = line.strip()
            if stripped.startswith("model_reasoning_effort"):
                _, _, value = stripped.partition("=")
                return value.strip().strip('"').strip("'") or "default"
        return "default"

    def _ensure_mcp_config(self):
        """Ensure ~/.codex/config.toml contains godot-screenshot MCP server config."""
        config_dir = Path.home() / ".codex"
        config_file = config_dir / "config.toml"

        mcp_config = '''
[mcp_servers.godot-screenshot]
command = "uv"
args = ["run", "unrealbench-mcp"]
'''

        config_dir.mkdir(parents=True, exist_ok=True)

        if config_file.exists():
            content = config_file.read_text()
            if "godot-screenshot" not in content:
                # Append MCP config
                with open(config_file, 'a') as f:
                    f.write("\n" + mcp_config)
                if self.debug:
                    print(f"Added godot-screenshot MCP config to {config_file}")
        else:
            # Create new config file
            config_file.write_text(mcp_config.strip())
            if self.debug:
                print(f"Created Codex config at {config_file}")

    @staticmethod
    def is_rate_limit_error(error_message: str) -> bool:
        """Check if the error message indicates API rate limit."""
        error_lower = error_message.lower()
        rate_limit_keywords = [
            "rate limit", "rate_limit", "ratelimit",
            "quota exceeded", "429", "too many requests",
        ]
        return any(keyword in error_lower for keyword in rate_limit_keywords)

    def solve_task(self) -> SolverResult:
        """Solve the task using Codex CLI."""
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
            print("SENDING PROMPT TO CODEX CLI:")
            print("=" * 60)
            print(prompt)
            print("=" * 60)

        try:
            # Build codex exec command
            cmd = ["codex"]
            if self.model:
                cmd.extend(["--model", self.model])
            reasoning_applied = self._configured_reasoning_level()
            if self.reasoning_level != "default":
                cmd.extend(["-c", f'model_reasoning_effort="{self.reasoning_level}"'])
                reasoning_applied = self.reasoning_level
            cmd.extend([
                "exec",
                "--json",
                "--skip-git-repo-check",
                "--yolo",
                "-s", 
                "danger-full-access",
                "-C", 
                str(os.getcwd()),
                prompt,
            ])

            if self.debug:
                cmd_str = " ".join([c if " " not in c else f'"{c}"' for c in cmd[:-1]])
                print(f"Running: {cmd_str} \"...\"")
                print("\nCODEX TRAJECTORY:")
                print("=" * 60)

            # Run Codex
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                # The codex CLI emits UTF-8 (smart quotes, em-dashes, box-drawing).
                # Without an explicit encoding, subprocess decodes with the platform
                # default — cp1252 on Windows — which throws UnicodeDecodeError on
                # non-mappable bytes (e.g. 0x9d) in the reader thread. Force UTF-8
                # and replace any stray bytes so capture never crashes.
                encoding="utf-8",
                errors="replace",
                timeout=self.timeout_seconds,
                cwd=os.getcwd(),
            )

            duration = time.time() - start_time
            stdout = result.stdout
            stderr = result.stderr

            if self.debug:
                # Parse and print key events
                self._print_trajectory(stdout)
                print(f"\n\nDuration: {duration:.2f} seconds")
                print(f"Exit code: {result.returncode}")
                if stderr:
                    print(f"Stderr: {stderr[:500]}")
                print("=" * 60)

            # Parse final response and token usage
            final_response = self._parse_final_response(stdout)
            token_usage = self._parse_token_usage(stdout)
            agent_steps = self._parse_agent_steps(stdout)
            model_used = self.model

            # Calculate cost
            cost_usd = 0.0
            if token_usage:
                cost_usd = token_usage.calculate_cost(model_used)
            cost_usd_is_estimate = bool(
                token_usage
                and token_usage.total_tokens > 0
                and token_usage.input_tokens == 0
                and token_usage.output_tokens == 0
            )
            cost_estimate_note = (
                "estimated from aggregate Codex `tokens used` count with blended pricing"
                if cost_usd_is_estimate
                else None
            )

            if self.debug and token_usage:
                print(f"Tokens: input={token_usage.input_tokens}, output={token_usage.output_tokens}, total={token_usage.total_tokens}")
                print(f"Cost: ${cost_usd:.4f}")

            # Construct message: include stderr if command failed
            if result.returncode != 0:
                error_msg = f"Codex command failed (exit code {result.returncode})"
                if stderr and stderr.strip():
                    error_msg += f"\nSTDERR: {stderr.strip()}"
                if final_response:
                    error_msg += f"\nFinal response: {final_response}"
                message = error_msg
            else:
                message = final_response or "No response detected."

            return SolverResult(
                success=result.returncode == 0,
                message=message,
                duration_seconds=duration,
                stdout=stdout,
                stderr=stderr,
                token_usage=token_usage,
                model=model_used,
                cost_usd=cost_usd,
                cost_usd_is_estimate=cost_usd_is_estimate,
                cost_estimate_note=cost_estimate_note,
                agent_steps=agent_steps,
                agent_steps_source="codex_jsonl_actions" if agent_steps is not None else None,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied=reasoning_applied,
            )

        except subprocess.TimeoutExpired:
            duration = time.time() - start_time
            return SolverResult(
                success=False,
                message=f"Codex execution timed out after {self.timeout_seconds}s",
                duration_seconds=duration,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied=(
                    self.reasoning_level
                    if self.reasoning_level != "default"
                    else "default"
                ),
            )
        except FileNotFoundError:
            return SolverResult(
                success=False,
                message="Codex CLI not found. Install with: npm i -g @openai/codex",
                duration_seconds=0.0,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )
        except Exception as e:
            duration = time.time() - start_time
            error_msg = str(e)
            is_rate_limited = self.is_rate_limit_error(error_msg)

            if self.debug:
                print(f"\nERROR INVOKING CODEX: {error_msg}")
                if is_rate_limited:
                    print("⚠️  DETECTED RATE LIMIT/QUOTA ERROR")
                print("=" * 60)

            return SolverResult(
                success=False,
                message=f"Error invoking Codex: {error_msg}",
                duration_seconds=duration,
                is_rate_limited=is_rate_limited,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied=(
                    self.reasoning_level
                    if self.reasoning_level != "default"
                    else "default"
                ),
            )

    def _print_trajectory(self, output: str):
        """Print key events from Codex execution trajectory."""
        for line in output.strip().split("\n"):
            if not line:
                continue
            try:
                event = json.loads(line)
                event_type = event.get("type", "")

                if event_type == "turn.started":
                    print(f"\n[Turn Started]")
                elif event_type == "item.tool_call":
                    tool_name = event.get("name", "unknown")
                    args = event.get("arguments", {})
                    print(f"\n[Tool Call] {tool_name}({json.dumps(args)[:100]})")
                elif event_type == "item.tool_result":
                    print(f"[Tool Result] received")
                elif event_type == "item.message":
                    content = event.get("content", "")
                    if content:
                        preview = content[:200] + "..." if len(content) > 200 else content
                        print(f"[Message] {preview}")
                elif event_type == "turn.completed":
                    print(f"\n[Turn Completed]")
                elif event_type == "item.file_edit":
                    file_path = event.get("path", "unknown")
                    print(f"[File Edit] {file_path}")
                elif event_type == "item.shell_command":
                    cmd = event.get("command", "")
                    print(f"[Shell] {cmd[:100]}")

            except json.JSONDecodeError:
                # Non-JSON line, possibly error message
                if line.strip() and self.debug:
                    print(f"[Raw] {line[:100]}")

    def _parse_final_response(self, output: str) -> Optional[str]:
        """Parse JSON Lines output to get final response."""
        final_response = None
        for line in output.strip().split("\n"):
            if not line:
                continue
            try:
                event = json.loads(line)
                if event.get("type") == "turn.completed":
                    final_response = event.get("finalResponse", "")
                elif event.get("type") == "item.message":
                    # Save last message as fallback
                    content = event.get("content", "")
                    if content:
                        final_response = content
            except json.JSONDecodeError:
                continue
        return final_response

    def _parse_token_usage(self, output: str) -> Optional[TokenUsage]:
        """Parse Codex output to get token usage."""
        total_input = 0
        total_output = 0
        total_cached = 0
        total_reported = 0

        for line in output.strip().split("\n"):
            if not line:
                continue
            try:
                event = json.loads(line)
                event_type = event.get("type", "")

                if event_type == "token_count":
                    total_input += event.get("input_tokens", 0)
                    total_output += event.get("output_tokens", 0)
                    total_cached += event.get("cached_tokens", 0)
                    total_reported += event.get("total_tokens", 0)
                elif event_type in {"turn.completed", "response.completed"}:
                    usage = event.get("usage", {})
                    if usage:
                        total_input += usage.get("input_tokens", 0)
                        total_output += usage.get("output_tokens", 0)
                        total_cached += usage.get("cached_tokens", 0)
                        total_cached += usage.get("cache_read_input_tokens", 0)
                        total_reported += usage.get("total_tokens", 0)

                payload = event.get("payload", {})
                if isinstance(payload, dict):
                    payload_type = payload.get("type", "")
                    if payload_type == "token_count":
                        total_input += payload.get("input_tokens", 0)
                        total_output += payload.get("output_tokens", 0)
                        total_cached += payload.get("cached_tokens", 0)
                        total_reported += payload.get("total_tokens", 0)

            except json.JSONDecodeError:
                continue

        if total_input > 0 or total_output > 0:
            return TokenUsage(
                input_tokens=total_input,
                output_tokens=total_output,
                total_tokens=total_input + total_output,
                cache_read_tokens=total_cached,
                cache_write_tokens=0,
            )

        aggregate_total = total_reported or self._parse_total_tokens_used(output)
        if aggregate_total > 0:
            return TokenUsage(total_tokens=aggregate_total)
        return None

    @staticmethod
    def _parse_total_tokens_used(output: str) -> int:
        """Parse Codex's plain text trailer: `tokens used` followed by a count."""
        match = re.search(r"(?im)^tokens used\s*\r?\n\s*([\d,]+)\s*$", output or "")
        if not match:
            return 0
        return int(match.group(1).replace(",", ""))

    @staticmethod
    def _parse_agent_steps(output: str) -> Optional[int]:
        """Count Codex agent actions from JSONL trajectory events."""
        count = 0
        action_types = {
            "item.tool_call",
            "item.file_edit",
            "item.shell_command",
        }
        payload_action_types = {
            "tool_call",
            "file_edit",
            "shell_command",
        }
        for line in output.strip().split("\n"):
            if not line:
                continue
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue
            if event.get("type") in action_types:
                count += 1
                continue
            item = event.get("item", {})
            if event.get("type") == "item.started" and isinstance(item, dict):
                if item.get("type") in {"command_execution", "file_change", "tool_call"}:
                    count += 1
                    continue
            payload = event.get("payload", {})
            if isinstance(payload, dict) and payload.get("type") in payload_action_types:
                count += 1
        return count if count > 0 else None



def main():
    """Main function for testing the solver."""
    solver = CodexSolver(debug=True)
    result = solver.solve_task()
    print("\n" + "=" * 60)
    print("RESULT:")
    print("=" * 60)
    print(f"Success: {result.success}")
    print(f"Message: {result.message[:500] if result.message else 'None'}")
    print(f"Duration: {result.duration_seconds:.2f}s")


if __name__ == "__main__":
    main()
