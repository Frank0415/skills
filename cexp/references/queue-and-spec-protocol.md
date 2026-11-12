# Redesigned Queue Schema, Progressive Disclosure & Round Protocol

## 1. Self-Contained CLI Location (Never Install to `PATH`)

`cexpctl` ships pre-compiled for Linux (`x86_64`, `aarch64`) and macOS (`arm64`, `x86_64`) directly inside the skill directory:

```bash
<cexp-skill-dir>/scripts/cexpctl --help
```

Never install `cexpctl` into global `PATH`, `/usr/local/bin`, or modify shell profiles. Always invoke it via its path inside the skill folder so the skill works out of the box on locked-down machines.

---

## 2. Progressive-Disclosure Queue Discovery (`discover` & `head`)

Before creating a queue document, search the workspace recursively using:

```bash
<cexp-skill-dir>/scripts/cexpctl discover <workspace-dir>
```

`discover` walks `<workspace-dir>` recursively and **progressively discloses ONLY the YAML frontmatter header** (`title`, `focus`, `contract`, `created`) plus compact round/item counts for every `cexp_queue: 1` file—never flooding context with full pointer tables. To inspect one file's header directly, run:

```bash
<cexp-skill-dir>/scripts/cexpctl head <queue-file>
```

### Decision Rule: Reuse vs. New Queue Document
1. **Explicit user override**: if the user explicitly asks to start a fresh queue document, run `cexpctl init` in a new campaign directory.
2. **Focus match (same overarching topic/system)**: if a discovered queue's `focus` covers the same research or optimization domain (and is not an unrelated legacy format), **reuse that queue document** by opening a new Round (`cexpctl new-round <queue-file> --summary "..." --mode <standard|recursive-improve>`) so Round 1, Round 2, etc. stay cleanly separated inside one unified history!
3. **Unrelated topic ("风马牛不相及") or no `cexp_queue: 1` file exists**: initialize a brand-new `CEXP-QUEUE.md` via `cexpctl init`. Never reuse ad-hoc non-`cexp_queue` files from older manual workflows.

---

## 3. Writing the `focus` Header (Progressive-Disclosure Calibration)

The `focus` field in the queue frontmatter is what future sessions see during `discover` and `head`. Calibrate its abstraction level carefully:

- **Right level (Umbrella Focus — 1 to 2 sentences)**: state the overarching system/research domain and core objective shared by the experiments in this queue.
  - *Good example*: `"围绕 Speculative Decoding 训练框架的低比特中间特征量化与快速显卡角色切换加速实验队列。"`
  - *Good example*: `"Optimize multi-tier NVMe-to-HBM feature prefetching and cache admission policies for long-context draft training."`
- **Too specific (Forbidden)**: do **not** enumerate individual tasks in `focus` (e.g., do not write *"E001 tests VMM pause, E002 probes INT4 on layer 3, E003 trains 2000 steps, E004..."*). Individual details belong in `specs/<ID>-*.md`.
- **Too vague (Forbidden)**: do not write generic placeholders like `"GPU experiments"` or `"Model training queue"`.

---

## 4. `cexp_queue: 1` Document Structure

Every queue document stays compact (~2 lines per experiment) by combining **YAML frontmatter**, a lightweight **`## Rounds` table**, and a **pure-pointer `## Pointers` table**:

```markdown
---
cexp_queue: 1
title: "Speculative Decoding Low-Bit Quantization & Framework Acceleration"
focus: "Evaluate low-bit activation quantization and fast GPU role-switching scheduling to maximize speculative training throughput."
contract: "specs/00-contract.md"
created: "2026-11-13"
---

## Rounds

| Round | Mode | Created | Summary |
| --- | --- | --- | --- |
| R1 | standard | 2026-11-13 | Initial VMM role-switch & quantization sensitivity baselines |
| R2 | recursive-improve | 2026-11-14 | Iterative I/O prefetch micro-optimization until ceiling |

## Pointers

| Round | ID | Kind | Depends | Resources | Status | Pointer |
| --- | --- | --- | --- | --- | --- | --- |
| R1 | E001 | spec | none | 1gpu:32G:excl | - | [VMM fast role switch](specs/E001-role-switch.md) |
| R1 | E002 | spec | none | 1gpu:40G:share | - | [Layer quant sensitivity](specs/E002-quant-probe.md) |
| R1 | E001 | result | - | - | complete | [E001 Result](results/E001.md) |
| R1 | E002 | result | - | - | complete | [E002 Result](results/E002.md) |
| R2 | E003 | spec | E001 | 2gpu:40G:excl | - | [Async I/O trace #1](specs/E003-io-trace1.md) |
```

### Key Invariants
1. **Round Separation (`R1`, `R2`, ...)**:
   - Every new user-initiated batch opens a new Round (`R1`, `R2`, `R3`...).
   - Within a single batch—such as in-scope bug-fix verifications or multiple micro-improvement iterations inside one `recursive-improve` round—newly appended `spec` pointers stay tagged with **that batch's Round ID** (e.g., all iterations of Round 2 belong to `R2`, linked by `Depends` chains `E003 -> E004 -> E005`).
   - When the user later asks for a report separating Round 1 vs. Round 2, run `cexpctl status <queue-file> --round R1` and `--round R2` to isolate each round cleanly.
2. **Compact Resource Token (`Resources` column)**:
   - Use `<N>gpu:<VRAM>G:<excl|share>` (e.g., `1gpu:32G:excl`, `2gpu:40G:excl`, `1gpu:24G:share`) so Worker sees GPU count, VRAM floor, and benchmark timing exclusivity at a glance, backed by full RAM/disk ceilings in `specs/<ID>-*.md`.
3. **Immutable History & `flock` Safety**:
   - Always mutate the queue via `cexpctl` (`init`, `new-round`, `add-spec`) and `cexp-worker` (`add-result`), both of which hold exclusive `flock` file locks.
   - Never overwrite completed `spec`/`result` pairs; append a new Spec ID (`E005`) when fixing or iterating.

---

## 5. Templates for Contracts & Specs

Copy or adapt the built-in assets when creating campaign files:
- Shared execution contract template → `assets/00-contract-template.md`
- Individual parallel-aware spec template → `assets/spec-template.md`
