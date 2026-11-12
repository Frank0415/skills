---
name: cexperiment-worker
description: "Execute pointer-queued research and systems specs autonomously on GPU servers with parallel out-of-order GPU packing and adaptive scheduled wakeups. Invoked by a Worker agent when dispatched by a Commander agent running cexp—never directly by end users. Enforces timer capability verification, the 5X / 30-minute wakeup formula, bug recovery, atomic result-pointer appending, and self-termination of timers on completion."
---

# cexperiment-worker

Act as the autonomous **Worker** agent paired with a **Commander** agent (`cexp`).

- **Invoked by Commander, not by the end user**: receive the Queue file path, Shared Contract path, Compute host, and Commander's return address from the Commander's dispatch message.
- **Strict execution boundary**: turn each `spec` pointer into reproducible code, clean server runs, verified metrics, and a complete `results/<ID>.md` report, then append a paired `result` pointer to the Queue file. **Never invent new research directions, silently relax acceptance criteria, or drift from the spec.**
- **Never launch-and-abandon**: while any server experiment is running, maintain an active scheduled wakeup timer until the entire queue finishes or blocks.
- **Platform-agnostic**: use whatever native scheduled timer/cron/wakeup primitive and cross-session messaging mechanism the current runtime provides. Never hardcode assumptions specific to a single UI harness.

Load supporting files as needed:

- Timer primitives, worked `5*X` / `30m` wakeup calculations, and multi-GPU bin-packing rules → `references/scheduler-and-gpu-packing.md`
- `results/<ID>.md` structure and Commander callback message payloads → `references/result-and-handoff-protocol.md`
- Exact wakeup calculator, pending-queue scanner, and `flock`-safe result-pointer appender → `scripts/worker_ctl.py`

---

## Step 0: Mandatory Timer-Capability Gate (Run First!)

Before writing code or starting any server process, check whether your agent runtime exposes a real **scheduled wakeup / timer / cron / loop** tool that wakes this Worker session after ending a turn (do **not** substitute blocking shell `sleep` loops inside a command).

- **If your runtime does NOT support scheduled timers/wakeups**:
  1. **Do NOT launch any experiments.**
  2. Immediately send a callback message to Commander's return address starting with `NO_TIMER_CAPABILITY` (see `references/result-and-handoff-protocol.md`).
  3. **Stop immediately.**
- **If your runtime supports scheduled timers/wakeups**: proceed to Step 1.

---

## Step 1: Read Queue, Contract & Live Server Headroom

1. Run `python3 <skill-dir>/scripts/worker_ctl.py pending <queue-file>` and read `specs/00-contract.md` plus every pending `specs/<ID>-*.md` (every `spec` row without a paired `result` row).
2. Probe live server resources on the target host:
   - Per-GPU free VRAM, utilization, and NVLink topology (`nvidia-smi`);
   - Host RAM headroom (`free -g`, `/dev/shm`, cgroup memory limits if containerized);
   - Disk free space (`df -h`) and existing active processes from `artifacts/active_runs.json` (if resuming on wakeup).
3. Respect shared-server rules: never kill processes belonging to other users; strictly honor host RAM and disk ceilings in `00-contract.md`.

---

## Step 2: Maximize GPU Utilization via Out-of-Order & Parallel Packing

Do **not** execute the pointer queue one-by-one in table order when spare GPUs and budgets exist. Read `references/scheduler-and-gpu-packing.md` and schedule greedily:

1. Filter pending specs to **ready specs**: those whose `depends_on` list is `none` or whose prerequisite IDs already have `result` pointers in the queue.
2. Pack as many ready specs **concurrently and out-of-order** across free GPUs as fit simultaneously within:
   - Each spec's `gpu_count` and `min_vram_gib_per_gpu`;
   - Combined `host_ram_budget_gib` and `disk_peak_gib` ceilings across all concurrently active runs;
   - **Benchmark isolation**: any spec with `exclusive_gpu_timing: true` gets dedicated physical GPU(s) unshared by other jobs in this campaign.
3. Overlap CPU-bound coding, unit tests, and data validation for upcoming specs while GPUs execute active jobs.
4. Launch every server job **detached** (e.g., `nohup` / `setsid` / `tmux` wrapper writing PID, start timestamp, structured log path, and exit-status sentinel file) and record all active runs in `artifacts/active_runs.json`. Confirm each newly launched process survives initial startup/warmup without immediate syntax or import crashes before going to sleep.

---

## Step 3: Compute & Arm the Scheduled Wakeup Timer

Worker's wakeup interval is governed **exclusively** by the formula below—never by any monitoring interval set on the Commander:

- **When `active_jobs >= 2` (2, 3, 4, or more experiments running concurrently)**:
  $$\text{wakeup\_period} = 30\text{ minutes}$$
- **When `active_jobs == 1` (single experiment running)**:
  Estimate remaining runtime to completion as $T_{\text{rem}} = 5X$ minutes, so $X = T_{\text{rem}} / 5$:
  - If $15 \le X \le 30$: $\text{wakeup\_period} = X$ minutes (rounded to nearest integer minute if required by scheduler).
  - If $X < 15$: $\text{wakeup\_period} = 15$ minutes.
  - If $X > 30$: $\text{wakeup\_period} = 30$ minutes.
  - Equivalently: $\text{wakeup\_period} = \min(30, \max(15, T_{\text{rem}} / 5))$ minutes.

Use `python3 <skill-dir>/scripts/worker_ctl.py calc-wakeup --remaining-mins <m1> [<m2> ...]` to compute the exact period without arithmetic mistakes. Arm the runtime's scheduled wakeup/cron timer for that period, record the timer ID in `artifacts/active_runs.json`, and end your turn cleanly to sleep.

---

## Step 4: Wakeup Inspection & Timer State Machine

Every time the scheduled timer wakes you up, inspect process liveness, GPU utilization, host RAM/disk watermarks, and tail logs for every job in `artifacts/active_runs.json`. Execute the matching state transition:

### Case A — All Active Jobs Healthy & No Free Slot Fits Another Ready Spec
- Keep the current scheduled timer active (if using recurring cron and the period is unchanged) or re-arm the next one-shot timer for the computed period, then end the turn cleanly.

### Case B — Bug, Crash, Hang, OOM, or Spec Blocker Detected on Any Job
1. **Immediately cancel/stop the current scheduled timer.**
2. Classify the failure:
   - **Implementation / environment / runtime bug within spec scope**:
     - Kill any wedged child processes and clean corrupted partial outputs.
     - Fix the bug, verify with a quick smoke check, and relaunch the experiment detached.
     - **Re-estimate remaining runtime from scratch**, recompute the wakeup period via Step 3 (`30m` if `>= 2` active jobs; `clamp(T_rem/5, 15, 30)` if `1` active job), arm a **fresh** scheduled timer, and sleep.
   - **Unresolvable spec ambiguity or contradiction**:
     - Write full diagnostics and the exact question into `results/<ID>.md` (`status: blocked`).
     - Send `SPEC_QUESTION <ID>: <concise question + result path>` to Commander's return address.
     - Continue running/launching any independent ready specs: if `active_jobs >= 1` remain, recompute and arm a fresh timer; if `active_jobs == 0` can run while awaiting Commander, ensure **all** self-timers remain canceled and stop cleanly until Commander replies.

### Case C — One or More Jobs Finished
1. **Immediately cancel/stop the current scheduled timer.**
2. For each finished job `<ID>`:
   - Verify correctness invariants, metrics completeness, and integrity checks required by `specs/<ID>-*.md`.
   - Delete bulky temporary shards/checkpoints per `00-contract.md` and record post-cleanup disk usage.
   - Write the complete report to `results/<ID>.md` (following `references/result-and-handoff-protocol.md`).
   - Atomically append the `result` pointer row to the Queue file via:
     `python3 <skill-dir>/scripts/worker_ctl.py append-result <queue-file> --id <ID> --label "<ID> Result" --path "results/<ID>.md"`
3. Re-evaluate the Queue file (`worker_ctl.py pending <queue-file>`):
   - **If pending ready specs exist or other concurrent jobs are still running**:
     - Pack and launch as many newly ready/fittable specs as current GPU/RAM/disk headroom allows (Step 2).
     - **Recompute the wakeup period from scratch** for the new active job set (Step 3), start a fresh scheduled timer, and sleep.
   - **If NO pending specs remain and `active_jobs == 0` (Queue 100% Complete)**:
     - Proceed immediately to Step 5.

---

## Step 5: Self-Aware Timer Teardown & Commander Wakeup

When all specs in the Queue file have paired `result` pointers (or remaining items are explicitly waiting on Commander with zero runnable jobs) and `active_jobs == 0`:

1. **Cancel and clear ALL of your own scheduled timers / cron tasks**. Verify zero self-timers remain active—do **not** rely on Commander to stop your timers.
2. Verify zero stray campaign GPU processes and confirm temporary disk artifacts are cleaned up.
3. Send `QUEUE_COMPLETE: <queue-path + per-ID status summary>` (see `references/result-and-handoff-protocol.md`) to Commander's return address.
4. End your turn and stay idle.
