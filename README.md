# Satisfactory Blueprints

A version-controlled Satisfactory blueprint library, linked into the game so
that saving a blueprint in game writes a file here.

## Layout

```
_master/            the blueprint library, one copy of each
_variants/          alternate versions of blueprints that share a name
scripts/            tools for linking, deduplicating and converting blueprints
docs/               documentation, including the tooling plan (docs/PLAN.md)
archive/            old and set-aside blueprints (local, not tracked)
links.conf.example  template for the local session mapping (links.conf)
```

Each blueprint is a `.sbp` file paired with a `.sbpcfg`. Both are required; a
`.sbp` without its partner does not appear in game.

## Scripts

| Script | Purpose |
|---|---|
| `scripts/link-blueprints.sh` | Links game sessions to folders in this repo ([docs](docs/session-links.md)) |
| `scripts/dedupe-blueprints.sh` | Collapses per-session copies into `_master` and `_variants` ([docs](docs/session-links.md#deduplicating)) |
| `scripts/bp-convert/` | Converts blueprints between normal, PEP/Skyblock and RWG maps ([docs](scripts/bp-convert/README.md)) |

Close Satisfactory before running any of them; the game rewrites the blueprint
folders when it exits.

## Notes

`.gitattributes` marks `.sbp` and `.sbpcfg` as binary so git does not apply
line-ending conversion, which would corrupt them.
