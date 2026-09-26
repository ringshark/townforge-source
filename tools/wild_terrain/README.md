# Wilderness terrain layout tool

Generates the river/ridge data in `main.cpp` (search for "Generated wilderness
terrain data"). Rivers and rock ridges are routed around every gameplay
position (spots, entrances, gates, shrines, house plots), and the tool checks
that everything stays reachable from the Emberhold gate.

Requires Python 3 with `numpy` and `Pillow`.

```
cd tools/wild_terrain
python3 extract_points.py   # read positions from main.cpp -> wildpts.json
python3 build.py            # relax paths (cached in relaxed.json), validate, write map.png
python3 export_cpp.py       # write terrain_data.inc (paste over the generated block)
```

- Edit `spec.py` to move rivers, ridges, roads, fords, the lake or the coast.
- `build.py` prints `UNREACHABLE` / `NEAR` lists. Both must be empty.
- `map.png` shows the layout, with gameplay spots and any conflicts circled.
- Run `export_cpp.py` and paste `terrain_data.inc` over the generated block
  in `main.cpp`.
- Keep `spec.py`'s coast and lake numbers in sync with `WildCoastX` and the
  `kWildLake*` constants in `main.cpp`.

If you move gameplay spots or house plots, re-run the tool so rivers and
ridges don't end up on top of them. The in-game collision always keeps
players out of water and ridges, so a missed spot would be unreachable, not
broken.
