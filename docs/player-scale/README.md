# Larger player sprite

Increase only the compact player's rendering scale by 40%, around its existing
foot point. Layered equipment, action-sheet fallbacks, shadows and prompts follow
the same scale. The shared NPC size and physical movement/collision radii retain
their values. Upper/lower arms, legs and boots are modestly thicker, and the torso
is broader, giving the avatar a sturdier silhouette. Armor still uses the shared
pose; the character preview uses those same proportions.

Validation: the existing equipment renderer check passed 84 art parts and 93
poses, including all four directions, independent armor slots, weapon grips,
two-handed poses, casting, hit/death, fishing, mining, chopping and equipment-save
round trips. Eighteen browser captures cover town, dungeon and compact interiors,
four walking directions, and persisted plate/mixed gear on desktop and phone
viewports, without runtime exceptions. These local phone captures use Chromium
touch emulation; the publishing pipeline runs its Chrome/WebKit checks separately.
