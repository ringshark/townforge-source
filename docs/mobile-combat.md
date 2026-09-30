# Mobile controls, Den movement, and Hiding cover

Eight spell shortcuts now use two rows of four 72-unit squares, with 12-unit
horizontal gaps. At a 390-pixel canvas width this gives 52-pixel touch targets.
Combat healing controls are 64 units; TARGET is separated from spells and the
movement stick. Keyboard assignments remain the same.

Den ground movement requires a genuine tap beginning on uncovered ground.
Joystick releases, drags returning to their origin, menu transitions and
release-only displacement cannot start an automatic walk. Manual movement uses
the Den camera orientation; tap targets retain world coordinates.

The pit has a symmetrical tiled court, contrasting marks and unobstructed
center. A stable duel camera follows the arena center. Strike, Lunge and Guard
are 98 by 84 units, and surrender is placed on its own row.

Hiding skill 0 through 30 fails in open ground. Above 30 the open-ground chance
rises by 1.3 percentage points per skill point (91% at 100). Nearby cover starts
at 20% and reaches 97% at 100. Nearby hostiles reduce either chance by 30 points.
Trees and tree clusters in the wilderness, wood gathering trees, and dungeon
walls within 60 units provide cover. Failure tells the player to seek a tree or
wall. Existing cooldown, skill training, Stealth and reveal rules still apply.

Arena reference: https://wiki.uooutlands.com/Arena_%26_Duels
Official footage: https://uooutlands.com/news/video-eleventh-official-1v1-arena-tournament/
Video playback was blocked during this pass; the court uses official arena
references rather than a claimed frame-by-frame analysis.

Checks: tests/mobile-combat.py compiles the production geometry, gesture and
Hiding chance helpers. tests/player-movement.py checks actual movement and Den
camera rotation, alongside the existing UI, combat, guild and ladder tests.
