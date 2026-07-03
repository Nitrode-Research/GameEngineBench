"""
Canonical task schema for Unreal benchmark authoring.

The file layout is intentionally split:

  - task.yaml         : lean execution contract
  - public_contract.yaml : optional static shape / writable-surface contract
  - scoring.yaml      : optional benchmark scoring metadata
  - calibration.yaml  : optional authoring lifecycle + readiness metadata

The in-memory TaskSchema still exposes a merged view so the rest of the pipeline
can stay mostly unchanged while task.yaml remains readable.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Optional

import yaml


CURRENT_SCHEMA_VERSION = 1

TASK_FAMILIES = {
    "time_simulation",
    "editor_property_config",
    "scene_construction",
    "procedural_spatial_math",
    "render_config",
    "render_pipeline_effect",
}

PUBLIC_VALIDATOR_TYPES = {"legacy_validate_script", "schema_static_sanity"}
PRIVATE_VALIDATOR_TYPES = {
    "runtime_behavioral",
    "editor_inspection",
    "scene_graph",
    "spatial_invariant",
    "config_plus_scene",
    "pipeline_and_frame",
    "static_only",
}

CAPABILITY_MODES = {"code_only", "hybrid"}
DIFFICULTIES = {"easy", "medium", "hard"}

LIFECYCLE_STATES = {
    "draft",
    "migrated",
    "calibrating",
    "stable",
    "official",
    "deprecated",
}

PRIVATE_VALIDATION_MODES = {"strict", "diagnostic"}

TASK_YAML_FIELDS = [
    "schema_version",
    "task_id",
    "task_name",
    "engine",
    "engine_version",
    "language",
    "capability_mode",
    "difficulty",
    "skill_category",
    "task_family",
    "instruction",
    "starting_state",
    "validation",
    "metadata",
]

TASK_YAML_OPTIONAL_INLINE_FIELDS = ["public_contract", "scoring", "calibration"]

SIDECAR_FILENAMES = {
    "public_contract": "public_contract.yaml",
    "scoring": "scoring.yaml",
    "calibration": "calibration.yaml",
}


@dataclass
class TaskSchema:
    schema_version: int
    task_id: str
    task_name: str
    engine: str
    engine_version: str
    language: str
    capability_mode: str
    difficulty: str
    skill_category: str
    task_family: str
    instruction: str
    starting_state: dict
    validation: dict
    metadata: dict
    public_contract: dict = field(default_factory=dict)
    scoring: dict = field(default_factory=dict)
    calibration: dict = field(default_factory=dict)
    task_dir: Optional[Path] = field(default=None, repr=False)

    @property
    def allowed_files(self) -> list[str]:
        return self.public_contract.get("allowed_files") or self.starting_state.get("writable_files", [])

    @property
    def files_to_edit(self) -> list[str]:
        return self.allowed_files

    @property
    def writable_files(self) -> list[str]:
        return self.starting_state.get("writable_files", self.allowed_files)

    @property
    def static_sanity_script(self) -> Optional[Path]:
        for v in self.public_validators:
            if v.get("type") == "legacy_validate_script":
                script = v.get("script")
                if script and self.task_dir:
                    return self.task_dir / script
        return None

    @property
    def public_validators(self) -> list[dict]:
        return self.validation.get("public", {}).get("validators", [])

    @property
    def private_validators(self) -> list[dict]:
        return self.validation.get("private", {}).get("validators", [])

    @property
    def private_mode(self) -> str:
        return self.validation.get("private", {}).get("mode", "strict")

    @property
    def lifecycle_status(self) -> str:
        return self.calibration.get("status", "draft")

    @property
    def requires_alt_solutions(self) -> bool:
        return bool(self.calibration.get("requires_alt_solutions", False))

    @property
    def applicable_tracks(self) -> list[str]:
        if self.capability_mode == "hybrid":
            return ["code_only", "hybrid"]
        return ["code_only"]

    def is_ready_for_track(self, track: str) -> bool:
        return bool(self.calibration.get("readiness", {}).get(track, False))

    @property
    def benchmark_ready(self) -> bool:
        return all(self.is_ready_for_track(t) for t in self.applicable_tracks)

    def to_dict(self) -> dict:
        return {
            "schema_version": self.schema_version,
            "task_id": self.task_id,
            "task_name": self.task_name,
            "engine": self.engine,
            "engine_version": self.engine_version,
            "language": self.language,
            "capability_mode": self.capability_mode,
            "difficulty": self.difficulty,
            "skill_category": self.skill_category,
            "task_family": self.task_family,
            "instruction": self.instruction,
            "starting_state": self.starting_state,
            "validation": self.validation,
            "metadata": self.metadata,
            "public_contract": self.public_contract,
            "scoring": self.scoring,
            "calibration": self.calibration,
        }


def load_task_schema(task_dir: Path, strict: bool = True) -> TaskSchema:
    task_dir = Path(task_dir)
    yaml_path = task_dir / "task.yaml"

    if not yaml_path.exists():
        raise FileNotFoundError(
            f"No task.yaml in {task_dir}. "
            "Run `python -m unrealbench.src.authoring.migrate` to generate one."
        )

    with yaml_path.open(encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}

    data = _merge_sidecars(task_dir, data)

    ver = data.get("schema_version")
    if ver is None:
        raise ValueError(f"{task_dir.name}/task.yaml: missing schema_version")
    if not isinstance(ver, int) or ver != CURRENT_SCHEMA_VERSION:
        raise ValueError(
            f"{task_dir.name}/task.yaml: schema_version={ver!r}, "
            f"expected {CURRENT_SCHEMA_VERSION}"
        )

    if strict:
        errors = validate_schema(data)
        if errors:
            raise ValueError(
                f"{task_dir.name}/task.yaml has schema errors:\n"
                + "\n".join(f"  - {e}" for e in errors)
            )

    schema = _build_from_dict(data)
    schema.task_dir = task_dir
    return schema


def load_task_schema_any(task_dir: Path) -> TaskSchema:
    task_dir = Path(task_dir)
    if (task_dir / "task.yaml").exists():
        return load_task_schema(task_dir)
    return synthesize_from_legacy(task_dir)


def validate_schema(data: dict) -> list[str]:
    errors: list[str] = []

    required_top = [
        "schema_version",
        "task_id",
        "task_name",
        "engine",
        "engine_version",
        "language",
        "capability_mode",
        "difficulty",
        "skill_category",
        "task_family",
        "instruction",
        "starting_state",
        "validation",
        "metadata",
    ]
    for key in required_top:
        if key not in data:
            errors.append(f"Missing required field: '{key}'")

    tf = data.get("task_family")
    if tf and tf not in TASK_FAMILIES:
        errors.append(f"task_family '{tf}' not in {sorted(TASK_FAMILIES)}")

    cm = data.get("capability_mode")
    if cm and cm not in CAPABILITY_MODES:
        errors.append(f"capability_mode '{cm}' not in {sorted(CAPABILITY_MODES)}")

    diff = data.get("difficulty")
    if diff and diff not in DIFFICULTIES:
        errors.append(f"difficulty '{diff}' not in {sorted(DIFFICULTIES)}")

    pc = data.get("public_contract", {})
    ss = data.get("starting_state", {})
    if isinstance(ss, dict) and not ss.get("writable_files"):
        errors.append("starting_state.writable_files is required and must be non-empty")

    if isinstance(pc, dict) and pc:
        if pc.get("allowed_files") and set(pc["allowed_files"]) != set(ss.get("writable_files", [])):
            errors.append(
                "public_contract.allowed_files must match starting_state.writable_files when provided"
            )
        has_output_surface = any(
            pc.get(k)
            for k in ("required_classes", "required_callables", "required_assets", "required_actors")
        )
        if not has_output_surface and not pc.get("behavior_only"):
            errors.append(
                "public_contract must define at least one required output surface "
                "(required_classes, required_callables, required_assets, or required_actors) "
                "unless behavior_only: true is set"
            )

    val = data.get("validation", {})
    if isinstance(val, dict):
        pub = val.get("public", {})
        if isinstance(pub, dict) and not pub.get("validators"):
            errors.append("validation.public.validators must be non-empty")

        for i, v in enumerate(pub.get("validators", []) if isinstance(pub, dict) else []):
            vtype = v.get("type") if isinstance(v, dict) else None
            if vtype and vtype not in PUBLIC_VALIDATOR_TYPES:
                errors.append(
                    f"validation.public.validators[{i}].type '{vtype}' not in "
                    f"{sorted(PUBLIC_VALIDATOR_TYPES)}"
                )
            elif vtype and isinstance(v, dict):
                cfg_errors = _validate_validator_config(vtype, v, is_public=True)
                errors.extend(
                    f"validation.public.validators[{i}] ({vtype}): {e}"
                    for e in cfg_errors
                )

        priv = val.get("private", {})
        if isinstance(priv, dict):
            mode = priv.get("mode", "strict")
            if mode not in PRIVATE_VALIDATION_MODES:
                errors.append(
                    f"validation.private.mode '{mode}' not in {sorted(PRIVATE_VALIDATION_MODES)}"
                )
            if cm == "hybrid" and not priv.get("validators"):
                errors.append("validation.private.validators should be non-empty for hybrid tasks")
            for i, v in enumerate(priv.get("validators", [])):
                vtype = v.get("type") if isinstance(v, dict) else None
                if vtype and vtype not in PRIVATE_VALIDATOR_TYPES:
                    errors.append(
                        f"validation.private.validators[{i}].type '{vtype}' not in "
                        f"{sorted(PRIVATE_VALIDATOR_TYPES)}"
                    )
                elif vtype and isinstance(v, dict):
                    cfg_errors = _validate_validator_config(vtype, v, is_public=False)
                    errors.extend(
                        f"validation.private.validators[{i}] ({vtype}): {e}"
                        for e in cfg_errors
                    )

    instruction = data.get("instruction", "")
    if isinstance(instruction, str):
        if not instruction.strip():
            errors.append("instruction is empty")
        for placeholder in ("TODO", "FIXME"):
            if placeholder in instruction:
                errors.append(f"instruction contains unresolved placeholder: {placeholder}")

    if isinstance(pc, dict):
        for field_name in ("required_classes", "required_callables"):
            val_list = pc.get(field_name, [])
            if isinstance(val_list, list):
                for item in val_list:
                    if isinstance(item, str) and ("TODO" in item or "FIXME" in item):
                        errors.append(
                            f"public_contract.{field_name} contains unresolved placeholder"
                        )

    cal = data.get("calibration", {})
    if isinstance(cal, dict) and cal:
        status = cal.get("status")
        if status and status not in LIFECYCLE_STATES:
            errors.append(f"calibration.status '{status}' not in {sorted(LIFECYCLE_STATES)}")

    return errors


def dump_task_schema(schema: TaskSchema, path: Path) -> None:
    path = Path(path)
    task_dir = path.parent
    task_dir.mkdir(parents=True, exist_ok=True)

    merged = schema.to_dict()
    task_doc = {k: merged[k] for k in TASK_YAML_FIELDS if k in merged}

    with path.open("w", encoding="utf-8") as f:
        yaml.dump(task_doc, f, default_flow_style=False, allow_unicode=True, sort_keys=False)

    _write_sidecar(task_dir, "public_contract", schema.public_contract)
    _write_sidecar(task_dir, "scoring", schema.scoring)
    _write_sidecar(task_dir, "calibration", schema.calibration)


def synthesize_from_legacy(task_dir: Path) -> TaskSchema:
    task_dir = Path(task_dir)
    config_path = task_dir / "task_config.json"
    if not config_path.exists():
        raise FileNotFoundError(f"No task_config.json or task.yaml in {task_dir}")

    with config_path.open(encoding="utf-8") as f:
        cfg = json.load(f)

    warnings: list[str] = []

    task_id = cfg.get("task_id", task_dir.name)
    capability_mode = cfg.get("capability_mode", "code_only")
    categories = cfg.get("categories", [])
    files_to_edit = cfg.get("files_to_edit", [])

    task_family = _infer_task_family(task_id, categories, capability_mode)
    if task_id not in _TASK_ID_FAMILY:
        warnings.append(
            f"task_family='{task_family}' inferred from task_id/categories — verify manually"
        )

    public_contract: dict[str, Any] = {
        "allowed_files": files_to_edit,
        "prohibited_files": [],
        "required_classes": [],
        "required_properties": [],
        "required_callables": [],
        "required_assets": [],
        "required_actors": [],
        "behavior_only": True,
    }
    warnings.extend([
        "public_contract.required_classes is empty — populate from task instruction manually",
        "public_contract.required_properties is empty — populate from task instruction manually",
        "public_contract.required_callables is empty — populate from task instruction manually",
        "public_contract.behavior_only=true set as placeholder — remove once required_classes/callables are populated",
    ])

    starting_state: dict[str, Any] = {
        "map": None,
        "writable_files": files_to_edit,
        "writable_assets": [],
        "required_assets": [],
        "plugins": ["UnrealMCP"],
        "pie_available": capability_mode == "hybrid",
        "map_edits_allowed": False,
        "project_state_hash": None,
    }
    warnings.append("starting_state.map not set — populate with actual map path")
    if cfg.get("starter_map_provides"):
        starting_state["starter_map_provides"] = [{"class": c} for c in cfg["starter_map_provides"]]
    if cfg.get("agent_must_place"):
        starting_state["agent_must_place"] = [{"class": c} for c in cfg["agent_must_place"]]
    if cfg.get("agent_must_wire"):
        starting_state["agent_must_wire"] = cfg["agent_must_wire"]

    validate_script_exists = (task_dir / "validate.py").exists()
    default_validator_type = _family_to_default_private_validator(task_family)

    public_validators: list[dict] = []
    if validate_script_exists:
        public_validators.append({
            "type": "legacy_validate_script",
            "script": "validate.py",
            "audit_start_should_fail": True,
        })
    else:
        public_validators.append({"type": "schema_static_sanity"})
        warnings.append("No validate.py found; schema_static_sanity used as public validator")

    validation: dict[str, Any] = {
        "public": {"validators": public_validators},
        "private": {
            "mode": "strict",
            "validators": [{"type": default_validator_type}],
        },
    }
    warnings.append(
        f"validation.private.validators[0].type='{default_validator_type}' is a placeholder — "
        "behavioral config (checkpoints/checks) must be authored before this task can pass hybrid calibration"
    )

    calibration: dict[str, Any] = {
        "status": "migrated",
        "requires_alt_solutions": False,
        "readiness": {"code_only": False, "hybrid": False},
        "metrics": {"code_only": _empty_metrics(), "hybrid": _empty_metrics()},
        "last_calibration_run": None,
        "migration_warnings": warnings + [f"{len(warnings)} migration warnings — review before admission"],
    }

    metadata: dict[str, Any] = {
        "godot_equivalent": cfg.get("godot_equivalent"),
        "categories": categories,
        "requires_multimodal": cfg.get("requires_multimodal", False),
        "authoring_domains": cfg.get("authoring_domains", []),
        "source": "migrated_from_legacy",
    }
    if cfg.get("metadata"):
        metadata["upstream_metadata"] = cfg["metadata"]

    return TaskSchema(
        schema_version=CURRENT_SCHEMA_VERSION,
        task_id=task_id,
        task_name=cfg.get("task_name", task_dir.name),
        engine=cfg.get("engine", "unreal"),
        engine_version=str(cfg.get("engine_version", "5.7")),
        language=cfg.get("language", "cpp"),
        capability_mode=capability_mode,
        difficulty=cfg.get("difficulty", "medium"),
        skill_category=cfg.get("skill_category", "general"),
        task_family=task_family,
        instruction=cfg.get("instruction", ""),
        starting_state=starting_state,
        validation=validation,
        metadata=metadata,
        public_contract=public_contract,
        calibration=calibration,
        task_dir=task_dir,
    )


def _build_from_dict(data: dict) -> TaskSchema:
    return TaskSchema(
        schema_version=data.get("schema_version", CURRENT_SCHEMA_VERSION),
        task_id=data["task_id"],
        task_name=data["task_name"],
        engine=data.get("engine", "unreal"),
        engine_version=str(data.get("engine_version", "5.7")),
        language=data.get("language", "cpp"),
        capability_mode=data["capability_mode"],
        difficulty=data["difficulty"],
        skill_category=data["skill_category"],
        task_family=data["task_family"],
        instruction=data["instruction"],
        starting_state=data.get("starting_state", {}),
        validation=data.get("validation", {}),
        metadata=data.get("metadata", {}),
        public_contract=data.get("public_contract", {}),
        scoring=data.get("scoring", {}),
        calibration=data.get("calibration", {}),
    )


def _merge_sidecars(task_dir: Path, data: dict) -> dict:
    merged = dict(data)
    for key, filename in SIDECAR_FILENAMES.items():
        sidecar_path = task_dir / filename
        if sidecar_path.exists():
            with sidecar_path.open(encoding="utf-8") as f:
                merged[key] = yaml.safe_load(f) or {}
        elif key in data:
            merged[key] = data.get(key) or {}
        else:
            merged[key] = {}
    return merged


def _write_sidecar(task_dir: Path, key: str, payload: dict) -> None:
    sidecar_path = task_dir / SIDECAR_FILENAMES[key]
    if payload:
        with sidecar_path.open("w", encoding="utf-8") as f:
            yaml.dump(payload, f, default_flow_style=False, allow_unicode=True, sort_keys=False)
        return
    if sidecar_path.exists():
        sidecar_path.unlink()


def _empty_metrics() -> dict:
    return {
        "good_solutions_pass_rate": None,
        "bad_solutions_fail_rate": None,
        "near_miss_fail_rate": None,
        "alt_good_pass_rate": None,
        "environment_failure_rate": None,
        "flake_rate": None,
    }


_TASK_ID_FAMILY: dict[str, str] = {
    "ue_task_0001": "time_simulation",
    "ue_task_0002": "editor_property_config",
    "ue_task_0003": "procedural_spatial_math",
    "ue_task_0004": "render_config",
    "ue_task_0005": "render_pipeline_effect",
    "ue_task_0006": "scene_construction",
}

_CATEGORY_FAMILY: dict[str, str] = {
    "lighting": "editor_property_config",
    "environment": "editor_property_config",
    "post_processing": "render_config",
    "fog": "editor_property_config",
    "color_grading": "render_config",
    "animation": "procedural_spatial_math",
    "scene_setup": "scene_construction",
    "rendering": "render_config",
}

_FAMILY_DEFAULT_PRIVATE_VALIDATOR: dict[str, str] = {
    "time_simulation": "runtime_behavioral",
    "editor_property_config": "editor_inspection",
    "scene_construction": "scene_graph",
    "procedural_spatial_math": "spatial_invariant",
    "render_config": "config_plus_scene",
    "render_pipeline_effect": "pipeline_and_frame",
}


def _infer_task_family(task_id: str, categories: list[str], capability_mode: str) -> str:
    if task_id in _TASK_ID_FAMILY:
        return _TASK_ID_FAMILY[task_id]
    for cat in categories:
        if cat in _CATEGORY_FAMILY:
            return _CATEGORY_FAMILY[cat]
    return "time_simulation" if capability_mode == "hybrid" else "editor_property_config"


def _family_to_default_private_validator(family: str) -> str:
    return _FAMILY_DEFAULT_PRIVATE_VALIDATOR.get(family, "static_only")


def _validate_validator_config(vtype: str, cfg: dict, is_public: bool) -> list[str]:
    try:
        from .validators.registry import validate_validator_config
        return validate_validator_config(vtype, cfg, is_public=is_public)
    except ImportError:
        return []
