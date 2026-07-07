#!/usr/bin/env bash
# Download MinerU model weights (Hugging Face cache).
set -euo pipefail

REPO="${PDF2MD_REPO:-}"
SKILL_SCRIPTS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo) REPO="$2"; shift 2 ;;
    *) shift ;;
  esac
done

if [[ -z "$REPO" || ! -d "$REPO" ]]; then
  echo "Set PDF2MD_REPO or pass --repo" >&2
  exit 1
fi

# shellcheck disable=SC1091
source "${REPO}/.venv/bin/activate"

if ! command -v mineru-models-download >/dev/null 2>&1; then
  echo "mineru-models-download not found. Run install.sh first." >&2
  exit 1
fi

echo "Downloading MinerU models (may take a while)..."
mineru-models-download -s huggingface -m all || mineru-models-download -m all
echo "Model download complete."
