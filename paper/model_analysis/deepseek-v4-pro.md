# DeepSeek v4 Pro Result Audit

Model/config:
- Agent wrapper: `qwen-code`
- Model: `deepseek-v4-pro`
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

## Flagged Runs

Eight DeepSeek runs were initially flagged as invalid because `solver_rate_limited=true`:

```text
0002, 0008, 0017, 0036, 0037, 0104, 0108, 0112
```

Manual inspection shows these are false positives. The rate-limit detector matched benign text such as:

- `fire rate limiting`
- `frame rate limiting`
- `.RateLimiting.dll`
- Unreal/compiler line references such as `ParticleSystem.h(429,15)`

No provider/API rate-limit evidence was found.

## Per-Task Classification

| Task | Status | Classification |
|---|---|---|
| `0002` | Compile OK, tests parsed 0 | Harness/test parser issue; keep model run |
| `0008` | Compile OK, tests parsed 0 | Harness/test parser issue; keep model run |
| `0017` | Compile OK, tests parsed 0 | Harness/test parser issue; keep model run |
| `0036` | Compile failed | Valid model compile failure |
| `0037` | Compile OK, tests parsed 0 | Harness/test parser issue; keep model run |
| `0104` | Compile failed | Valid model compile failure |
| `0108` | Compile failed | Valid model compile failure |
| `0112` | Compile failed | Valid model compile failure |

## Recommendation

Do not rerun the DeepSeek solver.

If cleaner hidden-test outcomes are needed, run evaluation-only retest for:

```text
0002, 0008, 0017, 0037
```

