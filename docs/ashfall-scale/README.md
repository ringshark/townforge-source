# Ashfall scale and natural walking arms

Read-only reference: the current `ringshark/Ashford-survival` main branch uses
96-pixel sprite cells in a 1152x648 viewport. Its usual Bigfoot walk silhouette
is about 121/128 of that cell, or approximately 91 logical pixels high. Matching
that fraction of the screen in Town Forge's 960x540 view gives a 76-pixel player.
The extra 15% interior multiplier is removed so native indoor and outdoor views
use the same player height. Feet remain anchored; movement collision and saves
retain their existing values. Ashfall source/art was not changed.

For idle/walking free arms, forward joint rotation now constructs the elbow and
wrist from stable upper/lower arm lengths. This avoids the old moving wrist
solver overstretching and straightening elbows at stride extremes. Side views
use a restrained opposite-leg swing; front/back views use subtle projected depth.
Occupied hands swing less. Two-handed weapons use a central carry with a steady
support grip while the legs walk. Combat and gathering retain their action poses.

The existing equipment test now samples complete walking cycles in all four
facings for unarmed, sword and two-handed spear carry. It verifies segment length
and frame-to-frame elbow/wrist continuity. The existing 93 rendered poses, weapon
grips, armor slot selection, gathering and save-roundtrip checks also pass.

Eighteen desktop/phone browser captures cover town/dungeon/interior presentation,
four walking directions, and persisted plate/mixed equipment without runtime
exceptions. The publishing workflow separately verifies Chrome and WebKit.
