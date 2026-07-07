#!/usr/bin/env bash
# Probe host and write $PDF2MD_REPO/.pdf2md/platform.json
# Profiles: mac_arm, linux_cpu, linux_gpu
set -euo pipefail

SKILL_SCRIPTS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="${PDF2MD_REPO:-}"

FORCE_CPU=0
REDETECT=0
NONINTERACTIVE=0

usage() {
  cat <<'EOF'
Usage: detect-platform.sh [--repo PATH] [--cpu] [--redetect] [--yes]

  --repo PATH  pdf2md repository root (or set PDF2MD_REPO)
  --cpu        Force linux_cpu even if NVIDIA GPU is present
  --redetect   Overwrite existing platform.json
  --yes        Non-interactive (abort on NVIDIA without CUDA)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo)
      REPO="$2"
      shift 2
      ;;
    --cpu) FORCE_CPU=1; shift ;;
    --redetect) REDETECT=1; shift ;;
    --yes) NONINTERACTIVE=1; shift ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done

if [[ -z "$REPO" || ! -d "$REPO" ]]; then
  echo "PDF2MD_REPO or --repo must point to the pdf2md git clone." >&2
  exit 1
fi

export PDF2MD_REPO="$REPO"
OUT_DIR="${REPO}/.pdf2md"
OUT_FILE="${OUT_DIR}/platform.json"

if [[ -f "$OUT_FILE" && "$REDETECT" -eq 0 ]]; then
  echo "platform.json exists (use --redetect to overwrite): $OUT_FILE"
  exit 0
fi

RAW_OS="$(uname -s)"
RAW_ARCH="$(uname -m)"
OS="$(echo "$RAW_OS" | tr '[:upper:]' '[:lower:]')"
ARCH="$RAW_ARCH"
[[ "$ARCH" == "aarch64" ]] && ARCH="arm64"

if [[ "$OS" != "darwin" && "$OS" != "linux" ]]; then
  echo "Unsupported OS: $RAW_OS (only macOS and Linux)" >&2
  exit 1
fi

if [[ "$OS" == "darwin" && "$ARCH" == "x86_64" ]]; then
  echo "Unsupported profile: macOS Intel (x86_64). Use Apple Silicon or Linux." >&2
  exit 1
fi

map_cuda_to_torch_index() {
  local ver="$1"
  local major minor
  major="${ver%%.*}"
  minor="${ver#*.}"
  minor="${minor%%.*}"
  if [[ "$major" -lt 11 ]]; then echo "cu118"; return; fi
  if [[ "$major" -eq 11 ]]; then echo "cu118"; return; fi
  if [[ "$major" -eq 12 && "$minor" -le 1 ]]; then echo "cu121"; return; fi
  if [[ "$major" -eq 12 && "$minor" -le 4 ]]; then echo "cu124"; return; fi
  if [[ "$major" -eq 12 ]]; then echo "cu126"; return; fi
  # Dynamic indexing for CUDA >= 13 (e.g., cu130, cu140)
  echo "cu${major}0"
}

detect_cuda_version() {
  local smi ver nvcc_ver
  if ! command -v nvidia-smi >/dev/null 2>&1; then
    return 1
  fi
  smi="$(nvidia-smi 2>/dev/null || true)"
  ver="$(echo "$smi" | sed -n 's/.*CUDA Version: \([0-9]\+\.[0-9]\+\).*/\1/p' | head -1)"
  if [[ -z "$ver" ]] && command -v nvcc >/dev/null 2>&1; then
    nvcc_ver="$(nvcc --version 2>/dev/null | sed -n 's/.*release \([0-9]\+\.[0-9]\+\).*/\1/p' | head -1)"
    ver="$nvcc_ver"
  fi
  if [[ -z "$ver" ]]; then
    return 1
  fi
  echo "$ver"
}

has_nvidia_gpu() {
  command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi >/dev/null 2>&1
}

prompt_cpu_fallback() {
  echo ""
  echo "NVIDIA GPU detected but CUDA runtime/toolkit not found."
  echo "Install NVIDIA driver + CUDA toolkit, or continue with CPU (linux_cpu / HF transformers)."
  echo ""
  if [[ "$NONINTERACTIVE" -eq 1 ]]; then
    echo "Non-interactive mode: re-run with --cpu or install CUDA first." >&2
    exit 1
  fi
  read -r -p "Continue with CPU? [y/N] " ans
  ans_lc="$(printf '%s' "$ans" | tr '[:upper:]' '[:lower:]')"
  if [[ "$ans_lc" == "y" || "$ans_lc" == "yes" ]]; then
    FORCE_CPU=1
  else
    echo "Aborted. Install CUDA or pass --cpu to force CPU profile." >&2
    exit 1
  fi
}

PROFILE=""
GPU_BACKEND=""
INFERENCE=""
MINERU_EXTRAS=""
CUDA_DETECTED="false"
CUDA_VERSION=""
TORCH_INDEX=""

if [[ "$OS" == "darwin" && "$ARCH" == "arm64" ]]; then
  PROFILE="mac_arm"
  GPU_BACKEND="mlx"
  INFERENCE="mlx"
  MINERU_EXTRAS='["core","mlx"]'
elif [[ "$OS" == "linux" ]]; then
  if [[ "$FORCE_CPU" -eq 1 ]]; then
    PROFILE="linux_cpu"
    GPU_BACKEND="cpu"
    INFERENCE="transformers"
    MINERU_EXTRAS='["core"]'
  elif has_nvidia_gpu; then
    if cuda_ver="$(detect_cuda_version)"; then
      PROFILE="linux_gpu"
      GPU_BACKEND="nvidia"
      INFERENCE="transformers"
      MINERU_EXTRAS='["core"]'
      CUDA_DETECTED="true"
      CUDA_VERSION="$cuda_ver"
      TORCH_INDEX="$(map_cuda_to_torch_index "$cuda_ver")"
    else
      prompt_cpu_fallback
      PROFILE="linux_cpu"
      GPU_BACKEND="cpu"
      INFERENCE="transformers"
      MINERU_EXTRAS='["core"]'
    fi
  else
    PROFILE="linux_cpu"
    GPU_BACKEND="cpu"
    INFERENCE="transformers"
    MINERU_EXTRAS='["core"]'
  fi
else
  echo "Unsupported platform: $OS $ARCH" >&2
  exit 1
fi

DETECTED_AT="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
mkdir -p "$OUT_DIR"

if [[ "$CUDA_DETECTED" == "true" ]]; then
  cat >"$OUT_FILE" <<EOF
{
  "profile": "$PROFILE",
  "os": "$OS",
  "arch": "$ARCH",
  "gpu_backend": "$GPU_BACKEND",
  "cuda_detected": true,
  "cuda_version": "$CUDA_VERSION",
  "torch_index": "$TORCH_INDEX",
  "mineru_extras": $MINERU_EXTRAS,
  "inference": "$INFERENCE",
  "detected_at": "$DETECTED_AT"
}
EOF
else
  cat >"$OUT_FILE" <<EOF
{
  "profile": "$PROFILE",
  "os": "$OS",
  "arch": "$ARCH",
  "gpu_backend": "$GPU_BACKEND",
  "cuda_detected": false,
  "mineru_extras": $MINERU_EXTRAS,
  "inference": "$INFERENCE",
  "detected_at": "$DETECTED_AT"
}
EOF
fi

echo "Wrote $OUT_FILE (profile=$PROFILE, inference=$INFERENCE)"
