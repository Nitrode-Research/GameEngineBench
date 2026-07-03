"""
Config-plus-scene validator for render_config tasks.

Combines editor inspection (PP volume property checks) with scene graph
(required actor presence). Delegates to the two sub-validators.

Config keys: union of editor_inspection config + scene_graph config.
"""

from __future__ import annotations

from pathlib import Path
from typing import Optional

from .base import BaseValidator, CheckResult, ValidationReport, UnrealRuntimeAdapter
from .editor_inspection import EditorInspectionValidator
from .scene_graph import SceneGraphValidator


class ConfigPlusSceneValidator(BaseValidator):
    validator_type = "config_plus_scene"

    def __init__(self, validator_config: dict):
        super().__init__(validator_config)
        self._editor_val = EditorInspectionValidator(validator_config)
        self._scene_val = SceneGraphValidator(validator_config)

    def validate(
        self,
        workspace: Path,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ) -> ValidationReport:
        if adapter is None:
            return self.as_sanity_only(workspace)

        editor_report = self._editor_val.validate(workspace, adapter)
        scene_report = self._scene_val.validate(workspace, adapter)

        all_checks = editor_report.checks + scene_report.checks
        passed = editor_report.passed and scene_report.passed
        n_pass = sum(c.passed for c in all_checks)

        # Merge environment health — pick most informative
        env_health = editor_report.environment_health or scene_report.environment_health

        failure_type = ""
        if not passed:
            if not editor_report.passed:
                failure_type = editor_report.failure_type or "wrong_editor_config"
            elif not scene_report.passed:
                failure_type = scene_report.failure_type or "wrong_scene_state"

        return ValidationReport(
            passed=passed,
            validator_type=self.validator_type,
            checks=all_checks,
            summary=f"{n_pass}/{len(all_checks)} checks passed",
            phase="behavioral",
            failure_type=failure_type,
            environment_health=env_health,
        )
