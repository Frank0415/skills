# Shared Execution Contract: <CAMPAIGN_TITLE>

- **Queue Document**: `<RELATIVE_OR_ABS_QUEUE_PATH>`
- **Commander Return Address**: `<COMMANDER_RETURN_ADDRESS>`
- **Worker Address**: `<WORKER_ADDRESS>`
- **Target Compute Host & Workdir**: `<SSH_HOST_OR_LOCAL>` / `<REMOTE_WORK_DIR>`

## 1. Division of Responsibilities
- **Commander (`cexp`)**: Owns all hypotheses, experimental controls, baseline definitions, scope boundaries, and convergence decisions.
- **Worker (`cexperiment-worker`)**: Translates each `spec` pointer into reproducible runs, packs ready specs out-of-order and in parallel across all available GPUs, supervises active jobs via adaptive scheduled wakeups, writes `results/<ID>.md`, registers `result` pointers via `cexp-worker`, and wakes Commander when blocked or finished.

## 2. Parallelism & Resource Guardrails
- **Out-of-order & multi-GPU execution**: Authorized and expected whenever a spec's `Depends` prerequisites have non-blocked `result` rows and combined resource budgets fit.
- **Shared-server etiquette**: Use any GPU with sufficient free VRAM; never terminate processes belonging to other users.
- **Timing isolation**: Any spec with `exclusive_gpu_timing: true` (or `excl` in its `Resources` tag) must not share physical GPUs with other active jobs from this campaign.
- **Host RAM ceiling**: Combined RSS + pinned memory + `/dev/shm` across concurrent jobs <= `<MAX_HOST_RAM_GIB>` GiB (safety abort guard at `<ABORT_RAM_GIB>` GiB).
- **Disk ceiling & cleanup**: Active temporary footprint <= `<MAX_ACTIVE_DISK_GIB>` GiB; delete raw intermediate shards/checkpoints immediately after verifying metrics so persistent footprint stays <= `<RETAINED_DISK_GIB>` GiB.

## 3. Verified Reusable Assets & Invariants
- List baseline harnesses, datasets, model weights, and verified numerical/system invariants that specs must reuse rather than re-inventing.
