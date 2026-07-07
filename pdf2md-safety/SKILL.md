---
name: pdf2md-safety
description: Scans PDFs for injection attacks, hidden text, and OCR-visible secrets without converting to markdown. Use when auditing PDF safety, checking for prompt injection or invisible text, reviewing untrusted downloads, or when the user wants security_report.json only — not PDF-to-markdown conversion (use pdf2md for that).
---

# pdf2md-safety

Security scan only. No MinerU, no `document.md`.

## Quick start

```bash
pdf2md scan paper.pdf -o ./scan-out
```

Stdout JSON when `-o` is omitted:

```bash
pdf2md scan paper.pdf
```

Install:

```bash
export PDF2MD_REPO=/path/to/pdf2md
~/.agents/skills/pdf2md-safety/scripts/install.sh --repo "$PDF2MD_REPO"
```

Requires `pdf2md` and `tesseract` on PATH. Does not require `mineru`.

## Workflow

```
- [ ] Step 1: Run scan
- [ ] Step 2: Read security_report.json (or stdout JSON)
- [ ] Step 3: Summarize findings for the user
- [ ] Step 4: Recommend block, force, or adjusted flags
- [ ] Step 5: Validate report
```

**Step 1 — Scan**

```bash
pdf2md scan <pdf> -o <out-dir>
```

**Step 2 — Interpret**

Full field reference: [references/security-report.md](references/security-report.md).

**Step 3 — Report to user**

Always include:
- `overall_severity`
- Which layer fired (`structural` vs `ocr_diff`)
- Page numbers and representative `content` from findings

**Step 4 — Recommend next step**

| `overall_severity` | Recommendation |
|---|---|
| `NONE` / `LOW` / `MEDIUM` | Safe to proceed with `pdf2md convert` if user wants markdown |
| `HIGH` / `CRITICAL` | Do not convert or ingest. Offer `--no-ocr-diff` retry for slide decks |
| User insists | Note they must pass `--force` on convert — document the risk |

**Step 5 — Validate**

```bash
python ~/.agents/skills/pdf2md-safety/scripts/validate_report.py ./scan-out
```

## Output

With `-o`:

```
scan-out/
└── security_report.json
```

## Flags

| Situation | Flag |
|---|---|
| Slide deck OCR noise | `--no-ocr-diff` |
| Exit 0 despite HIGH (audit complete) | `--force` |
| Full-document OCR diff | `--ocr-diff-full` |

CLI: [references/cli.md](references/cli.md). Examples: [references/examples.md](references/examples.md).
