# Qwen 3.7 Plus Result Audit

Model/config:
- Agent wrapper: `qwen-code`
- Model: `qwen3.7-plus`
- Snapshots present: 120/120

## Run Validity

Using the current paper definition of a good run:
- Compile failure counts as a valid run.
- Harness/test-runner failure counts as a valid run.
- Rate limit, authentication failure, timeout, or wrapper failure before a meaningful solver output counts as invalid.

Corrected status:

| Metric | Count |
|---|---:|
| Snapshots present | 120/120 |
| Good runs | 119/120 |
| Invalid solver runs | 1/120 |
| Solver reruns needed | 1 |
| False `solver_rate_limited` flags | 11 |

## Flagged Runs

Eleven Qwen runs were initially flagged as `solver_rate_limited=true`:

```text
0002, 0014, 0017, 0025, 0047, 0048, 0052, 0091, 0096, 0099, 0100
```

Manual inspection found no real provider/API rate-limit evidence. The apparent matches came from benign source text, compiler paths, line numbers, or Unreal enum text such as `TooManyRequests`.

These runs should be treated as valid unless separately invalidated by another solver-wrapper failure.

## Invalid Run

The only invalid Qwen run is:

```text
ue_task_0084_qwen-code_20260629_015920
```

Reason:

```text
Qwen Code failed with exit code 1
STDERR: The command line is too long.
```

This snapshot has zero edited files and should be treated as a wrapper/CLI failure rather than a model result.

## Solver-Success False But Kept

Several snapshots have `solver_success=false` because the Qwen wrapper exited with code 1, but they produced edited files and reached compile/test evaluation. These are kept as valid benchmark outcomes:

```text
0037, 0065, 0069, 0101, 0106
```

`0065` even compiled and passed its hidden test despite the wrapper exit code.

## Evaluation States

| State | Count |
|---|---:|
| Real hidden tests ran | 38 |
| Compile succeeded but tests parsed 0 | 39 |
| Compile failed | 43 |

Qwen runs with compile success but zero parsed tests:

```text
0001, 0002, 0003, 0004, 0005, 0006, 0007, 0008, 0009, 0010,
0012, 0013, 0015, 0016, 0017, 0018, 0019, 0020, 0021, 0032,
0034, 0036, 0038, 0039, 0040, 0041, 0042, 0043, 0044, 0045,
0046, 0047, 0048, 0050, 0057, 0058, 0059, 0060, 0063
```

Qwen compile failures:

```text
0023, 0024, 0025, 0026, 0027, 0028, 0035, 0037, 0049, 0051,
0052, 0053, 0055, 0056, 0061, 0062, 0068, 0069, 0070, 0071,
0072, 0073, 0075, 0076, 0077, 0078, 0079, 0081, 0085, 0086,
0088, 0089, 0090, 0091, 0092, 0093, 0094, 0095, 0098, 0100,
0101, 0102, 0106
```

## Recommendation

Rerun only:

```text
0084
```

Do not rerun the other Qwen snapshots. If cleaner hidden-test outcomes are needed, run evaluation-only retest for the 39 zero-test snapshots listed above. Keep compile failures as valid benchmark outcomes.

