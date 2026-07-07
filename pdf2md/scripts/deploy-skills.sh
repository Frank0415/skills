#!/usr/bin/env bash
# Copy pdf2md skill packages into ~/.agents/skills/
set -euo pipefail

SKILL_SCRIPTS="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKILL_ROOT="$(cd "${SKILL_SCRIPTS}/.." && pwd)"
REPO="$(cd "${SKILL_ROOT}/.." && pwd)"
DEST="${AGENTS_SKILLS_DIR:-${HOME}/.agents/skills}"

mkdir -p "$DEST"

for name in pdf2md pdf2md-safety; do
  src="${REPO}/${name}"
  if [[ ! -d "$src" ]]; then
    echo "missing skill source: $src" >&2
    exit 1
  fi
  rm -rf "${DEST}/${name}"
  cp -R "$src" "${DEST}/${name}"
  echo "installed ${DEST}/${name}"
done

echo ""
echo "Skills deployed. Set PDF2MD_REPO and run install:"
echo "  export PDF2MD_REPO=${REPO}"
echo "  ${DEST}/pdf2md/scripts/install.sh"
echo "  ${DEST}/pdf2md-safety/scripts/install.sh"
