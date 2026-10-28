# Shared 2D equipment and work animations

The compact hero now uses a weapon-free articulated sprite built from 84 inspected
RGBA atlas cells: 72 directional body/armor parts and 12 separate held items. Front,
profile and back views share the same joint layout; right-facing art mirrors the
profile. Leather, chain/ring mail, and plate select independently for helmet, chest,
arms, gloves, legs and gorget. Mixed equipment stays mixed. Existing shirt, pants
and shoe dyes tint uncovered parts. Unrecognized armor uses the leather family;
this release supplies material families, not bespoke art for every named item.

A pose supplies shoulders, elbows, wrists, hips, knees and feet. Body parts follow
those joints throughout walking, attacks, casting, recoil and falling. Weapon art
has an inspected grip pivot, independent of its alpha bounds or inventory icon.
Swords use the hilt, axes use the off-center handle, spears/staves use their shaft,
and bows use their own aim axis. Two-handed weapons place the second hand on the
same shaft/hilt; shields remain upright. Unarmed bodies contain no baked sword.
The existing compact sprite/actions remain an asset-load fallback.

Weapon families select slash, thrust, overhead or ranged poses from the real
combat-effect timers. Mining uses a pickaxe overhead arc; lumberjacking uses an
axe chopping arc. Fishing uses a rod with a casting windup, deployed line/bobber,
and a final reeling movement. Work poses read gatherDuration and
gatherSecondsRemaining, so they start with the actual activity. No extra damage,
resources, skill gains or tool charges are introduced by animation.

The 2D Equipment page previews the same renderer and try-on equipment. Its Pose
button cycles Idle, Walk, Attack, Cast, Chop, Mine and Fish; dragging turns the
figure. Preview timers and tools live in a separate display-only state and never
write to the character's equipped items. The 3D preview path remains available for
the archived 3D variant. Item serialization, equipment rules, combat, gathering,
saves, cloud/online configuration and deployment permissions are unchanged.

Art provenance: newly generated transparent modular part and weapon atlases.
Original images are retained without edits. Runtime atlas boundaries use inspected
row spacing and the plate shoulder's wider column rather than assuming a uniform
grid. Point filtering preserves the small sprite's silhouette. The native asset
check rejects empty or boundary-clipped parts and exports an SVG contact sheet
from the actual DrawTexturePro transforms. Native tests cover weapon aim, grip
metadata, animation phase changes, mixed slots, world-renderer integration and an
actual equipment save round trip. Chromium and phone WebKit tests restore all
three armor sets plus mixed equipment, then use real mouse/touch controls to open
Equipment and run all seven preview poses without changing saved equipment.
