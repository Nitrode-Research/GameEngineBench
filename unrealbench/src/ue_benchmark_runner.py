#!/usr/bin/env python3
"""
Benchmark runner for Unreal Engine tasks.

Capability modes (set per task via task_config.json / spec.yaml):
  ue_automation - Primary path: copy workspace from start/ -> solver edits C++ -> inject
                  hidden tests -> compile with UnrealBuildTool -> run UE automation tests
                  headless (listen-server, -NullRHI, no GPU) -> per-test pass/fail ->
                  optional LLM-as-judge. Requires a local UE install.
  code_only     - Solver edits C++/config; a static validate.py checks output. No UE install.
  hybrid        - Compile -> open headed editor with the MCP plugin -> solver edits files and
                  calls MCP tools -> collect editor_action_log.json. Requires a UE install,
                  a display/GPU, and chongdashu/unreal-mcp. (Not usable headless.)

Engine config:
  Engine root is read from the UE_ENGINE_ROOT environment variable (required); the build
  platform is auto-detected (Mac / Win64 / Linux) or overridden via UE_PLATFORM. See
  tasks_unreal/unreal_config.yaml. On Windows, workspaces are created under a short root
  (C:\\ub) to stay within the MAX_PATH limit for deep UBT intermediate paths.

Run modes:
  Solve+evaluate : --agent AGENT [--model MODEL] [--reasoning-level LEVEL]
                   [--task-id ID ...] [--warm-cache]
                   [--skip-judge] [--solver-timeout SEC] [--resume-from PATH]
  Retest         : --retest SNAPSHOT             re-run tests + judge on an existing snapshot
                   --retest-batch SNAPSHOT ...    re-run multiple snapshots for one task
                   --retest-baseline TASK_ID      run tests on start/ and solution/ only

  --warm-cache reuses a persistent per-task compiled base (built once, kept under the
  workspace root) and incrementally recompiles only changed files; auto-invalidates when
  peer plugins, scaffold, Build.cs, tests, or the engine change.

  --reasoning-level controls model reasoning effort for supported agents
  (claude-code, codex, gemini-cli). Supported values are default, low, medium,
  high, xhigh, and max. Each snapshot records reasoning_level_requested,
  reasoning_level_applied, and judge_model in result.json.

Usage:
    unrealbench-ue --tasks-dir tasks_unreal --output results/ue_results.json \\
        --agent claude-code [--model MODEL] [--reasoning-level high] \\
        [--task-id ue_task_0001] [--warm-cache] \\
        [--skip-judge] [--judge-model MODEL] [--debug] [--resume-from PATH]
    unrealbench-ue --retest ue_task_0001
"""

import argparse
import hashlib
import json
import os
import re
import socket
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable, Optional, TypeVar

from unrealbench.src.solver_factory import SolverFactory
from unrealbench.src.authoring.validators.registry import (
    run_full_validation,
    overall_passed,
)
from unrealbench.src.authoring.validators.base import UnrealConnectionAdapter
from unrealbench.src.ue_judge import run_unreal_judge, select_cross_family_judge_model

T = TypeVar("T")


def _load_repo_dotenv() -> None:
    """Load repo-local .env for benchmark runs when python-dotenv is installed."""
    try:
        from dotenv import load_dotenv
    except ImportError:
        return
    load_dotenv()


class UnrealBenchmarkRunner:
    def __init__(
        self,
        tasks_dir: str,
        output_file: str,
        agent: str,
        model: Optional[str] = None,
        debug: bool = False,
        resume_from: Optional[str] = None,
        run_judge: bool = True,
        solver_timeout: int = 3600,
        warm_cache: bool = False,
        reasoning_level: str = "default",
        judge_model: Optional[str] = None,
    ):
        self.tasks_dir = Path(tasks_dir)
        self.output_file = Path(output_file)
        self.agent = agent
        self.model = model or "default"
        self.debug = debug
        self.run_judge = run_judge
        self.solver_timeout = solver_timeout
        self.warm_cache = warm_cache
        self.reasoning_level = (reasoning_level or "default").strip().lower()
        self.judge_model_override = (judge_model or os.environ.get("UNREALBENCH_JUDGE_MODEL") or "").strip()
        # Per-invocation memo of warm-base results (key -> base Path or None),
        # so the base is built at most once even when multiple baselines/snapshots
        # are evaluated in one run.
        self._warm_base_memo: dict = {}
        self.completed = self._load_progress(resume_from)
        self.results = []
        self.ue_cfg = self._load_ue_config()

    def _judge_model_for(self, model: Optional[str] = None, agent: Optional[str] = None) -> str:
        if self.judge_model_override:
            return self.judge_model_override
        return select_cross_family_judge_model(
            solver_model=model or self.model,
            solver_agent=agent or self.agent,
        )

    # -- Cross-platform paths --------------------------------------------------

    @staticmethod
    def _detect_ue_platform() -> str:
        """Map sys.platform to the UE build-target platform string."""
        if sys.platform == "darwin":
            return "Mac"
        if sys.platform == "win32":
            return "Win64"
        return "Linux"

    def _default_results_root(self) -> Path:
        """Default location for saved benchmark snapshots.

        Unreal benchmark artifacts are kept inside the unrealbench repo under
        ``tasks_unreal/test_result`` by default because the surrounding
        authoring, replay, and manual review workflow expects them there.
        Override with the GAMEDEVBENCH_RESULTS_DIR environment variable when a
        different location is explicitly desired.
        """
        override = os.environ.get("GAMEDEVBENCH_RESULTS_DIR")
        if override:
            return Path(override).expanduser()
        return self.tasks_dir / "test_result"

    @staticmethod
    def _resolve_unreal_log(uproject_path: Path) -> Optional[Path]:
        """Return the most relevant UE log path across platforms.

        Workspace-local log is preferred because it cannot be polluted by
        concurrent or stale editor instances of the same project name. The
        per-user fallback differs by OS - checked only if the workspace log
        is missing or empty.
        """
        project_name = uproject_path.stem
        log_dir = uproject_path.parent / "Saved" / "Logs"
        # On Windows UE writes <ProjectName>.log; on Mac/Linux it writes UnrealEditor.log
        for log_name in (f"{project_name}.log", "UnrealEditor.log"):
            workspace_log = log_dir / log_name
            if workspace_log.exists() and workspace_log.stat().st_size > 0:
                return workspace_log

        candidates: list[Path] = []
        if sys.platform == "darwin":
            candidates.append(
                Path.home()
                / "Library"
                / "Logs"
                / "Unreal Engine"
                / f"{project_name}Editor"
                / f"{project_name}.log"
            )
        elif sys.platform == "win32":
            local = os.environ.get("LOCALAPPDATA")
            base = Path(local) if local else Path.home() / "AppData" / "Local"
            candidates.append(
                base
                / "UnrealEngine"
                / f"{project_name}Editor"
                / "Saved"
                / "Logs"
                / f"{project_name}.log"
            )
        else:
            candidates.append(
                Path.home()
                / ".config"
                / "Epic"
                / "UnrealEngine"
                / f"{project_name}Editor"
                / "Saved"
                / "Logs"
                / f"{project_name}.log"
            )
            candidates.append(
                Path.home()
                / ".local"
                / "share"
                / "Epic"
                / "UnrealEngine"
                / f"{project_name}Editor"
                / "Saved"
                / "Logs"
                / f"{project_name}.log"
            )

        for c in candidates:
            if c.exists() and c.stat().st_size > 0:
                return c
        return None

    @staticmethod
    def _sanitize_workspace_label(label: str) -> str:
        cleaned = re.sub(r"[^A-Za-z0-9_.-]+", "_", label).strip("._-")
        return cleaned or "workspace"

    @staticmethod
    def _task_workspace_label(task_dir: Path) -> str:
        match = re.match(r"^(ue_task_\d{4})", task_dir.name)
        return match.group(1) if match else UnrealBenchmarkRunner._sanitize_workspace_label(task_dir.name)

    @staticmethod
    def _clear_workspace_runtime_artifacts(workspace: Path) -> None:
        for rel in [
            "result.json",
            "compile_output.txt",
            "unreal_log.txt",
            "Saved/Logs/UnrealEditor.log",
        ]:
            path = workspace / rel
            if path.exists():
                path.unlink()

    @staticmethod
    def _remove_macos_sidecar_files(root: Path) -> None:
        if not root.exists():
            return
        for path in root.rglob("*"):
            if not path.is_file():
                continue
            if path.name == ".DS_Store" or path.name.startswith("._"):
                path.unlink(missing_ok=True)

    @staticmethod
    def _safe_rmtree(path: Path) -> None:
        # Cleanup must never crash a completed run. macOS/APFS can race a
        # sidecar (._*) or a still-flushing file into the tree during deletion,
        # producing OSError ENOTEMPTY. Retry a few times, then best-effort.
        if not path.exists():
            return
        for attempt in range(3):
            try:
                shutil.rmtree(path)
                return
            except FileNotFoundError:
                return
            except OSError:
                if attempt < 2:
                    time.sleep(0.5)
                    continue
                shutil.rmtree(path, ignore_errors=True)  # best-effort final pass
                return

    # -- UE config --------------------------------------------------------------

    def _load_ue_config(self) -> dict:
        """Load engine paths and MCP config from unreal_config.yaml."""
        config_path = self.tasks_dir / "unreal_config.yaml"
        if not config_path.exists():
            return {}
        try:
            import yaml
        except ImportError:
            print("WARNING: PyYAML not installed; UE config unavailable.")
            return {}
        with config_path.open() as f:
            cfg = yaml.safe_load(f)

        engine_root = os.environ.get("UE_ENGINE_ROOT", cfg.get("engine_root", ""))
        if not engine_root:
            raise RuntimeError(
                "UE engine root is not set. "
                "Set the UE_ENGINE_ROOT environment variable to your UE install path.\n"
                "  Mac:   export UE_ENGINE_ROOT=\"/Users/Shared/Epic Games/UE_5.7\"\n"
                "  Win64: $env:UE_ENGINE_ROOT=\"C:/Program Files/Epic Games/UE_5.7\""
            )
        platform = os.environ.get(
            "UE_PLATFORM", cfg.get("platform") or self._detect_ue_platform()
        )

        build_template = cfg.get("build_script_templates", {}).get(platform, "")
        build_script = (
            build_template.replace("{engine_root}", engine_root)
            if build_template
            else ""
        )

        editor_template = cfg.get("editor_binary_templates", {}).get(platform, "")
        editor_binary = (
            editor_template.replace("{engine_root}", engine_root)
            if editor_template
            else ""
        )

        tv_plugins_dir_raw = os.environ.get(
            "UE_TV_PLUGINS_DIR", cfg.get("tv_plugins_dir", "")
        )
        # Resolve relative paths against the config file's directory so the
        # same config works on any platform without absolute path edits.
        if tv_plugins_dir_raw and not Path(tv_plugins_dir_raw).is_absolute():
            tv_plugins_dir = str((config_path.parent / tv_plugins_dir_raw).resolve())
        else:
            tv_plugins_dir = tv_plugins_dir_raw

        return {
            "engine_root": engine_root,
            "platform": platform,
            "configuration": cfg.get("configuration", "Development"),
            "build_script": build_script,
            "editor_binary": editor_binary,
            "mcp": cfg.get("mcp", {}),
            "tv_plugins_dir": tv_plugins_dir,
        }

    # -- Progress / resume -----------------------------------------------------

    def _load_progress(self, resume_from: Optional[str]) -> set:
        if not resume_from:
            return set()
        data = json.loads(Path(resume_from).read_text())
        if isinstance(data, list):
            records = data
        else:
            records = data.get("tasks", data.get("results", []))
        return {r["task_id"] for r in records if r.get("solver_success")}

    # -- Task discovery --------------------------------------------------------

    def find_tasks(self):
        return sorted(self.tasks_dir.glob("ue_task_*"))

    def load_task_config(self, task_dir: Path) -> Optional[dict]:
        config_path = task_dir / "task_config.json"
        if config_path.exists():
            try:
                return json.loads(config_path.read_text())
            except Exception as e:
                print(f"  ERROR loading task_config.json: {e}")
                return None

        spec_path = task_dir / "spec.yaml"
        if spec_path.exists():
            try:
                import yaml

                with spec_path.open() as f:
                    spec = yaml.safe_load(f)
                return {
                    "task_id": spec.get("task_id", task_dir.name),
                    "task_name": spec.get("title", task_dir.name),
                    "capability_mode": "ue_automation",
                    "engine": "unreal",
                    "instruction": spec.get("instruction", ""),
                    "files_to_edit": spec.get("files_to_edit", []),
                    "hidden_requirements": spec.get("hidden_requirements", {}),
                    "test_requirements": spec.get("test_requirements", {}),
                    "tests": spec.get("tests", {}),
                    "tier": spec.get("tier", ""),
                }
            except Exception as e:
                print(f"  ERROR loading spec.yaml: {e}")
                return None

        return None

    # -- Forbidden-touch detection ---------------------------------------------

    def _extract_edits(self, workspace: Path, task_dir: Path, task_config: dict):
        """Return (files_edited, forbidden_touched).

        files_edited      - allowed files that actually changed vs start/ baseline.
        forbidden_touched - read-only files that were modified or newly created.
                            Recorded only; does NOT fail validation (v1 policy).
        """
        allowed = set(task_config.get("files_to_edit", []))
        allowed |= set(task_config.get("config_files_to_edit", []))
        readonly_globs = [
            "Config/**",
            "Source/**/*.Target.cs",
            "Source/**/*.Build.cs",
            "*.uproject",
        ]

        baseline_dir = task_dir / "start"
        forbidden = []
        files_edited = []

        for rel in allowed:
            ws_file = workspace / rel
            base_file = baseline_dir / rel
            if ws_file.exists():
                if (
                    not base_file.exists()
                    or ws_file.read_bytes() != base_file.read_bytes()
                ):
                    files_edited.append(rel)

        for pattern in readonly_globs:
            for f in workspace.glob(pattern):
                if not f.is_file():
                    continue
                rel = str(f.relative_to(workspace))
                baseline = baseline_dir / rel
                if not baseline.exists() or f.read_bytes() != baseline.read_bytes():
                    forbidden.append(rel)

        return files_edited, forbidden

    # -- Static validation -----------------------------------------------------

    def _validate(self, task_dir: Path, task_config: dict, workspace: Path):
        """Build val_copy from start/ + allowed edits, then run validate.py."""
        validate_script = task_dir / "validate.py"
        if not validate_script.exists():
            return False, "No validate.py found"

        val_copy = None
        try:
            val_copy = Path(tempfile.mkdtemp()) / "val"
            shutil.copytree(task_dir / "start", val_copy)

            for rel in task_config.get("files_to_edit", []):
                ws_file = workspace / rel
                if ws_file.exists():
                    dest = val_copy / rel
                    dest.parent.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(ws_file, dest)

            result = subprocess.run(
                [sys.executable, str(validate_script), str(val_copy)],
                capture_output=True,
                text=True,
            )
            output = result.stdout + result.stderr
            return result.returncode == 0, output

        finally:
            if val_copy and val_copy.exists():
                shutil.rmtree(val_copy)

    # -- Compile step ----------------------------------------------------------

    def _find_uproject(self, workspace: Path) -> Optional[Path]:
        uprojects = list(workspace.glob("*.uproject"))
        return uprojects[0] if uprojects else None

    @staticmethod
    def _read_ubt_log_tail(max_lines: int = 200) -> str:
        log_path = Path.home() / "Library/Application Support/Epic/UnrealBuildTool/Log.txt"
        if not log_path.exists():
            return ""

        try:
            lines = log_path.read_text(encoding="utf-8", errors="replace").splitlines()
        except OSError:
            return ""

        tail = "\n".join(lines[-max_lines:]).strip()
        if not tail:
            return ""

        return f"\n\n--- UnrealBuildTool Log Tail ---\n{tail}"

    def _build_compile_cmd(self, uproject_path: Path) -> list:
        """Build the UnrealBuildTool compile command for a project's editor target.

        Single source of truth for the compile invocation, shared by
        _compile_project and the make_workspace helper.
        """
        return [
            self.ue_cfg.get("build_script", ""),
            f"{uproject_path.stem}Editor",
            self.ue_cfg.get("platform", "Mac"),
            self.ue_cfg.get("configuration", "Development"),
            f"-Project={uproject_path.resolve()}",
            "-WaitMutex",
            "-NoHotReload",
        ]

    def _compile_project(self, uproject_path: Path) -> tuple:
        """Compile the UE project using the configured build script.

        Returns (success: bool, output: str).
        """
        build_script = self.ue_cfg.get("build_script", "")
        if not build_script:
            return False, "build_script not configured in unreal_config.yaml"
        if not Path(build_script).exists():
            return False, f"Build script not found: {build_script}"

        cmd = self._build_compile_cmd(uproject_path)
        if self.debug:
            print(f"  Compile: {' '.join(cmd)}")

        # Cold builds of TargetVector + ALSXT take ~25-40 min. Override via
        # compile_timeout_seconds in unreal_config.yaml.
        compile_timeout = int(self.ue_cfg.get("compile_timeout_seconds", 3600))
        try:
            result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                # UBT/MSVC output can carry non-ASCII (paths, warning text). Force
                # UTF-8 with replacement so the compile-log capture never throws on
                # the Windows cp1252 default mid-build.
                encoding="utf-8",
                errors="replace",
                timeout=compile_timeout,
            )
            output = result.stdout or ""
            if result.returncode != 0:
                output = f"{output}{self._read_ubt_log_tail()}"
            return result.returncode == 0, output
        except subprocess.TimeoutExpired as exc:
            partial_output = exc.stdout or ""
            if isinstance(partial_output, bytes):
                partial_output = partial_output.decode("utf-8", errors="replace")
            partial_output = f"{partial_output}{self._read_ubt_log_tail()}"
            timeout_note = f"\nCompile timed out after {compile_timeout}s"
            return False, f"{partial_output}{timeout_note}"

    def _run_with_heartbeat(
        self,
        label: str,
        fn: Callable[[], T],
        heartbeat_seconds: int = 30,
    ) -> T:
        """Run a blocking operation while printing lightweight progress heartbeats."""
        result: dict[str, object] = {"value": None, "error": None}
        done = threading.Event()

        def _target() -> None:
            try:
                result["value"] = fn()
            except BaseException as exc:  # pragma: no cover - re-raised below
                result["error"] = exc
            finally:
                done.set()

        start = time.time()
        thread = threading.Thread(target=_target, daemon=True)
        thread.start()
        status_width = 0

        while not done.wait(timeout=heartbeat_seconds):
            elapsed = int(time.time() - start)
            status = f"  {label} still running... {elapsed}s elapsed"
            status_width = max(status_width, len(status))
            print(f"\r{status.ljust(status_width)}", end="", flush=True)

        elapsed = time.time() - start
        error = result["error"]
        if status_width:
            print("\r" + (" " * status_width) + "\r", end="", flush=True)
        if error is not None:
            raise error  # type: ignore[misc]

        print(f"  {label} completed in {elapsed:.1f}s")
        return result["value"]  # type: ignore[return-value]

    # -- Editor session --------------------------------------------------------

    @staticmethod
    def _per_user_editor_log_dir(project_name: str) -> Optional[Path]:
        """Locate the per-user UE editor log directory across platforms.

        Used by the MCP-readiness watcher to detect editor startup before the
        workspace log file is flushed.
        """
        if sys.platform == "darwin":
            return (
                Path.home()
                / "Library"
                / "Logs"
                / "Unreal Engine"
                / f"{project_name}Editor"
            )
        if sys.platform == "win32":
            local = os.environ.get("LOCALAPPDATA")
            base = Path(local) if local else Path.home() / "AppData" / "Local"
            return base / "UnrealEngine" / f"{project_name}Editor" / "Saved" / "Logs"
        return (
            Path.home()
            / ".config"
            / "Epic"
            / "UnrealEngine"
            / f"{project_name}Editor"
            / "Saved"
            / "Logs"
        )

    def _latest_editor_log_path(
        self, project_name: str = "UnrealBenchTask"
    ) -> Optional[Path]:
        log_dir = self._per_user_editor_log_dir(project_name)
        if log_dir is None or not log_dir.exists():
            return None
        logs = sorted(
            log_dir.glob("*.log"), key=lambda p: p.stat().st_mtime, reverse=True
        )
        return logs[0] if logs else None

    def _recent_editor_logs(self, project_name: str = "UnrealBenchTask") -> list[Path]:
        log_dir = self._per_user_editor_log_dir(project_name)
        if log_dir is None or not log_dir.exists():
            return []
        return sorted(
            log_dir.glob("*.log"), key=lambda p: p.stat().st_mtime, reverse=True
        )[:5]

    @staticmethod
    def _tcp_port_open(host: str, port: int, timeout: float = 0.25) -> bool:
        try:
            with socket.create_connection((host, port), timeout=timeout):
                return True
        except OSError:
            return False

    def _wait_for_editor_ready(
        self,
        proc: subprocess.Popen,
        uproject_path: Path,
        timeout_seconds: int,
    ) -> bool:
        """Wait until the launched editor is ready for MCP-driven automation.

        Readiness is determined by either:
        - the UnrealMCP socket accepting connections on localhost:55557, or
        - a recent editor log for this project containing the UnrealMCP startup marker.
        """
        deadline = time.time() + timeout_seconds
        project_path = str(uproject_path.resolve())
        ready_markers = [
            "UnrealMCPBridge: Server started on",
            "Unreal MCP Module has started",
        ]
        mcp_host = "127.0.0.1"
        mcp_port = 55557

        project_name = uproject_path.stem

        while time.time() < deadline:
            if self._tcp_port_open(mcp_host, mcp_port):
                return True

            for log_path in self._recent_editor_logs(project_name):
                text = log_path.read_text(encoding="utf-8", errors="ignore")
                if project_path in text and any(
                    marker in text for marker in ready_markers
                ):
                    return True

            # On macOS, launching a .uproject can hand off to an already-running
            # UnrealEditor instance. In that case the original process exits even
            # though the editor continues opening the project. Keep waiting for
            # a positive readiness signal instead of treating early exit as fatal.
            time.sleep(1)

        return False

    def _open_editor(self, uproject_path: Path) -> Optional[subprocess.Popen]:
        """Launch headed Unreal Editor with the MCP plugin active.

        The editor process is returned; the caller is responsible for terminating it.
        stdout/stderr are discarded - UE writes its log to Saved/Logs/UnrealEditor.log.
        """
        editor_binary = self.ue_cfg.get("editor_binary", "")
        if not editor_binary:
            print("  ERROR: editor_binary not configured in unreal_config.yaml")
            return None
        if not Path(editor_binary).exists():
            print(f"  ERROR: Editor binary not found: {editor_binary}")
            return None

        cmd = [editor_binary, str(uproject_path.resolve()), "-NoHotReload"]

        # On Apple Silicon, UnrealEditor is a universal binary. If it launches
        # under Rosetta/x64 while the project modules were built arm64, the editor
        # reports the generic "game module could not be loaded" dialog. Force the
        # editor process to arm64 so its runtime architecture matches the built
        # dylibs produced by UBT.
        if sys.platform == "darwin" and os.uname().machine == "arm64":
            cmd = ["/usr/bin/arch", "-arm64", *cmd]
        if self.debug:
            print(f"  Editor: {' '.join(cmd)}")

        proc = subprocess.Popen(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )

        startup_wait = self.ue_cfg.get("mcp", {}).get("editor_startup_wait_seconds", 60)
        if self.debug:
            print(
                f"  Waiting up to {startup_wait}s for editor + MCP plugin to initialise..."
            )

        ready = self._wait_for_editor_ready(proc, uproject_path, startup_wait)
        if ready:
            if self.debug:
                print("  Editor ready")
            return proc

        if proc.poll() is not None:
            print("  ERROR: Editor exited during startup")
        else:
            print("  ERROR: Editor did not become ready before timeout")
        return None

    def _close_editor(self, proc: subprocess.Popen, workspace: Path) -> str:
        """Terminate the editor process and return the UE log contents."""
        try:
            proc.terminate()
            proc.wait(timeout=30)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()

        log_path = workspace / "Saved" / "Logs" / "UnrealEditor.log"
        if log_path.exists():
            return log_path.read_text(encoding="utf-8", errors="ignore")
        return ""

    # -- Action log ------------------------------------------------------------

    def _collect_editor_action_log(self, workspace: Path) -> list:
        """Read editor_action_log.json from workspace root if present.

        The agent writes this file as a first-class deliverable during hybrid runs.
        The runner collects it after the solver returns.
        If the file does not exist, the agent took no logged editor actions.
        """
        log_path = workspace / "editor_action_log.json"
        if not log_path.exists():
            return []
        try:
            data = json.loads(log_path.read_text())
            return data if isinstance(data, list) else data.get("actions", [])
        except Exception:
            return []

    def _summarize_actions(self, action_log: list) -> list:
        """Collapse the action log to one entry per (action_type, target) pair."""
        seen = set()
        summary = []
        for entry in action_log:
            target = entry.get("actor") or entry.get("level") or entry.get("class")
            key = (entry.get("action"), target)
            if key not in seen:
                seen.add(key)
                summary.append({"action": entry.get("action"), "target": target})
        return summary

    def _content_changed(self, workspace: Path, task_dir: Path) -> bool:
        """Return True if Content/ differs from start/Content/ baseline."""
        ws_content = workspace / "Content"
        start_content = task_dir / "start" / "Content"
        if not ws_content.exists():
            return False
        if not start_content.exists():
            return True  # Content directory created from scratch
        for f in ws_content.rglob("*"):
            if not f.is_file():
                continue
            baseline = start_content / f.relative_to(ws_content)
            if not baseline.exists() or f.read_bytes() != baseline.read_bytes():
                return True
        return False

    def _clean_generated_artifacts(self, workspace: Path) -> None:
        """Remove Unreal-generated build/cache state copied from task stubs.

        Task templates may contain stale Binaries/Intermediate/Saved state from a
        previous engine build. Copying those into a fresh temp workspace can make
        the editor try to load mismatched module receipts or dylibs before the
        current compile step has rebuilt them.
        """
        generated_dirs = [
            "Binaries",
            "Build",
            "Intermediate",
            "Plugins/UnrealMCP/Binaries",
            "Plugins/UnrealMCP/Intermediate",
            "Plugins/UnrealMCP/Saved",
        ]

        for rel in generated_dirs:
            path = workspace / rel
            self._safe_rmtree(path)

    @staticmethod
    def _patch_build_cs_for_tests(workspace: Path, project_name: str) -> None:
        """Add AutomationController to PublicDependencyModuleNames if not already present."""
        build_cs = workspace / "Source" / project_name / f"{project_name}.Build.cs"
        if not build_cs.exists():
            return
        text = build_cs.read_text(encoding="utf-8")
        if "AutomationController" in text:
            return
        # Insert after the first AddRange( call on PublicDependencyModuleNames
        import re

        patched = re.sub(
            r"(PublicDependencyModuleNames\.AddRange\(new string\[\] \{)",
            r'\1\n            "AutomationController",',
            text,
            count=1,
        )
        if patched != text:
            build_cs.write_text(patched, encoding="utf-8")

    @staticmethod
    def _unpatch_build_cs_for_tests(build_cs_path: Path) -> None:
        """Reverse _patch_build_cs_for_tests on a copied file.

        Used when sanitizing the saved snapshot so the AutomationController
        dependency we injected for evaluation does not leak into the artifact
        that downstream tooling treats as the solver's attempt."""
        if not build_cs_path.exists():
            return
        text = build_cs_path.read_text(encoding="utf-8")
        import re

        unpatched = re.sub(
            r'(PublicDependencyModuleNames\.AddRange\(new string\[\] \{)\n\s*"AutomationController",',
            r"\1",
            text,
            count=1,
        )
        if unpatched != text:
            build_cs_path.write_text(unpatched, encoding="utf-8")

    def _assert_no_test_contamination(self, workspace: Path, task_dir: Path) -> None:
        """Fail loudly if hidden test material is reachable to the solver.

        The benchmark contract is that ``tests/`` lives only at
        ``task_dir/tests/`` and is injected after the solver finishes. This
        check catches three failure modes that would silently turn a hidden
        contract into a visible one:

          1. ``task_dir/start/`` was authored or polluted with test source files
             whose names match ``task_dir/tests/`` - the solver would inherit
             them on every run.
          2. The freshly-copied workspace already contains a
             ``Source/<Project>/Tests/`` directory before injection.

        On detection, raises ``RuntimeError``; the caller marks the run failed
        rather than silently scoring a contaminated attempt.
        """
        tests_src = task_dir / "tests"
        if not tests_src.exists():
            return
        hidden_names = {f.name for f in tests_src.glob("*.cpp")}
        if not hidden_names:
            return

        violations: list[str] = []

        for src in workspace.rglob("*.cpp"):
            if src.name in hidden_names:
                violations.append(f"workspace:{src.relative_to(workspace)}")

        start_dir = task_dir / "start"
        if start_dir.exists():
            for src in start_dir.rglob("*.cpp"):
                if src.name in hidden_names:
                    violations.append(f"task_dir/start:{src.relative_to(start_dir)}")

        if violations:
            joined = "\n  - ".join(violations)
            raise RuntimeError(
                "Pre-run contamination check failed. The solver would be able "
                "to read hidden evaluation material:\n  - "
                f"{joined}\n"
                "Ensure task_dir/start/ and the hydrated workspace contain no "
                "hidden test source files before evaluation."
            )

    def _remove_solver_test_files(self, workspace: Path, task_dir: Path, inject_dest: Path) -> None:
        """Remove any test files the solver wrote during its run.

        Solvers sometimes create their own test files to verify their work.
        These would be picked up by the automation filter alongside the
        benchmark tests, inflating the pass rate with easier self-written checks.
        We remove any .cpp files found in test-like directories that were not
        present in task_dir/start/ and are not the injection target directory.
        """
        start_dir = task_dir / "start"
        test_dir_names = {"tests", "test"}

        for src_file in list(workspace.rglob("*.cpp")):
            # Skip the injection target directory - we'll populate that ourselves
            if src_file.parent == inject_dest:
                continue
            # Only care about files inside a directory named Tests/Test (case-insensitive)
            if src_file.parent.name.lower() not in test_dir_names:
                continue
            # If this file existed in start/, it belongs there - leave it
            try:
                rel = src_file.relative_to(workspace)
                if (start_dir / rel).exists():
                    continue
            except ValueError:
                pass
            # Solver-created test file - remove it
            src_file.unlink()
            if self.debug:
                print(f"  Removed solver-created test file: {src_file.relative_to(workspace)}")

    def _inject_tests_for_evaluation(self, workspace: Path, task_dir: Path) -> None:
        """Copy tests into the workspace only after solver execution, for evaluation."""
        tests_src = task_dir / "tests"
        if not tests_src.exists():
            return

        uproject = self._find_uproject(workspace)
        if not uproject:
            return

        test_dest = workspace / "Source" / uproject.stem / "Tests"

        # Remove any test files the solver may have written before injecting ours
        self._remove_solver_test_files(workspace, task_dir, test_dest)

        if test_dest.exists():
            shutil.rmtree(test_dest)
        test_dest.mkdir(parents=True, exist_ok=True)

        copied = 0
        for f in tests_src.glob("*.cpp"):
            shutil.copy2(f, test_dest / f.name)
            copied += 1
            if self.debug:
                print(
                    f"  Injected test for evaluation: {f.name} -> Source/{uproject.stem}/Tests/"
                )
        for f in tests_src.glob("*.h"):
            shutil.copy2(f, test_dest / f.name)
            if self.debug:
                print(
                    f"  Injected test header: {f.name} -> Source/{uproject.stem}/Tests/"
                )

        if copied:
            self._patch_build_cs_for_tests(workspace, uproject.stem)

    _SHARED_PLUGINS_MANIFEST = ".ub_shared_plugins.json"
    _SHARED_PLUGIN_PRUNE = {"Binaries", "Intermediate", "Saved", "Build"}
    _TARGETVECTOR_SHARED_PLUGIN_ALLOWLIST = {
        "ALS-Refactored",
        "ALSXT",
        "AsyncMixin",
        "CommonGame",
        "CommonLoadingScreen",
        "CommonUser",
        "GameplayMessageRouter",
        "ModularGameplayActors",
        "TargetVectorCode",
        "TargetVectorCommonUI",
        "TargetVectorContent",
        "TargetVectorEOS",
        "UIExtension",
    }

    def _populate_shared_plugins(self, workspace: Path) -> None:
        """Copy missing plugins from the configured shared TargetVector plugin volume.

        After the task's start/ is copied into the workspace, only the task-specific
        plugin (the one being modified) is present.  This method fills in every other
        plugin directory needed for the project to compile by copying from the shared
        plugin volume (tv_plugins_dir in unreal_config.yaml).

        Build artefacts (Binaries/Intermediate/Saved/Build) are excluded so the
        shared source is never mutated and workspace builds stay isolated.

        A manifest of which plugins came from the shared volume is written to
        ``.ub_shared_plugins.json`` in the workspace root.  _copy_attempt_dir reads
        this manifest to exclude shared plugins from result snapshots, keeping
        snapshots small.
        """
        tv_plugins_dir = Path(self.ue_cfg.get("tv_plugins_dir", ""))
        if not tv_plugins_dir.is_dir():
            return

        # Only apply to TargetVector tasks. Other projects (HordeTemplate, etc.)
        # must not receive TV plugins.
        uproject = self._find_uproject(workspace)
        if not uproject or uproject.name != "TargetVector.uproject":
            return

        ws_plugins = workspace / "Plugins"
        ws_plugins.mkdir(exist_ok=True)

        shared_names: list[str] = []
        prune = self._SHARED_PLUGIN_PRUNE

        # Only hydrate the shared plugins that the benchmarked TargetVector
        # workspace is expected to rely on. Copying every frozen plugin pulls
        # in unrelated editor/runtime modules and makes the temp workspace fail
        # in code that the task itself never declared.
        allowed = self._TARGETVECTOR_SHARED_PLUGIN_ALLOWLIST

        for plugin_src in sorted(tv_plugins_dir.iterdir()):
            if not plugin_src.is_dir():
                continue
            if plugin_src.name not in allowed:
                continue
            dest = ws_plugins / plugin_src.name
            if dest.exists():
                # Task-specific plugin already present from start/. Its Source is
                # being edited/graded, so keep that as-is. But the task copies ship
                # Source only (no Content), while demo levels + character assets used
                # by behavioral test fixtures live in the frozen plugin's Content/.
                # Overlay that Content/ when the task copy lacks it so fixtures like
                # L_Combat resolve at runtime. Content is identical across start/
                # and solution/, so this never affects what's graded.
                frozen_content = plugin_src / "Content"
                if frozen_content.is_dir() and not (dest / "Content").exists():
                    shutil.copytree(
                        frozen_content,
                        dest / "Content",
                        ignore=shutil.ignore_patterns(*prune),
                    )
                    if self.debug:
                        print(f"  Overlaid frozen Content/ onto task plugin {plugin_src.name}")
                continue
            shutil.copytree(
                plugin_src,
                dest,
                ignore=shutil.ignore_patterns(*prune),
            )
            shared_names.append(plugin_src.name)

        if shared_names:
            (workspace / self._SHARED_PLUGINS_MANIFEST).write_text(
                json.dumps(shared_names, indent=2), encoding="utf-8"
            )
            if self.debug:
                print(f"  Shared plugins added: {shared_names}")

    def _create_workspace(self, task_dir: Path) -> Path:
        """Create a stable per-run workspace outside the benchmark repo."""
        run_id = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
        root = self._workspace_root() / "ws"
        root.mkdir(parents=True, exist_ok=True)
        workspace = root / f"{self._task_workspace_label(task_dir)}_{run_id}"
        shutil.copytree(task_dir / "start", workspace)

        config_src = task_dir / "task_config.json"
        if config_src.exists():
            shutil.copy2(config_src, workspace / "task_config.json")
        else:
            # Generate task_config.json from spec.yaml so the solver can read it
            config = self.load_task_config(task_dir)
            if config:
                (workspace / "task_config.json").write_text(
                    json.dumps(config, indent=2), encoding="utf-8"
                )

        self._clean_generated_artifacts(workspace)
        self._populate_shared_plugins(workspace)
        self._assert_no_test_contamination(workspace, task_dir)
        if os.environ.get("GAMEDEVBENCH_UE_DISABLE_UNREAL_MCP") == "1":
            self._disable_unreal_mcp_plugin(workspace)
        self._patch_ddc_path(workspace)
        return workspace

    def _prepare_retest_workspace(self, source_dir: Path, task_dir: Path, label: str,
                                  dest: Optional[Path] = None) -> Path:
        """Create a workspace for a retest iteration.

        With dest=None (default) a fresh timestamped workspace is created. With
        dest given (the warm base), that exact path is used - so the warm base is
        prepared through the identical, proven code path as a cold retest build.
        """
        if dest is None:
            run_id = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
            root = self._workspace_root() / "ws"
            root.mkdir(parents=True, exist_ok=True)
            safe_label = self._sanitize_workspace_label(label)
            task_label = self._task_workspace_label(task_dir)
            workspace = root / f"{task_label}_{safe_label}_{run_id}"
        else:
            dest.parent.mkdir(parents=True, exist_ok=True)
            if dest.exists():
                self._safe_rmtree(dest)
            workspace = dest
        shutil.copytree(
            source_dir,
            workspace,
            ignore=shutil.ignore_patterns(
                "._*",
                ".DS_Store",
                "result.json",
                "task_config.json",
                "agent_trajectory.log",
                "compile_output.txt",
                "unreal_log.txt",
            ),
        )
        self._remove_macos_sidecar_files(workspace)
        self._clean_generated_artifacts(workspace)
        self._populate_shared_plugins(workspace)
        self._assert_no_test_contamination(workspace, task_dir)
        if os.environ.get("GAMEDEVBENCH_UE_DISABLE_UNREAL_MCP") == "1":
            self._disable_unreal_mcp_plugin(workspace)
        self._patch_ddc_path(workspace)
        return workspace

    def _patch_ddc_path(self, workspace: Path) -> None:
        """Replace %GAMEDIR%DerivedDataCache/ with a short fixed path.

        UE 5.7 crashes at startup if the LocalDerivedDataCache path exceeds 119
        characters. Workspace paths under /tmp/<run-id>/ routinely exceed that
        limit on macOS. Tasks extracted from older Bomber commits still carry the
        default %GAMEDIR% placeholder; this patch makes every workspace safe.
        """
        ini = workspace / "Config" / "DefaultEngine.ini"
        if not ini.exists():
            return
        text = ini.read_text(encoding="utf-8")
        patched = text.replace(
            "Path=%GAMEDIR%DerivedDataCache/",
            "Path=/tmp/ue_ddc/bomber/",
        )
        if patched != text:
            ini.write_text(patched, encoding="utf-8")

    def _disable_unreal_mcp_plugin(self, workspace: Path) -> None:
        """Remove the UnrealMCP project plugin from the copied workspace.

        This is a debugging aid to isolate whether editor startup failures are
        caused by the benchmark project module or by the bundled UnrealMCP plugin.
        """
        uproject_path = self._find_uproject(workspace)
        if not uproject_path:
            return

        data = json.loads(uproject_path.read_text())
        plugins = data.get("Plugins", [])
        data["Plugins"] = [p for p in plugins if p.get("Name") != "UnrealMCP"]
        uproject_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")

        plugin_dir = workspace / "Plugins" / "UnrealMCP"
        if plugin_dir.exists():
            shutil.rmtree(plugin_dir)

    # -- MCP server config -----------------------------------------------------

    def _build_mcp_servers(self) -> Optional[dict]:
        """Build the mcp_servers dict for the solver from unreal_config.yaml.

        Returns None if the MCP server is not configured, in which case the solver
        runs without MCP and the task is treated as code_only for this run.
        """
        mcp_cfg = self.ue_cfg.get("mcp", {})
        server_command = mcp_cfg.get("server_command")
        server_args = mcp_cfg.get("server_args", [])
        server_cwd = mcp_cfg.get("server_cwd")
        if not server_command:
            return None
        cfg: dict = {
            "type": "stdio",
            "command": server_command,
            "args": server_args,
        }
        if server_cwd:
            cfg["cwd"] = server_cwd
        return {"unreal": cfg}

    # -- Per-task runner -------------------------------------------------------

    def run_task(self, task_dir: Path) -> dict:
        task_config = self.load_task_config(task_dir)
        if not task_config:
            return {
                "task_id": task_dir.name,
                "task_name": task_dir.name,
                "capability_mode": "unknown",
                "agent": self.agent,
                "model": self.model,
                "timestamp": datetime.now(timezone.utc).isoformat(),
                "solver_success": False,
                "solver_message": None,
                "solver_rate_limited": False,
                "structural_validation_passed": False,
                "structural_validation_output": "",
                "compile_success": None,
                "files_edited": [],
                "forbidden_touched": [],
                "editor_actions_taken": [],
                "failure_reason": "task_config_load_failed",
                "error": "Could not load task_config.json",
                "token_usage": None,
                "cost_usd": None,
                "duration_seconds": None,
                "agent_steps": None,
            }

        task_id = task_config.get("task_id", task_dir.name)
        task_name = task_config.get("task_name", task_dir.name)
        capability_mode = task_config.get("capability_mode", "code_only")

        print(f"\n{'='*60}")
        print(f"Task: {task_id}  [{capability_mode}]")
        print(f"{'='*60}")

        if capability_mode == "ue_automation":
            return self._run_ue_automation(task_dir, task_config, task_id, task_name)

        if capability_mode == "hybrid":
            if self._build_mcp_servers() is None:
                print(
                    "  WARNING: MCP not configured (server_command is null in unreal_config.yaml)"
                )
                print("  Running code_only portion; editor actions skipped")
                record = self._run_code_only(task_dir, task_config, task_id, task_name)
                record["capability_mode"] = "hybrid"
                record["failure_reason"] = "mcp_not_configured"
                return record
            return self._run_hybrid(task_dir, task_config, task_id, task_name)
        return self._run_code_only(task_dir, task_config, task_id, task_name)

    # -- ue_automation path ----------------------------------------------------

    @staticmethod
    def _parse_automation_results(log_text: str, test_filter: str) -> list[dict]:
        """Extract per-test pass/fail from a UE automation run log.

        UE5 writes lines like:
          LogAutomationController: Display: Test Completed. Result={Success} Name={TestName} Path={...}
          LogAutomationController: Error: Test Completed. Result={Fail} Name={TestName} Path={...}
          LogAutomationController: Automation Test Failed: TestName  (older format)
        and a summary block at the end.
        """
        import re

        results = []
        seen = set()

        # UE5 format: Result={Success} Name={TestName} Path={...}
        completed_ue5_re = re.compile(
            r"LogAutomationController.*Test Completed\. Result=\{(\w+)\}\s+Name=\{([^}]+)\}"
        )
        # Legacy format: Result=Passed, Test=TestName
        completed_re = re.compile(
            r"LogAutomationController.*Test Completed\. Result=(\w+),.*Test=(.+?)(?:,|$)"
        )
        failed_re = re.compile(
            r"LogAutomationController.*Automation Test Failed: (.+)"
        )
        passed_re = re.compile(r"LogAutomationController.*\[PASSED\] (.+)")
        # UE5 format: 'TestName' ... passed/failed
        quoted_re = re.compile(
            r"LogAutomationController.*'("
            + re.escape(test_filter)
            + r"[^']*)'.*(passed|failed)",
            re.IGNORECASE,
        )

        for line in log_text.splitlines():
            m = completed_ue5_re.search(line)
            if m:
                status, name = m.group(1).lower(), m.group(2).strip()
                if name not in seen:
                    seen.add(name)
                    # UE5 uses "success" for pass and "fail" for failure
                    results.append({"id": name, "passed": status == "success"})
                continue

            m = completed_re.search(line)
            if m:
                status, name = m.group(1).lower(), m.group(2).strip()
                if name not in seen:
                    seen.add(name)
                    results.append({"id": name, "passed": status == "passed"})
                continue

            m = quoted_re.search(line)
            if m:
                name, status = m.group(1).strip(), m.group(2).lower()
                if name not in seen:
                    seen.add(name)
                    results.append({"id": name, "passed": status == "passed"})
                continue

            m = failed_re.search(line)
            if m:
                name = m.group(1).strip()
                if name not in seen:
                    seen.add(name)
                    results.append({"id": name, "passed": False})
                continue

            m = passed_re.search(line)
            if m:
                name = m.group(1).strip()
                if name not in seen:
                    seen.add(name)
                    results.append({"id": name, "passed": True})

        return results

    @staticmethod
    def _extract_declared_subchecks(test_file: Optional[Path]) -> dict[str, list[dict]]:
        """Recover assertion-level checks from batched hidden test source.

        Later Unreal tasks often batch many requirement checks into a single
        `RuntimeBehaviors` automation test for stability. This parser extracts
        the underlying B*/G* assertions from the hidden test source so result
        artifacts can expose more than just the top-level bucket names.
        """
        if test_file is None or not test_file.exists():
            return {}

        text = test_file.read_text(encoding="utf-8", errors="ignore")
        fn_re = re.compile(
            r"bool\s+\w+_(CDOChecks|RuntimeBehaviors)::RunTest\s*\([^)]*\)\s*\{",
            re.MULTILINE,
        )
        msg_re = re.compile(r'TEXT\("((?:[^"\\]|\\.)*)"\)')
        subcheck_re = re.compile(r"((?:B|G)\d+):\s*(.+)")
        matches = list(fn_re.finditer(text))
        if not matches:
            return {}

        def _unescape(raw: str) -> str:
            return raw.replace(r"\"", '"').replace(r"\n", " ").replace(r"\t", " ")

        subchecks_by_test: dict[str, list[dict]] = {}
        for idx, match in enumerate(matches):
            test_id = match.group(1)
            start = match.end()
            end = matches[idx + 1].start() if idx + 1 < len(matches) else len(text)
            body = text[start:end]
            seen: set[tuple[str, str]] = set()
            subchecks: list[dict] = []
            for raw_msg in msg_re.findall(body):
                msg = _unescape(raw_msg).strip()
                sub = subcheck_re.match(msg)
                if not sub:
                    continue
                sub_id = sub.group(1).strip()
                label = sub.group(2).strip()
                key = (sub_id, label)
                if key in seen:
                    continue
                seen.add(key)
                subchecks.append(
                    {
                        "id": sub_id,
                        "message": label,
                        "status": "unknown",
                    }
                )
            if subchecks:
                subchecks_by_test[test_id] = subchecks

        return subchecks_by_test

    @staticmethod
    def _extract_failed_subchecks_from_log(log_text: str) -> set[tuple[str, str]]:
        """Extract failed B*/G* checks from Unreal automation log lines."""
        failed: set[tuple[str, str]] = set()
        quoted_re = re.compile(r"'((?:B|G)\d+):\s*([^']+)'")
        plain_re = re.compile(r"((?:B|G)\d+):\s*(.+)")

        for line in log_text.splitlines():
            if "LogAutomationController: Error:" not in line:
                continue

            matched = False
            for match in quoted_re.finditer(line):
                failed.add((match.group(1).strip(), match.group(2).strip()))
                matched = True

            if not matched:
                plain = plain_re.search(line)
                if plain:
                    failed.add((plain.group(1).strip(), plain.group(2).strip()))

        return failed

    def _attach_subchecks(
        self,
        test_results: list[dict],
        log_text: str,
        test_file: Optional[Path],
    ) -> dict:
        """Attach subchecks to top-level test results and compute summary stats.

        Conservative policy:
        - If a top-level test passed, all declared subchecks under it are marked passed.
        - If a top-level test failed, only subchecks explicitly observed as failed in
          the log are marked failed; the rest remain unknown instead of being assumed
          passed.
        """
        declared = self._extract_declared_subchecks(test_file)
        failed = self._extract_failed_subchecks_from_log(log_text)

        total = passed = failed_count = unknown = 0
        for test in test_results:
            subchecks = declared.get(test["id"])
            if not subchecks:
                continue

            enriched: list[dict] = []
            for sub in subchecks:
                status = "unknown"
                if test["passed"]:
                    status = "passed"
                elif (sub["id"], sub["message"]) in failed:
                    status = "failed"

                enriched.append(
                    {
                        "id": sub["id"],
                        "message": sub["message"],
                        "status": status,
                    }
                )

                total += 1
                if status == "passed":
                    passed += 1
                elif status == "failed":
                    failed_count += 1
                else:
                    unknown += 1

            test["subchecks"] = enriched

        return {
            "top_level_tests_run": len(test_results),
            "subchecks_run": total,
            "subchecks_passed": passed,
            "subchecks_failed": failed_count,
            "subchecks_unknown": unknown,
        }

    def _build_test_cmd(
        self, uproject_path: Path, test_filter: str, num_clients: int
    ) -> list:
        """Build the headless automation-test command (listen-server mode).

        Single source of truth for the test invocation, shared by
        _run_headless_tests and the make_workspace helper. Includes the
        arch -arm64 prefix on Apple Silicon.
        """
        cmd = [
            self.ue_cfg.get("editor_binary", ""),
            str(uproject_path.resolve()),
            f"-ExecCmds=Automation RunTests {test_filter}; Quit",
            "-Unattended",
            "-NullRHI",
            "-NoSplash",
            f"-NumClients={num_clients}",
            "-ListenServer",
        ]
        if sys.platform == "darwin" and os.uname().machine == "arm64":
            cmd = ["/usr/bin/arch", "-arm64", *cmd]
        return cmd

    def _run_headless_tests(
        self, uproject_path: Path, test_filter: str, num_clients: int
    ) -> tuple[bool, list[dict], str, bool]:
        """Run UE automation tests headless in listen server mode.

        Returns (any_ran: bool, test_results: list[dict], log_text: str, crashed: bool).
        crashed=True means the UE process exited via a fatal signal (SIGBUS, SIGSEGV, etc.)
        before tests could complete - distinct from a timeout or clean test failure.
        """
        editor_binary = self.ue_cfg.get("editor_binary", "")
        if not editor_binary or not Path(editor_binary).exists():
            return False, [], "editor_binary not configured or not found", False

        cmd = self._build_test_cmd(uproject_path, test_filter, num_clients)

        if self.debug:
            print(f"  Headless test run: {' '.join(cmd)}")

        timed_out = False
        returncode = 0
        log_text = ""
        test_timeout = int(self.ue_cfg.get("test_timeout_seconds", 1800))
        try:
            proc = subprocess.run(
                cmd,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                timeout=test_timeout,
            )
            returncode = proc.returncode
        except subprocess.TimeoutExpired:
            timed_out = True

        # Prefer the workspace-local on-disk log (cross-platform) over the
        # per-user log. The workspace log cannot be polluted by a concurrent
        # editor instance for the same project name, which matters when the
        # benchmark runs on shared/CI hardware.
        log_path = self._resolve_unreal_log(uproject_path)
        if log_path is not None:
            log_text = log_path.read_text(encoding="utf-8", errors="ignore")

        # Detect crash: negative returncode = killed by signal (Unix), or
        # "Critical error" in the log indicates a fatal UE crash.
        crashed = (not timed_out and returncode < 0) or (
            "=== Critical error: ===" in log_text
        )

        test_results = self._parse_automation_results(log_text, test_filter)

        # If we have parsed results, the run succeeded even if the subprocess timed out
        # (editor failed to quit but tests already completed).
        if test_results:
            return True, test_results, log_text, False
        if not log_text.strip():
            return False, [], "No Unreal automation log captured from this run", False
        if (
            f"Automation RunTests {test_filter}" in log_text
            or f"RunTests='{test_filter}'" in log_text
            or "LogAutomationController: Display: Test Started." in log_text
        ):
            return False, [], (
                "UE ran automation tests but the runner parsed zero results. "
                "This is a harness error; inspect UnrealEditor.log parsing."
            ), False
        if timed_out:
            return False, [], log_text or f"Headless test run timed out after {test_timeout}s", False
        return True, test_results, log_text, crashed

    def _run_ue_automation(
        self, task_dir: Path, task_config: dict, task_id: str, task_name: str
    ) -> dict:
        """ue_automation execution path:
        1. Copy workspace from start/
        2. Invoke solver - model edits files in workspace
        3. Inject hidden tests for evaluation only
        4. Compile
        5. Run UE automation tests headless in listen server mode
        6. Collect per-test results and derive task_passed
        """
        workspace = None
        original_dir = Path.cwd()
        error_msg = None
        solver_result = None
        solver_success = False
        compile_success = None
        compile_output = ""
        failure_reason = None
        test_results: list[dict] = []
        tests_run = 0
        tests_passed_count = 0
        tests_failed_count = 0
        task_passed = False
        unreal_log = ""

        tests_cfg = task_config.get("tests", {})
        test_filter = tests_cfg.get("filter", "")
        num_clients = tests_cfg.get("num_clients", 1)

        try:
            print("  [1/4] Creating workspace...")
            workspace = self._create_workspace(task_dir)

            uproject_path = self._find_uproject(workspace)
            if uproject_path is None:
                failure_reason = "compile_failed"
                raise RuntimeError("No .uproject found in workspace")

            # Invoke solver
            os.chdir(workspace)
            print(f"  [2/4] Running solver with {self.agent}...")
            solver = SolverFactory.create_solver(
                agent=self.agent,
                model=self.model,
                debug=self.debug,
                timeout_seconds=self.solver_timeout,
                reasoning_level=self.reasoning_level,
            )
            solver_result = self._run_with_heartbeat("Solver", solver.solve_task)
            solver_success = solver_result.success
            if self.debug:
                print(f"  Solver: {'success' if solver_success else 'failed'}")

        except Exception as e:
            solver_success = False
            error_msg = str(e)
            print(f"  ERROR during solve: {e}")
        finally:
            os.chdir(original_dir)

        if workspace and workspace.exists() and failure_reason is None:
            # Evaluation root: normally the solver workspace, but with --warm-cache
            # we overlay the solver's changes onto the persistent warm base and
            # compile/test there (incremental). The solver workspace is still
            # preserved for snapshot/judge/_extract_edits.
            eval_root = workspace
            if self.warm_cache:
                base = self._get_or_build_warm_base(
                    task_dir, list(task_config.get("files_to_edit", []))
                )
                if base is not None:
                    self._overlay_changes_onto_base(
                        base, workspace, task_dir,
                        list(task_config.get("files_to_edit", [])), "model",
                    )
                    eval_root = base
                else:
                    print("  Warm cache unavailable - cold compiling solver workspace")

            uproject_path = self._find_uproject(eval_root)

            print("  [3/4] Injecting hidden tests for evaluation...")
            self._inject_tests_for_evaluation(eval_root, task_dir)
            # Compile
            print("  [4/4] Compiling task...")
            compile_success, compile_output = self._run_with_heartbeat(
                "Compile",
                lambda: self._compile_project(uproject_path),
            )
            (eval_root / "compile_output.txt").write_text(
                compile_output, encoding="utf-8"
            )
            if eval_root is not workspace:
                (workspace / "compile_output.txt").write_text(
                    compile_output, encoding="utf-8"
                )

            if not compile_success:
                failure_reason = "compile_failed"
                print("  Compile FAILED - skipping tests")
            else:
                print("  Compile OK")

                if not test_filter:
                    print("  WARNING: no test filter in spec - skipping test run")
                    failure_reason = "tests_failed"
                else:
                    print(f"  Running tests: {test_filter} ...")
                    ran, test_results, unreal_log, ue_crashed = self._run_with_heartbeat(
                        "Test run",
                        lambda: self._run_headless_tests(
                            uproject_path, test_filter, num_clients
                        ),
                    )
                    (workspace / "unreal_log.txt").write_text(
                        unreal_log, encoding="utf-8"
                    )

                    if ue_crashed:
                        failure_reason = "crash"
                        print("  UE process crashed (SIGBUS/SIGSEGV) - model code caused a fatal error")
                    elif not ran:
                        failure_reason = "test_runner_error"
                        print(f"  Test runner error: {unreal_log[:200]}")
                    else:
                        tests_run = len(test_results)
                        tests_passed_count = sum(1 for t in test_results if t["passed"])
                        tests_failed_count = tests_run - tests_passed_count
                        task_passed = tests_failed_count == 0 and tests_run > 0
                        if not task_passed:
                            failure_reason = "tests_failed"
                        print(
                            f"  Tests: {tests_passed_count}/{tests_run} passed"
                            + (" - TASK PASSED" if task_passed else " - TASK FAILED")
                        )

        files_edited, _ = [], []
        if workspace and workspace.exists():
            files_edited, _ = self._extract_edits(workspace, task_dir, task_config)

        coverage_summary = {
            "top_level_tests_run": len(test_results),
            "subchecks_run": 0,
            "subchecks_passed": 0,
            "subchecks_failed": 0,
            "subchecks_unknown": 0,
        }
        if test_results and unreal_log:
            coverage_summary = self._attach_subchecks(
                test_results,
                unreal_log,
                self._find_task_test_file(task_dir),
            )

        token_usage, cost_usd, duration_seconds, agent_steps = self._extract_solver_metrics(
            solver_result
        )
        timestamp_str = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
        reasoning_requested = getattr(
            solver_result, "reasoning_level_requested", self.reasoning_level
        )
        reasoning_applied = getattr(
            solver_result, "reasoning_level_applied", "default"
        )
        judge_model = self._judge_model_for(
            model=getattr(solver_result, "model", self.model)
        )

        record = {
            "task_id": task_id,
            "task_name": task_name,
            "capability_mode": "ue_automation",
            "agent": self.agent,
            "model": getattr(solver_result, "model", self.model) if solver_result else self.model,
            "reasoning_level_requested": reasoning_requested,
            "reasoning_level_applied": reasoning_applied,
            "judge_model": judge_model,
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "solver_success": solver_success,
            "solver_message": getattr(solver_result, "message", None) if solver_result else None,
            "solver_rate_limited": bool(getattr(solver_result, "is_rate_limited", False)) if solver_result else False,
            "compile_success": compile_success,
            "tests_run": tests_run,
            "tests_passed": tests_passed_count,
            "tests_failed": tests_failed_count,
            "task_passed": task_passed,
            "test_results": test_results,
            "coverage_summary": coverage_summary,
            "files_edited": files_edited,
            "failure_reason": failure_reason,
            "error": error_msg,
            "token_usage": token_usage,
            "token_usage_is_estimate": bool(getattr(solver_result, "token_usage_is_estimate", False)) if solver_result else False,
            "cost_usd": cost_usd,
            "cost_usd_is_estimate": bool(getattr(solver_result, "cost_usd_is_estimate", False)) if solver_result else False,
            "cost_estimate_note": getattr(solver_result, "cost_estimate_note", None) if solver_result else None,
            "duration_seconds": duration_seconds,
            "agent_steps": agent_steps,
            "agent_steps_source": getattr(solver_result, "agent_steps_source", None) if solver_result else None,
        }

        if workspace and workspace.exists():
            snapshot_dir = self._save_test_result(
                workspace,
                task_dir,
                task_id,
                task_name,
                timestamp_str,
                record,
                solver_result,
                compile_output=compile_output,
                unreal_log=unreal_log,
            )
            if self.run_judge:
                print("  Running LLM-as-judge...")
                try:
                    judge_result = run_unreal_judge(
                        snapshot_dir=snapshot_dir,
                        task_instruction=task_config.get("instruction", ""),
                        files_to_edit=task_config.get("files_to_edit", []),
                        solution_dir=task_dir / "solution",
                        test_file=self._find_task_test_file(task_dir),
                        hidden_requirements=task_config.get("hidden_requirements", {}),
                        test_requirements=task_config.get("test_requirements", {}),
                        judge_model=judge_model,
                    )
                    if not judge_result.get("skipped"):
                        record["judge"] = judge_result.get("persisted_judge", {})
                        print("  Judge OK")
                    else:
                        print(f"  Judge skipped: {judge_result.get('reason', 'unknown')}")
                except Exception as judge_exc:
                    # The judge is a secondary signal and the test result is already
                    # saved. A judge failure (e.g. unparseable model output) must
                    # never abort a multi-task run - record it and move on.
                    record["judge_error"] = str(judge_exc)
                    print(f"  WARNING: judge failed - continuing without it: {judge_exc}")
            # The saved snapshot keeps a sanitized project tree at the snapshot root, so the
            # workspace itself is safely disposable even on benchmark-outcome
            # failures (compile_failed, tests_failed). Only an uncaught
            # exception skipping this block would leave a workspace behind
            # for post-mortem debugging.
            self._safe_rmtree(workspace)

        return record

    # -- code_only path --------------------------------------------------------

    def _run_code_only(
        self, task_dir: Path, task_config: dict, task_id: str, task_name: str
    ) -> dict:
        workspace = None
        original_dir = Path.cwd()
        error_msg = None
        solver_result = None
        solver_success = False

        try:
            print("  [1/3] Creating workspace...")
            workspace = self._create_workspace(task_dir)

            os.chdir(workspace)
            print(f"  [2/3] Running solver with {self.agent}...")
            solver = SolverFactory.create_solver(
                agent=self.agent,
                model=self.model,
                debug=self.debug,
                timeout_seconds=self.solver_timeout,
                reasoning_level=self.reasoning_level,
            )
            solver_result = self._run_with_heartbeat("Solver", solver.solve_task)
            solver_success = solver_result.success

            if self.debug:
                print(f"  Solver: {'success' if solver_success else 'failed'}")

        except Exception as e:
            solver_success = False
            error_msg = str(e)
            print(f"  ERROR during solve: {e}")
        finally:
            os.chdir(original_dir)

        files_edited, forbidden_touched = [], []
        if workspace and workspace.exists():
            files_edited, forbidden_touched = self._extract_edits(
                workspace, task_dir, task_config
            )
            if forbidden_touched:
                print(f"  WARNING: forbidden files touched: {forbidden_touched}")

        structural_validation_passed = False
        structural_validation_output = ""
        if workspace and workspace.exists():
            print("  [3/3] Running structural validation...")
            structural_validation_passed, structural_validation_output = self._validate(
                task_dir, task_config, workspace
            )
            print(
                f"  Validation: {'PASSED' if structural_validation_passed else 'FAILED'}"
            )
            if not structural_validation_passed and self.debug:
                print(f"  {structural_validation_output.strip()}")

        token_usage, cost_usd, duration_seconds, agent_steps = self._extract_solver_metrics(
            solver_result
        )
        timestamp_str = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
        reasoning_requested = getattr(
            solver_result, "reasoning_level_requested", self.reasoning_level
        )
        reasoning_applied = getattr(
            solver_result, "reasoning_level_applied", "default"
        )

        record = {
            "task_id": task_id,
            "task_name": task_name,
            "capability_mode": "code_only",
            "agent": self.agent,
            "model": getattr(solver_result, "model", self.model) if solver_result else self.model,
            "reasoning_level_requested": reasoning_requested,
            "reasoning_level_applied": reasoning_applied,
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "solver_success": solver_success,
            "solver_message": getattr(solver_result, "message", None) if solver_result else None,
            "solver_rate_limited": bool(getattr(solver_result, "is_rate_limited", False)) if solver_result else False,
            "structural_validation_passed": structural_validation_passed,
            "structural_validation_output": structural_validation_output,
            "compile_success": None,
            "files_edited": files_edited,
            "forbidden_touched": forbidden_touched,
            "editor_actions_taken": [],
            "failure_reason": None,
            "error": error_msg,
            "token_usage": token_usage,
            "token_usage_is_estimate": bool(getattr(solver_result, "token_usage_is_estimate", False)) if solver_result else False,
            "cost_usd": cost_usd,
            "cost_usd_is_estimate": bool(getattr(solver_result, "cost_usd_is_estimate", False)) if solver_result else False,
            "cost_estimate_note": getattr(solver_result, "cost_estimate_note", None) if solver_result else None,
            "duration_seconds": duration_seconds,
            "agent_steps": agent_steps,
            "agent_steps_source": getattr(solver_result, "agent_steps_source", None) if solver_result else None,
        }

        if workspace and workspace.exists():
            self._save_test_result(
                workspace,
                task_dir,
                task_id,
                task_name,
                timestamp_str,
                record,
                solver_result,
            )
            self._safe_rmtree(workspace)

        return record

    # -- Behavioral validation -------------------------------------------------

    def _behavioral_validate(
        self, task_dir: Path, workspace: Path
    ) -> tuple[bool, dict]:
        """Run the full validation pipeline (public + private) against the live editor.

        Called in _run_hybrid() after the solver returns but before the editor is closed
        so that private validators can still query actor state via the MCP bridge.

        Returns (passed: bool, report: dict) suitable for embedding in the task record.
        """
        yaml_path = task_dir / "task.yaml"
        if not yaml_path.exists():
            return False, {"skipped": True, "reason": "no task.yaml found"}

        try:
            schema = load_task_schema(task_dir)
        except Exception as exc:
            return False, {"skipped": True, "reason": f"schema load failed: {exc}"}

        try:
            from unrealbench.src.unreal_connection import UnrealConnection  # type: ignore

            conn = UnrealConnection("127.0.0.1", 55557)
            adapter = UnrealConnectionAdapter(conn)
        except Exception:
            # If UnrealConnection is unavailable, fall back to code_only validation
            adapter = None

        pub_reports, priv_reports = run_full_validation(
            schema, workspace, adapter=adapter, mode="hybrid"
        )

        passed = overall_passed(pub_reports, priv_reports)

        def _report_dict(r):
            return {
                "validator_type": r.validator_type,
                "passed": r.passed,
                "skipped": r.skipped,
                "skip_reason": r.skip_reason,
                "summary": r.summary,
                "failure_type": r.failure_type,
                "phase": r.phase,
            }

        return passed, {
            "passed": passed,
            "public": [_report_dict(r) for r in pub_reports],
            "private": [_report_dict(r) for r in priv_reports],
        }

    # -- hybrid path -----------------------------------------------------------

    def _run_hybrid(
        self, task_dir: Path, task_config: dict, task_id: str, task_name: str
    ) -> dict:
        """Phase 2A execution contract:
        1. Copy workspace from start/
        2. Compile - if this fails, stop and label compile_failed
        3. Open headed editor with MCP plugin active
        4. Invoke solver (agent edits text files and calls MCP tools during run)
        5. Close editor, collect UE log
        6. Collect editor_action_log.json, detect failures, save artifacts
        """
        workspace = None
        original_dir = Path.cwd()
        error_msg = None
        solver_result = None
        solver_success = False
        compile_success = None
        compile_output = ""
        unreal_log = ""
        editor_actions_taken = []
        failure_reason = None
        editor_proc = None
        behavioral_validation_passed = None
        behavioral_validation_report: dict = {}

        try:
            # Step 1: workspace
            print("  [1/5] Creating workspace...")
            workspace = self._create_workspace(task_dir)

            uproject_path = self._find_uproject(workspace)
            if uproject_path is None:
                failure_reason = "compile_failed"
                error_msg = "No .uproject found in workspace"
                compile_success = False
                raise RuntimeError(error_msg)

            # Step 2: compile
            print("  [2/5] Compiling task...")
            compile_success, compile_output = self._run_with_heartbeat(
                "Compile",
                lambda: self._compile_project(uproject_path),
            )
            (workspace / "compile_output.txt").write_text(
                compile_output, encoding="utf-8"
            )

            if not compile_success:
                failure_reason = "compile_failed"
                print("  Compile FAILED - skipping editor session")
            else:
                print("  Compile OK")

                # Step 3: open editor
                print("  [3/5] Opening editor with MCP plugin...")
                editor_proc = self._open_editor(uproject_path)
                if editor_proc is None:
                    failure_reason = "mcp_session_failed"
                    print("  ERROR: Editor did not start - skipping solver")
                else:
                    # Step 4: invoke solver
                    os.chdir(workspace)
                    print(f"  [4/5] Running solver with {self.agent}...")
                    mcp_servers = self._build_mcp_servers()
                    solver = SolverFactory.create_solver(
                        agent=self.agent,
                        model=self.model,
                        debug=self.debug,
                        use_mcp=mcp_servers is not None,
                        mcp_servers=mcp_servers,
                        reasoning_level=self.reasoning_level,
                    )
                    solver_result = self._run_with_heartbeat(
                        "Solver",
                        solver.solve_task,
                    )
                    solver_success = solver_result.success
                    if self.debug:
                        print(f"  Solver: {'success' if solver_success else 'failed'}")

                    # Behavioral validation - runs while editor is still open
                    print("  [5/5] Running behavioral validation...")
                    behavioral_validation_passed, behavioral_validation_report = self._run_with_heartbeat(
                        "Behavioral validation",
                        lambda: self._behavioral_validate(task_dir, workspace),
                    )
                    print(
                        f"  Behavioral validation: "
                        f"{'PASSED' if behavioral_validation_passed else 'FAILED'}"
                    )

        except Exception as e:
            solver_success = False
            error_msg = str(e)
            print(f"  ERROR during hybrid run: {e}")
        finally:
            os.chdir(original_dir)
            # Step 5: close editor, collect UE log
            if editor_proc is not None and workspace:
                unreal_log = self._close_editor(editor_proc, workspace)

        # Step 6: collect artifacts and classify failures
        if workspace and workspace.exists():
            action_log = self._collect_editor_action_log(workspace)
            editor_actions_taken = self._summarize_actions(action_log)

            if failure_reason is None:
                if not action_log:
                    failure_reason = "no_editor_actions_taken"
                elif not self._content_changed(workspace, task_dir):
                    failure_reason = "editor_actions_no_changes_detected"

        files_edited, forbidden_touched = [], []
        if workspace and workspace.exists():
            files_edited, forbidden_touched = self._extract_edits(
                workspace, task_dir, task_config
            )
            if forbidden_touched:
                print(f"  WARNING: forbidden files touched: {forbidden_touched}")

        structural_validation_passed = False
        structural_validation_output = ""
        if workspace and workspace.exists():
            structural_validation_passed, structural_validation_output = self._validate(
                task_dir, task_config, workspace
            )
            print(
                f"  Validation: {'PASSED' if structural_validation_passed else 'FAILED'}"
            )

        token_usage, cost_usd, duration_seconds, agent_steps = self._extract_solver_metrics(
            solver_result
        )
        timestamp_str = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
        reasoning_requested = getattr(
            solver_result, "reasoning_level_requested", self.reasoning_level
        )
        reasoning_applied = getattr(
            solver_result, "reasoning_level_applied", "default"
        )

        record = {
            "task_id": task_id,
            "task_name": task_name,
            "capability_mode": "hybrid",
            "agent": self.agent,
            "model": getattr(solver_result, "model", self.model) if solver_result else self.model,
            "reasoning_level_requested": reasoning_requested,
            "reasoning_level_applied": reasoning_applied,
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "solver_success": solver_success,
            "solver_message": getattr(solver_result, "message", None) if solver_result else None,
            "solver_rate_limited": bool(getattr(solver_result, "is_rate_limited", False)) if solver_result else False,
            "structural_validation_passed": structural_validation_passed,
            "structural_validation_output": structural_validation_output,
            "behavioral_validation_passed": behavioral_validation_passed,
            "behavioral_validation_report": behavioral_validation_report,
            "compile_success": compile_success,
            "files_edited": files_edited,
            "forbidden_touched": forbidden_touched,
            "editor_actions_taken": editor_actions_taken,
            "failure_reason": failure_reason,
            "error": error_msg,
            "workspace_path": str(workspace) if workspace else None,
            "token_usage": token_usage,
            "token_usage_is_estimate": bool(getattr(solver_result, "token_usage_is_estimate", False)) if solver_result else False,
            "cost_usd": cost_usd,
            "cost_usd_is_estimate": bool(getattr(solver_result, "cost_usd_is_estimate", False)) if solver_result else False,
            "cost_estimate_note": getattr(solver_result, "cost_estimate_note", None) if solver_result else None,
            "duration_seconds": duration_seconds,
            "agent_steps": agent_steps,
            "agent_steps_source": getattr(solver_result, "agent_steps_source", None) if solver_result else None,
        }

        if workspace and workspace.exists():
            self._save_test_result(
                workspace,
                task_dir,
                task_id,
                task_name,
                timestamp_str,
                record,
                solver_result,
                compile_output=compile_output,
                unreal_log=unreal_log,
            )

            # If editor startup failed, Unreal may still be finishing a handoff
            # to an already-running editor instance that is opening this project.
            # Deleting the live workspace here can make the editor show a generic
            # "game module could not be found" dialog because the .uproject and
            # compiled binaries disappear mid-startup. Preserve the workspace on
            # failure for inspection and to avoid breaking that late startup path.
            if failure_reason is None:
                self._safe_rmtree(workspace)
            elif self.debug:
                print(f"  Preserving failed workspace at {workspace}")

        return record

    # -- Shared helpers --------------------------------------------------------

    @staticmethod
    def _extract_solver_metrics(solver_result) -> tuple:
        token_usage = None
        cost_usd = None
        duration_seconds = None
        agent_steps = None
        if solver_result:
            duration_seconds = solver_result.duration_seconds
            cost_usd = solver_result.cost_usd
            agent_steps = getattr(solver_result, "agent_steps", None)
            if solver_result.token_usage:
                token_usage = solver_result.token_usage.to_dict()
        return token_usage, cost_usd, duration_seconds, agent_steps

    # Build artifacts and per-run editor state - discarded from saved
    # snapshots because they are large, regenerable, and irrelevant to
    # judging the solver's attempt. Tests/ would also leak the hidden
    # behavioural contract; that exclusion happens via _copy_attempt_dir.
    _ATTEMPT_PRUNE_DIRS = (
        "Binaries",
        "Build",
        "Intermediate",
        "Saved",
        "DerivedDataCache",
    )

    def _copy_attempt_dir(
        self,
        workspace: Path,
        dest: Path,
        task_dir: Path,
    ) -> None:
        """Copy the solver's evaluated workspace into ``dest`` with hidden-test
        and build-artifact contamination removed.

        The injected ``Source/<Project>/Tests/`` tree is omitted entirely.
        ``Build.cs`` is reverted to remove the AutomationController patch added
        for evaluation. Generated build artefacts (Binaries/Intermediate/Saved/...)
        are skipped because they are bulky and regenerable.
        """
        uproject = self._find_uproject(workspace)
        project_name = uproject.stem if uproject else None
        prune_dirs = set(self._ATTEMPT_PRUNE_DIRS)
        injected_tests_parent = (
            workspace / "Source" / project_name if project_name else None
        )

        # Read the shared-plugin manifest written by _populate_shared_plugins so
        # we can omit those plugin directories from result snapshots.  Snapshots
        # only need the task-specific (solver-edited) plugin source.
        shared_plugin_names: set[str] = set()
        manifest_path = workspace / self._SHARED_PLUGINS_MANIFEST
        if manifest_path.exists():
            try:
                shared_plugin_names = set(
                    json.loads(manifest_path.read_text(encoding="utf-8"))
                )
            except Exception:
                pass

        def _ignore(dirpath: str, names: list[str]) -> list[str]:
            current = Path(dirpath)
            ignored: list[str] = []
            if current == workspace:
                ignored.extend(n for n in names if n in prune_dirs)
                ignored.extend(
                    n
                    for n in names
                    if n
                    in {
                        "result.json",
                        "compile_output.txt",
                        "unreal_log.txt",
                        "agent_trajectory.log",
                        "start",
                        self._SHARED_PLUGINS_MANIFEST,
                    }
                )
            if injected_tests_parent is not None and current == injected_tests_parent:
                ignored.extend(n for n in names if n == "Tests")
            # Exclude shared plugins from snapshots - only the task-specific plugin
            # (solver-edited) needs to be preserved for judging and retest.
            if current == workspace / "Plugins" and shared_plugin_names:
                ignored.extend(n for n in names if n in shared_plugin_names)
            # Per-plugin generated state.
            if current.parent.name == "Plugins":
                # No-op; we still want plugin source. Plugin Binaries/Intermediate/Saved
                # are pruned at the plugin-root level below.
                pass
            if (
                current.name
                and current.parent.parent
                and current.parent.parent.name == "Plugins"
            ):
                ignored.extend(n for n in names if n in prune_dirs)
            return ignored

        shutil.copytree(workspace, dest, ignore=_ignore, dirs_exist_ok=True)

        if project_name:
            self._unpatch_build_cs_for_tests(
                dest / "Source" / project_name / f"{project_name}.Build.cs"
            )

    def _write_trajectory_file(
        self,
        path: Path,
        workspace: Optional[Path],
        task_name: str,
        solver_result,
    ) -> None:
        display_model = self.model
        if solver_result and getattr(solver_result, "model", None):
            display_model = solver_result.model
        with path.open("w", encoding="utf-8") as f:
            f.write(f"Task: {task_name}\n")
            f.write(f"Agent: {self.agent}\n")
            f.write(f"Model: {display_model}\n")
            if workspace is not None:
                f.write(f"Workspace: {workspace}\n")
            f.write(f"Timestamp: {datetime.now().isoformat()}\n")
            f.write("=" * 80 + "\n\n")
            if solver_result:
                f.write(f"Success: {solver_result.success}\n")
                f.write(f"Message: {solver_result.message}\n")
                f.write(f"Duration: {solver_result.duration_seconds:.2f}s\n\n")
                f.write("STDOUT:\n")
                f.write(solver_result.stdout or "")
                f.write("\n\nSTDERR:\n")
                f.write(solver_result.stderr or "")

    @staticmethod
    def _find_task_test_file(task_dir: Path) -> Optional[Path]:
        test_files = sorted((task_dir / "tests").glob("*.cpp"))
        if len(test_files) != 1:
            return None
        return test_files[0]

    def _save_test_result(
        self,
        workspace: Path,
        task_dir: Path,
        task_id: str,
        task_name: str,
        timestamp_str: str,
        record: dict,
        solver_result,
        compile_output: str = "",
        unreal_log: str = "",
    ) -> Path:
        """Save sanitized run artifacts to ``<results_root>/<task>_<agent>_<ts>/``.

        Layout:
            result.json              - outcome record (also written to --output)
            compile_output.txt       - compile log (if applicable)
            unreal_log.txt           - UE automation log (if applicable)
            agent_trajectory.log     - solver stdout/stderr
            <project files>/         - solver workspace, scrubbed of injected
                                       Tests/ + reverted Build.cs + no build artefacts

        The injected hidden tests are NEVER copied into this snapshot, so
        the saved artifact is safe to retain alongside subsequent benchmark
        runs without leaking the behavioural contract.
        """
        result_root = self._default_results_root()
        result_subdir = result_root / f"{task_id}_{self.agent}_{timestamp_str}"
        result_subdir.mkdir(parents=True, exist_ok=True)

        with (result_subdir / "result.json").open("w") as f:
            json.dump(record, f, indent=2)

        if compile_output:
            (result_subdir / "compile_output.txt").write_text(
                compile_output, encoding="utf-8"
            )
        if unreal_log:
            (result_subdir / "unreal_log.txt").write_text(unreal_log, encoding="utf-8")

        self._write_trajectory_file(
            result_subdir / "agent_trajectory.log",
            workspace,
            task_name,
            solver_result,
        )

        if workspace.exists():
            self._copy_attempt_dir(workspace, result_subdir, task_dir)

        if self.debug:
            print(f"  Saved to {result_subdir}")
        return result_subdir

    # -- Main loop -------------------------------------------------------------

    def run_all(self, task_ids: Optional[list] = None):
        tasks = self.find_tasks()
        if not tasks:
            print(f"No ue_task_* directories found in {self.tasks_dir}")
            return

        if task_ids:
            tasks = [
                t
                for t in tasks
                if t.name in task_ids or any(t.name.startswith(tid) for tid in task_ids)
            ]
            if not tasks:
                print(f"No tasks matched: {task_ids}")
                return

        print(
            f"Found {len(tasks)} tasks. Completed (from resume): {len(self.completed)}"
        )

        for task_dir in tasks:
            config = self.load_task_config(task_dir)
            task_id = config.get("task_id", task_dir.name) if config else task_dir.name

            if task_id in self.completed:
                print(f"[SKIP] {task_id} - already completed")
                continue

            record = self.run_task(task_dir)
            self.results.append(record)
            self._write_results()

        print(f"\nDone. Results written to {self.output_file}")

    def _write_results(self):
        self.output_file.parent.mkdir(parents=True, exist_ok=True)
        with self.output_file.open("w") as f:
            json.dump(self.results, f, indent=2)

    # -- Warm compile cache ----------------------------------------------------
    # A persistent per-task workspace whose Intermediate/Binaries are kept warm.
    # A run overlays only the files that differ from the canonical (solution)
    # base and does an incremental compile, turning a ~25 min cold build into a
    # ~1-3 min relink. Opt-in via --warm-cache. UE bakes absolute paths into
    # generated headers, so the base must be reused IN PLACE at a stable path
    # (never copied), and same-task runs are therefore serialized.

    _WARM_PRUNE_DIRS = {"Intermediate", "Binaries", "Saved", "Build", ".git"}

    @staticmethod
    def _workspace_root() -> Path:
        # Windows MAX_PATH is 260; UBT generates filenames up to ~170 chars deep
        # inside the workspace, so the workspace prefix must stay under ~88 chars.
        # Use C:\ub\ (4 chars) so the full prefix comfortably fits.
        if sys.platform == "win32":
            return Path("C:/ub")
        return Path(tempfile.gettempdir())

    def _warm_cache_dir(self) -> Path:
        return self._workspace_root() / "wb"

    @staticmethod
    def _legacy_warm_task_label(task_label: str) -> Optional[str]:
        """Previous task label for warm bases created before the final reorder.

        Warm bases must be reused at their original filesystem path because UBT
        can bake absolute paths into generated build artifacts. When a final task
        is just a renumbered old task, use the old cache directory in place
        instead of renaming or copying it.
        """
        match = re.match(r"^ue_task_(\d{4})$", task_label)
        if not match:
            return None

        new_id = int(match.group(1))
        special = {
            23: 110,
            24: 111,
            25: 112,
            26: 113,
            27: 116,
            28: 119,
            29: 125,
            30: 126,
            31: 127,
            51: 114,
            52: 120,
            53: 121,
            71: 122,
            73: 115,
            74: 117,
            75: 118,
            88: 123,
            89: 124,
            109: 128,
            110: 129,
        }

        old_id = special.get(new_id)
        if old_id is None:
            if 8 <= new_id <= 22:
                old_id = new_id + 12
            elif 32 <= new_id <= 50:
                old_id = new_id + 3
            elif new_id == 72:
                old_id = 71
            elif 76 <= new_id <= 81:
                old_id = new_id - 4
            elif 82 <= new_id <= 87:
                old_id = new_id - 3
            elif 90 <= new_id <= 108:
                old_id = new_id - 5

        if old_id is None:
            return None
        return f"ue_task_{old_id:04d}"

    @staticmethod
    def _legacy_warm_source_labels() -> set[str]:
        labels: set[str] = set()
        for new_id in range(1, 111):
            legacy = UnrealBenchmarkRunner._legacy_warm_task_label(
                f"ue_task_{new_id:04d}"
            )
            if legacy:
                labels.add(legacy)
        return labels

    def _tree_fingerprint(self, root: Path, exclude_rel: set) -> str:
        """Stat-based (relpath,size,mtime_ns) fingerprint of a tree. Fast - no reads."""
        h = hashlib.sha256()
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = sorted(d for d in dirnames if d not in self._WARM_PRUNE_DIRS)
            for fn in sorted(filenames):
                fp = Path(dirpath) / fn
                rel = str(fp.relative_to(root))
                if rel in exclude_rel:
                    continue
                try:
                    st = fp.stat()
                except OSError:
                    continue
                h.update(f"{rel}|{st.st_size}|{st.st_mtime_ns}".encode())
        return h.hexdigest()

    def _iter_compile_surface_relpaths(
        self, root: Path, *, include_source_contents: bool
    ) -> list[str]:
        """Return compile-relevant files under ``root``.

        When ``include_source_contents`` is false, only build-graph files are
        included from Source/ trees (Build.cs/Target.cs). This is used for the
        warm-base key so ordinary code edits do not force a cold rebuild.

        When ``include_source_contents`` is true, all Source/ files are included.
        This is used for warm overlays so normal .cpp/.h edits still get copied
        onto the stable base and incrementally rebuilt.
        """
        rels: list[str] = []
        if not root.exists():
            return rels

        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = sorted(d for d in dirnames if d not in self._WARM_PRUNE_DIRS)
            dir_rel = Path(dirpath).relative_to(root)
            dir_parts = dir_rel.parts
            in_source_tree = "Source" in dir_parts
            in_config_tree = "Config" in dir_parts

            for fn in sorted(filenames):
                fp = Path(dirpath) / fn
                rel = str(fp.relative_to(root))

                if len(Path(rel).parts) == 1 and fn.endswith(".uproject"):
                    rels.append(rel)
                    continue

                if fn.endswith(".uplugin"):
                    rels.append(rel)
                    continue

                if in_config_tree:
                    rels.append(rel)
                    continue

                if not in_source_tree:
                    continue

                if include_source_contents or fn.endswith((".Build.cs", ".Target.cs")):
                    rels.append(rel)

        return rels

    def _selected_files_fingerprint(
        self, root: Path, relpaths: list[str], *, content_hash: bool
    ) -> str:
        """Fingerprint a selected set of files relative to ``root``."""
        h = hashlib.sha256()
        for rel in relpaths:
            fp = root / rel
            if not fp.exists():
                h.update(f"{rel}|missing".encode())
                continue
            if content_hash:
                h.update(f"{rel}|{self._file_hash(fp)}".encode())
            else:
                try:
                    st = fp.stat()
                except OSError:
                    h.update(f"{rel}|missing".encode())
                    continue
                h.update(f"{rel}|{st.st_size}|{st.st_mtime_ns}".encode())
        return h.hexdigest()

    @staticmethod
    def _file_hash(path: Path) -> str:
        try:
            return hashlib.sha256(path.read_bytes()).hexdigest()
        except OSError:
            return "missing"

    def _warm_cache_key(self, task_dir: Path, files_to_edit: list) -> str:
        """Key over everything that affects the compiled base EXCEPT files_to_edit."""
        h = hashlib.sha256()
        h.update(b"warm-v2")
        h.update(self.ue_cfg.get("engine_root", "").encode())
        h.update(repr(sorted(self._TARGETVECTOR_SHARED_PLUGIN_ALLOWLIST)).encode())
        h.update(repr(sorted(files_to_edit)).encode())
        # Canonical scaffold = solution build graph / workspace shape. Ordinary
        # code edits are overlaid onto the stable base and incrementally rebuilt,
        # so they should not force a cold warm-base rebuild.
        solution_relpaths = self._iter_compile_surface_relpaths(
            task_dir / "solution", include_source_contents=False
        )
        h.update(
            self._selected_files_fingerprint(
                task_dir / "solution", solution_relpaths, content_hash=True
            ).encode()
        )
        # frozen peer plugins
        tvp = Path(self.ue_cfg.get("tv_plugins_dir", ""))
        if tvp.is_dir():
            for name in sorted(self._TARGETVECTOR_SHARED_PLUGIN_ALLOWLIST):
                if (tvp / name).is_dir():
                    h.update(self._tree_fingerprint(tvp / name, set()).encode())
        # NOTE: the injected test is deliberately NOT part of the key. It is a
        # single isolated .cpp under Source/<Project>/Tests/ that is re-injected
        # and incrementally recompiled on every overlay, so editing it must NOT
        # force a full cold base rebuild - this is what makes test authoring fast.
        return h.hexdigest()[:16]

    def _warm_base_intact(self, base: Path) -> bool:
        """Cheap integrity check: project + game-module binary present.

        The compiled module lands in a platform-specific Binaries subdir with a
        platform-specific extension, so the check must match the build platform.
        """
        up = self._find_uproject(base)
        if not up:
            return False
        platform = self.ue_cfg.get("platform", self._detect_ue_platform())
        bin_subdir, ext = {
            "Mac": ("Mac", "dylib"),
            "Win64": ("Win64", "dll"),
            "Linux": ("Linux", "so"),
        }.get(platform, ("Mac", "dylib"))
        return bool(
            list((base / "Binaries" / bin_subdir).glob(f"*{up.stem}*.{ext}"))
        )

    def _materialize_base_workspace(self, task_dir: Path, dest: Path):
        """Build the warm base from solution/ at a FIXED path.

        The hidden test is deliberately NOT injected into the base: the base
        reflects only the stable reference solution, so a non-compiling test can
        never fail the cached base build (which would otherwise force a cold
        rebuild on every iteration). The test is added and compiled later as an
        incremental source add during the per-eval overlay
        (_overlay_changes_onto_base -> _inject_tests_for_evaluation).

        We still apply the test Build.cs dependency patch here so the base already
        links the test dependencies (AutomationController). Keeping the dependency
        set stable means the first overlay only recompiles the newly-added test
        .cpp and relinks, rather than rebuilding the whole module from a Build.cs
        change.
        """
        self._prepare_retest_workspace(task_dir / "solution", task_dir, "warmbase", dest=dest)
        uproject = self._find_uproject(dest)
        tests_src = task_dir / "tests"
        if uproject is not None and tests_src.exists() and any(tests_src.glob("*.cpp")):
            self._patch_build_cs_for_tests(dest, uproject.stem)
        return dest

    def _get_or_build_warm_base(self, task_dir: Path, files_to_edit: list):
        """Return a compiled warm base for the task, building it cold if needed.

        Returns the base Path, or None if warm caching can't be used (build
        failed) so the caller falls back to the normal cold path.
        """
        key = self._warm_cache_key(task_dir, files_to_edit)

        task_label = self._task_workspace_label(task_dir)
        memo_key = (task_label, key)

        # Built (or failed) already this invocation. Reuse the memoized result so
        # the base is never rebuilt per-baseline. Include the task label because
        # different tasks can share a build-graph key while requiring distinct
        # canonical source trees.
        if memo_key in self._warm_base_memo:
            base = self._warm_base_memo[memo_key]
            if base is not None:
                print(f"  Warm base MEMO HIT - reusing {base}")
            return base

        base = self._warm_cache_dir() / f"{task_label}_{key}"
        meta = base / ".ub_warm_base.json"

        if meta.exists():
            try:
                info = json.loads(meta.read_text())
            except Exception:
                info = {}
            if info.get("key") == key and self._warm_base_intact(base):
                print(f"  Warm base HIT - reusing {base}")
                self._warm_base_memo[memo_key] = base
                return base
            self._safe_rmtree(base)

        legacy_label = self._legacy_warm_task_label(task_label)
        if legacy_label and legacy_label != task_label:
            legacy_base = self._warm_cache_dir() / f"{legacy_label}_{key}"
            legacy_meta = legacy_base / ".ub_warm_base.json"
            if legacy_meta.exists():
                try:
                    legacy_info = json.loads(legacy_meta.read_text())
                except Exception:
                    legacy_info = {}
                if legacy_info.get("key") == key and self._warm_base_intact(legacy_base):
                    print(f"  Warm base LEGACY HIT - reusing {legacy_base}")
                    self._warm_base_memo[memo_key] = legacy_base
                    return legacy_base

        # Drop stale bases/logs for this task unless this label is also a
        # legacy source for another final task. In that case, deleting siblings
        # can remove a still-valid old-ID warm base used by the reorder shim.
        if task_label not in self._legacy_warm_source_labels():
            for sib in self._warm_cache_dir().glob(f"{task_label}_*"):
                self._safe_rmtree(sib)

        print(f"  Warm base MISS - building once (cold compile): {base.name}")
        try:
            self._materialize_base_workspace(task_dir, base)
        except Exception as exc:
            # Warm caching is an optimization - any setup failure (e.g. workspace
            # hydration error) must fall back to a cold build, never abort the run.
            print(f"  Warm base setup failed ({exc}) - falling back to cold builds")
            self._warm_base_memo[memo_key] = None
            return None
        up = self._find_uproject(base)
        if up is None:
            self._warm_base_memo[memo_key] = None
            return None
        ok, out = self._run_with_heartbeat(
            "Warm base compile", lambda: self._compile_project(up)
        )
        (base / "compile_output.txt").write_text(out, encoding="utf-8")
        if not ok:
            # Preserve the failed base AND its compile log for diagnosis - do NOT
            # delete it (the previous bug wiped the evidence). No meta is written,
            # so it is treated as invalid and rebuilt next time; memoize the
            # failure so this invocation falls back to cold without retrying.
            print("  Warm base FAILED to compile - falling back to cold builds")
            print(f"  Failed base preserved for diagnosis: {base / 'compile_output.txt'}")
            tail = "\n".join(out.strip().splitlines()[-30:]).strip()
            if tail:
                print(tail)
            self._warm_base_memo[memo_key] = None
            return None
        meta.write_text(
            json.dumps(
                {
                    "key": key,
                    "task": task_dir.name,
                    "files_to_edit": files_to_edit,
                    "built": datetime.now(timezone.utc).isoformat(),
                },
                indent=2,
            ),
            encoding="utf-8",
        )
        print(f"  Warm base ready: {base}")
        self._warm_base_memo[memo_key] = base
        return base

    def _read_shared_plugin_names(self, base: Path) -> set:
        """Names of shared/frozen plugins copied into the base from the TV volume.

        These live in the base but are deliberately absent from start/, solution/,
        and model snapshots, so they must be excluded from overlay reconciliation
        - otherwise the overlay would delete them from the base.
        """
        manifest = base / self._SHARED_PLUGINS_MANIFEST
        if not manifest.exists():
            return set()
        try:
            return set(json.loads(manifest.read_text(encoding="utf-8")))
        except Exception:
            return set()

    def _changed_files_vs_base(
        self, run_source: Path, base: Path, files_to_edit: list
    ) -> list:
        """Files to reconcile between a run source and the warm base.

        Bounded content diff over the project's compile surface. Considers
        files_to_edit, plus project Config/Source/, the .uproject, and every
        plugin Source/Config/.uplugin file - in BOTH the run source and the base.
        Including the base's surface is what lets the overlay delete files that
        exist only in the base (e.g. a non-editable file present in solution/ but
        not in the target), so the overlaid tree faithfully matches the target.

        Shared/frozen plugins are excluded: they exist only in the base and must
        be preserved, not deleted.
        """
        shared_plugins = self._read_shared_plugin_names(base)

        def _is_shared(rel: str) -> bool:
            parts = Path(rel).parts
            return (
                len(parts) >= 2
                and parts[0] == "Plugins"
                and parts[1] in shared_plugins
            )

        rel_candidates = set(str(f) for f in files_to_edit)
        for tree in (run_source, base):
            rel_candidates.update(
                self._iter_compile_surface_relpaths(
                    tree, include_source_contents=True
                )
            )
        rel_candidates = {r for r in rel_candidates if not _is_shared(r)}

        changed = []
        for rel in sorted(rel_candidates):
            a = run_source / rel
            b = base / rel
            if a.exists() != b.exists():
                changed.append(rel)
            elif a.exists() and self._file_hash(a) != self._file_hash(b):
                changed.append(rel)
        return changed

    def _overlay_changes_onto_base(
        self, base: Path, run_source: Path, task_dir: Path,
        files_to_edit: list, label: str,
    ) -> int:
        """Copy every file that differs from the base into the base (in place).

        Bumps mtime on each overlaid file so UBT recompiles exactly those, and
        re-injects the hidden tests (same content - keeps them present). Returns
        the number of files overlaid.
        """
        changed = self._changed_files_vs_base(run_source, base, files_to_edit)
        print(f"  [{label}] Warm overlay: {len(changed)} changed file(s)")
        for rel in changed:
            src = run_source / rel
            dst = base / rel
            if src.exists():
                dst.parent.mkdir(parents=True, exist_ok=True)
                if dst.exists():
                    try:
                        dst.chmod(0o666)
                    except OSError:
                        pass
                shutil.copy2(src, dst)
                os.utime(dst, None)  # bump mtime so UBT recompiles exactly this file
            elif dst.exists():
                try:
                    dst.chmod(0o666)
                except OSError:
                    pass
                dst.unlink()
        self._inject_tests_for_evaluation(base, task_dir)
        return len(changed)

    def _warm_eval(
        self,
        base: Path,
        run_source: Path,
        task_dir: Path,
        files_to_edit: list,
        test_filter: str,
        num_clients: int,
        label: str,
    ):
        """Overlay run-source changes onto the warm base, incremental-compile, test.

        Returns (compile_ok, test_results, unreal_log, compile_output) - same
        shape the cold path returns to _run_tests_from_dir.
        """
        self._overlay_changes_onto_base(base, run_source, task_dir, files_to_edit, label)

        up = self._find_uproject(base)
        print(f"  [{label}] Incremental compile (warm base)...")
        ok, out = self._run_with_heartbeat(
            f"Compile [{label}] warm", lambda: self._compile_project(up)
        )
        (base / "compile_output.txt").write_text(out, encoding="utf-8")
        if not ok:
            print(f"  [{label}] Warm incremental compile FAILED")
            return False, [], "", out

        print(f"  [{label}] Compile OK - running tests: {test_filter} ...")
        ran, results, unreal_log, crashed = self._run_with_heartbeat(
            f"Tests [{label}] warm",
            lambda: self._run_headless_tests(up, test_filter, num_clients),
        )
        if crashed:
            print(f"  [{label}] UE process crashed during warm test run")
        return True, results, unreal_log, out

    def _run_tests_from_dir(
        self,
        source_dir: Path,
        task_dir: Path,
        test_filter: str,
        num_clients: int,
        label: str,
    ) -> tuple[bool, list[dict], str, Path, str]:
        """Copy source_dir to a temp workspace, inject fresh tests, compile, run.

        Returns (compile_ok, test_results, unreal_log, workspace_path, compile_output).
        The caller is responsible for deleting the workspace (warm bases are
        skipped by the caller's cleanup - see the cleanup guards).
        """
        if self.warm_cache:
            cfg = self.load_task_config(task_dir) or {}
            files_to_edit = list(cfg.get("files_to_edit", []))
            base = self._get_or_build_warm_base(task_dir, files_to_edit)
            if base is not None:
                compile_ok, results, unreal_log, compile_out = self._warm_eval(
                    base, source_dir, task_dir, files_to_edit,
                    test_filter, num_clients, label,
                )
                if compile_ok:
                    return compile_ok, results, unreal_log, base, compile_out
                # A failed warm overlay compile is a real failure of this exact
                # source tree - tests are NOT run, so there is no stale-binary /
                # fabricated-results risk, and a cold rebuild would only fail the
                # same way after ~minutes. Report the compile failure directly.
                # This is what keeps test authoring fast: a non-compiling test
                # costs a quick incremental compile against the cached base, not a
                # full cold build. (A genuinely stale/corrupt base is rare; delete
                # it under the warm cache dir to force a clean rebuild.)
                print(
                    f"  [{label}] Warm overlay compile FAILED - reporting compile error"
                )
                return False, [], "", base, compile_out
            else:
                print(f"  [{label}] Warm cache unavailable - using cold build")

        print(f"\n  [{label}] Preparing workspace...")
        workspace = self._prepare_retest_workspace(source_dir, task_dir, label)

        uproject_path = self._find_uproject(workspace)
        if uproject_path is None:
            print(f"  [{label}] ERROR: no .uproject found")
            return False, [], "", workspace, ""

        print(f"  [{label}] Injecting current test files...")
        self._inject_tests_for_evaluation(workspace, task_dir)

        print(f"  [{label}] Compiling...")
        compile_ok, compile_out = self._run_with_heartbeat(
            f"Compile [{label}]",
            lambda: self._compile_project(uproject_path),
        )
        if compile_out:
            (workspace / "compile_output.txt").write_text(
                compile_out, encoding="utf-8"
            )
        if not compile_ok:
            print(f"  [{label}] Compile FAILED")
            tail = "\n".join(compile_out.strip().splitlines()[-40:]).strip()
            if tail:
                print(f"  [{label}] Compile output tail:")
                print(tail)
            print(f"  [{label}] Full compile log: {workspace / 'compile_output.txt'}")
            return False, [], "", workspace, compile_out

        print(f"  [{label}] Compile OK - running tests: {test_filter} ...")
        ran, test_results, unreal_log, ue_crashed = self._run_with_heartbeat(
            f"Tests [{label}]",
            lambda: self._run_headless_tests(uproject_path, test_filter, num_clients),
        )
        if ue_crashed:
            print(f"  [{label}] UE process crashed (SIGBUS/SIGSEGV)")
            return True, [], unreal_log, workspace, compile_out
        if not ran:
            print(f"  [{label}] Test runner error: {unreal_log[:200]}")
            return True, [], unreal_log, workspace, compile_out

        if unreal_log:
            (workspace / "unreal_log.txt").write_text(unreal_log, encoding="utf-8")

        return True, test_results, unreal_log, workspace, compile_out

    def retest_from_snapshot(self, snapshot_dir: Path, *, skip_baselines: bool = False) -> None:
        """Re-run the current test suite against an existing model snapshot."""
        self.retest_batch_from_snapshots([snapshot_dir], skip_baselines=skip_baselines)

    def _resolve_retest_snapshot(self, snapshot_ref: Path) -> Path:
        snapshot_dir = snapshot_ref
        if not snapshot_dir.is_absolute():
            snapshot_dir = Path.cwd() / snapshot_dir
        if not snapshot_dir.exists():
            snapshot_dir = self.tasks_dir.parent / snapshot_dir
        if not snapshot_dir.exists():
            matches = sorted(self._default_results_root().glob(f"{snapshot_dir.name}*"))
            matches = [m for m in matches if m.is_dir()]
            if not matches:
                raise FileNotFoundError(f"No snapshot found matching: {snapshot_dir.name}")
            snapshot_dir = matches[-1]
        return snapshot_dir

    def _load_retest_context(self, snapshot_dir: Path) -> dict:
        cfg_path = snapshot_dir / "task_config.json"
        res_path = snapshot_dir / "result.json"
        existing_record = {}
        if cfg_path.exists():
            task_config = json.loads(cfg_path.read_text(encoding="utf-8-sig"))
            task_id = task_config.get("task_id", "")
            if res_path.exists():
                existing_record = json.loads(res_path.read_text(encoding="utf-8-sig"))
        elif res_path.exists():
            existing_record = json.loads(res_path.read_text(encoding="utf-8-sig"))
            task_id = existing_record.get("task_id", "")
            task_config = {}
        else:
            raise FileNotFoundError(f"No task_config.json or result.json in {snapshot_dir}")

        task_dirs = [d for d in sorted(self.tasks_dir.glob(f"{task_id}*"))
                     if (d / "spec.yaml").exists()]
        if not task_dirs:
            raise FileNotFoundError(f"Task directory not found for {task_id} in {self.tasks_dir}")
        task_dir = task_dirs[0]

        if not task_config:
            task_config = load_task_schema(task_dir)

        tests_cfg = task_config.get("tests", {})
        test_filter = tests_cfg.get("filter", "")
        num_clients = tests_cfg.get("num_clients", 1)
        if not test_filter:
            raise ValueError("No test filter in spec - cannot run tests")

        import yaml  # type: ignore
        spec = yaml.safe_load((task_dir / "spec.yaml").read_text())
        return {
            "snapshot_dir": snapshot_dir,
            "task_id": task_id,
            "task_dir": task_dir,
            "task_config": task_config,
            "test_filter": test_filter,
            "num_clients": num_clients,
            "files_to_edit": spec.get("files_to_edit", []),
            "instruction": spec.get("instruction", ""),
            "hidden_requirements": spec.get("hidden_requirements", {}),
            "test_requirements": spec.get("test_requirements", {}),
            "test_file": self._find_task_test_file(task_dir),
            "existing_record": existing_record,
        }

    def _load_baseline_context(self, task_ref: str) -> dict:
        task_dirs = [
            d for d in sorted(self.tasks_dir.glob(f"{task_ref}*"))
            if (d / "spec.yaml").exists()
        ]
        if not task_dirs:
            raise FileNotFoundError(f"Task directory not found for {task_ref} in {self.tasks_dir}")
        task_dir = task_dirs[0]

        import yaml  # type: ignore
        task_config = yaml.safe_load((task_dir / "spec.yaml").read_text()) or {}
        tests_cfg = task_config.get("tests", {})
        test_filter = tests_cfg.get("filter", "")
        num_clients = tests_cfg.get("num_clients", 1)
        if not test_filter:
            raise ValueError("No test filter in spec - cannot run tests")

        return {
            "task_id": task_config.get("task_id", task_dir.name.split("_", 3)[0]),
            "task_dir": task_dir,
            "task_config": task_config,
            "test_filter": test_filter,
            "num_clients": num_clients,
            "files_to_edit": task_config.get("files_to_edit", []),
            "instruction": task_config.get("instruction", ""),
            "hidden_requirements": task_config.get("hidden_requirements", {}),
            "test_requirements": task_config.get("test_requirements", {}),
            "test_file": self._find_task_test_file(task_dir),
        }

    @staticmethod
    def _snapshot_label(snapshot_dir: Path, task_id: str) -> str:
        name = snapshot_dir.name
        if name.startswith(task_id + "_"):
            name = name[len(task_id) + 1:]
        return name or "model"

    @staticmethod
    def _print_judge_result(judge_result: dict) -> None:
        if judge_result.get("skipped"):
            print(f"  Judge skipped: {judge_result.get('reason')}")
            return

        j = judge_result.get("persisted_judge", {})
        mode = j.get("mode", "atomic")
        if mode == "dense":
            verdict = j.get("submission_verdict", "<=")
            t_verdict = j.get("test_verdict", "<=")
            conf = j.get("confidence", "<=")
            cat = j.get("confusion_category", "<=")
            rat = (j.get("rationale") or "")[:200].replace("\n", " ")
            print(f"\n  Judge (dense)  verdict={verdict}  test_quality={t_verdict}"
                  f"  confidence={conf}  confusion={cat}")
            print(f"  {rat}")
            for tb in j.get("test_breakdown", []):
                tid = str(tb.get("test_id", "<=")).split(".")[-1]
                outcome = tb.get("outcome", "<=")
                assessment = tb.get("assessment", "<=")
                notes = (tb.get("notes") or "")[:120].replace("\n", " ")
                print(f"  [{outcome}] {tid}  quality={assessment}")
                if notes:
                    print(f"       {notes}")
        else:
            counts = j.get("counts", {})
            trust = j.get("trust_score")
            full_task_ok = j.get("submission_satisfies_full_task")
            inferred = j.get("inferred_missing_requirements", [])
            print(f"\n  Judge (atomic)  trust={trust}  "
                  f"TP={counts.get('TP',0)} TN={counts.get('TN',0)} "
                  f"FP={counts.get('FP',0)} FN={counts.get('FN',0)}")
            if full_task_ok is not None or inferred:
                print(f"  Full task satisfied={full_task_ok}  "
                      f"inferred_missing={len(inferred)}")
            for c in j.get("classifications", []):
                lbl = c.get("classification", "<=")
                tid = c.get("test_id", "<=").split(".")[-1]
                correct = c.get("model_code_correct_for_test", c.get("model_code_correct", "<="))
                inferred_ids = ",".join(c.get("inferred_missing_requirement_ids", []))
                rat = (c.get("rationale") or "")[:160].replace("\n", " ")
                suffix = f"  inferred={inferred_ids}" if inferred_ids else ""
                print(f"  [{lbl}] {tid}  correct_for_test={correct}{suffix}")
                print(f"       {rat}")

    def _run_retest_judge(
        self,
        model_ws: Path,
        instruction: str,
        files_to_edit: list,
        solution_dir: Path,
        test_file: Optional[Path],
        hidden_requirements: Optional[dict] = None,
        test_requirements: Optional[dict] = None,
        existing_record: Optional[dict] = None,
    ) -> dict:
        (model_ws / "unreal_log.txt").touch(exist_ok=True)
        existing_record = existing_record or {}
        judge_model = self._judge_model_for(
            model=existing_record.get("model") or self.model,
            agent=existing_record.get("agent") or self.agent,
        )
        print("\n  Running judge on model output...")
        judge_result = run_unreal_judge(
            snapshot_dir=model_ws,
            task_instruction=instruction,
            files_to_edit=files_to_edit,
            solution_dir=solution_dir,
            test_file=test_file,
            hidden_requirements=hidden_requirements or {},
            test_requirements=test_requirements or {},
            judge_model=judge_model,
        )
        self._print_judge_result(judge_result)
        return judge_result

    @staticmethod
    def _persist_retest_snapshot_result(
        snapshot_dir: Path,
        existing_record: dict,
        compile_success: bool,
        compile_output: str,
        test_results: list[dict],
        unreal_log: str,
        judge_result: Optional[dict] = None,
    ) -> None:
        total = len(test_results)
        passed = sum(1 for t in test_results if t.get("passed"))
        updated = dict(existing_record)
        updated["timestamp"] = datetime.now(timezone.utc).isoformat()
        updated["compile_success"] = compile_success
        updated["tests_run"] = total
        updated["tests_passed"] = passed
        updated["tests_failed"] = max(total - passed, 0)
        updated["task_passed"] = compile_success and total > 0 and passed == total
        updated["test_results"] = test_results
        updated["failure_reason"] = (
            None if updated["task_passed"]
            else ("compile_failed" if not compile_success else "tests_failed")
        )
        updated["error"] = None
        if judge_result and not judge_result.get("skipped"):
            updated["judge"] = judge_result.get("persisted_judge", {})
            updated["judge_model"] = updated["judge"].get("model")
        else:
            updated.pop("judge", None)
            updated.pop("judge_model", None)

        (snapshot_dir / "result.json").write_text(
            json.dumps(updated, indent=2), encoding="utf-8"
        )
        if compile_output:
            (snapshot_dir / "compile_output.txt").write_text(
                compile_output, encoding="utf-8"
            )
        if unreal_log:
            (snapshot_dir / "unreal_log.txt").write_text(
                unreal_log, encoding="utf-8"
            )

    @staticmethod
    def _print_unreal_log_markers(
        label: str,
        unreal_log: str,
        markers: tuple[str, ...] = ("[Pursuit", "[Fallback", "[ZedFallback"),
    ) -> None:
        if not unreal_log:
            return

        lines = [
            line
            for line in unreal_log.splitlines()
            if any(marker in line for marker in markers)
        ]
        if not lines:
            return

        print(f"\n  [{label}] Diagnostic log lines:")
        for line in lines:
            print(f"    {line}")

    def retest_batch_from_snapshots(
        self,
        snapshot_refs: list[Path],
        *,
        skip_baselines: bool = False,
    ) -> None:
        """Re-run tests against one or more model snapshots, optionally with baselines."""
        try:
            contexts = [self._load_retest_context(self._resolve_retest_snapshot(ref)) for ref in snapshot_refs]
        except (FileNotFoundError, ValueError) as exc:
            print(exc)
            return

        first = contexts[0]
        task_id = first["task_id"]
        task_dir = first["task_dir"]
        test_filter = first["test_filter"]
        num_clients = first["num_clients"]
        files_to_edit = first["files_to_edit"]
        instruction = first["instruction"]
        test_file = first["test_file"]

        for ctx in contexts[1:]:
            if ctx["task_id"] != task_id:
                print("All --retest-batch snapshots must belong to the same task_id.")
                return
            if ctx["test_filter"] != test_filter or ctx["num_clients"] != num_clients:
                print("All --retest-batch snapshots must share the same test configuration.")
                return

        workspaces: list[Path] = []
        try:
            start_results: list[dict] = []
            sol_results: list[dict] = []
            if skip_baselines:
                print("\n  Baseline retests skipped via --skip-retest-baselines.")
            else:
                _, start_results, start_log, ws, _ = self._run_tests_from_dir(
                    task_dir / "start", task_dir, test_filter, num_clients, "start"
                )
                workspaces.append(ws)
                self._print_unreal_log_markers("start", start_log)

                _, sol_results, sol_log, ws, _ = self._run_tests_from_dir(
                    task_dir / "solution", task_dir, test_filter, num_clients, "solution"
                )
                workspaces.append(ws)
                self._print_unreal_log_markers("solution", sol_log)

            baseline_ids = list(
                dict.fromkeys([t["id"] for t in start_results] + [t["id"] for t in sol_results])
            )
            if baseline_ids:
                start_map = {t["id"]: t["passed"] for t in start_results}
                sol_map = {t["id"]: t["passed"] for t in sol_results}
                start_passed = sum(1 for t in start_results if t["passed"])
                sol_passed = sum(1 for t in sol_results if t["passed"])

                print("\n  Baseline summary (before model snapshots):")
                print(f"\n  {'='*70}")
                print(f"  {task_id}")
                print(f"  {'='*70}")
                print(f"  {'Test':<50} {'start':>6} {'solution':>9}")
                print(f"  {'-'*70}")
                for tid in baseline_ids:
                    print(
                        f"  {tid[:50]:<50} "
                        f"{('PASS' if start_map.get(tid) else 'FAIL'):>6} "
                        f"{('PASS' if sol_map.get(tid) else 'FAIL'):>9}"
                    )
                print(f"  {'-'*70}")
                print(
                    f"  {'TOTAL':<50} "
                    f"{f'{start_passed}/{len(baseline_ids)}':>6} "
                    f"{f'{sol_passed}/{len(baseline_ids)}':>9}"
                )

            model_runs: list[dict] = []
            for idx, ctx in enumerate(contexts, start=1):
                snapshot_dir = ctx["snapshot_dir"]
                label = "model" if len(contexts) == 1 else self._snapshot_label(snapshot_dir, task_id)
                if len(contexts) > 1 and any(run["label"] == label for run in model_runs):
                    label = f"{label}_{idx}"
                compile_success, model_results, model_log, model_ws, compile_output = self._run_tests_from_dir(
                    snapshot_dir, task_dir, test_filter, num_clients, label
                )
                workspaces.append(model_ws)
                model_runs.append({
                    "label": label,
                    "snapshot_dir": snapshot_dir,
                    "existing_record": ctx["existing_record"],
                    "hidden_requirements": ctx.get("hidden_requirements", {}),
                    "test_requirements": ctx.get("test_requirements", {}),
                    "compile_success": compile_success,
                    "compile_output": compile_output,
                    "results": model_results,
                    "log": model_log,
                    "workspace": model_ws,
                })

            start_map = {t["id"]: t["passed"] for t in start_results}
            sol_map = {t["id"]: t["passed"] for t in sol_results}
            all_ids = list(dict.fromkeys(
                [t["id"] for t in (start_results or sol_results)] +
                [t["id"] for run in model_runs for t in run["results"]]
            ))
            if not all_ids:
                print("\n  ERROR: retest completed, but no automation results were parsed from any run.")
                print("  This is a harness/log parsing failure, not a benchmark result.")
                return

            start_passed = sum(1 for t in start_results if t["passed"])
            sol_passed = sum(1 for t in sol_results if t["passed"])
            total = len(all_ids)

            model_labels = [run["label"] for run in model_runs]
            label_widths = {label: max(6, min(18, len(label))) for label in model_labels}

            print(f"\n  {'='*70}")
            print(f"  {task_id}")
            print(f"  {'='*70}")
            header = f"  {'Test':<50}"
            if not skip_baselines:
                header += f" {'start':>6} {'solution':>9}"
            for label in model_labels:
                header += f" {label[:label_widths[label]]:>{label_widths[label]}}"
            print(header)
            print(f"  {'-'*70}")

            warnings: list[str] = []
            for tid in all_ids:
                line = f"  {tid[:50]:<50} "
                if not skip_baselines:
                    line += f"{('PASS' if start_map.get(tid) else 'FAIL'):>6} "
                    line += f"{('PASS' if sol_map.get(tid) else 'FAIL'):>9}"
                for run in model_runs:
                    passed = {t['id']: t['passed'] for t in run["results"]}.get(tid)
                    line += f" {('PASS' if passed else 'FAIL'):>{label_widths[run['label']]}}"

                flags = ""
                if not skip_baselines and start_map.get(tid):
                    flags += " STUB_PASS"
                    warnings.append(f"  WARN  {tid}: passes on stub - test may be too easy or checking the wrong thing")
                if not skip_baselines and not sol_map.get(tid):
                    flags += " SOL_FAIL"
                    warnings.append(f"  WARN  {tid}: fails on solution - test bug or solution has a bug")
                print(line + flags)

            total_line = f"  {'TOTAL':<50}"
            if not skip_baselines:
                total_line += f" {start_passed:>5}/{total} {sol_passed:>8}/{total}"
            for run in model_runs:
                model_passed = sum(1 for t in run["results"] if t["passed"])
                total_line += f" {f'{model_passed}/{total}':>{label_widths[run['label']]}}"
            print(f"  {'-'*70}")
            print(total_line)

            if warnings:
                print("\n  Test quality warnings:")
                for w in warnings:
                    print(w)

            for run in model_runs:
                errors = [ln for ln in run["log"].splitlines() if "Error: Expected" in ln]
                if errors:
                    print(f"\n  Model failures [{run['label']}]:")
                    for e in errors:
                        print(f"    {e.split(']', 1)[-1].strip()}")

            if not self.run_judge:
                for run in model_runs:
                    self._persist_retest_snapshot_result(
                        run["snapshot_dir"],
                        run["existing_record"],
                        compile_success=run["compile_success"],
                        compile_output=run["compile_output"],
                        test_results=run["results"],
                        unreal_log=run["log"],
                    )
                print("\n  Judge skipped: disabled via --skip-judge")
                return

            for run in model_runs:
                model_passed = sum(1 for t in run["results"] if t["passed"])
                (run["workspace"] / "unreal_log.txt").write_text(run["log"], encoding="utf-8")
                (run["workspace"] / "result.json").write_text(json.dumps({
                    "task_id": task_id,
                    "compile_success": run["compile_success"],
                    "task_passed": run["compile_success"] and model_passed == total and total > 0,
                    "tests_run": total,
                    "tests_passed": model_passed,
                    "tests_failed": total - model_passed,
                    "test_results": run["results"],
                    "failure_reason": None if run["compile_success"] and model_passed == total else (
                        "compile_failed" if not run["compile_success"] else "tests_failed"
                    ),
                }), encoding="utf-8")
                print(f"\n  Judge target: {run['label']}")
                judge_result = self._run_retest_judge(
                    run["workspace"],
                    instruction=instruction,
                    files_to_edit=files_to_edit,
                    solution_dir=task_dir / "solution",
                    test_file=test_file,
                    hidden_requirements=run.get("hidden_requirements", {}),
                    test_requirements=run.get("test_requirements", {}),
                    existing_record=run["existing_record"],
                )
                self._persist_retest_snapshot_result(
                    run["snapshot_dir"],
                    run["existing_record"],
                    compile_success=run["compile_success"],
                    compile_output=run["compile_output"],
                    test_results=run["results"],
                    unreal_log=run["log"],
                    judge_result=judge_result,
                )
        finally:
            warm_root = self._warm_cache_dir()
            for ws in workspaces:
                if ws.exists():
                    if warm_root in ws.parents:
                        continue  # warm base - persists across runs, never delete
                    if self.debug:
                        print(f"  Preserving debug workspace: {ws}")
                    else:
                        shutil.rmtree(ws)

    def retest_task_baselines(self, task_ref: str) -> None:
        """Run the current test suite against start and solution only."""
        try:
            ctx = self._load_baseline_context(task_ref)
        except (FileNotFoundError, ValueError) as exc:
            print(exc)
            return

        task_id = ctx["task_id"]
        task_dir = ctx["task_dir"]
        test_filter = ctx["test_filter"]
        num_clients = ctx["num_clients"]

        workspaces: list[Path] = []
        try:
            _, start_results, start_log, ws, _ = self._run_tests_from_dir(
                task_dir / "start", task_dir, test_filter, num_clients, "start"
            )
            workspaces.append(ws)
            self._print_unreal_log_markers("start", start_log)

            _, sol_results, sol_log, ws, _ = self._run_tests_from_dir(
                task_dir / "solution", task_dir, test_filter, num_clients, "solution"
            )
            workspaces.append(ws)
            self._print_unreal_log_markers("solution", sol_log)

            all_ids = list(
                dict.fromkeys([t["id"] for t in start_results] + [t["id"] for t in sol_results])
            )
            if not all_ids:
                print("\n  ERROR: retest completed, but no automation results were parsed.")
                print("  This is a harness/log parsing failure, not a benchmark result.")
                return

            start_map = {t["id"]: t["passed"] for t in start_results}
            sol_map = {t["id"]: t["passed"] for t in sol_results}
            start_passed = sum(1 for t in start_results if t["passed"])
            sol_passed = sum(1 for t in sol_results if t["passed"])
            total = len(all_ids)

            print(f"\n  {'='*70}")
            print(f"  {task_id}")
            print(f"  {'='*70}")
            print(f"  {'Test':<50} {'start':>6} {'solution':>9}")
            print(f"  {'-'*70}")

            warnings: list[str] = []
            for tid in all_ids:
                line = (
                    f"  {tid[:50]:<50} "
                    f"{('OK' if start_map.get(tid) else 'NO'):>6} "
                    f"{('OK' if sol_map.get(tid) else 'NO'):>9}"
                )
                flags = ""
                if start_map.get(tid):
                    flags += " WARN STUB_PASS"
                    warnings.append(
                        f"  WARN   {tid}: passes on stub - test may be too easy or checking the wrong thing"
                    )
                if not sol_map.get(tid):
                    flags += " WARN SOL_FAIL"
                    warnings.append(
                        f"  WARN   {tid}: fails on solution - test bug or solution has a bug"
                    )
                print(line + flags)

            print(f"  {'-'*70}")
            print(
                f"  {'TOTAL':<50} {start_passed:>5}/{total} {sol_passed:>8}/{total}"
            )

            if warnings:
                print("\n  Test quality warnings:")
                for warning in warnings:
                    print(warning)
        finally:
            warm_root = self._warm_cache_dir()
            for ws in workspaces:
                if ws.exists():
                    if warm_root in ws.parents:
                        continue  # warm base - persists across runs, never delete
                    if self.debug:
                        print(f"  Preserving debug workspace: {ws}")
                    else:
                        shutil.rmtree(ws)


# -- Run manifest (multi-config YAML) --------------------------------------------


def _expand_task_ids(tasks):
    """Expand a manifest 'tasks' list (ints and 'lo-hi' range strings) into ue_task ids.
    None/omitted -> all tasks 1-71."""
    if tasks is None:
        nums = list(range(1, 72))
    else:
        nums = []
        for item in tasks:
            s = str(item).strip()
            if "-" in s:
                lo, hi = s.split("-", 1)
                nums.extend(range(int(lo), int(hi) + 1))
            else:
                nums.append(int(s))
    return [f"ue_task_{n:04d}" for n in dict.fromkeys(nums)]


def _infer_agent(model):
    m = (model or "").lower()
    if m.startswith("claude"):
        return "claude-code"
    if m.startswith(("gpt", "o1", "o3", "o4", "codex")):
        return "codex"
    if m.startswith(("gemini", "antigravity", "agy")):
        # Gemini CLI is deprecated; Gemini-family models now run through the
        # Antigravity CLI (`agy`) solver.
        return "antigravity-cli"
    if m.startswith("qwen"):
        return "qwen-code"
    if m.startswith("kimi"):
        return "kimi-code"
    if m.startswith("deepseek"):
        return "deepseek-code"
    if m.startswith(("glm", "zhipu")):
        return "glm-code"
    if m.startswith(("muse", "spark")):
        return "muse-code"
    return "claude-code"


def _auto_output(model, reasoning, task_ids):
    rtag = f"_{reasoning}" if reasoning and reasoning != "default" else ""
    nums = sorted(int(t.split("_")[-1]) for t in task_ids)
    rng = f"{nums[0]}-{nums[-1]}" if nums else "none"
    return f"results/{model}{rtag}_tasks_{rng}.json"


def _find_latest_snapshot(results_dir, task_id, agent, model, reasoning):
    import glob, json, os
    best = None
    for d in glob.glob(os.path.join(results_dir, f"{task_id}_{agent}_*")):
        rj = os.path.join(d, "result.json")
        if not os.path.isfile(rj):
            continue
        try:
            r = json.load(open(rj, encoding="utf-8"))
        except Exception:
            continue
        if model and r.get("model") != model:
            continue
        if reasoning and reasoning != "default" and r.get("reasoning_level_applied") != reasoning:
            continue
        name = os.path.basename(d)
        if best is None or name > best[0]:
            best = (name, d)
    return best[1] if best else None


def run_manifest(manifest_path):
    """Execute a multi-config YAML manifest. See the schema in run_manifest.yaml."""
    import yaml, os
    spec = yaml.safe_load(open(manifest_path, encoding="utf-8")) or {}
    defaults = spec.get("defaults") or {}
    runs = spec.get("runs") or []
    if not runs:
        print(f"Manifest {manifest_path} has no 'runs'.")
        return

    for i, entry in enumerate(runs, 1):
        cfg = {**defaults, **(entry or {})}
        model = cfg.get("model")
        if not model:
            print(f"[run {i}] skipped: no 'model'.")
            continue
        agent = cfg.get("agent") or _infer_agent(model)
        reasoning = cfg.get("reasoning") or "default"
        mode = cfg.get("mode") or "solve"
        warm_cache = bool(cfg.get("warm_cache", False))
        skip_judge = bool(cfg.get("skip_judge", False))
        judge_model = cfg.get("judge_model")
        tasks_dir = cfg.get("tasks_dir") or "tasks_unreal"
        task_ids = _expand_task_ids(cfg.get("tasks"))

        rtag = f" / {reasoning}" if reasoning != "default" else ""
        print(f"\n{'='*72}\n[run {i}/{len(runs)}] {model} / {agent}{rtag} / {mode} / "
              f"{len(task_ids)} tasks (warm_cache={warm_cache})\n{'='*72}")

        if mode == "retest":
            results_dir = os.path.join(tasks_dir, "test_result")
            for tid in task_ids:
                snap = _find_latest_snapshot(results_dir, tid, agent, model, reasoning)
                if not snap:
                    print(f"  {tid}: no {model}/{reasoning} snapshot - nothing to retest, skipping")
                    continue
                runner = UnrealBenchmarkRunner(
                    tasks_dir=tasks_dir, output_file=Path("/dev/null"),
                    agent=agent, model=model, debug=False,
                    run_judge=not skip_judge, warm_cache=warm_cache,
                    reasoning_level=reasoning, judge_model=judge_model,
                )
                runner.retest_from_snapshot(Path(snap))
        else:  # solve
            output = cfg.get("output") or _auto_output(model, reasoning, task_ids)
            runner = UnrealBenchmarkRunner(
                tasks_dir=tasks_dir, output_file=output,
                agent=agent, model=model, debug=False,
                run_judge=not skip_judge,
                solver_timeout=int(cfg.get("solver_timeout", 3600)),
                warm_cache=warm_cache, reasoning_level=reasoning,
                judge_model=judge_model,
            )
            runner.run_all(task_ids=task_ids)


# -- Entry point ---------------------------------------------------------------


def main():
    _load_repo_dotenv()

    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")

    parser = argparse.ArgumentParser(
        description="Run Unreal Engine benchmark tasks through a solver"
    )
    parser.add_argument(
        "--tasks-dir",
        default="tasks_unreal",
        help="Directory containing ue_task_* subdirectories (default: tasks_unreal)",
    )
    parser.add_argument(
        "--manifest",
        default=None,
        metavar="FILE",
        help=(
            "Run a multi-config YAML manifest (model x tasks x mode) instead of a single "
            "CLI run. Overrides the other run flags. See run_manifest.yaml for the schema."
        ),
    )
    parser.add_argument(
        "--output",
        required=False,
        default=None,
        help="Output JSON file path for results (not required with --retest)",
    )
    parser.add_argument(
        "--retest",
        metavar="SNAPSHOT",
        default=None,
        help=(
            "Re-run the current test suite against an existing model snapshot "
            "without invoking the solver. Pass the snapshot directory path or "
            "a task-id prefix (e.g. ue_task_0002). "
            "--output, --agent, and --model are not required in this mode."
        ),
    )
    parser.add_argument(
        "--retest-batch",
        nargs="+",
        metavar="SNAPSHOT",
        default=None,
        help=(
            "Re-run the current test suite against multiple existing model snapshots "
            "for the same task while sharing one start and one solution run. "
            "--output, --agent, and --model are not required in this mode."
        ),
    )
    parser.add_argument(
        "--retest-baseline",
        metavar="TASK_ID",
        default=None,
        help=(
            "Run the current test suite against only start and solution for a task "
            "(e.g. ue_task_0047) without invoking the solver or requiring a model snapshot."
        ),
    )
    parser.add_argument(
        "--agent",
        required=False,
        default=None,
        choices=SolverFactory.get_available_agents(),
        help="Solver agent to use",
    )
    parser.add_argument(
        "--model",
        default=None,
        help="Model name for the solver",
    )
    parser.add_argument(
        "--judge-model",
        default=None,
        help=(
            "Override the auto-selected LLM judge model. Also available via "
            "UNREALBENCH_JUDGE_MODEL."
        ),
    )
    parser.add_argument(
        "--reasoning-level",
        default="default",
        choices=["default", "low", "medium", "high", "xhigh", "max"],
        help=(
            "Optional reasoning setting for supported agents "
            "(claude-code, codex, gemini-cli)."
        ),
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable verbose debug output",
    )
    parser.add_argument(
        "--resume-from",
        default=None,
        help="Path to a previous results JSON; skips tasks where solver_success=true",
    )
    parser.add_argument(
        "--task-id",
        nargs="+",
        default=None,
        metavar="TASK_ID",
        help="Run only the specified task(s), e.g. --task-id ue_task_0001 ue_task_0002",
    )
    parser.add_argument(
        "--skip-judge",
        action="store_true",
        help="Skip the Unreal LLM-as-judge after the benchmark snapshot is saved.",
    )
    parser.add_argument(
        "--skip-retest-baselines",
        action="store_true",
        help=(
            "With --retest or --retest-batch, skip rerunning start/ and solution/ "
            "and only retest the provided model snapshot(s)."
        ),
    )
    parser.add_argument(
        "--solver-timeout",
        type=int,
        default=3600,
        metavar="SECONDS",
        help="Maximum wall-clock seconds allowed for the solver (default: 3600).",
    )
    parser.add_argument(
        "--warm-cache",
        action="store_true",
        help=(
            "Reuse a persistent per-task warm build base (compiled once, kept in "
            "$TMPDIR/unrealbench_warm_base) and incrementally recompile only the "
            "files that differ. Turns a ~25 min cold build into a ~1-3 min relink. "
            "Auto-invalidates when peer plugins, scaffold, Build.cs, tests, or the "
            "engine change. Same-task runs are serialized."
        ),
    )
    args = parser.parse_args()

    if args.manifest:
        run_manifest(args.manifest)
        return

    chosen_retest_modes = [bool(args.retest), bool(args.retest_batch), bool(args.retest_baseline)]
    if sum(chosen_retest_modes) > 1:
        parser.error("--retest, --retest-batch, and --retest-baseline are mutually exclusive")

    if args.retest:
        runner = UnrealBenchmarkRunner(
            tasks_dir=args.tasks_dir,
            output_file=Path("/dev/null"),
            agent=args.agent or "claude-code",
            model=args.model,
            debug=args.debug,
            run_judge=not args.skip_judge,
            warm_cache=args.warm_cache,
            reasoning_level=args.reasoning_level,
            judge_model=args.judge_model,
        )
        runner.retest_from_snapshot(Path(args.retest), skip_baselines=args.skip_retest_baselines)
        return

    if args.retest_batch:
        runner = UnrealBenchmarkRunner(
            tasks_dir=args.tasks_dir,
            output_file=Path("/dev/null"),
            agent=args.agent or "claude-code",
            model=args.model,
            debug=args.debug,
            run_judge=not args.skip_judge,
            warm_cache=args.warm_cache,
            reasoning_level=args.reasoning_level,
            judge_model=args.judge_model,
        )
        runner.retest_batch_from_snapshots(
            [Path(p) for p in args.retest_batch],
            skip_baselines=args.skip_retest_baselines,
        )
        return

    if args.retest_baseline:
        runner = UnrealBenchmarkRunner(
            tasks_dir=args.tasks_dir,
            output_file=Path("/dev/null"),
            agent=args.agent or "claude-code",
            model=args.model,
            debug=args.debug,
            run_judge=False,
            warm_cache=args.warm_cache,
            reasoning_level=args.reasoning_level,
            judge_model=args.judge_model,
        )
        runner.retest_task_baselines(args.retest_baseline)
        return

    if not args.output:
        parser.error("--output is required unless --retest, --retest-batch, or --retest-baseline is used")
    if not args.agent:
        parser.error("--agent is required unless --retest is used")

    runner = UnrealBenchmarkRunner(
        tasks_dir=args.tasks_dir,
        output_file=args.output,
        agent=args.agent,
        model=args.model,
        debug=args.debug,
        resume_from=args.resume_from,
        run_judge=not args.skip_judge,
        solver_timeout=args.solver_timeout,
        warm_cache=args.warm_cache,
        reasoning_level=args.reasoning_level,
        judge_model=args.judge_model,
    )

    runner.run_all(task_ids=args.task_id)


if __name__ == "__main__":
    main()
