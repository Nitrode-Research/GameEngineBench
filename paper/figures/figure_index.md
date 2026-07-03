# UnrealBench figure index

Generated from `C:\Users\Administrator\Documents\unrealbench\tasks_unreal\test_result` (tasks 1–120, 120 task specs).

All figures are written as both `.pdf` (LaTeX input) and `.png` (preview).

## Models

- **GPT-5.5 (xhigh)** (codex/gpt-5.5/xhigh) — 120/120 tasks, calibrated pass 28.7%
- **Claude Opus 4.8 (max)** (claude-code/claude-opus-4-8/max) — 120/120 tasks, calibrated pass 23.5%
- **GPT-5.5 (high)** (codex/gpt-5.5/high) — 120/120 tasks, calibrated pass 19.3%
- **Gemini 3.1 Pro (High)** (antigravity-cli/['*Gemini*', 'gemini-pro-default']/*) — 120/120 tasks, calibrated pass 24.4%
- **Claude Opus 4.8 (high)** (claude-code/claude-opus-4-8/high) — 120/120 tasks, calibrated pass 13.3%
- **Claude Opus 4.7 (high)** (claude-code/claude-opus-4-7/high) — 120/120 tasks, calibrated pass 10.4%
- **GPT-5.5 (medium)** (codex/gpt-5.5/medium) — 120/120 tasks, calibrated pass 9.6%
- **Kimi for Coding** (kimi-code/kimi-code/kimi-for-coding/default) — 120/120 tasks, calibrated pass 15.6%
- **DeepSeek 4 Pro** (qwen-code/deepseek-v4-pro/default) — 120/120 tasks, calibrated pass 16.7%
- **Qwen 3.7 Plus** (qwen-code/qwen3.7-plus/default) — 120/120 tasks, calibrated pass 10.5%

## Figures

- `01_pass_rate.pdf` — Headline pass rate per model under the selected scoring lens (Wilson 95% CIs).
- `02_pass_rate_breakdown.pdf` — Four scoring lenses side by side: raw all-tests-pass, judge-adjusted full-task verdict, mean fraction of requirements satisfied, and micro-averaged test pass rate.
- `03_task_outcome_matrix.pdf` — Per-task outcome matrix (green pass, red fail, blank not-yet-run) across all 120 tasks.
- `04_pass_rate_by_tier.pdf` — Pass rate split by task difficulty tier.
- `05_pass_rate_by_domain.pdf` — Pass rate by game-system domain (ALS, GAS, Common UI, SPUD, Nanite/Gaussian, …).
- `06_pass_rate_by_category.pdf` — Pass rate by requirement category (top categories by task count).
- `07_time_vs_pass_rate.pdf` — Efficiency frontier: median per-task solve time vs. pass rate; bubble size scales with tasks completed.
- `08_cost_vs_pass_rate.pdf` — Cost frontier: mean per-task USD cost vs. pass rate (populates as cost data lands for every agent).
- `09_output_tokens.pdf` — Mean output tokens spent per task, over all tasks vs. only passed tasks.
- `10_pass_rate_vs_steps.pdf` — Pass rate as a function of agent steps (tool calls/turns), binned by tercile.
- `11_failure_modes.pdf` — Outcome decomposition: pass, tests-fail, judge-disputed (tests fail but judge deems satisfied), compile-fail.
- `12_task_difficulty_curve.pdf` — Tasks ranked by the fraction of models that solve them — the benchmark's difficulty profile.
- `13_solved_by_n_models.pdf` — Distribution of per-task solve breadth (how many models solve each task).
- `14_model_complementarity.pdf` — Pairwise complementarity: tasks the row model solves that the column model misses.
- `15_coverage_progress.pdf` — Benchmark completion tracker — fraction of tasks each model has run so far.
- `16_compile_calls.pdf` — Mean number of Unreal compiler invocations (completed UBT builds) per task, parsed from agent trajectories.
- `17_compile_errors.pdf` — Mean number of Unreal compile errors encountered per task while solving, parsed from agent trajectories.

## Audit data

- `summary.csv` — one row per model config with every headline metric.
- `task_matrix.csv` — PASS/FAIL per task per model.
- `per_run.csv` — the selected latest run for every (model, task).
