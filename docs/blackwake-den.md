# Blackwake Den — first PvP test

Visit from MENU → Visit Blackwake Den while in a town. The separate harbor has a fenced dueling pit, spectator benches, The Loaded Die casino, inn, bank and ferry back to Saltmere. Walking and tap-to-walk use the existing movement code. The bank opens the existing personal storage screen.

## Consent-only arena

Both players must stand near the pit. A challenge states its stake and expires after 30 seconds; the other player explicitly accepts or declines. Acceptance reserves both stakes atomically, places fighters inside the pit and starts a three-second countdown. One match occupies the pit; visitors may watch.

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
