"""
Static sanity validators (public validation layer).

Two implementations:

  LegacyValidateScriptRunner   — subprocess-runs the task's validate.py (TRANSITIONAL)
  SchemaStaticSanityValidator  — checks public_contract fields directly (TARGET STATE)

Retirement policy:
  Newly authored tasks MUST use SchemaStaticSanityValidator (type: schema_static_sanity).
  LegacyValidateScriptRunner (type: legacy_validate_script) is permitted only for tasks
  with calibration.status == 'migrated', unless metadata.legacy_validator_reason is set.
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Optional, TYPE_CHECKING

from .base import (
    BaseValidator,
    CheckResult,
    ValidationReport,
    UnrealRuntimeAdapter,
)

if TYPE_CHECKING:
    from ..schema import TaskSchema


class LegacyValidateScriptRunner(BaseValidator):
    """Runs the task's validate.py against a workspace copy.

    Construction of the val_copy mirrors ue_benchmark_runner._validate():
      1. Copy start/ to a temp directory
      2. Overlay files from allowed_files (from workspace)
      3. Execute validate.py with val_copy as the argument

    This is TRANSITIONAL — tasks should migrate to SchemaStaticSanityValidator.
    """

    validator_type = "legacy_validate_script"

    def __init__(
        self,
        validator_config: dict,
        validate_script: Path,
        start_dir: Path,
        files_to_overlay: list[str],
    ):
        super().__init__(validator_config)
        self.validate_script = Path(validate_script)
        self.start_dir = Path(start_dir)
        self.files_to_overlay = files_to_overlay

    def validate(
        self,
        workspace: Path,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ) -> ValidationReport:
        val_copy: Optional[Path] = None
        try:
            val_copy = Path(tempfile.mkdtemp()) / "val"
            shutil.copytree(self.start_dir, val_copy)

            for rel in self.files_to_overlay:
                ws_file = Path(workspace) / rel
                if ws_file.exists():
                    dest = val_copy / rel
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(ws_file, dest)

            result = subprocess.run(
                [sys.executable, str(self.validate_script), str(val_copy)],
                capture_output=True,
                text=True,
            )
            output = (result.stdout + result.stderr).strip()
            passed = result.returncode == 0

            checks = [
                CheckResult(
                    name="validate_script_exit_code",
                    passed=passed,
                    expected=0,
                    actual=result.returncode,
                    severity="required",
                    category="static_shape",
                    message="" if passed else f"validate.py exited {result.returncode}",
                )
            ]

            return ValidationReport(
                passed=passed,
                validator_type=self.validator_type,
                checks=checks,
                summary="PASSED" if passed else "FAILED",
                raw_output=output,
                phase="static",
                failure_type="" if passed else "missing_public_contract",
            )

        except Exception as exc:
            return ValidationReport.error_report(
                self.validator_type, str(exc), failure_type="validator_runtime_error"
            )
        finally:
            if val_copy and val_copy.exists():
                shutil.rmtree(val_copy, ignore_errors=True)

    def as_sanity_only(self, workspace: Path) -> ValidationReport:
        return self.validate(workspace, adapter=None)


class SchemaStaticSanityValidator(BaseValidator):
    """Checks public_contract fields from the task schema against edited source files.

    Checks:
    - required_classes: class declaration exists in allowed files
    - required_properties: property name appears in source
    - required_callables: callable name appears in source

    For deep AST-level checks, tasks should still use LegacyValidateScriptRunner
    until this validator is extended with proper C++ parsing.
    """

    validator_type = "schema_static_sanity"

    def __init__(self, validator_config: dict, public_contract: dict, fallback_allowed_files: Optional[list[str]] = None):
        super().__init__(validator_config)
        self.public_contract = public_contract
        self.fallback_allowed_files = fallback_allowed_files or []

    def validate(
        self,
        workspace: Path,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ) -> ValidationReport:
        checks: list[CheckResult] = []
        workspace = Path(workspace)

        # Read all allowed (edited) source files
        source_text = ""
        allowed_files = self.public_contract.get("allowed_files") or self.fallback_allowed_files
        for rel in allowed_files:
            f = workspace / rel
            if f.exists():
                try:
                    source_text += f.read_text(encoding="utf-8", errors="replace")
                except OSError:
                    pass

        for cls_name in self.public_contract.get("required_classes", []):
            if not isinstance(cls_name, str):
                continue
            found = cls_name in source_text
            checks.append(CheckResult(
                name=f"class_{cls_name}",
                passed=found,
                expected=f"class {cls_name} declared",
                actual="found" if found else "not found",
                severity="required",
                category="static_shape",
                message="" if found else f"Class '{cls_name}' not found in edited files",
            ))

        for prop in self.public_contract.get("required_properties", []):
            name = prop.get("name", "") if isinstance(prop, dict) else ""
            if not name:
                continue
            found = name in source_text
            checks.append(CheckResult(
                name=f"property_{name}",
                passed=found,
                expected=f"{name} declared",
                actual="found" if found else "not found",
                severity="required",
                category="static_shape",
                message="" if found else f"Property '{name}' not found",
            ))

        for fn in self.public_contract.get("required_callables", []):
            name = fn.get("name", "") if isinstance(fn, dict) else ""
            if not name:
                continue
            found = name in source_text
            checks.append(CheckResult(
                name=f"callable_{name}",
                passed=found,
                expected=f"{name} declared",
                actual="found" if found else "not found",
                severity="required",
                category="static_shape",
                message="" if found else f"Callable '{name}' not found",
            ))

        passed = all(c.passed for c in checks if c.severity == "required") if checks else True
        return ValidationReport(
            passed=passed,
            validator_type=self.validator_type,
            checks=checks,
            summary="PASSED" if passed else "FAILED",
            phase="static",
            failure_type="" if passed else "missing_public_contract",
        )

    def as_sanity_only(self, workspace: Path) -> ValidationReport:
        return self.validate(workspace, adapter=None)


def build_public_validators(schema: "TaskSchema") -> list[BaseValidator]:
    """Instantiate all public validators declared in validation.public.validators."""
    validators = []
    start_dir = schema.task_dir / "start" if schema.task_dir else Path("start")

    for vcfg in schema.public_validators:
        vtype = vcfg.get("type")
        if vtype == "legacy_validate_script":
            script = schema.static_sanity_script
            if script and script.exists():
                validators.append(LegacyValidateScriptRunner(
                    validator_config=vcfg,
                    validate_script=script,
                    start_dir=start_dir,
                    files_to_overlay=schema.allowed_files,
                ))
        elif vtype == "schema_static_sanity":
            validators.append(SchemaStaticSanityValidator(
                validator_config=vcfg,
                public_contract=schema.public_contract,
                fallback_allowed_files=schema.allowed_files,
            ))

    return validators
