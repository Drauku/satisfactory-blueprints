#!/usr/bin/env bash
# Collapses per-session blueprint copies into _master, one file per blueprint name.
# Where a name has several distinct contents, the newest becomes the master copy
# and the others are kept under _variants/<name>/ so no version is discarded.
set -euo pipefail

# This script lives in scripts/; the blueprint folders are at the repo root.
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MASTER="$REPO/_master"
VARIANTS="$REPO/_variants"

# Folders that never hold library blueprints: git data, tooling, docs, and archived copies.
EXCLUDE=(-not -path "$REPO/.git/*" -not -path "$REPO/scripts/*" -not -path "$REPO/docs/*"
         -not -path "$REPO/archive/*" -not -path "$MASTER/*" -not -path "$VARIANTS/*")

mkdir -p "$MASTER" "$VARIANTS"

mapfile -t NAMES < <(find "$REPO" -name '*.sbp' "${EXCLUDE[@]}" -printf '%f\n' | sort -u)

echo "Distinct blueprint names: ${#NAMES[@]}"
divergent=0

for name in "${NAMES[@]}"; do
    mapfile -t paths < <(find "$REPO" -name "$name" "${EXCLUDE[@]}" -printf '%T@\t%p\n' | sort -rn | cut -f2-)

    newest="${paths[0]}"
    cp -p "$newest" "$MASTER/$name"
    cfg="${newest%.sbp}.sbpcfg"
    [[ -f "$cfg" ]] && cp -p "$cfg" "$MASTER/$(basename "$cfg")"

    # Distinct contents beyond the master copy are preserved for review.
    master_hash="$(md5sum "$newest" | cut -d' ' -f1)"
    seen="$master_hash"
    for p in "${paths[@]:1}"; do
        h="$(md5sum "$p" | cut -d' ' -f1)"
        [[ "$seen" == *"$h"* ]] && continue
        seen="$seen $h"
        vdir="$VARIANTS/${name%.sbp}"
        mkdir -p "$vdir"
        src_folder="$(basename "$(dirname "$p")")"
        cp -p "$p" "$vdir/${src_folder}-${h:0:8}.sbp"
        vcfg="${p%.sbp}.sbpcfg"
        [[ -f "$vcfg" ]] && cp -p "$vcfg" "$vdir/${src_folder}-${h:0:8}.sbpcfg"
        divergent=$((divergent+1))
    done
done

echo "Master blueprints: $(find "$MASTER" -name '*.sbp' | wc -l)"
echo "Variants preserved: $divergent across $(find "$VARIANTS" -mindepth 1 -maxdepth 1 -type d | wc -l) names"
