"""
Base classes and interfaces for the benchmark validator framework.

Key types:
  CheckResult        — a single assertion result with severity and category
  EnvironmentHealth  — bridge/PIE health; populated on ALL runs, not just calibration
  ValidationReport   — full result from one validator run
  BaseValidator      — abstract base all validators inherit from
  UnrealRuntimeAdapter — Protocol decoupling validators from the TCP bridge
  UnrealConnectionAdapter — concrete impl backed by UnrealConnection socket
  Tolerance          — standardized numeric tolerance with apply()

Failure taxonomy constants are defined here so all validators use the same values.
"""

from __future__ import annotations

import time
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Optional

try:
    from typing import Protocol, runtime_checkable
except ImportError:
    from typing_extensions import Protocol, runtime_checkable  # type: ignore


# ── Failure taxonomy ──────────────────────────────────────────────────────────

FAILURE_TAXONOMY = frozenset({
    "compile_failure",
    "missing_public_contract",
    "wrong_runtime_behavior",
    "wrong_editor_config",
    "wrong_scene_state",
    "wrong_asset_reference",
    "event_cadence_failure",
    "image_output_failure",
    "validator_runtime_error",
    "environment_nondeterministic",
    "bad_solution_false_positive",
    "alt_good_false_negative",
    "pie_start_timeout",
    "bridge_unreachable",
    "skipped",
    "not_implemented",
})

# ── Tolerance standard ────────────────────────────────────────────────────────

TOLERANCE_TYPES = frozenset({"abs", "rel", "angle_deg", "transform_cm", "count", "time_s"})


@dataclass
class Tolerance:
    """Standardized numeric tolerance.

    Types:
      abs         — absolute difference (|actual - expected| <= value)
      rel         — relative fraction (|actual - expected| / |expected| <= value)
      angle_deg   — degrees (same as abs but documented intent)
      transform_cm — Unreal units / cm (same as abs)
      count       — integer count (rounds both sides)
      time_s      — seconds (same as abs)
    """
    type: str
    value: float

    def apply(self, actual: float, expected: float) -> bool:
        if self.type in ("abs", "angle_deg", "transform_cm", "time_s"):
            return abs(actual - expected) <= self.value
        if self.type == "rel":
            if expected == 0:
                return abs(actual) <= self.value
            return abs((actual - expected) / expected) <= self.value
        if self.type == "count":
            return abs(int(actual) - int(expected)) <= int(self.value)
        return False

    @classmethod
    def from_dict(cls, d: dict) -> "Tolerance":
        return cls(type=d["type"], value=float(d["value"]))


# ── Check and report types ────────────────────────────────────────────────────

@dataclass
class CheckResult:
    name: str
    passed: bool
    expected: Any = None
    actual: Any = None
    message: str = ""
    severity: str = "required"   # required | advisory | diagnostic
    category: str = ""           # runtime_behavior | static_shape | scene_graph | etc.

    def to_dict(self) -> dict:
        return {
            "name": self.name,
            "passed": self.passed,
            "expected": self.expected,
            "actual": self.actual,
            "message": self.message,
            "severity": self.severity,
            "category": self.category,
        }


@dataclass
class EnvironmentHealth:
    """Bridge and editor health recorded on every validation run.

    Populated regardless of task outcome so production results are interpretable
    when infrastructure — not task logic — caused a failure.
    """
    unreal_launch_ok: Optional[bool] = None
    bridge_connect_ok: Optional[bool] = None
    pie_start_ok: Optional[bool] = None
    failure_reason: str = ""

    def to_dict(self) -> dict:
        return {
            "unreal_launch_ok": self.unreal_launch_ok,
            "bridge_connect_ok": self.bridge_connect_ok,
            "pie_start_ok": self.pie_start_ok,
            "failure_reason": self.failure_reason,
        }

    @property
    def healthy(self) -> bool:
        return (
            self.bridge_connect_ok is not False
            and self.pie_start_ok is not False
        )


@dataclass
class ValidationReport:
    passed: bool
    validator_type: str
    checks: list[CheckResult] = field(default_factory=list)
    summary: str = ""
    raw_output: str = ""
    skipped: bool = False
    skip_reason: str = ""
    phase: str = ""              # static | behavioral
    failure_type: str = ""       # from FAILURE_TAXONOMY
    validator_runtime_ms: int = 0
    environment_health: Optional[EnvironmentHealth] = None

    def to_dict(self) -> dict:
        return {
            "passed": self.passed,
            "validator_type": self.validator_type,
            "checks": [c.to_dict() for c in self.checks],
            "summary": self.summary,
            "raw_output": self.raw_output,
            "skipped": self.skipped,
            "skip_reason": self.skip_reason,
            "phase": self.phase,
            "failure_type": self.failure_type,
            "validator_runtime_ms": self.validator_runtime_ms,
            "environment_health": self.environment_health.to_dict() if self.environment_health else None,
        }

    @classmethod
    def skipped_report(
        cls, validator_type: str, reason: str, phase: str = ""
    ) -> "ValidationReport":
        return cls(
            passed=True,
            validator_type=validator_type,
            skipped=True,
            skip_reason=reason,
            summary=f"Skipped: {reason}",
            phase=phase,
            failure_type="skipped",
        )

    @classmethod
    def error_report(
        cls,
        validator_type: str,
        error: str,
        failure_type: str = "validator_runtime_error",
        env_health: Optional[EnvironmentHealth] = None,
    ) -> "ValidationReport":
        return cls(
            passed=False,
            validator_type=validator_type,
            summary=f"Error: {error}",
            raw_output=error,
            failure_type=failure_type,
            environment_health=env_health,
        )

    @property
    def required_checks_passed(self) -> bool:
        required = [c for c in self.checks if c.severity == "required"]
        return all(c.passed for c in required) if required else self.passed


# ── Runtime adapter interface ─────────────────────────────────────────────────

@runtime_checkable
class UnrealRuntimeAdapter(Protocol):
    """Protocol for querying a running Unreal Editor (port 55557).

    All methods return None on transient error. Callers should retry
    or treat None as a bridge failure.

    Concrete implementation: UnrealConnectionAdapter (below).
    For testing without a live editor: MockUnrealAdapter (implement locally).
    """

    def get_actor_properties(self, name: str) -> Optional[dict]: ...

    def get_actors_in_level(self) -> Optional[list]: ...

    def execute_console_command(self, command: str) -> Optional[dict]: ...


class UnrealConnectionAdapter:
    """Concrete UnrealRuntimeAdapter backed by an UnrealConnection TCP socket.

    Usage:
        conn = UnrealConnection()
        adapter = UnrealConnectionAdapter(conn)
        props = adapter.get_actor_properties("DayNightCycleActor_1")
    """

    def __init__(self, conn: Any):
        # conn: UnrealConnection from unreal_mcp_server.py
        # Typed Any to avoid hard import of unreal_mcp_server at module load
        self._conn = conn

    def get_actor_properties(self, name: str) -> Optional[dict]:
        return self._conn.send_command("get_actor_properties", {"name": name})

    def get_actors_in_level(self) -> Optional[list]:
        result = self._conn.send_command("get_actors_in_level", {})
        if isinstance(result, dict):
            return result.get("actors") or result.get("result") or []
        return result

    def execute_console_command(self, command: str) -> Optional[dict]:
        return self._conn.send_command("execute_console_command", {"command": command})


# ── Base validator ────────────────────────────────────────────────────────────

class BaseValidator(ABC):
    """Abstract base for all task family validators.

    __init__ receives the validator config dict from
    validation.public.validators[i] or validation.private.validators[i].

    validate() receives the workspace path and optionally a UnrealRuntimeAdapter.
    Validators must not import UnrealConnection directly — use the adapter.
    """

    validator_type: str = "base"

    def __init__(self, validator_config: dict):
        self.config = validator_config

    @abstractmethod
    def validate(
        self,
        workspace: Path,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ) -> ValidationReport:
        """Run validation and return a structured report."""

    def as_sanity_only(self, workspace: Path) -> ValidationReport:
        """Run without a live editor (code_only mode).

        Default: returns a skipped sentinel. Static validators override this.
        """
        return ValidationReport.skipped_report(
            self.validator_type,
            "Behavioral validation requires hybrid mode (live editor adapter).",
            phase="behavioral",
        )


# ── Assertion helpers ─────────────────────────────────────────────────────────

def eval_op(
    actual: Any,
    op: str,
    expected: Any,
    tolerance: Optional[Tolerance] = None,
) -> bool:
    """Evaluate a comparison assertion with optional tolerance.

    Ops: gt, lt, gte, lte, eq, neq, between.
    between: expected = [lo, hi] or {'lo': x, 'hi': y}.
    """
    try:
        if op == "eq":
            if tolerance is not None:
                return tolerance.apply(float(actual), float(expected))
            return actual == expected
        if op == "neq":
            return actual != expected
        if op == "gt":
            tol = tolerance.value if tolerance else 0
            return float(actual) > float(expected) - tol
        if op == "lt":
            tol = tolerance.value if tolerance else 0
            return float(actual) < float(expected) + tol
        if op == "gte":
            return float(actual) >= float(expected)
        if op == "lte":
            return float(actual) <= float(expected)
        if op == "between":
            if isinstance(expected, (list, tuple)):
                lo, hi = float(expected[0]), float(expected[1])
            else:
                lo, hi = float(expected["lo"]), float(expected["hi"])
            tol = tolerance.value if tolerance else 0
            return (lo - tol) <= float(actual) <= (hi + tol)
    except (TypeError, ValueError, KeyError, IndexError):
        pass
    return False


def walk_property_path(obj: Any, path: str) -> Any:
    """Walk a dot-separated property path into a nested dict.

    Returns None if any segment is missing.
    Example: walk_property_path(data, "FogComponent.FogDensity")
    """
    current = obj
    for part in path.split("."):
        if current is None:
            return None
        if isinstance(current, dict):
            current = current.get(part)
        else:
            return None
    return current


def make_check(
    name: str,
    actual: Any,
    op: str,
    expected: Any,
    category: str = "",
    severity: str = "required",
    tolerance: Optional[Tolerance] = None,
) -> CheckResult:
    """Build a CheckResult from an assertion."""
    passed = eval_op(actual, op, expected, tolerance)
    msg = "" if passed else f"Expected {op} {expected!r}, got {actual!r}"
    return CheckResult(
        name=name,
        passed=passed,
        expected=f"{op} {expected}",
        actual=actual,
        message=msg,
        severity=severity,
        category=category,
    )


def timed_validate(
    validator: BaseValidator,
    workspace: Path,
    adapter: Optional[Any] = None,
) -> ValidationReport:
    """Run validate() and populate validator_runtime_ms."""
    t0 = time.monotonic()
    report = validator.validate(workspace, adapter)
    report.validator_runtime_ms = int((time.monotonic() - t0) * 1000)
    return report
