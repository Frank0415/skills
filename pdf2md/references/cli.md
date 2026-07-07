# pdf2md CLI (convert)

## Primary command

```bash
pdf2md convert <pdf> -o <out-dir>
```

Runs security scan, writes `security_report.json`, then MinerU conversion if not blocked.

## Flags

| Flag | Description |
|---|---|
| `-o, --output` | Output directory (required) |
| `--no-ocr-diff` | Skip OCR-vs-extract diff in pre-convert scan |
| `--ocr-diff-full` | OCR diff all pages (default: sample) |
| `--force` | Convert even when `overall_severity` is HIGH |

## Other commands

```bash
pdf2md doctor    # check mineru + tesseract + platform manifest
```

For scan-only (no conversion), use the **pdf2md-safety** skill and `pdf2md scan`.

## Install

```bash
./scripts/install.sh
./scripts/install.sh --cpu   # Linux without NVIDIA (transformers on CPU, slow)
```

## Dependencies

- **mineru** — conversion
- **tesseract** — OCR diff in scan stage
- **uv** — install script
