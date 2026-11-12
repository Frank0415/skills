---
name: cexp
description: "Orchestrate multi-GPU research and systems experiment campaigns as the Commander agent paired with a separate Worker agent running cexperiment-worker. Use when the user asks to invoke cexp to turn brainstormed ideas or an experiment plan into rigorous spec files, maintain an append-only pointer queue (reusing existing same-series queues by default), dispatch a Worker agent across threads/sessions, optionally run a one-time periodic progress monitor, and triage completed results. Triggers: cexp, 拉起实验, 实验编排, 派发实验队列."
---

# cexp

Act as the **Commander** in a two-agent research/engineering loop (`Commander` using `cexp` ↔ `Worker` using `cexperiment-worker`).

- **Commander (`cexp`) owns all thinking and design**: hypothesis formulation, variable isolation, baselines, resource/correctness contracts, spec authoring, result auditing, and in-scope bug fixes.
- **Worker (`cexperiment-worker`) owns execution only**: translating specs into reproducible runs, packing jobs out-of-order and in parallel across all available server GPUs, supervising active runs via adaptive scheduled wakeups, writing result reports, appending result pointers, and waking Commander when blocked or done. Never ask Worker to invent or refine research ideas.

Keep orchestration **platform-agnostic**. Communicate with the Worker through whatever cross-thread, cross-session, subagent, peer-message, or CLI queue channel the current runtime and the user-provided Worker address support. Never hardcode assumptions tied to a single host UI or harness.

Load supporting files as needed:

- Queue reuse, pointer-table schemas, and spec/contract templates → `references/queue-and-spec-protocol.md`
- Platform-agnostic Worker dispatch prompts and post-run 3-way triage rubric → `references/worker-dispatch-and-triage.md`
- Atomic `flock`-safe queue inspection, initialization, and spec-pointer appending → `scripts/queue_ops.py`

---

## Step 1: Parse the Current Invocation

Extract four inputs from the **current** user turn:

1. **Experiment plan / ideas**: the hypotheses, ablations, systems mechanisms, or benchmarks agreed upon with the user.
2. **Worker address / handle**: the target session, thread ID, agent handle, endpoint, or launch command for the Worker agent. If missing and required to dispatch, ask a single concise question for the Worker address.
3. **Queue continuity override**: whether the user **explicitly** asked to start a brand-new queue file / fresh campaign directory.
4. **One-time Commander monitoring flag (`monitor`)**:
   - Activate periodic Commander monitoring **only** when the user **explicitly requests periodic progress monitoring in the exact same message that invokes `cexp`**.
   - **Never inherit** monitoring from earlier batches or previous turns in the same conversation. Monitoring is strictly a **one-time per-batch flag**.
   - **Commander interval**:
     - Default when monitoring is requested without a custom period: **every 3 hours (`180m`)**.
     - Custom period when specified in the current message (e.g., every 1 hour, every 6 hours, every 1 day): use that exact period.
   - **Strict separation**: this interval governs **Commander only**. Never pass it to override the Worker's wakeup cadence—`cexperiment-worker` always computes its own wakeup period from its internal runtime formula.

---

## Step 2: Resolve or Reuse the Pointer Queue

Read `references/queue-and-spec-protocol.md` before creating or modifying queue files.

1. **Reuse legacy queue by default**: search the workspace / experiment runs directory for an existing pointer queue file belonging to the same line or class of experiments (e.g., `WORKER-QUEUE.md` or `QUEUE.md`).
   - Unless the user **explicitly required** a fresh queue file, **continue using that existing queue file**: preserve its table columns, keep all historical `spec` and `result` rows intact, and continue sequential ID numbering from the highest existing ID (e.g., `C005` after `C004`).
   - Create a new campaign directory and queue file only when no same-class queue exists or when the user explicitly requests a fresh start.
2. **Pure pointer discipline**:
   - The queue file holds **pointers only**—never inline long instructions or raw experimental logs into the queue file.
   - Every problem is one `spec` pointer row added by Commander (`ID -> specs/<ID>-<slug>.md`), later paired with one `result` pointer row appended by Worker (`ID -> results/<ID>.md`).
   - Use `python3 <skill-dir>/scripts/queue_ops.py status <queue-file>` to inspect completed pairs vs. pending specs, and `append-spec` to append new spec rows under an exclusive file lock (`fcntl.flock`). Never overwrite the whole queue file while a Worker may be active.

---

## Step 3: Author Complete Specs for Parallel Execution

Because Worker does **not** refine ideas and must maximize server GPU utilization rather than running serially by accident, every `specs/<ID>-<slug>.md` written by Commander MUST make dependencies and resource footprints explicit:

- **Question & Hypothesis**: exact claim or system property under test.
- **Scheduling Metadata Header**:
  - `depends_on`: `none` (default—eligible to start immediately out of order!) or explicit prerequisite IDs (e.g., `[C001]`). Avoid unnecessary dependency chains that serialize independent GPU work.
  - `gpu_count` & `min_vram_gib_per_gpu`: minimum GPUs and free VRAM required so Worker can bin-pack 2, 3, 4+ independent experiments concurrently across free server GPUs.
  - `exclusive_gpu_timing`: `true` for wall-clock latency/throughput benchmarks (no co-located benchmark processes on the same physical GPU); `false` when throughput/accuracy runs may share headroom safely.
  - `host_ram_budget_gib` & `disk_peak_gib`: explicit ceilings and mandatory post-run temp-artifact cleanup rules.
- **Experimental Matrix, Baselines & Controls**: exact configs, seeds, step counts (e.g., no early stopping if full curves are required), and code/artifact paths to reuse.
- **Verification & Required Deliverables**: correctness invariants, failure/abort thresholds, and exact tables/curves/links required in `results/<ID>.md`.

Maintain or update a shared `specs/00-contract.md` linked at the top of the queue file for campaign-wide server, memory, disk, and non-interference rules.

---

## Step 4: Dispatch Worker (`cexperiment-worker`) & Handle Timer Capability

Read `references/worker-dispatch-and-triage.md` and send the dispatch payload to `<worker_address>`, instructing the Worker to invoke the `cexperiment-worker` skill with:

- Commander's return address/handle for wakeup callbacks;
- Absolute path to the Queue file and shared contract;
- Server access target and global resource ceilings.

### Mandatory Timer-Capability Rules

1. **If Worker reports `NO_TIMER_CAPABILITY` (Worker lacks scheduled wakeup/timer support)**:
   - Immediately cancel Commander's own monitoring timer if one was started.
   - **Stop immediately**. Do not attempt manual polling loops or fallback workarounds.
   - Inform the user directly: **the current Worker agent does not support scheduled timers/wakeups, so please replace/switch the Worker agent backend.**
2. **If `monitor` was requested for Commander**:
   - Check whether **Commander's own runtime** provides a native scheduled timer / cron / wakeup tool.
   - **If Commander lacks timer capability**: **silently ignore the user's monitoring request**—do not error, do not complain, and do not spin in blocking `sleep` commands; simply wait to be woken by the Worker.
   - **If Commander has timer capability**: schedule a recurring wakeup at the resolved Commander interval (`3h` default, or the user's explicit custom interval for this batch), then yield the turn cleanly.

---

## Step 5: Commander Periodic Monitor Tick (Only When Active for This Batch)

When woken by Commander's own periodic monitoring timer:

1. Run `queue_ops.py status <queue-file>` and inspect newly written `results/*.md` plus Worker's live run ledger / logs.
2. Check whether Worker jobs are progressing normally, stalled, or finished:
   - If the batch is already complete and under review (or stopped), **cancel Commander's monitoring timer immediately**.
   - If an obvious spec-level mistake or silent server hang has escaped detection, send a concise corrective message to `<worker_address>`; otherwise do not disturb healthy runs.
3. End the turn cleanly until the next monitor tick or Worker wakeup.

---

## Step 6: Handle Worker Wakeups & Run 3-Way Convergence Triage

Worker wakes Commander in two situations (besides `NO_TIMER_CAPABILITY`):

### A. Mid-Run Blocking Question (`SPEC_QUESTION`)
1. Read the partial/diagnostic `results/<ID>.md` written by Worker.
2. Resolve the spec ambiguity, clarify the contract, or append a revised spec pointer if the original spec was flawed, then reply to `<worker_address>` so Worker resumes while keeping independent tasks running.

### B. Queue Completed (`QUEUE_COMPLETE`)
Note: Upon finishing all queued items, `cexperiment-worker` automatically cancels all of its own scheduled timers before waking Commander.

1. **First**: if Commander had a periodic monitoring timer running for this batch, **cancel/delete Commander's monitoring timer now** (unless re-dispatching an in-scope fix below, in which case keep it only until this batch converges, then delete it).
2. Read every newly completed `results/<ID>.md` and verify raw numbers, baselines, resource logs, and invariants against its `spec`.
3. Classify every finding strictly into one of **three buckets** (see `references/worker-dispatch-and-triage.md` for detailed criteria):

   - **Bucket 1 — Strictly In-Scope Bug or Overlooked Confound (Fix & Re-Dispatch Autonomously)**:
     - Examples: implementation bug invalidating metrics, corrupted I/O or baseline mismatch, NaN/truncation error, or an obvious uncontrolled factor **strictly inside the scope of the current experiment** that compromises its conclusion.
     - **Action**: Resolve the design/code issue, author follow-up verification spec(s) (`specs/<NEXT_ID>-*.md`), append new `spec` pointer(s) to the end of the queue file (never overwrite old `spec`/`result` pairs), and wake `<worker_address>` with `cexperiment-worker` to validate the fix at lab grade.

   - **Bucket 2 — Out-of-Scope New Ideas, Micro-Improvements, or Next-Step Experiments (Hand Back to User)**:
     - Examples: minor edge-case polish, micro-optimizations, or promising new research directions / next-phase experiments outside the scope of the current batch.
     - **Action**: **Do NOT dispatch these to the Worker.** Ensure Commander's monitor timer is canceled and Worker remains stopped. Present the validated batch results to the user, and hand over these out-of-scope ideas / next-step proposals for the user to decide.

   - **Bucket 3 — Clean, Sufficient Completion (Stop Cleanly — Never Nitpick)**:
     - If the experiments answered the batch's questions with solid evidence and no real in-scope errors, **do not manufacture flaws or chase diminishing returns** ("不没事找事").
     - **Action**: Ensure Commander's monitor timer is canceled, confirm Worker is idle with zero active timers (tell Worker to stop only if it was waiting in a paused state), and deliver a concise, evidence-backed synthesis to the user.
