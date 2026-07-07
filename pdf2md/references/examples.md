# pdf2md examples

## Full mode (default)

```bash
pdf2md convert paper.pdf -o ./out
# read security_report.json, then polish document.md per format-full.md
python ~/.agents/skills/pdf2md/scripts/validate_bundle.py ./out
```

## Lite mode

User says `/pdf2md lite` or wants raw MinerU output:

```bash
pdf2md convert paper.pdf -o ./out
# use document.md as-is — skip polish step
python ~/.agents/skills/pdf2md/scripts/validate_bundle.py ./out
```

## Slides (OCR false positives)

```bash
pdf2md convert slides.pdf -o ./out --no-ocr-diff --force
# then polish in full mode
```

## Blocked conversion

```bash
pdf2md convert suspicious.pdf -o ./out
# exit 2 — read security_report.json before --force
```

## Install

```bash
cd ~/Tools/pdf2md && ./scripts/install.sh && pdf2md doctor
```
