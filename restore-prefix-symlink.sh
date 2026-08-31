#!/usr/bin/env bash
# Points Satisfactory's blueprint directory at this repo.
# A Proton prefix rebuild replaces the symlink with a real directory; re-running restores it.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SAVED="/mnt/games/SteamLibrary/steamapps/compatdata/526870/pfx/drive_c/users/steamuser/AppData/Local/FactoryGame/Saved/SaveGames"
LINK="$SAVED/blueprints"

if pgrep -f "FactoryGameSteam-Win64-Shipping" >/dev/null 2>&1; then
    echo "Satisfactory is running. Close it first; the game rewrites this directory on exit." >&2
    exit 1
fi

if [[ ! -d "$SAVED" ]]; then
    echo "Save directory not found: $SAVED" >&2
    echo "Launch the game once to recreate the prefix, then re-run this script." >&2
    exit 1
fi

if [[ -L "$LINK" && "$(readlink -f "$LINK")" == "$REPO" ]]; then
    echo "Already linked: $LINK -> $REPO"
    exit 0
fi

# A real directory here holds blueprints the game wrote while the link was absent.
if [[ -d "$LINK" && ! -L "$LINK" ]]; then
    echo "Real directory present; copying blueprints it holds into the repo."
    rsync -rlt --ignore-existing "$LINK/" "$REPO/"
    STASH="$LINK.replaced-$(date +%Y%m%d-%H%M%S)"
    mv "$LINK" "$STASH"
    echo "Previous directory kept at: $STASH"
fi

[[ -L "$LINK" ]] && rm -f "$LINK"
ln -s "$REPO" "$LINK"
echo "Linked: $LINK -> $REPO"
echo "Blueprints now tracked: $(find "$REPO" -name '*.sbp' | wc -l)"
