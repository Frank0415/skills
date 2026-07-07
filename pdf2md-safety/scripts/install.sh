#!/usr/bin/env bash
# Install pdf2md CLI for scan-only use (no MinerU / no model download).
set -euo pipefail

SKILL_SCRIPTS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="${PDF2MD_REPO:-}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo) REPO="$2"; shift 2 ;;
    *) shift ;;
  esac
done

if [[ -z "$REPO" || ! -d "$REPO" ]]; then
  cat >&2 <<'EOF'
pdf2md-safety install requires the git clone path.

  export PDF2MD_REPO=/path/to/pdf2md
  ~/.agents/skills/pdf2md-safety/scripts/install.sh
EOF
  exit 1
fi

export PDF2MD_REPO="$REPO"

if ! command -v uv >/dev/null 2>&1; then
  echo "uv is required. Install: https://docs.astral.sh/uv/" >&2
  exit 1
fi

bash "${SKILL_SCRIPTS}/install-system-deps.sh"

cd "$REPO"
if [[ ! -d .venv ]]; then
  uv venv .venv
fi
# shellcheck disable=SC1091
source .venv/bin/activate

uv pip install -U pip
uv pip install -e .

BIN_DIR="${HOME}/.local/bin"
mkdir -p "$BIN_DIR"
ln -sf "${REPO}/.venv/bin/pdf2md" "${BIN_DIR}/pdf2md"

echo ""
echo "pdf2md-safety install complete (scan-only; no MinerU)."
echo "  pdf2md: ${BIN_DIR}/pdf2md"
echo "For conversion + models, run: ~/.agents/skills/pdf2md/scripts/install.sh"
