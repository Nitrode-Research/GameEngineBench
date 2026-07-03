#!/usr/bin/env python3
"""Corpus health gate for Unreal tasks.

Static mode    : verifies start/ stubs FAIL and solution/ dirs PASS validation.
Compile mode   : builds all solution/ directories against the installed UE engine.
Schema mode    : verifies each task has a valid spec.yaml with required fields.
Run-tests mode : injects tests/ into solution/, compiles, runs UE automation tests
                 headless in listen server mode, and reports per-test pass/fail.

Usage:
    python audit.py --schema
    python audit.py --static
    python audit.py --compile
    python audit.py --run-tests
    python audit.py --schema --static --compile
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import threading
import time
from pathlib import Path

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML not installed. Run: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

HERE = Path(__file__).parent

REQUIRED_SPEC_FIELDS = ["task_id", "title", "tier", "instruction", "files_to_edit", "tests"]
REQUIRED_TESTS_FIELDS = ["runner", "mode", "num_clients", "filter"]
VALID_TIERS = {"easy", "medium", "hard"}


# ── Config loading ────────────────────────────────────────────────────────────

def _detect_ue_platform() -> str:
    if sys.platform == "darwin":
        return "Mac"
    if sys.platform == "win32":
        return "Win64"
    return "Linux"


def load_config():
    config_path = HERE / "unreal_config.yaml"
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
    platform = os.environ.get("UE_PLATFORM", cfg.get("platform") or _detect_ue_platform())
    configuration = cfg.get("configuration", "Development")

    build_template = cfg["build_script_templates"][platform]
    build_script = build_template.replace("{engine_root}", engine_root)

    editor_binary = None
    editor_templates = cfg.get("editor_binary_templates", {})
    if platform in editor_templates:
        editor_binary = editor_templates[platform].replace("{engine_root}", engine_root)

    return {
        "engine_root": engine_root,
        "platform": platform,
        "configuration": configuration,
        "build_script": build_script,
        "editor_binary": editor_binary,
    }


def find_tasks():
    return sorted(HERE.glob("ue_task_*"))


def _resolve_unreal_log(uproject_path: Path) -> "tuple[Path | None, list[Path]]":
    """Return (chosen_log, candidates_inspected) for cross-platform UE log lookup.

    Workspace-local log is preferred to avoid pollution from concurrent or
    stale editor instances. Falls back to per-user paths that differ by OS.
    """
    project_name = uproject_path.stem
    workspace_log = uproject_path.parent / "Saved" / "Logs" / "UnrealEditor.log"

    candidates: list[Path] = [workspace_log]
    if sys.platform == "darwin":
        candidates.append(
            Path.home() / "Library" / "Logs" / "Unreal Engine"
            / f"{project_name}Editor" / f"{project_name}.log"
        )
    elif sys.platform == "win32":
        local = os.environ.get("LOCALAPPDATA")
        base = Path(local) if local else Path.home() / "AppData" / "Local"
        candidates.append(
            base / "UnrealEngine" / f"{project_name}Editor"
            / "Saved" / "Logs" / f"{project_name}.log"
        )
    else:
        candidates.append(
            Path.home() / ".config" / "Epic" / "UnrealEngine"
            / f"{project_name}Editor" / "Saved" / "Logs" / f"{project_name}.log"
        )
        candidates.append(
            Path.home() / ".local" / "share" / "Epic" / "UnrealEngine"
            / f"{project_name}Editor" / "Saved" / "Logs" / f"{project_name}.log"
        )

    for c in candidates:
        if c.exists() and c.stat().st_size > 0:
            return c, candidates
    return None, candidates


# ── Schema audit ──────────────────────────────────────────────────────────────

def run_schema(tasks):
    print("=" * 60)
    print("SCHEMA AUDIT")
    print("=" * 60)

    all_passed = True

    for task_dir in tasks:
        spec_path = task_dir / "spec.yaml"
        print(f"\n{task_dir.name}")

        if not spec_path.exists():
            print(f"  [FAIL] missing spec.yaml")
            all_passed = False
            continue

        try:
            with spec_path.open() as f:
                spec = yaml.safe_load(f)
        except yaml.YAMLError as e:
            print(f"  [FAIL] spec.yaml parse error: {e}")
            all_passed = False
            continue

        task_ok = True

        for field in REQUIRED_SPEC_FIELDS:
            if field not in spec or spec[field] is None:
                print(f"  [FAIL] missing required field: {field}")
                task_ok = False
                all_passed = False

        tier = spec.get("tier")
        if tier and tier not in VALID_TIERS:
            print(f"  [FAIL] invalid tier '{tier}' — must be one of {VALID_TIERS}")
            task_ok = False
            all_passed = False

        tests = spec.get("tests", {})
        if isinstance(tests, dict):
            for field in REQUIRED_TESTS_FIELDS:
                if field not in tests:
                    print(f"  [FAIL] tests.{field} is missing")
                    task_ok = False
                    all_passed = False

        files_to_edit = spec.get("files_to_edit", [])
        if not isinstance(files_to_edit, list) or len(files_to_edit) == 0:
            print(f"  [FAIL] files_to_edit must be a non-empty list")
            task_ok = False
            all_passed = False

        if not (task_dir / "start").exists():
            print(f"  [FAIL] missing start/ directory")
            task_ok = False
            all_passed = False

        if not (task_dir / "tests").exists():
            print(f"  [FAIL] missing tests/ directory")
            task_ok = False
            all_passed = False

        if not (task_dir / "solution").exists():
            print(f"  [FAIL] missing solution/ directory")
            task_ok = False
            all_passed = False

        if task_ok:
            print(f"  [PASS] spec.yaml valid")

    print()
    if all_passed:
        print("SCHEMA AUDIT: ALL PASSED")
    else:
        print("SCHEMA AUDIT: FAILURES DETECTED")

    return all_passed


# ── Static audit ──────────────────────────────────────────────────────────────

def run_static(tasks):
    print("=" * 60)
    print("STATIC AUDIT")
    print("=" * 60)

    all_passed = True

    for task_dir in tasks:
        validate_script = task_dir / "validate.py"
        if not validate_script.exists():
            print(f"[SKIP] {task_dir.name} — no validate.py")
            continue

        start_dir = task_dir / "start"
        solution_dir = task_dir / "solution"

        print(f"\n{task_dir.name}")

        result = subprocess.run(
            [sys.executable, str(validate_script), str(start_dir)],
            capture_output=True, text=True
        )
        if result.returncode != 0:
            print(f"  [PASS] start/ correctly fails validation")
        else:
            print(f"  [FAIL] start/ unexpectedly passed validation — stub is too complete")
            all_passed = False

        result = subprocess.run(
            [sys.executable, str(validate_script), str(solution_dir)],
            capture_output=True, text=True
        )
        output = result.stdout + result.stderr
        if result.returncode == 0:
            print(f"  [PASS] solution/ passes validation")
        else:
            print(f"  [FAIL] solution/ failed validation")
            for line in output.strip().splitlines():
                print(f"         {line}")
            all_passed = False

    print()
    if all_passed:
        print("STATIC AUDIT: ALL PASSED")
    else:
        print("STATIC AUDIT: FAILURES DETECTED")

    return all_passed


# ── Compile audit ─────────────────────────────────────────────────────────────

def run_compile(tasks, cfg):
    print("=" * 60)
    print("COMPILE AUDIT")
    print("=" * 60)

    build_script = cfg["build_script"]
    platform = cfg["platform"]
    configuration = cfg["configuration"]

    if not Path(build_script).exists():
        print(f"ERROR: Build script not found: {build_script}", file=sys.stderr)
        sys.exit(1)

    all_passed = True

    for task_dir in tasks:
        solution_dir = task_dir / "solution"

        uprojects = list(solution_dir.rglob("*.uproject")) if solution_dir.exists() else []
        if not uprojects:
            print(f"\n[SKIP] {task_dir.name} — no .uproject in solution/")
            continue

        uproject = uprojects[0]
        project_name = uproject.stem
        editor_target = f"{project_name}Editor"

        print(f"\n{task_dir.name}  ({project_name})")

        cmd = [
            build_script, editor_target, platform, configuration,
            f"-Project={uproject.resolve()}", "-WaitMutex", "-NoHotReload",
        ]

        t0 = time.time()
        result = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        )
        elapsed = time.time() - t0

        if result.returncode == 0:
            print(f"  [PASS] compiled successfully ({elapsed:.1f}s)")
        else:
            print(f"  [FAIL] compilation failed (exit {result.returncode}, {elapsed:.1f}s)")
            for line in result.stdout.strip().splitlines()[-20:]:
                print(f"         {line}")
            all_passed = False

    print()
    print("COMPILE AUDIT: " + ("ALL PASSED" if all_passed else "FAILURES DETECTED"))
    return all_passed


# ── Tests audit ───────────────────────────────────────────────────────────────

def _patch_build_cs_for_tests(solution_dir: Path, project_name: str) -> bool:
    """Ensure Build.cs has AutomationController and UnrealEd (editor-only).

    Returns True if the file was modified.
    """
    build_cs = solution_dir / "Source" / project_name / f"{project_name}.Build.cs"
    if not build_cs.exists():
        return False
    text = build_cs.read_text(encoding="utf-8")
    patched = text

    if "AutomationController" not in patched:
        patched = re.sub(
            r'(PublicDependencyModuleNames\.AddRange\(new string\[\] \{)',
            r'\1\n            "AutomationController",',
            patched,
            count=1,
        )

    if "UnrealEd" not in patched:
        unrealed_block = (
            '\n        if (Target.bBuildEditor)\n'
            '        {\n'
            '            PublicDependencyModuleNames.AddRange(new string[] { "UnrealEd" });\n'
            '        }\n'
        )
        # Insert before the LAST `    }` (which closes the constructor)
        lines = patched.splitlines(keepends=True)
        for i in range(len(lines) - 1, -1, -1):
            if lines[i].rstrip() == "}":
                for j in range(i - 1, -1, -1):
                    stripped = lines[j].rstrip()
                    if stripped == "    }" or stripped == "\t}":
                        lines.insert(j, unrealed_block)
                        patched = "".join(lines)
                        break
                break

    if patched != text:
        build_cs.write_text(patched, encoding="utf-8")
        return True
    return False


def _clean_test_build_artifacts(target_dir: Path) -> None:
    """Remove per-target build outputs before test compiles.

    start/ and solution/ share the same project and module names, so Unreal's
    incremental artifacts can bleed across trees and make start/ look more
    complete than it really is. For run-tests mode we want isolation over speed.
    """
    for name in ("Binaries", "Intermediate"):
        path = target_dir / name
        if path.exists():
            shutil.rmtree(path)

    saved_logs = target_dir / "Saved" / "Logs"
    if saved_logs.exists():
        shutil.rmtree(saved_logs)


def _parse_automation_results(log_text: str) -> list[dict]:
    """Extract per-test pass/fail and error messages from a UE automation run log.

    UE5 writes one "Test Completed" line per test plus a BeginEvents/EndEvents
    block containing every TestTrue/TestNotNull/TestEqual failure as an
    "Error:" line. We capture both so the user can see WHY each test failed.
    """
    results = []
    seen = set()
    errors_by_test: dict[str, list[str]] = {}

    result_re = re.compile(r"Result=\{?(\w+)\}?")
    path_re = re.compile(r"Path=\{?([^}\s]+)\}?")
    name_re = re.compile(r"Name=\{?([^}\s]+)\}?")
    begin_re = re.compile(r"LogAutomationController.*?BeginEvents:\s*(\S+)")
    end_re = re.compile(r"LogAutomationController.*?EndEvents:\s*(\S+)")
    error_re = re.compile(r"LogAutomationController.*?Error:\s*(.+)")

    current_test: str | None = None

    for line in log_text.splitlines():
        mbegin = begin_re.search(line)
        if mbegin:
            current_test = mbegin.group(1).strip()
            errors_by_test.setdefault(current_test, [])
            continue

        mend = end_re.search(line)
        if mend:
            current_test = None
            continue

        if current_test is not None:
            merr = error_re.search(line)
            if merr:
                msg = merr.group(1).strip()
                # Skip noisy framework lines that aren't actual test assertions
                if msg.startswith("Test Completed"):
                    continue
                errors_by_test[current_test].append(msg)

        if "LogAutomationController" not in line or "Test Completed" not in line:
            continue
        r = result_re.search(line)
        p = path_re.search(line) or name_re.search(line)
        if not r or not p:
            continue
        status = r.group(1).lower()
        test_id = p.group(1).strip()
        if test_id in seen:
            continue
        seen.add(test_id)
        results.append({
            "id": test_id,
            "passed": status in ("success", "passed"),
            "errors": errors_by_test.get(test_id, []),
        })

    # Attach any errors that arrived after the Test Completed line
    for r in results:
        if not r["errors"]:
            r["errors"] = errors_by_test.get(r["id"], [])

    return results


def _detect_log_crash(log_text):
    """Scan a UE log for crash markers. Returns a one-line summary or None.

    Without this, an editor crash mid-run looks like a clean partial pass to the
    test runner — the parser only counts 'Test Completed' lines, so any tests
    that ran before the crash are reported as successful and the rest silently
    vanish. This catches the crash explicitly so the audit can fail loudly.
    """
    crash_markers = (
        "Fatal error:",
        "appError called",
        "=== Critical error",
        "SIGSEGV:",
        "Assertion failed:",
    )
    lines = log_text.splitlines()
    for i, line in enumerate(lines):
        for marker in crash_markers:
            if marker in line:
                reason = ""
                for j in range(i + 1, min(i + 6, len(lines))):
                    candidate = lines[j].strip()
                    if candidate and not candidate.startswith("[20"):
                        reason = candidate
                        break
                return f"{marker.rstrip(':')} — {reason}" if reason else marker.rstrip(":")
    return None


def _stream_subprocess(cmd, timeout, log_tail_path=None):
    """Run a subprocess and stream stdout line-by-line to the console, with
    optional fallback to a log file if the process produces no output.

    Returns (returncode, full_output).
    """
    proc = subprocess.Popen(
        cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1,
    )

    output_lines = []
    interesting_patterns = (
        "LogAutomationController",
        "LogAutomationCommandLine",
        "LogInit: Display:",
        "LogLoad: Took",
        "LogShaderCompilers: Display: Shaders left",
        "LogPIE:",
        "Test Started",
        "Test Completed",
        "Engine exit",
        "Error:",
        "Fatal",
    )

    def _drain():
        for line in iter(proc.stdout.readline, ""):
            output_lines.append(line)
            if any(p in line for p in interesting_patterns):
                print(f"    | {line.rstrip()}", flush=True)

    drainer = threading.Thread(target=_drain, daemon=True)
    drainer.start()

    try:
        proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        drainer.join(timeout=2.0)
        return -1, "".join(output_lines)

    drainer.join(timeout=2.0)
    return proc.returncode, "".join(output_lines)


def run_tests(tasks, cfg, target="solution"):
    """Run headless automation tests against either solution/ or start/.

    target="solution" (default) — expect all tests to pass. Validates that the
        reference implementation satisfies the behavioral contract.
    target="start" — expect tests to fail. Validates that the stripped stubs
        don't accidentally pass, confirming benchmark validity.
    """
    assert target in ("solution", "start")
    expectation = "should pass all tests" if target == "solution" else "should fail tests"
    print("=" * 60)
    print(f"TESTS AUDIT — target={target}/ ({expectation})")
    print("=" * 60)

    build_script = cfg["build_script"]
    editor_binary = cfg["editor_binary"]
    platform = cfg["platform"]
    configuration = cfg["configuration"]

    if not editor_binary or not Path(editor_binary).exists():
        print(f"ERROR: Editor binary not found: {editor_binary}", file=sys.stderr)
        print("Add editor_binary_templates to unreal_config.yaml", file=sys.stderr)
        sys.exit(1)

    if not Path(build_script).exists():
        print(f"ERROR: Build script not found: {build_script}", file=sys.stderr)
        sys.exit(1)

    all_passed = True

    for task_dir in tasks:
        target_dir = task_dir / target
        tests_dir = task_dir / "tests"
        spec_path = task_dir / "spec.yaml"

        if not target_dir.exists() or not tests_dir.exists() or not spec_path.exists():
            print(f"\n[SKIP] {task_dir.name} — missing {target}/, tests/, or spec.yaml")
            continue

        with spec_path.open() as f:
            spec = yaml.safe_load(f)
        tests_cfg = spec.get("tests") or {}
        test_filter = tests_cfg.get("filter", "")
        num_clients = tests_cfg.get("num_clients", 1)

        if not test_filter:
            print(f"\n[SKIP] {task_dir.name} — no test filter in spec.yaml")
            continue

        uprojects = list(target_dir.rglob("*.uproject"))
        if not uprojects:
            print(f"\n[SKIP] {task_dir.name} — no .uproject in {target}/")
            continue

        uproject = uprojects[0]
        project_name = uproject.stem
        print(f"\n{task_dir.name}  ({project_name})  filter={test_filter}")

        # Snapshot state for cleanup in phase 5. If anything below raises or
        # fails, the `finally` block still restores the target tree to its
        # pre-run state so `git status` stays clean.
        dest_tests_dir = target_dir / "Source" / project_name / "Tests"
        build_cs_path = target_dir / "Source" / project_name / f"{project_name}.Build.cs"
        build_cs_original = build_cs_path.read_text(encoding="utf-8") if build_cs_path.exists() else None

        try:
            # Phase 1: Inject test files
            t0 = time.time()
            if dest_tests_dir.exists():
                shutil.rmtree(dest_tests_dir)
            dest_tests_dir.mkdir(parents=True, exist_ok=True)
            copied = [t.name for t in tests_dir.glob("*.cpp")]
            for t in tests_dir.glob("*.cpp"):
                shutil.copy2(t, dest_tests_dir / t.name)
            for t in tests_dir.glob("*.h"):
                shutil.copy2(t, dest_tests_dir / t.name)
            print(f"  [1/5] Injected {len(copied)} test file(s): {', '.join(copied)}  ({time.time()-t0:.1f}s)")

            # Phase 2: Patch Build.cs
            t0 = time.time()
            patched = _patch_build_cs_for_tests(target_dir, project_name)
            status = "patched" if patched else "already patched"
            print(f"  [2/5] Build.cs {status} (AutomationController + UnrealEd)  ({time.time()-t0:.1f}s)")

            # Phase 2.5: Clean per-target build artifacts so start/ and solution/
            # don't reuse each other's binaries/intermediates.
            t0 = time.time()
            _clean_test_build_artifacts(target_dir)
            print(f"  [2.5/5] Cleaned Binaries/, Intermediate/, and Saved/Logs/  ({time.time()-t0:.1f}s)")

            # Phase 3: Compile
            editor_target = f"{project_name}Editor"
            build_cmd = [
                build_script, editor_target, platform, configuration,
                f"-Project={uproject.resolve()}", "-WaitMutex", "-NoHotReload",
            ]
            print(f"  [3/5] Compiling {editor_target}...")
            t0 = time.time()
            result = subprocess.run(
                build_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            )
            compile_elapsed = time.time() - t0
            if result.returncode != 0:
                # Compile failure on EITHER target is a setup bug. In a well-formed
                # task, start/ and solution/ have identical headers — only cpp
                # bodies differ — so compilation is all-or-nothing across both.
                print(f"        [FAIL] compile failed (exit {result.returncode}, {compile_elapsed:.1f}s)")
                for line in result.stdout.strip().splitlines()[-20:]:
                    print(f"         {line}")
                all_passed = False
                continue
            print(f"        Compile OK  ({compile_elapsed:.1f}s)")

            # Phase 4: Run headless automation tests
            exec_cmd = f"Automation RunTests {test_filter}; Quit"
            test_cmd = [
                editor_binary, str(uproject.resolve()),
                f"-ExecCmds={exec_cmd}",
                "-Unattended", "-NullRHI", "-NoSplash",
                f"-NumClients={num_clients}", "-ListenServer",
            ]
            if sys.platform == "darwin" and os.uname().machine == "arm64":
                test_cmd = ["/usr/bin/arch", "-arm64", *test_cmd]

            print(f"  [4/5] Running tests: {test_filter}  (live log below, editor auto-exits on completion)")
            t0 = time.time()
            returncode, log_text = _stream_subprocess(test_cmd, timeout=900)
            test_elapsed = time.time() - t0

            if returncode == -1:
                print(f"        [FAIL] test run timed out after 900s")
                all_passed = False
                continue

            log_file, log_candidates = _resolve_unreal_log(uproject)
            if log_file:
                print(f"        Reading log: {log_file}")
                log_text = log_file.read_text(encoding="utf-8", errors="ignore")

            crash_summary = _detect_log_crash(log_text) if log_text else None
            results = _parse_automation_results(log_text)
            if not results:
                print(f"        [FAIL] no test results parsed ({test_elapsed:.1f}s)")
                print(f"        Checked log paths:")
                for candidate in log_candidates:
                    print(f"          - {candidate}  (exists={candidate.exists()})")
                print(f"        Tail of output:")
                for line in log_text.splitlines()[-20:]:
                    print(f"         {line}")
                all_passed = False
                continue

            passed_count = sum(1 for r in results if r["passed"])
            failed_count = len(results) - passed_count
            print(f"        Completed {len(results)} test(s) in {test_elapsed:.1f}s:")
            for r in results:
                status = "[PASS]" if r["passed"] else "[FAIL]"
                short_id = r["id"]
                if short_id.startswith(test_filter + "."):
                    short_id = short_id[len(test_filter) + 1:]
                print(f"          {status} {short_id}")
                if not r["passed"] and r.get("errors"):
                    for msg in r["errors"][:5]:
                        print(f"                 → {msg}")
                    if len(r["errors"]) > 5:
                        print(f"                 → ... and {len(r['errors']) - 5} more")
            print(f"        Summary: {passed_count}/{len(results)} passed, {failed_count} failed")
            if crash_summary:
                print(f"        [CRASH] UE editor crashed mid-run: {crash_summary}")
                print(f"        Tests not listed above never ran. Treating audit as failed.")
                all_passed = False

            if target == "solution":
                if failed_count > 0:
                    all_passed = False
            else:  # target == "start"
                # For start/, we expect tests to FAIL. If all passed, the stub
                # is too complete — the benchmark is invalid.
                if failed_count == 0:
                    print(f"        [INVALID] start/ passed every test — stub is too complete, model has nothing to implement")
                    all_passed = False
                else:
                    print(f"        [PASS] start/ correctly fails {failed_count} test(s) — benchmark is valid")
        finally:
            # Phase 5: Restore solution/ to its pre-run state so git stays clean.
            t0 = time.time()
            restored = []
            if dest_tests_dir.exists():
                shutil.rmtree(dest_tests_dir)
                restored.append(f"removed {dest_tests_dir.relative_to(target_dir)}")
            if build_cs_original is not None and build_cs_path.exists():
                current = build_cs_path.read_text(encoding="utf-8")
                if current != build_cs_original:
                    build_cs_path.write_text(build_cs_original, encoding="utf-8")
                    restored.append(f"reverted {build_cs_path.name}")
            if restored:
                print(f"  [5/5] Cleanup: {', '.join(restored)}  ({time.time()-t0:.1f}s)")
            else:
                print(f"  [5/5] Cleanup: nothing to restore  ({time.time()-t0:.1f}s)")

    print()
    print("TESTS AUDIT: " + ("ALL PASSED" if all_passed else "FAILURES DETECTED"))
    return all_passed


# ── Entry point ───────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="Unreal task corpus health gate")
    parser.add_argument("--schema", action="store_true",
                        help="Validate spec.yaml for each task")
    parser.add_argument("--static", action="store_true",
                        help="Run static validation audit (requires validate.py per task)")
    parser.add_argument("--compile", action="store_true",
                        help="Compile all solution/ directories")
    parser.add_argument("--run-tests", action="store_true", dest="run_tests",
                        help="Inject tests into the target dir, compile, and run headless UE automation tests")
    parser.add_argument("--target", choices=("solution", "start"), default="solution",
                        help="Which tree to run tests against: solution (default, expects PASS) "
                             "or start (expects FAIL — validates the stub doesn't trivially solve the task)")
    parser.add_argument("--task-id", nargs="+", metavar="TASK_ID",
                        help="Limit audit to specific task(s), e.g. --task-id ue_task_0001 ue_task_0002")
    args = parser.parse_args()

    if not args.schema and not args.static and not args.compile and not args.run_tests:
        parser.print_help()
        sys.exit(1)

    tasks = find_tasks()
    if not tasks:
        print("No ue_task_* directories found.", file=sys.stderr)
        sys.exit(1)

    if args.task_id:
        tasks = [t for t in tasks if any(t.name == tid or t.name.startswith(tid) for tid in args.task_id)]
        if not tasks:
            print(f"No tasks matched: {args.task_id}", file=sys.stderr)
            sys.exit(1)

    ok = True

    if args.schema:
        ok = run_schema(tasks) and ok

    if args.static:
        ok = run_static(tasks) and ok

    if args.compile:
        if (args.schema or args.static) and not ok:
            print("Skipping compile audit — earlier audit has failures.")
            sys.exit(1)
        cfg = load_config()
        ok = run_compile(tasks, cfg) and ok

    if args.run_tests:
        if (args.schema or args.static or args.compile) and not ok:
            print("Skipping tests audit — earlier audit has failures.")
            sys.exit(1)
        cfg = load_config()
        ok = run_tests(tasks, cfg, target=args.target) and ok

    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
