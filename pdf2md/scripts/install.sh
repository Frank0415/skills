#!/usr/bin/env bash
# Install pdf2md CLI, MinerU stack, and model weights for the detected platform.
set -euo pipefail

SKILL_SCRIPTS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="${PDF2MD_REPO:-}"

PASS_ARGS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo)
      REPO="$2"
      shift 2
      ;;
    *)
      PASS_ARGS+=("$1")
      shift
      ;;
  esac
done

if [[ -z "$REPO" || ! -d "$REPO" ]]; then
  cat >&2 <<'EOF'
pdf2md install requires the git clone path.

  export PDF2MD_REPO=/path/to/pdf2md
  ~/.agents/skills/pdf2md/scripts/install.sh

  # or
  ~/.agents/skills/pdf2md/scripts/install.sh --repo /path/to/pdf2md
EOF
  exit 1
fi

export PDF2MD_REPO="$REPO"

if ! command -v uv >/dev/null 2>&1; then
  echo "uv is required. Install: https://docs.astral.sh/uv/" >&2
  exit 1
fi

if ((${#PASS_ARGS[@]})); then
  bash "${SKILL_SCRIPTS}/detect-platform.sh" --repo "$REPO" "${PASS_ARGS[@]}"
else
  bash "${SKILL_SCRIPTS}/detect-platform.sh" --repo "$REPO"
fi

MANIFEST="${REPO}/.pdf2md/platform.json"
if [[ ! -f "$MANIFEST" ]]; then
  echo "platform.json not found after detection" >&2
  exit 1
fi

read_manifest() {
  python3 -c "import json,sys; d=json.load(open('${MANIFEST}')); print(d.get(sys.argv[1],''))" "$1"
}

PROFILE="$(read_manifest profile)"
GPU_BACKEND="$(read_manifest gpu_backend)"
TORCH_INDEX="$(read_manifest torch_index)"
INFERENCE="$(read_manifest inference)"
MINERU_EXTRAS="$(read_manifest mineru_extras)"

echo "Installing for profile: $PROFILE"

bash "${SKILL_SCRIPTS}/install-system-deps.sh" || true

cd "$REPO"
if [[ ! -d .venv ]]; then
  uv venv .venv
fi
# shellcheck disable=SC1091
source .venv/bin/activate

uv pip install -U pip

case "$PROFILE" in
  mac_arm)
    uv pip install -U "mineru[core,mlx]"
    ;;
  linux_gpu)
    if [[ -n "$TORCH_INDEX" ]]; then
      uv pip install -U torch torchvision --index-url "https://download.pytorch.org/whl/${TORCH_INDEX}"
    fi
    uv pip install -U "mineru[core]"
    uv pip uninstall -y vllm lmdeploy 2>/dev/null || true
    ;;
  linux_cpu)
    uv pip install -U "mineru[core]"
    uv pip uninstall -y vllm lmdeploy 2>/dev/null || true
    ;;
  *)
    echo "Unknown profile: $PROFILE" >&2
    exit 1
    ;;
esac

uv pip install 'transformers>=4.49,<5'
uv pip install -e .

bash "${SKILL_SCRIPTS}/download-models.sh" --repo "$REPO" || true

BIN_DIR="${HOME}/.local/bin"
mkdir -p "$BIN_DIR"
ln -sf "${REPO}/.venv/bin/pdf2md" "${BIN_DIR}/pdf2md"

echo ""
echo "Install complete."
echo "  profile:      $PROFILE"
echo "  gpu_backend:  $GPU_BACKEND"
echo "  inference:    $INFERENCE"
echo "  pdf2md:       ${BIN_DIR}/pdf2md"
echo "  repo:         $REPO"
echo "Run: pdf2md doctor"
