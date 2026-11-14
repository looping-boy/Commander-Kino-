#!/usr/bin/env bash
# Vérifie que la logique de roulette de la télécommande est identique à celle
# de la veste. Usage : test/check_sync.sh <chemin vers LedVestProto2>
set -euo pipefail
here="$(cd "$(dirname "$0")/.." && pwd)"
vest="${1:?Donne le chemin du dépôt LedVestProto2}/LedCoatAnim/Roulette.h"
remote="$here/CommanderKino/RouletteLogic.h"
extract_vest() { sed -n '/^#include <stdint.h>/,/^#ifdef ARDUINO/p' "$1" | sed '$d'; }
extract_remote() { sed -n '/^#include <stdint.h>/,/^#endif  \/\/ ROULETTE_LOGIC_H/p' "$1" | sed '$d'; }
if diff <(extract_vest "$vest") <(extract_remote "$remote") >/dev/null; then
  echo "OK : la télécommande et la veste ont la même logique de roulette."
else
  echo "DIFFÉRENT : recopie la logique de $vest dans $remote." >&2
  diff <(extract_vest "$vest") <(extract_remote "$remote") >&2 || true
  exit 1
fi
