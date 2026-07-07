#!/usr/bin/env bash
# Check tesseract (and report nvidia-smi). No auto-sudo.
set -euo pipefail

OS="$(uname -s)"
MISSING=0

check_cmd() {
  local cmd="$1"
  if command -v "$cmd" >/dev/null 2>&1; then
    echo "  ok: $cmd"
    return 0
  fi
  echo "  missing: $cmd"
  MISSING=1
  return 1
}

echo "Checking system dependencies..."

check_cmd tesseract || {
  if [[ "$OS" == "Darwin" ]]; then
    echo "    install: brew install tesseract"
  else
    echo "    install: sudo apt install tesseract-ocr   # or: sudo dnf install tesseract"
  fi
}

if command -v nvidia-smi >/dev/null 2>&1; then
  echo "  ok: nvidia-smi"
  nvidia-smi --query-gpu=name,driver_version --format=csv,noheader 2>/dev/null | sed 's/^/    /' || true
else
  echo "  info: nvidia-smi not found (expected on mac_arm / linux_cpu)"
fi

if [[ "$MISSING" -ne 0 ]]; then
  echo ""
  echo "Some system dependencies are missing."
  exit 1
fi

echo "All required system dependencies found."
