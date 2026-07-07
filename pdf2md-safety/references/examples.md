# pdf2md-safety examples

## Scan to file

```bash
pdf2md scan untrusted.pdf -o ./scan-out
python ~/.agents/skills/pdf2md-safety/scripts/validate_report.py ./scan-out
```

## Scan to stdout

```bash
pdf2md scan paper.pdf | jq .overall_severity
```

## Slide deck (reduce OCR noise)

```bash
pdf2md scan slides.pdf -o ./scan-out --no-ocr-diff
```

## Full OCR pass (slow)

```bash
pdf2md scan book.pdf -o ./scan-out --ocr-diff-full
```

## Audit then convert (two skills)

```bash
pdf2md scan paper.pdf -o ./scan-out
# review security_report.json
pdf2md convert paper.pdf -o ./out   # pdf2md skill
```
