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

## Two games from one source (2026-10-19)

Mark asked for a 2D version and a 3D version. They are the same source built twice with
`TF_VARIANT` (`web/build_web.sh`, `web/deploy_web.sh`):

| `TF_VARIANT` | Game | Output |
|---|---|---|
| `2d` | the classic top-down game only (`kVariantView == 2`, `-DTF_VARIANT_2D`) | `build-web-2d` |
| `3d` | the 3D game only (`kVariantView == 3`, `-DTF_VARIANT_3D`) | `build-web-3d` |
| empty | the old combined build: 2D by default with the V key and the Options row to switch | `build-web` |

In a variant the view is forced in the one place it is derived each frame, and the switch
(the V key and the "Classic 2D view" Options row) does not exist. The 3D site is named
"Town Forge 3D" (home-screen name and page title); the 2D site stays "Town Forge".
The source workflow builds and packages both (`townforge-web-2d`, `townforge-web-3d`
artifacts). Publishing: the 3D site is `ringshark/townforge-3d-preview` (its `townforge.yml`
must set `TF_VARIANT=3d` for the compile and package steps); the 2D site is `ringshark/townforge`
with its own workflow of the same shape and `TF_VARIANT=2d`.

Both sites are on one web address, so the browser shares their storage:
- **Saves** (`/persist/townforge_save.txt` in IndexedDB, plus the cloud and guild keys in
  localStorage) are shared: a character made in one site opens in the other. Do not run both
  at once (each autosaves every 2 s; the last write wins). An installed iPhone home-screen app
  keeps its own storage apart from Safari.
- **Cache storage** is shared too, so each service worker names its caches `townforge-<variant>-<version>`
  and only deletes its own (the 3D one also clears the old untagged `townforge-<timestamp>-<commit>`
  caches). `tests/build-variants.cjs` checks this.

The game data is still the whole `assets` folder in every variant (about 93 MB). Slimming the 2D data
pack (leaving out the 3D models and art) needs a test that the 2D game never asks for them first.

### Backup code

Options (web builds) has "Back up my save" and "Restore from a backup". The code is
`TFSAVE1:` + a checksum + the save file as base64, copied to the clipboard (or shown in a box to
copy by hand). Restoring checks the format, the checksum and the `version=` header, asks to
confirm, keeps the replaced save as `townforge_save.prev.txt`, blocks the autosave, writes the
save, syncs IndexedDB and reloads. It carries a character between phones, browsers, an installed
app and Safari, and between the 2D and 3D sites, and survives Safari clearing a site's data.

## Roads, walled towns and a larger hero in the connected 2D world (2026-10-26)

Mark: "the roads look horrible, in town and in the wilderness; the towns should have 4 walls with a gate; the sprite should be larger, maybe 2x".

- **Camera.** Outdoors the world is drawn at `g_outdoorZoom` (1.5) and people and creatures at `kOutdoorActorBoost` (1.34) on top, so the hero is about twice his old size on screen (34 px became ~68). `DrawWildernessScreen` sets `kViewport` to the part of the world that fits, wraps the world pass in a `Camera2D`, and scales the mouse back (`SetMouseScale`) so tapping a monster still finds it; the HUD is drawn afterwards at the normal scale. Interiors and the 3D view are untouched.
- **Towns face a compass direction.** `TownGateFacing` snaps the road direction to N/E/S/W (scored against water, ridge and the world edge behind the gate), so each town is a plain square compound with four walls. `OpenTownWalls` (open_world_2d.h) draws them: wall pieces sorted against the hero, crenellated, corner and mid-side towers, gate towers either side of the archway, a lintel on north/south gates and pennants. The 2D archway is wider than the 3D one (`GateCollide` uses the 2D numbers when `g_openWorld2D`): towers at 76 and radius 26 leave a 66-unit passage for the hero.
- **Roads** (`world_roads_2d.h`, namespace `tfroad`). Wilderness roads are soft-edged dirt ribbons (verge that melts into the grass, worn centre, wheel ruts, pebbles and tufts, an edge that wanders) drawn from `OpenRoadNetwork()`: `g_wtRoads` with every stretch that lay inside a town's yard cut out and led round the walls to the front of the gate. Town streets are kerbed cobble rectangles drawn as one network (kerbs first, then the cobble) with a gate apron. Scenery keeps clear of the roads as drawn. The atlas map uses the same network.
- **Art fixes.** Cragmoor's buildings sat on a pale blue backdrop (white boxes in the game): `KeyBorderBackdrop` clears it at load. The shore/water overlay is 2048 px in 2D (it was blurry zoomed in) and bridges are drawn as planked decks with rails.
- **Dev tool.** Compile with `-DTF_SHOTS` (and `-DTF_HOUR=13` to pin the daylight) and run with `TF_SHOTS="name:x:y;name:x:y"` to teleport over the wilderness and save `name.png` for each (`TF_ZOOM=0.17` shows the whole world, `TF_INTERIOR=smith` an interior). It does nothing in a normal build.
- **Tests.** `tests/open-world.cpp` now also checks that every town faces a compass direction with open ground behind it, that no road runs through a yard, and that the archway passes the hero while the towers and walls block him.
- **Not done.** Building interiors still draw the hero at his old size in a portrait-shaped room; the in-town buildings are the existing art, only re-laid onto the new streets.
