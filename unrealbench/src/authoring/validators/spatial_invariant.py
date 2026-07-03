"""
Spatial invariant validator for procedural_spatial_math tasks (stub).

Evaluates geometric invariants (distance, symmetry, continuity) sampled
over multiple PIE frames. To be implemented when a spatial_math task is
upgraded to hybrid mode.
"""

from __future__ import annotations

from pathlib import Path
from typing import Optional

from .base import BaseValidator, ValidationReport, UnrealRuntimeAdapter


class SpatialInvariantValidator(BaseValidator):
    validator_type = "spatial_invariant"

    def validate(
        self,
        workspace: Path,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ) -> ValidationReport:
        return ValidationReport.skipped_report(
            self.validator_type,
            "spatial_invariant validator not yet implemented — "
            "upgrade task to hybrid and author invariant config",
            phase="behavioral",
        )
