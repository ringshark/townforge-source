# Compact interiors and framed building mattes

The screenshot reported a real Cragmoor loader defect. Its pale export frame
surrounded a solid pale background; clearing only alpha or white pixels directly
on the edge missed the opaque interior. `CleanExteriorMatte` traverses connected
transparent margins, pale tinted frames and their baked matte. Enclosed white
architecture remains intact. This loader is used for Cragmoor and Frostmere.

The audit now reports all ten raw framed mattes rather than incorrectly claiming
the source PNGs are clean. The native regression test decodes all ten actual PNGs,
checks the reported inset pixel becomes transparent and checks building silhouette
retention. A synthetic framed export also verifies enclosed white is preserved.
Browser checks read back the actual GPU textures for all ten buildings at desktop
and phone viewport sizes (twenty checks), alongside rendered town screenshots.

Authored 2D rooms now use 340–420 unit footprints rather than a universal 520x620
hall: about 45–64% less floor area. Furniture and characters retain their sizes.
Positions, windows, shopkeepers, doors and entrance points follow each smaller
plan. Existing custom homestead plans and their placed furniture retain their
layout. The 3D room plans are unchanged.

Production collision-footprint flood checks verify a clear spawn, bounded blockers
and reachable exit, stations and keeper in all twelve standard rooms across four
town variants. The browser review covers four towns, six dungeons and thirteen
interiors at desktop and phone viewport sizes, including live room movement.

The review scene setter and GPU readback helpers are not in the shipping build.
