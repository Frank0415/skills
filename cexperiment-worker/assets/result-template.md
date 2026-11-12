# <ID> Result: <SHORT_TITLE>

- **Round**: `<ROUND_ID>` (`<standard | recursive-improve>`)
- **Status**: `complete` | `negative` | `blocked`
- **Spec**: [<ID> Spec](../specs/<ID>-<slug>.md)
- **Host & Physical GPUs**: `<HOST>` / GPUs `[...]` (`exclusive_gpu_timing`: `true|false`)
- **Execution Window**: `<START_ISO>` → `<END_ISO>`

## 1. Core Conclusion Table

| Arm / Condition | Primary Metric(s) | Delta vs. Baseline / Previous Iter | Verdict |
| --- | --- | --- | --- |
| ... | ... | ... | ... |

## 2. Raw Measurements / Convergence Curves
- **Systems / Latency Benchmarks**: report every individual repeat plus mean and std.
- **Training / Quality Runs**: report step milestones, full curves, and final metrics.

## 3. Performance Trace & Bottleneck Breakdown (Mandatory in `recursive-improve` Rounds)
- Stage/component wall-time breakdown (compute vs. H2D/D2H/NVMe I/O vs. sync/launch bubbles).
- Hardware utilization vs. theoretical ceiling (effective GB/s, TFLOP/s, overlap fraction).
- Pointer to raw trace/profiler summary artifact: `[...](../artifacts/...)`

## 4. Correctness & Validity Gates
Record pass/fail verification for lossless hashes, numerical parity, absence of NaN/OOM, and GPU timing isolation.

## 5. Resource Peaks & Cleanup Audit
- **Peak GPU VRAM**: `<X>` GiB per GPU
- **Peak Host RAM (RSS + pinned + `/dev/shm`)**: `<Y>` GiB
- **Peak Active Disk → Retained Disk After Cleanup**: `<Peak>` GiB → `<Retained>` GiB

## 6. Reproducibility Links & Execution Notes
- Driver / script: `[...](../artifacts/...)`
- Raw metrics JSON / log: `[...](../artifacts/...)`
- Runtime fixes applied or factual anomalies observed.
