# UnrealBench — Benchmark Progress

Living record of model performance on the Unreal tasks. One section per model.
Update after each run. Regenerate the numbers with `python results/util/_analyze.py <model_id> [agent]`
(e.g. `claude-opus-4-8`, `claude-opus-4-7`, or `gpt-5.5 codex`).

---

## Current full-task snapshot (2026-06-25)

These are the current full-task audit pass rates over tasks **0001-0071**. The full-task label is read from `judge.submission_satisfies_full_task` for atomic audits, or `judge.submission_verdict == "CORRECT"` for dense audits. Compile failures are counted as full-task failures even when no full-task judge field exists. Confirmed test-time editor crashes caused by model-edited code are also counted as full-task failures. `NO-AUDIT` means the snapshot compiled but only has older subcheck-level or fallback judge metadata, so the full-task audit outcome is unknown.

| Model | Reasoning | Valid snapshots | Full-task audited/known | Full-task pass rate | Raw pass rate | Test-calibrated pass rate | Notes |
|-------|-----------|-----------------|-------------------------|---------------------|---------------|---------------------------|-------|
| Opus 4.8 | high | 71/71 | 71 | **27/71 = 38.0%** | 34/71 = 47.9% | 33/71 = 46.5% | 0004/0007 retests audited fail |
| Opus 4.7 | high | 71/71 | 70 | **23/70 = 32.9%** | 25/71 = 35.2% | 30/71 = 42.3% | no full-task audit: 0067 |
| GPT-5.5 | medium | 71/71 | 71 | **25/71 = 35.2%** | 25/71 = 35.2% | 30/71 = 42.3% | 0004/0007 audited fail; 0051 crash fail |
| GPT-5.5 | high | 71/71 | 71 | **27/71 = 38.0%** | 29/71 = 40.8% | 35/71 = 49.3% | 0004/0007 audited fail; 0047 crash fail |

### Per-task full-task audit matrix

| Task | Opus 4.8 high | Opus 4.7 high | GPT-5.5 medium | GPT-5.5 high |
|------|---------------|---------------|----------------|--------------|
| ue_task_0001 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0002 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0003 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0004 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0005 | PASS | PASS | PASS | PASS |
| ue_task_0006 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0007 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0008 | PASS | PASS | FAIL | PASS |
| ue_task_0009 | PASS | PASS | PASS | PASS |
| ue_task_0010 | PASS | PASS | PASS | PASS |
| ue_task_0011 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0012 | PASS | PASS | PASS | PASS |
| ue_task_0013 | PASS | PASS | FAIL | PASS |
| ue_task_0014 | FAIL | FAIL | PASS | PASS |
| ue_task_0015 | PASS | PASS | PASS | PASS |
| ue_task_0016 | PASS | PASS | PASS | PASS |
| ue_task_0017 | PASS | PASS | PASS | PASS |
| ue_task_0018 | PASS | PASS | PASS | PASS |
| ue_task_0019 | PASS | PASS | PASS | PASS |
| ue_task_0020 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0021 | PASS | FAIL | FAIL | FAIL |
| ue_task_0022 | PASS | PASS | PASS | PASS |
| ue_task_0023 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0024 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0025 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0026 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0027 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0028 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0029 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0030 | FAIL | FAIL | PASS | PASS |
| ue_task_0031 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0032 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0033 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0034 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0035 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0036 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0037 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0038 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0039 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0040 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0041 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0042 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0043 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0044 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0045 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0046 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0047 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0048 | PASS | PASS | FAIL | FAIL |
| ue_task_0049 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0050 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0051 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0052 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0053 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0054 | PASS | PASS | PASS | PASS |
| ue_task_0055 | PASS | PASS | PASS | PASS |
| ue_task_0056 | PASS | PASS | PASS | PASS |
| ue_task_0057 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0058 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0059 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0060 | PASS | PASS | PASS | PASS |
| ue_task_0061 | PASS | PASS | PASS | PASS |
| ue_task_0062 | PASS | PASS | PASS | PASS |
| ue_task_0063 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0064 | PASS | PASS | PASS | PASS |
| ue_task_0065 | PASS | PASS | PASS | PASS |
| ue_task_0066 | PASS | FAIL | PASS | PASS |
| ue_task_0067 | PASS | NO-AUDIT | PASS | PASS |
| ue_task_0068 | PASS | FAIL | PASS | PASS |
| ue_task_0069 | FAIL | FAIL | FAIL | FAIL |
| ue_task_0070 | PASS | PASS | PASS | PASS |
| ue_task_0071 | PASS | PASS | PASS | PASS |

---

## How to read this

Each task result carries three independent signals:

1. **Did it pass?** — `task_passed` (every test in the task passed).
2. **Is the result trustworthy?** — an LLM **judge** (Opus 4.7, `atomic` mode) re-examines every
   test outcome against whether the model's code was *actually* correct, emitting a **trust_score**
   (fraction of outcomes the judge agrees with) plus:
   - **FP** = test **failed** but code judged **correct** → the *test* is wrong, not the model.
   - **FN** = test **passed** but code judged **wrong** → the test is too lenient.
3. **Compile failures count as failures.** A model that emits non-compiling code failed the task —
   it is a genuine model failure, not an excluded/uanalyzable case.

### Status legend

| Status | Meaning | Counts as |
|--------|---------|-----------|
| **PASS** | All tests passed. | pass |
| **FAIL** | Failed; judge agrees ≥1 failing test caught a real bug (genuine model miss). | fail |
| **FAIL-compile** | Model's code did not compile; 0 tests ran. | fail |
| **FAIL?** | Failed, but **every** failing test was judged a false alarm (TP=0, FP>0). The model arguably solved it; the **task's tests need fixing** before the result is trustworthy. | fail (but suspect) |

**FAIL?** (suspect tests) is the "needs critical improvement before it can be cleanly analyzed"
bucket. Compile failures are **not** in that bucket anymore — they're real model failures.

### Pass-rate definitions

| Name | Ground truth | Formula | Meaning |
|------|--------------|---------|---------|
| **Raw Pass Rate** | the **tests** | (tasks whose tests all passed) ? valid snapshots | Conservative floor. Compile failures and suspect-test tasks all count as failures. |
| **Test-Calibrated Pass Rate** | subcheck-level LLM judge | (tasks with compile success, TP=0, FN=0) ? valid snapshots | Credits false-positive test failures and denies false-negative test passes at the tested-behavior level. |
| **Full-Task Pass Rate** | full-task LLM audit | (tasks whose submission satisfies the full task) ? valid snapshots with a known full-task outcome | Stricter task-level judgment. Report audit coverage beside this number because older records may not contain the full-task audit field. |

Raw and Test-Calibrated use the same denominator (valid task snapshots). Full-Task uses only valid
snapshots with a known full-task outcome. Compile failures are known full-task failures. Compiled
snapshots without full-task judge fields are listed as **NO-AUDIT** instead of being silently counted either way.

> Caveat: the judge is itself an LLM (Opus 4.7), not ground truth. Test-Calibrated and Full-Task Pass
> Rates can over-credit if the judge is wrong. Treat them as audit-based estimates, not verified scores.

---

## Suggested additional metrics to track

In rough priority order:

1. **Baseline (solution) validation per task** — does each task's *reference solution* pass its own
   tests (and the *start* stub fail them)? A task whose own solution can't reach n/n is broken,
   independent of any model. Best single filter for "is this task analyzable." The trust-0.00 tasks
   (0008, 0027, 0048) are prime suspects to validate first.
2. **Cross-model agreement** — same task across Opus 4.8 / Opus 4.7 / GPT-5.5. A task *all* models
   fail (especially with FP) is almost certainly broken, not hard. Separates "hard" from "broken"
   far better than the judge alone.
3. **Subcheck-level health** — `coverage_summary` shows many "unknown" subchecks (CDO/reflection
   checks silently not executing); worth tracking as harness health.
4. **Run metadata** — git commit, exact command, model id, judge model, date — for reproducibility.
5. **Cost & time per task** (captured here) — model efficiency, not just accuracy.

