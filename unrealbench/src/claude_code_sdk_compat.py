"""Compatibility helpers for claude-code-sdk stream parsing."""

from __future__ import annotations

from typing import Any


def install() -> None:
    """Patch older claude-code-sdk versions to tolerate newer event types.

    Some SDK builds raise MessageParseError on top-level stream items such as
    ``rate_limit_event``. Those are informational and should not abort the
    caller's query loop, so coerce them into a SystemMessage instead.
    """

    try:
        import claude_code_sdk._internal.client as client_mod
        import claude_code_sdk._internal.message_parser as parser_mod
        from claude_code_sdk._errors import MessageParseError
        from claude_code_sdk.types import SystemMessage
    except Exception:
        return

    if getattr(parser_mod.parse_message, "_unrealbench_compat_installed", False):
        return

    original_parse_message = parser_mod.parse_message

    def tolerant_parse_message(data: dict[str, Any]):
        try:
            return original_parse_message(data)
        except MessageParseError:
            if isinstance(data, dict) and data.get("type") == "rate_limit_event":
                return SystemMessage(subtype="rate_limit_event", data=data)
            raise

    tolerant_parse_message._unrealbench_compat_installed = True  # type: ignore[attr-defined]

    parser_mod.parse_message = tolerant_parse_message
    client_mod.parse_message = tolerant_parse_message
