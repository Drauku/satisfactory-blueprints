#!/usr/bin/env bash
# Links per-session blueprint folders from this repo into Satisfactory's Proton prefix.
# Satisfactory reads blueprints from blueprints/<session name>/, so each mapped
# session gets its own symlink rather than the whole directory being shared.
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SAVED="/mnt/games/SteamLibrary/steamapps/compatdata/526870/pfx/drive_c/users/steamuser/AppData/Local/FactoryGame/Saved/SaveGames"
LINKDIR="$SAVED/blueprints"
CONF="$REPO/links.conf"

die() { echo "$*" >&2; exit 1; }

game_running() { pgrep -f "FactoryGameSteam-Win64-Shipping" >/dev/null 2>&1; }

# Session names are stored either as a sessionName= token or as a bare string
# following the save name in the header. Both forms are best-effort.
discover_sessions() {
    find "$SAVED" -maxdepth 2 -name '*.sav' -print0 2>/dev/null | while IFS= read -r -d '' f; do
        head -c 1200 "$f" | strings -n 3 | grep -oE 'sessionName=[^?]+' | head -1 | cut -d= -f2
    done | sort -u | grep -v '^$'
}

cmd_list() {
    echo "Sessions found in saves:"
    discover_sessions | sed 's/^/  /' || true
    echo
    echo "Blueprint folders in this repo:"
    find "$REPO" -mindepth 1 -maxdepth 1 -type d -not -name '.git' -printf '  %P\n' | sort
    echo
    echo "Links currently in the prefix:"
    if [[ -d "$LINKDIR" && ! -L "$LINKDIR" ]]; then
        find "$LINKDIR" -mindepth 1 -maxdepth 1 -printf '  %P -> %l\n' 2>/dev/null | sort
    else
        echo "  (blueprints/ is not a directory)"
    fi
}

cmd_apply() {
    game_running && die "Satisfactory is running. Close it; the game rewrites this directory on exit."
    [[ -d "$SAVED" ]] || die "Save directory not found: $SAVED"$'\n'"Launch the game once to recreate the prefix."
    [[ -f "$CONF" ]] || die "Missing config: $CONF"

    # A whole-directory symlink or a stray file here blocks per-session links.
    if [[ -L "$LINKDIR" ]]; then
        rm -f "$LINKDIR"
        echo "Removed whole-directory symlink at blueprints/"
    fi
    mkdir -p "$LINKDIR"

    local linked=0 skipped=0
    while IFS= read -r line; do
        line="${line%%#*}"
        [[ "$line" =~ ^[[:space:]]*$ ]] && continue
        [[ "$line" == *=* ]] || { echo "  skip (no '='): $line"; continue; }

        local session folder
        session="${line%%=*}"; folder="${line#*=}"
        session="$(echo "$session" | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')"
        folder="$(echo "$folder"  | sed 's/^[[:space:]]*//; s/[[:space:]]*$//')"
        [[ -n "$session" && -n "$folder" ]] || continue

        local target="$REPO/$folder" link="$LINKDIR/$session"
        if [[ ! -d "$target" ]]; then
            echo "  MISSING folder '$folder' for session '$session'"
            skipped=$((skipped+1)); continue
        fi

        # A real directory here holds blueprints saved while the link was absent.
        if [[ -d "$link" && ! -L "$link" ]]; then
            echo "  rescuing blueprints from real dir: $session"
            rsync -rlt --ignore-existing "$link/" "$target/"
            mv "$link" "$link.replaced-$(date +%Y%m%d-%H%M%S)"
        fi

        [[ -L "$link" ]] && rm -f "$link"
        ln -s "$target" "$link"
        echo "  $session -> $folder ($(find "$target" -name '*.sbp' | wc -l) blueprints)"
        linked=$((linked+1))
    done < "$CONF"

    echo "Linked $linked session(s); $skipped skipped."
}

case "${1:-apply}" in
    --list|list) cmd_list ;;
    --apply|apply) cmd_apply ;;
    *) echo "Usage: $(basename "$0") [--apply|--list]" >&2; exit 2 ;;
esac
