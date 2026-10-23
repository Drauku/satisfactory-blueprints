# Session links

Satisfactory reads blueprints from `SaveGames/blueprints/<session name>/`, so
blueprints are scoped to a session. `scripts/link-blueprints.sh` replaces each
session folder in the Proton prefix with a symlink to a folder in this repo, so
saving a blueprint in game writes the file here.

Steam Cloud syncs Satisfactory saves but not blueprints, so this repository is
the backup for them.

## Mapping sessions to folders

`links.conf` at the repo root maps each session to a folder, and several
sessions may share one folder:

```
<session name> = <repo folder>
```

`links.conf` is local and not tracked, since session names differ per machine.
Start from the template:

```bash
cp links.conf.example links.conf
```

Apply the mapping:

```bash
scripts/link-blueprints.sh --apply
```

`--list` shows the sessions found in your saves, the blueprint folders here, and
the links currently in place. Both are safe to run repeatedly.

If a session already has a real folder in the prefix (blueprints saved while
the link was missing), `--apply` copies any blueprints not already in the repo
folder into it, then moves the old folder to `archive/<session>.replaced-<date>`.

Close Satisfactory first. The game caches the blueprint list at load and
rewrites the directory on exit.

## Map types

Blueprints saved in one kind of map lose their wires and some belt connections
in another kind, because each map type uses a different level name inside the
blueprint (vanilla and SatisfactoryPlus: `Persistent_Level`; PEP and Skyblock:
`SkyblockLevel`; Random World Generator: `ProceduralWorldLevel`). Sessions of
different map types should not share a folder unless the blueprints are
converted; see [`scripts/bp-convert`](../scripts/bp-convert/README.md).

## After a Proton update

Steam owns the prefix, so a Proton change or a prefix rebuild removes the
symlinks. Re-run `scripts/link-blueprints.sh --apply` to restore them.

## Deduplicating

`scripts/dedupe-blueprints.sh` collapses per-session folders into `_master` and
`_variants`. Where a blueprint name has several distinct contents, the newest
goes to `_master` and the others to `_variants/<name>/`. It skips `scripts/`,
`docs/` and `archive/`, and is only needed if per-session copies reappear, such
as when importing a library from another machine.
