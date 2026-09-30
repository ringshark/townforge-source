# Character equipment and Try on

Open MENU → Character (Me). Gear shows the character, Pack shows the full
backpack, Skills opens skills, and Spells opens the spellbook directly.

Armor, Clothes and Jewels select the displayed equipment slots. Slot captions
show the equipped item's short name or Empty. Backpack cards show names; tap
one for its full name and details.

Tap Try on to temporarily display a backpack item on the character. The item
stays in the backpack, combat stats stay unchanged, and the world keeps your
actual equipment. Equip applies the change. Cancel preview, closing inspection,
selecting another item or leaving Me ends the preview. Trying on from Pack
opens Gear so the character is visible. Drag the character to inspect its sides.

The preview and actual equipping share ApplyEquipmentItem, including removal
of incompatible shields/two-handed weapons and returning displaced items once.
The world and Me use the same modular character, equipment layers and grips.

The normal-character draw path previously fell through into skeleton rendering
when no imported clothing mesh was active. Skeleton geometry now draws only
for actual skeletons, eliminating the extra bone shapes over human hands/body.
This change reuses existing art and spends no Meshy generation credits.

Validation: tests/equipment-preview.py exercises production equipment changes,
non-mutating previews, clothing/jewelry replacement and two-handed transitions.
The existing UI input suite checks gesture ownership and modal interception.
