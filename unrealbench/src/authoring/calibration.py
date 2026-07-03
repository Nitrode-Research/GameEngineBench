"""
Calibration harness for benchmark task quality gating.

Runs good/bad/near-miss/alt-good solution sets through the full validation
pipeline and evaluates 10 readiness criteria (5 per track where applicable).

Usage:
    from unrealbench.src.authoring.calibration import CalibrationHarness
    harness = CalibrationHarness(schema, workspace_root)
    report = harness.run(mode="code_only")          # or "hybrid"
    report = harness.run(mode="both")               # runs both tracks

Criteria (per track):
  C1  good_solutions_pass_rate  ≥ 1.0   (all good solutions must pass)
  C2  bad_solutions_fail_rate   ≥ 0.80  (≥ 80% of bad solutions must fail)
  C3  near_miss_fail_rate       ≥ 0.80  (≥ 80% of near-miss solutions must fail)
  C4  alt_good_pass_rate        ≥ 1.0   (all alt_good must pass, if flag set)
  C5  environment_failure_rate  ≤ 0.10  (≤ 10% environment failures allowed)
  (C4 skipped when requires_alt_solutions is False)

Workspace contract:
  - calibration/               under task_dir
    - bad_solutions/bad_01/ …
    - near_miss_solutions/near_01/ …
    - alt_good_solutions/alt_01/ …   (required if requires_alt_solutions)
  - start/                     starting state snapshot (read-only reference)
  - solution/                  canonical good solution (must be present)

Each solution directory must contain writable_files relative to the task root.
The harness builds an isolated workspace copy per solution run.
"""

from __future__ import annotations

import shutil
import tempfile
import traceback
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, TYPE_CHECKING

from .validators.base import ValidationReport, UnrealRuntimeAdapter
from .validators.registry import run_full_validation, overall_passed

if TYPE_CHECKING:
    from .schema import TaskSchema


# ── Thresholds ────────────────────────────────────────────────────────────────

GOOD_PASS_THRESHOLD = 1.0        # all good solutions must pass
BAD_FAIL_THRESHOLD = 0.80        # ≥ 80% of bad solutions must fail
NEAR_MISS_FAIL_THRESHOLD = 0.80  # ≥ 80% of near-miss solutions must fail
ALT_GOOD_PASS_THRESHOLD = 1.0    # all alt-good solutions must pass
ENV_FAIL_THRESHOLD = 0.10        # ≤ 10% environment failures allowed


# ── Data structures ───────────────────────────────────────────────────────────

@dataclass
class SolutionRunResult:
    """Result of running a single solution through full validation."""
    solution_name: str
    solution_set: str          # good | bad | near_miss | alt_good
    passed: bool
    environment_ok: bool
    public_reports: list[ValidationReport]
    private_reports: list[ValidationReport]
    error: str = ""            # non-empty if harness-level exception occurred


@dataclass
class CriterionResult:
    criterion: str             # e.g. "good_solutions_pass_rate"
    passed: bool | None        # None = skipped (not applicable)
    value: float | None        # measured value (None if skipped or error)
    threshold: float | None    # required threshold
    detail: str = ""


@dataclass
class TrackCalibrationResult:
    track: str                 # code_only | hybrid
    criteria: list[CriterionResult]
    solution_runs: list[SolutionRunResult]
    ready: bool                # all applicable criteria passed

    @property
    def metrics(self) -> dict:
        out: dict = {}
        for cr in self.criteria:
            out[cr.criterion] = cr.value
        return out


@dataclass
class CalibrationReport:
    task_id: str
    tracks: dict[str, TrackCalibrationResult]  # track → result
    overall_ready: bool
    warnings: list[str] = field(default_factory=list)

    def as_calibration_dict(self) -> dict:
        """Build the calibration block to write back to task.yaml."""
        metrics: dict = {}
        readiness: dict = {}

        for track, result in self.tracks.items():
            metrics[track] = result.metrics
            readiness[track] = result.ready

        return {
            "readiness": readiness,
            "metrics": metrics,
        }


# ── Harness ───────────────────────────────────────────────────────────────────

class CalibrationHarness:
    """Runs calibration solution sets and evaluates readiness criteria."""

    def __init__(
        self,
        schema: "TaskSchema",
        workspace_root: Optional[Path] = None,
        adapter: Optional[UnrealRuntimeAdapter] = None,
    ):
        self.schema = schema
        self.adapter = adapter

        if workspace_root is not None:
            self.workspace_root = Path(workspace_root)
        elif schema.task_dir is not None:
            self.workspace_root = schema.task_dir
        else:
            raise ValueError("Either workspace_root or schema.task_dir must be set")

        self._calibration_dir = self.workspace_root / "calibration"
        self._start_dir = self.workspace_root / "start"
        self._solution_dir = self.workspace_root / "solution"

    # ── Public entry point ────────────────────────────────────────────────────

    def run(self, mode: str = "code_only") -> CalibrationReport:
        """Run calibration for one or both tracks.

        mode: "code_only" | "hybrid" | "both"
        """
        if mode not in ("code_only", "hybrid", "both"):
            raise ValueError(f"Invalid calibration mode: {mode!r}")

        tracks_to_run: list[str] = []
        if mode == "both":
            tracks_to_run = list(self.schema.applicable_tracks)
        elif mode in self.schema.applicable_tracks:
            tracks_to_run = [mode]
        else:
            tracks_to_run = []

        warnings: list[str] = []
        track_results: dict[str, TrackCalibrationResult] = {}

        preflight_warnings = self._preflight_check()
        warnings.extend(preflight_warnings)

        for track in tracks_to_run:
            result = self._run_track(track)
            track_results[track] = result

        overall_ready = bool(track_results) and all(
            r.ready for r in track_results.values()
        )

        return CalibrationReport(
            task_id=self.schema.task_id,
            tracks=track_results,
            overall_ready=overall_ready,
            warnings=warnings,
        )

    # ── Preflight checks ──────────────────────────────────────────────────────

    def _preflight_check(self) -> list[str]:
        warnings: list[str] = []

        if not self._start_dir.exists():
            warnings.append("start/ directory missing — static validation may skip file overlay")

        if not self._solution_dir.exists():
            warnings.append("solution/ directory missing — no canonical good solution to test")

        if self.schema.requires_alt_solutions:
            alt_dir = self._calibration_dir / "alt_good_solutions"
            if not alt_dir.exists() or not any(alt_dir.iterdir()):
                warnings.append(
                    "requires_alt_solutions is true but calibration/alt_good_solutions/ "
                    "is empty or missing"
                )

        bad_dir = self._calibration_dir / "bad_solutions"
        if not bad_dir.exists() or not any(bad_dir.iterdir()):
            warnings.append(
                "calibration/bad_solutions/ is empty or missing — "
                "bad_solutions_fail_rate cannot be evaluated"
            )

        return warnings

    # ── Per-track runner ──────────────────────────────────────────────────────

    def _run_track(self, track: str) -> TrackCalibrationResult:
        """Run all solution sets for a single track and evaluate criteria."""
        validation_mode = "code_only" if track == "code_only" else "hybrid"
        adapter = self.adapter if track == "hybrid" else None

        solution_runs: list[SolutionRunResult] = []

        # Canonical good solution
        if self._solution_dir.exists():
            result = self._run_solution(
                self._solution_dir, "good", "solution", validation_mode, adapter
            )
            solution_runs.append(result)

        # Bad solutions
        solution_runs.extend(
            self._run_solution_set("bad_solutions", "bad", validation_mode, adapter)
        )

        # Near-miss solutions
        solution_runs.extend(
            self._run_solution_set("near_miss_solutions", "near_miss", validation_mode, adapter)
        )

        # Alt-good solutions (only if flag is set)
        if self.schema.requires_alt_solutions:
            solution_runs.extend(
                self._run_solution_set("alt_good_solutions", "alt_good", validation_mode, adapter)
            )

        criteria = self._evaluate_criteria(solution_runs, track)
        ready = all(
            cr.passed for cr in criteria if cr.passed is not None
        )

        return TrackCalibrationResult(
            track=track,
            criteria=criteria,
            solution_runs=solution_runs,
            ready=ready,
        )

    def _run_solution_set(
        self,
        subdir: str,
        solution_set: str,
        validation_mode: str,
        adapter: Optional[UnrealRuntimeAdapter],
    ) -> list[SolutionRunResult]:
        set_dir = self._calibration_dir / subdir
        if not set_dir.exists():
            return []

        results: list[SolutionRunResult] = []
        for sol_dir in sorted(set_dir.iterdir()):
            if not sol_dir.is_dir():
                continue
            result = self._run_solution(sol_dir, solution_set, sol_dir.name, validation_mode, adapter)
            results.append(result)
        return results

    def _run_solution(
        self,
        solution_dir: Path,
        solution_set: str,
        solution_name: str,
        validation_mode: str,
        adapter: Optional[UnrealRuntimeAdapter],
    ) -> SolutionRunResult:
        """Build an isolated workspace for one solution and run validation."""
        tmp_dir: Optional[Path] = None
        try:
            tmp_dir = Path(tempfile.mkdtemp(prefix=f"calib_{solution_name}_"))
            workspace = self._build_workspace(solution_dir, tmp_dir)

            pub_reports, priv_reports = run_full_validation(
                self.schema, workspace, adapter=adapter, mode=validation_mode
            )
            passed = overall_passed(pub_reports, priv_reports)
            env_ok = self._environment_ok(pub_reports + priv_reports)

            return SolutionRunResult(
                solution_name=solution_name,
                solution_set=solution_set,
                passed=passed,
                environment_ok=env_ok,
                public_reports=pub_reports,
                private_reports=priv_reports,
            )
        except Exception as exc:
            return SolutionRunResult(
                solution_name=solution_name,
                solution_set=solution_set,
                passed=False,
                environment_ok=False,
                public_reports=[],
                private_reports=[],
                error=f"{exc}\n{traceback.format_exc()}",
            )
        finally:
            if tmp_dir is not None and tmp_dir.exists():
                shutil.rmtree(tmp_dir, ignore_errors=True)

    def _build_workspace(self, solution_dir: Path, tmp_dir: Path) -> Path:
        """Copy start/ into tmp_dir, then overlay solution files.

        Layout in tmp_dir mirrors the task workspace:
          writable_files are overlaid from solution_dir on top of start/.
        Everything else in start/ is copied as-is (read-only context).
        """
        # Copy start/ skeleton (may not exist for some tasks)
        if self._start_dir.exists():
            shutil.copytree(self._start_dir, tmp_dir, dirs_exist_ok=True)

        # Overlay solution files (only writable_files from public_contract)
        for rel_path in self.schema.writable_files:
            src = solution_dir / rel_path
            if src.exists():
                dst = tmp_dir / rel_path
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dst)
            # If solution_dir contains files matching rel_path pattern — check direct file
            else:
                # Try as direct path under solution_dir without intermediate dirs
                fname = Path(rel_path).name
                candidate = solution_dir / fname
                if candidate.exists():
                    dst = tmp_dir / rel_path
                    dst.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(candidate, dst)

        return tmp_dir

    @staticmethod
    def _environment_ok(reports: list[ValidationReport]) -> bool:
        """True if no environment-level failure (bridge/PIE/launch) occurred."""
        for r in reports:
            if r.environment_health is not None:
                eh = r.environment_health
                if eh.unreal_launch_ok is False:
                    return False
                if eh.bridge_connect_ok is False:
                    return False
                if eh.pie_start_ok is False:
                    return False
        return True

    # ── Criteria evaluation ───────────────────────────────────────────────────

    def _evaluate_criteria(
        self,
        runs: list[SolutionRunResult],
        track: str,
    ) -> list[CriterionResult]:
        criteria: list[CriterionResult] = []

        # C1: good_solutions_pass_rate
        good_runs = [r for r in runs if r.solution_set in ("good",)]
        criteria.append(self._rate_criterion(
            "good_solutions_pass_rate",
            numerator=sum(r.passed for r in good_runs if r.environment_ok),
            denominator=len([r for r in good_runs if r.environment_ok]),
            threshold=GOOD_PASS_THRESHOLD,
            higher_is_better=True,
        ))

        # C2: bad_solutions_fail_rate
        bad_runs = [r for r in runs if r.solution_set == "bad"]
        criteria.append(self._rate_criterion(
            "bad_solutions_fail_rate",
            numerator=sum(not r.passed for r in bad_runs if r.environment_ok),
            denominator=len([r for r in bad_runs if r.environment_ok]),
            threshold=BAD_FAIL_THRESHOLD,
            higher_is_better=True,
        ))

        # C3: near_miss_fail_rate
        near_runs = [r for r in runs if r.solution_set == "near_miss"]
        criteria.append(self._rate_criterion(
            "near_miss_fail_rate",
            numerator=sum(not r.passed for r in near_runs if r.environment_ok),
            denominator=len([r for r in near_runs if r.environment_ok]),
            threshold=NEAR_MISS_FAIL_THRESHOLD,
            higher_is_better=True,
        ))

        # C4: alt_good_pass_rate (only if flag is set)
        if self.schema.requires_alt_solutions:
            alt_runs = [r for r in runs if r.solution_set == "alt_good"]
            criteria.append(self._rate_criterion(
                "alt_good_pass_rate",
                numerator=sum(r.passed for r in alt_runs if r.environment_ok),
                denominator=len([r for r in alt_runs if r.environment_ok]),
                threshold=ALT_GOOD_PASS_THRESHOLD,
                higher_is_better=True,
            ))
        else:
            criteria.append(CriterionResult(
                criterion="alt_good_pass_rate",
                passed=None,
                value=None,
                threshold=None,
                detail="skipped — requires_alt_solutions is False",
            ))

        # C5: environment_failure_rate (inverted — lower is better)
        all_runs = [r for r in runs if not r.error]
        env_fail_count = sum(1 for r in all_runs if not r.environment_ok)
        criteria.append(self._rate_criterion(
            "environment_failure_rate",
            numerator=env_fail_count,
            denominator=len(all_runs),
            threshold=ENV_FAIL_THRESHOLD,
            higher_is_better=False,
        ))

        return criteria

    @staticmethod
    def _rate_criterion(
        criterion: str,
        numerator: int,
        denominator: int,
        threshold: float,
        higher_is_better: bool,
    ) -> CriterionResult:
        if denominator == 0:
            return CriterionResult(
                criterion=criterion,
                passed=None,
                value=None,
                threshold=threshold,
                detail="skipped — no applicable solutions found",
            )

        value = numerator / denominator
        if higher_is_better:
            passed = value >= threshold
        else:
            passed = value <= threshold

        direction = ">=" if higher_is_better else "<="
        return CriterionResult(
            criterion=criterion,
            passed=passed,
            value=round(value, 4),
            threshold=threshold,
            detail=f"{value:.1%} {direction} {threshold:.1%} → {'PASS' if passed else 'FAIL'}",
        )
