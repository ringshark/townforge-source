# 2D character, creature, interior and dungeon pass

The compact adventurer retains their identity and existing four-direction walking
art. Sixty-four additional inspected action cells provide sword attacks, casting,
hit reactions, falling/defeat and gathering poses. North-facing combat has its own
back-view atlas. Animation phases read the actual attack/cast timers; the death
sequence follows the countdown. Equipped non-sword weapon art is retained, hidden
characters are translucent, sprinting has a faster cadence, and a ridden pet is
visible beneath its rider.

All eleven tameable species have explicit persistent identity on newly tamed pets.
An additive final field in pet save rows preserves that identity after renaming.
Old rows still load; existing species names and unambiguous original stat ranges are inferred when possible, with the
existing role-based fallback for old renamed pets whose original species was never
recorded. The 2D renderer shows every active follower, selects locomotion versus
combat poses from movement/attack events, and displays hit reactions. Existing
pet stats, bonding, training and follower limits are unchanged.

The audit covers the eleven creature sheets, five wild-monster sheets, five unique
dungeon families and six boss textures. Uniform white matte is flood-cleared only
from cell borders, retaining enclosed highlights. Attack stances no longer replay
a walking cycle. Enemy body families distinguish imps, hounds, raiders, drakes,
undead, spiders, rats and brutes. Bosses use opaque-bound scaling and existing
combat effects; defeated actors visibly tip/fade. No combat damage is added by
visual animation.

Sixty new furnishing and detail sprites replace the colored shape stand-ins throughout
all thirteen interiors, including custom home furniture and workstations. Existing
floor/wall textures, furniture placement, collisions, storage and actions remain.
Shop NPCs use character sprites. Source atlases are retained unmodified; metadata
uses their inspected row spacing and alpha bounds.

Each dungeon is now a 3600 by 3600 world with four times its former room area.
The six original connected room topologies remain distinct. Twenty regular spawn
locations are spread with deterministic farthest-point placement; the boss retains
its own chamber and the entrance has a safety clearance. Encounter identity,
rewards, difficulty and boss unlock requirements remain. Live respawn arrays are
transient; saved dungeon progression does not need a migration. World homes,
farms, PvP, orc groups, cloud configuration and deployment settings are unaffected.

A minimap is visible from entry. The full overview is revealed immediately, shows
rooms/foes/exit/player and opens with M or a large touch target. The camera follows
the character during exploration. The overview owns input and cannot be opened
during an active fight. Paths and every spawn are checked for connectivity and
spacing. Native tests cover animation phases, pet save compatibility, all interior
controls and existing homestead/save behavior. Browser checks restore a synthetic
save, use actual controls to enter a dungeon, and verify full-map input ownership
on desktop and phone. Art provenance: newly generated RGBA hero-action and
furniture atlases, alongside the retained existing game art.

The atlas audit includes each source file and usable frame bounds in
`full-graphics-audit.json`. Generated action/furniture images are never assumed to
have the requested dimensions: the runtime uses their inspected actual dimensions
and row boundaries. Fishing keeps its own rod, line and bobber presentation rather
than borrowing the chopping action.
