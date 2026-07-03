#!/usr/bin/env python3

from dataclasses import dataclass, field
from datetime import datetime
import json
import os
from pathlib import Path
from typing import Dict, Optional


# Token pricing per 1M tokens (USD).
# When only an aggregate "tokens used" count is available, "blended" is used
# as an explicit estimate and result records should set cost_usd_is_estimate.
TOKEN_PRICING = {
    # Anthropic Claude
    "claude-sonnet-4-20250514": {"input": 3.00, "output": 15.00},
    "claude-3-5-sonnet": {"input": 3.00, "output": 15.00},
    "claude-3-opus": {"input": 15.00, "output": 75.00},
    "claude-3-haiku": {"input": 0.25, "output": 1.25},
    # OpenAI GPT
    "gpt-4o": {"input": 2.50, "output": 10.00},
    "gpt-4o-mini": {"input": 0.15, "output": 0.60},
    "gpt-4": {"input": 30.00, "output": 60.00},
    "gpt-4-turbo": {"input": 10.00, "output": 30.00},
    "o1": {"input": 15.00, "output": 60.00},
    "o1-mini": {"input": 3.00, "output": 12.00},
    # OpenAI Codex/GPT benchmark estimates. Keep exact billing analysis on
    # provider-side usage logs when available.
    "gpt-5.5": {"input": 2.50, "output": 10.00, "blended": 6.25},
    "codex": {"input": 2.50, "output": 10.00, "blended": 6.25},
    # ModelStudio / token-plan models. Pricing is list-price estimate in USD
    # per 1M tokens; provider billing exports remain the source of truth.
    "qwen3.7-plus": {"input": 0.30, "output": 1.20, "cache_read": 0.06},
    "qwen3.6-plus": {"input": 0.30, "output": 1.20, "cache_read": 0.06},
    "qwen3.7-max": {"input": 1.20, "output": 4.80, "cache_read": 0.24},
    "deepseek-v4-pro": {"input": 1.74, "output": 3.48, "cache_read": 0.348},
    # Kimi API Platform list pricing, USD per 1M tokens.
    "kimi-code/kimi-for-coding": {"input": 0.95, "output": 4.00, "cache_read": 0.19},
    "kimi-for-coding": {"input": 0.95, "output": 4.00, "cache_read": 0.19},
    "kimi-k2.7-code": {"input": 0.95, "output": 4.00, "cache_read": 0.19},
    "kimi-k2.7": {"input": 0.95, "output": 4.00, "cache_read": 0.19},
    "kimi-2.7": {"input": 0.95, "output": 4.00, "cache_read": 0.19},
    # GLM-5.2 public API list-price estimate, USD per 1M tokens.
    "glm-5.2": {"input": 1.40, "output": 4.40, "cache_read": 0.28},
    # Google Gemini API-equivalent pricing estimates. Antigravity CLI may run
    # through a product plan rather than direct API billing, but these entries
    # keep benchmark cost estimates comparable when token usage is estimated.
    "gemini-3.5-flash": {"input": 1.50, "output": 9.00, "cache_read": 0.15},
    "gemini-3-flash-a": {"input": 1.50, "output": 9.00, "cache_read": 0.15},
    # Google Gemini CLI/free fallback when no specific Gemini API model matches.
    "gemini": {"input": 0.00, "output": 0.00},
    # Default fallback
    "default": {"input": 3.00, "output": 15.00},
}


def _normalize_pricing(raw: dict) -> dict:
    pricing = {}
    for key, value in (raw or {}).items():
        if not isinstance(value, dict):
            continue
        entry = {}
        aliases = {
            "input": "input",
            "output": "output",
            "cache_read": "cache_read",
            "cache_hit": "cache_read",
            "blended": "blended",
            "inputPerMillionTokens": "input",
            "outputPerMillionTokens": "output",
            "cacheReadPerMillionTokens": "cache_read",
            "cacheHitPerMillionTokens": "cache_read",
        }
        for src, dest in aliases.items():
            if src not in value:
                continue
            try:
                entry[dest] = float(value[src])
            except (TypeError, ValueError):
                continue
        if "input" in entry and "output" in entry:
            pricing[str(key).lower()] = entry
    return pricing


def _load_pricing_overrides() -> dict:
    merged = {}
    override_file = os.environ.get("UNREALBENCH_MODEL_PRICING_FILE", "").strip()
    if override_file:
        try:
            merged.update(_normalize_pricing(json.loads(Path(override_file).read_text(encoding="utf-8"))))
        except (OSError, json.JSONDecodeError):
            pass
    override_json = os.environ.get("UNREALBENCH_MODEL_PRICING_JSON", "").strip()
    if override_json:
        try:
            merged.update(_normalize_pricing(json.loads(override_json)))
        except json.JSONDecodeError:
            pass
    return merged


def effective_token_pricing() -> Dict[str, Dict]:
    pricing = dict(TOKEN_PRICING)
    pricing.update(_load_pricing_overrides())
    return pricing


def pricing_for_model(model: str) -> tuple[str, Dict] | tuple[None, None]:
    """Return explicit pricing match for a model, excluding default fallback."""
    model_lower = (model or "").lower()
    pricing_table = effective_token_pricing()
    for key in sorted(pricing_table, key=len, reverse=True):
        pricing = pricing_table[key]
        if key == "default":
            continue
        if key in model_lower:
            return key, pricing
    return None, None


@dataclass
class TokenUsage:
    """Container for token usage statistics."""

    input_tokens: int = 0
    output_tokens: int = 0
    total_tokens: int = 0
    cache_read_tokens: int = 0
    cache_write_tokens: int = 0

    def calculate_cost(self, model: str) -> float:
        """Calculate cost in USD based on model pricing."""
        # Normalize model name
        model_lower = model.lower()
        pricing_table = effective_token_pricing()
        pricing = pricing_table.get("default")

        for key in sorted(pricing_table, key=len, reverse=True):
            if key in model_lower:
                pricing = pricing_table[key]
                break

        if self.input_tokens == 0 and self.output_tokens == 0 and self.total_tokens > 0:
            blended = pricing.get("blended", (pricing["input"] + pricing["output"]) / 2)
            return (self.total_tokens / 1_000_000) * blended

        cache_read_rate = pricing.get("cache_read", pricing.get("cache_hit"))
        if cache_read_rate is not None and self.cache_read_tokens:
            cached_input_tokens = min(self.cache_read_tokens, self.input_tokens)
            uncached_input_tokens = max(self.input_tokens - cached_input_tokens, 0)
            input_cost = (
                (uncached_input_tokens / 1_000_000) * pricing["input"]
                + (cached_input_tokens / 1_000_000) * cache_read_rate
            )
        else:
            input_cost = (self.input_tokens / 1_000_000) * pricing["input"]
        output_cost = (self.output_tokens / 1_000_000) * pricing["output"]

        return input_cost + output_cost

    def to_dict(self) -> Dict:
        return {
            "input_tokens": self.input_tokens,
            "output_tokens": self.output_tokens,
            "total_tokens": self.total_tokens,
            "cache_read_tokens": self.cache_read_tokens,
            "cache_write_tokens": self.cache_write_tokens,
        }


@dataclass
class ValidationResult:
    """Container for validation test results."""

    success: bool
    message: str
    details: Optional[Dict] = field(default_factory=dict)
    timestamp: str = field(default_factory=lambda: datetime.now().isoformat())

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        return {
            "success": self.success,
            "message": self.message,
            "details": self.details,
            "timestamp": self.timestamp,
        }

    def __str__(self) -> str:
        status = "PASSED" if self.success else "FAILED"
        return f"{status}: {self.message}"


@dataclass
class SolverResult:
    """Container for solver results with token usage tracking."""

    success: bool
    message: str
    duration_seconds: float
    stdout: str = ""
    stderr: str = ""
    timestamp: str = field(default_factory=lambda: datetime.now().isoformat())
    is_rate_limited: bool = False  # Flag for API quota/rate limit errors
    token_usage: Optional[TokenUsage] = None  # Token usage statistics
    token_usage_is_estimate: bool = False
    model: str = ""  # Model used for this run
    cost_usd: float = 0.0  # Calculated cost in USD
    cost_usd_is_estimate: bool = False
    cost_estimate_note: Optional[str] = None
    agent_steps: Optional[int] = None
    agent_steps_source: Optional[str] = None
    reasoning_level_requested: str = "default"
    reasoning_level_applied: str = "default"

    def calculate_cost(self) -> float:
        """Calculate and store the cost based on token usage."""
        if self.token_usage and self.model:
            self.cost_usd = self.token_usage.calculate_cost(self.model)
        return self.cost_usd

    def to_dict(self) -> Dict:
        """Convert to dictionary for JSON serialization."""
        result = {
            "success": self.success,
            "message": self.message,
            "duration_seconds": self.duration_seconds,
            "stdout": self.stdout,
            "stderr": self.stderr,
            "timestamp": self.timestamp,
            "is_rate_limited": self.is_rate_limited,
            "model": self.model,
            "cost_usd": self.cost_usd,
            "cost_usd_is_estimate": self.cost_usd_is_estimate,
            "token_usage_is_estimate": self.token_usage_is_estimate,
            "agent_steps": self.agent_steps,
            "reasoning_level_requested": self.reasoning_level_requested,
            "reasoning_level_applied": self.reasoning_level_applied,
        }
        if self.cost_estimate_note:
            result["cost_estimate_note"] = self.cost_estimate_note
        if self.agent_steps_source:
            result["agent_steps_source"] = self.agent_steps_source
        if self.token_usage:
            result["token_usage"] = self.token_usage.to_dict()
        return result

    def __str__(self) -> str:
        status = "COMPLETED" if self.success else "FAILED"
        token_info = ""
        if self.token_usage:
            token_info = f", tokens: {self.token_usage.total_tokens}, cost: ${self.cost_usd:.4f}"
        return f"{status}: {self.message} (took {self.duration_seconds:.2f}s{token_info})"
