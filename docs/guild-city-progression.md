# Guild city progression

This release adds four linked stages to the shared 3D guild city.

## Economy and receipts

Storehouse deposits cost 20 wood, ore or leather, or 100 gold. Each earns 10 merit. Foundation orders now spend real pack resources: timber 10 wood, ore 10 ore, tools 5 wood + 5 ore, supply contracts 50 gold. Existing foundation progress is retained. The legacy free-order endpoint is retired.

Research costs are unchanged, but donations now use the same inventory receipt flow. The Storehouse ledger displays the latest 20 contributions and officer actions. Hall construction, research recommendations, notices and rank changes are audited.

A transaction locks membership, the guild, and the player's cloud save. It compares the exact server timestamp, spends resources and records an immutable UUID receipt in one database transaction. Cloud uploads use `tf_save_commit` with the same revision check. Direct save writes are removed by RLS; reads remain private to the owner.

The client reserves gameplay during a pending transaction, stores its UUID and pre-action snapshot, persists the applied UUID with the character, and only then acknowledges it. Lost responses can be retried; a replay returns the existing receipt. A failed database transaction rolls back its spend and releases the reservation.

Inventory earning and dungeon victories remain client-originated because Town Forge supports local/offline progression. This provides concurrency and retry safety, not authoritative verification of combat or offline earnings against a modified client.

## City buildings

Workshop, Storehouse, Aid Lodge and Command Post each have independent queues and levels 1–5, capped by Hall level. Workshop and Storehouse require Hall 2. Officers start upgrades; all members can help once per queue UUID. Level L costs 50L wood + 50L ore + 100L guild funds and takes 15L minutes.

- Workshop: +5 merit per additional level on contracts.
- Storehouse: +10 gold per additional level on activity rewards.
- Aid Lodge: +5 seconds per additional level, multiplied by the building level, when helping city construction.
- Command Post: reduces enemy objective-order points by 5 per additional level, minimum 5 points.

City buildings show their level and added banners; active upgrades show scaffolding. A building click opens its progression panel and links to its existing research, shop or help function.

## Cooperative activities

Each member can finish three supply contracts per UTC day at Hall 2+: 20 timber, 15 ore or 15 leather. Materials enter the treasury; contracts add 25 funds and award 30 merit plus the Workshop bonus.

Officers may launch one expedition and one boss raid per guild per UTC day. Launch costs 50 or 100 funds. Joining costs 25 personal gold and is recorded once.

Expeditions need three members and resolve after 30 minutes. Boss raids last one hour and need 200 damage. Joined members' dungeon-boss victories contribute 5–20 damage based on recorded member power, with a 30-second server cooldown and per-member contribution cap. A successful raid closes early. Boss rewards require a dungeon-victory contribution, not merely joining.

Successful participants can claim one reward: expedition 75 gold, raid 150 gold, plus the Storehouse bonus; both grant 10 wood, 10 ore and 50 merit. Failed activities award nothing. Claims stay available for two days after the scheduled end.

## Scheduled warfare

Officers first declare a rival using the existing Wars panel, then schedule it from the Command Post or Wars panel. Each guild can have one battle per weekly window. Battles start Saturday 18:00 UTC and end at 19:00 UTC, with at least one hour of notice. Scheduling during or shortly before that window chooses the next week.

Members issue up to three city objective orders: gate costs 15 wood for 30 base points; tower costs 15 ore for 40; keep costs 15 leather for 50. The opponent's Command Post reduces these points. This is a server-scored city objective battle, not a new real-time PvP combat simulation. A player cannot contribute for both sides of one battle.

After the window, participating members claim once: victory 200 gold + 80 merit; draw 100 gold + 40 merit; defeat 50 gold + 40 merit. Nonparticipants cannot claim. Scores and caps are server-owned; client-supplied scores are not accepted.

## Migration and validation

Apply `web/supabase.sql`, then `web/guild-city.sql`, in one transaction. Reapplying the base file alone temporarily restores the old free endpoint and save policy; always apply the extension afterward. The migration workflow performs PostgreSQL scenario tests before applying both files using the existing `SUPABASE_DB_URL` secret.

Checks:

- `NODE_PATH=<PGlite installation>/node_modules node tests/guild-city.cjs`
- `node tests/guild-city-cloud.cjs`
- existing guild client and production C++ parser checks
- Emscripten build and browser visual checks
