# Connected 2D outdoors — 2026-10-23

All four outdoor towns now occupy the wilderness coordinate system. Crossing an
arch keeps the wilderness screen, camera and player position; shop interiors and
dungeons retain their existing entrances and interior scenes. The 3D world stays
on its existing path.

Town layout positions are projected using `OpenTownWorld` / `OpenTownLocal`.
`InOutdoorTown` retains town services, tutorial return checks, navigation and NPC
panels without requiring a Town screen. Existing save fields and shop return
positions are reused. Town depth is bounded by the actual playable map; Saltmere
uses the coastal spacing constant to keep every building inside the boundary.

The outdoor presentation adds blended biome turf, deterministic foot-anchored
forest props, depth layering, ground shadows, water highlights and physical gate
towers. Scenery is culled to the camera and kept off roads, houses, gathering
nodes and dungeon entrances. Terrain and buildings still use existing collision.
The wilderness shares the native 960×540 landscape target, with visible movement,
interaction and combat controls. Town arrivals retain healing and restocking.

The 2D minimap no longer loads the 3D prop models. The accompanying startup fix
uses a growing 128 MB initial heap and skips 3D idle preparation.

Local validation: all four town transforms round-trip; buildings remain within
playable bounds; real gate and terrain collision permit both directions;
existing input, movement, combat, menu, save/variant and full landscape-page
checks pass. Browser release validation runs on Chromium and WebKit, including
phone navigation and real input for walking out of town and returning without a
screen switch or coordinate jump. Publication is gated on these browser checks.
