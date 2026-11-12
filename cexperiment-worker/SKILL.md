---
name: cexperiment-worker
description: "Execute cexp_queue v1 experiment specs autonomously on GPU servers with out-of-order multi-GPU parallel packing and adaptive scheduled wakeups via the bundled cexp-worker binary. Invoked by a Worker agent when dispatched by a Commander agent running cexp—never directly by end users. Enforces timer capability verification, the 5X / 30-minute wakeup rule, performance trace capture for recursive-improve rounds, flock-safe result registration, and self-termination of timers."
---

# cexperiment-worker

Act as the autonomous **Worker** agent dispatched by a **Commander** agent (`cexp`).

- **Dispatched by Commander, never called directly by end users**: receive `<COMMANDER_RETURN_ADDRESS>`, `<ABSOLUTE_QUEUE_PATH>`, `<ABSOLUTE_CONTRACT_PATH>`, and target compute host from Commander.
- **Strict execution scope**: turn each `spec` pointer into reproducible code, clean GPU runs, verified metrics (plus bottleneck traces in `recursive-improve` rounds), and a complete `results/<ID>.md` report, then register its `result` pointer. **Never invent research hypotheses or silently alter spec criteria.**
- **Self-contained CLI (never install to `PATH`)**: use `<skill-dir>/scripts/cexp-worker` directly from the skill folder (run `<skill-dir>/scripts/cexp-worker --help`). Never copy it into global `PATH` or mutate machine environment config.

Load supporting files as needed:

- Timer gate, `5*X` / `30m` wakeup calculations, and multi-GPU bin-packing rules → `references/scheduler-and-gpu-packing.md`
- Result registration, `recursive-improve` trace requirements, and callback templates → `references/result-and-handoff-protocol.md`
- Result report starter template → `assets/result-template.md`
- Bundled native Worker CLI (Linux & macOS) → `scripts/cexp-worker`

---

## Step 0: Mandatory Timer-Capability Gate (Run First!)

Before writing code or starting any server job, check whether your agent runtime exposes a real **scheduled wakeup / timer / cron / loop** tool (never substitute blocking shell `sleep` loops).

- **If unsupported**: do **not** start any server experiments; immediately send `NO_TIMER_CAPABILITY` (see `references/result-and-handoff-protocol.md`) to `<COMMANDER_RETURN_ADDRESS>` and **stop**.
- **If supported**: proceed to Step 1.

---

## Step 1: Discover Ready Specs & Pack All Available GPUs

1. Inspect the queue header and uncompleted specs via the bundled binary:
   ```bash
   <skill-dir>/scripts/cexp-worker head <queue-file>
   <skill-dir>/scripts/cexp-worker pending <queue-file>
   ```
2. Read `specs/00-contract.md` and every `[READY]` spec file listed by `pending`.
3. Probe live server GPUs (`nvidia-smi`), NVLink topology, host RAM (`free -g`), and disk (`df -BG`). Never kill other users' processes.
4. **Pack out-of-order and in parallel** (following `references/scheduler-and-gpu-packing.md`):
   - Launch **all** `[READY]` specs that fit concurrently across free GPUs within per-GPU VRAM, `excl` benchmark timing isolation, and combined host RAM/disk ceilings—never serialize blindly by table order!
   - Overlap CPU coding, unit tests, and data prep while GPUs execute active runs.
   - Start every server job detached (`nohup` / `setsid` / `tmux` with `.pid`, `.log`, and `.exit` files), verify startup health, and record active jobs in `artifacts/active_runs.json`.

---

## Step 2: Arm the Scheduled Wakeup Timer (`5*X` / `30m` Rule)

Worker's wakeup cadence is governed **exclusively** by the invariant formula in `cexp-worker wakeup` (never by any monitoring interval set on Commander):

```bash
<skill-dir>/scripts/cexp-worker wakeup <rem_mins_1> [<rem_mins_2> ...]
```

- **`active_jobs >= 2` (2, 3, 4+ concurrent jobs)**: fixed **30 minutes (`1800 s`)**.
- **`active_jobs == 1`**: estimate remaining minutes $T_{\text{rem}} = 5X \implies X = T_{\text{rem}} / 5$, clamped to **$[15, 30]$ minutes** (`<15m -> 15m`, `>30m -> 30m`, otherwise `round(X)`).
- Arm the scheduled timer, save `timer_id` in `artifacts/active_runs.json`, and end the turn cleanly.

---

## Step 3: Wakeup Inspection & Timer State Machine

On every scheduled wakeup, audit every active run in `artifacts/active_runs.json`:

1. **All jobs healthy & no free GPU slot fits another `[READY]` spec**:
   - Keep the recurring timer active (or re-arm the next one-shot wakeup for the computed period) and sleep.
2. **Crash, hang, OOM, or bug detected**:
   - **Cancel the current scheduled timer immediately.**
   - If it is an implementation/runtime bug within spec scope: clean up wedged processes/corrupt outputs, fix and smoke-test, relaunch detached, **re-measure remaining time from scratch**, run `cexp-worker wakeup`, and arm a **fresh** timer.
   - If it is an unresolvable spec contradiction/ambiguity: write diagnostics to `results/<ID>.md`, register `--status blocked` via `cexp-worker add-result`, send `SPEC_QUESTION <ID>` to `<COMMANDER_RETURN_ADDRESS>`, and keep running any independent `[READY]` specs (or clear all timers and pause if `active_jobs == 0`).
3. **One or more jobs finished**:
   - **Cancel the current scheduled timer immediately.**
   - Verify validity checks, delete bulky temporary shards/checkpoints per contract, write `results/<ID>.md` from `assets/result-template.md` (including Section 3 performance traces whenever the Round mode is `recursive-improve`!), and register the pointer:
     ```bash
     <skill-dir>/scripts/cexp-worker add-result <queue-file> --id <ID> --path "results/<ID>.md" --status <complete|negative>
     ```
   - Re-run `<skill-dir>/scripts/cexp-worker pending <queue-file>`:
     - If `[READY]` specs can now launch (including newly unblocked dependents!) or other concurrent jobs remain active: pack free GPUs, **recompute `cexp-worker wakeup` from scratch**, arm a fresh timer, and sleep.
     - If zero specs remain runnable and `active_jobs == 0`: proceed to Step 4.

---

## Step 4: Self-Aware Timer Teardown & Commander Handoff

Once `pending` shows `ready=0, waiting_on_deps=0` and `active_jobs == 0`:

1. **Cancel and delete ALL scheduled timers / cron tasks owned by this Worker** (`cexp-worker wakeup` with 0 args → `STOP_ALL_TIMERS`). Never wait for Commander to stop your timers.
2. Confirm zero stray campaign GPU processes and clean post-run disk usage.
3. Send `QUEUE_COMPLETE` (see `references/result-and-handoff-protocol.md`) to `<COMMANDER_RETURN_ADDRESS>`, then end the turn and stay idle.
