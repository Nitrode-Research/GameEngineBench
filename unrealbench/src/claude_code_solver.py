#!/usr/bin/env python3
"""
Claude Code solver for gamedev benchmark tasks.
"""

import asyncio
import json
import os
import time
from typing import Optional

from unrealbench.src.claude_code_sdk_compat import install as install_claude_code_sdk_compat
from claude_code_sdk import query, ClaudeCodeOptions
from unrealbench.src.base_solver import BaseSolver
from unrealbench.src.utils.claude_auth import claude_sdk_auth_environment
from unrealbench.src.utils.data_types import SolverResult, TokenUsage
from unrealbench.src.utils.prompts import create_system_prompt

install_claude_code_sdk_compat()


class ClaudeCodeSolver(BaseSolver):
    """Solver that uses Claude Code to complete game development tasks."""

    # Solver capabilities (required by BaseSolver)
    SUPPORTS_MCP = True
    SUPPORTS_SYSTEM_PROMPT = True

    def __init__(
        self,
        timeout_seconds: int = 3600,
        debug: bool = False,
        use_mcp: bool = False,
        use_runtime_video: bool = False,
        model: Optional[str] = None,
        mcp_servers: Optional[dict] = None,
        reasoning_level: str = "default",
    ):
        """Initialize the Claude Code solver.

        Args:
            mcp_servers: Optional dict of MCP server configs. When provided, overrides the
                         default Godot MCP server. Format matches ClaudeCodeOptions.mcp_servers.
        """
        super().__init__(
            timeout_seconds,
            debug,
            use_mcp,
            use_runtime_video,
            reasoning_level=reasoning_level,
        )
        self.model = model
        self.mcp_servers = mcp_servers

    @staticmethod
    def is_rate_limit_error(error_message: str) -> bool:
        """Check if the error message indicates API rate limit or quota exceeded."""
        error_lower = error_message.lower()
        rate_limit_keywords = [
            "overloaded",
            "rate limit",
            "rate_limit",
            "ratelimit",
            "quota exceeded",
            "quota_exceeded",
            "429",
            "too many requests",
            "capacity",
            "usage limit",
        ]
        return any(keyword in error_lower for keyword in rate_limit_keywords)

    async def solve_task_async(self) -> SolverResult:
        """Solve the task in the current directory using Claude Code SDK."""
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
            print("SENDING PROMPT TO CLAUDE CODE:")
            print("=" * 60)
            print(prompt)
            print("=" * 60)

        try:
            if self.debug:
                print("\nCLAUDE CODE TRAJECTORY:")
                print("=" * 60)

            options_kwargs = dict(
                system_prompt=create_system_prompt(self.use_mcp),
                permission_mode="bypassPermissions",
                cwd=os.getcwd(),
            )
            reasoning_applied = "default"

            if self.model:
                options_kwargs["model"] = self.model

            if self.reasoning_level != "default":
                options_kwargs["extra_args"] = {"effort": self.reasoning_level}
                reasoning_applied = self.reasoning_level

            if self.use_mcp:
                if self.mcp_servers is not None:
                    options_kwargs["mcp_servers"] = self.mcp_servers
                else:
                    # Default: Godot screenshot MCP server
                    options_kwargs["mcp_servers"] = {
                        "godot-screenshot": {
                            "type": "stdio",
                            "command": "uv",
                            "args": ["run", "unrealbench-mcp"],
                        }
                    }
                    options_kwargs["allowed_tools"] = [
                        "mcp__godot-screenshot__godot-screenshot"
                    ]

            options = ClaudeCodeOptions(**options_kwargs)

            full_response = []
            token_usage = TokenUsage()
            total_cost = 0.0
            agent_steps = 0
            agent_steps_source = None
            model_used = self.model or "claude-sonnet-4-20250514"  # Default model

            with claude_sdk_auth_environment():
                async for message in query(prompt=prompt, options=options):
                    if self.debug:
                        # Add newline after each message for better readability
                        message_str = str(message)
                        # Add separator line and newlines for all message types
                        if message_str.startswith('SystemMessage'):
                            print(f"\n{'='*80}\n{message_str}\n{'='*80}\n", flush=True)
                        elif message_str.startswith('ResultMessage'):
                            print(f"\n{'-'*80}\n{message_str}\n{'-'*80}\n", flush=True)
                        elif message_str.startswith(('AssistantMessage', 'UserMessage')):
                            print(f"{message_str}\n", flush=True)
                        else:
                            # For any other message types, just print with newline
                            print(message_str, flush=True)
                    full_response.append(str(message))

                    message_steps = self._count_message_steps(message)
                    if message_steps:
                        agent_steps += message_steps
                        agent_steps_source = "claude_sdk_tool_use"

                    if hasattr(message, 'num_turns') and message.num_turns:
                        agent_steps = message.num_turns
                        agent_steps_source = "claude_sdk_num_turns"

                    # Check for ResultMessage which contains usage info
                    if hasattr(message, 'usage') and message.usage:
                        usage = message.usage
                        # Accumulate token usage across all messages
                        token_usage.input_tokens += usage.get('input_tokens', 0)
                        token_usage.output_tokens += usage.get('output_tokens', 0)
                        token_usage.total_tokens = token_usage.input_tokens + token_usage.output_tokens
                        token_usage.cache_read_tokens += usage.get('cache_read_input_tokens', 0)
                        token_usage.cache_write_tokens += usage.get('cache_creation_input_tokens', 0)

                    if hasattr(message, 'total_cost_usd') and message.total_cost_usd:
                        # Accumulate cost across all messages
                        total_cost += message.total_cost_usd

                    if hasattr(message, 'model') and message.model:
                        model_used = message.model

            duration = time.time() - start_time
            response_text = "".join(full_response)

            if self.debug:
                print(f"\n\nDuration: {duration:.2f} seconds")
                if token_usage.total_tokens > 0:
                    print(f"Tokens: input={token_usage.input_tokens}, output={token_usage.output_tokens}, total={token_usage.total_tokens}")
                    print(f"Cost: ${total_cost:.4f}")
                print("=" * 60)

            result = SolverResult(
                success=True,
                message="Task completed",
                duration_seconds=duration,
                stdout=response_text,
                stderr="",
                token_usage=token_usage if token_usage.total_tokens > 0 else None,
                model=model_used,
                cost_usd=total_cost,
                agent_steps=agent_steps if agent_steps > 0 else None,
                agent_steps_source=agent_steps_source,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied=reasoning_applied,
            )
            return result

        except Exception as e:
            duration = time.time() - start_time
            error_msg = str(e)
            is_rate_limited = self.is_rate_limit_error(error_msg)

            if self.debug:
                print(f"\nERROR INVOKING CLAUDE CODE: {error_msg}")
                if is_rate_limited:
                    print("⚠️  DETECTED RATE LIMIT/QUOTA ERROR")
                print("=" * 60)

            return SolverResult(
                success=False,
                message=f"Error invoking Claude Code: {error_msg}",
                duration_seconds=duration,
                is_rate_limited=is_rate_limited,
                reasoning_level_requested=self.reasoning_level,
                reasoning_level_applied=(
                    self.reasoning_level
                    if self.reasoning_level != "default"
                    else "default"
                ),
            )


    @staticmethod
    def _count_message_steps(message) -> int:
        content = getattr(message, "content", None)
        if not content:
            return 0
        if not isinstance(content, (list, tuple)):
            content = [content]
        count = 0
        for item in content:
            item_type = getattr(item, "type", None)
            if item_type is None and isinstance(item, dict):
                item_type = item.get("type")
            if item_type == "tool_use":
                count += 1
        return count

    def solve_task(self) -> SolverResult:
        """Synchronous wrapper for async solve_task_async."""
        return asyncio.run(self.solve_task_async())


def main():
    """Main function for testing the solver."""
    solver = ClaudeCodeSolver()
    result = solver.solve_task()
    print(result)


if __name__ == "__main__":
    main()
