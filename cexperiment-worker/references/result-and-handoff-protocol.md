# Result Report & Commander Callback Protocol

## 1. Required `results/<ID>.md` Template

Always finish writing and verifying `results/<ID>.md` **before** appending its `result` pointer row to the Queue file.

```markdown
# <ID> Result: <Short Title>

- **Status**: `complete` | `negative` | `blocked`
- **Spec**: [<ID> Spec](../specs/<ID>-<slug>.md)
- **Host & GPUs Used**: `<host>` / Physical GPUs `[...]` (`exclusive_gpu_timing`: `true|false`)
- **Wall-Clock Window**: `<start-iso>` → `<end-iso>`

## 1. Core Conclusion Table
Summarize the direct quantitative answer to every question in the spec.

| Arm / Condition | Primary Metric(s) | Baseline Delta | Verdict |
| --- | --- | --- | --- |
| ... | ... | ... | ... |

## 2. Raw Measurements / Training Curves
- **Systems / Latency Benchmarks**: report every individual repeat alongside mean and std (never report a single cherry-picked run).
- **Training / Quality Runs**: report step milestones, convergence trajectory, and final evaluation metrics across all mandated steps.

## 3. Correctness & Invariant Checks
Document pass/fail evidence for every validity check required by the spec (e.g., SHA256 lossless verification, numerical parity, absence of NaN/truncation, GPU isolation check during timing).

## 4. Resource Peaks & Cleanup Audit
- **Peak GPU VRAM**: `<X>` GiB per GPU
- **Peak Host RAM (RSS + pinned + `/dev/shm`)**: `<Y>` GiB (Contract limit: `<Limit>` GiB)
- **Peak Active Disk / Post-Cleanup Disk**: `<Peak>` GiB → `<Retained>` GiB after removing temporary shards/checkpoints

## 5. Reproducibility Artifacts & Anomalies
- Driver / script: `[...](../artifacts/...)`
- Raw JSON / log: `[...](../artifacts/...)`
- Bugs fixed during execution or boundary caveats observed (factual record only; do not propose new research scope).
```

---

## 2. Atomic Result-Pointer Registration

Never rewrite the whole Queue file (another process or Commander could be reading or appending specs). Always append via `scripts/worker_ctl.py`:

```bash
python3 <skill-dir>/scripts/worker_ctl.py append-result <queue-file> \
  --id <ID> \
  --label "<ID> Result" \
  --path "results/<ID>.md"
```

---

## 3. Commander Callback Payloads (Platform-Agnostic)

Deliver callbacks to `<COMMANDER_RETURN_ADDRESS>` using whatever cross-session messaging, thread-send, CLI queue, or agent notification tool the current runtime provides.

### A. Missing Timer Capability (`NO_TIMER_CAPABILITY`)

Send immediately in Step 0 when the Worker runtime lacks a native scheduled wakeup / timer tool:

```text
NO_TIMER_CAPABILITY: Current Worker runtime does not expose a scheduled wakeup / cron / timer capability. Per `cexperiment-worker`, zero server experiments were started and this Worker has stopped. Please notify the user to switch the Worker agent backend.
```

### B. Blocking Spec Question (`SPEC_QUESTION`)

Send only when a spec contains an unresolvable contradiction, missing prerequisite asset, or ambiguity that would alter experimental meaning (never for ordinary code bugs you can fix yourself):

```text
SPEC_QUESTION <ID>: <1-2 sentence statement of the exact blocker and decision needed>
- Diagnostic report: <ABSOLUTE_OR_REPO_PATH_TO_results_ID_md>
- Queue file: <ABSOLUTE_QUEUE_PATH>
- Other independent specs status: <running on GPUs [...] with active timer | none remaining>
```

### C. Full Queue Completion (`QUEUE_COMPLETE`)

Send **only after** verifying:
1. Every `spec` row in `<ABSOLUTE_QUEUE_PATH>` has a paired `result` row;
2. **All scheduled timers / cron jobs created by this Worker have been canceled/deleted**;
3. Zero stray GPU processes remain and temporary large artifacts are cleaned per contract.

```text
QUEUE_COMPLETE: All queued specs in <ABSOLUTE_QUEUE_PATH> have completed and result pointers are appended.
- Completed items: <ID1 (complete), ID2 (complete), ID3 (negative), ...>
- Worker timers: ALL CANCELED (0 active timers)
- Server cleanup: 0 active campaign GPU processes; retained disk <X> GiB within contract
Ready for Commander analysis and triage.
```
