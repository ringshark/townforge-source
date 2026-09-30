# Town trees and fountain water

Reviewed the current 3D source at `52585d76` on `ringshark-patch-1`.
The detailed characters, glazed architecture and earlier lighting improvements
make the simple tree crowns and still fountain water the strongest next art
targets. Existing root flares, scenery shadows and fountain ripple overlays
are retained.

## Changes

- Five cached town tree meshes replace the simple Nature Kit tree shapes.
  Broadleaves have forked branches, staggered leaf masses, face color variation
  and darker undersides. Pine skirts overlap at five heights. Each replacement
  remains one mesh/material and uses the existing scale, rotation and wind shader.
  Heights and horizontal extents stay within the previous trees' approximate bounds.
  Bushes and the separate KayKit wilderness tree meshes retain their existing art.
- Wind height is measured from each instance's base, keeping hillside trunks
  planted instead of making elevation contribute to wind deformation.
- The fountain has recessed basin water, an open upper bowl, a stone rim and six
  curved spill streams with impact foam. All water geometry is one cached mesh.
  A WebGL1 shader animates ripple normals and stream highlights, adds directional
  sun glints and sky reflection, and follows the existing day/night light and fog.
  Failed shader loading falls back to the shared lit shader.
- Existing ripple overlays fade in/out with depth writes disabled during their
  draws. The opaque water surface uses normal depth testing/writes.

Town layout, collision, picking, characters, multiplayer and save data are unchanged.
The geometry builders are included from `tools/town_tree_mesh.inc` and
`tools/town_fountain_water.inc`; keep these beside the source when building.

## Validation

- C++17 syntax check against raylib 6.0 headers passes when the two pre-existing
  undeclared desktop functions (`JS_CloudOpen` and `CleanupAndClose`) are declared
  by a temporary check header. No implementations or game source fixes for those
  unrelated build issues are included.
- All six shader pairings in `tests/shader-smoke.cjs` compile/link in WebGL1,
  including `lit.vs` with the new `fountain.fs`.
- The exact geometry builders were compiled/executed in a temporary CPU harness.
  Vertex attribute lengths, finite positions and unit normals passed checks.
  Broadleaf variants use 660 triangles each, pine 154; fountain water uses 864.
- The resulting meshes were rendered with the actual game shaders in a WebGL1
  harness with depth testing and backface culling. Both animation times (0 and
  1.2 seconds) produced no WebGL errors. Tree silhouettes, basin coverage, stream
  connections and foam placement were visually inspected.

This is source validation and isolated art rendering, not a rebuilt full-game
or physical mobile performance test. Rebuild the C++/Emscripten game and deploy
its generated files through the normal deployment process to update the live game.
