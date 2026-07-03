"""
Pipeline-and-frame validator for render_pipeline_effect tasks (stub).

Checks render pass activation via console variables and optionally
compares frame output against a baseline screenshot. To be implemented
when a render_pipeline task is upgraded to hybrid mode.
"""

from __future__ import annotations

from pathlib import Path
from typing import Optional

from .base import BaseValidator, ValidationReport, UnrealRuntimeAdapter


class PipelineAndFrameValidator(BaseValidator):
    validator_type = "pipeline_and_frame"

    def validate(
        self,
        workspace: Path,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ) -> ValidationReport:
        return ValidationReport.skipped_report(
            self.validator_type,
            "pipeline_and_frame validator not yet implemented — "
            "upgrade task to hybrid and author cvar/screenshot config",
            phase="behavioral",
        )
