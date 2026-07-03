"""
Runtime behavioral validator for time_simulation tasks.

Uses a state machine to manage PIE lifecycle safely:

  NOT_STARTED → STARTING → RUNNING → SAMPLING → STOPPING → STOPPED
                  ↓ timeout       ↓ error              ↓ cleanup error
                 FAILED ←──────────────────────────────────┘

Key properties:
- PIE start is CONFIRMED by executing "stat unit" and getting a response,
  not just by wall-clock wait. This avoids false-ready states.
- Checkpoint times are relative to confirmed PIE start, not wall clock.
- 3-retry policy for transient bridge failures with 0.5s delay between retries.
- PIE stop is always attempted in cleanup even if sampling fails.
- All bridge/PIE failures populate environment_health in the report.
"""

from __future__ import annotations

import time
from enum import Enum, auto
from pathlib import Path
from typing import Any, Optional

from .base import (
    BaseValidator,
    CheckResult,
    EnvironmentHealth,
    Tolerance,
    ValidationReport,
    UnrealRuntimeAdapter,
    eval_op,
    make_check,
    walk_property_path,
)


class PIEState(Enum):
    NOT_STARTED = auto()
    STARTING = auto()
    RUNNING = auto()
    SAMPLING = auto()
    STOPPING = auto()
    STOPPED = auto()
    FAILED = auto()


_RETRY_COUNT = 3
_RETRY_DELAY = 0.5   # seconds between retries
_PIE_CONFIRM_CMD = "stat unit"
_PIE_STOP_CMD = "ke * EndPlayMap"
_DEFAULT_PIE_WAIT = 3       # seconds to wait after PIE confirmed before first sample
_DEFAULT_START_TIMEOUT = 30  # seconds to wait for PIE to become responsive


class RuntimeBehavioralValidator(BaseValidator):
    """Validates time_simulation tasks by sampling actor state after PIE starts.

    Config keys (from validation.private.validators[i]):
      pie_wait_seconds:  int   = 3       seconds to wait after PIE confirmation
      pie_start_timeout: int   = 30      seconds to wait for PIE to become responsive
      checkpoints: list[{
        time_s: float                    seconds after PIE start (relative)
        checks: list[{
          actor:     str                 actor name in the level
          property:  str                 dot-separated property path
          op:        str                 gt | lt | gte | lte | eq | neq | between
          value:     Any                 expected value
          tolerance: {type, value}       optional tolerance dict
          severity:  str = "required"    required | advisory | diagnostic
        }]
      }]
    """

    validator_type = "runtime_behavioral"

    def __init__(self, validator_config: dict):
        super().__init__(validator_config)
        self.pie_wait = int(self.config.get("pie_wait_seconds", _DEFAULT_PIE_WAIT))
        self.pie_start_timeout = int(
            self.config.get("pie_start_timeout", _DEFAULT_START_TIMEOUT)
        )
        self.checkpoints = self.config.get("checkpoints", [])

    def validate(
        self,
        workspace: Path,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ) -> ValidationReport:
        if adapter is None:
            return self.as_sanity_only(workspace)

        env_health = EnvironmentHealth()
        checks: list[CheckResult] = []
        state = PIEState.NOT_STARTED
        pie_start_time: Optional[float] = None

        try:
            # Confirm bridge is reachable
            probe = _retry(lambda: adapter.execute_console_command("stat unit"), _RETRY_COUNT)
            if probe is None:
                env_health.bridge_connect_ok = False
                env_health.failure_reason = "bridge unreachable before PIE start"
                return ValidationReport.error_report(
                    self.validator_type,
                    "Bridge unreachable — could not confirm editor connection",
                    failure_type="bridge_unreachable",
                    env_health=env_health,
                )
            env_health.bridge_connect_ok = True

            # Start PIE
            state = PIEState.STARTING
            _retry(lambda: adapter.execute_console_command("pie"), _RETRY_COUNT)

            # Confirm PIE started (stat unit returns without error)
            confirmed = self._wait_for_pie(adapter, env_health)
            if not confirmed:
                state = PIEState.FAILED
                return ValidationReport.error_report(
                    self.validator_type,
                    f"PIE did not start within {self.pie_start_timeout}s",
                    failure_type="pie_start_timeout",
                    env_health=env_health,
                )

            state = PIEState.RUNNING
            env_health.pie_start_ok = True
            pie_start_time = time.monotonic()

            # Initial wait after PIE confirmation
            time.sleep(self.pie_wait)

            # Sample checkpoints
            state = PIEState.SAMPLING
            for cp in self.checkpoints:
                target_t = float(cp.get("time_s", 0.0))
                elapsed = time.monotonic() - pie_start_time
                remaining = target_t - elapsed
                if remaining > 0:
                    time.sleep(remaining)

                for chk in cp.get("checks", []):
                    result_check = self._run_check(adapter, chk, target_t)
                    checks.append(result_check)

        except Exception as exc:
            checks.append(CheckResult(
                name="runtime_exception",
                passed=False,
                actual=str(exc),
                severity="required",
                category="runtime_behavior",
                message=str(exc),
            ))
        finally:
            # Always stop PIE
            state = PIEState.STOPPING
            try:
                _retry(lambda: adapter.execute_console_command(_PIE_STOP_CMD), _RETRY_COUNT)
                state = PIEState.STOPPED
            except Exception:
                pass  # PIE stop failure is not a validation failure

        required = [c for c in checks if c.severity == "required"]
        passed = all(c.passed for c in required) if required else False
        n_pass = sum(c.passed for c in checks)
        summary = f"{n_pass}/{len(checks)} checks passed"

        return ValidationReport(
            passed=passed,
            validator_type=self.validator_type,
            checks=checks,
            summary=summary,
            phase="behavioral",
            failure_type="" if passed else "wrong_runtime_behavior",
            environment_health=env_health,
        )

    def _wait_for_pie(
        self, adapter: UnrealRuntimeAdapter, env_health: EnvironmentHealth
    ) -> bool:
        """Poll stat unit until PIE is responsive or timeout."""
        deadline = time.monotonic() + self.pie_start_timeout
        while time.monotonic() < deadline:
            result = _retry(
                lambda: adapter.execute_console_command(_PIE_CONFIRM_CMD), 1
            )
            if result is not None:
                return True
            time.sleep(1.0)
        env_health.pie_start_ok = False
        env_health.failure_reason = f"PIE not responsive after {self.pie_start_timeout}s"
        return False

    def _run_check(
        self,
        adapter: UnrealRuntimeAdapter,
        chk: dict,
        time_s: float,
    ) -> CheckResult:
        actor_name = chk.get("actor", "")
        prop_path = chk.get("property", "")
        op = chk.get("op", "eq")
        expected = chk.get("value")
        severity = chk.get("severity", "required")

        tol_cfg = chk.get("tolerance")
        tolerance = Tolerance.from_dict(tol_cfg) if tol_cfg else None

        check_name = f"{actor_name}.{prop_path}@{time_s}s"

        props = _retry(lambda: adapter.get_actor_properties(actor_name), _RETRY_COUNT)
        if props is None:
            return CheckResult(
                name=check_name,
                passed=False,
                expected=f"{op} {expected}",
                actual=None,
                severity=severity,
                category="runtime_behavior",
                message=f"Could not get properties for '{actor_name}'",
            )

        actual = walk_property_path(props, prop_path)
        return make_check(
            name=check_name,
            actual=actual,
            op=op,
            expected=expected,
            category="runtime_behavior",
            severity=severity,
            tolerance=tolerance,
        )


def _retry(fn: Any, max_attempts: int) -> Any:
    """Call fn() up to max_attempts times, returning on first non-None result."""
    for attempt in range(max_attempts):
        try:
            result = fn()
            if result is not None:
                return result
        except Exception:
            pass
        if attempt < max_attempts - 1:
            time.sleep(_RETRY_DELAY)
    return None
