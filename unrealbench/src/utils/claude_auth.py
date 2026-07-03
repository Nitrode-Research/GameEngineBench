#!/usr/bin/env python3

from __future__ import annotations

import os
from contextlib import contextmanager

@contextmanager
def claude_sdk_auth_environment():
    """Force Claude SDK calls to prefer subscription auth."""
    removed_env: dict[str, str] = {}
    for key in ("ANTHROPIC_API_KEY", "ANTHROPIC_AUTH_TOKEN"):
        if key in os.environ:
            removed_env[key] = os.environ.pop(key)

    try:
        yield
    finally:
        os.environ.update(removed_env)
