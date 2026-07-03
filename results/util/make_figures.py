#!/usr/bin/env python3
"""UnrealBench paper figures — resilient, rerunnable, gorgeous.

Reads the canonical merged snapshot directory `tasks_unreal/test_result`, keeps
only the *latest* run for every (model-config, task) pair, and renders a full
set of publication-grade figures (PDF + PNG) plus audit CSVs.

    python results/util/make_figures.py

The script is built to be run repeatedly as more S3 worker results land. It is
resilient to *heavy* amounts of missing data:

  * Missing / corrupt `result.json`            -> that run is skipped (warned).
  * A planned model with zero runs             -> shown as a "pending" column.
  * A model present for only some tasks         -> denominators use present tasks
                                                  only; missing task IDs reported.
  * Fields absent for some agents (tokens, cost,
    steps are currently agent-specific)         -> those figures plot only the
                                                  series that actually have data.
  * A brand-new model/agent never seen before   -> auto-discovered and appended
                                                  so it appears the moment results
                                                  show up (no config edit needed).
  * Any single figure raising an error          -> logged, the rest still render.

The planned solver roster lives in `results/util/model_roster.json`. Edit it to
rename, recolor, reorder, or add planned configs.

Outputs (to --out-dir, default paper/figures/):
  Figures (each as .pdf and .png)
    01_pass_rate ................... headline pass rate
    02_pass_rate_breakdown ......... raw / judge-adjusted / requirement / test
    03_task_outcome_matrix ......... model x task pass/fail/missing heatmap
    04_pass_rate_by_tier ........... grouped bars by difficulty tier
    05_pass_rate_by_domain ......... heatmap by game-system domain/family
    06_pass_rate_by_category ....... heatmap by requirement category (top-N)
    07_time_vs_pass_rate ........... efficiency frontier (median solve time)
    08_cost_vs_pass_rate ........... cost Pareto frontier (as cost data lands)
    09_output_tokens ............... mean output tokens per solved/all task
    10_pass_rate_vs_steps .......... pass rate as a function of agent steps
    11_failure_modes ............... stacked outcome decomposition
    12_task_difficulty_curve ....... tasks ranked by how many models solve them
    13_solved_by_n_models .......... histogram of per-task solve breadth
    14_model_complementarity ....... pairwise "solves that the other misses"
    15_coverage_progress ........... benchmark completion tracker
  CSVs
    summary.csv, task_matrix.csv, per_run.csv
  figure_index.md  (caption-ready manifest)
"""

from __future__ import annotations

import argparse
import csv
import fnmatch
import json
import math
import re
import sys
import traceback
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Callable, Iterable, Optional

try:
    import yaml
except Exception:  # pragma: no cover
    yaml = None

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib import colors as mcolors
from matplotlib.patches import Patch
from matplotlib.ticker import MultipleLocator, PercentFormatter

# ----------------------------------------------------------------------------
# Paths & constants
# ----------------------------------------------------------------------------

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent  # results/util -> results -> repo root
DEFAULT_RESULTS = REPO / "tasks_unreal" / "test_result"
DEFAULT_TASKS = REPO / "tasks_unreal"
# Plain runs write the full dashboard here. The paper's calibrated, 6-config
# figure set is generated separately into paper/figures via the explicit command
# documented in results/util/paper_roster.json, so a plain run never clobbers it.
DEFAULT_OUT = REPO / "results" / "figures"
DEFAULT_ROSTER = HERE / "model_roster.json"

SNAPSHOT_RE = re.compile(r"^(ue_task_\d{4})_(.+)_(\d{8}_\d{6})$")
TASKDIR_RE = re.compile(r"^(ue_task_(\d{4}))(?:_(.*))?$")

# ----------------------------------------------------------------------------
# Aesthetics — clean serif look tuned for a two-column paper.
# ----------------------------------------------------------------------------

plt.rcParams.update({
    "figure.facecolor": "white",
    "axes.facecolor": "white",
    "savefig.facecolor": "white",
    "font.family": "serif",
    "font.serif": ["Times New Roman", "Nimbus Roman", "Times", "DejaVu Serif"],
    "mathtext.fontset": "stix",
    "font.size": 9.0,
    "axes.titlesize": 9.5,
    "axes.labelsize": 9.0,
    "xtick.labelsize": 8.0,
    "ytick.labelsize": 8.0,
    "legend.fontsize": 8.0,
    "axes.linewidth": 0.8,
    "pdf.fonttype": 42,   # embed TrueType so LaTeX/Acrobat get real glyphs
    "ps.fonttype": 42,
    "savefig.dpi": 600,
    "figure.dpi": 120,
})

INK = "#23262B"        # near-black text/spine
SOFT_INK = "#5B6068"   # secondary labels
GRID = "#E7E9EC"       # gridlines
MISSING = "#EEF0F2"    # missing cell fill
PENDING = "#CFD3D8"    # pending bar fill
PASS = "#3D8C5F"       # green
FAIL = "#C9544B"       # red
DISPUTED = "#E0A03A"   # amber (judge-disputed)
COMPILE_FAIL = "#7C8088"  # gray

# Vendor identities. Reasoning variants are rendered as lightness shades.
VENDOR_COLORS = {
    "openai": "#0B8C73",
    "anthropic": "#C8623C",
    "google": "#3B6FE0",
    "qwen": "#7A3FC4",
    "kimi": "#D63A86",
    "deepseek": "#1F5FBF",
    "zhipu": "#0E9FB8",
    "_fallback": "#5A6472",
}
VENDOR_GUESS = [
    ("openai", ("codex", "gpt", "o1", "o3", "o4")),
    ("anthropic", ("claude", "opus", "sonnet", "haiku", "anthropic")),
    ("google", ("antigravity", "gemini", "google")),
    ("qwen", ("qwen",)),
    ("kimi", ("kimi", "moonshot")),
    ("deepseek", ("deepseek",)),
    ("zhipu", ("glm", "zhipu", "chatglm")),
]
# Strongest reasoning first -> darkest shade.
REASONING_RANK = {"max": 0, "xhigh": 1, "high": 2, "medium": 3, "low": 4,
                  "default": 5, "none": 6, "": 7}

TIER_ORDER = ["easy", "medium", "hard", "unspecified"]

# Headline pass definition used by the figures. "raw" = every hidden test passes
# (task_passed). "calibrated" = the reconciled LLM judge full-task verdict;
# missing full-task verdicts, including compile-fallback judge outputs, count as failures.
HEADLINE = "raw"


def headline_rows(rows: list) -> list:
    """Rows that belong in the denominator for the active headline metric."""
    return list(rows)


def is_pass(r: "Run") -> bool:
    """Whether a run counts as a pass under HEADLINE."""
    if HEADLINE == "calibrated":
        return bool(r.judge_pass)
    return bool(r.raw_pass)


def headline_count_rate(rows: list) -> tuple[int, int, Optional[float]]:
    denom = headline_rows(rows)
    if not denom:
        return (0, 0, None)
    k = sum(1 for r in denom if is_pass(r))
    return (k, len(denom), k / len(denom))


def pass_label() -> str:
    return "Calibrated pass rate" if HEADLINE == "calibrated" else "Raw task pass rate"


def pass_word() -> str:
    return "calibrated" if HEADLINE == "calibrated" else "raw"

# Coarse game-system domains derived from the task slug. First hit wins.
DOMAIN_RULES = [
    ("ALS Locomotion", ("alsxt", "als_refactored", "als ", "_als_")),
    ("Common UI / GameFeatures", ("common", "uiextension", "gameplay_message", "game_ui", "mfea", "modular_features", "ui_policy")),
    ("Online / Sessions (EOS/EIK)", ("eik", "session", "commonsession", "commonuser", "voice", "auth", "presence", "lobby")),
    ("Serialization (DataConfig)", ("dataconfig",)),
    ("Save / Persistence (SPUD)", ("spud", "persistence", "savegame")),
    ("Gaussian Splatting (Nano)", ("nano",)),
    ("Movement / Physics", ("pb_", "mover", "velocity", "braking", "crouch", "slide", "ladder", "noclip", "jump", "slope")),
    ("XR / Spatial Anchors", ("lasaa", "xr_", "anchor", "calibration")),
    ("Gameplay Ability System", ("ability", "gameplay_ability", "gameplay_effect", "attribute", "ability_system", "montage")),
    ("Weapons / Combat", ("weapon", "projectile", "ricochet", "combat", "recoil", "ranged", "ammo", "barrel")),
    ("AI / Enemies", ("zombie", "zed", "monster", "ai_controller", "corpse", "significance")),
    ("Inventory / Interaction", ("inventory", "pickup", "interaction", "trader", "treasure", "chest", "quickbar", "customization", "cosmetic", "parts")),
    ("Engine Systems / Pooling", ("pool", "actor_pooling", "data_assets", "subsystem", "sounds", "instanced", "replication_graph", "indicator", "camera")),
]


# ----------------------------------------------------------------------------
# Data structures
# ----------------------------------------------------------------------------

@dataclass(frozen=True)
class Config:
    key: str
    label: str
    agent: str
    model: str            # exact, list (json), '*', or fnmatch glob
    reasoning: str
    vendor: str
    planned: bool         # from roster (True) vs auto-discovered (False)

    def matches(self, agent: str, model: str, reasoning: str) -> bool:
        return (
            _match(self.agent, agent)
            and _match(self.model, model)
            and _match(self.reasoning, reasoning)
        )


@dataclass
class Run:
    task_num: int
    task_id: str
    snapshot: str
    agent: str
    model: str
    reasoning: str
    timestamp: str
    sort_key: tuple
    raw_pass: bool
    judge_pass: Optional[bool]
    compile_success: bool
    tests_run: int
    tests_passed: int
    req_satisfied: Optional[int]
    req_total: Optional[int]
    duration_s: float
    cost_usd: Optional[float]
    output_tokens: Optional[int]
    files_edited: int
    failure_reason: str
    path: Path
    steps: Optional[int] = None  # filled lazily from trajectory
    compile_calls: Optional[int] = None  # completed UBT builds during the solve
    compile_errors: Optional[int] = None  # compile errors hit during the solve


@dataclass
class TaskMeta:
    task_num: int
    task_id: str
    title: str
    tier: str
    n_req: int
    categories: set
    domain: str


@dataclass
class Series:
    """One model-config's selected results, indexed by task number."""
    cfg: Config
    color: str
    by_task: dict  # task_num -> Run

    @property
    def present(self) -> list:
        return [self.by_task[n] for n in sorted(self.by_task)]

    @property
    def n(self) -> int:
        return len(self.by_task)

    def rate(self, attr: str) -> Optional[float]:
        rows = self.present
        if not rows:
            return None
        good = sum(1 for r in rows if getattr(r, attr))
        return good / len(rows)

    def pass_rate(self) -> Optional[float]:
        """Pass rate under the active HEADLINE metric (raw or calibrated)."""
        _, _, rate = headline_count_rate(self.present)
        return rate


# ----------------------------------------------------------------------------
# Small helpers
# ----------------------------------------------------------------------------

def warn(msg: str) -> None:
    print(f"  [warn] {msg}", file=sys.stderr)


def _task_num_from_token(token: str) -> Optional[int]:
    token = token.strip()
    if not token:
        return None
    m = re.search(r"(\d+)", token)
    if not m:
        return None
    return int(m.group(1))


def load_excluded_tasks(paths: list[Path], inline: list[str]) -> set[int]:
    excluded: set[int] = set()
    tokens: list[str] = []
    tokens.extend(inline or [])
    for path in paths or []:
        if not path.exists():
            warn(f"exclude file not found: {path}")
            continue
        for line in path.read_text(encoding="utf-8").splitlines():
            line = line.split("#", 1)[0].replace(",", " ")
            tokens.extend(line.split())
    for token in tokens:
        n = _task_num_from_token(token)
        if n is not None:
            excluded.add(n)
    return excluded


def _match(spec, value: str) -> bool:
    """Match a roster spec against a value.

    spec may be: None or '*' (any); a list (match if ANY element matches, with
    glob support per element); or a string (fnmatch glob if it contains
    */?/[, else case-insensitive exact match).
    """
    if spec is None or spec == "*":
        return True
    if isinstance(spec, (list, tuple)):
        return any(_match(s, value) for s in spec)
    spec = str(spec)
    v = value or ""
    if "*" in spec or "?" in spec or "[" in spec:
        return fnmatch.fnmatch(v.lower(), spec.lower())
    return v.lower() == spec.lower()


def guess_vendor(agent: str, model: str) -> str:
    blob = f"{agent} {model}".lower()
    for vendor, keys in VENDOR_GUESS:
        if any(k in blob for k in keys):
            return vendor
    return "_fallback"


def shade(hex_color: str, lightness: float) -> str:
    """Blend a hex color toward white (lightness>0) or black (<0)."""
    r, g, b = mcolors.to_rgb(hex_color)
    if lightness >= 0:
        r = r + (1 - r) * lightness
        g = g + (1 - g) * lightness
        b = b + (1 - b) * lightness
    else:
        f = 1 + lightness
        r, g, b = r * f, g * f, b * f
    return mcolors.to_hex((r, g, b))




def task_num_of(task_id: str) -> int:
    m = re.search(r"(\d{4})", task_id)
    return int(m.group(1)) if m else -1


def domain_for(slug: str) -> str:
    s = (slug or "").lower()
    for name, keys in DOMAIN_RULES:
        if any(k in s for k in keys):
            return name
    return "Core Gameplay"


def fmt_pct(x: Optional[float]) -> str:
    return "n/a" if x is None or (isinstance(x, float) and math.isnan(x)) else f"{x * 100:.1f}%"


# ----------------------------------------------------------------------------
# Roster + config assignment
# ----------------------------------------------------------------------------

def load_roster(path: Path) -> list:
    if not path.exists():
        warn(f"roster file not found at {path}; using auto-discovery only")
        return []
    try:
        data = json.loads(path.read_text(encoding="utf-8-sig"))
    except Exception as exc:
        warn(f"could not parse roster {path}: {exc}; using auto-discovery only")
        return []
    items = data.get("models", data) if isinstance(data, dict) else data
    out = []
    for i, it in enumerate(items or []):
        if not isinstance(it, dict) or "agent" not in it:
            continue
        agent = str(it["agent"])
        model = it.get("model", "*")
        reasoning = it.get("reasoning", "*")
        out.append(Config(
            key=str(it.get("key") or f"cfg{i}"),
            label=str(it.get("label") or f"{agent}"),
            agent=agent,
            model=model if isinstance(model, list) else str(model),
            reasoning=reasoning if isinstance(reasoning, list) else str(reasoning),
            vendor=str(it.get("vendor") or guess_vendor(agent, str(model))),
            planned=True,
        ))
    return out


def assign_color(configs: list) -> dict:
    """Assign each config a stable color: vendor hue + reasoning lightness."""
    # Group by vendor to spread reasoning shades.
    by_vendor: dict = {}
    for c in configs:
        by_vendor.setdefault(c.vendor, []).append(c)
    colors: dict = {}
    for vendor, group in by_vendor.items():
        base = VENDOR_COLORS.get(vendor, VENDOR_COLORS["_fallback"])
        # order group by reasoning strength so darkest = strongest
        ordered = sorted(group, key=lambda c: REASONING_RANK.get(str(c.reasoning).lower(), 5))
        n = len(ordered)
        for i, c in enumerate(ordered):
            # spread lightness in [-0.08, +0.42] across variants of one vendor
            if n == 1:
                light = 0.0
            else:
                light = -0.05 + (i / (n - 1)) * 0.5
            colors[c.key] = shade(base, light)
    return colors


# ----------------------------------------------------------------------------
# Parsing result.json
# ----------------------------------------------------------------------------

def _read_json(path: Path) -> Optional[dict]:
    try:
        with path.open("r", encoding="utf-8") as f:
            d = json.load(f)
        return d if isinstance(d, dict) else None
    except Exception:
        return None


def reasoning_of(rec: dict) -> str:
    return str(rec.get("reasoning_level_applied")
               or rec.get("reasoning_level_requested")
               or "default")


def requirement_satisfaction(judge: dict) -> tuple:
    """(satisfied, total) from the judge's full-task audit, else (None, None)."""
    if not isinstance(judge, dict):
        return (None, None)
    fa = judge.get("full_task_audit") or {}
    ers = fa.get("explicit_requirement_satisfaction")
    if isinstance(ers, dict) and ers:
        vals = [v for v in ers.values() if isinstance(v, dict)]
        if vals:
            sat = sum(1 for v in vals if v.get("satisfied") is True)
            return (sat, len(vals))
    return (None, None)


def judge_verdict(judge: dict) -> Optional[bool]:
    """Did the reconciled judge conclude the submission satisfies the whole task?"""
    if not isinstance(judge, dict):
        return None
    if "submission_satisfies_full_task" in judge:
        value = judge["submission_satisfies_full_task"]
        return bool(value) if value is not None else None
    fa = judge.get("full_task_audit") or {}
    if isinstance(fa, dict) and "submission_satisfies_full_task" in fa:
        value = fa["submission_satisfies_full_task"]
        return bool(value) if value is not None else None
    if judge.get("mode") == "dense" and "submission_verdict" in judge:
        return judge.get("submission_verdict") == "CORRECT"
    return None

def _numeric(value) -> Optional[float]:
    if isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value):
        return float(value)
    return None


def _imputed_field(rec: dict, field: str) -> Optional[float]:
    imputed = rec.get("imputed_metrics") or {}
    fields = imputed.get("fields") if isinstance(imputed, dict) else None
    if not isinstance(fields, dict):
        return None
    entry = fields.get(field)
    if isinstance(entry, dict):
        return _numeric(entry.get("value"))
    return None


def _imputed_token(rec: dict, token_field: str) -> Optional[float]:
    imputed = rec.get("imputed_metrics") or {}
    fields = imputed.get("fields") if isinstance(imputed, dict) else None
    token_usage = fields.get("token_usage") if isinstance(fields, dict) else None
    if not isinstance(token_usage, dict):
        return None
    entry = token_usage.get(token_field)
    if isinstance(entry, dict):
        return _numeric(entry.get("value"))
    return None

def parse_run(snap_dir: Path) -> Optional[Run]:
    rec = _read_json(snap_dir / "result.json")
    if not rec:
        return None
    m = SNAPSHOT_RE.match(snap_dir.name)
    task_id = str(rec.get("task_id") or (m.group(1) if m else ""))
    if not task_id.startswith("ue_task_"):
        return None
    tnum = task_num_of(task_id)
    if tnum < 0:
        return None

    # A run that edited nothing and produced no evaluable benchmark record never
    # really executed -> drop it. Keep compile-fallback records when a judge
    # block exists: those are valid benchmark outcomes and must remain in
    # denominators, even if the solver edited no files before failing compile.
    files = rec.get("files_edited") or []
    if isinstance(files, list) and len(files) == 0 and not rec.get("compile_success"):
        # Keep genuine compile failures that still touched files or were judged;
        # drop empty no-ops with neither tests nor judge evidence.
        if not rec.get("tests_run") and not rec.get("judge"):
            return None
    judge = rec.get("judge") or {}
    req_sat, req_tot = requirement_satisfaction(judge)

    tu = rec.get("token_usage") or {}
    out_tok = tu.get("output_tokens") if isinstance(tu, dict) else None
    out_tok = _numeric(out_tok)
    if out_tok is None:
        out_tok = _imputed_token(rec, "output_tokens")

    cost = _numeric(rec.get("cost_usd"))
    if not cost:
        cost = _imputed_field(rec, "cost_usd")

    steps = _numeric(rec.get("agent_steps"))
    if steps is None:
        steps = _imputed_field(rec, "agent_steps")
    ts = str(rec.get("timestamp") or "")
    # tiebreak with the snapshot's own timestamp suffix, then dir name
    snap_ts = m.group(3) if m else ""
    sort_key = (ts, snap_ts, snap_dir.name)

    return Run(
        task_num=tnum,
        task_id=task_id,
        snapshot=snap_dir.name,
        agent=str(rec.get("agent") or (m.group(2) if m else "")),
        model=str(rec.get("model") or ""),
        reasoning=reasoning_of(rec),
        timestamp=ts,
        sort_key=sort_key,
        raw_pass=bool(rec.get("task_passed")),
        judge_pass=judge_verdict(judge),
        compile_success=bool(rec.get("compile_success")),
        tests_run=int(rec.get("tests_run") or 0),
        tests_passed=int(rec.get("tests_passed") or 0),
        req_satisfied=req_sat,
        req_total=req_tot,
        duration_s=float(rec.get("duration_seconds") or 0.0),
        cost_usd=cost,
        output_tokens=int(out_tok) if out_tok is not None else None,
        files_edited=len(files) if isinstance(files, list) else 0,
        steps=int(round(steps)) if steps is not None else None,
        failure_reason=str(rec.get("failure_reason") or ("" if rec.get("task_passed") else "unknown")),
        path=snap_dir,
    )


def collect_runs(results_root: Path, lo: int, hi: int, excluded: Optional[set[int]] = None) -> list:
    runs = []
    if not results_root.exists():
        warn(f"results root {results_root} does not exist")
        return runs
    skipped = 0
    for snap in sorted(p for p in results_root.iterdir() if p.is_dir()):
        try:
            r = parse_run(snap)
        except Exception as exc:
            warn(f"failed to parse {snap.name}: {exc}")
            r = None
        if r is None:
            skipped += 1
            continue
        if lo <= r.task_num <= hi and r.task_num not in (excluded or set()):
            runs.append(r)
    if skipped:
        print(f"  parsed {len(runs)} runs, skipped {skipped} (no/invalid result.json)")
    return runs


# ----------------------------------------------------------------------------
# Step extraction (best effort, agent specific, only for selected runs)
# ----------------------------------------------------------------------------

def _read_text(path: Path, cap: int = 12_000_000) -> str:
    try:
        with path.open("r", encoding="utf-8", errors="replace") as f:
            return f.read(cap)
    except Exception:
        return ""


# Unreal Build Tool emits one "Result: Succeeded|Failed" line per completed
# build; compiler errors show up as MSVC codes (C2065...), clang ": error:", or
# fatal errors. These are agent-agnostic because they come from UBT/compiler
# output captured in the trajectory, not from agent-specific formatting.
COMPILE_CALL_RE = re.compile(r"Result:\s*(?:Succeeded|Failed)")
COMPILE_ERR_RE = re.compile(r"error\s+[A-Z]{1,2}\d{3,}|:\s*error:|:\s*fatal error", re.I)
BUILD_MARKERS = ("UnrealBuildTool", "Build.bat", "BuildCookRun", "Result: Succeeded",
                 "Result: Failed")


def extract_trajectory_metrics(run: Run) -> tuple:
    """Best-effort (steps, compile_calls, compile_errors) from the trajectory.

    Any element may be None when the trajectory does not capture that signal
    (e.g. agents that emit no structured transcript, or no build output).
    """
    log = run.path / "agent_trajectory.log"
    if not log.exists():
        return (None, None, None)
    text = _read_text(log)
    if not text:
        return (None, None, None)
    agent = run.agent.lower()

    steps = None
    if "claude" in agent:
        steps = text.count("ToolUseBlock(") or None
    elif "codex" in agent:
        steps = len(re.findall(r"(?m)^codex\b", text)) or None

    # Compile metrics only when the trajectory actually captured build output.
    if not any(marker in text for marker in BUILD_MARKERS):
        return (steps, None, None)
    calls = len(COMPILE_CALL_RE.findall(text))
    errors = len(COMPILE_ERR_RE.findall(text))
    return (steps, calls, errors)


def fill_trajectory_metrics(series_list: list) -> int:
    filled = 0
    for s in series_list:
        for r in s.by_task.values():
            if r.steps is None or r.compile_calls is None:
                steps, compile_calls, compile_errors = extract_trajectory_metrics(r)
                changed = False
                if r.steps is None and steps is not None:
                    r.steps = steps
                    changed = True
                if r.compile_calls is None and compile_calls is not None:
                    r.compile_calls = compile_calls
                    r.compile_errors = compile_errors
                    changed = True
                if changed:
                    filled += 1
    return filled

# ----------------------------------------------------------------------------
# Task metadata
# ----------------------------------------------------------------------------

def collect_task_meta(tasks_dir: Path, lo: int, hi: int, excluded: Optional[set[int]] = None) -> dict:
    meta: dict = {}
    if not tasks_dir.exists():
        return meta
    for td in sorted(tasks_dir.glob("ue_task_*")):
        if not td.is_dir():
            continue
        m = TASKDIR_RE.match(td.name)
        if not m:
            continue
        tnum = int(m.group(2))
        if not (lo <= tnum <= hi) or tnum in (excluded or set()):
            continue
        slug = m.group(3) or ""
        title = slug.replace("_", " ").title() if slug else m.group(1)
        tier = "unspecified"
        n_req = 0
        cats: set = set()
        spec = td / "spec.yaml"
        if yaml is not None and spec.exists():
            try:
                s = yaml.safe_load(spec.read_text(encoding="utf-8")) or {}
                title = str(s.get("title") or title)
                tier = str(s.get("tier") or "unspecified")
                for key in ("hidden_requirements", "test_requirements"):
                    reqs = s.get(key) or {}
                    if isinstance(reqs, dict):
                        for rv in reqs.values():
                            if isinstance(rv, dict) and rv.get("category"):
                                cats.add(str(rv["category"]))
                        if key == "hidden_requirements":
                            n_req = len(reqs)
            except Exception as exc:
                warn(f"spec parse failed for {td.name}: {exc}")
        meta[tnum] = TaskMeta(
            task_num=tnum, task_id=m.group(1), title=title, tier=tier,
            n_req=n_req, categories=cats or {"unspecified"}, domain=domain_for(slug),
        )
    return meta


# ----------------------------------------------------------------------------
# Build series (assign runs to configs, select latest)
# ----------------------------------------------------------------------------

def build_series(runs: list, roster: list, include_auto: bool) -> list:
    # First-match-wins assignment to roster configs.
    assigned: dict = {c.key: {} for c in roster}
    unmatched: list = []
    for r in runs:
        hit = None
        for c in roster:
            if c.matches(r.agent, r.model, r.reasoning):
                hit = c
                break
        if hit is None:
            unmatched.append(r)
            continue
        bucket = assigned[hit.key]
        prev = bucket.get(r.task_num)
        if prev is None or r.sort_key > prev.sort_key:
            bucket[r.task_num] = r

    configs = list(roster)

    # Auto-discover any (agent, model, reasoning) the roster didn't claim.
    if include_auto and unmatched:
        groups: dict = {}
        for r in unmatched:
            groups.setdefault((r.agent, r.model, r.reasoning), {})
        for r in unmatched:
            bucket = groups[(r.agent, r.model, r.reasoning)]
            prev = bucket.get(r.task_num)
            if prev is None or r.sort_key > prev.sort_key:
                bucket[r.task_num] = r
        for (agent, model, reasoning), bucket in sorted(groups.items()):
            rl = "" if reasoning in ("default", "") else f" ({reasoning})"
            label = f"{model or agent}{rl}"
            cfg = Config(
                key=f"auto:{agent}:{model}:{reasoning}",
                label=label, agent=agent, model=model, reasoning=reasoning,
                vendor=guess_vendor(agent, model), planned=False,
            )
            configs.append(cfg)
            assigned[cfg.key] = bucket
            print(f"  auto-discovered config: {label}  [{agent}/{model}/{reasoning}] "
                  f"({len(bucket)} tasks)")

    colors = assign_color(configs)
    series = [Series(cfg=c, color=colors.get(c.key, VENDOR_COLORS["_fallback"]),
                     by_task=assigned.get(c.key, {})) for c in configs]
    return series


def order_for_plot(series_list: list, present_only: bool) -> list:
    """Stable, readable ordering: present configs by pass rate desc, then pending."""
    present = [s for s in series_list if s.n > 0]
    pending = [s for s in series_list if s.n == 0]
    present.sort(key=lambda s: (-(s.pass_rate() or 0), s.cfg.label))
    if present_only:
        return present
    return present + pending


# ----------------------------------------------------------------------------
# Figure scaffolding
# ----------------------------------------------------------------------------

def save(fig, out_dir: Path, stem: str) -> None:
    fig.savefig(out_dir / f"{stem}.pdf", bbox_inches="tight", pad_inches=0.03)
    fig.savefig(out_dir / f"{stem}.png", dpi=300, bbox_inches="tight", pad_inches=0.03)
    plt.close(fig)


def style_axes(ax, hide=("top", "right", "left")) -> None:
    for sp in hide:
        ax.spines[sp].set_visible(False)
    for sp in ax.spines.values():
        if sp.get_visible():
            sp.set_color("#BFC4CA")
    ax.tick_params(length=0, colors=SOFT_INK)
    ax.set_axisbelow(True)


def wrap_label(text: str, width: int = 16) -> str:
    words = text.split()
    lines, cur = [], ""
    for w in words:
        if len(cur) + len(w) + 1 > width and cur:
            lines.append(cur)
            cur = w
        else:
            cur = f"{cur} {w}".strip()
    if cur:
        lines.append(cur)
    return "\n".join(lines)


# ----------------------------------------------------------------------------
# Figures
# ----------------------------------------------------------------------------

def fig_pass_rate(series_list, out_dir, lo, hi):
    order = order_for_plot(series_list, present_only=False)
    if not order:
        return
    labels = [s.cfg.label for s in order]
    fig_w = max(6.4, len(order) * 0.92)
    fig, ax = plt.subplots(figsize=(fig_w, 3.3))

    xs = range(len(order))
    maxrate = 0.0
    for i, s in enumerate(order):
        if s.n == 0:
            ax.bar(i, 0, color=PENDING, width=0.66)
            ax.text(i, 0.012, "pending", ha="center", va="bottom",
                    fontsize=7.4, color=SOFT_INK, rotation=90)
            continue
        k, denom, _ = headline_count_rate(s.present)
        if denom == 0:
            ax.text(i, 0.025, "no judge", ha="center", va="bottom", fontsize=7.0, color=SOFT_INK, rotation=90)
            continue
        p = k / denom
        maxrate = max(maxrate, p)
        ax.bar(i, p, color=s.color, width=0.66, zorder=3)
        ax.text(i, p + 0.012, f"{k}/{denom}\n{p*100:.1f}%", ha="center",
                va="bottom", fontsize=7.4, color=INK, linespacing=0.95)

    ax.set_ylabel(pass_label())
    ax.set_ylim(0, max(0.35, maxrate + 0.09))
    ax.yaxis.set_major_formatter(PercentFormatter(1.0))
    ax.yaxis.set_major_locator(MultipleLocator(0.1))
    ax.grid(axis="y", color=GRID, linewidth=0.7)
    ax.set_xticks(list(xs))
    if len(order) > 8:
        ax.set_xticklabels([wrap_label(l, 18) for l in labels], rotation=22, ha="right")
    else:
        ax.set_xticklabels([wrap_label(l, 14) for l in labels], rotation=0)
    ax.tick_params(axis="x", labelsize=7.6)
    style_axes(ax)
    ax.set_title(f"UnrealBench - tasks {lo}-{hi}",
                 color=SOFT_INK, fontsize=8.5, pad=8)
    save(fig, out_dir, "01_pass_rate")


def fig_pass_rate_breakdown(series_list, out_dir):
    order = order_for_plot(series_list, present_only=True)
    if not order:
        return
    metrics = [
        ("Raw (all tests pass)", lambda s: s.rate("raw_pass")),
        ("Judge-adjusted", lambda s: _judge_rate(s)),
        ("Requirements satisfied", lambda s: _req_rate(s)),
        ("Test-level (micro)", lambda s: _test_rate(s)),
    ]
    mcolors_ = ["#3D6FB0", "#3D8C5F", "#9A6FB0", "#C8893C"]
    labels = [s.cfg.label for s in order]
    x = range(len(order))
    width = 0.2
    fig, ax = plt.subplots(figsize=(max(6.6, len(order) * 1.05), 3.5))
    for j, (name, fn) in enumerate(metrics):
        vals = [fn(s) or 0 for s in order]
        offs = [i + (j - 1.5) * width for i in x]
        ax.bar(offs, vals, width, label=name, color=mcolors_[j], zorder=3)
    ax.set_ylabel("Score")
    ax.set_ylim(0, 1.0)
    ax.yaxis.set_major_formatter(PercentFormatter(1.0))
    ax.grid(axis="y", color=GRID, linewidth=0.7)
    ax.set_xticks(list(x))
    ax.set_xticklabels([wrap_label(l, 14) for l in labels], fontsize=7.6)
    style_axes(ax)
    ax.legend(frameon=False, loc="upper center", bbox_to_anchor=(0.5, -0.16),
              ncols=4, fontsize=7.6)
    save(fig, out_dir, "02_pass_rate_breakdown")


def _judge_rate(s):
    rows = [r for r in s.present if r.judge_pass is not None]
    return (sum(1 for r in rows if r.judge_pass) / len(rows)) if rows else None


def _req_rate(s):
    rows = [r for r in s.present if r.req_total]
    if not rows:
        return None
    return sum(r.req_satisfied / r.req_total for r in rows) / len(rows)


def _test_rate(s):
    run = sum(r.tests_run for r in s.present)
    pas = sum(r.tests_passed for r in s.present)
    return (pas / run) if run else None


def fig_task_matrix(series_list, out_dir, lo, hi, excluded=None):
    order = order_for_plot(series_list, present_only=True)
    if not order:
        return
    excluded = excluded or set()
    task_nums = [n for n in range(lo, hi + 1) if n not in excluded]
    grid = []
    for s in order:
        row = []
        for n in task_nums:
            r = s.by_task.get(n)
            row.append(0 if r is None else (2 if is_pass(r) else 1))
        grid.append(row)
    cmap = mcolors.ListedColormap([MISSING, FAIL, PASS])
    norm = mcolors.BoundaryNorm([-0.5, 0.5, 1.5, 2.5], cmap.N)
    width = max(7.0, len(task_nums) * 0.10)
    height = max(2.0, len(order) * 0.36 + 0.9)
    fig, ax = plt.subplots(figsize=(width, height))
    ax.imshow(grid, aspect="auto", cmap=cmap, norm=norm, interpolation="nearest")
    ax.set_yticks(range(len(order)))
    ax.set_yticklabels([s.cfg.label for s in order], color=INK, fontsize=8)
    tick_ns = []
    if task_nums:
        tick_ns = [task_nums[0]] + [n for n in task_nums[1:-1] if n % 10 == 0]
        if task_nums[-1] not in tick_ns and all(abs(task_nums[-1] - t) >= 3 for t in tick_ns):
            tick_ns.append(task_nums[-1])
    ticks = [task_nums.index(n) for n in tick_ns]
    ax.set_xticks(ticks)
    ax.set_xticklabels([str(n) for n in tick_ns], color=SOFT_INK)
    ax.set_xlabel("Task ID")
    ax.tick_params(length=0)
    for sp in ax.spines.values():
        sp.set_visible(False)
    ax.legend(handles=[Patch(facecolor=PASS, label="Pass"),
                       Patch(facecolor=FAIL, label="Fail"),
                       Patch(facecolor=MISSING, label="No run yet")],
              frameon=False, loc="upper center", bbox_to_anchor=(0.5, -0.22 / (height/2.6)),
              ncols=3)
    save(fig, out_dir, "03_task_outcome_matrix")


def fig_pass_rate_by_tier(series_list, meta, out_dir):
    order = order_for_plot(series_list, present_only=True)
    if not order or not meta:
        return
    tiers = sorted({m.tier for m in meta.values()},
                   key=lambda t: TIER_ORDER.index(t) if t in TIER_ORDER else 9)
    x = range(len(tiers))
    width = min(0.8 / max(1, len(order)), 0.22)
    fig, ax = plt.subplots(figsize=(max(5.6, len(tiers) * 1.5), 3.3))
    for i, s in enumerate(order):
        vals, anns = [], []
        for t in tiers:
            tnums = [n for n, mm in meta.items() if mm.tier == t]
            rows = headline_rows([s.by_task[n] for n in tnums if n in s.by_task])
            if rows:
                k = sum(1 for r in rows if is_pass(r))
                vals.append(k / len(rows))
                anns.append(f"{k}/{len(rows)}")
            else:
                vals.append(0)
                anns.append("")
        offs = [p + (i - (len(order) - 1) / 2) * width for p in x]
        bars = ax.bar(offs, vals, width, label=s.cfg.label, color=s.color, zorder=3)
        for b, a in zip(bars, anns):
            if a:
                ax.text(b.get_x() + b.get_width() / 2, b.get_height() + 0.012, a,
                        ha="center", va="bottom", fontsize=6.2, color=SOFT_INK)
    ax.set_ylabel(pass_label())
    ax.set_xticks(list(x))
    ax.set_xticklabels([f"{t.title()}\n(n={sum(1 for m in meta.values() if m.tier==t)})"
                        for t in tiers])
    ax.set_ylim(0, 1.0)
    ax.yaxis.set_major_formatter(PercentFormatter(1.0))
    ax.grid(axis="y", color=GRID, linewidth=0.7)
    style_axes(ax)
    ax.legend(frameon=False, loc="upper center", bbox_to_anchor=(0.5, -0.13),
              ncols=min(4, len(order)), fontsize=7.4)
    save(fig, out_dir, "04_pass_rate_by_tier")


def _heatmap_by_group(series_list, out_dir, stem, groups, group_label):
    """groups: list of (name, set_of_task_nums). Generic pass-rate heatmap."""
    order = order_for_plot(series_list, present_only=True)
    if not order or not groups:
        return
    M, A = [], []
    for name, tnums in groups:
        row, ann = [], []
        for s in order:
            rows = headline_rows([s.by_task[n] for n in tnums if n in s.by_task])
            if rows:
                k = sum(1 for r in rows if is_pass(r))
                row.append(k / len(rows) * 100)
                ann.append(f"{k}/{len(rows)}")
            else:
                row.append(math.nan)
                ann.append("")
        M.append(row)
        A.append(ann)
    cmap = plt.get_cmap("YlGnBu").copy()
    cmap.set_bad(MISSING)
    fig, ax = plt.subplots(figsize=(max(5.4, len(order) * 1.15),
                                    max(2.6, len(groups) * 0.42 + 0.7)))
    im = ax.imshow(M, aspect="auto", vmin=0, vmax=100, cmap=cmap)
    ax.set_xticks(range(len(order)))
    ax.set_xticklabels([s.cfg.label for s in order], rotation=26, ha="right", fontsize=7.6)
    ax.set_yticks(range(len(groups)))
    ax.set_yticklabels([f"{n}  (n={len(t)})" for n, t in groups], fontsize=7.8)
    ax.tick_params(length=0)
    for y, row in enumerate(M):
        for xx, v in enumerate(row):
            if isinstance(v, float) and math.isnan(v):
                continue
            ax.text(xx, y, A[y][xx], ha="center", va="center", fontsize=6.6,
                    color="#1A1A1A" if v < 60 else "white")
    for sp in ax.spines.values():
        sp.set_visible(False)
    cb = fig.colorbar(im, ax=ax, fraction=0.025, pad=0.02)
    cb.set_label("Pass rate (%)", fontsize=7.6, color=SOFT_INK)
    cb.ax.tick_params(length=0, labelsize=7)
    ax.set_title(group_label, color=SOFT_INK, fontsize=8.5, pad=6)
    save(fig, out_dir, stem)


def fig_pass_rate_by_domain(series_list, meta, out_dir):
    dom: dict = {}
    for m in meta.values():
        dom.setdefault(m.domain, set()).add(m.task_num)
    groups = sorted(dom.items(), key=lambda kv: (-len(kv[1]), kv[0]))
    _heatmap_by_group(series_list, out_dir, "05_pass_rate_by_domain",
                      groups, "Pass rate by game-system domain")


def fig_pass_rate_by_category(series_list, meta, out_dir, top_n=14):
    cat: dict = {}
    for m in meta.values():
        for c in m.categories:
            cat.setdefault(c, set()).add(m.task_num)
    groups = sorted(cat.items(), key=lambda kv: (-len(kv[1]), kv[0]))[:top_n]
    groups = [(c.replace("_", " ").title(), t) for c, t in groups]
    _heatmap_by_group(series_list, out_dir, "06_pass_rate_by_category",
                      groups, f"Pass rate by requirement category (top {top_n})")


def _scatter_metric_vs_pass(series_list, out_dir, stem, xfn, xlabel, title, log_x=False):
    order = order_for_plot(series_list, present_only=True)
    pts = [(s, xfn(s)) for s in order]
    pts = [(s, x) for s, x in pts if x is not None and not (isinstance(x, float) and math.isnan(x))]
    if not pts:
        return
    fig, ax = plt.subplots(figsize=(5.4, 3.5))
    for s, x in pts:
        y = (s.pass_rate() or 0) * 100
        ax.scatter(x, y, s=42 + s.n * 1.2, color=s.color, edgecolor="white",
                   linewidth=0.9, zorder=3)
        ax.annotate(wrap_label(s.cfg.label, 18), (x, y), xytext=(6, 4),
                    textcoords="offset points", fontsize=7.0, color=INK)
    if log_x:
        ax.set_xscale("log")
    ax.set_xlabel(xlabel)
    ax.set_ylabel(f"{pass_label()} (%)")
    ax.grid(color=GRID, linewidth=0.7)
    style_axes(ax)
    ax.set_title(title, color=SOFT_INK, fontsize=8.5, pad=6)
    # bubble-size legend note
    ax.text(0.99, 0.02, "bubble size scales with tasks completed", transform=ax.transAxes,
            ha="right", va="bottom", fontsize=6.6, color=SOFT_INK, style="italic")
    save(fig, out_dir, stem)


def _median(xs):
    xs = sorted(xs)
    if not xs:
        return None
    n = len(xs)
    return xs[n // 2] if n % 2 else (xs[n // 2 - 1] + xs[n // 2]) / 2


def fig_time_vs_pass(series_list, out_dir):
    def xfn(s):
        ds = [r.duration_s for r in s.present if r.duration_s > 0]
        m = _median(ds)
        return m / 60.0 if m else None
    _scatter_metric_vs_pass(series_list, out_dir, "07_time_vs_pass_rate", xfn,
                            "Median solve time per task (minutes)",
                            "Speed vs. quality")


def fig_cost_vs_pass(series_list, out_dir):
    def xfn(s):
        cs = [r.cost_usd for r in s.present if r.cost_usd]
        return (sum(cs) / len(cs)) if cs else None
    has = any(xfn(s) for s in series_list if s.n)
    if not has:
        print("  [skip] 08_cost_vs_pass_rate: no cost data yet (expected to populate later)")
        return
    _scatter_metric_vs_pass(series_list, out_dir, "08_cost_vs_pass_rate", xfn,
                            "Mean cost per task (USD)",
                            "Cost vs. quality (Pareto frontier)")


def fig_output_tokens(series_list, out_dir):
    order = [s for s in order_for_plot(series_list, present_only=True)
             if any(r.output_tokens for r in s.present)]
    if not order:
        print("  [skip] 09_output_tokens: no token data yet (expected to populate later)")
        return
    labels, all_mean, pass_mean = [], [], []
    for s in order:
        toks = [r.output_tokens for r in s.present if r.output_tokens]
        ptoks = [r.output_tokens for r in s.present if r.output_tokens and is_pass(r)]
        labels.append(s.cfg.label)
        all_mean.append(sum(toks) / len(toks) / 1000 if toks else 0)
        pass_mean.append(sum(ptoks) / len(ptoks) / 1000 if ptoks else 0)
    x = range(len(order))
    width = 0.38
    fig, ax = plt.subplots(figsize=(max(5.4, len(order) * 1.05), 3.3))
    ax.bar([i - width/2 for i in x], all_mean, width, label="All tasks",
           color=[s.color for s in order], zorder=3)
    ax.bar([i + width/2 for i in x], pass_mean, width, label="Passed tasks",
           color=[shade(s.color, 0.4) for s in order], zorder=3)
    ax.set_ylabel("Mean output tokens (thousands)")
    ax.set_xticks(list(x))
    ax.set_xticklabels([wrap_label(l, 14) for l in labels], fontsize=7.6)
    ax.grid(axis="y", color=GRID, linewidth=0.7)
    style_axes(ax)
    ax.legend(frameon=False, loc="upper center", bbox_to_anchor=(0.5, -0.14),
              ncols=2, handles=[Patch(facecolor=SOFT_INK, label="All tasks"),
                                Patch(facecolor=shade(SOFT_INK, 0.4), label="Passed tasks")])
    save(fig, out_dir, "09_output_tokens")


def fig_pass_vs_steps(series_list, out_dir):
    order = [s for s in order_for_plot(series_list, present_only=True)
             if any(r.steps for r in s.present)]
    if not order:
        print("  [skip] 10_pass_rate_vs_steps: no step data extractable yet")
        return
    fig, ax = plt.subplots(figsize=(5.6, 3.6))
    drew = False
    for s in order:
        pairs = [(r.steps, is_pass(r)) for r in headline_rows(s.present) if r.steps]
        if len(pairs) < 4:
            continue
        steps_sorted = sorted(p[0] for p in pairs)
        # tercile edges by quantile
        q = len(steps_sorted)
        edges = [steps_sorted[0], steps_sorted[q // 3], steps_sorted[2 * q // 3],
                 steps_sorted[-1]]
        bins = [(edges[i], edges[i + 1]) for i in range(3)]
        xs, ys = [], []
        for bi, (a, b) in enumerate(bins):
            if bi < 2:
                inb = [p for p in pairs if a <= p[0] < b] if b > a else [p for p in pairs if p[0] == a]
            else:
                inb = [p for p in pairs if a <= p[0] <= b]
            if not inb:
                continue
            xs.append(sum(p[0] for p in inb) / len(inb))
            ys.append(sum(1 for p in inb if p[1]) / len(inb) * 100)
        if len(xs) >= 2:
            ax.plot(xs, ys, "-o", color=s.color, markersize=5, linewidth=1.6,
                    label=s.cfg.label, markeredgecolor="white", markeredgewidth=0.8)
            drew = True
    if not drew:
        plt.close(fig)
        print("  [skip] 10_pass_rate_vs_steps: not enough per-bin samples")
        return
    ax.set_xlabel("Agent steps (tool calls / turns, binned by tercile)")
    ax.set_ylabel(f"{pass_label()} (%)")
    ax.grid(color=GRID, linewidth=0.7)
    style_axes(ax)
    ax.legend(frameon=False, loc="best", fontsize=7.4)
    ax.set_title("Does more agent effort pay off?", color=SOFT_INK, fontsize=8.5, pad=6)
    save(fig, out_dir, "10_pass_rate_vs_steps")


def _per_run_mean(s: Series, attr: str) -> Optional[float]:
    vals = [getattr(r, attr) for r in s.present if getattr(r, attr) is not None]
    return (sum(vals) / len(vals)) if vals else None


def _metric_bar(series_list, out_dir, stem, attr, ylabel, title, fmt, skip_msg):
    order = [s for s in order_for_plot(series_list, present_only=True)
             if _per_run_mean(s, attr) is not None]
    if not order:
        print(f"  [skip] {stem}: {skip_msg}")
        return
    labels = [s.cfg.label for s in order]
    vals = [_per_run_mean(s, attr) for s in order]
    fig, ax = plt.subplots(figsize=(max(5.0, len(order) * 0.95), 3.2))
    bars = ax.bar(range(len(order)), vals, color=[s.color for s in order],
                  width=0.66, zorder=3)
    for b, v in zip(bars, vals):
        ax.text(b.get_x() + b.get_width() / 2, v, fmt.format(v), ha="center",
                va="bottom", fontsize=7.4, color=INK)
    ax.set_ylabel(ylabel)
    ax.set_ylim(0, (max(vals) if vals else 1) * 1.18 or 1)
    ax.set_xticks(range(len(order)))
    ax.set_xticklabels([wrap_label(l, 14) for l in labels], fontsize=7.6)
    ax.grid(axis="y", color=GRID, linewidth=0.7)
    style_axes(ax)
    ax.set_title(title, color=SOFT_INK, fontsize=8.5, pad=6)
    save(fig, out_dir, stem)


def fig_compile_calls(series_list, out_dir):
    _metric_bar(series_list, out_dir, "16_compile_calls", "compile_calls",
                "Mean compiler invocations per task",
                "How often models invoke the Unreal compiler", "{:.1f}",
                "no trajectory build data (e.g. agents without captured build output)")


def fig_compile_errors(series_list, out_dir):
    _metric_bar(series_list, out_dir, "17_compile_errors", "compile_errors",
                "Mean compile errors per task",
                "Compile errors encountered while solving", "{:.1f}",
                "no trajectory build data (e.g. agents without captured build output)")


def classify_outcome(r: Run) -> str:
    if r.raw_pass:
        return "Pass"
    if not r.compile_success:
        return "Compile fail"
    if r.judge_pass:
        return "Judge-disputed"
    return "Tests fail"


def fig_failure_modes(series_list, out_dir):
    order = order_for_plot(series_list, present_only=True)
    if not order:
        return
    modes = ["Pass", "Tests fail", "Judge-disputed", "Compile fail"]
    cmap = {"Pass": PASS, "Tests fail": FAIL, "Judge-disputed": DISPUTED,
            "Compile fail": COMPILE_FAIL}
    fig, ax = plt.subplots(figsize=(6.2, max(2.0, len(order) * 0.46)))
    y = range(len(order))
    left = [0.0] * len(order)
    for mode in modes:
        vals = []
        for s in order:
            rows = s.present
            vals.append(sum(1 for r in rows if classify_outcome(r) == mode) / len(rows) * 100
                        if rows else 0)
        ax.barh(list(y), vals, left=left, color=cmap[mode], height=0.62, label=mode, zorder=3)
        left = [a + b for a, b in zip(left, vals)]
    ax.set_yticks(list(y))
    ax.set_yticklabels([s.cfg.label for s in order], fontsize=8)
    ax.invert_yaxis()
    ax.set_xlabel("Share of completed tasks (%)")
    ax.set_xlim(0, 100)
    ax.grid(axis="x", color=GRID, linewidth=0.7)
    style_axes(ax)
    ax.legend(frameon=False, loc="upper center", bbox_to_anchor=(0.5, -0.18 / (len(order)/4 or 1)),
              ncols=4, fontsize=7.4)
    save(fig, out_dir, "11_failure_modes")


def fig_difficulty_curve(series_list, out_dir, lo, hi, excluded=None):
    order = order_for_plot(series_list, present_only=True)
    if not order:
        return
    excluded = excluded or set()
    rows = []
    for n in [x for x in range(lo, hi + 1) if x not in excluded]:
        present = [s.by_task[n] for s in order if n in s.by_task]
        if not present:
            continue
        solved = sum(1 for r in present if is_pass(r))
        rows.append((n, solved, len(present)))
    if not rows:
        return
    rows.sort(key=lambda t: (t[1] / t[2], t[1]))
    fracs = [s / a * 100 for _, s, a in rows]
    fig, ax = plt.subplots(figsize=(6.4, 3.0))
    ax.bar(range(len(rows)), fracs, color="#4C78A8", width=1.0, zorder=3)
    ax.set_xlabel("Tasks ranked easiest → hardest (by fraction of models solving)")
    ax.set_ylabel("Models solving (%)")
    ax.set_ylim(0, 100)
    ax.set_xlim(-0.5, len(rows) - 0.5)
    ax.yaxis.set_major_formatter(PercentFormatter(100))
    ax.grid(axis="y", color=GRID, linewidth=0.7)
    style_axes(ax)
    n_unsolved = sum(1 for _, s, _ in rows if s == 0)
    n_allsolved = sum(1 for _, s, a in rows if s == a)
    ax.set_title(f"Task difficulty profile — {n_unsolved} unsolved by all, "
                 f"{n_allsolved} solved by all ({len(rows)} tasks)",
                 color=SOFT_INK, fontsize=8.2, pad=6)
    save(fig, out_dir, "12_task_difficulty_curve")


def fig_solved_by_n(series_list, out_dir, lo, hi, excluded=None):
    order = order_for_plot(series_list, present_only=True)
    if not order:
        return
    excluded = excluded or set()
    counts: dict = {}
    for n in [x for x in range(lo, hi + 1) if x not in excluded]:
        present = [s.by_task[n] for s in order if n in s.by_task]
        if not present:
            continue
        solved = sum(1 for r in present if is_pass(r))
        counts[solved] = counts.get(solved, 0) + 1
    if not counts:
        return
    xs = list(range(0, len(order) + 1))
    ys = [counts.get(x, 0) for x in xs]
    fig, ax = plt.subplots(figsize=(5.0, 3.0))
    bars = ax.bar(xs, ys, color="#6A8FBF", width=0.74, zorder=3)
    for b, v in zip(bars, ys):
        if v:
            ax.text(b.get_x() + b.get_width() / 2, v + 0.4, str(v), ha="center",
                    va="bottom", fontsize=7.2, color=INK)
    ax.set_xlabel("Number of models solving a task")
    ax.set_ylabel("Tasks")
    ax.set_xticks(xs)
    ax.grid(axis="y", color=GRID, linewidth=0.7)
    style_axes(ax)
    save(fig, out_dir, "13_solved_by_n_models")


def fig_complementarity(series_list, out_dir):
    order = order_for_plot(series_list, present_only=True)
    if len(order) < 2:
        return
    n = len(order)
    M = [[math.nan] * n for _ in range(n)]
    for i, a in enumerate(order):
        for j, b in enumerate(order):
            shared = [t for t in a.by_task if t in b.by_task]
            if not shared:
                continue
            if i == j:
                M[i][j] = sum(1 for t in shared if is_pass(a.by_task[t]))
            else:
                M[i][j] = sum(1 for t in shared
                              if is_pass(a.by_task[t]) and not is_pass(b.by_task[t]))
    cmap = plt.get_cmap("Oranges").copy()
    cmap.set_bad(MISSING)
    fig, ax = plt.subplots(figsize=(max(4.6, n * 0.92), max(4.0, n * 0.82)))
    arr = [[v if not (isinstance(v, float) and math.isnan(v)) else math.nan for v in row] for row in M]
    vmax = max((v for row in arr for v in row
                if not (isinstance(v, float) and math.isnan(v))), default=1)
    im = ax.imshow(arr, cmap=cmap, vmin=0, vmax=max(1, vmax))
    short = [wrap_label(s.cfg.label, 12) for s in order]
    ax.set_xticks(range(n)); ax.set_xticklabels(short, rotation=26, ha="right", fontsize=7.0)
    ax.set_yticks(range(n)); ax.set_yticklabels([s.cfg.label for s in order], fontsize=7.4)
    for i in range(n):
        for j in range(n):
            v = arr[i][j]
            if isinstance(v, float) and math.isnan(v):
                continue
            ax.text(j, i, str(int(v)), ha="center", va="center", fontsize=7.0,
                    color="white" if v > vmax * 0.6 else "#1A1A1A",
                    fontweight="bold" if i == j else "normal")
    ax.set_xlabel("…that the column model misses")
    ax.set_ylabel("Tasks the row model solves…")
    ax.tick_params(length=0)
    for sp in ax.spines.values():
        sp.set_visible(False)
    ax.set_title("Model complementarity (diagonal = own solves)",
                 color=SOFT_INK, fontsize=8.5, pad=6)
    save(fig, out_dir, "14_model_complementarity")


def fig_coverage(series_list, out_dir, total_tasks):
    order = order_for_plot(series_list, present_only=False)
    if not order:
        return
    labels = [s.cfg.label for s in order]
    fracs = [s.n / total_tasks * 100 if total_tasks else 0 for s in order]
    fig, ax = plt.subplots(figsize=(5.6, max(2.0, len(order) * 0.42)))
    y = range(len(order))
    colors = [s.color if s.n else PENDING for s in order]
    ax.barh(list(y), fracs, color=colors, height=0.64, zorder=3)
    for yi, s in zip(y, order):
        ax.text(min(99, s.n / total_tasks * 100) + 1 if total_tasks else 1, yi,
                f"{s.n}/{total_tasks}", va="center", fontsize=7.2, color=INK)
    ax.set_yticks(list(y))
    ax.set_yticklabels(labels, fontsize=8)
    ax.invert_yaxis()
    ax.set_xlim(0, 100)
    ax.set_xlabel("Benchmark coverage (% of tasks run)")
    ax.xaxis.set_major_formatter(PercentFormatter(100))
    ax.grid(axis="x", color=GRID, linewidth=0.7)
    style_axes(ax)
    save(fig, out_dir, "15_coverage_progress")


# ----------------------------------------------------------------------------
# CSV + manifest
# ----------------------------------------------------------------------------

def write_csvs(out_dir, series_list, meta, lo, hi, excluded=None):
    excluded = excluded or set()
    expected_tasks = len([n for n in range(lo, hi + 1) if n not in excluded])
    order = order_for_plot(series_list, present_only=False)

    with (out_dir / "summary.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["key", "label", "agent", "model", "reasoning", "status",
                    "tasks_present", "raw_pass", "raw_rate", "judge_rate",
                    "requirement_rate", "test_micro_rate", "compile_rate",
                    "median_minutes", "mean_cost_usd", "mean_output_tokens",
                    "mean_compile_calls", "mean_compile_errors",
                    "missing_tasks"])
        for s in order:
            status = "pending" if s.n == 0 else ("partial" if s.n < expected_tasks else "complete")
            k = sum(1 for r in s.present if r.raw_pass)
            ds = [r.duration_s for r in s.present if r.duration_s > 0]
            cs = [r.cost_usd for r in s.present if r.cost_usd]
            tk = [r.output_tokens for r in s.present if r.output_tokens]
            cc = _per_run_mean(s, "compile_calls")
            ce = _per_run_mean(s, "compile_errors")
            missing = [n for n in range(lo, hi + 1) if n not in excluded and n not in s.by_task]
            w.writerow([
                s.cfg.key, s.cfg.label, s.cfg.agent, s.cfg.model, s.cfg.reasoning, status,
                s.n, k,
                f"{(s.rate('raw_pass') or 0):.4f}" if s.n else "",
                f"{_judge_rate(s):.4f}" if _judge_rate(s) is not None else "",
                f"{_req_rate(s):.4f}" if _req_rate(s) is not None else "",
                f"{_test_rate(s):.4f}" if _test_rate(s) is not None else "",
                f"{(s.rate('compile_success') or 0):.4f}" if s.n else "",
                f"{(_median(ds) / 60):.2f}" if ds else "",
                f"{sum(cs)/len(cs):.4f}" if cs else "",
                f"{sum(tk)/len(tk):.0f}" if tk else "",
                f"{cc:.2f}" if cc is not None else "",
                f"{ce:.2f}" if ce is not None else "",
                " ".join(f"{n:04d}" for n in missing),
            ])

    with (out_dir / "task_matrix.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        present = [s for s in order if s.n]
        w.writerow(["task_id", "title", "tier", "domain"] + [s.cfg.label for s in present])
        for n in [x for x in range(lo, hi + 1) if x not in excluded]:
            mm = meta.get(n)
            base = [f"ue_task_{n:04d}",
                    mm.title if mm else "", mm.tier if mm else "", mm.domain if mm else ""]
            for s in present:
                r = s.by_task.get(n)
                base.append("" if r is None else ("PASS" if is_pass(r) else "FAIL"))
            w.writerow(base)

    with (out_dir / "per_run.csv").open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["task_id", "config", "agent", "model", "reasoning", "snapshot",
                    "raw_pass", "judge_pass", "compile", "tests_passed", "tests_run",
                    "req_satisfied", "req_total", "duration_s", "cost_usd",
                    "output_tokens", "steps", "compile_calls", "compile_errors",
                    "files_edited"])
        for s in order:
            for n in sorted(s.by_task):
                r = s.by_task[n]
                w.writerow([r.task_id, s.cfg.label, r.agent, r.model, r.reasoning,
                            r.snapshot, int(r.raw_pass),
                            "" if r.judge_pass is None else int(r.judge_pass),
                            int(r.compile_success), r.tests_passed, r.tests_run,
                            r.req_satisfied if r.req_satisfied is not None else "",
                            r.req_total if r.req_total is not None else "",
                            f"{r.duration_s:.1f}", r.cost_usd if r.cost_usd else "",
                            r.output_tokens if r.output_tokens else "",
                            r.steps if r.steps is not None else "",
                            r.compile_calls if r.compile_calls is not None else "",
                            r.compile_errors if r.compile_errors is not None else "",
                            r.files_edited])


FIGURE_CAPTIONS = [
    ("01_pass_rate", "Headline pass rate per model under the selected scoring lens."),
    ("02_pass_rate_breakdown", "Four scoring lenses side by side: raw all-tests-pass, judge-adjusted full-task verdict, mean fraction of requirements satisfied, and micro-averaged test pass rate."),
    ("03_task_outcome_matrix", "Per-task outcome matrix (green pass, red fail, blank not-yet-run) across all 120 tasks."),
    ("04_pass_rate_by_tier", "Pass rate split by task difficulty tier."),
    ("05_pass_rate_by_domain", "Pass rate by game-system domain (ALS, GAS, Common UI, SPUD, Nanite/Gaussian, …)."),
    ("06_pass_rate_by_category", "Pass rate by requirement category (top categories by task count)."),
    ("07_time_vs_pass_rate", "Efficiency frontier: median per-task solve time vs. pass rate; bubble size scales with tasks completed."),
    ("08_cost_vs_pass_rate", "Cost frontier: mean per-task USD cost vs. pass rate (populates as cost data lands for every agent)."),
    ("09_output_tokens", "Mean output tokens spent per task, over all tasks vs. only passed tasks."),
    ("10_pass_rate_vs_steps", "Pass rate as a function of agent steps (tool calls/turns), binned by tercile."),
    ("11_failure_modes", "Outcome decomposition: pass, tests-fail, judge-disputed (tests fail but judge deems satisfied), compile-fail."),
    ("12_task_difficulty_curve", "Tasks ranked by the fraction of models that solve them — the benchmark's difficulty profile."),
    ("13_solved_by_n_models", "Distribution of per-task solve breadth (how many models solve each task)."),
    ("14_model_complementarity", "Pairwise complementarity: tasks the row model solves that the column model misses."),
    ("15_coverage_progress", "Benchmark completion tracker — fraction of tasks each model has run so far."),
    ("16_compile_calls", "Mean number of Unreal compiler invocations (completed UBT builds) per task, parsed from agent trajectories."),
    ("17_compile_errors", "Mean number of Unreal compile errors encountered per task while solving, parsed from agent trajectories."),
]


def write_manifest(out_dir, series_list, lo, hi, total_tasks, args):
    order = order_for_plot(series_list, present_only=False)
    lines = ["# UnrealBench figure index", "",
             f"Generated from `{args.results_root}` (tasks {lo}–{hi}, {total_tasks} task specs).",
             "",
             "All figures are written as both `.pdf` (LaTeX input) and `.png` (preview).",
             "", "## Models", ""]
    for s in order:
        status = "**pending**" if s.n == 0 else f"{s.n}/{total_tasks} tasks"
        rate_value = _judge_rate(s) if HEADLINE == "calibrated" else s.pass_rate()
        rate = fmt_pct(rate_value) if s.n else "—"
        lines.append(f"- **{s.cfg.label}** ({s.cfg.agent}/{s.cfg.model}/{s.cfg.reasoning}) — {status}, {pass_word()} pass {rate}")
    lines += ["", "## Figures", ""]
    existing = {p.stem for p in out_dir.glob("*.pdf")}
    for stem, cap in FIGURE_CAPTIONS:
        mark = "" if stem in existing else "  _(not rendered — insufficient data)_"
        lines.append(f"- `{stem}.pdf` — {cap}{mark}")
    lines += ["", "## Audit data", "",
              "- `summary.csv` — one row per model config with every headline metric.",
              "- `task_matrix.csv` — PASS/FAIL per task per model.",
              "- `per_run.csv` — the selected latest run for every (model, task).", ""]
    (out_dir / "figure_index.md").write_text("\n".join(lines), encoding="utf-8")


# ----------------------------------------------------------------------------
# Orchestration
# ----------------------------------------------------------------------------

def run_fig(name: str, fn: Callable, *a) -> None:
    try:
        fn(*a)
    except Exception:
        warn(f"figure '{name}' failed:\n{traceback.format_exc()}")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--results-root", type=Path, default=DEFAULT_RESULTS)
    ap.add_argument("--tasks-dir", type=Path, default=DEFAULT_TASKS)
    ap.add_argument("--out-dir", type=Path, default=DEFAULT_OUT)
    ap.add_argument("--roster", type=Path, default=DEFAULT_ROSTER)
    ap.add_argument("--lo", type=int, default=1)
    ap.add_argument("--hi", type=int, default=120)
    ap.add_argument("--no-auto", action="store_true",
                    help="Do not auto-discover configs missing from the roster.")
    ap.add_argument("--no-steps", action="store_true",
                    help="Skip trajectory parsing for the agent-steps figure.")
    ap.add_argument("--headline", choices=["raw", "calibrated"], default="raw",
                    help="Headline pass metric for the figures. 'raw' = all hidden "
                         "tests pass; 'calibrated' = LLM-judge full-task verdict; "
                         "missing verdicts count as failures.")
    ap.add_argument("--exclude-task", action="append", default=[],
                    help="Task id or number to exclude from this analysis; may be repeated.")
    ap.add_argument("--exclude-file", type=Path, action="append", default=[],
                    help="File containing task ids/numbers to exclude, whitespace or comma separated.")
    args = ap.parse_args()

    global HEADLINE
    HEADLINE = args.headline
    excluded = load_excluded_tasks(args.exclude_file, args.exclude_task)

    print(f"Reading runs from {args.results_root}  (headline metric: {HEADLINE})")
    if excluded:
        print("  excluding tasks: " + ", ".join(f"ue_task_{n:04d}" for n in sorted(excluded)))
    runs = collect_runs(args.results_root, args.lo, args.hi, excluded)
    if not runs:
        warn("no usable runs found — writing an empty manifest and exiting cleanly")
        args.out_dir.mkdir(parents=True, exist_ok=True)
        (args.out_dir / "figure_index.md").write_text(
            "# UnrealBench figure index\n\nNo usable runs found.\n", encoding="utf-8")
        return 0

    roster = load_roster(args.roster)
    series = build_series(runs, roster, include_auto=not args.no_auto)
    meta = collect_task_meta(args.tasks_dir, args.lo, args.hi, excluded)
    total_tasks = len(meta) or (args.hi - args.lo + 1)

    present = [s for s in series if s.n]
    if not args.no_steps:
        filled = fill_trajectory_metrics(present)
        if filled:
            print(f"  extracted trajectory metrics (steps/compiles/errors) for {filled} runs")

    args.out_dir.mkdir(parents=True, exist_ok=True)

    run_fig("01_pass_rate", fig_pass_rate, series, args.out_dir, args.lo, args.hi)
    run_fig("02_pass_rate_breakdown", fig_pass_rate_breakdown, series, args.out_dir)
    run_fig("03_task_outcome_matrix", fig_task_matrix, series, args.out_dir, args.lo, args.hi, excluded)
    run_fig("04_pass_rate_by_tier", fig_pass_rate_by_tier, series, meta, args.out_dir)
    run_fig("05_pass_rate_by_domain", fig_pass_rate_by_domain, series, meta, args.out_dir)
    run_fig("06_pass_rate_by_category", fig_pass_rate_by_category, series, meta, args.out_dir)
    run_fig("07_time_vs_pass_rate", fig_time_vs_pass, series, args.out_dir)
    run_fig("08_cost_vs_pass_rate", fig_cost_vs_pass, series, args.out_dir)
    run_fig("09_output_tokens", fig_output_tokens, series, args.out_dir)
    run_fig("10_pass_rate_vs_steps", fig_pass_vs_steps, series, args.out_dir)
    run_fig("11_failure_modes", fig_failure_modes, series, args.out_dir)
    run_fig("12_task_difficulty_curve", fig_difficulty_curve, series, args.out_dir, args.lo, args.hi, excluded)
    run_fig("13_solved_by_n_models", fig_solved_by_n, series, args.out_dir, args.lo, args.hi, excluded)
    run_fig("14_model_complementarity", fig_complementarity, series, args.out_dir)
    run_fig("15_coverage_progress", fig_coverage, series, args.out_dir, total_tasks)
    run_fig("16_compile_calls", fig_compile_calls, series, args.out_dir)
    run_fig("17_compile_errors", fig_compile_errors, series, args.out_dir)

    run_fig("csvs", write_csvs, args.out_dir, series, meta, args.lo, args.hi, excluded)
    run_fig("manifest", write_manifest, args.out_dir, series, args.lo, args.hi, total_tasks, args)

    print(f"\nWrote figures + CSVs to {args.out_dir}")
    print(f"{'MODEL':<34}{'STATUS':<11}{'HEADLINE':<16}{'RAW PASS':<16}{'JUDGE':<9}{'REQ':<9}")
    for s in order_for_plot(series, present_only=False):
        if s.n == 0:
            print(f"{s.cfg.label:<34}{'pending':<11}{'-':<16}{'-':<16}{'-':<9}{'-':<9}")
            continue
        raw_k = sum(1 for r in s.present if r.raw_pass)
        head_k, head_n, head_rate = headline_count_rate(s.present)
        jr, rr = _judge_rate(s), _req_rate(s)
        status = "complete" if s.n >= total_tasks else "partial"
        headline_col = f"{head_k}/{head_n} ({fmt_pct(head_rate)})" if head_n else "-"
        raw_col = f"{raw_k}/{s.n} ({fmt_pct(s.rate('raw_pass'))})"
        print(f"{s.cfg.label:<34}{status:<11}{headline_col:<16}{raw_col:<16}{fmt_pct(jr):<9}{fmt_pct(rr):<9}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
