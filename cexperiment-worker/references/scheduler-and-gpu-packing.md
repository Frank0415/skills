# Scheduled Wakeup Mechanics & Multi-GPU Packing Guide

## 1. Self-Contained Worker CLI (Never Install to `PATH`)

Invoke the bundled multi-OS binary launcher directly from the skill folder (works out of the box on Linux `x86_64`/`aarch64` and macOS `arm64`/`x86_64` with zero `PATH` or host changes):

```bash
<cexperiment-worker-skill-dir>/scripts/cexp-worker --help
```

---

## 2. Verifying Native Timer Capability (Step 0 Gate)

A Worker runtime **passes** the timer gate only if it exposes an agent-controllable scheduled wakeup primitive that lets the current turn end cleanly and wakes the session after the computed interval (e.g., built-in session timer/cron tools, `/loop`, `ScheduleWakeup`, `CronCreate`, self-reminder timers, Automations, or an active scheduler plugin).

A Worker runtime **fails** the timer gate if:
- No scheduled wakeup/timer tool exists in the active runtime (e.g., vanilla OpenCode without a scheduler plugin, or a non-interactive headless run where timers are disabled);
- Waiting would require holding open a foreground shell `sleep` loop. **Never fake scheduled timers with blocking shell `sleep`.**

When the gate fails, immediately send `NO_TIMER_CAPABILITY` to Commander and stop.

---

## 3. Exact Wakeup Formula (`cexp-worker wakeup`)

Always compute the wakeup period via:

```bash
<cexperiment-worker-skill-dir>/scripts/cexp-worker wakeup <rem_mins_job1> [<rem_mins_job2> ...]
```

| Concurrent Active Jobs ($N_{\text{active}}$) | Estimated Remaining Time ($T_{\text{rem}} = 5X$) | $X = T_{\text{rem}} / 5$ | Scheduled Wakeup Period |
| --- | --- | --- | --- |
| `0` (queue done or none runnable) | — | — | **`STOP_ALL_TIMERS` (`0 min`)** |
| `1` | `40 min` | `8 min` (`< 15`) | **`15 min`** (`900 s`) |
| `1` | `110 min` | `22 min` (`15 <= X <= 30`) | **`22 min`** (`1320 s`) |
| `1` | `250 min` | `50 min` (`> 30`) | **`30 min`** (`1800 s`) |
| `>= 2` (2, 3, 4+ concurrent jobs) | any | — | **`30 min`** (`1800 s`, fixed) |

### Timer Cancellation & Re-Measurement Rules
1. **Healthy tick (nothing finished, nothing crashed, no newly fillable GPU slot)**: leave a recurring timer untouched if its period still matches, or re-arm the next one-shot timer for the computed period.
2. **Any job crashes, hangs, or hits a bug**: **cancel the current timer immediately**, repair the implementation bug, relaunch detached, **re-measure step rate from scratch**, recompute `cexp-worker wakeup`, and start a fresh timer.
3. **Any job finishes**: **cancel the current timer immediately**, finalize `results/<ID>.md` + `cexp-worker add-result`, pack any newly `READY` specs onto free GPUs, **recompute `cexp-worker wakeup` from scratch** for the updated active set, and arm a fresh timer (or cancel all self-timers and wake Commander when `active_jobs == 0` and no `READY` specs remain).

---

## 4. Out-of-Order & Parallel Multi-GPU Bin-Packing

1. **Discover Ready Specs**:
   ```bash
   <cexperiment-worker-skill-dir>/scripts/cexp-worker pending <queue-file> --only-ready
   ```
   Every `READY` item has `Depends` already satisfied and can start **out of order** and **in parallel** with other `READY` items!

2. **Probe Live Server Headroom**:
   ```bash
   nvidia-smi --query-gpu=index,name,memory.free,memory.total,utilization.gpu --format=csv,noheader,nounits
   free -g && df -BG <work_dir>
   ```
   Check `nvidia-smi topo -m` when packing multi-GPU jobs (`2gpu:...`).

3. **Greedy Bin-Packing Rules**:
   - Pack multi-GPU / tier-1 `READY` specs onto clean NVLink pairs first, then fill every remaining qualifying GPU with single-GPU `READY` specs concurrently.
   - Honor four constraints simultaneously:
     1. Per-GPU free VRAM >= spec's `min_vram_gib_per_gpu` (+ safety margin);
     2. Specs marked `excl` (`exclusive_gpu_timing: true`) never share physical GPUs with another active job in this campaign;
     3. Sum of `host_ram_budget_gib` across active jobs <= Contract Host RAM ceiling;
     4. Sum of `disk_peak_gib` across active jobs <= Contract Active Disk ceiling.

4. **Detached Execution & `artifacts/active_runs.json` Ledger**:
   Launch jobs detached (`nohup` / `setsid` / `tmux` writing `.pid`, `.log`, and `.exit` files) and persist active PIDs, assigned GPU indices, estimated remaining minutes, and current `timer_id` in `artifacts/active_runs.json`.
