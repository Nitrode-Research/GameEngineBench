# Claude Opus 4.7 High Result Audit

Model/config:
- Agent wrapper: `claude-code`
- Model: `claude-opus-4-7`
- Reasoning level: `high`
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
| Good runs | 120/120 |
| Invalid solver runs | 0/120 |
| Solver reruns needed | 0 |
| False `solver_rate_limited` flags | 0 |

## Coverage

This config has full 120-task coverage when grouped by `reasoning_level_applied=high`.

The first 42 tasks were recorded with `reasoning_level_requested=default` but `reasoning_level_applied=high`; the later 78 tasks were recorded as `high/high`. These results were not deleted. They are split by requested-label metadata.

Requested-label split:

| Requested label | Applied label | Tasks |
|---|---|---:|
| `default` | `high` | 42 |
| `high` | `high` | 78 |

Tasks in the `default/high` slice:

```text
0001, 0002, 0003, 0005, 0006, 0008, 0009, 0010, 0012, 0013,
0015, 0016, 0017, 0018, 0019, 0020, 0021, 0022, 0032, 0033,
0034, 0035, 0036, 0037, 0038, 0039, 0040, 0041, 0042, 0043,
0044, 0045, 0046, 0047, 0048, 0049, 0050, 0057, 0058, 0059,
0063, 0069
```

Tasks in the `high/high` slice:

```text
0004, 0007, 0011, 0014, 0023, 0024, 0025, 0026, 0027, 0028,
0029, 0030, 0031, 0051, 0052, 0053, 0054, 0055, 0056, 0060,
0061, 0062, 0064, 0065, 0066, 0067, 0068, 0070, 0071, 0072,
0073, 0074, 0075, 0076, 0077, 0078, 0079, 0080, 0081, 0082,
0083, 0084, 0085, 0086, 0087, 0088, 0089, 0090, 0091, 0092,
0093, 0094, 0095, 0096, 0097, 0098, 0099, 0100, 0101, 0102,
0103, 0104, 0105, 0106, 0107, 0108, 0109, 0110, 0111, 0112,
0113, 0114, 0115, 0116, 0117, 0118, 0119, 0120
```

## Evaluation States

| State | Count |
|---|---:|
| Real hidden tests ran | 115 |
| Compile failed | 5 |
| Compile succeeded but tests parsed 0 | 0 |

Compile failures:

```text
0076, 0077, 0078, 0079, 0081
```

These compile failures should be kept as valid benchmark outcomes.

## Recommendation

No reruns are needed. Treat this as a complete 120-task `claude-opus-4-7` high-applied result set.
