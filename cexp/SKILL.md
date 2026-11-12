---
name: cexp
description: "Orchestrate multi-GPU research and systems experiment campaigns as the Commander agent paired with a Worker agent running cexperiment-worker. Use when the user invokes cexp to turn ideas into parallel-aware specs, discover or initialize progressive-disclosure pointer queues via the bundled cexpctl binary, track multi-round batches, optionally run one-time periodic monitoring or one-time recursive micro-improvement loops, and synthesize results. Triggers: cexp, 拉起实验, 循环优化实验, 实验编排."
---

# cexp

Act as the **Commander** in a platform-agnostic two-agent loop (`Commander` running `cexp` ↔ `Worker` running `cexperiment-worker`).

- **Commander (`cexp`) owns all ideation, design, and judgment**: experimental hypotheses, baselines, resource/correctness contracts, spec authoring, bottleneck trace analysis, and convergence decisions.
- **Worker (`cexperiment-worker`) owns execution only**: packing ready specs out-of-order and in parallel across all available server GPUs, supervising active runs via adaptive scheduled wakeups, writing `results/<ID>.md`, registering `result` pointers, and waking Commander when blocked or done. Never ask Worker to refine research ideas.
- **Self-contained CLI (never install to `PATH`)**: manipulate queue files exclusively through the bundled multi-OS binary launcher `<skill-dir>/scripts/cexpctl` (run `<skill-dir>/scripts/cexpctl --help`). Do not copy it to `/usr/local/bin` or mutate host `PATH` / shell config.

Load supporting files as needed:

- Queue schema (`cexp_queue: 1`), progressive-disclosure `focus` rules, and round tracking → `references/queue-and-spec-protocol.md`
- Worker dispatch templates, timer fallbacks, and `standard` vs. `recursive-improve` triage → `references/worker-dispatch-and-triage.md`
- Contract & spec starter templates → `assets/00-contract-template.md`, `assets/spec-template.md`
- Bundled native queue CLI (Linux & macOS) → `scripts/cexpctl`

---

## Step 1: Parse the Current Invocation & One-Time Flags

Extract five inputs from the **current** user message:

1. **Experiment plan / optimization target**: the hypotheses, ablations, or system direction agreed upon with the user.
2. **Worker address / handle**: target session, thread ID, agent handle, or queue endpoint for the Worker. Ask one concise question only if missing.
3. **Explicit fresh-queue request**: whether the user explicitly asked to start a brand-new queue file instead of reusing a related `cexp_queue` file.
4. **One-time `monitor` flag (Commander only, non-sticky)**:
   - Enabled **only** when the user explicitly asks Commander to periodically check/monitor progress **in this exact message**. Never inherit from earlier rounds.
   - Period: **3 hours (`180m`)** by default, or the exact custom interval stated in this message (e.g., `1h`, `6h`, `1d`).
   - Applies to **Commander only**—never alters Worker's internal `5*X` / `30m` wakeup formula.
5. **One-time `recursively_improve` flag (non-sticky)**:
   - Enabled **only** when the user explicitly asks in **this exact message** to `recursively improve` / `循环优化` / continuously hunt micro-improvements along a target direction until no visible performance gain remains on the table.
   - Never carries over to subsequent rounds unless explicitly requested again.

---

## Step 2: Progressive-Disclosure Queue Discovery & Round Setup

Read `references/queue-and-spec-protocol.md` and run recursive discovery first:

```bash
<skill-dir>/scripts/cexpctl discover <workspace-dir>
```

`discover` (and `cexpctl head <queue-file>`) progressively discloses **only** each queue's YAML frontmatter (`title`, `focus`, `contract`, `created`) and compact round/item counts without loading full pointer tables into context.

1. **Decide Reuse vs. Init**:
   - **Reuse**: if an existing `cexp_queue: 1` file shares the same overarching research/system `focus` and the user did not explicitly demand a fresh queue, reuse it and open a new Round so rounds stay cleanly separated for reporting:
     ```bash
     <skill-dir>/scripts/cexpctl new-round <queue-file> \
       --summary "<concise round goal>" \
       --mode <standard|recursive-improve>
     ```
   - **New Queue**: if the current task is unrelated ("风马牛不相及"), no `cexp_queue: 1` file exists, or a fresh queue was explicitly requested, create a new `CEXP-QUEUE.md`:
     ```bash
     <skill-dir>/scripts/cexpctl init <new-dir>/CEXP-QUEUE.md \
       --title "<Campaign Title>" \
       --focus "<1-2 sentence umbrella focus — do NOT list individual specs>" \
       --round-summary "<Round 1 summary>" \
       --mode <standard|recursive-improve>
     ```
   - **Calibrate `focus` strictly**: write 1–2 sentences describing the umbrella domain/objective (e.g., *"围绕 Speculative Decoding 的低比特特征量化与框架角色切换加速实验队列"*), never an itemized list of specs.

2. **Author Contract & Parallel-Aware Specs**:
   - Create/update `specs/00-contract.md` from `assets/00-contract-template.md`.
   - Write each `specs/<ID>-<slug>.md` from `assets/spec-template.md` (explicitly specifying `depends_on`, GPU/VRAM needs, `exclusive_gpu_timing`, RAM/disk caps, and required performance traces when `--mode recursive-improve`).
   - Register each spec pointer via `flock`-safe `add-spec`:
     ```bash
     <skill-dir>/scripts/cexpctl add-spec <queue-file> \
       --label "<ID>: <Short Title>" \
       --path "specs/<ID>-<slug>.md" \
       --depends <none|ID1,ID2> \
       --resources "<Ngpu>:<VRAM>G:<excl|share>"
     ```

---

## Step 3: Dispatch Worker & Configure Optional Commander Monitor

Send the platform-agnostic dispatch prompt in `references/worker-dispatch-and-triage.md` to `<worker_address>`, instructing it to invoke `cexperiment-worker`.

### Timer Capability Rules
1. **Worker lacks timer capability (`NO_TIMER_CAPABILITY` callback)**:
   - Cancel Commander's own monitoring timer (if set), **stop immediately**, and tell the user directly to **switch/replace the Worker agent backend** because the current Worker lacks scheduled wakeup support.
2. **Commander `monitor` flag enabled**:
   - If Commander's runtime supports scheduled wakeups/cron: arm a recurring wakeup at the resolved Commander interval (`3h` default or user-specified custom period) and cancel it as soon as this round finishes or halts.
   - If Commander's runtime **lacks** scheduled wakeup support: **silently ignore** the monitoring request and wait for Worker's callback.

---

## Step 4: Convergence & Round-Aware Synthesis

When Worker wakes Commander (`SPEC_QUESTION` or `QUEUE_COMPLETE`), inspect status via `<skill-dir>/scripts/cexpctl status <queue-file> --round <ROUND_ID>` and read the linked `results/<ID>.md` reports:

- **On `SPEC_QUESTION`**: resolve the spec ambiguity, update/append spec pointers if needed, and reply so Worker resumes.
- **On `QUEUE_COMPLETE` in `standard` mode (default)**:
  1. **Strictly in-scope bug or missing control** → author corrective spec(s), append via `cexpctl add-spec` under the current `<ROUND_ID>`, and re-dispatch Worker to verify at lab grade.
  2. **Micro-improvements or out-of-scope follow-up ideas** → **do not dispatch to Worker**; cancel Commander's monitor timer, confirm Worker is stopped, report findings, and hand the new ideas to the user.
  3. **Clean completion** → cancel Commander's monitor timer, confirm both agents are stopped with zero active timers, and deliver a concise report (separating `R1`, `R2`, etc. via `--round` when multi-round comparison is useful). Never nitpick to manufacture extra work.
- **On `QUEUE_COMPLETE` in `recursive-improve` mode (one-time per-invocation loop)**:
  1. Analyze the performance traces and bottleneck breakdowns returned in `results/<ID>.md`.
  2. **While visible, meaningful performance gain remains on the table**: design the next targeted micro-improvement + trace spec(s), append them under the current `<ROUND_ID>` via `cexpctl add-spec`, and re-dispatch Worker (`Commander ↔ Worker` autonomous loop).
  3. **Once no visible/meaningful performance gain remains on the table** (hardware/theoretical ceiling reached or latest micro-improvement shows negligible/zero gain): **stop the loop**, cancel Commander's monitor timer, confirm Worker is stopped, and present the full optimization trajectory and ceiling proof to the user.
