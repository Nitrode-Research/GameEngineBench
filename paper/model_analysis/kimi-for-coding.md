# Kimi for Coding Result Audit

Model/config:
- Agent wrapper: `kimi-code`
- Model: `kimi-code/kimi-for-coding`
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
| Good runs | 120/120 |
| Invalid solver runs | 0/120 |
| Solver reruns needed | 0 |
| False `solver_rate_limited` flags | 25 |

## Flagged Runs

Twenty-five Kimi runs were initially flagged as `solver_rate_limited=true`:

```text
0008, 0011, 0013, 0014, 0015, 0016, 0018, 0020, 0029, 0032,
0037, 0056, 0059, 0068, 0090, 0092, 0095, 0100, 0104, 0105,
0106, 0107, 0108, 0112, 0113
```

Manual inspection found no real Kimi provider rate-limit or timeout evidence. The detector matched benign text such as:

- Unreal/compiler line numbers like `429`
- source comments mentioning rate limiting
- `.RateLimiting.dll`
- Unreal enum text such as `TooManyRequests`

These should be treated as valid runs.

## Evaluation States

| State | Count |
|---|---:|
| Real hidden tests ran | 64 |
| Compile succeeded but tests parsed 0 | 45 |
| Compile failed | 11 |

Kimi runs with real hidden-test execution:

```text
0007, 0008, 0010, 0014, 0016, 0022, 0024, 0025, 0026, 0027,
0028, 0029, 0030, 0031, 0034, 0043, 0045, 0050, 0051, 0052,
0053, 0054, 0055, 0056, 0060, 0061, 0062, 0064, 0065, 0066,
0067, 0068, 0070, 0071, 0072, 0073, 0074, 0075, 0080, 0082,
0083, 0084, 0085, 0086, 0087, 0088, 0089, 0090, 0091, 0092,
0093, 0094, 0095, 0096, 0097, 0098, 0099, 0100, 0105, 0107,
0111, 0112, 0115, 0120
```

Kimi runs with compile success but zero parsed tests:

```text
0001, 0003, 0004, 0005, 0006, 0009, 0011, 0012, 0013, 0015,
0017, 0018, 0019, 0020, 0021, 0032, 0033, 0035, 0036, 0037,
0038, 0039, 0040, 0041, 0042, 0044, 0048, 0049, 0057, 0058,
0059, 0063, 0069, 0102, 0103, 0104, 0106, 0109, 0110, 0113,
0114, 0116, 0117, 0118, 0119
```

Kimi compile failures:

```text
0002, 0023, 0046, 0047, 0076, 0077, 0078, 0079, 0081, 0101, 0108
```

## Recommendation

Do not rerun the Kimi solver.

If cleaner hidden-test outcomes are needed, run evaluation-only retest for the 45 zero-test snapshots listed above. Keep the 11 compile failures as valid benchmark outcomes.

