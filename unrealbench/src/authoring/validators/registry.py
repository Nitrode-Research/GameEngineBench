"""
Validator registry and dispatcher.

Public API:
  run_full_validation(schema, workspace, adapter, mode)
    → tuple[list[ValidationReport], list[ValidationReport]]
      (public_reports, private_reports)

  validate_validator_config(vtype, cfg) → list[str]
    Per-validator config schema check. Called by schema.validate_schema()
    so bad configs are caught at load time, not at runtime.

Registration:
  PUBLIC_VALIDATOR_REGISTRY  — maps type string to class
  PRIVATE_VALIDATOR_REGISTRY — maps type string to class
"""

from __future__ import annotations

from pathlib import Path
from typing import Optional, TYPE_CHECKING

from .base import (
    BaseValidator,
    ValidationReport,
    UnrealRuntimeAdapter,
    timed_validate,
)
from .static_sanity import (
    LegacyValidateScriptRunner,
    SchemaStaticSanityValidator,
    build_public_validators,
)
from .runtime_behavioral import RuntimeBehavioralValidator
from .editor_inspection import EditorInspectionValidator
from .scene_graph import SceneGraphValidator
from .spatial_invariant import SpatialInvariantValidator
from .config_plus_scene import ConfigPlusSceneValidator
from .pipeline_and_frame import PipelineAndFrameValidator

if TYPE_CHECKING:
    from ..schema import TaskSchema


# ── Validator registries ──────────────────────────────────────────────────────

PUBLIC_VALIDATOR_REGISTRY: dict[str, type] = {
    "legacy_validate_script": LegacyValidateScriptRunner,
    "schema_static_sanity": SchemaStaticSanityValidator,
}

PRIVATE_VALIDATOR_REGISTRY: dict[str, type] = {
    "runtime_behavioral": RuntimeBehavioralValidator,
    "editor_inspection": EditorInspectionValidator,
    "scene_graph": SceneGraphValidator,
    "spatial_invariant": SpatialInvariantValidator,
    "config_plus_scene": ConfigPlusSceneValidator,
    "pipeline_and_frame": PipelineAndFrameValidator,
}


# ── Per-validator config validators ──────────────────────────────────────────
# Called at schema load time so bad configs are caught early.

def validate_legacy_validate_script_config(cfg: dict) -> list[str]:
    errors = []
    if not cfg.get("script"):
        errors.append("legacy_validate_script requires 'script' field")
    return errors


def validate_schema_static_sanity_config(cfg: dict) -> list[str]:
    return []  # No required config fields


def validate_runtime_behavioral_config(cfg: dict) -> list[str]:
    errors = []
    checkpoints = cfg.get("checkpoints")
    if not checkpoints or not isinstance(checkpoints, list):
        errors.append("runtime_behavioral requires non-empty 'checkpoints' list")
        return errors
    for i, cp in enumerate(checkpoints):
        if not isinstance(cp, dict):
            errors.append(f"checkpoints[{i}] must be a dict")
            continue
        if "time_s" not in cp:
            errors.append(f"checkpoints[{i}] missing 'time_s'")
        checks = cp.get("checks", [])
        if not checks:
            errors.append(f"checkpoints[{i}] has no checks")
        for j, chk in enumerate(checks):
            for required in ("actor", "property", "op", "value"):
                if required not in chk:
                    errors.append(
                        f"checkpoints[{i}].checks[{j}] missing '{required}'"
                    )
    return errors


def validate_editor_inspection_config(cfg: dict) -> list[str]:
    errors = []
    actor_checks = cfg.get("actor_checks")
    if not actor_checks or not isinstance(actor_checks, list):
        errors.append("editor_inspection requires non-empty 'actor_checks' list")
        return errors
    for i, chk in enumerate(actor_checks):
        for required in ("actor_name", "property_path", "op", "value"):
            if required not in chk:
                errors.append(f"actor_checks[{i}] missing '{required}'")
    return errors


def validate_scene_graph_config(cfg: dict) -> list[str]:
    errors = []
    if not cfg.get("required_actors") and not cfg.get("forbidden_actors"):
        errors.append(
            "scene_graph requires at least one of 'required_actors' or 'forbidden_actors'"
        )
    return errors


def validate_spatial_invariant_config(cfg: dict) -> list[str]:
    # Stub — full validation when implementation is complete
    return []


def validate_config_plus_scene_config(cfg: dict) -> list[str]:
    # Delegates to sub-validators; no additional top-level requirements
    return []


def validate_pipeline_and_frame_config(cfg: dict) -> list[str]:
    # Stub
    return []


_PUBLIC_CONFIG_VALIDATORS = {
    "legacy_validate_script": validate_legacy_validate_script_config,
    "schema_static_sanity": validate_schema_static_sanity_config,
}

_PRIVATE_CONFIG_VALIDATORS = {
    "runtime_behavioral": validate_runtime_behavioral_config,
    "editor_inspection": validate_editor_inspection_config,
    "scene_graph": validate_scene_graph_config,
    "spatial_invariant": validate_spatial_invariant_config,
    "config_plus_scene": validate_config_plus_scene_config,
    "pipeline_and_frame": validate_pipeline_and_frame_config,
}


def validate_validator_config(vtype: str, cfg: dict, is_public: bool = False) -> list[str]:
    """Validate the config dict for a specific validator type.

    Returns a list of error strings. Called by schema.validate_schema().
    """
    registry = _PUBLIC_CONFIG_VALIDATORS if is_public else _PRIVATE_CONFIG_VALIDATORS
    fn = registry.get(vtype)
    if fn is None:
        return []  # Unknown type — caught by schema type enumeration check
    return fn(cfg)


# ── Main dispatcher ───────────────────────────────────────────────────────────

def run_full_validation(
    schema: "TaskSchema",
    workspace: Path,
    adapter: Optional[UnrealRuntimeAdapter] = None,
    mode: str = "code_only",
) -> tuple[list[ValidationReport], list[ValidationReport]]:
    """Run all public then (optionally) private validators for a task.

    Returns (public_reports, private_reports).

    In code_only mode:
      - Public validators run (static sanity).
      - Private validators return skipped sentinels.

    In hybrid mode with mode='strict' (default):
      - Public validators run first.
      - Private validators only run if ALL public validators passed.

    In hybrid mode with mode='diagnostic':
      - Public validators run.
      - Private validators always run regardless of public result.
      - Overall pass requires BOTH public and private to pass.

    adapter: UnrealRuntimeAdapter or None.
    """
    workspace = Path(workspace)

    # ── Public validators ─────────────────────────────────────────────────────
    public_validators = build_public_validators(schema)
    public_reports: list[ValidationReport] = []

    for v in public_validators:
        report = timed_validate(v, workspace, adapter=None)  # public = static only
        public_reports.append(report)

    public_passed = all(r.passed for r in public_reports)

    # ── Private validators ────────────────────────────────────────────────────
    private_reports: list[ValidationReport] = []

    if mode == "code_only" or adapter is None:
        # Skip all private validators
        for vcfg in schema.private_validators:
            vtype = vcfg.get("type", "unknown")
            private_reports.append(
                ValidationReport.skipped_report(
                    vtype,
                    "Private validation requires hybrid mode with live editor adapter.",
                    phase="behavioral",
                )
            )
        return public_reports, private_reports

    # hybrid mode
    private_mode = schema.private_mode  # strict | diagnostic
    should_run_private = public_passed or private_mode == "diagnostic"

    for vcfg in schema.private_validators:
        vtype = vcfg.get("type", "unknown")

        if not should_run_private:
            private_reports.append(
                ValidationReport.skipped_report(
                    vtype,
                    "Skipped: public validator failed and mode is 'strict'.",
                    phase="behavioral",
                )
            )
            continue

        cls = PRIVATE_VALIDATOR_REGISTRY.get(vtype)
        if cls is None:
            private_reports.append(
                ValidationReport.error_report(
                    vtype,
                    f"Unknown private validator type: '{vtype}'",
                    failure_type="not_implemented",
                )
            )
            continue

        validator = cls(vcfg)
        report = timed_validate(validator, workspace, adapter)
        private_reports.append(report)

    return public_reports, private_reports


def overall_passed(
    public_reports: list[ValidationReport],
    private_reports: list[ValidationReport],
) -> bool:
    """True if all required (non-skipped) reports passed."""
    for r in public_reports + private_reports:
        if r.skipped:
            continue
        if not r.passed:
            return False
    return True
