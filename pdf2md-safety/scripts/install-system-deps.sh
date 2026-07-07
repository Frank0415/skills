#!/usr/bin/env bash
# Scan-only dependencies: tesseract.
set -euo pipefail

OS="$(uname -s)"
MISSING=0

if command -v tesseract >/dev/null 2>&1; then
  echo "  ok: tesseract"
else
  echo "  missing: tesseract"
  MISSING=1
  if [[ "$OS" == "Darwin" ]]; then
    echo "    install: brew install tesseract"
  else
    echo "    install: sudo apt install tesseract-ocr"
  fi
fi

if [[ "$MISSING" -ne 0 ]]; then
  exit 1
fi

echo "Scan dependencies OK."
