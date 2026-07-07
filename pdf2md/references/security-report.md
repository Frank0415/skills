# security_report.json (convert context)

Written to the output directory before conversion. If `overall_severity` is `HIGH` or `CRITICAL`, `pdf2md convert` exits 2 and does not write `document.md` unless `--force`.

## Key fields

- `overall_severity` — max of structural + OCR diff; read this first
- `structural.findings` — low-contrast, tiny, off-page, injection-pattern hits
- `ocr_diff.hidden_by_ocr_diff` — words visible to OCR but not in text layer

Full schema and reporting guidance: see **pdf2md-safety** skill `references/security-report.md`.

## Quick decision

| `overall_severity` | `document.md` safe to use? |
|---|---|
| `NONE` / `LOW` / `MEDIUM` | Yes |
| `HIGH` / `CRITICAL` | No (unless user approved `--force`) |
