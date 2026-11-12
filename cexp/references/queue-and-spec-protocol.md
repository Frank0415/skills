# Pointer Queue & Experiment Spec Protocol

## 1. Queue Discovery & Legacy Reuse Rule

Before creating any new queue file, inspect the active project / experiment directory (e.g., `experiments/runs/**`, `WORKER-QUEUE.md`, `QUEUE.md`) for an existing pointer queue belonging to the **same class or line of experiments**.

- **Default behavior (Reuse & Continue)**:
  - Unless the user **explicitly asks** to start a brand-new queue file or separate campaign, reuse the most recent queue file from the same experiment family.
  - Preserve all existing `spec` and `result` rows without editing or deleting completed history.
  - Preserve the existing table schema (whether 3-column or 5-column) and ID prefix/numbering (e.g., if the highest ID in the existing queue is `C004`, start new specs at `C005`; if `S012`, start at `S013`).
  - Update `specs/00-contract.md` only if the Worker address, server directory, or global resource bounds changed.
- **Explicit fresh-queue override**:
  - Create a new campaign folder (e.g., `experiments/runs/<YYYY-MM-DD>-<campaign-slug>/`) with a fresh `WORKER-QUEUE.md`, `specs/00-contract.md`, `specs/`, `results/`, and `artifacts/` **only** when no same-class queue exists or the user explicitly instructs to start a new queue.

---

## 2. Pure Pointer Table Formats

A queue file MUST remain a pure table of pointers. Never paste multi-paragraph plans, code snippets, or experimental logs into the queue file.

### Format A: 5-Column Parallel-Aware Pointer Table (Default for New Queues)

```markdown
# <Campaign Name> Work Queue (Pointer Table)

Shared Contract: [00-contract.md](specs/00-contract.md)

| ID | Type | Depends | Resources | Pointer |
| --- | --- | --- | --- | --- |
| E001 | spec | none | 1xGPU (>=24GiB, exclusive) | [E001 Spec: Short Title](specs/E001-slug.md) |
| E002 | spec | none | 1xGPU (>=40GiB, shared-ok) | [E002 Spec: Short Title](specs/E002-slug.md) |
| E003 | spec | E001 | 2xGPU (>=40GiB, exclusive) | [E003 Spec: Short Title](specs/E003-slug.md) |
| E002 | result | - | - | [E002 Result](results/E002.md) |
```

### Format B: 3-Column Legacy Pointer Table (Supported Seamlessly on Reuse)

```markdown
# <Campaign Name> 工作队列（纯指针表）

共享执行契约：[00-contract.md](specs/00-contract.md)

| 问题 | 类型 | 指针 |
| --- | --- | --- |
| C001 | spec | [C001规格：标题](specs/C001-slug.md) |
| C001 | result | [C001答复](results/C001.md) |
```

When reusing a 3-column queue, keep the 3-column format in the queue file and put the full `depends_on` and GPU/memory resource metadata at the top of each `specs/<ID>-<slug>.md` file so `cexperiment-worker` still schedules out-of-order and in parallel.

### Append-Only & Atomic Locking Invariants

1. **Commander appends `spec` rows; Worker appends `result` rows.**
2. **Never rewrite the whole file** when a Worker might be running concurrently. Always append rows under an exclusive `fcntl.flock` lock via `scripts/queue_ops.py`.
3. **Never overwrite a completed `spec`/`result` pair.** If an in-scope bug requires fixing and re-running `C002`, leave `C002`'s original `spec` and `result` untouched as audit history and append a new `C005` spec pointer (e.g., `C005: Fix & Re-verify C002 ...`).

---

## 3. Shared Contract Template (`specs/00-contract.md`)

Every queue file links to one shared execution contract defining campaign-wide rules:

```markdown
# Shared Execution Contract (<Campaign Name>)

- **Queue File**: `<absolute-or-repo-relative-path-to-queue>`
- **Commander Address**: `<commander-return-handle-or-session-id>`
- **Worker Address**: `<worker-handle-or-session-id>`
- **Target Compute Host(s)**: `<ssh-alias-or-local-host>` / Working Dir: `<remote-work-dir>`

## 1. Division of Labor
- Commander owns all research hypotheses, experimental designs, spec revisions, and scope decisions.
- Worker executes specs faithfully, maximizes GPU/server utilization via out-of-order and parallel packing, supervises active runs via scheduled wakeups, writes `results/<ID>.md`, and appends `result` pointers.

## 2. Parallelism & Resource Guardrails
- Out-of-order and multi-GPU parallel execution are explicitly authorized whenever `depends_on` prerequisites have `result` rows and combined resource budgets fit.
- **GPU etiquette**: Use any GPU with sufficient free VRAM; never kill processes owned by other users.
- **Benchmark isolation**: Specs marked `exclusive_gpu_timing: true` must not share physical GPUs with other active jobs from this campaign.
- **Host RAM ceiling**: Combined RSS + pinned memory + `/dev/shm` across all concurrent jobs <= `<N>` GiB (abort guard at `<0.85*N>` GiB).
- **Disk ceiling & cleanup**: Active temporary data <= `<M>` GiB; delete raw intermediate shards/checkpoints immediately after verifying metrics so persistent footprint stays <= `<K>` GiB.

## 3. Reusable Baseline Assets
- List verified scripts, datasets, model weights, or I/O harnesses that specs must reuse instead of re-implementing from scratch.
```

---

## 4. Individual Spec Template (`specs/<ID>-<slug>.md`)

Write every spec so an execution-focused Worker has zero ambiguity and can bin-pack it alongside other specs immediately:

```markdown
# <ID>: <Concise Experiment Title>

## Scheduling & Resource Profile
- `id`: `<ID>`
- `depends_on`: `none`  <!-- or comma-separated IDs like C001, C002 -->
- `priority_tier`: `1`  <!-- 1 = launch first when tied; 2 = fill spare GPUs or run after tier 1 -->
- `gpu_count`: `1`
- `min_vram_gib_per_gpu`: `32`
- `prefer_nvlink_pair`: `false`
- `exclusive_gpu_timing`: `true`  <!-- true for wall-time/latency benchmarks; false for quality/training runs -->
- `host_ram_budget_gib`: `16`
- `disk_peak_gib`: `20`
- `estimated_wall_minutes`: `90`  <!-- initial rough hint; Worker measures/updates actual 5*X estimate -->

## 1. Objective & Question
State the exact technical question and falsifiable hypothesis.

## 2. Experimental Matrix & Controls
Define every arm/condition, fixed hyperparameters, dataset slice, random seeds, warmup/repeat counts, and whether early stopping is forbidden.

## 3. Implementation & Reuse Notes
Point to exact codebase files, baseline scripts, environment variables, and known pitfalls to avoid.

## 4. Correctness & Validity Checks
Specify sanity checks (e.g., lossless hash match, baseline parity within tolerance, no NaN/OOM, GPU isolation check) required before accepting numbers.

## 5. Required Deliverables in `results/<ID>.md`
- `status`: `complete` | `negative` | `blocked`
- Summary conclusion table answering Section 1
- Raw per-repeat measurements + mean/std (for systems benchmarks) or full step curves + final eval metrics (for training runs)
- Peak GPU VRAM, host RAM, and disk usage + confirmation of temp cleanup
- Links to reproducible driver scripts, configs, and raw JSON/log artifacts
```
