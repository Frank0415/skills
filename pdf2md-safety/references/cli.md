# pdf2md-safety CLI (scan)

## Primary command

```bash
pdf2md scan <pdf> -o <out-dir>
```

Writes `security_report.json` to the output directory.

## Flags

| Flag | Description |
|---|---|
| `-o, --output` | Directory for `security_report.json` |
| `--no-ocr-diff` | Skip OCR-vs-extract diff layer |
| `--ocr-diff-full` | OCR diff all pages (default: sample) |
| `--force` | Exit 0 even when severity is HIGH |

## Without output dir

```bash
pdf2md scan paper.pdf
```

Prints JSON to stdout. Exit code 2 when `overall_severity` is HIGH or CRITICAL (unless `--force`).

## Dependencies

- **tesseract** — OCR diff layer
- **pdf2md** CLI — `pip install -e .` or `./scripts/install.sh`

MinerU is not required for scan-only.
