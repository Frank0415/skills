# security_report.json

## Top level

```json
{
  "structural": { "severity": "...", "findings": [], "pages_scanned": 0 },
  "ocr_diff": {
    "enabled": true,
    "severity": "...",
    "pages_sampled": [1, 2, 3],
    "hidden_by_ocr_diff": ["..."]
  },
  "overall_severity": "HIGH"
}
```

`overall_severity` is the max of `structural.severity` and `ocr_diff.severity`.

## structural (layer A)

Always runs on every page with extractable characters.

| Finding type | Meaning |
|---|---|
| Low-contrast text | Text nearly invisible against rendered background |
| Tiny text | Font size below threshold |
| Off-page text | Characters outside page bounds |
| Suspicious patterns | Regex hits for prompt-injection phrases |

Each finding includes `page`, `type`, `description`, `content`, `location`, `severity`.

## ocr_diff (layer B)

Runs on digital PDFs (enough embedded text). Compares text-layer extraction to Tesseract OCR on rendered pages.

- `hidden_by_ocr_diff`: tokens OCR sees but extraction does not
- `enabled: false` + `skipped_reason` when `--no-ocr-diff` or scanned/non-digital PDF

## Severity scale

`NONE` < `LOW` < `MEDIUM` < `HIGH` < `CRITICAL`

## Blocking

- `pdf2md scan`: exit 2 on `HIGH`/`CRITICAL` unless `--force`
- `pdf2md convert`: same rule; conversion skipped on block

## Reporting template

```markdown
## PDF security scan: <filename>

**Overall:** HIGH

**Structural:** NONE (35 pages)
**OCR diff:** HIGH — pages [1, 18, 35]; hidden tokens: "ignore previous", ...

**Recommendation:** Do not ingest. If this is a slide deck, retry with `--no-ocr-diff`.
```
