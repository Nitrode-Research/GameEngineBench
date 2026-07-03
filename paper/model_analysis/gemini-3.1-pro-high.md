# Gemini 3.1 Pro High Result Audit

Model/config:
- Agent wrapper: `antigravity-cli`
- Model: `Gemini 3.1 Pro (High)`
- Snapshots present: 120/120

## Run Validity

Using the current paper definition of a good run:
- Compile failure counts as a valid run.
- Harness/test-runner failure counts as a valid run.
- Rate limit, authentication failure, or timeout counts as invalid.

Corrected status:

| Metric | Count |
|---|---:|
| Snapshots present | 120/120 |
| Good runs | 119/120 |
| Invalid solver runs | 1/120 |
| Solver reruns needed | 1 |
| False `solver_rate_limited` flags | 48 |

## Flagged Runs

Forty-eight Gemini runs were initially flagged as `solver_rate_limited=true`. Manual inspection found no real provider/API rate-limit evidence in those flagged runs. They either ran hidden tests or reached compile-fallback normally.

The false flags appear stale or incorrectly propagated rather than real Gemini API failures.

## Invalid Run

The only invalid Gemini run is:

```text
ue_task_0091_antigravity-cli_20260630_045648
```

Reason:

```text
Antigravity CLI is not authenticated. Run `agy` once interactively to sign in (Google account / device code), then re-run the benchmark.
```

This is a CLI authentication failure, not a model result.

## Evaluation States

| State | Count |
|---|---:|
| Real hidden tests ran | 85 |
| Compile failed | 35 |
| Compile succeeded but tests parsed 0 | 0 |

Gemini compile failures:

```text
0006, 0007, 0012, 0013, 0014, 0015, 0021, 0022, 0033, 0035,
0037, 0038, 0040, 0041, 0042, 0043, 0044, 0045, 0046, 0048,
0049, 0069, 0076, 0077, 0078, 0079, 0091, 0094, 0100, 0101,
0105, 0106, 0107, 0108, 0110
```

Only `0091` is invalid due to authentication. The other compile failures should be kept as valid benchmark outcomes.

## Recommendation

Rerun only:

```text
0091
```

Do not rerun the other Gemini snapshots. Treat the remaining compile failures as valid model outcomes.

