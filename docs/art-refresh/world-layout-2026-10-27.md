# Connected-world layout redesign

Existing shop identities, interiors, recipes, services and progression remain.
Emberhold is organized around a civic square, workshop and market streets, and
a south-facing gate forecourt. The trade road turns outside its walls rather
than running through the building plots. Frostmere retains a larger courtyard;
Saltmere uses two coastal terraces; Cragmoor has a compact mountain courtyard.
Their different space constraints no longer compress one shared grid. Town wall
widths and collision use the same bounds, and shop approaches connect to paving
at the visible down-screen threshold.

The wilderness keeps existing terrain, bridges, encounters and housing locations.
A bounded multi-source search creates secondary walking trails from the existing
roads to gathering spots, shrines, dungeon approaches and the outside of housing
plots. Every segment is checked against the movement terrain; trails avoid water,
ridges, house footprints and fortified camps. Main roads have quieter earth
shoulders and surfaces; town streets are painted as a complete union before stone
joints, preventing outlined overlap rings at junctions.

Six newly generated entrance sprites replace the repeated archway presentation:
a burial crypt, webbed natural cavern, flooded temple ruin, volcanic cave,
frozen barrow and timbered mine. The RGBA atlas is 1536x1024 with six 512px cells,
loaded once and sampled as rectangles without six separate texture allocations.
The 2D entrance threshold faces south to match the artwork; collision, walk-in,
E interaction and exit placement all use the same entrance helpers. Original
entrance art remains archived. No save migration is required.

Validation includes original building keys, compound bounds, spacing, invertible
coordinates, a movement-footprint flood from every town gate to every shop,
reachable dungeon thresholds, preserved home locations, safe trail segments,
existing homestead/save checks and desktop/phone browser validation.
