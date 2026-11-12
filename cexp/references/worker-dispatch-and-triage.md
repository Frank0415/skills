# Worker Dispatch Templates, One-Time Flags & Convergence Modes

## 1. Two Strict One-Time Per-Invocation Flags (Commander Only)

Both flags below are **strictly non-sticky**—they activate **only** when explicitly requested in the **same user message** that invokes `cexp`, and **never** carry over to future rounds or later invocations:

| Flag | Trigger Condition (Current Message Only) | Default When Omitted | Behavior |
| --- | --- | --- | --- |
| **`monitor`** | User explicitly asks Commander to periodically check/monitor progress (default `3h`, or user-specified interval like `1h`, `6h`, `1d`). | `off` | Commander arms its own recurring wakeup timer (or silently ignores if Commander runtime lacks timer support) and cancels it as soon as the round finishes/stops. **Never alters Worker's `5*X`/`30m` wakeup timer.** |
| **`recursively_improve`** | User explicitly asks to `recursively improve` / `循环优化` / continuously hunt micro-improvements along a target direction until no visible gain remains. | `off` (`standard` mode) | Round is created with `--mode recursive-improve`. Commander and Worker iterate back-and-forth using performance traces until no meaningful performance gain remains on the table, then stop cleanly. |

---

## 2. Worker Dispatch Prompt (Commander → Worker, Platform-Agnostic)

Send this payload to `<WORKER_ADDRESS>` via whatever cross-session messaging, queue, or agent-dispatch channel the host runtime provides:

```text
Use the `cexperiment-worker` skill to execute the pending experiment queue.

- Commander return address (wake me here when blocked or when all pending specs finish): <COMMANDER_RETURN_ADDRESS>
- Queue document (`cexp_queue: 1`): <ABSOLUTE_QUEUE_PATH>
- Active Round: <ROUND_ID> (mode: <standard | recursive-improve>)
- Shared contract: <ABSOLUTE_CONTRACT_PATH>
- Compute target / server: <SSH_HOST_OR_LOCAL> (work dir: <REMOTE_WORK_DIR>)

Execution reminders:
1. Run the Step 0 timer gate first: if your runtime lacks scheduled wakeup / cron / timer support, do NOT start any server jobs—immediately send `NO_TIMER_CAPABILITY` to my return address and stop.
2. Use `<cexperiment-worker-skill-dir>/scripts/cexp-worker pending <ABSOLUTE_QUEUE_PATH>` to find all READY specs and pack them out-of-order and in parallel across all available server GPUs within contract bounds.
3. Supervise active runs strictly via `cexp-worker wakeup` (`clamp(T_rem/5, 15, 30)` min for 1 job; `30` min for >=2 concurrent jobs). When `active_jobs == 0` and no pending specs remain, cancel ALL of your own timers before waking me with `QUEUE_COMPLETE`.
4. If active round mode is `recursive-improve`, include fine-grained performance traces / bottleneck breakdowns in each `results/<ID>.md` as specified so I can evaluate remaining headroom.
```

---

## 3. Timer-Capability Edge Cases

| Case | Commander Action |
| --- | --- |
| Worker replies `NO_TIMER_CAPABILITY` | Cancel Commander's own monitoring timer (if active). Stop immediately. Tell the user directly: **"当前 Worker Agent 不支持定时唤醒功能，请更换 Worker Agent 后端。"** |
| `monitor` requested & Commander lacks timer support | **Silently ignore** the monitoring request and proceed normally; wait for Worker's callback. |

---

## 4. Post-Run Review & Convergence Upon `QUEUE_COMPLETE`

Inspect all newly completed `results/<ID>.md` files for the current round (`cexpctl status <queue-file> --round <ROUND_ID>`). Apply the convergence rule matching **this round's mode**:

### Mode A: `standard` Mode (Default When `recursively_improve` Was NOT Requested)

Classify findings strictly into three buckets:

1. **Strictly In-Scope Bug or Missing Control → Fix & Re-Verify Autonomously**
   - Code/measurement bug (wrong offset, corrupted read, broken baseline parity, NaN/truncation, benchmark GPU contention) or an obvious overlooked control **strictly required to validate the current spec's question**.
   - **Action**: Write corrective verification spec(s), register them under the current `<ROUND_ID>` via `cexpctl add-spec`, and re-dispatch Worker.
2. **Micro-Improvements, Corner Tweaks, or New Out-of-Scope Ideas → Hand Back to User**
   - Further optimization ideas, micro-improvements, or next-stage hypotheses beyond the current spec scope.
   - **Action**: **Do NOT dispatch to Worker.** Cancel Commander's monitor timer (if active), confirm Worker is stopped, report the round's conclusions, and present these proposals to the user.
3. **Clean Completion → Stop Cleanly (Never Nitpick)**
   - Questions answered cleanly within reasonable depth.
   - **Action**: Cancel Commander's monitor timer (if active), confirm both agents are stopped with zero active timers, and report results (grouped by Round if requested).

---

### Mode B: `recursive-improve` Mode (Active ONLY When Explicitly Requested in Current Invocation)

In a `recursive-improve` round targeting a specific system/algorithmic optimization direction (e.g., I/O pipeline, kernel scheduling, quantization throughput):

1. **First**: if a run suffered an implementation/measurement bug, fix and re-verify just like Bucket 1 above.
2. **Analyze Performance Traces & Remaining Headroom**:
   - Inspect profiler breakdowns, hardware utilization ceilings (bandwidth, FLOPs, overlap ratio, bubble time), and the latest iteration's net speedup / metric delta over the prior best arm.
3. **Decide Continue vs. Converge**:
   - **Observable / Meaningful Gain Still on the Table (`CONTINUE LOOP`)**:
     - Traces expose a clear remaining bottleneck (e.g., unhidden synchronization, suboptimal buffer staging, lock contention, kernel launch gap) with non-trivial headroom still uncaptured.
     - **Action**: Formulate the next targeted micro-improvement + trace spec(s) (`specs/<NEXT_ID>-*.md`), append them under the **same `<ROUND_ID>`** via `cexpctl add-spec` (setting `--depends <PREV_ID>` where sequential comparison applies, or parallel arms when comparing candidates), and wake `<WORKER_ADDRESS>` to execute and collect the new performance trace!
   - **No Visible / Meaningful Gain Left on the Table (`CONVERGE & STOP`)**:
     - Throughput/latency has reached near the hardware/theoretical bound, bottleneck is hidden inside unavoidable work, or the latest micro-improvement(s) yielded negligible / noisy / zero net gain.
     - **Action**: Stop the loop! Cancel Commander's monitor timer (if active), confirm Worker has zero active timers, and present the complete optimization trajectory (`baseline -> iter 1 -> ... -> converged optimum` + trace proof of why nothing significant remains on the table) to the user.
