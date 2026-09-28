# Guild settlements: review and proposed first release

Status: design proposal, not implemented. Reviewed source branch ringshark-patch-1
at b7dcb09 and web/guildnet.js + web/supabase.sql. No production database changes.

## Existing behavior

Online guilds have creation/join/leave, 30-member capacity, member/officer/leader
roles, a message of the day, weekly points, standings and war declarations.
The Guild screen also contains a personal NPC Warband. Player-versus-player
Battle Day is explicitly described as coming in the current UI; declarations
must not be presented as a complete multiplayer combat system.

## Concrete issues

1. Guild refresh checked roster errors but ignored failed score, message and war
   queries. The accompanying client fix surfaces any of these failures instead
   of treating missing data as zero scores or peace.
2. tf_submit accepts client-reported running totals (capped at 5,000). A cap is
   not proof that resources were earned. Never mint a shared stockpile by
   accepting an arbitrary client resource balance or donation amount.
3. war_scores is keyed by week/day/user; tf_submit updates guild_id on conflict.
   Joining another guild and submitting can transfer that day's score to the
   new guild, despite the UI promising earned points stay with the old guild.
   Before changing the schema, decide attribution: record immutable earned
   increments against the guild at earning time, with event IDs for retries.
4. The Wars screen mixes real-player declarations and personal NPC faction wars.
   Give these explicit headings: Guild diplomacy and Warband campaigns.
5. Shared settlement projects are absent from the reviewed guild client/API.
   Personal house/settlement progress must remain distinct from guild ownership.

## Proposed loop inspired by the requested WOS direction

Gather or craft -> fulfill guild work orders -> advance a visible construction
project -> unlock a shared service -> take on a harder cooperative objective.

Start with a Guild Hall and one project: a Workshop. The hall is the social
center; the Workshop unlocks shared crafting orders rather than a large combat
stat bonus. Add storehouse capacity, gathering/logistics research and scheduled
PvE defense only after the first project is reliable and satisfying.

### First playable slice

- Officers select a project; all members can inspect exact material targets.
- Gatherers supply timber/ore, crafters supply beams/tools, adventurers complete
  supply contracts. Every role has a meaningful path to contribute.
- Show contribution receipts and remaining requirements, not just a leaderboard.
- One active project keeps small guilds focused. Use explicit per-project caps
  to prevent a single veteran buying all participation out of the loop.
- Progress replaces scaffolding with a completed building; banners carry guild
  colors. The result must be visible in the 3D settlement.
- Completion grants access and modest convenience, with rewards earned through
  participation. Avoid uncapped permanent combat multipliers.
- Construction progress persists; weekly cooperative goals reset separately.
- Personal homes and NPC warbands remain personal. Leaving a guild never moves
  shared resources, completed buildings or old contribution receipts.

### Persistence and authority required before launch

Use guild_settlements, guild_projects and an append-only contribution ledger.
The server must validate membership, permission, project state, contribution
limits and resource ownership. Atomically debit an authoritative inventory and
credit a project; retry using a unique request ID. Lock the project for concurrent
contributions and clamp accepted material to the remaining requirement. Return
the accepted amount and updated balances. Offline requests are intents only.

The current browser/local-save resource model is not an authoritative inventory.
Until server-owned balances or validated earning events exist, prototype with
clearly marked test resources and no competitive or transferable rewards.

### Acceptance scenarios

Two members finish the same project concurrently; retries after lost responses;
full stockpile; membership changes during donation; leadership handover;
project cancellation/refund policy; reconnect after offline play; weekly reset
without loss of permanent buildings; account switching; unauthorized RPC calls.

## Art direction

Preserve the stylized fantasy identity. Favor readable silhouettes, restrained
surface highlights, directional water reflections, finer bronze UI framing and
recognizable settlement landmarks. Upgrade stages should visibly change roofs,
stonework, scaffolding, guild banners and work activity. Preserve mobile budgets;
do not re-enable the disabled shadow-map path without device testing.

The initial code pass reduces universal plastic-looking specular highlights,
uses direction-aware water glints, corrects normal transforms to use vectors,
and simplifies panel frames. Runtime visual QA is still required before release.
