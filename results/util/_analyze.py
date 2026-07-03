"""Benchmark analysis for BENCHMARK_PROGRESS.md.

Usage: python results/util/_analyze.py <model_id> [agent] [reasoning_level]
  e.g. python results/util/_analyze.py claude-opus-4-8
       python results/util/_analyze.py claude-opus-4-7
       python results/util/_analyze.py gpt-5.5 codex medium
       python results/util/_analyze.py gpt-5.5 codex high

Covers tasks 1-71 with 1-50 / 51-71 / 1-71 subtotals.
  Raw Pass Rate             = tests as truth  (task_passed).
  Test-Calibrated Pass Rate = test-outcome judge as truth (compiled AND TP==0 AND FN==0).
  Full-Task Pass Rate       = full-task judge audit (compiled AND submission_satisfies_full_task=true), when present.
For codex runs, pass the reasoning level (medium/high) — runs of different levels share the
model id `gpt-5.5`, so without it the latest-by-timestamp pick silently MIXES configs.
"""
import json, glob, re, sys

model = sys.argv[1] if len(sys.argv) > 1 else 'claude-opus-4-8'
agent = sys.argv[2] if len(sys.argv) > 2 else 'claude-code'
level = sys.argv[3] if len(sys.argv) > 3 else None
LO, HI = 1, 71

# Confirmed from debug retests on 2026-06-25: UE started the automation test and
# then crashed in model-edited code before emitting automation result rows.
KNOWN_FULL_TASK_FAILURES = {
    ('ue_task_0047', 'gpt-5.5', 'codex', 'high'),
    ('ue_task_0051', 'gpt-5.5', 'codex', 'medium'),
}

def reasoning_level(d):
    return d.get('reasoning_level_applied') or d.get('reasoning_level_requested') or 'default'

def is_known_full_task_failure(tid, d):
    return (
        not d.get('compile_success')
        or (tid, d.get('model'), d.get('agent'), reasoning_level(d)) in KNOWN_FULL_TASK_FAILURES
    )

def collect(model, agent, level):
    rows, levels_seen = {}, set()
    for f in glob.glob(f'tasks_unreal/test_result/ue_task_*_{agent}_*/result.json'):
        m = re.search(r'(ue_task_\d{4})_' + re.escape(agent) + r'_(\d{8}_\d{6})', f.replace('\\', '/'))
        if not m:
            continue
        tid, ts = m.group(1), m.group(2)
        n = int(tid.split('_')[-1])
        if not (LO <= n <= HI):
            continue
        try:
            d = json.load(open(f, encoding='utf-8'))
        except Exception:
            continue
        if d.get('model') != model:
            continue
        lvl = reasoning_level(d)
        if lvl is not None:
            levels_seen.add(lvl)
        if level is not None and lvl != level:
            continue
        if tid not in rows or ts > rows[tid][0]:
            rows[tid] = (ts, d)
    return rows, levels_seen

rows, levels_seen = collect(model, agent, level)
if level is None and len(levels_seen) > 1:
    print(f'WARNING: runs span multiple reasoning levels {sorted(levels_seen)} — pass one as the '
          f'3rd arg (e.g. `{model} {agent} {sorted(levels_seen)[0]}`) to avoid mixing configs.\n')

sub = {
    (1, 50): [0, 0, 0, 0, 0],
    (51, 71): [0, 0, 0, 0, 0],
    (1, 71): [0, 0, 0, 0, 0],
}  # [valid, raw, test_cal, spec_known, spec_pass]
present, table = [], []
tot_cost = tot_dur = 0.0
for n in range(LO, HI + 1):
    tid = f'ue_task_{n:04d}'
    if tid not in rows:
        continue
    d = rows[tid][1]
    if len(d.get('files_edited') or []) == 0:   # no-op = invalid measurement
        continue
    present.append(n)
    j = d.get('judge') or {}
    c = j.get('counts') or {}
    fp, fn, tp = c.get('FP', 0), c.get('FN', 0), c.get('TP', 0)
    raw = 1 if d.get('task_passed') else 0
    cal = 1 if (not is_known_full_task_failure(tid, d) and d.get('compile_success') and j and tp == 0 and fn == 0) else 0
    spec_known = 0
    spec_pass = 0
    if is_known_full_task_failure(tid, d):
        spec_known = 1
        spec_pass = 0
    elif j.get('mode') == 'atomic' and 'submission_satisfies_full_task' in j:
        spec_known = 1
        spec_pass = 1 if (d.get('compile_success') and j.get('submission_satisfies_full_task') is True) else 0
    elif j.get('mode') == 'dense' and 'submission_verdict' in j:
        spec_known = 1
        spec_pass = 1 if (d.get('compile_success') and j.get('submission_verdict') == 'CORRECT') else 0
    tot_cost += d.get('cost_usd') or 0
    tot_dur += (d.get('duration_seconds') or 0) / 60
    for span in sub:
        if span[0] <= n <= span[1]:
            sub[span][0] += 1
            sub[span][1] += raw
            sub[span][2] += cal
            sub[span][3] += spec_known
            sub[span][4] += spec_pass
    if raw:
        st = 'PASS'
    elif not d.get('compile_success'):
        st = 'FAIL-compile'
    elif fp > 0 and tp == 0:
        st = 'FAIL?'
    else:
        st = 'FAIL'
    ts = j.get('trust_score')
    table.append((tid, st, f"{d.get('tests_passed') or 0}/{d.get('tests_run') or 0}",
                  f'{ts:.2f}' if ts is not None else '-', fp, fn, (d.get('duration_seconds') or 0) / 60))

tag = f'{model} / {agent}' + (f' / reasoning={level}' if level else '')
missing = [n for n in range(LO, HI + 1)
           if n not in present and glob.glob(f'tasks_unreal/ue_task_{n:04d}_*')]
print(f'MODEL: {tag}')
print(f'valid tasks: {len(present)} | missing/no-op: {missing}\n')
for span in [(1, 50), (51, 71), (1, 71)]:
    v, r, cal, spec_known, spec_pass = sub[span]
    if v:
        print(f'  tasks {span[0]:>2}-{span[1]}: Raw {r}/{v} = {r/v*100:4.1f}%   '
              f'Test-Calibrated {cal}/{v} = {cal/v*100:4.1f}%'
              + (f'   Full-Task {spec_pass}/{spec_known} = {spec_pass/spec_known*100:4.1f}%'
                 if spec_known else ''))
print(f'\ncost=${tot_cost:.2f}  time={tot_dur:.0f}min ({tot_dur/60:.1f}h)'
      + ('   [codex cost often unrecorded]' if agent == 'codex' else ''))
print('\n| Task | Status | Tests | Trust | FP | FN | Min |')
print('|------|--------|-------|-------|----|----|-----|')
for r in table:
    print(f'| {r[0]} | {r[1]} | {r[2]} | {r[3]} | {r[4]} | {r[5]} | {r[6]:.1f} |')
