"""
Editor inspection validator for editor_property_config tasks.

Checks component properties via get_actor_properties (no PIE needed).

Config keys:
  actor_checks: list[{
    actor_name:    str
    property_path: str   dot-separated path into actor property dict
    op:            str   gt | lt | gte | lte | eq | neq | between
    value:         Any
    tolerance:     {type, value}   optional
    severity:      str = "required"
  }]
"""

from __future__ import annotations

from pathlib import Path
from typing import Optional

from .base import (
    BaseValidator,
    CheckResult,
    EnvironmentHealth,
    Tolerance,
    ValidationReport,
    UnrealRuntimeAdapter,
    make_check,
    walk_property_path,
)
from .runtime_behavioral import _retry, _RETRY_COUNT


class EditorInspectionValidator(BaseValidator):
    validator_type = "editor_inspection"

    def validate(
        self,
        workspace: Path,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ) -> ValidationReport:
        if adapter is None:
            return self.as_sanity_only(workspace)

        env_health = EnvironmentHealth()
        checks: list[CheckResult] = []

        probe = _retry(lambda: adapter.execute_console_command("stat unit"), _RETRY_COUNT)
        if probe is None:
            env_health.bridge_connect_ok = False
            env_health.failure_reason = "bridge unreachable"
            return ValidationReport.error_report(
                self.validator_type, "Bridge unreachable",
                failure_type="bridge_unreachable", env_health=env_health,
            )
        env_health.bridge_connect_ok = True

        for chk in self.config.get("actor_checks", []):
            actor_name = chk.get("actor_name", "")
            prop_path = chk.get("property_path", "")
            op = chk.get("op", "eq")
            expected = chk.get("value")
            severity = chk.get("severity", "required")
            tol_cfg = chk.get("tolerance")
            tolerance = Tolerance.from_dict(tol_cfg) if tol_cfg else None

            check_name = f"{actor_name}.{prop_path}"
            props = _retry(lambda: adapter.get_actor_properties(actor_name), _RETRY_COUNT)
            if props is None:
                checks.append(CheckResult(
                    name=check_name, passed=False,
                    expected=f"{op} {expected}", actual=None,
                    severity=severity, category="editor_config",
                    message=f"Could not get properties for '{actor_name}'",
                ))
                continue

            actual = walk_property_path(props, prop_path)
            checks.append(make_check(
                name=check_name, actual=actual, op=op, expected=expected,
                category="editor_config", severity=severity, tolerance=tolerance,
            ))

        required = [c for c in checks if c.severity == "required"]
        passed = all(c.passed for c in required) if required else False
        n_pass = sum(c.passed for c in checks)

        return ValidationReport(
            passed=passed,
            validator_type=self.validator_type,
            checks=checks,
            summary=f"{n_pass}/{len(checks)} checks passed",
            phase="behavioral",
            failure_type="" if passed else "wrong_editor_config",
            environment_health=env_health,
        )
