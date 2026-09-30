# Blackwake Den — first PvP test

Visit from MENU → Visit Blackwake Den while in a town. The separate harbor has a fenced dueling pit, spectator benches, The Loaded Die casino, inn, bank and ferry back to Saltmere. Walking and tap-to-walk use the existing movement code. The bank opens the existing personal storage screen.

## Consent-only arena

Both players must stand near the pit. A challenge states its stake and expires after 30 seconds; the other player explicitly accepts or declines. Acceptance reserves both stakes atomically, places fighters inside the pit and starts a three-second countdown. One match occupies the pit; visitors may watch. Choose Practice with Captain Vale for a free NPC match. The trainer closes distance, strikes and guards using the same server combat rules; NPC practice never pays gold or progression rewards.

Arena stats are normalized to 100 HP and 100 stamina. Appearance still follows equipment. Strike deals 10 damage at close range; Lunge deals 16 at a longer range and costs more stamina. Guard halves the next blow during its short window. The server checks membership, range, cooldown, stamina, movement speed and arena bounds. Clients never report damage or winners.

The first knockout or surrender ends the match and pays the complete pot to the winner exactly once. An active disconnect forfeits; an interrupted countdown or three-minute timeout refunds both stakes. Gear, character health, fame, karma and main-character gold are unchanged. This is consensual dueling, not unrestricted wilderness PvP. Spells, pets, ranked gear-based combat and spectator betting are future work.

## Test purse and casino

A browser receives a separate 500-gold Den test purse. A random 256-bit bearer token stays in browser storage; the server stores its hash, balance and settlement state in the zone Durable Object. This test purse is not a Supabase account wallet and does not transfer between devices or into the main economy. Clearing browser data creates a new test purse. Do not treat these balances as a production economy.

The Loaded Die rolls one unbiased server-side d6. Pick a face and stake 10, 25 or 50 test gold. A matching face returns 5× the stake (including the stake); a miss loses it. Win probability is 1/6. There is no real-money purchase, payout or cash value.

Casino requests carry a persisted UUID and server sequence. Only one unresolved roll is allowed by the client. Reconnecting recovers that UUID; the server replays the accepted result and rejects an old sequence rather than charging it again. Browser storage is required to preserve the purse and pending request.

## Deployment

The front end capability-checks `den_state.version=1`. An older presence/chat Worker allows exploring the town but cannot offer playable duels or casino rolls. It displays an update-needed message instead of simulating another player.

`.github/workflows/deploy-mp.yml` tests the engine, actual WebSockets, legacy presence/chat and casino recovery before deployment. Add `CLOUDFLARE_API_TOKEN` and `CLOUDFLARE_ACCOUNT_ID` as GitHub Actions secrets in `ringshark/townforge-source`; the token must be authorized to deploy the existing `townforge-mp` Worker and Durable Object namespace. The workflow also has Run workflow for retrying after setup. No new Durable Object migration or Supabase migration is required.

Checks: `node server/mp/test/den.mjs`, `node tests/den-client.cjs`, the original movement/input/guild checks, Emscripten build, `server/mp/test/smoke.mjs` and `server/mp/test/den-live.mjs` against a running Worker, plus browser navigation/layout checks.

## Weekly NPC ladder

The pit's Weekly NPC ladder is separate from unlimited free practice. It has five fixed opponents: Dockhand Jory, Cutlass Mira, Bosun Rook, Captain Vale and The Blackwake Champion. Each stage must be cleared in order by a server-confirmed knockout. Opponents gain health, movement pace, lunge range, reactive guards and spacing behavior. Players retain 100 HP and the ordinary arena actions. Entry and retries are free; gear, fame and main-character gold are untouched. Surrender, disconnect and timeout do not clear a stage. Countdown interruptions are not recorded as losses.

Ranked entry requires the existing Cloud save account. The Worker verifies the access token with the configured Supabase Auth server, ignores any client-supplied account ID and keeps account-linked records in the Den Durable Object. A new device resumes the same stages after Cloud sign-in. Bearer tokens are not stored in attachments or ladder records. Only public character names, cleared stages and cumulative winning-fight times appear in the leaderboard.

Weeks start Monday at 00:00 UTC. Progress, wins/losses and the leaderboard reset; unlocked cosmetic titles remain. Stage 3 unlocks Pit Contender; stage 5 unlocks Blackwake Champion. The title appears above your character inside Blackwake Den and in the ladder panel. A match crossing a weekly boundary cannot advance the new week. Leaderboard order is highest cleared stage, then lowest cumulative time in winning fights. Completing all five stages leaves free practice and friend duels available until reset.

The Worker uses the existing public Cloud project URL and public anon key in `wrangler.toml`; no database migration or new secret is required. `node server/mp/test/ladder.mjs` verifies account rejection, ordered stages, settlements, cross-device persistence, rewards and weekly boundaries.
