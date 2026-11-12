# Scheduled Wakeup Mechanics & Multi-GPU Packing Guide

## 1. Verifying Native Timer Capability (Step 0 Gate)

A Worker runtime **passes** the timer gate only if it exposes an agent-controllable scheduled wakeup primitive that lets the current turn end cleanly and automatically resumes/wakes the Worker after the target interval—for example:
- Built-in session timer / cron / schedule tools (one-shot duration timers or recurring cron expressions);
- Built-in `/loop`, `ScheduleWakeup`, `CronCreate`, self-reminder timers, or agent `automations` tools;
- Installed session-scheduler plugins registered as native agent tools.

A Worker runtime **fails** the timer gate if:
- No scheduling/timer/wakeup tool is available in the current session (for instance, vanilla core OpenCode without a scheduler plugin, or a headless non-interactive CLI run where wakeups are disabled);
- The only way to wait would be holding open a foreground shell `sleep` loop. **Never fake scheduled wakeups with blocking `sleep` loops.**

When the gate fails, immediately send `NO_TIMER_CAPABILITY` to Commander and stop.

---

## 2. Exact Wakeup Period Calculation & Examples

Let $N_{\text{active}}$ be the number of server experiments currently running concurrently:

| Concurrent Active Jobs ($N_{\text{active}}$) | Estimated Remaining Time ($T_{\text{rem}}$) | One-Fifth ($X = T_{\text{rem}} / 5$) | Scheduled Wakeup Period |
| --- | --- | --- | --- |
| `0` (all done / none runnable) | — | — | **Cancel all timers & stop** |
| `1` | `40 min` | `8 min` (`< 15`) | **`15 min`** (lower clamp) |
| `1` | `75 min` | `15 min` | **`15 min`** |
| `1` | `110 min` | `22 min` (`15 <= X <= 30`) | **`22 min`** |
| `1` | `150 min` | `30 min` | **`30 min`** |
| `1` | `300 min` | `60 min` (`> 30`) | **`30 min`** (upper clamp) |
| `>= 2` (2, 3, 4+ jobs running) | any | — | **`30 min`** (fixed multi-job cadence) |

### How to Estimate $T_{\text{rem}} = 5X$ Accurately
- **Initial launch**: run 5–20 warmup/logged steps (or inspect throughput from the first 30–60 seconds of startup logs) to extrapolate total wall-clock minutes (`remaining_steps * sec_per_step / 60` + eval overhead), falling back to the spec's `estimated_wall_minutes` hint only before step timing is observable.
- **After bug fix & relaunch**: discard old timing estimates, measure step rate on the fixed run, and recompute $T_{\text{rem}}$ and $X$ from scratch.
- **After a job finishes and next job(s) launch**: always cancel the previous timer first, launch all newly fittable specs, and compute a brand-new wakeup period for the updated active set.

### One-Shot vs. Recurring Timer Primitives
- **Recurring cron/loop tool** (e.g., cron expression `*/22 * * * *` or `*/30 * * * *`):
  - Keep the cron task ID in `artifacts/active_runs.json`.
  - When all jobs are healthy and $N_{\text{active}}$ / period has not changed, leave the recurring timer running.
  - Whenever **any** job finishes, crashes, or a new job starts, **delete/cancel** that recurring timer immediately before repairing, summarizing, or re-scheduling.
- **One-shot wakeup tool** (e.g., `DurationSeconds = period_mins * 60`):
  - On each healthy wakeup where work continues, re-arm a fresh one-shot wakeup for `period_mins * 60` seconds right before ending the turn.
  - If a timer is still pending when an early event wakes you, cancel the old timer ID before creating a replacement.

---

## 3. Out-of-Order & Parallel Multi-GPU Bin-Packing

To prevent serial bottlenecks that waste server GPUs:

1. **Build the Ready Set**:
   - Parse all `spec` rows in the Queue file that do not yet have a `result` row.
   - Read each pending spec's `Scheduling & Resource Profile`.
   - Mark a spec **Ready** if `depends_on` is `none` or every listed prerequisite ID already has a `result` row in the Queue file.

2. **Query Live Server Capacity**:
   ```bash
   nvidia-smi --query-gpu=index,name,memory.free,memory.total,utilization.gpu --format=csv,noheader,nounits
   free -g
   df -BG <work_dir>
   ```
   Also inspect `nvidia-smi topo -m` when any ready spec requests `gpu_count >= 2` or `prefer_nvlink_pair: true`.

3. **Greedy Resource-Aware Packing**:
   - Sort Ready specs by `(priority_tier ASC, gpu_count DESC, estimated_wall_minutes DESC)`—packing multi-GPU / tier-1 jobs onto clean NVLink pairs first, then filling all remaining free GPUs with independent single-GPU specs out of order!
   - Before adding candidate spec $S$ to the active set, verify all four constraints simultaneously:
     1. Enough GPUs have `free_vram_gib >= S.min_vram_gib_per_gpu` (plus safety margin, e.g., `2–4 GiB`);
     2. If `S.exclusive_gpu_timing == true` (or an already-assigned job on that GPU has `exclusive_gpu_timing == true`), do **not** co-locate another campaign job on the same physical GPU index;
     3. $\sum_{\text{active} \cup \{S\}} \text{host\_ram\_budget\_gib} \le \text{Contract Host RAM Ceiling}$;
     4. $\sum_{\text{active} \cup \{S\}} \text{disk\_peak\_gib} \le \text{Contract Disk Ceiling}$.

---

## 4. Detached Launch & `artifacts/active_runs.json` Ledger

Always launch experiments so they survive across agent turns without TTY attachment:

```bash
nohup bash -c '
  export CUDA_VISIBLE_DEVICES=0
  python3 -u run_exp.py --config configs/E001.json > logs/E001.log 2>&1
  echo $? > logs/E001.exit
' >/dev/null 2>&1 &
echo $! > logs/E001.pid
```

Maintain `artifacts/active_runs.json` on every launch, wakeup, and completion:

```json
{
  "updated_at": "2026-11-13T01:00:00+08:00",
  "active_timer": {
    "timer_id": "<runtime-timer-id>",
    "mode": "recurring_or_oneshot",
    "period_minutes": 30
  },
  "active_jobs": [
    {
      "id": "C001",
      "gpus": [0, 1],
      "exclusive_gpu_timing": true,
      "pid": 184920,
      "host": "lab_H100",
      "log_path": "/path/to/logs/C001.log",
      "exit_file": "/path/to/logs/C001.exit",
      "started_at": "2026-11-13T00:30:00+08:00",
      "estimated_remaining_minutes": 45,
      "host_ram_budget_gib": 24,
      "disk_peak_gib": 10
    },
    {
      "id": "C003",
      "gpus": [6],
      "exclusive_gpu_timing": false,
      "pid": 185104,
      "host": "lab_H100",
      "log_path": "/path/to/logs/C003.log",
      "exit_file": "/path/to/logs/C003.exit",
      "started_at": "2026-11-13T00:31:00+08:00",
      "estimated_remaining_minutes": 120,
      "host_ram_budget_gib": 16,
      "disk_peak_gib": 40
    }
  ]
}
```
