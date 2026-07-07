---
name: pdf2md
description: >
  Runs PDF security scan then converts to markdown with MinerU. Default (full) mode
  also polishes document.md into proper study notes after the CLI finishes. Lite mode
  skips polish and uses raw MinerU output. Use for PDF-to-markdown, RAG ingest, or
  pdf2md convert. Lite only when user says /pdf2md lite or asks for raw/unpolished output.
  For scan-only, use pdf2md-safety.
---

# pdf2md

Scan → convert (CLI). Then polish (agent, full mode only).

Default: **full**. Switch: `/pdf2md lite`.

| Mode | After CLI completes |
|---|---|
| **full** (default) | Agent reads `document.md`, rewrites it into clean study notes |
| **lite** | Use `document.md` as-is — no agent rewrite |

CLI is identical in both modes. Only post-CLI agent behavior differs.

## Quick start

```bash
pdf2md convert paper.pdf -o ./out
```

Then **full mode**: polish `document.md` per [references/format-full.md](references/format-full.md).

Install (from deployed skill):

```bash
export PDF2MD_REPO=/path/to/pdf2md
~/.agents/skills/pdf2md/scripts/install.sh --repo "$PDF2MD_REPO"
pdf2md doctor
```

## Workflow

```
- [ ] Step 1: Run convert
- [ ] Step 2: Read security_report.json
- [ ] Step 3: If blocked (exit 2), stop — do not use document.md
- [ ] Step 4: If OK, open document.md
- [ ] Step 5: Full mode only — polish document.md in place
- [ ] Step 6: Validate bundle
```

**Step 1 — Convert**

```bash
pdf2md convert <pdf> -o <out-dir>
```

Blocked on `HIGH`/`CRITICAL` unless `--force`. Report still written.

**Step 2 — Security**

[references/security-report.md](references/security-report.md). Slides with structural `NONE` but OCR `HIGH` → retry `--no-ocr-diff`.

**Step 3–4 — Output**

```
out/
├── document.md
├── security_report.json
├── meta.json
└── images/
```

**Step 5 — Polish (full mode only)**

Skip this step in **lite** mode (`/pdf2md lite`).

Read `document.md`. Rewrite **in place** so it reads like real notes, not raw slide OCR. Rules and examples: [references/format-full.md](references/format-full.md). Stitch logically attached things (such as split sentences or code blocks) back together across page boundaries.

Do not delete `![](images/...)` lines. Do not invent facts — restructure and clean only.

**Step 6 — Validate**

```bash
python ~/.agents/skills/pdf2md/scripts/validate_bundle.py ./out
```

## CLI flags

| Situation | Flag |
|---|---|
| Slide OCR noise | `--no-ocr-diff` |
| User accepts HIGH risk | `--force` |
| Full OCR diff (slow) | `--ocr-diff-full` |

Details: [references/cli.md](references/cli.md). Examples: [references/examples.md](references/examples.md).

## Platform

Inference from `.pdf2md/platform.json` (MinerU `vlm-auto-engine` under the hood):

| Profile | Inference |
|---|---|
| `mac_arm` | MLX |
| `linux_gpu` | HuggingFace transformers (CUDA) |

Only MLX and HF transformers are supported. vLLM / hybrid / lmdeploy are not used.
