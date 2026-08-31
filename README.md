# Satisfactory Blueprints

A version-controlled blueprint library, linked into Satisfactory so that saving
a blueprint in-game writes a file here.

Steam Cloud syncs Satisfactory saves but not blueprints, so this repository is
the backup for them.

## Layout

```
_master/     the blueprint library, one copy of each
_variants/   alternate versions of blueprints that share a name
```

Each blueprint is a `.sbp` file paired with a `.sbpcfg`. Both are required — a
`.sbp` without its partner does not appear in-game.

## Usage

Satisfactory reads blueprints from `SaveGames/blueprints/<session name>/`, so
blueprints are scoped to a session. `links.conf` maps each session to a folder
here, and several sessions may share one folder:

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
./link-blueprints.sh --apply
```

`--list` shows the sessions found in your saves, the folders available here, and
the links currently in place. Both are safe to run repeatedly.

Close Satisfactory first. The game caches the blueprint list at load and
rewrites the directory on exit.

## After a Proton update

Steam owns the prefix, so a Proton change or a prefix rebuild removes the
symlinks. Re-run `./link-blueprints.sh --apply` to restore them.

## Other scripts

`dedupe-blueprints.sh` collapses per-session folders into `_master` and
`_variants`. It is only needed if per-session copies reappear, such as when
importing a library from another machine.

## Notes

`.gitattributes` marks `.sbp` and `.sbpcfg` as binary so git does not apply
line-ending conversion, which would corrupt them.
