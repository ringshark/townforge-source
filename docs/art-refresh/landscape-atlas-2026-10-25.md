# Landscape world atlas — 2026-10-25

The 2D full map now uses the native 960×540 landscape canvas. It stays open
while selecting places or dragging, rather than dismissing on every tap. The
atlas has bounded panning, anchored wheel zoom, large zoom/overview/player
controls, and Places, Supplies and Homes filters. Locations come from the
existing town, dungeon, shrine, gathering, housing and danger-landmark data.

A selected destination reports distance and direction. Setting a waypoint
adds a bearing marker to the minimap and a live destination strip in the
world. It does not teleport or automatically move the character. Waypoints
and map view state are transient; existing save formats remain unchanged.
Character movement is held while the full map owns input.

The terrain bake uses 1024px RGBA data with muted biome colors, fine ground
variation, forest marks and bordered roads. Bounded CPU strokes avoid image
corruption while painting short or clipped road segments. The world terrain,
collision and existing town/dungeon connections are unchanged.

Checks cover projection round trips, zoom anchoring and limits, pan bounds,
clipped/zero-length raster strokes, existing movement and interface behavior,
and actual Chromium/WebKit atlas controls on desktop and phone viewports.
Browser checks also inspect northern snow and Mirrormere water colors to
catch map raster corruption. Publication follows successful browser checks
and screenshot review.
