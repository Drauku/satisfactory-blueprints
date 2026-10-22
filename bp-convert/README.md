# convert-blueprints

Satisfactory stamps every object and connection inside a blueprint with the level name of the
map it was saved in, and does not translate that name when the blueprint is placed in a
different kind of map. All power wires and some belt and lift connections then come up
disconnected. Belts and lifts whose ends sit exactly on a port can still snap back, which is
why the breakage looks inconsistent.

| Map type | Level name | File-name prefix | Option |
|---|---|---|---|
| Vanilla, SatisfactoryPlus | `Persistent_Level` | none | `--to normal` |
| Platform Expansion Program, Skyblock | `SkyblockLevel` | `PEP - ` | `--to pep` |
| Random World Generator | `ProceduralWorldLevel` | `RWG - ` | `--to rwg` |

The script detects each blueprint's current map type from its contents, rewrites the level
name, and swaps the prefix to match the target (for example `PEP - X` to `--to rwg` gives
`RWG - X`). The rest of the name keeps its original capitalization. Another map type needs
one entry in the `FORMATS` table at the top of the script.

## Usage

    npm install
    node convert-blueprints.mjs "<blueprint folder>" --to normal|pep|rwg [--replace] [--dry-run] [name ...]

- `name ...` limits the run to those blueprints (file name without `.sbp`).
- `--dry-run` lists what would change and writes nothing.
- Without `--replace` the converted blueprints are written next to the originals.
- With `--replace` the originals move to `<blueprint folder>-backup-before-<target>`.
- A blueprint that keeps its name is overwritten in place and is always backed up first.
- The script lists everything it will do and asks before writing. Close Satisfactory first;
  the game rewrites blueprint folders when it exits.

## Safety checks

- Every converted blueprint is parsed again before anything is written. It must keep the same
  object count, use only the target level name, and have no more dangling references than before.
- Blueprints already in the target format and name, with mixed level names, or whose target
  name already exists (compared without case, as Windows does) are skipped and reported.
- The backup step refuses to overwrite files already in the backup folder.

## Tested

- Normal, PEP and RWG blueprints placed in mismatched maps lose all wires and some belts;
  converted to the matching format they connect fully (normal and PEP tested both ways,
  RWG tested from RWG into a normal map).
- normal -> PEP -> RWG -> normal reproduces the original files byte for byte.

Uses `@etothepii/satisfactory-file-parser` 4.1.2.
