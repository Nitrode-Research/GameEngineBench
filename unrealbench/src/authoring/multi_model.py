"""
Thin multi-model client wrapper for structured LLM calls.

Wraps Anthropic, OpenAI, and Google Generative AI APIs with a uniform interface.
All calls use structured JSON output enforced at the API level where supported,
with strict re-parsing as a fallback.

Usage:
    from unrealbench.src.authoring.multi_model import build_available_clients, ModelResponse

    clients = build_available_clients()
    responses = [c.complete(system, user, schema) for c in clients]

API key detection (env vars):
    ANTHROPIC_API_KEY  → AnthropicClient (claude-opus-4-6)
    OPENAI_API_KEY     → OpenAIClient (gpt-4o)
    GOOGLE_API_KEY     → GeminiClient (gemini-1.5-pro)

Missing key → client skipped with a warning; never raises.
"""

from __future__ import annotations

import json
import logging
import time
from dataclasses import dataclass, field
from typing import Any, Optional, Protocol

logger = logging.getLogger(__name__)

_MAX_RETRIES = 3
_BACKOFF_BASE = 1.0   # seconds; doubles each retry


# ── Response container ────────────────────────────────────────────────────────

@dataclass
class ModelResponse:
    provider: str           # "claude" | "openai" | "gemini"
    model_id: str           # exact model version string returned by API
    parsed: Optional[dict]  # structured output parsed from JSON; None on failure
    parse_ok: bool
    raw_text: str           # full raw text response (for local cache, not committed)
    retries_used: int = 0
    error: str = ""         # non-empty on failure


# ── Protocol ──────────────────────────────────────────────────────────────────

class ModelClient(Protocol):
    provider: str

    def complete(
        self,
        system: str,
        user: str,
        response_schema: dict,
    ) -> ModelResponse: ...


# ── Retry helper ──────────────────────────────────────────────────────────────

def _with_retry(fn, retries: int = _MAX_RETRIES, backoff: float = _BACKOFF_BASE):
    """Call fn() up to retries times with exponential backoff. Returns (result, retries_used)."""
    last_exc: Optional[Exception] = None
    for attempt in range(retries):
        try:
            return fn(), attempt
        except Exception as exc:
            last_exc = exc
            wait = backoff * (2 ** attempt)
            logger.warning(f"  Attempt {attempt + 1} failed ({exc}); retrying in {wait:.1f}s")
            time.sleep(wait)
    raise last_exc  # type: ignore[misc]


def _parse_json_strict(text: str) -> Optional[dict]:
    """Extract and parse the first JSON object from text. Returns None on failure."""
    text = text.strip()
    # Strip markdown code fences if present
    if text.startswith("```"):
        lines = text.splitlines()
        text = "\n".join(lines[1:-1] if lines[-1].strip() == "```" else lines[1:])
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        # Try to extract first {...} block
        start = text.find("{")
        end = text.rfind("}")
        if start != -1 and end != -1 and end > start:
            try:
                return json.loads(text[start:end + 1])
            except json.JSONDecodeError:
                pass
    return None


# ── Anthropic client ──────────────────────────────────────────────────────────

class AnthropicClient:
    provider = "claude"
    _DEFAULT_MODEL = "claude-opus-4-6"

    def __init__(self, api_key: str, model: str = _DEFAULT_MODEL):
        import anthropic  # type: ignore
        self._client = anthropic.Anthropic(api_key=api_key)
        self._model = model

    def complete(self, system: str, user: str, response_schema: dict) -> ModelResponse:
        schema_instruction = (
            "\n\nYou MUST respond with ONLY a valid JSON object matching this schema. "
            "No explanation, no markdown fences — raw JSON only.\n"
            f"Schema: {json.dumps(response_schema, indent=2)}"
        )

        def _call():
            msg = self._client.messages.create(
                model=self._model,
                max_tokens=4096,
                system=system + schema_instruction,
                messages=[{"role": "user", "content": user}],
            )
            return msg

        try:
            response, retries = _with_retry(_call)
            raw = response.content[0].text
            model_id = response.model
            parsed = _parse_json_strict(raw)
            return ModelResponse(
                provider=self.provider,
                model_id=model_id,
                parsed=parsed,
                parse_ok=parsed is not None,
                raw_text=raw,
                retries_used=retries,
                error="" if parsed is not None else "JSON parse failed",
            )
        except Exception as exc:
            return ModelResponse(
                provider=self.provider,
                model_id=self._model,
                parsed=None,
                parse_ok=False,
                raw_text="",
                error=str(exc),
            )


# ── OpenAI client ─────────────────────────────────────────────────────────────

class OpenAIClient:
    provider = "openai"
    _DEFAULT_MODEL = "gpt-4o"

    def __init__(self, api_key: str, model: str = _DEFAULT_MODEL):
        import openai  # type: ignore
        self._client = openai.OpenAI(api_key=api_key)
        self._model = model

    def complete(self, system: str, user: str, response_schema: dict) -> ModelResponse:
        schema_instruction = (
            "\n\nRespond with ONLY a valid JSON object matching this schema. "
            "No explanation, no markdown fences.\n"
            f"Schema: {json.dumps(response_schema, indent=2)}"
        )

        def _call():
            return self._client.chat.completions.create(
                model=self._model,
                response_format={"type": "json_object"},
                messages=[
                    {"role": "system", "content": system + schema_instruction},
                    {"role": "user", "content": user},
                ],
            )

        try:
            response, retries = _with_retry(_call)
            raw = response.choices[0].message.content or ""
            model_id = response.model
            parsed = _parse_json_strict(raw)
            return ModelResponse(
                provider=self.provider,
                model_id=model_id,
                parsed=parsed,
                parse_ok=parsed is not None,
                raw_text=raw,
                retries_used=retries,
                error="" if parsed is not None else "JSON parse failed",
            )
        except Exception as exc:
            return ModelResponse(
                provider=self.provider,
                model_id=self._model,
                parsed=None,
                parse_ok=False,
                raw_text="",
                error=str(exc),
            )


# ── Gemini client ─────────────────────────────────────────────────────────────

class GeminiClient:
    provider = "gemini"
    _DEFAULT_MODEL = "gemini-3-flash-preview"

    def __init__(self, api_key: str, model: str = _DEFAULT_MODEL):
        from google import genai  # type: ignore
        self._client = genai.Client(api_key=api_key)
        self._model_name = model

    def complete(self, system: str, user: str, response_schema: dict) -> ModelResponse:
        from google import genai  # type: ignore
        schema_instruction = (
            "\n\nRespond with ONLY a valid JSON object matching this schema. "
            "No explanation, no markdown fences.\n"
            f"Schema: {json.dumps(response_schema, indent=2)}"
        )
        full_prompt = f"{system}{schema_instruction}\n\n{user}"

        def _call():
            return self._client.models.generate_content(
                model=self._model_name,
                contents=full_prompt,
                config=genai.types.GenerateContentConfig(
                    response_mime_type="application/json",
                ),
            )

        try:
            response, retries = _with_retry(_call)
            raw = response.text
            parsed = _parse_json_strict(raw)
            return ModelResponse(
                provider=self.provider,
                model_id=self._model_name,
                parsed=parsed,
                parse_ok=parsed is not None,
                raw_text=raw,
                retries_used=retries,
                error="" if parsed is not None else "JSON parse failed",
            )
        except Exception as exc:
            return ModelResponse(
                provider=self.provider,
                model_id=self._model_name,
                parsed=None,
                parse_ok=False,
                raw_text="",
                error=str(exc),
            )


# ── Factory ───────────────────────────────────────────────────────────────────

def build_available_clients() -> list:
    """Build one client per provider for each configured API key.

    Loads from .env automatically. Missing keys are skipped with a warning.
    Returns empty list if none configured.

    Supported env vars:
        ANTHROPIC_API_KEY  → claude-opus-4-6
        OPENAI_API_KEY     → gpt-4o
        GOOGLE_API_KEY or GEMINI_API_KEY → gemini-1.5-pro
    """
    import os
    try:
        from dotenv import load_dotenv
        load_dotenv()
    except ImportError:
        pass  # dotenv optional; env vars may already be set

    clients = []

    anthropic_key = os.environ.get("ANTHROPIC_API_KEY")
    if anthropic_key:
        try:
            clients.append(AnthropicClient(anthropic_key))
        except ImportError:
            logger.warning("anthropic SDK not installed; claude skipped (pip install anthropic)")
    else:
        logger.warning("ANTHROPIC_API_KEY not set; claude skipped")

    openai_key = os.environ.get("OPENAI_API_KEY")
    if openai_key:
        try:
            clients.append(OpenAIClient(openai_key))
        except ImportError:
            logger.warning("openai SDK not installed; openai skipped (pip install openai)")
    else:
        logger.warning("OPENAI_API_KEY not set; openai skipped")

    # Support both GOOGLE_API_KEY and GEMINI_API_KEY (.env uses GEMINI_API_KEY)
    google_key = os.environ.get("GOOGLE_API_KEY") or os.environ.get("GEMINI_API_KEY")
    if google_key:
        try:
            clients.append(GeminiClient(google_key))
        except ImportError:
            logger.warning(
                "google-genai SDK not installed; gemini skipped "
                "(pip install google-genai)"
            )
    else:
        logger.warning("GOOGLE_API_KEY / GEMINI_API_KEY not set; gemini skipped")

    return clients


def run_parallel(
    clients: list,
    system: str,
    user: str,
    response_schema: dict,
) -> list[ModelResponse]:
    """Run all clients with the same prompt. Returns list of ModelResponse in client order."""
    # Parallel via threads since each call is I/O-bound
    import concurrent.futures
    with concurrent.futures.ThreadPoolExecutor(max_workers=len(clients)) as pool:
        futures = {pool.submit(c.complete, system, user, response_schema): c for c in clients}
        results = []
        for future in concurrent.futures.as_completed(futures):
            try:
                results.append(future.result())
            except Exception as exc:
                client = futures[future]
                results.append(ModelResponse(
                    provider=client.provider,
                    model_id="unknown",
                    parsed=None,
                    parse_ok=False,
                    raw_text="",
                    error=str(exc),
                ))
    # Restore client order
    provider_order = [c.provider for c in clients]
    results.sort(key=lambda r: provider_order.index(r.provider) if r.provider in provider_order else 99)
    return results
