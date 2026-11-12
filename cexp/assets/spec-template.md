# <ID>: <CONCISE_EXPERIMENT_TITLE>

## Scheduling & Resource Profile
- `id`: `<ID>`
- `round`: `<ROUND_ID>`
- `depends_on`: `none`  <!-- or comma-separated IDs, e.g. E001,E002 -->
- `priority_tier`: `1`  <!-- 1 = launch first on tie; 2 = fill spare GPUs -->
- `gpu_count`: `1`
- `min_vram_gib_per_gpu`: `32`
- `prefer_nvlink_pair`: `false`
- `exclusive_gpu_timing`: `true`  <!-- true (`excl`) for latency/throughput benchmarks; false (`share`) otherwise -->
- `host_ram_budget_gib`: `16`
- `disk_peak_gib`: `20`
- `estimated_wall_minutes`: `60`  <!-- initial rough hint; Worker measures actual 5*X remaining time -->

## 1. Question & Hypothesis
State the exact technical question, target metric, and falsifiable hypothesis (or target bottleneck if running inside a `recursive-improve` round).

## 2. Experimental Matrix & Controls
Specify every condition/arm, fixed baselines, dataset/workload slice, random seeds, warmup/repeat counts, and early-stopping rules.

## 3. Implementation & Reuse Notes
Reference exact repository files, reusable harnesses, environment flags, and known pitfalls.

## 4. Validity & Sanity Gates
Define mandatory checks before numbers are accepted (e.g., lossless hash / numerical parity, baseline reproduction, absence of NaN/OOM, clean GPU isolation during timing).

## 5. Required Deliverables in `results/<ID>.md`
- `status`: `complete` | `negative` | `blocked`
- Summary conclusion table directly answering Section 1
- Raw per-repeat values + mean/std (systems benchmarks) or full step curves + final metrics (training runs)
- **Performance trace / bottleneck breakdown** (mandatory when `round` mode is `recursive-improve`, so Commander can identify whether further gain remains on the table)
- Peak GPU VRAM, host RAM, and disk audit + cleanup confirmation
- Relative links to driver scripts, configs, and raw JSON/log artifacts
