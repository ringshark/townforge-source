# Guild foundation and dungeon release

Active source: `ringshark-patch-1`. Emscripten 4.0.20 and raylib 6.0 are pinned.
`RAYLIB_DIR=/path/to/raylib bash web/build_web.sh` compiles the 3D game;
`bash web/deploy_web.sh build-web site` packages it. The output names match the
shell and service worker (`townforge.js`, `.wasm`, `.data`, `index.html`).

The source workflow validates the client and publishes a deployable artifact.
The preview repository's `townforge.yml` polls this branch every five minutes,
builds only when `source-version.txt` differs, preserves cloud/multiplayer
configuration, and deploys through GitHub Pages. Pages must allow Actions
publishing. The actual deployed commit is readable at `source-version.txt`.

## Guild Stage One

Run `web/guild-stage-one.sql` in the existing Supabase project's SQL Editor.
For a fresh project, run the complete `web/supabase.sql` instead. The migration
is transactional and repeatable. Foundation orders require 20 timber loads,
12 ore loads, 8 tool sets and 3 supply contracts. Each guild member supplies one
order per UTC day; receipts prevent retries from duplicating contributions.
Officers start the one-hour Hall build; each member can help once, reducing its
original duration by 5%. Hall level 2 opens tier-two research and the quartermaster.
These are explicitly prototype, server-issued supplies. The local save is not
an authoritative multiplayer inventory; world-earned material donations and
validated boss objectives remain future work.

## Dungeon art

Walls retain full height and occlude actors. Removed camera-dependent vertex
lowering and all associated uniforms. Flagstones have sparse cracks and damp,
dark borders; masonry has restrained grain, bevels and darker lower courses.
Existing themed props, room/corridor layout and collision remain in use.

## Validation

Emscripten compilation and packaging passed locally. Guild client tests passed.
Embedded PostgreSQL validated rerunnable migration, permissions, per-day limits,
receipt replay, build help and completion. Runtime art review is still required.
