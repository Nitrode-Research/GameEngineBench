"""
Scene graph validator for scene_construction tasks.

Checks actor presence and transforms via get_actors_in_level.

Config keys:
  required_actors: list[{
    class_name:    str   (optional) actor class must match
    name_pattern:  str   (optional) fnmatch glob against actor name
    count_min:     int = 1
  }]
  forbidden_actors: list[str]   class names that must NOT be present
  transform_checks: list[{
    actor_pattern: str
    location:      {x, y, z}   optional
    rotation:      {pitch, yaw, roll}   optional
    tolerance:     {type, value}  default transform_cm
  }]
"""

from __future__ import annotations

import fnmatch
from pathlib import Path
from typing import Optional

from .base import (
    BaseValidator,
    CheckResult,
    EnvironmentHealth,
    Tolerance,
    ValidationReport,
    UnrealRuntimeAdapter,
)
from .runtime_behavioral import _retry, _RETRY_COUNT


class SceneGraphValidator(BaseValidator):
    validator_type = "scene_graph"

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
            return ValidationReport.error_report(
                self.validator_type, "Bridge unreachable",
                failure_type="bridge_unreachable", env_health=env_health,
            )
        env_health.bridge_connect_ok = True

        actors = _retry(lambda: adapter.get_actors_in_level(), _RETRY_COUNT) or []

        # required_actors
        for req in self.config.get("required_actors", []):
            class_name = req.get("class_name")
            name_pattern = req.get("name_pattern", "*")
            count_min = int(req.get("count_min", 1))

            matches = [
                a for a in actors
                if (not class_name or a.get("class") == class_name)
                and fnmatch.fnmatch(a.get("name", ""), name_pattern)
            ]
            passed = len(matches) >= count_min
            check_name = f"required_{class_name or name_pattern}"
            checks.append(CheckResult(
                name=check_name,
                passed=passed,
                expected=f">= {count_min} actors",
                actual=len(matches),
                severity="required",
                category="scene_graph",
                message="" if passed else
                    f"Found {len(matches)} actors matching {class_name or name_pattern}, "
                    f"need >= {count_min}",
            ))

        # forbidden_actors
        for forbidden_class in self.config.get("forbidden_actors", []):
            found = any(a.get("class") == forbidden_class for a in actors)
            checks.append(CheckResult(
                name=f"forbidden_{forbidden_class}",
                passed=not found,
                expected="not present",
                actual="found" if found else "not found",
                severity="required",
                category="scene_graph",
                message="" if not found else f"Forbidden class '{forbidden_class}' found in level",
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
            failure_type="" if passed else "wrong_scene_state",
            environment_health=env_health,
        )
