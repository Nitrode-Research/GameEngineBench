# UnrealBench figure index

Generated from `C:\Users\Administrator\Documents\unrealbench\tasks_unreal\test_result` (tasks 1–120, 110 task specs).

All figures are written as both `.pdf` (LaTeX input) and `.png` (preview).

## Models

- **Claude Fable 5 (max)** (claude-code/claude-fable-5/max) — 110/110 tasks, calibrated pass 57.5%
- **GPT-5.5 (xhigh)** (codex/gpt-5.5/xhigh) — 110/110 tasks, calibrated pass 30.2%
- **Claude Opus 4.8 (max)** (claude-code/claude-opus-4-8/max) — 110/110 tasks, calibrated pass 24.8%
- **GPT-5.5 (high)** (codex/gpt-5.5/high) — 110/110 tasks, calibrated pass 19.8%
- **Gemini 3.1 Pro (High)** (antigravity-cli/['*Gemini*', 'gemini-pro-default']/*) — 110/110 tasks, calibrated pass 24.7%
- **Claude Opus 4.8 (high)** (claude-code/claude-opus-4-8/high) — 110/110 tasks, calibrated pass 13.6%
- **Claude Opus 4.7 (high)** (claude-code/claude-opus-4-7/high) — 110/110 tasks, calibrated pass 10.4%
- **GPT-5.5 (medium)** (codex/gpt-5.5/medium) — 110/110 tasks, calibrated pass 9.5%
- **Kimi for Coding** (kimi-code/kimi-code/kimi-for-coding/default) — 110/110 tasks, calibrated pass 15.0%
- **Claude Sonnet 4.6 (high)** (claude-code/claude-sonnet-4-6/high) — 110/110 tasks, calibrated pass 7.6%
- **Qwen 3.7 Plus** (qwen-code/qwen3.7-plus/default) — 110/110 tasks, calibrated pass 11.4%
- **DeepSeek 4 Pro** (qwen-code/deepseek-v4-pro/default) — 110/110 tasks, calibrated pass 13.0%

## Figures

- `01_pass_rate.pdf` — Headline pass rate per model under the selected scoring lens.
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

- `21_calibrated_success_rate.pdf` - paper-style regenerated main-text figure.
- `25_requirement_vs_pass.pdf` - paper-style regenerated main-text figure.
- `18_efficiency_frontier.pdf` - paper-style regenerated main-text figure.
- `28_calibrated_coverage_complementarity.pdf` - paper-style regenerated main-text figure.
- `27_task_area_coverage.pdf` - paper-style regenerated main-text figure.
- `11_failure_modes.pdf` - paper-style regenerated main-text figure.