# Worker Dispatch Templates & Post-Run Triage Rubric

## 1. Initial / Appended Queue Dispatch Prompt (Commander → Worker)

Send the following platform-agnostic message to the user-supplied Worker address/handle via the runtime's cross-session messaging, queue, or agent-dispatch mechanism:

```text
Use the `cexperiment-worker` skill to execute the pending experiment queue.

- Commander return address (wake me here when blocked or when all queued specs are complete): <COMMANDER_RETURN_ADDRESS>
- Queue file (append-only pointer table): <ABSOLUTE_QUEUE_PATH>
- Shared contract: <ABSOLUTE_CONTRACT_PATH>
- Compute target / server: <SSH_HOST_OR_LOCAL> (work dir: <REMOTE_WORK_DIR>)

Execution reminders:
1. First verify that your runtime supports scheduled timers / wakeups per `cexperiment-worker`. If your runtime lacks timer capability, do NOT launch any experiments—immediately message me back with `NO_TIMER_CAPABILITY` and stop.
2. Inspect all pending `spec` rows (those without a paired `result` row) and pack ready specs out-of-order and in parallel across all available server GPUs within the shared contract's VRAM, host RAM, disk, and timing-isolation bounds.
3. Supervise active runs strictly using `cexperiment-worker`'s adaptive wakeup formula (never leave server jobs unattended, and cancel all of your own timers once the queue is completely finished before waking me).
4. Do not alter research hypotheses or invent new scope—if a spec has a blocking ambiguity or contradiction, document the state in `results/<ID>.md`, send `SPEC_QUESTION <ID>: <summary + path>` to my return address, and continue any independent specs.
```

> **Important**: Never include Commander's optional monitoring period (e.g., `3h` or `1h`) in the Worker prompt. Worker's wakeup period is always computed solely from `cexperiment-worker`'s `5*X` / `30m` formula.

---

## 2. Mid-Run Question Reply Template (Commander → Worker)

When Worker sends `SPEC_QUESTION <ID>: ...`:

```text
Continue under `cexperiment-worker`.

Resolution for <ID>:
- <Direct ruling on the ambiguity or updated pointer>
- Queue file: <ABSOLUTE_QUEUE_PATH> (updated spec / appended follow-up if applicable)

Resume <ID> (and any unblocked dependents), re-compute your scheduled wakeup period per `cexperiment-worker`, and wake me at <COMMANDER_RETURN_ADDRESS> when the queue is complete or if another blocker arises.
```

---

## 3. Timer-Capability Edge Cases

| Situation | Required Commander Action |
| --- | --- |
| Worker replies `NO_TIMER_CAPABILITY` | Cancel Commander's own monitoring timer (if set for this batch). Stop orchestration immediately. Tell the user: **"当前 Worker Agent 不支持定时唤醒功能，请更换支持定时任务的 Worker Agent 后端。"** (or matching user language). Do not attempt sleep loops or manual workarounds. |
| User asked Commander to monitor periodically in this turn, and Commander supports timers | Arm a recurring schedule on Commander (`3h` by default, or the explicit custom interval stated in this turn). Cancel it as soon as this batch finishes or halts. |
| User asked Commander to monitor periodically in this turn, and Commander does **not** support timers | **Silently ignore** the monitoring request. Dispatch Worker normally and wait for Worker's callback. |
| User did **not** ask Commander to monitor in **this** `cexp` invocation (even if enabled in an earlier batch) | Do **not** arm any Commander monitoring timer. Monitoring is strictly a one-time per-batch flag. |

---

## 4. Three-Way Convergence & Triage Rubric (Upon `QUEUE_COMPLETE`)

When Worker finishes the queue (having already canceled all of its own timers) and wakes Commander, evaluate every new `results/<ID>.md` against its `specs/<ID>-*.md` using this strict 3-bucket classification:

### Bucket 1: Strictly In-Scope Error or Missing Control → **Solve & Re-Verify Autonomously**

Qualifies **only** when the validity of the **currently scoped** experiment is compromised:
- Code/measurement bug (wrong tensor slice/offset, corrupted cache read, broken metric calculation, silent OOM/truncation, unaligned batch size or seed parity).
- Co-scheduling contamination (a latency benchmark ran while another process dirtied the same GPU or exceeded host RAM swap).
- An obvious missing control **strictly required to establish the current spec's claim** (e.g., evaluating throughput speedup without verifying output correctness, or omitting the direct baseline arm specified by the question).

**Commander Action**:
1. Diagnose root cause and formulate the exact fix / missing control.
2. Write new `specs/<NEXT_ID>-*.md` file(s) and append new `spec` pointer(s) to `<ABSOLUTE_QUEUE_PATH>` via `scripts/queue_ops.py append-spec` (never overwrite completed rows).
3. Wake `<worker_address>` using the dispatch template in Section 1 to verify at lab grade.

### Bucket 2: Out-of-Scope New Ideas, Micro-Improvements, or Next-Phase Follow-Ups → **Hand Back to User**

Qualifies when the current batch's experiments are valid, and inspecting the results sparks:
- New research hypotheses or next-stage architectural changes outside this batch's scope;
- Micro-optimizations, hyperparameter polish, or minor corner-case tweaks ("边边角角 / micro-improvements").

**Commander Action**:
1. **Do NOT append new specs or wake the Worker.**
2. Cancel Commander's monitoring timer (if active). Confirm Worker is already stopped.
3. Present to the user:
   - Concise, evidence-backed summary of completed batch results (with links to `WORKER-QUEUE.md`, `specs/`, and `results/`);
   - A clearly separated **"Proposed Next Steps / Out-of-Scope Ideas for Your Decision"** section so the user decides what to run next.

### Bucket 3: Clean & Sufficient Completion → **Stop Cleanly (No Nitpicking)**

Qualifies when all queued questions are answered reliably within reasonable experimental depth, whether results are positive or negative.

**Commander Action**:
1. Do **not** invent extra ablations or nitpick minor noise just to prolong the campaign.
2. Cancel Commander's monitoring timer (if active). Confirm Worker is stopped (if Worker was paused awaiting instructions rather than already self-stopped on empty queue, send a brief stand-down message).
3. Report final synthesized conclusions and result file links to the user.
