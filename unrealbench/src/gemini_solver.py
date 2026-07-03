#!/usr/bin/env python3
"""
Google Gemini solver for gamedev benchmark tasks.
Uses Gemini CLI (https://github.com/google-gemini/gemini-cli) for task completion.
"""

import asyncio
import json
import time
import os
import shutil
from pathlib import Path
from typing import Optional

from unrealbench.src.base_solver import BaseSolver
from unrealbench.src.utils.data_types import SolverResult, TokenUsage


GEMINI_THINKING_BUDGETS = {
    "low": 1024,
    "medium": 4096,
    "high": 8192,
    "xhigh": 16384,
    "max": 24576,
}


class GeminiSolver(BaseSolver):
    """Solver that uses Google Gemini CLI to complete game development tasks."""

    # Solver capabilities (required by BaseSolver)
    SUPPORTS_MCP = True
    SUPPORTS_SYSTEM_PROMPT = False

    def __init__(
        self,
        timeout_seconds: int = 3600,
        debug: bool = False,
        use_yolo: bool = True,  # Auto-approve all actions
        model: Optional[str] = None,  # Model name to use with --model flag
        use_mcp: bool = False,
        use_runtime_video: bool = False,
        reasoning_level: str = "default",
    ):
        """Initialize the Gemini solver.

        Args:
            timeout_seconds: Maximum time to wait for completion
            debug: Enable verbose output
            use_yolo: Use --yolo flag to auto-approve all actions
            model: Model name to pass via --model flag (optional)
            use_mcp: Whether to use MCP tools (enables unrealbench-mcp server via gemini mcp enable/disable)
            use_runtime_video: Whether to append Godot runtime video instructions to prompts
        """
        # Call parent constructor (handles MCP validation)
        super().__init__(
            timeout_seconds,
            debug,
            use_mcp,
            use_runtime_video,
            reasoning_level=reasoning_level,
        )

        # Gemini-specific parameters
        self.use_yolo = use_yolo
        self.model = model
        # On Windows the CLI is installed as an npm ".cmd" shim (no ".exe"), which
        # asyncio.create_subprocess_exec cannot launch by bare name — it raises
        # FileNotFoundError. Resolve the full path (incl. extension) so the launch
        # works on every platform. Falls back to bare name if not on PATH.
        self.gemini_cmd = shutil.which("gemini") or "gemini"

    def _configure_reasoning_alias(self) -> tuple[Optional[str], str]:
        """Configure Gemini thinking budget through workspace settings."""
        if self.reasoning_level == "default":
            return self.model, "default"
        budget = GEMINI_THINKING_BUDGETS.get(self.reasoning_level)
        if budget is None or not self.model:
            return self.model, "default"

        settings_dir = Path(os.getcwd()) / ".gemini"
        settings_dir.mkdir(exist_ok=True)
        settings_path = settings_dir / "settings.json"
        settings = {}
        if settings_path.exists():
            try:
                settings = json.loads(settings_path.read_text(encoding="utf-8"))
            except json.JSONDecodeError:
                settings = {}

        alias = f"unrealbench-{self.model}-{self.reasoning_level}".replace("/", "-")
        model_configs = settings.setdefault("modelConfigs", {})
        aliases = model_configs.setdefault("customAliases", {})
        aliases[alias] = {
            "modelConfig": {
                "model": self.model,
                "generateContentConfig": {
                    "thinkingConfig": {
                        "thinkingBudget": budget,
                    }
                },
            }
        }
        settings_path.write_text(json.dumps(settings, indent=2), encoding="utf-8")
        return alias, self.reasoning_level

    @staticmethod
    def is_rate_limit_error(error_message: str) -> bool:
        """Check if the error message indicates API rate limit or quota exceeded."""
        error_lower = error_message.lower()
        rate_limit_keywords = [
            "rate limit",
            "rate_limit",
            "ratelimit",
            "quota exceeded",
            "quota_exceeded",
            "429",
            "too many requests",
            "resource exhausted",
            "resource_exhausted",
        ]
        return any(keyword in error_lower for keyword in rate_limit_keywords)

    async def _ensure_mcp_server_configured(self) -> bool:
        """Ensure the unrealbench-mcp server is configured in Gemini CLI.

        Checks if server exists, adds it if missing.

        Returns:
            True if server is configured, False otherwise
        """
        # Check if server is already configured by listing MCP servers
        try:
            proc = await asyncio.create_subprocess_exec(
                self.gemini_cmd, "mcp", "list",
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            stdout_bytes, _ = await proc.communicate()
            stdout = (stdout_bytes or b"").decode(errors="ignore")

            # If unrealbench-mcp is in the list, it's already configured
            if "unrealbench-mcp" in stdout:
                if self.debug:
                    print("MCP server unrealbench-mcp is already configured")
                return True

            # Server not found, add it
            if self.debug:
                print("Adding MCP server unrealbench-mcp...")

            proc = await asyncio.create_subprocess_exec(
                self.gemini_cmd, "mcp", "add", "unrealbench-mcp", "uv", "run", "unrealbench-mcp",
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )
            await proc.communicate()

            if proc.returncode == 0:
                if self.debug:
                    print("MCP server unrealbench-mcp added successfully")
                return True
            else:
                if self.debug:
                    print(f"Failed to add MCP server (exit code: {proc.returncode})")
                return False

        except Exception as e:
            if self.debug:
                print(f"Error configuring MCP server: {e}")
            return False

    async def solve_task_async(self) -> SolverResult:
        """Solve the task in the current directory using Gemini CLI."""
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
            print("SENDING PROMPT TO GEMINI CLI:")
            print("=" * 60)
            print(prompt)
            print("=" * 60)

        # Ensure MCP server is configured if requested
        if self.use_mcp:
            mcp_configured = await self._ensure_mcp_server_configured()
            if not mcp_configured and self.debug:
                print("Warning: Could not configure MCP server. Continuing without screenshot capability.")

        try:
            # Build gemini command
            cmd = [self.gemini_cmd]
            model_for_cli, reasoning_applied = self._configure_reasoning_alias()

            if self.use_yolo:
                cmd.append("--yolo")

            # Benchmark tasks run in ephemeral workspaces, so Gemini CLI trust
            # prompts must be bypassed for non-interactive execution.
            cmd.append("--skip-trust")

            if model_for_cli:
                cmd.extend(["--model", model_for_cli])

            cmd.extend(["--output-format", "stream-json"])

            cmd.extend(["-p", prompt])

            if self.debug:
                print(f"\nRunning: {' '.join(cmd[:3])} -p \"...\"")
                print("\nGEMINI TRAJECTORY:")
                print("=" * 60)

            # Run Gemini CLI
            proc = await asyncio.create_subprocess_exec(
                *cmd,
                cwd=os.getcwd(),
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE,
            )

            try:
                stdout_bytes, stderr_bytes = await asyncio.wait_for(
                    proc.communicate(),
                    timeout=self.timeout_seconds,
                )
            except asyncio.TimeoutError:
                proc.kill()
                await proc.wait()
                duration = time.time() - start_time
                return SolverResult(
                    success=False,
                    message=f"Gemini CLI timed out after {self.timeout_seconds}s",
                    duration_seconds=duration,
                    reasoning_level_requested=self.reasoning_level,
                    reasoning_level_applied=(
                        self.reasoning_level
                        if self.reasoning_level != "default"
                        else "default"
                    ),
                )

            duration = time.time() - start_time
            stdout = (stdout_bytes or b"").decode(errors="ignore")
            stderr = (stderr_bytes or b"").decode(errors="ignore")

            if self.debug:
                if stdout:
                    print(stdout)
                if stderr:
                    print(f"\nStderr: {stderr}")
                print(f"\n\nDuration: {duration:.2f} seconds")
                print(f"Exit code: {proc.returncode}")
                print("=" * 60)

            # Parse token usage and model info if available (from JSON output)
            token_usage = self._parse_token_usage(stdout)
            agent_steps = self._parse_agent_steps(stdout)
            model_used = self._parse_model_name(stdout) or self.model or "gemini"

            # Calculate cost
            cost_usd = 0.0
            if token_usage:
                cost_usd = token_usage.calculate_cost(model_used)

            if self.debug and token_usage:
                print(f"Tokens: input={token_usage.input_tokens}, output={token_usage.output_tokens}, total={token_usage.total_tokens}")
                print(f"Cost: ${cost_usd:.4f}")

            # Check for errors in stderr
            combined_output = stdout if not stderr else f"{stdout}\n{stderr}"

            return SolverResult(
                success=proc.returncode == 0,
                message="Task completed" if proc.returncode == 0 else f"Gemini CLI exited with code {proc.returncode}",
                duration_seconds=duration,
                stdout=stdout,
                stderr=stderr,
                token_usage=token_usage,
                model=model_used,
                cost_usd=cost_usd,
                agent_steps=agent_steps,
                agent_steps_source="gemini_stream_json_actions" if agent_steps is not None else None,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied=reasoning_applied,
            )

        except FileNotFoundError:
            return SolverResult(
                success=False,
                message="Gemini CLI not found. Install from: https://github.com/google-gemini/gemini-cli",
                duration_seconds=0.0,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied="default",
            )
        except Exception as e:
            duration = time.time() - start_time
            error_msg = str(e)
            is_rate_limited = self.is_rate_limit_error(error_msg)

            if self.debug:
                print(f"\nERROR INVOKING GEMINI CLI: {error_msg}")
                if is_rate_limited:
                    print("⚠️  DETECTED RATE LIMIT/QUOTA ERROR")
                print("=" * 60)

            return SolverResult(
                success=False,
                message=f"Error invoking Gemini CLI: {error_msg}",
                duration_seconds=duration,
                is_rate_limited=is_rate_limited,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied=(
                    self.reasoning_level
                    if self.reasoning_level != "default"
                    else "default"
                ),
            )

    def _parse_token_usage(self, output: str) -> Optional[TokenUsage]:
        """Parse JSON output to extract token usage information.

        Gemini CLI with --output-format stream-json outputs events like:
        {"type": "usage", "input_tokens": 123, "output_tokens": 456}
        """
        total_input = 0
        total_output = 0
        total_cached = 0

        for line in output.strip().split("\n"):
            if not line:
                continue
            try:
                event = json.loads(line)

                # Look for usage information in various formats
                if event.get("type") == "usage":
                    total_input += event.get("input_tokens", 0)
                    total_output += event.get("output_tokens", 0)
                    total_cached += event.get("cached_tokens", 0)

                # Also check for usage nested in other events
                usage = event.get("usage", {})
                if usage:
                    total_input += usage.get("input_tokens", 0)
                    total_output += usage.get("output_tokens", 0)
                    total_cached += usage.get("cached_tokens", 0)

            except json.JSONDecodeError:
                # Not a JSON line, skip
                continue

        if total_input > 0 or total_output > 0:
            return TokenUsage(
                input_tokens=total_input,
                output_tokens=total_output,
                total_tokens=total_input + total_output,
                cache_read_tokens=total_cached,
                cache_write_tokens=0,
            )
        return None

    def _parse_model_name(self, output: str) -> Optional[str]:
        """Parse JSON output to extract the model name."""
        for line in output.strip().split("\n"):
            if not line:
                continue
            try:
                event = json.loads(line)
                model = event.get("model")
                if model:
                    return model
            except json.JSONDecodeError:
                continue
        return None


    def _parse_agent_steps(self, output: str) -> Optional[int]:
        """Count Gemini CLI action/tool events from stream JSON."""
        count = 0
        action_markers = {"tool_call", "toolCall", "function_call", "functionCall"}
        for line in output.strip().split("\n"):
            if not line:
                continue
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                continue
            event_type = str(event.get("type", ""))
            if event_type in action_markers or "tool" in event_type.lower():
                count += 1
                continue
            if any(key in event for key in action_markers):
                count += 1
        return count if count > 0 else None

    def solve_task(self) -> SolverResult:
        """Synchronous wrapper for async solve_task_async."""
        return asyncio.run(self.solve_task_async())


def main():
    """Main function for testing the solver."""
    solver = GeminiSolver(debug=True)
    result = solver.solve_task()
    print("\n" + "=" * 60)
    print("RESULT:")
    print("=" * 60)
    print(f"Success: {result.success}")
    print(f"Message: {result.message}")
    print(f"Duration: {result.duration_seconds:.2f}s")
    if result.token_usage:
        print(f"Tokens: {result.token_usage.total_tokens}")
        print(f"Cost: ${result.cost_usd:.4f}")


if __name__ == "__main__":
    main()
