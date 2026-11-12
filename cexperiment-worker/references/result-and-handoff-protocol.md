# Result Registration, Trace Feedback & Commander Callback Protocol

## 1. Writing `results/<ID>.md` & Trace Requirements

Format every result report from `assets/result-template.md` **before** registering its pointer in the Queue file:

- Always verify correctness gates and clean up temporary raw shards/checkpoints per `specs/00-contract.md` first.
- **When `cexp-worker pending` shows `mode=recursive-improve` on the spec's Round**: include fine-grained **Performance Trace & Bottleneck Breakdown** data (compute vs. I/O vs. sync bubble wall-times, hardware bandwidth/FLOP utilization, and profiler summary links) in Section 3 of `results/<ID>.md` so Commander can judge whether visible performance gain remains on the table.

---

## 2. Atomic Result-Pointer Registration (`cexp-worker add-result`)

Register completed (or blocked) results exclusively via the bundled `flock`-safe binary:

```bash
<cexperiment-worker-skill-dir>/scripts/cexp-worker add-result <queue-file> \
  --id <ID> \
  --path "results/<ID>.md" \
  --label "<ID> Result" \
  --status <complete | negative | blocked>
```

`cexp-worker add-result` verifies `results/<ID>.md` exists on disk, acquires an exclusive `flock`, inherits `<ID>`'s `Round` tag automatically, and appends (or upgrades a previously `blocked` entry) without touching other rows.

---

## 3. Platform-Agnostic Commander Callback Payloads

Send callbacks to `<COMMANDER_RETURN_ADDRESS>` through whatever cross-session messaging, thread-send, queue, or agent callback tool the runtime exposes:

### A. Missing Timer Capability (`NO_TIMER_CAPABILITY`)

```text
NO_TIMER_CAPABILITY: Current Worker runtime does not support scheduled wakeups / timers. Zero server jobs were started and this Worker has stopped. Please ask the user to switch the Worker agent backend.
```

### B. Blocking Spec Ambiguity (`SPEC_QUESTION`)

```text
SPEC_QUESTION <ID>: <1-2 sentence statement of the spec contradiction/ambiguity>
- Diagnostic report: <PATH_TO_results_ID_md>
- Queue file: <ABSOLUTE_QUEUE_PATH>
- Independent specs status: <running on GPUs [...] with active timer | 0 runnable, timers cleared>
```

### C. Queue Completed (`QUEUE_COMPLETE`)

Send **only after** confirming `cexp-worker pending <queue-file>` shows `ready=0, waiting_on_deps=0`, `active_jobs == 0`, and **all scheduled timers created by this Worker are canceled**:

```text
QUEUE_COMPLETE: All runnable specs in <ABSOLUTE_QUEUE_PATH> have finished and result pointers are registered.
- Round(s) updated: <ROUND_ID(s)>
- Completed items: <ID1 (complete), ID2 (complete), ...>
- Worker timers: ALL CANCELED (0 active timers)
- Server state: 0 active campaign GPU processes; temp artifacts cleaned per contract
Ready for Commander analysis and convergence check.
```
