// Town Forge — raylib/C++ port of the town-building core
// -----------------------------------------------------------------------
// Ported from town-forge-prototype.html (a single-page JS idle RPG).
// That file is ~5600 lines covering town building, gathering, crafting,
// combat, magic, taming, notoriety, etc. Porting all of it 1:1 is a
// multi-week project on its own, so this file focuses on the piece you
// asked for explicitly: the main game loop, input handling, and cell
// rendering for the Town Map — i.e. the `BUILDINGS` object and the
// `renderTownMap()` function from the original JS, rebuilt as real,
// running C++.
//
// What's faithfully ported from the JS:
//   - The BUILDINGS table: Smith/Carpenter/Tailor/Alchemy, each with the
//     same 5 levels, the same gold+resource upgrade costs, and the same
//     upgrade times (in seconds) as the original `levels:[...]` arrays.
//   - Starting resources (gold:100, wood:10, ore:0, leather:5) match
//     the JS `state = {...}` defaults (oreByType tracked as a single
//     `ore` pool here for simplicity).
//   - The 3x3 town-map-grid layout and per-tile selection behavior
//     (click a tile -> select it -> see its detail panel) from
//     renderTownMap()/selectBuilding() in the JS.
//   - Gathering wood/ore takes 8 seconds per action, matching the JS
//     comment "Gather wood or ore (8s per action)"; each action yields
//     3-5 units (JS: `3 + Math.floor(Math.random()*3)`).
//   - Combat/dungeons: the 4 dungeons, their monster rosters (name/level/
//     baseGold/baseLeather), boss XP unlock thresholds, and the core
//     hit-chance/damage formulas from winChanceAgainst()/playerAttackRoll()/
//     monsterAttackRoll() are ported as-is.
//   - Crafting & equipment: the Smith/Carpenter/Tailor weapon & armor
//     recipe lists (name/category/slot/handed/reqSkill/cost/power), the
//     quality-tier system (Flimsy -> Legendary, scaled by min(skill,
//     workshop cap)), the skill-gain taper as skill approaches 100, and
//     equip/sell straight from the backpack.
//   - Gathering skills & auto-gather: real Lumberjacking/Mining/Skinning
//     skill values (capped at 120) using the exact rollGatherSkillGain()
//     taper from the JS; an Auto-Gather toggle that requires 30 skill and
//     chains Mining -> Wood -> stop at 100/100, matching
//     nextAutoGatherType()/toggleAutoGather(); and corpses left behind by
//     combat wins (see skinCorpse()) that must be skinned for their
//     leather+gold rather than looted instantly.
//   - Magic / spellcasting: the full 16-spell table across all 8 circles,
//     spellSuccessChance()/spellPowerFor()'s exact formulas, mana regen
//     scaled by Meditation, and the same Magery/Eval Int/Meditation
//     training on every cast (combat or practice) as applySpellTraining().
//     Offensive spells and the two Mending heal spells are fully castable
//     in combat as alternatives to a physical attack, and every spell can
//     be practiced risk-free (mana only, no reagents) on the Magic tab.
//   - Taming & pets (added in this revision): all 11 WILD_CREATURES with
//     their real difficulty/stat ranges, the exact tameChance() slope and
//     hard ceiling (dragons never become a sure thing), petSlotCapacity()
//     from Taming+Lore+Veterinary, and a tamed pet's real turn in combat
//     (casts if it's a Caster with mana, otherwise a wrestling-style
//     bite) — including the monster sometimes targeting the pet instead
//     of you, at the same role-based chance as the JS. Healing, instant
//     gold-for-skill training (to 30), release, and selling all use the
//     JS's real formulas. See "Taming & pets" below for what's simplified.
//   - Notoriety, murderers & innocents (added in this revision): the real
//     notoriety decay rate and Innocent/Criminal/Murderer tiers, random
//     ambush encounters scaled by both your combat power and your current
//     notoriety (matching maybeTriggerAmbush()'s multiplier), a murderer
//     fight's distinct win/loss/flee payouts (gold+Fame+Karma on a win;
//     40% gold and your whole backpack on a loss; a lighter 15%/15% if you
//     break off mid-fight), the Shaken status (-15% combat power for 3
//     fights), and innocent-traveler encounters with real Spare/Snoop/
//     Steal/Murder formulas and consequences. See "Notoriety, murderers &
//     innocents" below for what's simplified.
//   - Save / load: every durable field (gold, resources, all skills,
//     buildings, backpack, equipped gear, pets, corpses, potions, bank
//     contents, weekly-goal progress, notoriety/fame/karma) autosaves to
//     a local text file every 2 seconds and on quit, loads once at
//     startup, and runs the JS's offline Auto-Gather catch-up (capped at
//     1 hour / 2.5 total skill, exactly like applyOfflineAutoGather())
//     using how much real-world time passed since the last save. See
//     "Save / load" below for what's simplified (no JSON, transient
//     state like an active fight isn't persisted).
//   - Alchemy potions (added in this revision): all 12 potion recipes
//     (Lesser/Regular/Greater Heal, Refresh, Poison, Explosion) on a 4th
//     Alchemy tab in Craft, using reagents rather than a resource pool
//     and — matching the JS exactly — NOT quality-scaled, only skill-
//     gated. Heal potions restore HP anytime; Poison Potions coat your
//     weapon for real bonus damage over N charges; Explosion Potions
//     Throw as a free action in combat (no monster counter-attack
//     follows, exactly like throwExplosionPotion()). See "Alchemy
//     potions" below for what's simplified (Refresh potions have
//     nothing to restore — no stamina stat here).
//   - Banking (added in this revision): the Vaultkeep — deposit/withdraw
//     gold and items on their own Bank tab, completely safe from every
//     danger system above (a murderer loss, a failed Snoop, or a
//     guard-zone confiscation never touches the bank).
//   - The Hearthmoot's weekly goals: all 8 real goals (gather/craft/brew/
//     defeat/tame plus the 3 Bloodstained Road bounties) on a real 7-day
//     cycle tracked by elapsed time, each claimable for gold once met,
//     and a Weekly Blessing (+10% combat power) for clearing the
//     original five in the same week — wired into every system that
//     reports progress into it.
//   - Weapon-category combat skills (added in this revision):
//     Swordsmanship/Fencing/Macing/Archery/Wrestling all train for real
//     (whichever matches your equipped weapon's category, or Wrestling
//     unarmed) and add a genuine hit-chance bonus, exactly matching
//     playerAttackRoll()'s weaponSkill*0.2 term. Tactics and Anatomy add
//     real CombatPower() bonus percentages; Magic Resistance actually
//     tapers spell-disruption chance instead of being fixed at 0.
//   - Bandages & Healing (added in this revision): a plain consumable
//     count (not a backpack item), crafted by the Tailor (2 leather -> 5,
//     no skill required) or bought from the Provisioner stand-in; using
//     one — in or out of combat — always trains Healing (and a 30% chance
//     of Anatomy), with the heal itself a skill-scaled coin flip, matching
//     useBandageOutOfCombat()/combatBandage() exactly.
//   - The Echo system (added in this revision): all 18 capped combat/
//     magic/animal/rogue skills share a real 700-point active budget
//     (kTotalSkillCap) on the new Skills tab — every skill still trains
//     freely to its own 120 cap regardless, but only "Active" skills
//     apply to actual gameplay (EffectiveSkill()), and benching one frees
//     room to activate another without losing any trained progress.
//     Trade skills (Lumberjacking..Alchemy) were never part of this
//     budget and stay always-active, matching the JS exactly.
//   - The Bloodstained Road (added in this revision): all 3 permanent
//     ladders (Blue/Gray/Red), 5 tiers each with a named boss, real
//     level/gold scaling off your own combat power, boss-loop scaling
//     (+15% per return trip) once a path's boss falls, path-specific
//     Fame/Karma/Notoriety effects, and the Gray path's distinct
//     Snoop-or-Steal-or-Fight choice (a failed Snoop drops you straight
//     into the fight with a free hit already landed). Reachable from a
//     Dungeons/Bloodstained Road sub-tab on the Hunt screen.
//
// What's intentionally NOT ported here (flagged so nothing is silently
// dropped): Debuff/Buff/Summon spell effects (listed with real data but
// not castable — no status-effect system here), a pet's bleed/poison
// attacks (only its base bite/spell damage is ported), ore
// tiers/quality (a single "ore" pool stands in for Iron through
// Valorite), rarity-item corpse drops (murderer and Bloodstained kills
// pay gold only, no rolled items), the theft-duo counter-fight for a
// failed field Snoop (simplified to the same fine as a failed town
// Snoop). Each of those is its own subsystem in the original file and is
// a reasonable next step to port incrementally on top of this scaffold.
//
// Build:
//   g++ -std=c++17 main.cpp -o townforge -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
// (or use the CMakeLists.txt / vcpkg raylib package on Windows/macOS)
// -----------------------------------------------------------------------

#include "raylib.h"
#include <array>
#include <string>
#include <optional>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdio>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

// ---------------------------------------------------------------------
// Shared UI palette — a deliberate 60/30/10 scheme: soft cream dominates as the page
// background (60%), sage green marks secondary structure like panels/cards/plates
// (30%), and warm slate is reserved for highlight/focus — buttons, selection, "in
// range" rings (10%). Replaces the earlier gold/parchment scheme entirely (every bare
// `kColorSlate` use in the file was swapped to kColorSlate in the same pass, except the two
// boss-node color literals, which got the same swap for full consistency too).
// ---------------------------------------------------------------------
static const Color kColorText    = { 26, 23, 20, 255 };    // primary body text — dark warm neutral, reads on both cream and sage (darkened 2026-09-22)
static const Color kColorHeading = { 45, 40, 34, 255 };    // screen titles / "Fighting: X" etc. (darkened 2026-09-22)
static const Color kColorAccent  = { 55, 62, 46, 255 };    // sub-headers, labels, section titles — dark sage, ties to the card color (darkened 2026-09-22)
static const Color kColorPanelBg = { 163, 177, 143, 255 }; // secondary structure — panels/cards/plates (30%): sage green
static const Color kColorPageBg  = { 248, 244, 234, 255 }; // dominant page/world background (60%): soft cream
static const Color kColorSlate   = { 122, 110, 98, 255 };  // highlight/focus accent (10%): warm slate

// ---------------------------------------------------------------------
// Data: mirrors BUILDINGS in the JS. Each craftable building has up to
// 5 levels; level 5 is capped (upgradeCost == none, matching `null` in JS).
// ---------------------------------------------------------------------

enum class Resource { Wood, Ore, Leather, None };

// A craftable recipe (weapon or armor) — ported from each building's `recipes:[...]`
// list in the JS BUILDINGS table. `slot` is only meaningful for armor; `handed` only
// for weapons.
enum class ItemType { Weapon, Armor, Potion }; // Potion recipes store their effect in
                                                 // `category` ("heal"/"stamina"/"poison"/
                                                 // "damage") and potency in `power`.
struct Recipe {
    std::string name;
    ItemType type;
    std::string category;   // "Swordsmanship", "Plate Mail", etc. — matches JS `category`
    std::string slot;        // armor only: "helmet","gorget","gloves","arms","legs","chest"
    std::string handed;      // weapon only: "1h" or "2h"
    int reqSkill;
    int cost;                 // resource units consumed (ore/wood/leather, matching the building)
    int power;                 // base power before quality multiplier
};

struct BuildingLevel {
    int level;
    int cap;              // skill cap this level unlocks
    int goldCost;          // -1 = maxed, no further upgrade
    int resourceCost;
    float upgradeTimeSec;
};

struct BuildingDef {
    std::string key;       // "smith", "carpenter", ...
    std::string name;       // display name
    Resource resource;     // resource type consumed on upgrade
    Color color;            // tile color
    std::array<BuildingLevel, 5> levels;
    bool craftable;         // false for static amenity tiles (bank, healer, ...)
    std::vector<Recipe> recipes; // empty for non-craftable buildings
};

// Craft-building costs copied verbatim from the JS BUILDINGS table. Recipe lists are
// also copied verbatim (name/category/slot/handed/reqSkill/cost/power) from the
// Smith/Carpenter/Tailor `recipes:[...]` arrays — the Alchemy potion list is left out
// since potions belong to the Alchemy/magic system, not weapons & armor.
static const std::array<BuildingDef, 4> kCraftBuildings = {{
    { "smith", "The Cinderforge", Resource::Ore, {180, 70, 40, 255}, {{
        {1, 50,  30,  15, 8.0f},
        {2, 65,  60,  30, 12.0f},
        {3, 80,  110, 50, 16.0f},
        {4, 100, 200, 80, 20.0f},
        {5, 100, -1,  -1, 0.0f},
    }}, true, {
        // Swordsmanship
        {"Cutlass", ItemType::Weapon, "Swordsmanship", "", "1h", 0, 8, 4},
        {"Scimitar", ItemType::Weapon, "Swordsmanship", "", "1h", 20, 12, 6},
        {"Katana", ItemType::Weapon, "Swordsmanship", "", "1h", 40, 16, 9},
        {"Longsword", ItemType::Weapon, "Swordsmanship", "", "1h", 60, 20, 12},
        {"Broadsword", ItemType::Weapon, "Swordsmanship", "", "1h", 80, 28, 16},
        {"Bardiche", ItemType::Weapon, "Swordsmanship", "", "2h", 90, 32, 19},
        {"Viking Sword", ItemType::Weapon, "Swordsmanship", "", "2h", 100, 40, 22},
        {"Halberd", ItemType::Weapon, "Swordsmanship", "", "2h", 100, 45, 24},
        // Fencing
        {"Kryss", ItemType::Weapon, "Fencing", "", "1h", 0, 8, 4},
        {"Pitchfork", ItemType::Weapon, "Fencing", "", "2h", 20, 12, 6},
        {"Short Spear", ItemType::Weapon, "Fencing", "", "2h", 45, 18, 10},
        {"War Fork", ItemType::Weapon, "Fencing", "", "2h", 70, 24, 14},
        {"Spear", ItemType::Weapon, "Fencing", "", "2h", 100, 34, 20},
        // Macing
        {"Club", ItemType::Weapon, "Macing", "", "1h", 0, 6, 3},
        {"Mace", ItemType::Weapon, "Macing", "", "1h", 25, 12, 7},
        {"Maul", ItemType::Weapon, "Macing", "", "1h", 50, 18, 10},
        {"War Axe", ItemType::Weapon, "Macing", "", "1h", 70, 24, 13},
        {"War Hammer", ItemType::Weapon, "Macing", "", "2h", 100, 34, 20},
        // Armor — Ring Mail
        {"Ring Mail Sleeves", ItemType::Armor, "Ring Mail", "arms", "", 10, 8, 2},
        {"Ring Mail Gloves", ItemType::Armor, "Ring Mail", "gloves", "", 15, 6, 2},
        {"Ring Mail Tunic", ItemType::Armor, "Ring Mail", "chest", "", 20, 14, 4},
        {"Ring Mail Leggings", ItemType::Armor, "Ring Mail", "legs", "", 25, 10, 3},
        // Armor — Chain Mail
        {"Chain Coif", ItemType::Armor, "Chain Mail", "helmet", "", 40, 10, 4},
        {"Chainmail Sleeves", ItemType::Armor, "Chain Mail", "arms", "", 42, 9, 3},
        {"Chainmail Gloves", ItemType::Armor, "Chain Mail", "gloves", "", 44, 7, 3},
        {"Chainmail Leggings", ItemType::Armor, "Chain Mail", "legs", "", 46, 16, 6},
        {"Chain Tunic", ItemType::Armor, "Chain Mail", "chest", "", 50, 20, 7},
        // Armor — Plate Mail
        {"Plate Helm", ItemType::Armor, "Plate Mail", "helmet", "", 70, 18, 6},
        {"Plate Gorget", ItemType::Armor, "Plate Mail", "gorget", "", 75, 14, 5},
        {"Plate Gloves", ItemType::Armor, "Plate Mail", "gloves", "", 80, 16, 6},
        {"Plate Arms", ItemType::Armor, "Plate Mail", "arms", "", 85, 20, 7},
        {"Plate Legs", ItemType::Armor, "Plate Mail", "legs", "", 90, 24, 8},
        {"Plate Chest", ItemType::Armor, "Plate Mail", "chest", "", 100, 40, 16},
    } },
    { "carpenter", "The Hewnwood Hall", Resource::Wood, {120, 80, 40, 255}, {{
        {1, 50,  25,  15, 8.0f},
        {2, 65,  50,  30, 12.0f},
        {3, 80,  95,  50, 16.0f},
        {4, 100, 170, 80, 20.0f},
        {5, 100, -1,  -1, 0.0f},
    }}, true, {
        // Macing (staves)
        {"Shepherd's Crook", ItemType::Weapon, "Macing", "", "2h", 0, 6, 3},
        {"Gnarled Staff", ItemType::Weapon, "Macing", "", "2h", 30, 12, 6},
        {"Quarterstaff", ItemType::Weapon, "Macing", "", "2h", 55, 18, 9},
        {"Black Staff", ItemType::Weapon, "Macing", "", "2h", 80, 26, 13},
        // Archery
        {"Short Bow", ItemType::Weapon, "Archery", "", "2h", 0, 8, 4},
        {"Bow", ItemType::Weapon, "Archery", "", "2h", 25, 14, 7},
        {"Composite Bow", ItemType::Weapon, "Archery", "", "2h", 50, 20, 11},
        {"Crossbow", ItemType::Weapon, "Archery", "", "2h", 75, 28, 15},
        {"Heavy Crossbow", ItemType::Weapon, "Archery", "", "2h", 100, 40, 21},
    } },
    { "tailor", "The Woven Hearth", Resource::Leather, {180, 140, 60, 255}, {{
        {1, 50,  20,  12, 8.0f},
        {2, 65,  45,  24, 12.0f},
        {3, 80,  85,  40, 16.0f},
        {4, 100, 150, 65, 20.0f},
        {5, 100, -1,  -1, 0.0f},
    }}, true, {
        // Armor — Leather
        {"Leather Cap", ItemType::Armor, "Leather", "helmet", "", 0, 6, 2},
        {"Leather Gorget", ItemType::Armor, "Leather", "gorget", "", 10, 6, 2},
        {"Leather Gloves", ItemType::Armor, "Leather", "gloves", "", 15, 6, 2},
        {"Leather Sleeves", ItemType::Armor, "Leather", "arms", "", 20, 8, 3},
        {"Leather Leggings", ItemType::Armor, "Leather", "legs", "", 25, 10, 3},
        {"Leather Tunic", ItemType::Armor, "Leather", "chest", "", 30, 14, 5},
        // Armor — Studded Leather
        {"Studded Cap", ItemType::Armor, "Studded Leather", "helmet", "", 50, 10, 5},
        {"Studded Gorget", ItemType::Armor, "Studded Leather", "gorget", "", 55, 10, 5},
        {"Studded Gloves", ItemType::Armor, "Studded Leather", "gloves", "", 60, 10, 5},
        {"Studded Sleeves", ItemType::Armor, "Studded Leather", "arms", "", 65, 14, 7},
        {"Studded Leggings", ItemType::Armor, "Studded Leather", "legs", "", 70, 18, 8},
        {"Studded Tunic", ItemType::Armor, "Studded Leather", "chest", "", 80, 24, 12},
    } },
    { "alchemy", "The Bitterroot Still", Resource::None, {100, 140, 60, 255}, {{
        // Reagents-based upgrade costs in the JS; folded into gold-only here for the
        // *workshop* upgrade specifically (the potion recipes below do use reagents).
        {1, 50,  35,  0, 8.0f},
        {2, 65,  70,  0, 12.0f},
        {3, 80,  120, 0, 16.0f},
        {4, 100, 210, 0, 20.0f},
        {5, 100, -1,  -1, 0.0f},
    }}, true, {
        // Potion recipes copied verbatim from BUILDINGS.alchemy.recipes in the JS.
        // `slot`/`handed` are unused for potions; `category` carries the effect and
        // `power` carries baseAmount (potion potency — see TryCraftPotion below).
        {"Lesser Heal Potion", ItemType::Potion, "heal", "", "", 0, 3, 15},
        {"Heal Potion", ItemType::Potion, "heal", "", "", 40, 6, 30},
        {"Greater Heal Potion", ItemType::Potion, "heal", "", "", 80, 10, 50},
        {"Lesser Refresh Potion", ItemType::Potion, "stamina", "", "", 0, 3, 10},
        {"Refresh Potion", ItemType::Potion, "stamina", "", "", 40, 6, 20},
        {"Greater Refresh Potion", ItemType::Potion, "stamina", "", "", 80, 10, 35},
        {"Lesser Poison Potion", ItemType::Potion, "poison", "", "", 20, 5, 3},
        {"Poison Potion", ItemType::Potion, "poison", "", "", 55, 8, 5},
        {"Greater Poison Potion", ItemType::Potion, "poison", "", "", 90, 12, 8},
        {"Lesser Explosion Potion", ItemType::Potion, "damage", "", "", 25, 6, 20},
        {"Explosion Potion", ItemType::Potion, "damage", "", "", 60, 10, 40},
        {"Greater Explosion Potion", ItemType::Potion, "damage", "", "", 95, 15, 65},
    } },
}};

// Static tiles that exist in the town map but have no upgrade levels.
struct StaticTile { std::string key, name; Color color; };
static const std::array<StaticTile, 6> kStaticTiles = {{
    {"provisioner", "Provisioner", {140, 120, 80, 255}},
    {"stable",      "The Wildkeep", {90, 130, 60, 255}},
    {"healer",      "Healer",       {120, 160, 180, 255}},
    {"bank",        "The Vaultkeep",{70, 90, 120, 255}},
    {"townhall",    "Town Hall",    {184, 134, 11, 255}},
    {"house",       "Your House",   {150, 110, 70, 255}},
}};

// ---------------------------------------------------------------------
// Player housing — ported from the JS's HOUSE_TIERS/HOUSE_HUES/HOME_MODULE_DEF/
// HOME_MODULE_LEVELS (state.house/state.houseModules), which this C++ port never
// wired up until now (see GameState::kBackpackCap's old comment). Numbers copied
// verbatim, not invented — same porting convention as everything else here.
// "none" (index 0, houseTierIdx's default) is the JS's starting state — no house yet,
// capBonus/hueOptions/moduleSlots all 0 — the building itself still stands on the Town
// map so there's something to walk up to and buy a Cottage from.
// ---------------------------------------------------------------------
struct HouseTier { std::string key, name; int cost, capBonus, hueOptions, moduleSlots; };
static const std::array<HouseTier, 6> kHouseTiers = {{
    {"none",         "No House",     0,    0,   0,  0},
    {"cottage",      "Cottage",      300,  15,  3,  1},
    {"homestead",    "Homestead",    800,  30,  4,  2},
    {"manor",        "Manor",        1600, 50,  6,  3},
    {"estate",       "Estate",       3000, 70,  8,  4},
    {"grand_estate", "Grand Estate", 5000, 100, 10, 4},
}};
struct HouseHue { std::string name; Color color; };
static const std::array<HouseHue, 8> kHouseHues = {{
    {"Bronze",       {138, 106, 58,  255}}, {"Wax Red",      {122, 46,  46,  255}},
    {"Forest",       {63,  94,  63,  255}}, {"Slate Blue",   {74,  111, 165, 255}},
    {"Royal Purple", {107, 74,  138, 255}}, {"Gilded Gold",  {184, 147, 58,  255}},
    {"Teal",         {58,  138, 130, 255}}, {"Ash Gray",     {106, 106, 106, 255}},
}};
// buildingIdx indexes kCraftBuildings directly (smith=0/carpenter=1/tailor=2/alchemy=3) —
// a home wing is the same craft building, just capped lower and reached without a trip
// to town.
struct HomeModuleDef { int buildingIdx; std::string label; };
static const std::array<HomeModuleDef, 4> kHomeModuleDefs = {{
    {0, "Blacksmith Wing"}, {1, "Carpenter Wing"}, {2, "Tailor Wing"}, {3, "Alchemist Corner"},
}};
// A home module always sits one tier behind its real-building counterpart (JS comment,
// kept verbatim) — the point is genuine convenience without erasing the reason to go
// into town (the real buildings cap at 100, these cap at 85).
struct HomeModuleLevel { int level, cap, cost; };
static const std::array<HomeModuleLevel, 5> kHomeModuleLevels = {{
    {1, 30, 150}, {2, 45, 250}, {3, 60, 400}, {4, 75, 600}, {5, 85, 900},
}};

// ---------------------------------------------------------------------
// Combat / dungeons — ported from DUNGEONS, startCombat(), playerAttackRoll(),
// monsterAttackRoll(), endCombatWin()/endCombatLoss() in the JS.
//
// Simplifications from the original (flagged, not silently dropped):
//   - No equipment/inventory system yet, so "weapon power" is a single
//     upgradeable stat bought directly with gold, standing in for the
//     JS's equipped-item power. The tier names/power values are taken
//     from the real Smith recipe list (Cutlass/Katana/Broadsword/Viking
//     Sword) so the numbers you'd end up with by crafting them for real
//     later will feel familiar.
//   - No armor, so monster damage isn't reduced by totalDefense() yet.
//   - No magic items/rarity drops from rollCorpseDrop() — kills grant
//     gold + leather only, same amounts as the JS's baseGold/baseLeather.
//   - Str/Dex are fixed at their state.ts defaults (10) rather than
//     trainable stats, since there's no stat-training UI here yet.
// Everything else — monster levels, dungeon XP thresholds for unlocking
// bosses, and the hit-chance/damage formulas — matches the JS exactly.
// ---------------------------------------------------------------------

struct DungeonMonster {
    std::string name;
    int level;
    int baseLeather;
    int baseGold;
    bool isBoss = false;
    bool isMurderer = false; // true for ambush encounters — see "Notoriety & murderers"
    int bloodstainedPathIdx = -1; // -1 = not a Bloodstained Road fight; see that section below
    int bloodstainedTierIdx = -1;
    // Set for Wilderness monsters (dungeonIdx -1, like ambushes/Bloodstained, but with
    // real art instead of a plain circle) — the combat panel checks this before falling
    // back to MonsterFamilySheet(dungeonIdx), which returns null for any negative index.
    const Texture2D* icon = nullptr;
};

struct DungeonDef {
    std::string name;
    std::string theme;
    int bossUnlockXp;
    std::array<DungeonMonster, 5> monsters;
    DungeonMonster boss;
};

// Copied verbatim (level/baseLeather/baseGold/bossUnlockXp) from DUNGEONS in the JS —
// except The Hollow Warrens, which doesn't exist in the JS prototype (added directly in
// the C++ port, see the "Dungeon Tileset" art entry in memory/CLAUDE.md history). Its
// numbers aren't copied from anywhere; they extend the existing 4 dungeons' escalating
// pattern (level/bossUnlockXp/gold/leather all roughly +25-30% over Wyrmscar Depths) as
// the new 5th/hardest tier, rather than being an exact-formula port like the other 4.
static const std::array<DungeonDef, 5> kDungeons = {{
    { "Emberveil Hollow", "A caustic playground of living elements", 150, {{
        {"Silt Wretch", 2, 2, 2}, {"Stoneborn", 6, 4, 4}, {"Cinderling", 11, 6, 7},
        {"Bloatspore", 16, 9, 11}, {"Ridgeback Troll", 22, 14, 18},
    }}, {"Cinderlord", 30, 20, 60, true} },
    { "Bloodtusk Hold", "A raider stronghold carved into the hills", 220, {{
        {"Orc Whelp", 4, 3, 3}, {"Orc Skirmisher", 9, 5, 6}, {"Orc Warbringer", 15, 8, 10},
        {"Orc Berserker", 21, 12, 16}, {"Orc Warlord", 27, 17, 24},
    }}, {"Orc Overlord", 35, 24, 80, true} },
    { "The Sunken Crypt", "A flooded tomb where the dead do not rest", 300, {{
        {"Bonewalker", 6, 3, 4}, {"Rotbound Corpse", 12, 5, 8}, {"Gravewretch", 18, 8, 13},
        {"Grave Warden", 25, 12, 20}, {"Crypt Sovereign", 32, 17, 30},
    }}, {"The Hollow King", 42, 28, 110, true} },
    { "Wyrmscar Depths", "Ancestral hunting ground of the wyrm-kin", 400, {{
        {"Fen Serpent", 8, 4, 5}, {"Scalekin Raider", 15, 6, 9}, {"Emberdrake", 22, 10, 16},
        {"Skywyrm", 30, 15, 26}, {"Sovereign Wyrm", 38, 22, 42},
    }}, {"The Ancient Sovereign", 50, 35, 160, true} },
    { "The Hollow Warrens", "A collapsed dwarven mine, its tunnels claimed by things that fled the dark", 520, {{
        {"Warren Rat", 10, 5, 6}, {"Tunnel Skulker", 18, 7, 13}, {"Pickaxe Wraith", 26, 12, 20},
        {"Cave Brute", 35, 18, 32}, {"Deep Marauder", 44, 26, 46},
    }}, {"The Warren King", 58, 42, 220, true} },
}};

// The Bloodstained Road — three permanent, always-climbable ladders, one per
// notoriety color, ported from BLOODSTAINED_PATHS. Each is 5 tiers, the 5th a named
// boss; defeating a boss is a one-time unlock that opens a recurring weekly bounty
// for that path at the Hearthmoot (see kWeeklyGoals' *Bounty entries) — separate
// from, and never required for, the core Weekly Blessing.
struct BloodstainedTier { std::string label; float mult; bool isBoss = false; };
struct BloodstainedPathDef {
    std::string name, flavor;
    Color color;
    std::array<BloodstainedTier, 5> tiers;
};
static const std::array<BloodstainedPathDef, 3> kBloodstainedPaths = {{
    { "Bounty Hunter", "Hunt the wanted — Fame and Karma for cleaning up the road", {63, 126, 201, 255}, {{
        {"Petty Thief", 0.75f}, {"Highway Robber", 0.95f}, {"Hired Blade", 1.15f},
        {"Blood Warrant", 1.35f}, {"The Kingslayer", 1.6f, true},
    }} },
    { "Mercenary", "Fight for hire, then work the pockets — Stealing decides your real payday here",
      {122, 122, 122, 255}, {{
        {"Cutpurse Contract", 0.8f}, {"Smuggler's Job", 1.0f}, {"Fence's Target", 1.2f},
        {"Black Market Enforcer", 1.4f}, {"The Shadow Broker", 1.65f, true},
    }} },
    { "Outlaw", "Bigger risk, bigger reward — climbing this path raises your Notoriety",
      {201, 63, 63, 255}, {{
        {"Cutpurse Rival", 0.85f}, {"Blade for Hire", 1.05f}, {"Ruthless Duelist", 1.25f},
        {"Crime Lord's Enforcer", 1.45f}, {"The Kingpin", 1.7f, true},
    }} },
}};
enum BloodstainedPathIdx { kPathBlue = 0, kPathGray = 1, kPathRed = 2 };


// Item/quality/equipment system — ported from qualityFor()/craftItem()/equipItem() in
// the JS. An Item is what a Recipe becomes once crafted: same shape (power/type/slot/
// handed/category) plus a quality-scaled final power and a unique id.
// Bottom tier renamed "Flimsy" -> "Standard" 2026-09-21 (Mark's call — vendor-bought
// gear from the new Buy tabs is always this tier, and "Standard" reads better for a
// shop-bought item than "Flimsy" did). Thresholds/multipliers unchanged from
// QUALITY_TIERS. Mark separately floated renaming the whole ladder (fewer/renamed
// tiers) — deliberately NOT done here, out of scope for this pass.
const std::array<std::pair<int, std::pair<const char*, float>>, 6> kQualityTiersData = {{
    // max skill (exclusive), {label, power multiplier} — copied verbatim from QUALITY_TIERS
    {50, {"Standard", 0.5f}}, {70, {"Adeptly built", 0.7f}}, {90, {"Well-crafted", 0.85f}},
    {100, {"Master quality", 1.0f}}, {120, {"Grandmaster quality", 1.15f}}, {999999, {"Legendary quality", 1.3f}},
}};
static std::pair<std::string, float> QualityFor(float skillVal) {
    for (auto& tier : kQualityTiersData)
        if (skillVal < tier.first) return { tier.second.first, tier.second.second };
    return { "Legendary quality", 1.3f };
}

struct Item {
    int id;
    std::string name;
    ItemType type;
    std::string slot;    // armor only
    std::string handed;   // weapon only
    int power;
    std::string category; // weapon only: "Swordsmanship"/"Fencing"/"Macing"/"Archery" —
                            // drives which combat skill trains/applies (see ActiveWeaponSkillField)
};

// One weapon can occupy leftHand, rightHand, or both (2h); armor has one slot each.
struct Equipment {
    std::optional<Item> leftHand, rightHand;
    std::optional<Item> helmet, gorget, gloves, arms, legs, chest;
};

// ---------------------------------------------------------------------
// Magic / spellcasting — ported from SPELLS, spellSuccessChance(),
// spellPowerFor(), applySpellTraining(), rollSpellDisrupted(), and the
// combat-cast branch of playerAttackRoll()/combatHealSpell() in the JS.
//
// Simplifications from the original (flagged, not silently dropped):
//   - Only Offensive (damage) and Utility (the two Mending heal spells)
//     types are actually castable here. Debuff/Buff/Summon spells are
//     listed with real data but not wired into combat — those need the
//     status-effect and pet/summon systems this scaffold doesn't have.
//   - Magic Resistance isn't a trained stat here, so spell-disruption
//     chance uses the JS formula with magicResist fixed at 0 (a flat 30%
//     chance to disrupt a cast after being hit, instead of tapering down
//     as Magic Resistance trains).
//   - INT is fixed at its state.ts default (10) rather than trainable,
//     matching how STR/DEX are already handled — so max mana is a fixed
//     10 rather than growing over time.
// ---------------------------------------------------------------------

enum class SpellType { Offensive, Utility, Debuff, Buff, Summon };
struct Spell {
    std::string name;
    int circle;
    SpellType type;
    int minSkill, maxSkill; // Magery range for success-chance scaling
    int manaCost, reagentCost;
    int baseDamage; // also doubles as heal amount for Utility (heal) spells
};

// Copied verbatim from SPELLS in the JS (all 16, across 8 circles), except Circle
// 1's minSkill: was 10 in the JS (like every other circle's minSkill being
// (circle)*10), dropped to 0 here specifically so TryPracticeSpell's hard skill
// gate (added 2026-09-21, see its comment) doesn't lock a starting Magery-0
// character out of practicing anything at all — a deliberate deviation from the
// port, not a formula mismatch.
static const std::array<Spell, 16> kSpells = {{
    {"Spark Dart", 1, SpellType::Offensive, 0, 50, 4, 1, 4},
    {"Mending Word", 1, SpellType::Utility, 0, 50, 4, 1, 4},
    {"Sap Strength", 1, SpellType::Debuff, 0, 50, 4, 1, 4},
    {"Cloud Mind", 1, SpellType::Debuff, 0, 50, 4, 1, 4},
    {"Fumbling Curse", 1, SpellType::Debuff, 0, 50, 4, 1, 4},
    {"Wounding Touch", 2, SpellType::Offensive, 20, 60, 6, 1, 8},
    {"Blessing of Vigor", 3, SpellType::Buff, 30, 70, 9, 2, 12},
    {"Ember Burst", 3, SpellType::Offensive, 30, 70, 9, 2, 12},
    {"Venom Sting", 3, SpellType::Offensive, 30, 70, 9, 2, 8},
    {"Greater Mending", 4, SpellType::Utility, 40, 80, 11, 2, 16},
    {"Storm Lance", 4, SpellType::Offensive, 40, 80, 11, 2, 16},
    {"Psychic Shatter", 5, SpellType::Offensive, 50, 90, 14, 3, 20},
    {"Arc Bolt", 6, SpellType::Offensive, 60, 100, 20, 3, 24},
    {"Detonation", 6, SpellType::Offensive, 60, 100, 20, 3, 24},
    {"Inferno Strike", 7, SpellType::Offensive, 70, 110, 40, 4, 28},
    {"Summon Fiend", 8, SpellType::Summon, 80, 120, 50, 5, 32},
}};

enum class CombatPhase { PlayerTurn, Won, Lost };

// Which knight sprite-sheet is currently playing on the combat panel — Idle loops
// forever; every other value is a one-shot that reverts to Idle once its strip has
// played through once (see UpdateCombatAnim). Not persisted across save/load, same as
// the rest of CombatState.
enum class CombatAnim { Idle, Attack1, Attack2, Hurt, Defend, Protect };

struct CombatState {
    int dungeonIdx;
    int monsterHP;
    int monsterMaxHP;
    DungeonMonster monster; // copy, since it may be the boss (not in the .monsters array)
    std::vector<std::string> log; // last few lines, like state.combat.log in the JS
    CombatPhase phase = CombatPhase::PlayerTurn;
    bool playerWasHit = false; // JS state.combat.playerWasHit — feeds rollSpellDisrupted()
    float spellScroll = 0;      // mouse-wheel scroll offset for the in-combat spell list
    CombatAnim anim = CombatAnim::Idle;
    float animTime = 0.0f; // seconds elapsed in the current anim — drives frame index
    CombatAnim monsterAnim = CombatAnim::Idle; // only Idle/Attack1/Hurt are meaningful here
    float monsterAnimTime = 0.0f;

    void Log(const std::string& msg) {
        log.push_back(msg);
        if (log.size() > 6) log.erase(log.begin());
    }
};

// ---------------------------------------------------------------------
// Taming & pets — ported from WILD_CREATURES, tameChance(), petSlotCapacity(),
// resolveTameAttempt(), the pet's turn inside combatRound(), and
// healPet()/sellPet()/trainPetSkill() in the JS.
//
// Simplifications from the original (flagged, not silently dropped):
//   - Only the "wrestling or magery" attack branch is ported for a pet's
//     turn in combat; the JS's pet-targeting-vs-player-targeting split is
//     kept, but bleed/poison status effects a pet could apply are not
//     (those need the same status-effect system combat/magic already
//     flagged as out of scope).
//   - Pet mana regenerates continuously every frame here (like the
//     player's), rather than only being recalculated once per combat
//     round from a stored timestamp — same formula, simpler plumbing.
// ---------------------------------------------------------------------

enum class PetRole { Melee, Tank, Caster };
struct WildCreature {
    std::string name;
    int difficulty;
    PetRole role;
    std::array<int, 2> strRange, dexRange, intRange;
    bool isApex = false; // Forest Dragon: no skill cushion, full mastery required
};

// Copied verbatim from WILD_CREATURES in the JS.
static const std::array<WildCreature, 11> kWildCreatures = {{
    {"Stray Dog", 0, PetRole::Melee, {5,10}, {10,15}, {3,6}},
    {"Timber Wolf", 20, PetRole::Melee, {18,28}, {22,32}, {6,11}},
    {"Grizzly Bear", 30, PetRole::Tank, {35,50}, {10,15}, {5,10}},
    {"Dire Panther", 40, PetRole::Melee, {28,38}, {38,48}, {6,11}},
    {"Plains Bison", 50, PetRole::Tank, {45,60}, {12,18}, {5,10}},
    {"War Horse", 60, PetRole::Tank, {50,65}, {22,32}, {5,10}},
    {"Sabertooth Cat", 70, PetRole::Melee, {38,48}, {45,55}, {8,13}},
    {"Storm Griffin", 80, PetRole::Tank, {55,70}, {30,40}, {15,25}},
    {"Young Drake", 90, PetRole::Caster, {22,32}, {16,26}, {45,60}},
    {"Elder Wyvern", 100, PetRole::Caster, {25,35}, {18,28}, {50,65}},
    {"Forest Dragon", 100, PetRole::Caster, {30,40}, {20,30}, {60,80}, true},
}};

struct Pet {
    int id;
    std::string name;
    PetRole role;
    int str, dex, intStat;
    float hp, maxHp;
    float mana, maxMana;
    float wrestling = 0, tactics = 0, anatomy = 0, magery = 0, evalInt = 0, meditation = 0;
    bool active = false;
};

struct TamingAttempt {
    int creatureIdx;
    float secondsRemaining;
};

// ---------------------------------------------------------------------
// Game state — mirrors `let state = {...}` in the JS (trimmed to what
// this scaffold actually uses).
// ---------------------------------------------------------------------

struct UpgradeInProgress {
    std::string buildingKey;
    float secondsRemaining;
};

// Wilderness is deliberately not in the Tab-cycle order and has no tab-bar button —
// it's reached by walking to a gate at the edge of Town, not by clicking a tab, so
// it's excluded wherever the other 8 screens are enumerated for that UI.
enum class Screen { Character, Town, Hunt, Craft, Magic, Pets, Bank, House, Skills, Wilderness, Provisioner };

struct Corpse {
    std::string monsterName;
    int baseLeather;
    int gold;
};

// A stack of one brewed potion type — mirrors state.potions[name] = {count,effect,potency}.
struct PotionStack {
    std::string name;
    std::string effect; // "heal" | "stamina" | "poison" | "damage"
    int potency;
    int count;
};

// JS WEEKLY_GOALS — fixed 5-goal list, indices used throughout instead of string keys.
// JS: WEEKLY_GOALS (the first 5 — count toward the Weekly Blessing) plus
// BLOODSTAINED_WEEKLY_BONUSES (the last 3 — separate, path-specific, never
// required for the Blessing). kCoreWeeklyGoalCount marks the boundary.
enum WeeklyGoalIdx {
    kGoalGather = 0, kGoalCraft, kGoalBrew, kGoalDefeat, kGoalTame,
    kGoalBlueBounty, kGoalGrayBounty, kGoalRedBounty, kWeeklyGoalCount
};
static const int kCoreWeeklyGoalCount = 5;
struct WeeklyGoalDef { const char* key; const char* label; int target; int reward; };
static const std::array<WeeklyGoalDef, kWeeklyGoalCount> kWeeklyGoals = {{
    {"gather", "Gather 600 wood or ore", 600, 150},
    {"craft", "Craft 20 weapons or armor", 20, 150},
    {"brew", "Brew 12 potions", 12, 150},
    {"defeat", "Defeat 50 monsters or murderers", 50, 150},
    {"tame", "Tame 2 creatures", 2, 150},
    {"blueBounty", "Blue Path: win 3 Bounty Hunter fights", 3, 200},
    {"grayBounty", "Gray Path: win 3 Mercenary contracts", 3, 200},
    {"redBounty", "Red Path: win 3 Outlaw fights", 3, 200},
}};
static const long long kWeekSeconds = 7LL * 24 * 60 * 60;

struct GameState {
    int gold = 100;
    int wood = 10;
    int ore = 0;
    int leather = 5;

    // current level (1-5) per craftable building, indexed same as kCraftBuildings
    std::array<int, kCraftBuildings.size()> buildingLevel = {1, 1, 1, 1};

    std::optional<UpgradeInProgress> upgrading;   // only one upgrade at a time
    std::optional<std::string> gatheringResource; // "wood" | "ore"
    float gatherSecondsRemaining = 0.0f;

    std::optional<std::string> selectedTile; // key of selected grid tile (any tile)
    // Which kTownNPCs index is currently greeted (open name+greeting popup), if any —
    // transient UI state, not saved, same as selectedTile above.
    std::optional<int> greetedNPC;
    // Second town (2026-09-22, "second town" plan) — 0 = Town 1 (existing), 1 = Town 2.
    // Reuses Town 1's exact layout/collision/roads (see DrawTownScreen); only the
    // building tint/texture, ground texture, and NPC flavor differ per town, per the
    // confirmed shared-economy design (same kCraftBuildings/Bank/Pets underneath either
    // way — FindCraftBuildingIndex etc. are keyed by building type, never by town).
    int selectedTown = 0;

    std::string logLine = "Welcome to Town Forge.";

    // --- Combat/dungeon state (see the section above for what's simplified) ---
    Screen screen = Screen::Character; // matches the HTML's default/first tab
    int str = 50, dex = 20;                 // starting defaults (Mark's own numbers, not the
                                               // JS's 10/10/10 — a "finished" character is meant
                                               // to land around 100/100/60 or 100/90/70); grow via
                                               // MaybeGainStat, capped at kStatCapIndividual/Total
    int maxHp = 50;                           // literal STR=HP (Mark's call, overriding the
                                               // JS's "50 + str" maxHP() formula) — kept in
                                               // sync with str by MaybeGainStat since this is
                                               // a stored field here, not a live function
    int hp = 50;
    std::array<int, kDungeons.size()> dungeonXP = {0, 0, 0, 0, 0};
    std::optional<int> selectedDungeon;      // index into kDungeons
    std::optional<CombatState> combat;

    // --- Crafting/equipment state — mirrors state.smithSkill/carpSkill/tailorSkill,
    // state.backpack, and state.equipped in the JS ---
    std::array<float, kCraftBuildings.size()> buildingSkill = {0, 0, 0, 0}; // smith/carpenter/tailor/(alchemy unused)
    std::vector<Item> backpack;
    int nextItemId = 1;
    Equipment equipped;
    int craftBuildingTab = 0;   // which of kCraftBuildings is open on the Craft screen
    int craftModeTab = 0;        // 0 = Craft (existing UI), 1 = Buy pre-made gear (2026-09-21)
    float craftScroll = 0;       // mouse-wheel scroll offset for the recipe list
    // Transient UI state for the live-combat spell hotbar's assignment picker (2026-09-22)
    // — which slot (0-4, into combatHotbar) is currently open for reassignment, if any.
    // Not saved, same as the other transient UI fields on this line.
    std::optional<int> hotbarPickerSlot;
    float backpackScroll = 0;    // mouse-wheel scroll offset for the backpack list
    int provisionerTab = 0;      // 0 = Buy, 1 = Sell (Screen::Provisioner, added 2026-09-21)

    static const int kBackpackCap = 20; // JS BASE_BACKPACK_CAP — see BackpackCap(s) for the
                                          // house-tier bonus on top of this (now ported, see
                                          // kHouseTiers)

    // --- Player housing — ported from the JS's HOUSE_TIERS/HOUSE_HUES/HOME_MODULE_*
    // (state.house/state.houseModules) — see kHouseTiers/kHouseHues/kHomeModuleDefs. ---
    int houseTierIdx = 0;          // index into kHouseTiers; 0 = "none", no house yet
    int houseHue = -1;              // index into kHouseHues; -1 = no hue chosen (base tint)
    std::string houseName;
    std::array<int, 4> houseModuleLevel = {0, 0, 0, 0}; // parallel to kHomeModuleDefs; 0 = not built

    // --- Gathering skills / auto-gather — mirrors state.lumberjacking/mining/skinning
    // and state.autoGather in the JS ---
    float lumberjacking = 0, mining = 0, skinning = 0; // trade skills, capped at 120
    bool autoGather = false;
    std::vector<Corpse> corpses; // left behind by combat wins, skinned for leather+gold

    // --- Magic / spellcasting — mirrors state.magery/evalInt/meditation/mana/reagents ---
    float magery = 0, evalInt = 0, meditation = 0; // capped at 120
    int intStat = 20; // starting default (Mark's own number, not the JS's 10); grows via MaybeGainStat
    float mana = 20;   // starts at the real cap (= intStat), same reasoning as before this
                          // was changed from the JS's inconsistent starting numbers
    int reagents = 5;
    int magicScreenTab = 0;      // 0 = Spellcraft practice list
    float magicScroll = 0;        // mouse-wheel scroll offset for the practice spell list

    // --- The Character page — mirrors state.characterName/titleLordEarned and the
    // titleFor()/karmaAdjective()/topVocationTitle()/characterDisplayName() naming
    // system (titleLordEarned itself already exists in the Notoriety section below).
    std::string characterName;

    // --- Taming & pets — mirrors state.animalTaming/animalLore/veterinary/pets ---
    float animalTaming = 0, animalLore = 0, veterinary = 0; // capped at 120
    std::vector<Pet> pets;
    int nextPetId = 1;
    std::optional<TamingAttempt> tamingAttempt;
    float petsScroll = 0;    // mouse-wheel scroll offset for the pet roster list
    float creatureScroll = 0; // mouse-wheel scroll offset for the wild-creature list
    // AI companion (2026-09-22, "AI players" plan Part 2) — the active Pet's world
    // position on the Wilderness/dungeon screens. Not saved, same as the player
    // position fields below (purely runtime, resets on relaunch); snapped rather than
    // eased in on first use or after a screen change, see UpdateCompanionFollow.
    Vector2 companionPos = {0, 0};
    bool companionFollowInitialized = false;
    float companionAttackCooldown = 0.0f;

    // The Rival Adventurer (2026-09-23, "Rival hunts you" plan) — no longer a static
    // kWildernessMonsterSpots row, since it now roams and grows persistently instead of
    // sitting at one fixed spot forever. rivalLevel/rivalPos/rivalHasBeatenPlayer are
    // saved (see SaveGame/LoadGame — offline growth catch-up reuses the existing
    // lastActiveEpoch/elapsed calculation already there for ApplyOfflineAutoGather,
    // rather than tracking a second, redundant epoch just for the Rival); the rest is
    // transient, same reasoning as companionPos above.
    float rivalLevel = 16.0f; // starting value matches the old static spot's level
    Vector2 rivalPos = { 900, 900 }; // the old spot's position, now just a starting point
    bool rivalHasBeatenPlayer = false; // once true, losing to it again is a harsher "murderer" loss
    enum class RivalActivity { Patrol, Hunting };
    RivalActivity rivalActivity = RivalActivity::Patrol;
    Vector2 rivalPatrolTarget = { 900, 900 };
    float rivalActivityTimer = 0.0f; // counts down to the next patrol-target pick, or the next hunt attempt

    // --- Notoriety, murderers & innocents — mirrors state.notoriety/fame/karma/
    // shaken/ambush/innocentEncounter in the JS ---
    float notoriety = 0;
    float fame = 0, karma = 0;
    bool titleLordEarned = false;
    int shaken = 0; // fights remaining with -15% combat power (see IsShaken())
    float stealing = 0, snooping = 0; // capped at 120

    struct AmbushEncounter { std::string name; int level; };
    std::optional<AmbushEncounter> ambush;

    struct InnocentEncounter { std::string name; int gold; bool canSteal = false; std::string source; };
    std::optional<InnocentEncounter> innocentEncounter;

    // Per-spot runtime state for the roaming Innocent NPCs (2026-09-23) — parallel to
    // kWildernessInnocentSpots by index. Transient (not saved), same reasoning as
    // companionPos/rivalActivity — respawns fresh each session rather than trying to
    // preserve "who was standing where" across a save. `present=false` with
    // `respawnTimer=0` on a fresh GameState means every spot rolls its first traveler
    // immediately on entering the Wilderness rather than waiting out a cooldown that
    // never actually started.
    struct InnocentSpotState { bool present = false; std::string name; int gold = 0; float respawnTimer = 0.0f; };
    std::array<InnocentSpotState, 4> innocentSpots;

    std::string pendingEncounterCheck; // "gather" | "" — see UpdateGathering + main()

    // --- Alchemy potions — mirrors state.potions/state.poisoning/state.weaponPoisoned.
    // buildingSkill[3] (already declared above) is Alchemy's skill, now put to use. ---
    std::vector<PotionStack> potions;
    float poisoning = 0; // capped at 120
    int weaponPoisonCharges = 0, weaponPoisonPotency = 0;

    // --- Banking (the Vaultkeep) — mirrors state.bank.gold/state.bank.items. Safe from
    // every danger system above: murderer losses and guard-zone confiscation never
    // touch these. ---
    int bankGold = 0;
    std::vector<Item> bankItems;
    float bankScroll = 0; // mouse-wheel scroll offset for the bank's backpack-side list
    float bankItemsScroll = 0; // mouse-wheel scroll offset for the bank's own item list
    float houseScroll = 0; // mouse-wheel scroll offset for the House screen's workshop-wing list

    // --- The Hearthmoot's weekly goals — mirrors state.weeklyGoals. Tracked by
    // elapsed real time (epoch seconds), not a server calendar, matching the JS. ---
    std::array<int, kWeeklyGoalCount> weeklyProgress = {0, 0, 0, 0, 0, 0, 0, 0};
    std::array<bool, kWeeklyGoalCount> weeklyClaimed = {false, false, false, false, false, false, false, false};
    long long weekStartEpoch = 0; // 0 = not yet initialized; set on first CheckWeeklyReset()
    bool blessingClaimedThisWeek = false;
    long long blessingUntilEpoch = 0; // hasWeeklyBlessing(): now < this

    // --- Weapon-category combat skills + Tactics/Anatomy/Magic Resistance/Healing —
    // mirrors state.weaponSkills/tactics/anatomy/magicResist/healing. All 18 of these
    // (5 weapon skills + these 4 + Magery/EvalInt/Meditation + Taming/Lore/Vet +
    // Stealing/Snooping/Poisoning) share the Echo system's 700-point active budget
    // below — trade skills (Lumberjacking..Alchemy) are NOT part of it and stay
    // always-active, matching the JS exactly. ---
    float swordsmanship = 0, fencing = 0, macing = 0, archery = 0, wrestling = 0;
    float tactics = 0, anatomy = 0, magicResist = 0, healing = 0; // capped at 120 each

    // --- Bandages — mirrors state.bandages. A plain consumable count, not a backpack
    // item; crafted by the Tailor or bought from the Provisioner stand-in (see
    // "Bandages & Healing" below), and trains the Healing skill above on use. ---
    int bandages = 3;

    // --- The Echo system — mirrors state.skillActive. true = contributing to
    // gameplay right now; false = benched (still fully trained, just inactive).
    // Indexed by WeeklyGoalIdx... no — indexed by position in kCappedSkills below. ---
    std::array<bool, 18> skillActive = { true, true, true, true, true, true, true, true,
                                          true, true, true, true, true, true, true, true, true, true };

    // --- The Bloodstained Road — mirrors state.bloodstainedProgress/bloodstainedLoop/
    // bloodstainedBossDefeated/grayEncounter. Tier index 0-4 = current rung; reaching
    // 5 (i.e. having beaten tier 4, the boss) loops back to 0 with loop+1 and marks
    // the boss permanently defeated. ---
    std::array<int, 3> bloodstainedProgress = {0, 0, 0};
    std::array<int, 3> bloodstainedLoop = {0, 0, 0};
    std::array<bool, 3> bloodstainedBossDefeated = {false, false, false};

    // Configurable live-combat spell hotbar (2026-09-22) — indices into kSpells, -1 =
    // empty slot. Real player configuration (unlike the transient UI-tab fields below),
    // so it's saved/loaded like any other persistent field.
    std::array<int, 5> combatHotbar = {-1, -1, -1, -1, -1};

    struct GrayEncounter { std::string name; int level, baseGold, tierIdx; bool canSteal = false; int previewGold = 0; };
    std::optional<GrayEncounter> grayEncounter;

    int huntSubView = 0; // 0 = Dungeons, 1 = Bloodstained Road — sub-tab within the Hunt screen

    // --- Free-movement exploration — the player's world position in each explorable
    // space. Only one is "active" at a time depending on state.screen/selectedDungeon,
    // but both persist independently so leaving and returning keeps your spot. ---
    Vector2 townPlayerPos = {450, 600}; // on the main north-south street, clear of Townhall/Bank's collision radius
    Vector2 dungeonPlayerPos = {900, 900};
    Vector2 bloodstainedPlayerPos = {450, 700};
    Vector2 wildernessPlayerPos = {900, 1650}; // just inside the gate from Town
    Vector2 playerFacing = {0, 1}; // last nonzero movement direction, for a facing indicator
    float worldTime = 0; // elapsed seconds, ticks every frame — drives monster wander motion

    // --- Live Wilderness combat (first slice of the real-time combat rework — see
    // kWildernessMonsterSpots/DrawWildernessScreen for the rest). Unlike every other
    // monster in the game, an engaged Wilderness monster needs real per-instance state
    // (it moves and has its own HP) instead of being derived statelessly from
    // worldTime. Exactly one can be engaged at a time; the other 4 keep idle-wandering
    // exactly as before. Dungeon monsters and ambushes are untouched, still the older
    // panel-based state.combat system. ---
    struct ActiveMonster {
        int spotIdx; // index into kWildernessMonsterSpots
        Vector2 pos, spawnPos;
        float hp, maxHp;
        float monsterAttackCooldown = 0.0f; // counts down; monster can swing when <= 0
        float playerAttackCooldown = 0.0f;  // counts down; player can swing when <= 0
        // Separate from playerAttackCooldown so melee and magic don't share one clock —
        // a flat cast time (kWildSpellCastCooldown), not DEX-scaled like the sword swing.
        float playerSpellCooldown = 0.0f;
        // Purely visual, not gameplay — see kSwingEffectDuration. Ticks down independently
        // of playerAttackCooldown (which can be much longer/shorter depending on DEX) so
        // the flash duration stays consistent regardless of swing speed.
        float swingEffectTimer = 0.0f;
        // Same idea as swingEffectTimer but for spellcasting (2026-09-23, once the hero
        // sheet got a real Cast pose) — set alongside playerSpellCooldown, drives
        // DrawPlayer's ActorAnim::Cast selection while live.
        float castEffectTimer = 0.0f;
        // Used only by the one tactical opponent — harmless unused defaults for every
        // other monster.
        bool isFleeing = false;
        float fleeTimer = 0.0f;
        float monsterSpecialCooldown = 0.0f; // the opponent's own ranged-strike cooldown
        // True for the Rival Adventurer specifically (2026-09-23) — it no longer has a
        // kWildernessMonsterSpots row at all (see GameState::rivalLevel's comment), so
        // `spotIdx` is meaningless for it (left at -1) and every lookup that used to go
        // through kWildernessMonsterSpots[spotIdx] checks this flag first instead.
        bool isRival = false;
    };
    std::optional<ActiveMonster> wildEngaged;

    // --- Live dungeon combat (2026-09-22 — porting the Wilderness pattern above to all
    // 5 dungeons, per the "Live combat for Wilderness + all 5 dungeons" plan). Same
    // shape as ActiveMonster; kept as a separate struct/field rather than reused since
    // dungeon monsters are indexed 0-4 (+5=boss) within the *current* selectedDungeon,
    // not into a flat global spot array like kWildernessMonsterSpots. Ambushes/murderer
    // fights/Bloodstained Road are untouched, still the older panel-based state.combat. ---
    struct ActiveDungeonMonster {
        int monsterIdx; // 0-4 = dungeon.monsters[idx], 5 = dungeon.boss (see isBoss)
        bool isBoss;
        Vector2 pos, spawnPos;
        float hp, maxHp;
        float monsterAttackCooldown = 0.0f;
        float playerAttackCooldown = 0.0f;
        float playerSpellCooldown = 0.0f;
        float swingEffectTimer = 0.0f;
        float castEffectTimer = 0.0f;
    };
    std::optional<ActiveDungeonMonster> dungeonEngaged;
};

// ---------------------------------------------------------------------
// Free-movement exploration — a small top-down world layer over the Town
// and Hunt (dungeon) screens. This has no equivalent in the original JS,
// which was entirely menu/tab based; it's new territory built to give
// the player a character that actually walks around, per request.
//
// Both explorable spaces share one coordinate system: a world rectangle
// larger than the visible viewport, a camera that follows the player and
// clamps to the world edges, and proximity-based interaction (walk up to
// something, press E) instead of clicking a button for it. Once an
// interaction opens a panel (a building's upgrade panel, a combat panel),
// movement pauses until it closes — the world is still visible underneath
// but input goes to the panel instead.
// ---------------------------------------------------------------------

static const float kPlayerSpeed = 220.0f;   // world units/sec
static const float kPlayerRadius = 17.0f;     // bumped from 14 for a less cramped, more legible view
static const float kInteractRange = 54.0f;   // distance at which "[E] interact" becomes available
static const float kNodeRadius = 50.0f;       // world/building/monster node radius — bumped from 40;
                                                // moved up here
                                                // (rather than by its original kTownNodePositions
                                                // table further down) since DrawBuildingNode needs
                                                // it and is defined earlier in the file
static const float kPlayerEdgeMargin = 70.0f; // how close the player's world position can get to any
                                                // world edge — bigger than kPlayerRadius (the actual
                                                // collision hitbox) because the hero sprite is drawn
                                                // much larger than that hitbox (kHeroSpriteScale); a
                                                // margin as small as kPlayerRadius let the sprite's top
                                                // clip past the viewport into the HUD near a world edge.
                                                // Scaled up 45->56->70 (same 1.25x as kHeroSpriteScale
                                                // each time) as the sprite itself grew 25% twice, to
                                                // keep the same buffer.
static const float kWorldSize = 900.0f;       // Bloodstained Road world size (Town used to share
                                                // this too — see kTownWorldSize below for why it
                                                // doesn't anymore)
// Town's own world size, separate from kWorldSize above (2026-09-21: Mark asked for buildings
// spaced further apart; bumping the shared kWorldSize would have also grown the Bloodstained
// Road, which wasn't asked for — same reasoning as kTownVisualScale not touching kNodeRadius).
static const float kTownWorldSize = 1000.0f;
static const float kDungeonWorldSize = 1800.0f; // dungeons get their own, much bigger world — real
                                                   // room-and-corridor space to actually walk and explore
// Wilderness's own world size, separate from kDungeonWorldSize above (2026-09-22,
// "second town" plan) — Mark wants the walk from Town 1 to the new Town 2 to feel like
// a real journey, not an instant blip. Bumping kDungeonWorldSize directly would also
// enlarge every dungeon interior's usable bounds, which wasn't asked for — same
// reasoning as kTownWorldSize being split out from the shared kWorldSize earlier this
// session. All of Wilderness's existing content (gather/tame/monster nodes, the 5
// dungeon entrances, foliage, the Town 1 gate) keeps its original 0-1800 coordinates
// unchanged; the extra space is new territory toward Town 2's gate.
static const float kWildernessWorldSize = 3200.0f;
// Town 2's name and its gate position out in the newly added Wilderness space — a
// straight-line ~2000 units from the Town 1 return gate (kWildernessReturnGatePos,
// {900,1750}), well past the original 1800-unit map's edge, so reaching it is a real
// walk. Declared here (rather than near kTown2NPCs) since DrawWildernessScreen needs it
// and is defined well before that point in the file.
static const char* kTown2Name = "Saltmere";
static const Vector2 kWildernessTown2GatePos = { 2900, 1750 };
// Mark asked for "everything in Town a little larger" since the camera scrolls with
// the player anyway — rather than bumping kNodeRadius/kPlayerRadius/kWorldSize above
// (which would also resize Hunt's dungeons and the Bloodstained Road, neither of which
// was asked for), this scales only Town's own visual draw sizes (buildings, ground
// tile, fences, foliage, props, the player sprite) via optional scale parameters on
// DrawBuildingNode/DrawPlayer and by multiplying Town's own DrawTiledGround/
// DrawWallBand/DrawIconCentered call sites. World positions/spacing
// (kTownNodePositions, kTownPlaza) are untouched — buildings are 250 units apart on
// the grid with ~155 units of clearance today, comfortably more than the ~14px this
// adds to each building's footprint, so nothing overlaps.
static const float kTownVisualScale = 1.15f;
                                                   // instead of the small arena Town/Bloodstained still use
static const Rectangle kViewport = { 0, 110, 540, 790 }; // screen-space area the world renders into

static Vector2 ClampToWorld(Vector2 p, float margin, float worldSize = kWorldSize) {
    return { std::clamp(p.x, margin, worldSize - margin), std::clamp(p.y, margin, worldSize - margin) };
}
// Top-left of the camera in world space: centers the player in the viewport, clamped
// so the camera never shows past the world's edge. `worldSize` defaults to Town/
// Bloodstained's shared size; dungeons pass kDungeonWorldSize explicitly.
static Vector2 CameraTopLeft(Vector2 playerPos, float worldSize = kWorldSize) {
    Vector2 topLeft = { playerPos.x - kViewport.width / 2.0f, playerPos.y - kViewport.height / 2.0f };
    topLeft.x = std::clamp(topLeft.x, 0.0f, worldSize - kViewport.width);
    topLeft.y = std::clamp(topLeft.y, 0.0f, worldSize - kViewport.height);
    return topLeft;
}
static Vector2 WorldToScreen(Vector2 worldPos, Vector2 cameraTopLeft) {
    return { kViewport.x + worldPos.x - cameraTopLeft.x, kViewport.y + worldPos.y - cameraTopLeft.y };
}

// ---------------------------------------------------------------------
// Touch/mouse-drag virtual joystick — a WASD alternative for builds with no keyboard
// (the web/mobile build in particular: raylib's Emscripten GLFW3 layer maps a single
// touch to the left mouse button, so this works unmodified on a phone). Touching down
// inside kJoystickZone (bottom-left corner of the viewport, clear of every screen's
// buttons since movement is only ever live when no panel/combat UI is open) starts a
// drag; the offset from that start point becomes the move direction, capped at
// kJoystickMaxDrag. A single knob rather than 4 D-pad buttons specifically because a
// single touch point can't hold multiple buttons at once for diagonal movement.
// Input (VirtualJoystickDir) and drawing (DrawVirtualJoystick) are split because the
// input has to be read before movement is resolved, but the drawing has to happen
// after the world is drawn (else the ground/buildings would paint over it) — see the
// DrawVirtualJoystick() call near the end of each movement screen's draw function.
// ---------------------------------------------------------------------
static const float kJoystickMaxDrag = 50.0f;
static const Rectangle kJoystickZone = { 0, kViewport.y + kViewport.height - 170.0f, 170.0f, 170.0f };
static Vector2 g_joystickOrigin = { 0, 0 };
static Vector2 g_joystickCurrent = { 0, 0 };
static bool g_joystickActive = false;

static Vector2 VirtualJoystickDir() {
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, kJoystickZone)) {
        g_joystickActive = true;
        g_joystickOrigin = mouse;
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) g_joystickActive = false;
    g_joystickCurrent = mouse;
    if (!g_joystickActive) return { 0, 0 };
    Vector2 delta = { mouse.x - g_joystickOrigin.x, mouse.y - g_joystickOrigin.y };
    float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (len < 8.0f) return { 0, 0 }; // dead zone — avoids jitter right at the touch point
    return { delta.x / len, delta.y / len };
}
// Mirrors VirtualJoystickDir()'s own dead-zone check using the globals it just updated
// this frame (called from UpdatePlayerMovement before this runs) — lets DrawPlayer's
// walk-animation gate see touch-driven movement too. Needed because AnyMoveKeyDown()
// below only checks physical keys: on a phone with no keyboard, that always returned
// false, so the player visibly moved via the on-screen joystick but the walk animation
// never advanced past its standing frame — reported as "feet don't move on my phone."
static bool VirtualJoystickIsMoving() {
    if (!g_joystickActive) return false;
    Vector2 delta = { g_joystickCurrent.x - g_joystickOrigin.x, g_joystickCurrent.y - g_joystickOrigin.y };
    return std::sqrt(delta.x * delta.x + delta.y * delta.y) >= 8.0f;
}
// Call after the world/viewport has already been drawn (outside any active scissor),
// so this always renders on top instead of underneath the ground/buildings. A no-op
// (and cheap to call unconditionally every frame) whenever the joystick isn't active.
static void DrawVirtualJoystick() {
    if (!g_joystickActive) return;
    Vector2 delta = { g_joystickCurrent.x - g_joystickOrigin.x, g_joystickCurrent.y - g_joystickOrigin.y };
    float len = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (len > kJoystickMaxDrag) { delta.x = delta.x / len * kJoystickMaxDrag; delta.y = delta.y / len * kJoystickMaxDrag; }
    DrawCircleV(g_joystickOrigin, 54.0f, Fade(BLACK, 0.18f));
    DrawCircleV({ g_joystickOrigin.x + delta.x, g_joystickOrigin.y + delta.y }, 26.0f, Fade(RAYWHITE, 0.7f));
}

// Moves `pos` per WASD/arrow keys held this frame (falling back to the virtual
// joystick above if no key is held), updates `facing` if actually moving, and clamps
// to the world. Returns true if the player moved at all this frame.
static bool UpdatePlayerMovement(Vector2& pos, Vector2& facing, float dt, float worldSize = kWorldSize) {
    Vector2 dir = {0, 0};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) dir.y -= 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) dir.y += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) dir.x -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) dir.x += 1;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len <= 0.0001f) {
        dir = VirtualJoystickDir();
        len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len <= 0.0001f) return false;
    } else {
        dir.x /= len; dir.y /= len;
    }
    pos.x += dir.x * kPlayerSpeed * dt;
    pos.y += dir.y * kPlayerSpeed * dt;
    pos = ClampToWorld(pos, kPlayerEdgeMargin, worldSize);
    facing = dir;
    return true;
}
static float Dist(Vector2 a, Vector2 b) { return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)); }

// ---------------------------------------------------------------------
// World art — optional. Every draw call below falls back to the original
// colored-circle placeholder if a texture failed to load (missing file,
// bad path, etc.), so a partially-filled assets/ folder never crashes or
// blanks anything out — it just shows circles for whatever's missing.
//
// Expected files (all optional, checked individually):
//   assets/player.png                     — the player character (single
//                                            forward-facing sprite; swap in
//                                            directional frames later by
//                                            loading more textures and
//                                            picking one from s.playerFacing)
//   assets/buildings/<key>.png            — one per Town building; <key> is
//                                            smith/carpenter/tailor/alchemy/
//                                            provisioner/stable/healer/bank/
//                                            townhall
//   assets/monsters/emberveil.png         — Emberveil Hollow (dungeon 0), regular monsters
//   assets/monsters/bloodtusk.png         — Bloodtusk Hold (dungeon 1), regular monsters
//   assets/monsters/sunkencrypt.png       — The Sunken Crypt (dungeon 2), regular monsters
//   assets/monsters/wyrmscar.png          — Wyrmscar Depths (dungeon 3), regular monsters
//   assets/monsters_boss/<same 4 names>   — that dungeon's boss specifically (falls back
//                                            to the regular-monster texture, gold-tinted,
//                                            if a boss-specific file isn't found)
//   assets/ground/grass.png, dirt.png     — Town's tiled ground + plaza
//   assets/dungeon/floor.png, wall.png    — generic dungeon look (fallback if a themed
//                                            texture below isn't found)
//   assets/dungeon_themed/<theme>_floor.png, <theme>_wall.png — per-dungeon theming;
//                                            <theme> is emberveil/bloodtusk/sunkencrypt/
//                                            wyrmscar, same order as kDungeons
//   assets/buildings/door.png             — shared door art for every Town building
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// Paper doll — layered character art. Originally built entirely from the
// CC0-licensed Dungeon Crawl Stone Soup tileset (assets/paperdoll/, curated
// from the full pack under assets/dungeon crawl/ — see its LICENSE.txt:
// public domain, no attribution required), pre-aligned 32x32 equipment
// layers (bare body, then boots/legs/chest/head/glove/weapon overlays).
// Still true for Helmet/weapons (kept temporarily even though they no longer
// align — see DrawPaperdollLayers' comment).
//
// 2026-09-21: Mark asked for higher-quality paperdoll art across the board.
// base/hair/boots/Arms/Gorget/Chest/Legs/Gloves are now Gemini-generated art
// (originally on a 64x64 canvas, doubled to 128x128 same day after Mark
// reported the result still looked blurry — the blur was downscaling
// ~600-1000px source art all the way down to 64px; rebuilt every piece above
// from the same already-cleaned source crops at 2x the target size/position,
// no new art needed) — DrawTexturePro stretches every layer to the same
// destination square regardless of native resolution, so mixing canvas sizes
// is fine, but pieces MUST be authored against the same reference body
// proportions to land correctly (confirmed
// the hard way: the new taller body with angled-out arms does not line up
// with the old compact/arms-close-to-torso DCSS pieces at all). The new
// base was itself built from 3 separate Gemini renders (bald+barefoot,
// +hair, +hair+boots) rather than isolated transparent layers — hair/boots
// were extracted by isolating their distinct color region (hue/luminance
// threshold) from each full-body render, not by diffing images against each
// other (confirmed those 3 renders aren't pixel-aligned to each other, only
// close). Chest/Legs/Gloves are now done too, all generated against the new
// base as the alignment reference. Next up: Helmet (4 materials), then
// weapons.
//
// Each entry is keyed either by a fixed slot name ("base"/"hair"/"boots" —
// always drawn, no game slot backs them) or by the exact Recipe::name of a
// craftable weapon/armor item, so finding the layer for an equipped item
// is just a name lookup (see FindPaperdollTexture/DrawPaperdollLayers
// below).
// ---------------------------------------------------------------------
static const std::vector<std::pair<std::string, std::string>> kPaperdollManifest = {
    {"base", "base/human_male.png"},
    {"hair", "hair/brown_1.png"},
    {"boots", "boots/middle_brown.png"},
    // Helmets — all 4 materials now have new 128x128 Gemini art (2026-09-21).
    // Studded Cap deliberately reuses Leather Cap's art (Mark's call — no separate
    // studded-leather helm generated).
    {"Leather Cap", "head/hood_ybrown.png"},
    {"Studded Cap", "head/hood_ybrown.png"},
    {"Plate Helm", "head/iron_1.png"},
    {"Chain Coif", "head/chain.png"},
    // Gloves
    {"Ring Mail Gloves", "gloves/glove_gray.png"},
    {"Chainmail Gloves", "gloves/gauntlet_blue.png"},
    {"Plate Gloves", "gloves/glove_gold.png"},
    {"Leather Gloves", "gloves/glove_brown.png"},
    {"Studded Gloves", "gloves/glove_black.png"},
    // Legs
    {"Ring Mail Leggings", "legs/leg_armor_1.png"},
    {"Chainmail Leggings", "legs/leg_armor_2.png"},
    {"Plate Legs", "legs/leg_armor_4.png"},
    {"Leather Leggings", "legs/pants_brown.png"},
    {"Studded Leggings", "legs/pants_black.png"},
    // Chest
    {"Ring Mail Tunic", "body/ringmail.png"},
    {"Chain Tunic", "body/chainmail.png"},
    {"Plate Chest", "body/plate.png"},
    {"Leather Tunic", "body/leather_armor.png"},
    {"Studded Tunic", "body/leather_stud.png"},
    // Arms/Gorget — Gemini-generated (not from the DCSS pack, which has no
    // arms/gorget layer at all), downscaled from the original ~600x1000
    // painterly art onto a 128x128 canvas (4x this pack's native 32x32 —
    // DrawTexturePro stretches every layer to the same destination square
    // regardless of its source resolution, so mixing canvas sizes across
    // layers is safe).
    {"Ring Mail Sleeves", "arms/ringmail.png"},
    {"Chainmail Sleeves", "arms/chainmail.png"},
    {"Plate Arms", "arms/plate.png"},
    {"Studded Sleeves", "arms/studded.png"},
    {"Leather Sleeves", "arms/leather.png"},
    {"Plate Gorget", "gorget/plate.png"},
    {"Leather Gorget", "gorget/leather.png"},
    {"Studded Gorget", "gorget/studded.png"},
    // Weapons — Smith (Swordsmanship/Fencing/Macing)
    {"Cutlass", "hand_right/sabre.png"},
    {"Scimitar", "hand_right/scimitar_new.png"},
    {"Katana", "hand_right/katana.png"},
    {"Longsword", "hand_right/long_sword.png"},
    {"Broadsword", "hand_right/broadsword.png"},
    {"Bardiche", "hand_right/glaive_new.png"},
    {"Viking Sword", "hand_right/heavy_sword.png"},
    {"Halberd", "hand_right/halberd_new.png"},
    {"Kryss", "hand_right/rapier.png"},
    {"Pitchfork", "hand_right/fork_2.png"},
    {"Short Spear", "hand_right/spear_1.png"},
    {"War Fork", "hand_right/pole_forked.png"},
    {"Spear", "hand_right/spear.png"},
    {"Club", "hand_right/club.png"},
    {"Mace", "hand_right/mace_new.png"},
    {"Maul", "hand_right/large_mace.png"},
    {"War Axe", "hand_right/war_axe_new.png"},
    {"War Hammer", "hand_right/great_mace.png"},
    // Weapons — Carpenter (Macing staves / Archery)
    {"Shepherd's Crook", "hand_right/staff_plain.png"},
    {"Gnarled Staff", "hand_right/staff_organic.png"},
    {"Quarterstaff", "hand_right/quarterstaff.png"},
    {"Black Staff", "hand_right/staff_evil.png"},
    {"Short Bow", "hand_right/bow.png"},
    {"Bow", "hand_right/bow_2.png"},
    {"Composite Bow", "hand_right/bow_3.png"},
    {"Crossbow", "hand_right/crossbow.png"},
    {"Heavy Crossbow", "hand_right/crossbow_3.png"},
};

// ---------------------------------------------------------------------
// Backpack-list item icons — real classic UO art (CorvaeOboro's "ultima_online_mods",
// CC0 1.0, see the spell-icon comment above GameAssets.spellIconPerSpell for the same
// source/license). Unlike the paperdoll manifest above (exact-name keyed, and — found
// while building this — apparently never actually matches since equipped items' names
// always carry a quality prefix like "Fine Broadsword", not the bare "Broadsword" the
// manifest keys expect; a pre-existing latent bug, not fixed here, out of scope for
// this pass), GearIconForItem below does a *suffix* match against these keys so the
// quality prefix doesn't break the lookup. Coverage: every weapon has a real match;
// armor has real UO icons for glove/arms/legs/chest pieces where the source pack has
// them (18 of the 20 possible material+slot combos — missing Ring Mail gloves and
// Chainmail arms, which fall back to the generic gauntlet/shield2 icons same as
// before), but this pack has no helmet or gorget icons at all, so those 7 recipes
// (Chain Coif/Plate Helm/Leather Cap/Studded Cap/Plate Gorget/Leather Gorget/Studded
// Gorget) keep the existing generic fallback too. Potions are keyed by effect
// ("heal"/"stamina"/"poison"/"damage", matching PotionStack::effect exactly) rather
// than by name, looked up directly (not via suffix match) in the potion pouch list.
static const std::vector<std::pair<std::string, std::string>> kItemIconManifest = {
    // Weapons
    {"Cutlass", "Cutlass.bmp"}, {"Scimitar", "Scimitar.bmp"}, {"Katana", "Katana.bmp"},
    {"Longsword", "Longsword.bmp"}, {"Broadsword", "Broadsword.bmp"}, {"Bardiche", "Bardiche.bmp"},
    {"Viking Sword", "VikingSword.bmp"}, {"Halberd", "Halberd.bmp"}, {"Kryss", "Kryss.bmp"},
    {"Pitchfork", "Pitchfork.bmp"}, {"Short Spear", "ShortSpear.bmp"}, {"War Fork", "WarFork.bmp"},
    {"Spear", "Spear.bmp"}, {"Club", "Club.bmp"}, {"Mace", "Mace.bmp"}, {"Maul", "Maul.bmp"},
    {"War Axe", "WarAxe.bmp"}, {"War Hammer", "WarHammer.bmp"}, {"Shepherd's Crook", "ShepherdsCrook.bmp"},
    {"Gnarled Staff", "GnarledStaff.bmp"}, {"Quarterstaff", "Quarterstaff.bmp"}, {"Black Staff", "BlackStaff.bmp"},
    {"Short Bow", "Bow.bmp"}, {"Bow", "Bow.bmp"}, {"Composite Bow", "CompositeBow.bmp"},
    {"Crossbow", "Crossbow.bmp"}, {"Heavy Crossbow", "HeavyCrossbow.bmp"},
    // Armor
    {"Ring Mail Sleeves", "RingMailSleeves.bmp"}, {"Ring Mail Tunic", "RingMailTunic.bmp"},
    {"Ring Mail Leggings", "RingMailLeggings.bmp"}, {"Chainmail Gloves", "ChainmailGloves.bmp"},
    {"Chainmail Leggings", "ChainmailLeggings.bmp"}, {"Chain Tunic", "ChainTunic.bmp"},
    {"Plate Gloves", "PlateGloves.bmp"}, {"Plate Arms", "PlateArms.bmp"}, {"Plate Legs", "PlateLegs.bmp"},
    {"Plate Chest", "PlateChest.bmp"}, {"Leather Gloves", "LeatherGloves.bmp"},
    {"Leather Sleeves", "LeatherSleeves.bmp"}, {"Leather Leggings", "LeatherLeggings.bmp"},
    {"Leather Tunic", "LeatherTunic.bmp"}, {"Studded Gloves", "StuddedGloves.bmp"},
    {"Studded Sleeves", "StuddedSleeves.bmp"}, {"Studded Leggings", "StuddedLeggings.bmp"},
    {"Studded Tunic", "StuddedTunic.bmp"},
    // Potions (keyed by PotionStack::effect, looked up directly — not a suffix match)
    {"heal", "PotionHeal.bmp"}, {"stamina", "PotionStamina.bmp"},
    {"poison", "PotionPoison.bmp"}, {"damage", "PotionDamage.bmp"},
};

// ---------------------------------------------------------------------
// Combat sprite animations — CraftPix "Knight" pack (assets/knight/, curated
// from assets/Knight character/Knight_1/). Licensed for personal/commercial
// use per assets/Knight character/License.txt (craftpix.net/file-licenses/);
// only reselling the raw art files is restricted. Each sheet is a single
// horizontal strip of 128x128 frames. It's a side-view character (faces
// right only), which fits the stationary combat panel but not the top-down
// overworld — the overworld keeps the paper doll from DrawPaperdollLayers.
// ---------------------------------------------------------------------
struct SpriteSheet {
    Texture2D tex{};
    bool ok = false;
    int frames = 1;
};

// A true top-down (viewed-from-above) character sheet: `rows` rows of `frameW`x`frameH`
// frames, one row per facing direction in Down/Left/Right/Up order — originally the
// CraftPix "4 direction male" pack's layout (assets/hero/, fixed 64x64), generalized
// 2026-09-23 to support Mark's own commissioned art (assets/Carl/...), which uses much
// bigger, per-sheet-varying frame dimensions and richer per-direction frame counts.
// The walk/attack/cast column ranges are PER ROW (indexed 0=Down,1=Left,2=Right,3=Up),
// not one global range — the hero-replacement sheet is the reason why: each direction
// splits its 8 columns differently (Down: 6 walk + 1 attack frame; Left/Right: 3 walk +
// 4 attack; Up: a dedicated 1-frame Cast pose + 4 walk + 3 attack), found by actually
// viewing all 32 source frames rather than assumed uniform. They default to "the whole
// row is one cycle, idle holds frame 0" (LoadDirSpriteSheet sets every row's walkCount
// = framesPerRow) — exactly matching the original hero-only behavior for any sheet that
// doesn't override them, so generalizing this struct changed nothing until the hero-
// replacement sheet became the first to actually narrow these ranges per row.
enum class ActorAnim { Idle, Walk, Attack, Cast };
struct DirSpriteSheet {
    Texture2D tex{};
    bool ok = false;
    int frameW = 64;
    int frameH = 64;
    int framesPerRow = 1;
    int rows = 4;
    std::array<int, 4> idleCol{0, 0, 0, 0};
    std::array<int, 4> walkStart{0, 0, 0, 0}, walkCount{1, 1, 1, 1};
    std::array<int, 4> attackStart{0, 0, 0, 0}, attackCount{1, 1, 1, 1};
    std::array<int, 4> castStart{0, 0, 0, 0}, castCount{1, 1, 1, 1};
    // Which actual texture row each logical direction (0=Down,1=Left,2=Right,3=Up)
    // reads from, and whether that row is drawn horizontally mirrored — added
    // 2026-09-23 for Mark's monster/creature art, which (unlike the hero sheet) is
    // stored Down/Up/Left/Left-again rather than Down/Left/Right/Up, and has no real
    // Right-facing content at all. Rather than physically rebuild each sheet (the
    // hero's approach, needed there because its "Right" row was a genuine content
    // mess, not just reordered), this indirection lets a sheet reuse another row's
    // pixels — e.g. rowMap={0,2,2,1}, rowFlip={f,f,true,f} reads Up from texture row 1
    // and Right from texture row 2 mirrored, leaving the pixels untouched. Defaults to
    // identity (no remap, no flip) so any sheet that already IS in Down/Left/Right/Up
    // order — the hero, and anything else laid out that way — is unaffected.
    std::array<int, 4> rowMap{0, 1, 2, 3};
    std::array<bool, 4> rowFlip{false, false, false, false};
};

struct GameAssets {
    Texture2D player{};
    bool playerOk = false;
    std::array<std::pair<std::string, Texture2D>, 10> building{};
    std::array<bool, 10> buildingOk{};
    // Real building art (assets/town_buildings/, CraftPix "Tropical Medieval City" set)
    // shown in place of the generic wall+roof+door composite when present — see
    // DrawBuildingNode's realBuildingTex branch. House's is from the same pack's larger
    // "mid city" source sheet (assets/mid city/), a building never cropped into this
    // folder until now.
    std::array<std::pair<std::string, Texture2D>, 10> townBuilding{};
    std::array<bool, 10> townBuildingOk{};
    // Saltmere's own real building art (assets/saltmere_buildings/, Mark's "Carl" art
    // drop, 2026-09-23) — same kBuildingKeys, same role as townBuilding above but for
    // Town 2. Before this, Town 2 had no dedicated building art at all and fell back to
    // a slate-blue-tinted generic composite (see DrawTownScreen's old bodyTint comment).
    std::array<std::pair<std::string, Texture2D>, 10> saltmereBuilding{};
    std::array<bool, 10> saltmereBuildingOk{};
    // Regular dungeon monster, indexed by dungeon (0=Emberveil..4=Hollow Warrens) — a
    // DirSpriteSheet since 2026-09-23 (Mark's "Carl" art drop gave these real
    // directional idle/walk frames instead of one static Texture2D each; DirSpriteSheet
    // already carries its own `.ok`, so no separate *Ok array is needed here anymore).
    std::array<DirSpriteSheet, 5> monsterFamily{};
    std::array<Texture2D, 5> bossFamily{}; // distinct art per dungeon's boss, not just a tinted regular monster — stays a single static Texture2D, bosses don't move
    std::array<bool, 5> bossFamilyOk{};
    Texture2D groundGrass{}, groundDirt{}, dungeonWall{}, dungeonFloor{}, buildingDoor{}, foliage{};
    bool groundGrassOk = false, groundDirtOk = false, dungeonWallOk = false, dungeonFloorOk = false, buildingDoorOk = false, foliageOk = false;
    // Per-dungeon themed floor/wall — falls back to the generic dungeonFloor/dungeonWall
    // above (and from there to a flat color) if a themed texture isn't found.
    std::array<Texture2D, 5> dungeonFloorThemed{}, dungeonWallThemed{};
    std::array<bool, 5> dungeonFloorThemedOk{}, dungeonWallThemedOk{};
    // Per-building interior backdrop, added 2026-09-21 (Mark: buildings should "open
    // into a space that looks like the building type" — a static themed backdrop, not
    // a walkable room, drawn behind the Craft screen's existing UI). Same pattern and
    // same DCSS source pack as the per-dungeon theming above; 4 entries indexed the
    // same way kCraftBuildings already is (0=Smith/1=Carpenter/2=Tailor/3=Alchemy).
    std::array<Texture2D, 4> craftFloorThemed{}, craftWallThemed{};
    std::array<bool, 4> craftFloorThemedOk{}, craftWallThemedOk{};
    // The Provisioner has no BuildingDef entry (it's not craftable), so its backdrop
    // gets its own pair rather than a 5th array slot.
    Texture2D provisionerFloor{}, provisionerWall{};
    bool provisionerFloorOk = false, provisionerWallOk = false;
    // The Hollow Warrens' boss-room rug and scattered torch decoration — see the
    // room-layout section below (same pattern as Sunken Crypt's water/Emberveil's braziers).
    Texture2D hollowWarrensRug{}, hollowWarrensTorch{};
    bool hollowWarrensRugOk = false, hollowWarrensTorchOk = false;
    // Parallel to kPaperdollManifest above, loaded in the same order.
    std::vector<Texture2D> paperdollTex;
    std::vector<bool> paperdollOk;
    // Parallel to kItemIconManifest above, loaded in the same order.
    std::vector<Texture2D> itemIconTex;
    std::vector<bool> itemIconOk;
    // Player-side combat animations — see "Combat sprite animations" above.
    SpriteSheet knightIdle, knightAttack1, knightAttack2, knightHurt, knightDefend, knightProtect;
    // Monster-side combat animations, Sunken Crypt only for now (assets/skeleton/,
    // curated from the CraftPix "skeleton monster" pack's Skeleton_Warrior — same
    // license/format as the knight). Other dungeons still show a static icon.
    SpriteSheet skeletonIdle, skeletonAttack1, skeletonHurt;
    // Overworld hero — see "DirSpriteSheet" above.
    DirSpriteSheet heroSheet; // was heroWalk — renamed 2026-09-23 once it grew real
                                // idle/walk/attack/cast ranges, not just a walk cycle.
    // Town dressing — see "Village dressing" above: per-craft-building animated doors
    // (open on approach), scattered fence posts, and farmland ground patches.
    SpriteSheet doorSmith, doorCarpenter, doorTailor, doorAlchemy;
    Texture2D fencePost{}; bool fencePostOk = false;
    Texture2D farmland{}; bool farmlandOk = false;
    // Town flavor props — fountain/streetlamp/signage/stalls/clutter, same "top down
    // village" CraftPix pack as the doors/farmland above. See kTownProps.
    Texture2D townFountain{}, townLamp{}, townSignSmith{}, townStall1{}, townStall2{},
              townStall3{}, townLumberpile{}, townBarrel{}, townCrate{}, townAnvil{};
    bool townFountainOk = false, townLampOk = false, townSignSmithOk = false, townStall1Ok = false,
         townStall2Ok = false, townStall3Ok = false, townLumberpileOk = false, townBarrelOk = false,
         townCrateOk = false, townAnvilOk = false;
    // Plaza statue + a splash of autumn-colored foliage — "Mage City Arcanos" by
    // Hyptosis (CC0, opengameart.org/content/mage-city-arcanos), a different pixel-art
    // hand from the rest of Town's dressing, used only for standalone accent props
    // (not ground/wall tiles, which stay in the already-matched "top down village" set).
    Texture2D townStatue{}, townAutumnBush{};
    bool townStatueOk = false, townAutumnBushOk = false;
    // Trade-themed prop dressing, added 2026-09-21 (Mark asked whether other buildings
    // could get something as trade-fitting as the Smith's anvil) — two more Kenney CC0
    // "Tiny" packs already sitting in assets/ unused all project (tiny farm, tiny
    // dungeon; see kTownProps), individual 16x16 tiles cropped out and copied flat into
    // assets/village/ alongside the CraftPix props for one consistent loading spot,
    // despite being a third, unrelated source pack.
    Texture2D townSheep{}, townCow{}, townChicken{}, townPotionPurple{}, townPotionRed{},
              townChest{}, townBookshelf{};
    bool townSheepOk = false, townCowOk = false, townChickenOk = false, townPotionPurpleOk = false,
         townPotionRedOk = false, townChestOk = false, townBookshelfOk = false;
    // Spell-type icons — "Moderna Graphical Interface" by Jorge Avila (CC-BY 3.0/LGPL,
    // opengameart.org/content/moderna-graphical-interface; attribution owed if this game
    // is ever published). The source PSD only has 4 finished icons (not one per spell),
    // so they're reused by SpellType (see SpellTypeIcon) the same way dungeons already
    // share one monster-family icon across every monster in that dungeon — Summon
    // reuses the Debuff icon since neither the source file nor this game's spell list
    // has anything dedicated to it.
    Texture2D spellIconOffensive{}, spellIconDebuff{}, spellIconBuff{}, spellIconUtility{};
    bool spellIconOffensiveOk = false, spellIconDebuffOk = false, spellIconBuffOk = false, spellIconUtilityOk = false;
    // Real per-spell icons, one per kSpells entry — CorvaeOboro's "ultima_online_mods"
    // (CC0 1.0, github.com/CorvaeOboro/ultima_online_mods), classic UO magery spell
    // icons picked by thematic match since kSpells' names are original, not UO's own
    // (e.g. "Ember Burst" -> Fireball, "Sap Strength" -> Weaken). Falls back to the
    // 4 type-based Moderna icons above (see SpellIcon) if one of these fails to load.
    std::array<Texture2D, 16> spellIconPerSpell{};
    std::array<bool, 16> spellIconPerSpellOk{};
    // Gear icons for the backpack/inventory list — AI-generated (Gemini) by Mark, reused by
    // item type/slot (see GearIconForItem) the same way spell icons are shared by SpellType.
    Texture2D gearIconSword{}, gearIconShield{}, gearIconShield2{}, gearIconHelmet{}, gearIconGauntlet{}, gearIconAmulet{};
    bool gearIconSwordOk = false, gearIconShieldOk = false, gearIconShield2Ok = false, gearIconHelmetOk = false, gearIconGauntletOk = false, gearIconAmuletOk = false;
    // Sunken Crypt's boss-room water pool — see the room-layout section above.
    Texture2D sunkenCryptWater{}; bool sunkenCryptWaterOk = false;
    // Emberveil Hollow's scattered fire braziers — see the room-layout section above.
    Texture2D emberveilBrazier{}; bool emberveilBrazierOk = false;
    // The Wilderness — see kWildernessGatherNodes/kWildernessCreatureSpots above.
    Texture2D wildTree{}; bool wildTreeOk = false;
    Texture2D wildRock{}; bool wildRockOk = false;
    // Parallel to kWildCreatures (not kWildernessCreatureSpots) — creatureIdx indexes
    // this directly (see kWildernessCreatureSpots for the history of why that wasn't
    // always safe).
    // Originally only 5 of 11 creatures had art (Dog/Wolf/Bear/Griffin/Dragon); Mark
    // generated the other 6 (Panther/Bison/Horse/Sabertooth/Drake/Wyvern) with Gemini
    // for the wilderness overhaul, filling the full roster. Upgraded to real directional
    // DirSpriteSheets 2026-09-23 ("Carl" art drop) — also what the AI companion now
    // renders as, closing the "no dedicated art" gap from the weapon-swing session.
    std::array<DirSpriteSheet, 11> wildCreatureTex{};
    // Decorative wilderness props (water/deerskull/chest/bush/rocks/cactus/fence/grass/
    // haybale/plant) — same Gemini "Medieval Animal Set" batch as the creatures above.
    // See WildFoliageIcon (variants 5-14) for how these attach to kWildernessFoliage.
    std::array<Texture2D, 10> wildPropTex{};
    std::array<bool, 10> wildPropTexOk{};
    // Rock variety for the 3 ore nodes (CraftPix "Rocks & Bushes", assets/wilderness/) —
    // was a single flat rock.png for all 3; ore1/2/3 give each a distinct look.
    std::array<Texture2D, 3> wildOreTex{};
    std::array<bool, 3> wildOreTexOk{};
    // Purely decorative scatter, no collision — same role as Town's kFoliagePositions.
    Texture2D wildBush1{}, wildBush2{}, wildFern1{};
    bool wildBush1Ok = false, wildBush2Ok = false, wildFern1Ok = false;
    // Fightable Wilderness monsters — parallel to kWildernessMonsterSpots. Originally
    // single frames cropped from OpenGameArt animation sheets (Redshrike's LPC Goblin/
    // Imp/Wolf Howl, bagzie's bat, Calciumtrice's Animated Rogue), replaced 2026-09-23
    // with real directional DirSpriteSheets from Mark's "Carl" art drop — these chase
    // the player, so a real walk cycle that turns to face movement is a genuine
    // improvement over the old static icon, not just a style refresh.
    std::array<DirSpriteSheet, 5> wildMonsterTex{};
    // The Rival Adventurer's own dedicated sheet (previously reused the Bandit's
    // wildMonsterTex entry). Used both while it's engaged in combat and while it's
    // roaming/hunting (see GameState::rivalLevel's comment) — no longer tied to a
    // kWildernessMonsterSpots row at all.
    DirSpriteSheet rivalAdventurerSheet{};
    // Town NPCs (2026-09-23, Mark's "Carl" art drop) — parallel to kTownNPCs/kTown2NPCs
    // by index, same DirSpriteSheet convention as the monsters/creatures above. Before
    // this, every Town NPC was a plain colored circle (see the "AI players" session).
    std::array<DirSpriteSheet, 6> townNPCSheets{};
    std::array<DirSpriteSheet, 6> saltmereNPCSheets{};
    // Dungeon entrance markers on the Wilderness map — parallel to
    // kWildernessDungeonEntrances (same order: Emberveil/Bloodtusk/SunkenCrypt/Wyrmscar/
    // HollowWarrens). Dungeon Crawl Stone Soup (CC0), assets/wilderness_entrances/ —
    // picked by name match to each dungeon's theme (stone_arch_hell for the fire hollow,
    // enter_orc for the tusked hold, enter_crypt, enter_lair for the wyrm's den, plain
    // entrance for the warrens). Falls back to the existing flat color circle if missing.
    std::array<Texture2D, 5> wildEntranceTex{};
    std::array<bool, 5> wildEntranceTexOk{};
    // UI text font — see UiFont()/DrawUIText()/MeasureUIText() below. Falls back to
    // raylib's default bitmap font (blocky, hard to read at UI sizes) if this fails to
    // load, same "never crash on missing art" convention as every texture here.
    Font uiFont{}; bool uiFontOk = false;
};
static GameAssets g_assets;

// Mark found raylib's built-in default font hard to read across every text-heavy
// screen (Town/Skills/Magic/Craft in particular). Nunito (Google Fonts, OFL-licensed,
// assets/fonts/) is a clean, highly legible rounded sans-serif — swapped in everywhere
// via these three wrappers instead of raylib's raw DrawText/MeasureText, so every one of
// the 130+ existing call sites just needed its function name changed (DrawText ->
// DrawUIText, MeasureText -> MeasureUIText), not its argument list — same signatures,
// same argument order, just backed by uiFont instead of GetFontDefault().
static inline Font UiFont() { return g_assets.uiFontOk ? g_assets.uiFont : GetFontDefault(); }
static inline void DrawUIText(const char* text, int posX, int posY, int fontSize, Color color) {
    DrawTextEx(UiFont(), text, { (float)posX, (float)posY }, (float)fontSize, 1.0f, color);
}
static inline int MeasureUIText(const char* text, int fontSize) {
    return (int)MeasureTextEx(UiFont(), text, (float)fontSize, 1.0f).x;
}

static Texture2D TryLoadTexture(const std::string& path, bool& okOut) {
    Texture2D t = LoadTexture(path.c_str());
    okOut = (t.id != 0);
    if (okOut) SetTextureFilter(t, TEXTURE_FILTER_POINT); // crisp pixel art, no blurring when scaled up
    return t;
}

static SpriteSheet LoadSpriteSheet(const std::string& path, int frameWidth) {
    SpriteSheet sheet;
    sheet.tex = TryLoadTexture(path, sheet.ok);
    if (sheet.ok) sheet.frames = std::max(1, sheet.tex.width / frameWidth);
    return sheet;
}

static DirSpriteSheet LoadDirSpriteSheet(const std::string& path, int frameW = 64, int frameH = 64, int rows = 4) {
    DirSpriteSheet sheet;
    sheet.frameW = frameW;
    sheet.frameH = frameH;
    sheet.rows = rows;
    sheet.tex = TryLoadTexture(path, sheet.ok);
    if (sheet.ok) {
        sheet.framesPerRow = std::max(1, sheet.tex.width / frameW);
        // Whole row = one walk cycle, idle = frame 0, attack/cast reuse the same cycle
        // — the default for every row, for any sheet that doesn't narrow these ranges
        // per row after loading (see the hero-replacement sheet for one that does).
        sheet.walkCount.fill(sheet.framesPerRow);
        sheet.attackCount.fill(sheet.framesPerRow);
        sheet.castCount.fill(sheet.framesPerRow);
    }
    return sheet;
}

// Loads one of Mark's "Carl" art drop's monster/creature sheets (2026-09-23). Unlike
// the hero sheet, these DO have a working merged "-clean.png" (no reassembly needed),
// but they're laid out Down/Up/Left/Left-again rather than Down/Left/Right/Up — checked
// by directly viewing several of them, not assumed — and none have a usable 4th row (the
// last row is either a duplicate of Left, or in a couple of cases just missing, giving a
// 3-row sheet). `srcRows` is 3 or 4 depending on which; `walkColsOverride`, when >0,
// narrows the walk cycle to fewer columns than the sheet's full width divided by
// `cols` — needed for a handful of sheets (creature-panther, creature-wyvern,
// monster-wyrmscar-regular) whose side-view rows only have 5 real frames trailed by
// blank padding out to the sheet's full column count, found by viewing them directly
// rather than trusting the frame count blindly.
static DirSpriteSheet LoadCarlActorSheet(const std::string& path, int cols, int srcRows = 4, int walkColsOverride = 0) {
    DirSpriteSheet sheet;
    sheet.tex = TryLoadTexture(path, sheet.ok);
    if (sheet.ok) {
        sheet.frameW = sheet.tex.width / cols;
        sheet.frameH = sheet.tex.height / srcRows;
        sheet.framesPerRow = cols;
        sheet.rows = srcRows;
        int walkCols = walkColsOverride > 0 ? walkColsOverride : cols;
        sheet.walkCount.fill(walkCols);
        sheet.attackCount.fill(walkCols);
        sheet.castCount.fill(walkCols);
        // Down/Left/Right/Up (dest) <- Down/Up/Left/Left (src rows 0,1,2,2) — Right is
        // a horizontally-mirrored read of Left's row, since no sheet has real Right art.
        sheet.rowMap = {0, 2, 2, 1};
        sheet.rowFlip = {false, false, true, false};
    }
    return sheet;
}

static void LoadGameAssets() {
    // Loaded at 48px (0/nullptr = default codepoints, just standard ASCII — every string
    // in the game is plain English) so it covers the largest UI text (title, 22px)
    // without much upscale blur while not needing an extreme downscale for the smallest
    // (11px). Mipmaps + trilinear are what actually keep that small text legible — plain
    // bilinear with no mipmap chain let thin strokes on a heavily-minified glyph atlas
    // alias away to near-nothing, which was less readable than raylib's default font,
    // the opposite of the goal.
    g_assets.uiFont = LoadFontEx("assets/fonts/Nunito.ttf", 48, nullptr, 0);
    g_assets.uiFontOk = (g_assets.uiFont.texture.id != 0) && (g_assets.uiFont.glyphCount > 0);
    if (g_assets.uiFontOk) {
        GenTextureMipmaps(&g_assets.uiFont.texture);
        SetTextureFilter(g_assets.uiFont.texture, TEXTURE_FILTER_TRILINEAR);
    }

    g_assets.player = TryLoadTexture("assets/player.png", g_assets.playerOk);

    static const char* kBuildingKeys[10] = {
        "smith", "carpenter", "tailor", "alchemy", "provisioner", "stable", "healer", "bank", "townhall", "house"
    };
    for (int i = 0; i < 10; i++) {
        bool ok = false;
        Texture2D t = TryLoadTexture(std::string("assets/buildings/") + kBuildingKeys[i] + ".png", ok);
        g_assets.building[i] = { kBuildingKeys[i], t };
        g_assets.buildingOk[i] = ok;
    }
    for (int i = 0; i < 10; i++) {
        bool ok = false;
        Texture2D t = TryLoadTexture(std::string("assets/town_buildings/") + kBuildingKeys[i] + ".png", ok);
        g_assets.townBuilding[i] = { kBuildingKeys[i], t };
        g_assets.townBuildingOk[i] = ok;
    }
    for (int i = 0; i < 10; i++) {
        bool ok = false;
        Texture2D t = TryLoadTexture(std::string("assets/saltmere_buildings/") + kBuildingKeys[i] + ".png", ok);
        g_assets.saltmereBuilding[i] = { kBuildingKeys[i], t };
        g_assets.saltmereBuildingOk[i] = ok;
    }

    // File names match dungeon theme (emberveil/bloodtusk/sunkencrypt/wyrmscar/
    // hollowwarrens), in the same 0-4 order as kDungeons, for both the regular-monster
    // and boss sets. Regular-monster art replaced 2026-09-23 (Mark's "Carl" art drop)
    // with real directional sheets (assets/monsters_v2/) — bosses stay a single static
    // Texture2D each (assets/monsters_boss_v2/, also refreshed) since they don't move.
    // Column counts and the one 3-row exception (Hollow Warrens) were determined by
    // viewing each sheet directly, not assumed from the file name; the Wyrmscar sheet's
    // side-view rows only have 5 real frames trailed by blank padding (same issue as a
    // couple of the wilderness creatures below), hence its walkColsOverride.
    static const char* kMonsterFiles[5] = {
        "assets/monsters_v2/emberveil.png", "assets/monsters_v2/bloodtusk.png",
        "assets/monsters_v2/sunkencrypt.png", "assets/monsters_v2/wyrmscar.png",
        "assets/monsters_v2/hollowwarrens.png"
    };
    static const int kMonsterCols[5] = { 5, 8, 8, 6, 8 };
    static const int kMonsterRows[5] = { 4, 4, 4, 4, 3 };
    static const int kMonsterWalkOverride[5] = { 0, 0, 0, 5, 0 };
    static const char* kBossFiles[5] = {
        "assets/monsters_boss_v2/emberveil.png", "assets/monsters_boss_v2/bloodtusk.png",
        "assets/monsters_boss_v2/sunkencrypt.png", "assets/monsters_boss_v2/wyrmscar.png",
        "assets/monsters_boss_v2/hollowwarrens.png"
    };
    for (int i = 0; i < 5; i++) {
        g_assets.monsterFamily[i] = LoadCarlActorSheet(kMonsterFiles[i], kMonsterCols[i], kMonsterRows[i], kMonsterWalkOverride[i]);
        g_assets.bossFamily[i] = TryLoadTexture(kBossFiles[i], g_assets.bossFamilyOk[i]);
    }

    g_assets.groundGrass = TryLoadTexture("assets/ground/grass.png", g_assets.groundGrassOk);
    g_assets.groundDirt = TryLoadTexture("assets/ground/dirt.png", g_assets.groundDirtOk);
    g_assets.dungeonWall = TryLoadTexture("assets/dungeon/wall.png", g_assets.dungeonWallOk);
    g_assets.dungeonFloor = TryLoadTexture("assets/dungeon/floor.png", g_assets.dungeonFloorOk);
    g_assets.buildingDoor = TryLoadTexture("assets/buildings/door.png", g_assets.buildingDoorOk);
    g_assets.foliage = TryLoadTexture("assets/ground/foliage.png", g_assets.foliageOk);

    // Per-dungeon theming — same 0-4 order as kDungeons/kMonsterFiles above.
    static const char* kThemedFloorFiles[5] = {
        "assets/dungeon_themed/emberveil_floor.png", "assets/dungeon_themed/bloodtusk_floor.png",
        "assets/dungeon_themed/sunkencrypt_floor.png", "assets/dungeon_themed/wyrmscar_floor.png",
        "assets/dungeon_themed/hollowwarrens_floor.png"
    };
    static const char* kThemedWallFiles[5] = {
        "assets/dungeon_themed/emberveil_wall.png", "assets/dungeon_themed/bloodtusk_wall.png",
        "assets/dungeon_themed/sunkencrypt_wall.png", "assets/dungeon_themed/wyrmscar_wall.png",
        "assets/dungeon_themed/hollowwarrens_wall.png"
    };
    for (int i = 0; i < 5; i++) {
        g_assets.dungeonFloorThemed[i] = TryLoadTexture(kThemedFloorFiles[i], g_assets.dungeonFloorThemedOk[i]);
        g_assets.dungeonWallThemed[i] = TryLoadTexture(kThemedWallFiles[i], g_assets.dungeonWallThemedOk[i]);
    }
    g_assets.hollowWarrensRug = TryLoadTexture("assets/dungeon_themed/hollowwarrens_rug.png", g_assets.hollowWarrensRugOk);
    g_assets.hollowWarrensTorch = TryLoadTexture("assets/dungeon_themed/hollowwarrens_torch.png", g_assets.hollowWarrensTorchOk);

    // Per-building interior backdrop — same 0-3 order as kCraftBuildings.
    static const char* kCraftFloorFiles[4] = {
        "assets/dungeon_themed/smith_floor.png", "assets/dungeon_themed/carpenter_floor.png",
        "assets/dungeon_themed/tailor_floor.png", "assets/dungeon_themed/alchemy_floor.png"
    };
    static const char* kCraftWallFiles[4] = {
        "assets/dungeon_themed/smith_wall.png", "assets/dungeon_themed/carpenter_wall.png",
        "assets/dungeon_themed/tailor_wall.png", "assets/dungeon_themed/alchemy_wall.png"
    };
    for (int i = 0; i < 4; i++) {
        g_assets.craftFloorThemed[i] = TryLoadTexture(kCraftFloorFiles[i], g_assets.craftFloorThemedOk[i]);
        g_assets.craftWallThemed[i] = TryLoadTexture(kCraftWallFiles[i], g_assets.craftWallThemedOk[i]);
    }
    g_assets.provisionerFloor = TryLoadTexture("assets/dungeon_themed/provisioner_floor.png", g_assets.provisionerFloorOk);
    g_assets.provisionerWall = TryLoadTexture("assets/dungeon_themed/provisioner_wall.png", g_assets.provisionerWallOk);

    g_assets.paperdollTex.resize(kPaperdollManifest.size());
    g_assets.paperdollOk.resize(kPaperdollManifest.size());
    for (size_t i = 0; i < kPaperdollManifest.size(); i++) {
        bool ok = false;
        g_assets.paperdollTex[i] = TryLoadTexture("assets/paperdoll/" + kPaperdollManifest[i].second, ok);
        g_assets.paperdollOk[i] = ok; // std::vector<bool> can't bind a bool& directly
    }
    g_assets.itemIconTex.resize(kItemIconManifest.size());
    g_assets.itemIconOk.resize(kItemIconManifest.size());
    for (size_t i = 0; i < kItemIconManifest.size(); i++) {
        bool ok = false;
        g_assets.itemIconTex[i] = TryLoadTexture("assets/item_icons/" + kItemIconManifest[i].second, ok);
        g_assets.itemIconOk[i] = ok;
    }

    g_assets.knightIdle = LoadSpriteSheet("assets/knight/idle.png", 128);
    g_assets.knightAttack1 = LoadSpriteSheet("assets/knight/attack1.png", 128);
    g_assets.knightAttack2 = LoadSpriteSheet("assets/knight/attack2.png", 128);
    g_assets.knightHurt = LoadSpriteSheet("assets/knight/hurt.png", 128);
    g_assets.knightDefend = LoadSpriteSheet("assets/knight/defend.png", 128);
    g_assets.knightProtect = LoadSpriteSheet("assets/knight/protect.png", 128);

    g_assets.skeletonIdle = LoadSpriteSheet("assets/skeleton/idle.png", 128);
    g_assets.skeletonAttack1 = LoadSpriteSheet("assets/skeleton/attack1.png", 128);
    g_assets.skeletonHurt = LoadSpriteSheet("assets/skeleton/hurt.png", 128);

    // Hero replacement (2026-09-23, Mark's "Carl" art drop) — assets/hero/hero_v2.png,
    // reassembled from 32 loose extracted frames (hero-sprite-sheet-v1) since the
    // cleanup tool never produced a merged sheet for this one. Built as Down/Left/Up
    // from their own clean 8-frame rows; Right has no clean source (the sheet's 4th
    // row turned out to be a mix of leftover Left poses and only 3 genuine Right-facing
    // attack frames with no idle/walk) so Right's row is a horizontally-mirrored copy
    // of Left's, baked into the sheet at build time rather than flipped at draw time —
    // keeps DrawActorSprite completely generic for sheets that DO have real Right art.
    // Column ranges below were determined by viewing all 32 source frames directly, not
    // assumed — each direction splits its 8 columns differently:
    // 2026-09-24: replaced with Carl's green-hooded-rogue sheet (hero_v3.png) at Mark's
    // request. This sheet's native texture rows are Down/Up/Left/[no Right] — the same
    // Down/Up/Left ordering LoadCarlActorSheet already handles for every monster/creature
    // sheet — rather than hero_v2's pre-reassembled Down/Left/Right/Up order, so instead
    // of physically reordering the image this uses the same rowMap/rowFlip trick: Right
    // reads Left's texture row mirrored. Confirmed by slicing the sheet into its 32 cells
    // and measuring each frame's pixel bounding box directly, not assumed:
    //  - Down (native row 0): cols 0-1 are a near-duplicate idle pair (~157px wide, legs
    //    together); cols 2-6 are a real 5-frame walk cycle (~181-185px, legs spread); col 7
    //    is a distinct sword-across-body guard pose, used as the single Attack frame.
    //  - Up (native row 1): cols 0-4 were originally near-static back-view poses (the same
    //    "barely changes when walking" limitation hero_v2 had). 2026-09-24: Mark had Carl
    //    draw 3 genuine back-view walk-stride frames specifically to fill this gap: they
    //    were chroma-keyed, confirmed as real motion via bbox measurement (left-leg-forward
    //    / passing / right-leg-forward, not near-duplicates), and composited directly into
    //    this sheet's row-1 cols 1-3 (col 4's old near-static frame is now unused, left in
    //    place but out of the walk cycle range). Cols 5-7 remain the arm-raise-then-thrust
    //    Attack sequence (an improvement over hero_v2's weaker overhead-hold).
    //  - Left (native row 2): col 0 idle, cols 1-3 a real 3-frame walk cycle (cloak
    //    flowing), cols 4-7 a two-handed thrust Attack sequence.
    //  - Right: no usable native row. Row 3 looks like Left at a glance but pixel-diffing
    //    row3 vs row2 (col 0 nearly identical, cols 1-3 diverge ~90%+ even mirrored) and
    //    zooming the attack frames' head silhouettes showed row 3's walk still faces left
    //    while only its attack spins to face right mid-swing — not a clean independent
    //    Right direction. Same call as hero_v2: Right is Left's texture row mirrored via
    //    rowFlip rather than drawn from row 3.
    g_assets.heroSheet = LoadDirSpriteSheet("assets/hero/hero_v3.png", 276, 264, 4);
    if (g_assets.heroSheet.ok) {
        g_assets.heroSheet.rowMap = {0, 2, 2, 1};
        g_assets.heroSheet.rowFlip = {false, false, true, false};
        // Down
        g_assets.heroSheet.idleCol[0] = 0;
        g_assets.heroSheet.walkStart[0] = 2; g_assets.heroSheet.walkCount[0] = 5;
        g_assets.heroSheet.attackStart[0] = 7; g_assets.heroSheet.attackCount[0] = 1;
        // Left
        g_assets.heroSheet.idleCol[1] = 0;
        g_assets.heroSheet.walkStart[1] = 1; g_assets.heroSheet.walkCount[1] = 3;
        g_assets.heroSheet.attackStart[1] = 4; g_assets.heroSheet.attackCount[1] = 4;
        g_assets.heroSheet.castStart[1] = 0; g_assets.heroSheet.castCount[1] = 1;
        // Right: identical column layout to Left (the row is a mirrored copy of it).
        g_assets.heroSheet.idleCol[2] = 0;
        g_assets.heroSheet.walkStart[2] = 1; g_assets.heroSheet.walkCount[2] = 3;
        g_assets.heroSheet.attackStart[2] = 4; g_assets.heroSheet.attackCount[2] = 4;
        g_assets.heroSheet.castStart[2] = 0; g_assets.heroSheet.castCount[2] = 1;
        // Up: walk is now 3 real stride frames (cols 1-3) — see comment above.
        g_assets.heroSheet.idleCol[3] = 0;
        g_assets.heroSheet.walkStart[3] = 1; g_assets.heroSheet.walkCount[3] = 3;
        g_assets.heroSheet.attackStart[3] = 5; g_assets.heroSheet.attackCount[3] = 3;
        g_assets.heroSheet.castStart[3] = 0; g_assets.heroSheet.castCount[3] = 1;
    }

    g_assets.doorSmith = LoadSpriteSheet("assets/village/door_smith.png", 64);
    g_assets.doorCarpenter = LoadSpriteSheet("assets/village/door_carpenter.png", 64);
    g_assets.doorTailor = LoadSpriteSheet("assets/village/door_tailor.png", 64);
    g_assets.doorAlchemy = LoadSpriteSheet("assets/village/door_alchemy.png", 64);
    g_assets.fencePost = TryLoadTexture("assets/village/fencepost.png", g_assets.fencePostOk);
    g_assets.farmland = TryLoadTexture("assets/village/farmland.png", g_assets.farmlandOk);
    g_assets.townFountain = TryLoadTexture("assets/village/fountain.png", g_assets.townFountainOk);
    g_assets.townLamp = TryLoadTexture("assets/village/streetlamp.png", g_assets.townLampOk);
    g_assets.townSignSmith = TryLoadTexture("assets/village/sign_smith.png", g_assets.townSignSmithOk);
    g_assets.townStall1 = TryLoadTexture("assets/village/stall1.png", g_assets.townStall1Ok);
    g_assets.townStall2 = TryLoadTexture("assets/village/stall2.png", g_assets.townStall2Ok);
    g_assets.townStall3 = TryLoadTexture("assets/village/stall3.png", g_assets.townStall3Ok);
    g_assets.townLumberpile = TryLoadTexture("assets/village/lumberpile.png", g_assets.townLumberpileOk);
    g_assets.townBarrel = TryLoadTexture("assets/village/barrel.png", g_assets.townBarrelOk);
    g_assets.townCrate = TryLoadTexture("assets/village/crate.png", g_assets.townCrateOk);
    g_assets.townAnvil = TryLoadTexture("assets/village/anvil.png", g_assets.townAnvilOk);
    g_assets.townStatue = TryLoadTexture("assets/village/statue.png", g_assets.townStatueOk);
    g_assets.townAutumnBush = TryLoadTexture("assets/village/autumnbush.png", g_assets.townAutumnBushOk);
    g_assets.townSheep = TryLoadTexture("assets/village/sheep.png", g_assets.townSheepOk);
    g_assets.townCow = TryLoadTexture("assets/village/cow.png", g_assets.townCowOk);
    g_assets.townChicken = TryLoadTexture("assets/village/chicken.png", g_assets.townChickenOk);
    g_assets.townPotionPurple = TryLoadTexture("assets/village/potion_purple.png", g_assets.townPotionPurpleOk);
    g_assets.townPotionRed = TryLoadTexture("assets/village/potion_red.png", g_assets.townPotionRedOk);
    g_assets.townChest = TryLoadTexture("assets/village/chest.png", g_assets.townChestOk);
    g_assets.townBookshelf = TryLoadTexture("assets/village/bookshelf.png", g_assets.townBookshelfOk);
    g_assets.spellIconOffensive = TryLoadTexture("assets/spellbook_icons/spell_offensive.png", g_assets.spellIconOffensiveOk);
    g_assets.spellIconDebuff = TryLoadTexture("assets/spellbook_icons/spell_debuff.png", g_assets.spellIconDebuffOk);
    g_assets.spellIconBuff = TryLoadTexture("assets/spellbook_icons/spell_buff.png", g_assets.spellIconBuffOk);
    g_assets.spellIconUtility = TryLoadTexture("assets/spellbook_icons/spell_utility.png", g_assets.spellIconUtilityOk);
    // Parallel to kSpells (index 0-15) — see the GameAssets field comment above.
    static const char* kSpellIconFiles[16] = {
        "assets/spell_icons/SparkDart.bmp", "assets/spell_icons/MendingWord.bmp",
        "assets/spell_icons/SapStrength.bmp", "assets/spell_icons/CloudMind.bmp",
        "assets/spell_icons/FumblingCurse.bmp", "assets/spell_icons/WoundingTouch.bmp",
        "assets/spell_icons/BlessingOfVigor.bmp", "assets/spell_icons/EmberBurst.bmp",
        "assets/spell_icons/VenomSting.bmp", "assets/spell_icons/GreaterMending.bmp",
        "assets/spell_icons/StormLance.bmp", "assets/spell_icons/PsychicShatter.bmp",
        "assets/spell_icons/ArcBolt.bmp", "assets/spell_icons/Detonation.bmp",
        "assets/spell_icons/InfernoStrike.bmp", "assets/spell_icons/SummonFiend.bmp",
    };
    for (int i = 0; i < 16; i++) {
        bool ok = false;
        g_assets.spellIconPerSpell[i] = TryLoadTexture(kSpellIconFiles[i], ok);
        g_assets.spellIconPerSpellOk[i] = ok;
    }
    g_assets.gearIconSword = TryLoadTexture("assets/gear_icons/sword.png", g_assets.gearIconSwordOk);
    g_assets.gearIconShield = TryLoadTexture("assets/gear_icons/shield.png", g_assets.gearIconShieldOk);
    g_assets.gearIconShield2 = TryLoadTexture("assets/gear_icons/shield2.png", g_assets.gearIconShield2Ok);
    g_assets.gearIconHelmet = TryLoadTexture("assets/gear_icons/helmet.png", g_assets.gearIconHelmetOk);
    g_assets.gearIconGauntlet = TryLoadTexture("assets/gear_icons/gauntlet.png", g_assets.gearIconGauntletOk);
    g_assets.gearIconAmulet = TryLoadTexture("assets/gear_icons/amulet.png", g_assets.gearIconAmuletOk);
    g_assets.sunkenCryptWater = TryLoadTexture("assets/dungeon_themed/sunkencrypt_water.png", g_assets.sunkenCryptWaterOk);
    g_assets.emberveilBrazier = TryLoadTexture("assets/dungeon_themed/emberveil_brazier.png", g_assets.emberveilBrazierOk);

    g_assets.wildTree = TryLoadTexture("assets/wilderness/tree.png", g_assets.wildTreeOk);
    g_assets.wildRock = TryLoadTexture("assets/wilderness/rock.png", g_assets.wildRockOk);
    // Replaced 2026-09-23 with real directional sheets (Mark's "Carl" art drop,
    // assets/wilderness_v2/creatures/) — also what the AI companion now renders as.
    // Column counts and the two walkColsOverride cases (Panther, Wyvern — side-view
    // rows only have 5 real frames trailed by blank padding) came from viewing each
    // sheet directly, same as the dungeon monsters above.
    static const char* kWildCreatureFiles[11] = {
        "assets/wilderness_v2/creatures/dog.png", "assets/wilderness_v2/creatures/wolf.png", "assets/wilderness_v2/creatures/bear.png",
        "assets/wilderness_v2/creatures/panther.png", "assets/wilderness_v2/creatures/bison.png", "assets/wilderness_v2/creatures/horse.png",
        "assets/wilderness_v2/creatures/sabertooth.png", "assets/wilderness_v2/creatures/griffin.png", "assets/wilderness_v2/creatures/drake.png",
        "assets/wilderness_v2/creatures/wyvern.png", "assets/wilderness_v2/creatures/dragon.png"
    };
    static const int kWildCreatureCols[11] = { 6, 8, 8, 8, 5, 8, 8, 5, 8, 6, 5 };
    static const int kWildCreatureWalkOverride[11] = { 0, 0, 0, 5, 0, 0, 0, 0, 0, 5, 0 };
    for (int i = 0; i < (int)kWildCreatures.size(); i++) {
        g_assets.wildCreatureTex[i] = LoadCarlActorSheet(kWildCreatureFiles[i], kWildCreatureCols[i], 4, kWildCreatureWalkOverride[i]);
    }
    static const char* kWildPropFiles[10] = {
        "assets/wilderness_props/water.png", "assets/wilderness_props/deerskull.png",
        "assets/wilderness_props/chest.png", "assets/wilderness_props/bush.png",
        "assets/wilderness_props/rocks.png", "assets/wilderness_props/cactus.png",
        "assets/wilderness_props/fence.png", "assets/wilderness_props/grass.png",
        "assets/wilderness_props/haybale.png", "assets/wilderness_props/plant.png"
    };
    for (int i = 0; i < 10; i++) {
        bool ok = false;
        g_assets.wildPropTex[i] = TryLoadTexture(kWildPropFiles[i], ok);
        g_assets.wildPropTexOk[i] = ok;
    }
    static const char* kWildOreFiles[3] = {
        "assets/wilderness/ore1.png", "assets/wilderness/ore2.png", "assets/wilderness/ore3.png"
    };
    for (int i = 0; i < 3; i++) {
        bool ok = false;
        g_assets.wildOreTex[i] = TryLoadTexture(kWildOreFiles[i], ok);
        g_assets.wildOreTexOk[i] = ok;
    }
    g_assets.wildBush1 = TryLoadTexture("assets/wilderness/bush1.png", g_assets.wildBush1Ok);
    g_assets.wildBush2 = TryLoadTexture("assets/wilderness/bush2.png", g_assets.wildBush2Ok);
    g_assets.wildFern1 = TryLoadTexture("assets/wilderness/fern1.png", g_assets.wildFern1Ok);
    // Replaced 2026-09-23 with real directional sheets, same drop as above — these
    // chase the player, so DrawWildernessScreen derives real facing from movement now.
    static const char* kWildMonsterFiles[5] = {
        "assets/wilderness_v2/monsters/bat.png", "assets/wilderness_v2/monsters/goblin.png", "assets/wilderness_v2/monsters/wolf.png",
        "assets/wilderness_v2/monsters/imp.png", "assets/wilderness_v2/monsters/bandit.png"
    };
    static const int kWildMonsterCols[5] = { 8, 5, 8, 8, 5 };
    for (int i = 0; i < 5; i++) {
        g_assets.wildMonsterTex[i] = LoadCarlActorSheet(kWildMonsterFiles[i], kWildMonsterCols[i]);
    }
    g_assets.rivalAdventurerSheet = LoadCarlActorSheet("assets/wilderness_v2/rival_adventurer.png", 8);

    // Town NPCs — order matches kTownNPCs/kTown2NPCs exactly (see those arrays' own
    // declaration for the name list). walkColsOverride covers two sheets whose Left row
    // has fewer real frames than their nominal column count, found by viewing them
    // directly, same as the monster/creature sheets above: Cobb (8 nominal, 6 real) and
    // Young Petra (8 nominal, 7 real) both need their Left row's walkCount narrowed
    // after loading since LoadCarlActorSheet only takes one uniform override.
    static const char* kTownNPCFiles[6] = {
        "assets/npcs_v2/old_miran.png", "assets/npcs_v2/young_petra.png", "assets/npcs_v2/wystan_baker.png",
        "assets/npcs_v2/widow_aelith.png", "assets/npcs_v2/cobb_stableboy.png", "assets/npcs_v2/sister_meraude.png"
    };
    static const int kTownNPCCols[6] = { 8, 8, 5, 7, 8, 8 };
    for (int i = 0; i < 6; i++) g_assets.townNPCSheets[i] = LoadCarlActorSheet(kTownNPCFiles[i], kTownNPCCols[i]);
    g_assets.townNPCSheets[1].walkCount[1] = 7; // Young Petra's Left row: 7 real frames, not 8
    g_assets.townNPCSheets[4].walkCount[1] = 6; // Cobb's Left row: 6 real frames, not 8
    g_assets.townNPCSheets[4].walkCount[3] = 6; // Cobb's Up row: 6 real frames, not 8

    static const char* kSaltmereNPCFiles[6] = {
        "assets/npcs_v2/harbormaster_thane.png", "assets/npcs_v2/salty_bjorn.png", "assets/npcs_v2/nessa_netmender.png",
        "assets/npcs_v2/old_corwin.png", "assets/npcs_v2/dockhand_fenn.png", "assets/npcs_v2/captain_ysolde.png"
    };
    static const int kSaltmereNPCCols[6] = { 8, 8, 8, 8, 8, 5 };
    for (int i = 0; i < 6; i++) g_assets.saltmereNPCSheets[i] = LoadCarlActorSheet(kSaltmereNPCFiles[i], kSaltmereNPCCols[i]);
    static const char* kWildEntranceFiles[5] = {
        "assets/wilderness_entrances/emberveil.png", "assets/wilderness_entrances/bloodtusk.png",
        "assets/wilderness_entrances/sunkencrypt.png", "assets/wilderness_entrances/wyrmscar.png",
        "assets/wilderness_entrances/hollowwarrens.png"
    };
    for (int i = 0; i < 5; i++) {
        bool ok = false;
        g_assets.wildEntranceTex[i] = TryLoadTexture(kWildEntranceFiles[i], ok);
        g_assets.wildEntranceTexOk[i] = ok;
    }
}

static const Texture2D* FindPaperdollTexture(const std::string& key) {
    for (size_t i = 0; i < kPaperdollManifest.size(); i++)
        if (g_assets.paperdollOk[i] && kPaperdollManifest[i].first == key) return &g_assets.paperdollTex[i];
    return nullptr;
}
// Suffix match — same reasoning/fix as FindItemIconTextureBySuffix (see its comment):
// an equipped Item's name always carries a quality prefix ("Fine Broadsword"), so the
// plain exact-match FindPaperdollTexture above never actually hit for real crafted
// gear. Used for the 5 equipped-item layers in DrawPaperdollLayers; "base"/"boots"/
// "hair" pass literal (non-item) keys and keep using the exact match.
static const Texture2D* FindPaperdollTextureBySuffix(const std::string& itemName) {
    const Texture2D* best = nullptr;
    size_t bestLen = 0;
    for (size_t i = 0; i < kPaperdollManifest.size(); i++) {
        const std::string& key = kPaperdollManifest[i].first;
        if (!g_assets.paperdollOk[i] || key.size() <= bestLen || key.size() > itemName.size()) continue;
        if (itemName.compare(itemName.size() - key.size(), key.size(), key) == 0) {
            best = &g_assets.paperdollTex[i];
            bestLen = key.size();
        }
    }
    return best;
}
// Exact match — used directly for potions (keyed by effect, e.g. "heal").
static const Texture2D* FindItemIconTexture(const std::string& key) {
    for (size_t i = 0; i < kItemIconManifest.size(); i++)
        if (g_assets.itemIconOk[i] && kItemIconManifest[i].first == key) return &g_assets.itemIconTex[i];
    return nullptr;
}
// Suffix match — used for gear, since a crafted Item's name always carries a quality
// prefix ("Fine Broadsword"), not the bare recipe name the manifest keys use. Returns
// the LONGEST matching key's texture so a specific match ("Composite Bow") wins over a
// shorter one that's also technically a suffix ("Bow").
static const Texture2D* FindItemIconTextureBySuffix(const std::string& itemName) {
    const Texture2D* best = nullptr;
    size_t bestLen = 0;
    for (size_t i = 0; i < kItemIconManifest.size(); i++) {
        const std::string& key = kItemIconManifest[i].first;
        if (!g_assets.itemIconOk[i] || key.size() <= bestLen || key.size() > itemName.size()) continue;
        if (itemName.compare(itemName.size() - key.size(), key.size(), key) == 0) {
            best = &g_assets.itemIconTex[i];
            bestLen = key.size();
        }
    }
    return best;
}

// Draws the equipped-gear stack centered at `center`, scaled so the 32x32-native art
// fills a `size`x`size` square — shared by the small in-world sprite (DrawPlayer) and
// the full-size Character screen paper doll, same layer order both places.
static void DrawPaperdollLayers(const GameState& s, Vector2 center, float size) {
    Rectangle dest = { center.x - size / 2.0f, center.y - size / 2.0f, size, size };
    auto drawTex = [&](const Texture2D* tex) {
        if (!tex) return;
        Rectangle src = { 0, 0, (float)tex->width, (float)tex->height };
        DrawTexturePro(*tex, src, dest, { 0, 0 }, 0.0f, WHITE);
    };
    auto drawLayer = [&](const std::string& key) { drawTex(FindPaperdollTexture(key)); };
    auto drawItemLayer = [&](const std::optional<Item>& item) { if (item) drawTex(FindPaperdollTextureBySuffix(item->name)); };
    drawLayer("base");
    drawLayer("boots");
    // Legs draws AFTER boots (not before) so materials whose art includes its own
    // sabaton/boot (Plate, Studded) naturally cover the default boots layer at the
    // feet, while materials that stop at the ankle (Ring Mail, Chain Mail) leave the
    // default boots showing through below them — verified visually for both cases.
    drawItemLayer(s.equipped.legs);
    drawItemLayer(s.equipped.gorget);
    drawItemLayer(s.equipped.arms);
    drawItemLayer(s.equipped.chest);
    // Gloves draws after Arms so the hand sits at the tip of the sleeve (slight
    // overlap with the arm cuff, drawn on top), matching where the new base body's
    // hands actually are (found via the same alpha-silhouette technique as the other
    // pieces), not the old DCSS wrist position.
    drawItemLayer(s.equipped.gloves);
    drawLayer("hair");
    // Helmet draws after hair (covers it). All 4 materials now have new art (see
    // kPaperdollManifest's Helmets comment).
    drawItemLayer(s.equipped.helmet);
    // Weapon is still the OLD DCSS-pack art, proportioned for the old 32x32 base body
    // (compact, arms close to the torso). base/boots/hair/Arms/Gorget/Chest/Legs/
    // Gloves/Helmet(Leather Cap) above were just replaced with new, higher-detail
    // Gemini-generated art built for a taller, differently-posed body (arms angled
    // out and down) — see the "everything, bigger native size" paperdoll upgrade in
    // memory. Because DrawTexturePro stretches every layer to the same destination
    // square, this old piece now lands badly distorted/misplaced against the new base
    // (verified visually, not assumed) rather than just "a bit off". Suppressed here
    // rather than shown broken; stat bonuses from equipped items still apply
    // regardless of whether a layer is drawn.
    (void)s.equipped.rightHand;
}

// ---------------------------------------------------------------------
// Combat sprite playback — see "Combat sprite animations" above for what
// these sheets are and where they came from. Idle loops forever; every
// other CombatAnim is a one-shot triggered by a specific combat action
// (see the TriggerCombatAnim call sites: ResolveCombatRound, CastOffensiveSpell,
// CastHealSpell, UseBandageInCombat, and the hit branch of
// MonsterCounterAndMaybeEnd) and reverts to Idle once its strip finishes.
// ---------------------------------------------------------------------
static const float kCombatAnimFps = 10.0f;

static const SpriteSheet& KnightSheetFor(CombatAnim anim) {
    switch (anim) {
        case CombatAnim::Attack1: return g_assets.knightAttack1;
        case CombatAnim::Attack2: return g_assets.knightAttack2;
        case CombatAnim::Hurt:    return g_assets.knightHurt;
        case CombatAnim::Defend:  return g_assets.knightDefend;
        case CombatAnim::Protect: return g_assets.knightProtect;
        default:                  return g_assets.knightIdle;
    }
}
static void TriggerCombatAnim(CombatState& c, CombatAnim anim) { c.anim = anim; c.animTime = 0.0f; }
static void TriggerMonsterAnim(CombatState& c, CombatAnim anim) { c.monsterAnim = anim; c.monsterAnimTime = 0.0f; }

// Monster side only ever plays Idle/Attack1/Hurt (see CombatState::monsterAnim); every
// other CombatAnim value falls back to Idle since the monster never casts/bandages.
// Only wired up for the Sunken Crypt (Skeleton Warrior) so far — see kDungeons.
static const SpriteSheet& MonsterSheetFor(CombatAnim anim) {
    switch (anim) {
        case CombatAnim::Attack1: return g_assets.skeletonAttack1;
        case CombatAnim::Hurt:    return g_assets.skeletonHurt;
        default:                  return g_assets.skeletonIdle;
    }
}

// Called once a frame from main(); advances both the player's and the monster's
// animTime and reverts a finished one-shot clip back to Idle. A no-op outside combat.
static void UpdateCombatAnim(GameState& s, float dt) {
    if (!s.combat.has_value()) return;
    CombatState& c = *s.combat;
    c.animTime += dt;
    if (c.anim != CombatAnim::Idle) {
        const SpriteSheet& sheet = KnightSheetFor(c.anim);
        float duration = sheet.ok ? (float)sheet.frames / kCombatAnimFps : 0.4f;
        if (c.animTime >= duration) TriggerCombatAnim(c, CombatAnim::Idle);
    }
    c.monsterAnimTime += dt;
    if (c.monsterAnim != CombatAnim::Idle) {
        const SpriteSheet& sheet = MonsterSheetFor(c.monsterAnim);
        float duration = sheet.ok ? (float)sheet.frames / kCombatAnimFps : 0.4f;
        if (c.monsterAnimTime >= duration) TriggerMonsterAnim(c, CombatAnim::Idle);
    }
}
// Draws one looping frame of `sheet` (picked from `animTime`), centered at `center` and
// scaled so its native 128px frame fills a `size`x`size` square. No-op if the sheet
// failed to load (side-view sprites are a bonus, never load-bearing for combat itself).
static void DrawSpriteFrame(const SpriteSheet& sheet, float animTime, Vector2 center, float size) {
    if (!sheet.ok) return;
    int frame = (int)(animTime * kCombatAnimFps) % sheet.frames;
    float frameW = (float)sheet.tex.width / sheet.frames;
    float frameH = (float)sheet.tex.height;
    Rectangle src = { frame * frameW, 0, frameW, frameH };
    Rectangle dest = { center.x - size / 2.0f, center.y - size / 2.0f, size, size };
    DrawTexturePro(sheet.tex, src, dest, { 0, 0 }, 0.0f, WHITE);
}

// One icon per SpellType, reused across every spell of that type — the source PSD
// ("Moderna Graphical Interface", see GameAssets' spellIcon* fields) only has 4
// finished icons, not one per spell. Summon shares Debuff's.
static const Texture2D* SpellTypeIcon(SpellType type) {
    switch (type) {
        case SpellType::Offensive: return g_assets.spellIconOffensiveOk ? &g_assets.spellIconOffensive : nullptr;
        case SpellType::Buff:      return g_assets.spellIconBuffOk ? &g_assets.spellIconBuff : nullptr;
        case SpellType::Utility:   return g_assets.spellIconUtilityOk ? &g_assets.spellIconUtility : nullptr;
        case SpellType::Debuff:
        case SpellType::Summon:
        default:                   return g_assets.spellIconDebuffOk ? &g_assets.spellIconDebuff : nullptr;
    }
}
// Real per-spell icon (see GameAssets.spellIconPerSpell) with a graceful fallback to
// the old shared type icon if the specific one didn't load — never a hard failure.
static const Texture2D* SpellIcon(int spellIdx) {
    if (spellIdx >= 0 && spellIdx < (int)g_assets.spellIconPerSpell.size() && g_assets.spellIconPerSpellOk[spellIdx])
        return &g_assets.spellIconPerSpell[spellIdx];
    if (spellIdx >= 0 && spellIdx < (int)kSpells.size()) return SpellTypeIcon(kSpells[spellIdx].type);
    return nullptr;
}

// One icon per gear category, reused across every item of that category — same sharing
// pattern as SpellTypeIcon. Item/Recipe carry no icon field, so this derives the icon from
// existing type/slot data rather than adding one. Amulet has no equip slot in Equipment
// today (see struct Item/Equipment) so it's unused by GearIconForItem — kept for a future
// cosmetic/rare-drop use.
enum class GearIcon { Sword, Shield, Shield2, Helmet, Gauntlet, Amulet };
static const Texture2D* GearIconTexture(GearIcon icon) {
    switch (icon) {
        case GearIcon::Sword:    return g_assets.gearIconSwordOk ? &g_assets.gearIconSword : nullptr;
        case GearIcon::Shield2:  return g_assets.gearIconShield2Ok ? &g_assets.gearIconShield2 : nullptr;
        case GearIcon::Helmet:   return g_assets.gearIconHelmetOk ? &g_assets.gearIconHelmet : nullptr;
        case GearIcon::Gauntlet: return g_assets.gearIconGauntletOk ? &g_assets.gearIconGauntlet : nullptr;
        case GearIcon::Amulet:   return g_assets.gearIconAmuletOk ? &g_assets.gearIconAmulet : nullptr;
        case GearIcon::Shield:
        default:                 return g_assets.gearIconShieldOk ? &g_assets.gearIconShield : nullptr;
    }
}
static const Texture2D* GearIconForItem(const Item& item) {
    // Real UO icon by exact weapon/piece name first (see kItemIconManifest) — falls
    // through to the generic category icons below for anything the UO pack doesn't
    // cover (helmets, gorgets, Ring Mail gloves, Chainmail arms — see the manifest's
    // own comment for the exact gaps).
    if (const Texture2D* real = FindItemIconTextureBySuffix(item.name)) return real;
    if (item.type == ItemType::Weapon) return GearIconTexture(GearIcon::Sword);
    if (item.type == ItemType::Armor) {
        if (item.slot == "helmet") return GearIconTexture(GearIcon::Helmet);
        if (item.slot == "gloves") return GearIconTexture(GearIcon::Gauntlet);
        if (item.slot == "gorget" || item.slot == "chest") return GearIconTexture(GearIcon::Shield2);
        return GearIconTexture(GearIcon::Shield);
    }
    return nullptr; // potions: no gear icon
}
static void DrawItemIcon(const Item& item, float x, float y, float size) {
    const Texture2D* tex = GearIconForItem(item);
    if (!tex) return;
    Rectangle src = { 0, 0, (float)tex->width, (float)tex->height };
    Rectangle dest = { x, y, size, size };
    DrawTexturePro(*tex, src, dest, { 0, 0 }, 0.0f, WHITE);
}

// Falls back generic -> flat color, same 3-tier pattern for both floor and wall.
static const Texture2D* ThemedDungeonFloor(int dungeonIdx) {
    if (dungeonIdx >= 0 && dungeonIdx <= 4 && g_assets.dungeonFloorThemedOk[dungeonIdx])
        return &g_assets.dungeonFloorThemed[dungeonIdx];
    return g_assets.dungeonFloorOk ? &g_assets.dungeonFloor : nullptr;
}
static const Texture2D* ThemedDungeonWall(int dungeonIdx) {
    if (dungeonIdx >= 0 && dungeonIdx <= 4 && g_assets.dungeonWallThemedOk[dungeonIdx])
        return &g_assets.dungeonWallThemed[dungeonIdx];
    return g_assets.dungeonWallOk ? &g_assets.dungeonWall : nullptr;
}

static const Texture2D* FindBuildingTexture(const std::string& key) {
    for (int i = 0; i < 10; i++)
        if (g_assets.buildingOk[i] && g_assets.building[i].first == key) return &g_assets.building[i].second;
    return nullptr;
}
static const Texture2D* FindTownBuildingTexture(const std::string& key) {
    for (int i = 0; i < 10; i++)
        if (g_assets.townBuildingOk[i] && g_assets.townBuilding[i].first == key) return &g_assets.townBuilding[i].second;
    return nullptr;
}
static const Texture2D* FindSaltmereBuildingTexture(const std::string& key) {
    for (int i = 0; i < 10; i++)
        if (g_assets.saltmereBuildingOk[i] && g_assets.saltmereBuilding[i].first == key) return &g_assets.saltmereBuilding[i].second;
    return nullptr;
}
// One of the 4 CraftPix "village" animated doors per craft building, for a little visual
// distinction beyond just roof color — see "Village dressing" above. Amenity buildings
// (Provisioner/Stable/Healer/Bank/Townhall) still use the plain generic door.
static const SpriteSheet* DoorAnimForBuilding(const std::string& key) {
    if (key == "smith") return &g_assets.doorSmith;
    if (key == "carpenter") return &g_assets.doorCarpenter;
    if (key == "tailor") return &g_assets.doorTailor;
    if (key == "alchemy") return &g_assets.doorAlchemy;
    return nullptr;
}
// Picks one kWildCreatures index as a visual stand-in for a Pet of the given role —
// used to render the AI companion (2026-09-23). Not the pet's exact tamed species:
// `Pet` only records role/stats/name, not which of the 11 creatures it came from, so
// this is "a believable creature for a Melee/Tank/Caster companion" rather than a
// precise match. Wolf/Bear/Drake read as reasonably representative of their roles
// without being the most extreme (Forest Dragon, Elder Wyvern) pick for Caster.
static const DirSpriteSheet& WildCreatureSheetForRole(PetRole role) {
    switch (role) {
        case PetRole::Tank: return g_assets.wildCreatureTex[2];   // Grizzly Bear
        case PetRole::Caster: return g_assets.wildCreatureTex[8]; // Young Drake
        case PetRole::Melee: default: return g_assets.wildCreatureTex[1]; // Timber Wolf
    }
}
static const DirSpriteSheet* MonsterFamilySheet(int dungeonIdx) {
    if (dungeonIdx < 0 || dungeonIdx > 4 || !g_assets.monsterFamily[dungeonIdx].ok) return nullptr;
    return &g_assets.monsterFamily[dungeonIdx];
}
static const Texture2D* BossFamilyTexture(int dungeonIdx) {
    if (dungeonIdx < 0 || dungeonIdx > 4 || !g_assets.bossFamilyOk[dungeonIdx]) return nullptr;
    return &g_assets.bossFamily[dungeonIdx];
}

// Draws a texture centered at `center`, scaled (uniformly) so its larger side equals
// `targetSize`, tinted by `tint` (WHITE = no tint).
static void DrawIconCentered(const Texture2D& tex, Vector2 center, float targetSize, Color tint) {
    float scale = targetSize / (float)std::max(tex.width, tex.height);
    float w = tex.width * scale, h = tex.height * scale;
    DrawTextureEx(tex, { center.x - w / 2.0f, center.y - h / 2.0f }, 0.0f, scale, tint);
}
// Same as above but for one cropped frame of a sheet (srcRect, e.g. from ActorSrcRect)
// rather than the whole texture — used by DrawWorldNode's icon path for animated
// monster/creature/NPC sprites, which still want its ring/plate/label chrome.
static void DrawIconCenteredRect(const Texture2D& tex, Rectangle srcRect, Vector2 center, float targetSize, Color tint) {
    float srcW = std::fabs(srcRect.width), srcH = std::fabs(srcRect.height);
    float scale = targetSize / std::max(srcW, srcH);
    float w = srcW * scale, h = srcH * scale;
    Rectangle dest = { center.x - w / 2.0f, center.y - h / 2.0f, w, h };
    DrawTexturePro(tex, srcRect, dest, { 0, 0 }, 0.0f, tint);
}

// Tiles `tex` across `screenArea` at `worldTileSize` world-units per tile, scrolling
// correctly with the camera (tiles wrap seamlessly as the camera moves — no popping or
// sliding artifacts — since each tile's screen position is derived from its world grid
// cell minus the camera offset, wrapped with fmod). Falls back to a flat fillColor if
// the texture is missing. Does NOT set up its own scissor/clip region — the caller is
// expected to already have one active (every screen that uses this wraps its whole
// world-render block in one BeginScissorMode/EndScissorMode pair); nesting scissor
// calls would break clipping, since raylib's EndScissorMode() just disables scissoring
// rather than restoring whatever clip rect was active before.
static void DrawTiledGround(const Texture2D* tex, Rectangle screenArea, Vector2 cameraTopLeft,
                              float worldTileSize, Color fillColor, Color tint = WHITE) {
    if (!tex) { DrawRectangleRec(screenArea, fillColor); return; }
    float scale = worldTileSize / (float)tex->width;
    float startX = screenArea.x - std::fmod(cameraTopLeft.x, worldTileSize);
    float startY = screenArea.y - std::fmod(cameraTopLeft.y, worldTileSize);
    for (float y = startY; y < screenArea.y + screenArea.height; y += worldTileSize)
        for (float x = startX; x < screenArea.x + screenArea.width; x += worldTileSize)
            DrawTextureEx(*tex, { x, y }, 0.0f, scale, tint);
}

// Draws one wall band — a rectangle given in WORLD coordinates — tiled with `tex`,
// converted to screen space via the camera. Used four times (one per side) to frame a
// room; each call is a no-op if that band is currently scrolled off-screen. Tiles from
// the band's own resolved screen position (NOT via DrawTiledGround's camera-relative
// math — that would double-apply the camera offset, since WorldToScreen already baked
// it in here). No internal scissor (see DrawTiledGround's note).
static void DrawWallBand(Rectangle worldBand, Vector2 cameraTopLeft, const Texture2D* tex,
                           float worldTileSize, Color fillColor) {
    Vector2 topLeft = WorldToScreen({ worldBand.x, worldBand.y }, cameraTopLeft);
    Rectangle screenBand = { topLeft.x, topLeft.y, worldBand.width, worldBand.height };
    if (screenBand.x + screenBand.width < kViewport.x || screenBand.x > kViewport.x + kViewport.width ||
        screenBand.y + screenBand.height < kViewport.y || screenBand.y > kViewport.y + kViewport.height) return;
    if (!tex) { DrawRectangleRec(screenBand, fillColor); }
    else {
        float scale = worldTileSize / (float)tex->width;
        for (float y = screenBand.y; y < screenBand.y + screenBand.height; y += worldTileSize)
            for (float x = screenBand.x; x < screenBand.x + screenBand.width; x += worldTileSize)
                DrawTextureEx(*tex, { x, y }, 0.0f, scale, WHITE);
    }
    // A 1px solid black outline used to be drawn here (road/path/plaza edges) — Mark
    // went back and forth on it a few times this project (2px/faded -> too thick,
    // thinnest-possible 1px -> still didn't read right) and asked for it gone entirely.
}

// A static "you're inside this building" backdrop, added 2026-09-21 — a thin wall
// band along the top plus a floor fill for the rest of the content area, drawn behind
// whatever UI the caller draws next (raylib is immediate-mode, so draw order is
// z-order — this just needs to run first). No camera/scrolling since nothing walks
// around back here; tiling always starts from {0,0}. `topY` is where the screen's
// persistent header (title/resources/tab bar) ends.
static void DrawInteriorBackdrop(const Texture2D* wallTex, const Texture2D* floorTex, int screenW, int screenH, int topY) {
    const float kWallBandHeight = 60.0f;
    Rectangle wallArea = { 0, (float)topY, (float)screenW, kWallBandHeight };
    Rectangle floorArea = { 0, topY + kWallBandHeight, (float)screenW, (float)screenH - topY - kWallBandHeight };
    // Drawn faded over the page's existing cream ClearBackground, not at full opacity —
    // these DCSS textures are dark dungeon-arena art meant to fill a screen with nothing
    // else on it. At full brightness they drowned every button/text drawn on top of them
    // (2026-09-22 fix); a light wash still reads as "themed room" without fighting the UI
    // for contrast. The dark Color args from the original version were a bug — they went
    // to DrawTiledGround's unused `fillColor` (missing-texture fallback) instead of its
    // `tint` parameter, so they never actually affected the loaded textures at all.
    // Floor gets an extra-light wash relative to the wall band: it sits behind the entire
    // scrollable recipe/backpack list, whose per-row text has no backing plate of its own
    // (unlike DrawInfoLine's standalone status lines just above) — a tiled texture's own
    // internal light/dark variation can still hurt contrast at low opacity if a text row
    // happens to land on its darkest patch, so the floor needs more headroom than the wall.
    DrawTiledGround(wallTex, wallArea, { 0, 0 }, 48.0f, kColorPageBg, Fade(WHITE, 0.3f));
    DrawTiledGround(floorTex, floorArea, { 0, 0 }, 48.0f, kColorPageBg, Fade(WHITE, 0.16f));
}

// A standalone status/info line, backed by a light solid plate — for text drawn directly
// on a themed backdrop (see DrawInteriorBackdrop) with no button/panel of its own behind
// it. A tiled wall/floor texture has enough internal light/dark variation that plain text
// can land on its darkest patch and nearly vanish there even at low overall opacity; a
// button survives this because its own fill is solid, but a bare line of text has nothing
// else guaranteeing contrast. Same "always readable regardless of what's behind it" idea
// as the neutral plate already used behind world-node icons (see DrawWorldNode).
static void DrawInfoLine(const char* text, int x, int y, int fontSize, Color color = kColorText) {
    int tw = MeasureUIText(text, fontSize);
    Rectangle backing = { (float)x - 6, (float)y - 3, (float)tw + 12, (float)fontSize + 8 };
    DrawRectangleRounded(backing, 0.25f, 4, Fade(kColorPageBg, 0.88f));
    DrawUIText(text, x, y, fontSize, color);
}

// Stretches a texture to exactly fill `dest`, no tiling — used for small composite
// pieces (a building's wall panel, its door) where a single stretched image reads
// fine at this scale and avoids any tiling/clipping complexity.
static void DrawStretched(const Texture2D& tex, Rectangle dest, Color tint) {
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    DrawTexturePro(tex, src, dest, { 0, 0 }, 0.0f, tint);
}

// A Town building: a stone wall panel with a door, a colored roof (keeps each
// building's existing color identity), and its shop-icon "sign" floating above —
// replacing the flat colored-circle-with-icon look used for monsters/paths. Falls
// back to plain shapes for any piece whose texture didn't load.
static void DrawBuildingNode(Vector2 screenPos, Color roofColor, const std::string& label, bool nearPlayer,
                               const std::string& sublabel, const Texture2D* signIcon,
                               const SpriteSheet* doorAnim = nullptr, const Texture2D* realBuildingTex = nullptr,
                               Color bodyTint = WHITE, float scale = 1.0f) {
    float kNodeRadius = ::kNodeRadius * scale; // shadows the global on purpose — every size
                                                 // below already reads "kNodeRadius", so scaling
                                                 // it locally scales the whole function for free
                                                 // without duplicating every line.
    if (realBuildingTex) {
        // Real building art (CraftPix "Tropical Medieval City" set, assets/town_buildings/)
        // in place of the generic wall+roof+door composite below. Scaled to a consistent
        // on-screen footprint regardless of each building's native aspect ratio, and
        // anchored at its bottom-center — screenPos is the tile's ground point, and
        // these images have a roof going up from there, not a centered blob. bodyTint
        // defaults to WHITE (no change) for every existing building; the House uses it
        // for its chosen hue (see kHouseHues/DrawTownScreen).
        float targetSize = kNodeRadius * 1.89f; // 1.8x + 5%, after that felt slightly too small
        float scale = targetSize / (float)std::max(realBuildingTex->width, realBuildingTex->height);
        float rw = realBuildingTex->width * scale, rh = realBuildingTex->height * scale;
        Vector2 topLeft = { screenPos.x - rw / 2.0f, screenPos.y + kNodeRadius * 0.6f - rh };
        bool onScreenReal = topLeft.x + rw > kViewport.x - 20 && topLeft.x < kViewport.x + kViewport.width + 20 &&
                              topLeft.y + rh > kViewport.y - 40 && topLeft.y < kViewport.y + kViewport.height + 40;
        if (!onScreenReal) return;
        DrawTextureEx(*realBuildingTex, topLeft, 0.0f, scale, bodyTint);
        if (nearPlayer) {
            Rectangle glow = { topLeft.x - 4, topLeft.y - 4, rw + 8, rh + 8 };
            DrawRectangleRoundedLines(glow, 0.15f, 4, kColorSlate);
        }
        int rtw = MeasureUIText(label.c_str(), 13);
        Rectangle rLabelBg = { screenPos.x - rtw / 2.0f - 4, topLeft.y + rh + 2, (float)rtw + 8, 14 };
        DrawRectangleRec(rLabelBg, Fade(kColorPanelBg, 0.9f));
        DrawUIText(label.c_str(), (int)screenPos.x - rtw / 2, (int)(topLeft.y + rh + 4), 13, kColorText);
        if (!sublabel.empty()) {
            int rstw = MeasureUIText(sublabel.c_str(), 11);
            DrawUIText(sublabel.c_str(), (int)screenPos.x - rstw / 2, (int)(topLeft.y + rh + 16), 11, Fade(kColorText, 0.85f));
        }
        return;
    }
    float w = kNodeRadius * 2.0f, h = kNodeRadius * 1.5f;
    Rectangle wallRect = { screenPos.x - w / 2.0f, screenPos.y - h / 2.0f + kNodeRadius * 0.3f, w, h };
    bool onScreen = wallRect.x + wallRect.width > kViewport.x - 20 && wallRect.x < kViewport.x + kViewport.width + 20 &&
                     wallRect.y + wallRect.height > kViewport.y - 40 && wallRect.y < kViewport.y + kViewport.height + 40;
    if (!onScreen) return;

    if (g_assets.dungeonWallOk) DrawStretched(g_assets.dungeonWall, wallRect, WHITE);
    else DrawRectangleRec(wallRect, Fade(GRAY, 0.6f));

    // Roof — a plain triangle, no texture needed; keeps the building's original color
    // as an at-a-glance identity, same role the colored circle used to play.
    Vector2 roofLeft = { wallRect.x - 8, wallRect.y };
    Vector2 roofRight = { wallRect.x + wallRect.width + 8, wallRect.y };
    Vector2 roofTop = { screenPos.x, wallRect.y - h * 0.55f };
    DrawTriangle(roofLeft, roofTop, roofRight, roofColor);

    if (doorAnim && doorAnim->ok) {
        // A little life on approach: shows its closed (first) frame from a distance and
        // its open (last) frame once you're actually near — see the CraftPix "village"
        // animated door sheets this is built from, assets/village/door_*.png.
        float doorW = w * 0.3f, doorH = h * 0.55f;
        int frame = nearPlayer ? doorAnim->frames - 1 : 0;
        float frameW = (float)doorAnim->tex.width / doorAnim->frames;
        Rectangle src = { frame * frameW, 0, frameW, (float)doorAnim->tex.height };
        Rectangle dest = { screenPos.x - doorW / 2.0f, wallRect.y + wallRect.height - doorH, doorW, doorH };
        DrawTexturePro(doorAnim->tex, src, dest, { 0, 0 }, 0.0f, WHITE);
    } else if (g_assets.buildingDoorOk) {
        float doorW = w * 0.3f, doorH = h * 0.55f;
        DrawStretched(g_assets.buildingDoor, { screenPos.x - doorW / 2.0f, wallRect.y + wallRect.height - doorH, doorW, doorH }, WHITE);
    }

    if (nearPlayer) {
        Rectangle glow = { wallRect.x - 4, wallRect.y - 4, wallRect.width + 8, wallRect.height + 8 };
        DrawRectangleRoundedLines(glow, 0.15f, 4, kColorSlate);
    }

    if (signIcon) {
        // Backing plate removed 2026-09-23 at Mark's request ("remove the circle behind
        // all of the images") — was originally added because a translucent backing let
        // grass bleed through and muddy contrast; if a specific icon turns out hard to
        // read against a specific ground color again, that's the thing to revisit.
        Vector2 signPos = { screenPos.x, wallRect.y - h * 0.78f };
        DrawIconCentered(*signIcon, signPos, kNodeRadius * 0.7f, WHITE);
    }

    // Label text on its own solid plate too, for the same reason — dark text alone
    // reads inconsistently against a busy grass texture.
    int tw = MeasureUIText(label.c_str(), 13);
    Rectangle labelBg = { screenPos.x - tw / 2.0f - 4, (float)(wallRect.y + wallRect.height + 2), (float)tw + 8, 14 };
    DrawRectangleRec(labelBg, Fade(kColorPanelBg, 0.9f));
    DrawUIText(label.c_str(), (int)screenPos.x - tw / 2, (int)(wallRect.y + wallRect.height + 4), 13, kColorText);
    if (!sublabel.empty()) {
        int stw = MeasureUIText(sublabel.c_str(), 11);
        DrawUIText(sublabel.c_str(), (int)screenPos.x - stw / 2, (int)(wallRect.y + wallRect.height + 16), 11, Fade(kColorText, 0.85f));
    }
}

// Connects a building to the town plaza with a simple cardinal-direction dirt path —
// a straight line if the building already lines up with the plaza on one axis, an
// L-shaped bend (horizontal then vertical) otherwise. No road drawn if the building
// is already inside the plaza. Avoids any rotation math by only ever using
// axis-aligned bands (reuses DrawWallBand), which keeps this simple and matches the
// blocky, cardinal-direction feel of the rest of this tile-based world.
static void DrawRoadToPlaza(Vector2 buildingPos, Rectangle plaza, Vector2 camera, const Texture2D* dirtTex) {
    const float kRoadWidth = 28.0f * kTownVisualScale;
    const Color kRoadFallback = { 196, 164, 100, 255 };
    bool insideX = buildingPos.x >= plaza.x && buildingPos.x <= plaza.x + plaza.width;
    bool insideY = buildingPos.y >= plaza.y && buildingPos.y <= plaza.y + plaza.height;
    if (insideX && insideY) return; // already standing on the plaza

    if (insideX) {
        float plazaEdgeY = (buildingPos.y < plaza.y) ? plaza.y : plaza.y + plaza.height;
        float top = std::min(buildingPos.y, plazaEdgeY), bottom = std::max(buildingPos.y, plazaEdgeY);
        DrawWallBand({ buildingPos.x - kRoadWidth / 2, top, kRoadWidth, bottom - top }, camera, dirtTex, 48.0f * kTownVisualScale, kRoadFallback);
    } else if (insideY) {
        float plazaEdgeX = (buildingPos.x < plaza.x) ? plaza.x : plaza.x + plaza.width;
        float left = std::min(buildingPos.x, plazaEdgeX), right = std::max(buildingPos.x, plazaEdgeX);
        DrawWallBand({ left, buildingPos.y - kRoadWidth / 2, right - left, kRoadWidth }, camera, dirtTex, 48.0f * kTownVisualScale, kRoadFallback);
    } else {
        // Bend at the plaza's CENTER x, not its edge — this lands exactly on the
        // straight spoke already drawn by the edge-mid building sharing this row
        // (e.g. Carpenter's spoke for the grid used today), so a corner's road
        // merges into that spoke instead of running its own parallel line a few
        // tiles away. Without this, each side of the grid drew 3 near-parallel
        // road strips (the straight spoke plus each corner's own edge-bend) in a
        // narrow gap — reads as a cluttered "road pile-up," not a real network.
        float plazaCenterX = plaza.x + plaza.width / 2.0f;
        float plazaEdgeY = (buildingPos.y < plaza.y) ? plaza.y : plaza.y + plaza.height;
        float left = std::min(buildingPos.x, plazaCenterX), right = std::max(buildingPos.x, plazaCenterX);
        DrawWallBand({ left, buildingPos.y - kRoadWidth / 2, right - left, kRoadWidth }, camera, dirtTex, 48.0f * kTownVisualScale, kRoadFallback);
        float top = std::min(buildingPos.y, plazaEdgeY), bottom = std::max(buildingPos.y, plazaEdgeY);
        DrawWallBand({ plazaCenterX - kRoadWidth / 2, top, kRoadWidth, bottom - top }, camera, dirtTex, 48.0f * kTownVisualScale, kRoadFallback);
    }
}

// Same L-bend idea as DrawRoadToPlaza but between two arbitrary points instead of a
// point and a rectangle — used to connect the Wilderness's Return Gate to each dungeon
// entrance, so the map reads as a place with real paths through it (matching Town's
// road network) instead of open grass with icons scattered on it.
static void DrawWildPath(Vector2 from, Vector2 to, Vector2 camera, const Texture2D* dirtTex) {
    const float kPathWidth = 24.0f;
    const Color kPathFallback = { 196, 164, 100, 255 };
    float left = std::min(from.x, to.x), right = std::max(from.x, to.x);
    float top = std::min(from.y, to.y), bottom = std::max(from.y, to.y);
    DrawWallBand({ from.x - kPathWidth / 2, top, kPathWidth, bottom - top }, camera, dirtTex, 48.0f, kPathFallback);
    DrawWallBand({ left, to.y - kPathWidth / 2, right - left, kPathWidth }, camera, dirtTex, 48.0f, kPathFallback);
}

// Purely decorative foliage scattered around the grass, hand-placed clear of every
// building, road, the plaza, and the two farmland patches so nothing ever renders on
// top of or blocks them. Bumped from 10 to 44 (Mark wanted the town to feel more
// lush/alive) by filling out the 3x3 grid's 4 open quadrants more densely and adding
// four outer bands (north/south/west/east of the whole grid, where there's no road or
// building at all) — see kTownNodePositions/kTownPlaza for the grid this is placed
// around. `variant` picks the icon (0=ground bush, 1=Wilderness tree, 2/3=Wilderness
// bush1/bush2, 4=Wilderness fern — the latter four textures already loaded for the
// Wilderness screen, just reused here for variety instead of one repeated bush;
// 5=autumn bush, "Mage City Arcanos" pack, see the GameAssets comment above — a splash
// of warm color instead of one more green bush).
struct TownFoliage { Vector2 pos; int variant; };
// Repositioned 2026-09-21 for the 250->300-unit grid spacing bump (see kTownNodePositions'
// comment) — every coordinate run through the same piecewise transform used for the grid
// itself (unchanged below 200, scaled 1.2x between 200-700, shifted +100 beyond 700) so
// each piece's relationship to its original landmark (a road spoke, a plaza corner, a
// quadrant's open ground) is preserved exactly rather than left stale against the old grid.
static const std::array<TownFoliage, 44> kFoliagePositions = {{
    // Quadrant interiors (between the grid's cross-shaped roads)
    {{296, 272}, 0}, {{404, 320}, 1}, {{704, 272}, 2}, {{632, 416}, 3},
    {{272, 632}, 4}, {{416, 704}, 0}, {{716, 680}, 1}, {{632, 596}, 2},
    {{344, 416}, 3}, {{248, 320}, 4}, {{584, 368}, 0}, {{680, 320}, 1},
    {{260, 680}, 2}, {{320, 740}, 3}, {{680, 740}, 4}, {{740, 656}, 0},
    // North of the grid (above the top building row, y < 150)
    {{60, 90}, 1}, {{110, 140}, 2}, {{344, 80}, 5}, {{392, 120}, 4},
    {{632, 90}, 0}, {{680, 130}, 1}, {{900, 80}, 2}, {{950, 120}, 3},
    // South of the grid (below the bottom row, clear of the Wilderness Gate's road)
    {{60, 900}, 4}, {{100, 940}, 0}, {{344, 890}, 1}, {{392, 930}, 2},
    {{632, 910}, 3}, {{656, 945}, 4}, {{900, 890}, 5}, {{950, 930}, 1},
    // West of the grid (clear of both farmland patches)
    {{70, 180}, 2}, {{80, 236}, 3}, {{50, 548}, 4}, {{90, 820}, 0},
    {{120, 500}, 1}, {{60, 800}, 5}, {{40, 880}, 3},
    // East of the grid
    {{880, 500}, 2}, {{920, 572}, 3}, {{950, 236}, 4}, {{910, 740}, 0}, {{940, 320}, 1},
}};

// Hand-placed town-flavor props, same "top down village" CraftPix pack as the doors/
// farmland/foliage above (assets/village/*.png) — purely decorative, no collision, same
// role as kFoliagePositions just with a curated small set instead of a repeated icon.
// `kind` indexes the switch in DrawTownScreen's render loop: 0=fountain, 1=streetlamp,
// 2=Smith's sign, 3/4/5=market stalls, 6=lumber pile, 7=barrel, 8=crate, 9=anvil,
// 10=statue.
struct TownProp { Vector2 pos; int kind; float size; };
// Repositioned 2026-09-21 alongside kFoliagePositions — same piecewise transform, same
// reasoning (grid spacing 250->300; every prop's relationship to its original landmark
// preserved exactly, not left stale). Also grew 18->26 the same day (Gemini's "open green
// spaces feel a bit empty" note, which Mark asked to act on) — Tailor/Alchemy/Healer/
// Stable/Bank previously had no prop clutter of their own at all (only Smith/Carpenter/
// Provisioner did); those entries give each of those a small barrel/crate/lumberpile
// cluster, reusing existing icon kinds rather than sourcing new art. Grew again 26->33
// same day once Mark asked for genuinely trade-fitting dressing (like Smith's anvil) —
// kinds 11-17 are new textures, not reused ones (see GameAssets' comment on
// townSheep/etc.): Stable gets real farm animals, Alchemy gets potions, Bank gets a
// chest, Townhall gets a bookshelf, all cropped from two more already-on-disk CC0
// Kenney "Tiny" packs (farm, dungeon) rather than generating anything new.
static const std::array<TownProp, 33> kTownProps = {{
    // Fountain, tucked in a corner of the plaza clear of Town Hall and the crossroads
    // running through the plaza's center.
    {{446, 560}, 0, 40.0f},
    // Statue in the NW quadrant's open ground — "Mage City Arcanos" pack, see the
    // GameAssets comment above. (Originally placed in the plaza itself, but that spot
    // sat behind Town Hall's tall roof sprite from most camera angles since props draw
    // before buildings — moved out to open ground instead of fighting the draw order.)
    {{344, 344}, 10, 44.0f},
    // Street lamps flanking each of the 4 road spokes at its midpoint — reinforces the
    // crossroads shape from the road-merge fix (see DrawRoadToPlaza).
    {{452, 302}, 1, 32.0f}, {{548, 302}, 1, 32.0f},
    {{452, 698}, 1, 32.0f}, {{548, 698}, 1, 32.0f},
    {{302, 452}, 1, 32.0f}, {{302, 548}, 1, 32.0f},
    {{698, 452}, 1, 32.0f}, {{698, 548}, 1, 32.0f},
    // Cinderforge (Smith) gets a hanging anvil sign plus a physical anvil prop — the one
    // building whose available sign icon happens to match its trade exactly.
    {{266, 230}, 2, 30.0f}, {{150, 230}, 9, 26.0f},
    // A lumber pile beside the Hewnwood Hall (Carpenter), plus a crate of fittings.
    {{548, 254}, 6, 32.0f}, {{430, 270}, 8, 20.0f},
    // A small market cluster south of the Provisioner — 3 stall colors plus a barrel
    // and a crate, reading as a little market square rather than a lone building.
    {{740, 860}, 3, 48.0f}, {{800, 880}, 4, 48.0f}, {{850, 860}, 5, 48.0f},
    {{704, 830}, 7, 24.0f}, {{880, 830}, 8, 24.0f},
    // Tailor's crate-and-barrel delivery, east side of the building.
    {{880, 270}, 7, 24.0f}, {{740, 270}, 8, 24.0f},
    // A rain barrel outside Alchemy's garden wall.
    {{270, 560}, 7, 24.0f},
    // A supply crate by the Healer's door.
    {{270, 860}, 8, 24.0f},
    // Feed barrels and a crate flanking the Stable.
    {{870, 560}, 7, 24.0f}, {{870, 440}, 8, 24.0f},
    // A strongbox crate outside the Vaultkeep (Bank).
    {{560, 870}, 8, 24.0f},
    // A small pen of farm animals beside the Stable — sheep, cow, chicken (kinds 11-13).
    {{740, 570}, 11, 22.0f}, {{740, 430}, 12, 24.0f}, {{770, 600}, 13, 18.0f},
    // A pair of brewing potions outside Alchemy (kinds 14-15).
    {{270, 440}, 14, 18.0f}, {{170, 570}, 15, 18.0f},
    // A strongbox chest outside the Vaultkeep, next to its crate (kind 16).
    {{440, 860}, 16, 26.0f},
    // A bookshelf of town records just outside Town Hall (kind 17).
    {{560, 460}, 17, 24.0f},
}};

// ---------------------------------------------------------------------
// Wandering, interactable townsfolk (2026-09-22, "AI players" plan, Part 1) — purely
// decorative NPCs that give Town its first-ever ambient motion (previously fully
// static — only the player ever moved). Walking up and pressing E shows a name +
// greeting, dismissed the same way. No combat, no branching dialogue, no collision
// (walking through one is fine — they're atmosphere, not obstacles).
// ---------------------------------------------------------------------
struct TownNPC { Vector2 homePos; std::string name, greeting; };
static const std::array<TownNPC, 6> kTownNPCs = {{
    { {350, 350}, "Old Miran", "Fine morning for it, isn't it?" },
    { {650, 350}, "Young Petra", "Careful past the gate — I hear the wolves have been bold lately." },
    { {350, 650}, "Wystan the Baker", "Bread's fresh if you've got the coin." },
    { {650, 650}, "Widow Aelith", "You look like you could use a good meal." },
    { {150, 500}, "Cobb the Stableboy", "Mind the horses, they spook easy." },
    { {850, 650}, "Sister Meraude", "May your travels be safe, traveler." },
}};
// Town 2's own flavor (2026-09-22, "second town" plan) — same 6 wander spots (reuses
// Town 1's exact layout, see DrawTownScreen), different names/greetings for a coastal
// trade-port identity (echoing the UO Outlands "Horseshoe Bay" research).
static const std::array<TownNPC, 6> kTown2NPCs = {{
    { {350, 350}, "Harbormaster Thane", "Tide's good today — ships are making fine time." },
    { {650, 350}, "Salty Bjorn", "Careful past the gate — the wilds don't care about your coin." },
    { {350, 650}, "Nessa the Netmender", "Torn nets don't mend themselves, but talk's free." },
    { {650, 650}, "Old Corwin", "Been trading gems out of this bay longer than you've been alive." },
    { {150, 500}, "Dockhand Fenn", "Mind the crates, they shift when the tide turns." },
    { {850, 650}, "Captain Ysolde", "Every port's got a story. This one's got a few too many." },
}};
// TownNPCLivePos (their wander position) is defined later, right after
// WildernessMonsterLivePos — it needs MonsterWanderOffset, which isn't declared yet at
// this point in the file.

// Pushes `pos` out of a circular obstacle if it's overlapping — simple circle-circle
// collision, called once per obstacle after movement so the player can walk right up
// to a building or monster but never through it.
static void ResolveCircleCollision(Vector2& pos, float radius, Vector2 obstaclePos, float obstacleRadius) {
    float dx = pos.x - obstaclePos.x, dy = pos.y - obstaclePos.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float minDist = radius + obstacleRadius;
    if (dist >= minDist) return;
    if (dist < 0.0001f) { pos.x = obstaclePos.x + minDist; return; } // exactly overlapping: push along +x
    float push = (minDist - dist) / dist;
    pos.x += dx * push;
    pos.y += dy * push;
}

// Row order in the CraftPix "4 direction male" sheets: Down, Left, Right, Up.
static int DirRowForFacing(Vector2 facing) {
    if (std::fabs(facing.y) >= std::fabs(facing.x)) return facing.y < 0 ? 3 : 0; // Up : Down
    return facing.x < 0 ? 1 : 2; // Left : Right
}
static const float kHeroWalkFps = 8.0f;
static const float kHeroSpriteScale = 2.625f; // visual size only (x kPlayerRadius) — doesn't affect collision/movement;
                                              // was 5.25 (a deliberate +25% bump for the old CraftPix sprite), dropped
                                              // to 4.2 (pre-bump value) for the new "character options" sprites, then
                                              // halved again to 2.1 — still read as too big at 4.2 in Town — then
                                              // +25% again to 2.625 for the AI-generated knight sprite (see
                                              // kPlayerEdgeMargin, bumped the same 1.25x to match)
// Computes the source-rect crop for one frame of a DirSpriteSheet-backed actor (hero,
// NPCs, monsters, creatures, the companion, the rival adventurer — see the "Carl art"
// integration plan). Picks the row from facing, the column from `anim`'s range on
// `sheet` (Idle holds `idleCol`; Walk/Attack/Cast each cycle their own
// [start, start+count) range at `fps`), and maps through `rowMap`/`rowFlip` for sheets
// whose texture rows aren't already in Down/Left/Right/Up order (see that field's
// comment). Factored out from DrawActorSprite so DrawWorldNode's icon path — used by
// monsters/creatures/NPCs, which need the ring/plate/label chrome DrawActorSprite
// doesn't have — can compute the same crop without duplicating this logic.
static Rectangle ActorSrcRect(const DirSpriteSheet& sheet, Vector2 facing, ActorAnim anim, float worldTime,
                                float fps = 8.0f) {
    int row = DirRowForFacing(facing);
    int frame;
    switch (anim) {
        case ActorAnim::Walk:
            frame = sheet.walkStart[row] + (sheet.walkCount[row] > 0 ? (int)(worldTime * fps) % sheet.walkCount[row] : 0);
            break;
        case ActorAnim::Attack:
            frame = sheet.attackStart[row] + (sheet.attackCount[row] > 0 ? (int)(worldTime * fps) % sheet.attackCount[row] : 0);
            break;
        case ActorAnim::Cast:
            frame = sheet.castStart[row] + (sheet.castCount[row] > 0 ? (int)(worldTime * fps) % sheet.castCount[row] : 0);
            break;
        case ActorAnim::Idle:
        default:
            frame = sheet.idleCol[row];
            break;
    }
    float fw = (float)sheet.frameW, fh = (float)sheet.frameH;
    int texRow = sheet.rowMap[row];
    bool flip = sheet.rowFlip[row];
    // A negative source width tells raylib to sample the frame mirrored horizontally —
    // start at the frame's right edge and read backwards — used for rows synthesized
    // from another direction's art rather than physically flipped pixels on disk.
    return { frame * fw + (flip ? fw : 0.0f), texRow * fh, flip ? -fw : fw, fh };
}
// Draws one DirSpriteSheet-backed actor frame centered on `center` at `size`, rotated
// by `rotationDeg` around its own middle if given (dest.x/y is the rotation pivot in
// screen space once `origin` is nonzero, not the rect's top-left corner — a real gotcha
// the first time this was wired up for the weapon-swing rotation effect, kept here
// since some callers may still pass a nonzero rotation during the transition off that
// effect).
static void DrawActorSprite(const DirSpriteSheet& sheet, Vector2 facing, ActorAnim anim, float worldTime,
                              Vector2 center, float size, float fps = 8.0f, float rotationDeg = 0.0f) {
    if (!sheet.ok) return;
    Rectangle src = ActorSrcRect(sheet, facing, anim, worldTime, fps);
    Rectangle dest = { center.x, center.y, size, size };
    Vector2 origin = { size / 2.0f, size / 2.0f };
    DrawTexturePro(sheet.tex, src, dest, origin, rotationDeg, WHITE);
}
static bool AnyMoveKeyDown() {
    return IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN) ||
           IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
}

// Draws the player as the top-down hero sprite, falling back to the paper doll (see
// DrawPaperdollLayers), then the old flat sprite, then a plain circle, as each
// successive layer of art fails to load, plus (optionally) an "[E] interact" prompt
// above them.
// `combatAnim` (2026-09-23, replacing the old rotation-arc swing hack now that the
// hero sheet has real attack/cast frames) forces Attack or Cast whenever a swing or
// spell-cast is live, taking priority over the normal movement-based Idle/Walk choice
// — Idle is used as the "no override" sentinel since nothing ever needs to force Idle
// specifically over the movement-based choice. Only the heroSheet path uses it — the
// paperdoll/icon/circle fallbacks are static art with no equivalent animation states.
static void DrawPlayer(const GameState& s, Vector2 screenPos, Vector2 facing, const std::string& interactPrompt,
                         float visualScale = 1.0f, ActorAnim combatAnim = ActorAnim::Idle) {
    float kPlayerRadius = ::kPlayerRadius * visualScale; // shadows the global on purpose — see
                                                           // DrawBuildingNode's identical trick.
    if (g_assets.heroSheet.ok) {
        bool moving = AnyMoveKeyDown() || VirtualJoystickIsMoving();
        ActorAnim anim = (combatAnim != ActorAnim::Idle) ? combatAnim : (moving ? ActorAnim::Walk : ActorAnim::Idle);
        DrawActorSprite(g_assets.heroSheet, facing, anim, s.worldTime, screenPos, kPlayerRadius * kHeroSpriteScale, kHeroWalkFps);
    } else if (FindPaperdollTexture("base")) {
        DrawPaperdollLayers(s, screenPos, kPlayerRadius * 2.4f);
    } else if (g_assets.playerOk) {
        DrawIconCentered(g_assets.player, screenPos, kPlayerRadius * 2.4f, WHITE);
    } else {
        DrawCircleV(screenPos, kPlayerRadius, Color{ 63, 94, 150, 255 });
        Vector2 nose = { screenPos.x + facing.x * (kPlayerRadius + 6), screenPos.y + facing.y * (kPlayerRadius + 6) };
        DrawCircleV(nose, 4, Color{ 220, 200, 140, 255 });
    }
    if (!interactPrompt.empty()) {
        int tw = MeasureUIText(interactPrompt.c_str(), 12);
        DrawRectangle((int)screenPos.x - tw / 2 - 6, (int)screenPos.y - kPlayerRadius - 26, tw + 12, 18,
                       Fade(BLACK, 0.6f));
        DrawUIText(interactPrompt.c_str(), (int)screenPos.x - tw / 2, (int)screenPos.y - (int)kPlayerRadius - 23, 12, RAYWHITE);
    }
}


// ---------------------------------------------------------------------
// The Hearthmoot's weekly goals — ported from checkWeeklyReset()/
// addWeeklyProgress()/claimWeeklyGoal()/checkAllWeeklyGoalsComplete()/
// hasWeeklyBlessing() in the JS. Placed early since combat, gathering,
// crafting, and taming all report progress into it.
//
// Simplification: the Weekly Blessing's "+10% combat power" is applied
// (see CombatPower() below), but its "faster recovery" half is not —
// this scaffold still has no passive out-of-combat regen system for it
// to speed up (same gap noted for Shaken in the notoriety section).
// ---------------------------------------------------------------------

static void CheckWeeklyReset(GameState& s) {
    long long now = (long long)std::time(nullptr);
    if (s.weekStartEpoch == 0) { s.weekStartEpoch = now; return; }
    if (now - s.weekStartEpoch >= kWeekSeconds) {
        s.weekStartEpoch = now;
        s.weeklyProgress.fill(0);
        s.weeklyClaimed.fill(false);
        s.blessingClaimedThisWeek = false;
        s.blessingUntilEpoch = 0;
        s.logLine = "The Hearthmoot posts a new set of weekly goals.";
    }
}
static void AddWeeklyProgress(GameState& s, int goalIdx, int amount) {
    if (amount <= 0) return;
    CheckWeeklyReset(s);
    s.weeklyProgress[goalIdx] += amount;
}
static void CheckAllWeeklyGoalsComplete(GameState& s) {
    bool allDone = true;
    for (int i = 0; i < kCoreWeeklyGoalCount; i++) if (!s.weeklyClaimed[i]) { allDone = false; break; }
    if (allDone && !s.blessingClaimedThisWeek) {
        s.blessingClaimedThisWeek = true;
        s.blessingUntilEpoch = s.weekStartEpoch + kWeekSeconds;
        s.logLine = "The Hearthmoot grants its Weekly Blessing — +10% combat power until next week's reset!";
    }
}
static void ClaimWeeklyGoal(GameState& s, int goalIdx) {
    CheckWeeklyReset(s);
    const WeeklyGoalDef& goal = kWeeklyGoals[goalIdx];
    if (s.weeklyProgress[goalIdx] < goal.target || s.weeklyClaimed[goalIdx]) return;
    s.weeklyClaimed[goalIdx] = true;
    s.gold += goal.reward;
    s.logLine = std::string("Claimed weekly goal \"") + goal.label + "\" for " +
                 std::to_string(goal.reward) + " gold.";
    if (goalIdx < kCoreWeeklyGoalCount) CheckAllWeeklyGoalsComplete(s); // Bloodstained bounties don't count toward the Blessing
}
static bool HasWeeklyBlessing(const GameState& s) {
    return s.blessingUntilEpoch > (long long)std::time(nullptr);
}

// ---------------------------------------------------------------------
// The Echo system — ported from CAPPED_SKILL_KEYS/isSkillActive()/
// effectiveSkill()/activeSkillTotal()/setSkillActive() in the JS. Every
// skill still trains freely to its own 120 cap regardless of anything
// here; this only governs whether that skill's value actually counts
// toward gameplay right now. Trade skills (Lumberjacking..Alchemy) were
// never part of this system and are always fully active.
// ---------------------------------------------------------------------

struct CappedSkillDef { const char* label; float GameState::* field; };
static const std::array<CappedSkillDef, 18> kCappedSkills = {{
    {"Swordsmanship", &GameState::swordsmanship}, {"Fencing", &GameState::fencing},
    {"Macing", &GameState::macing}, {"Archery", &GameState::archery}, {"Wrestling", &GameState::wrestling},
    {"Tactics", &GameState::tactics}, {"Anatomy", &GameState::anatomy},
    {"Magic Resistance", &GameState::magicResist}, {"Healing", &GameState::healing},
    {"Magery", &GameState::magery}, {"Eval Int", &GameState::evalInt}, {"Meditation", &GameState::meditation},
    {"Animal Taming", &GameState::animalTaming}, {"Animal Lore", &GameState::animalLore},
    {"Veterinary", &GameState::veterinary},
    {"Stealing", &GameState::stealing}, {"Snooping", &GameState::snooping}, {"Poisoning", &GameState::poisoning},
}};
static const float kTotalSkillCap = 700.0f;

static int CappedSkillIndex(float GameState::* field) {
    for (size_t i = 0; i < kCappedSkills.size(); i++) if (kCappedSkills[i].field == field) return (int)i;
    return -1;
}
// JS effectiveSkill(): the value if active, 0 if benched. Skills outside the Echo
// system (trade skills) aren't looked up this way and are just read directly.
static float EffectiveSkill(const GameState& s, float GameState::* field) {
    int idx = CappedSkillIndex(field);
    if (idx < 0) return s.*field; // not a capped skill — always fully active
    return s.skillActive[idx] ? s.*field : 0.0f;
}
static float ActiveSkillTotal(const GameState& s) {
    float total = 0.0f;
    for (size_t i = 0; i < kCappedSkills.size(); i++) if (s.skillActive[i]) total += s.*(kCappedSkills[i].field);
    return total;
}
// JS setSkillActive(): activating checks the 700 budget first; benching is always free.
static bool SetSkillActive(GameState& s, int idx, bool active) {
    if (idx < 0 || idx >= (int)kCappedSkills.size()) return false;
    if (active) {
        float val = s.*(kCappedSkills[idx].field);
        if (ActiveSkillTotal(s) + val > kTotalSkillCap) {
            s.logLine = "No room in your active build for that (" + std::to_string((int)ActiveSkillTotal(s)) +
                         "/" + std::to_string((int)kTotalSkillCap) + " active) — bench something else first.";
            return false;
        }
        s.skillActive[idx] = true;
    } else {
        s.skillActive[idx] = false;
    }
    return true;
}

// JS activeWeaponCategory(): the equipped weapon's combat category, or Wrestling
// unarmed. Returns the matching skill field so callers can read/train it directly.
static float GameState::* ActiveWeaponSkillField(const GameState& s) {
    const Item* weapon = s.equipped.rightHand ? &*s.equipped.rightHand : (s.equipped.leftHand ? &*s.equipped.leftHand : nullptr);
    if (weapon && weapon->type == ItemType::Weapon) {
        if (weapon->category == "Swordsmanship") return &GameState::swordsmanship;
        if (weapon->category == "Fencing") return &GameState::fencing;
        if (weapon->category == "Macing") return &GameState::macing;
        if (weapon->category == "Archery") return &GameState::archery;
    }
    return &GameState::wrestling;
}
static std::string ActiveWeaponCategoryLabel(const GameState& s) {
    float GameState::* field = ActiveWeaponSkillField(s);
    int idx = CappedSkillIndex(field);
    return idx >= 0 ? kCappedSkills[idx].label : "Wrestling";
}

// ---------------------------------------------------------------------
// Character titles & naming — ported from titleFor()/karmaAdjective()/
// topVocationTitle()/characterDisplayName() in the JS. Skill-tier titles
// (Novice..Legendary) are based on your single highest skill; vocation
// titles (Warrior/Mage/Tamer/Craftsman/Gatherer) group skills into broad
// archetypes the same way, so the two combine into things like "Kind
// Grandmaster Warrior." Rogue skills (Stealing/Snooping/Poisoning) are
// deliberately excluded from both — matches the JS exactly, which has no
// "vocation" for them either.
// ---------------------------------------------------------------------

static std::string TitleFor(float skill) {
    if (skill < 50.0f) return "Novice";
    if (skill < 60.0f) return "Apprentice";
    if (skill < 70.0f) return "Journeyman";
    if (skill < 80.0f) return "Expert";
    if (skill < 90.0f) return "Adept";
    if (skill < 100.0f) return "Master";
    if (skill < 110.0f) return "Grandmaster";
    if (skill < 120.0f) return "Elder";
    return "Legendary";
}
// JS overallSkill(): highest of every trade + capped skill EXCEPT the 3 rogue skills.
static float OverallSkill(const GameState& s) {
    float best = std::max({ s.lumberjacking, s.mining, s.skinning,
                              s.buildingSkill[0], s.buildingSkill[1], s.buildingSkill[2], s.buildingSkill[3] });
    for (int i = 0; i < 15; i++) best = std::max(best, s.*(kCappedSkills[i].field)); // 0-14 excludes Stealing/Snooping/Poisoning (15-17)
    return best;
}
static std::string KarmaAdjective(const GameState& s) {
    if (s.karma <= -60.0f) return "Vile";
    if (s.karma <= -20.0f) return "Wicked";
    if (s.karma >= 60.0f) return "Virtuous";
    if (s.karma >= 20.0f) return "Kind";
    return "";
}
// JS topVocationTitle(): whichever single skill (trade or capped, rogue skills again
// excluded) you've raised highest determines both the tier word and the archetype.
static std::string TopVocationTitle(const GameState& s) {
    struct Entry { float val; int vocation; }; // 0=Gatherer 1=Craftsman 2=Warrior 3=Mage 4=Tamer
    std::vector<Entry> entries = {
        { s.lumberjacking, 0 }, { s.mining, 0 }, { s.skinning, 0 },
        { s.buildingSkill[0], 1 }, { s.buildingSkill[1], 1 }, { s.buildingSkill[2], 1 }, { s.buildingSkill[3], 1 },
    };
    for (int i = 0; i < 9; i++) entries.push_back({ s.*(kCappedSkills[i].field), 2 });   // 5 weapon skills + Tactics/Anatomy/MagicResist/Healing
    for (int i = 9; i < 12; i++) entries.push_back({ s.*(kCappedSkills[i].field), 3 });  // Magery/EvalInt/Meditation
    for (int i = 12; i < 15; i++) entries.push_back({ s.*(kCappedSkills[i].field), 4 }); // Taming/Lore/Vet
    static const char* kVocationNames[5] = { "Gatherer", "Craftsman", "Warrior", "Mage", "Tamer" };
    float bestVal = 0.0f; int bestVocation = -1;
    for (auto& e : entries) if (e.val > bestVal) { bestVal = e.val; bestVocation = e.vocation; }
    if (bestVocation < 0 || bestVal < 10.0f) return "";
    return TitleFor(bestVal) + " " + kVocationNames[bestVocation];
}
static std::string CharacterDisplayName(const GameState& s) {
    std::string name = s.characterName.empty() ? "the Adventurer" : s.characterName;
    std::string vocation = TopVocationTitle(s);
    if (s.titleLordEarned) return vocation.empty() ? ("Lord " + name) : ("Lord " + name + " the " + vocation);
    std::vector<std::string> descParts;
    std::string karma = KarmaAdjective(s);
    if (!karma.empty()) descParts.push_back(karma);
    if (!vocation.empty()) descParts.push_back(vocation);
    if (descParts.empty()) return name;
    std::string joined;
    for (size_t i = 0; i < descParts.size(); i++) { joined += descParts[i]; if (i + 1 < descParts.size()) joined += " "; }
    return name + " the " + joined;
}

static const BuildingDef* FindCraftBuilding(const std::string& key) {
    for (auto& b : kCraftBuildings) if (b.key == key) return &b;
    return nullptr;
}
static int FindCraftBuildingIndex(const std::string& key) {
    for (size_t i = 0; i < kCraftBuildings.size(); i++)
        if (kCraftBuildings[i].key == key) return (int)i;
    return -1;
}

static float RandUnit() { return (float)std::rand() / (float)RAND_MAX; } // [0,1)

// ---------------------------------------------------------------------
// Update (non-render) logic — gathering + upgrade timers.
// Mirrors tickGather()/tickUpgrade() in the JS, minus offline catch-up
// (that needs a save/load system, which this scaffold doesn't have yet).
// ---------------------------------------------------------------------

// JS rollGatherSkillGain(): full range below 80, tapering chance+size from 80-100,
// rare fixed +0.1 past 100. This is the "manual gather" gain roll; auto-gather uses
// a flat 0.1 instead (see UpdateGathering below), matching the JS's autoGather branch.
static float RollGatherSkillGain(float skill) {
    if (skill >= 100.0f) return (RandUnit() < 0.15f) ? 0.1f : 0.0f;
    if (skill >= 80.0f) {
        float t = (skill - 80.0f) / 20.0f;
        float chance = 1.0f - t * 0.75f;
        if (RandUnit() >= chance) return 0.0f;
        float gainMax = std::max(0.1f, 1.1f - t * 1.0f);
        float gainMin = std::max(0.1f, 0.3f - t * 0.2f);
        return gainMin + RandUnit() * (gainMax - gainMin);
    }
    return 0.3f + RandUnit() * 0.8f;
}

// JS gainSkill(): clamps the raw gain so the skill never exceeds `cap`, returns the
// actual amount applied (0 if already at cap).
static float GainSkillCapped(float& skill, float amount, float cap = 120.0f) {
    if (amount <= 0.0f) return 0.0f;
    float actual = std::min(amount, std::max(0.0f, cap - skill));
    skill += actual;
    return actual;
}

// STR/DEX/INT growth (2026-09-20, Mark's own design — the JS prototype's live stat-gain
// path, maybeGainStat(), has no combined cap at all, just 100 per stat; a second,
// more UO-faithful 225-total/125-individual version exists in the JS but was dead code,
// never actually wired to anything). Mark asked for a cap "slightly higher than UO":
// 260 combined, 100 individual — a finished character lands around 100/100/60 or
// 100/90/70, never a single-stat extreme dump.
static const int kStatCapIndividual = 100;
static const int kStatCapTotal = 260;
// A small chance for a successful action to raise a stat by 1, same shape as the JS's
// maybeGainStat() — capped both individually and against the combined total above.
// Keeps `maxHp` (a stored field here, not a live function like the JS's maxHP()) in
// sync whenever `str` itself grows, since nothing else would notice the change.
// Rate multiplier (2026-09-22) — Mark reported stat growth (STR/DEX/INT) feeling far
// too slow relative to skill growth: he GM'd Meditation and Eval Int (~150-200 clicks,
// since RollGatherSkillGain is nearly guaranteed below skill 80) in the time INT moved
// from 20 to 26. At the original flat per-call chances, raising a stat 80 points needs
// on the order of 1000+ successful actions (e.g. 80 / 0.06) — an 8-9x disparity versus
// a skill reaching Grandmaster, which matches what he saw almost exactly. Applied here
// (not at each of the dozen individual MaybeGainStat call sites) so their existing
// relative pacing — gathering's 0.10 vs. combat/craft's 0.06 vs. taming's 0.08 — stays
// intact; a single tunable knob if this number needs revisiting again.
static const float kStatGainRateMultiplier = 3.0f;
static bool MaybeGainStat(GameState& s, int GameState::*statField, float chance) {
    if (s.*statField >= kStatCapIndividual) return false;
    if (s.str + s.dex + s.intStat >= kStatCapTotal) return false;
    if (RandUnit() >= chance * kStatGainRateMultiplier) return false;
    s.*statField += 1;
    if (statField == &GameState::str) { s.maxHp += 1; s.hp += 1; }
    return true;
}

// JS applyCraftGainTaper(): full skill (0-100) gains freely; 80-100 gains taper off in
// both chance and size; 100+ gains rarely, fixed at +0.1 when it happens. Shared by
// crafting (TryCraftItem) and Magery training (ApplySpellTraining below).
static float CraftGainTaper(float skill, float baseGain) {
    if (skill >= 100.0f) return (RandUnit() < 0.15f) ? 0.1f : 0.0f;
    if (skill >= 80.0f) {
        float t = (skill - 80.0f) / 20.0f;
        float chance = 1.0f - t * 0.75f;
        if (RandUnit() >= chance) return 0.0f;
        float gainMax = std::max(0.1f, 1.1f - t * 1.0f);
        return std::min(baseGain, gainMax);
    }
    return baseGain;
}

// JS nextAutoGatherType(): mining first, then wood, then nothing once both hit 100.
static std::string NextAutoGatherType(const GameState& s) {
    if (s.mining < 100.0f) return "ore";
    if (s.lumberjacking < 100.0f) return "wood";
    return "";
}

static const float kAutoGatherMinSkill = 30.0f; // JS AUTO_GATHER_MIN_SKILL

// `seconds` defaults to the JS-matched Town rate (8s per action); the Wilderness's
// walk-up-to-a-node gather nodes pass 5s instead, rewarding active play with a faster
// rate than Town's passive/idle HUD buttons — see the call site in DrawWildernessScreen.
static void TryStartGather(GameState& s, const std::string& resourceKey, float seconds = 8.0f) {
    if (s.gatheringResource.has_value()) { s.logLine = "Already gathering."; return; }
    if (s.ambush.has_value() || s.innocentEncounter.has_value()) { s.logLine = "Deal with what's in front of you first."; return; }
    s.gatheringResource = resourceKey;
    s.gatherSecondsRemaining = seconds;
    s.logLine = "Gathering " + resourceKey + "...";
}

static void ToggleAutoGather(GameState& s) {
    if (s.autoGather) { s.autoGather = false; s.logLine = "Auto-gather stopped."; return; }
    std::string next = NextAutoGatherType(s);
    if (next.empty()) { s.logLine = "Mining and Lumberjacking are already at 100."; return; }
    float skillVal = (next == "wood") ? s.lumberjacking : s.mining;
    if (skillVal < kAutoGatherMinSkill) {
        s.logLine = "Auto-gather requires " + std::to_string((int)kAutoGatherMinSkill) +
                     " " + (next == "wood" ? "Lumberjacking" : "Mining") + " — gather manually until then.";
        return;
    }
    s.autoGather = true;
    s.logLine = "Auto-gather started.";
    if (!s.gatheringResource.has_value()) TryStartGather(s, next);
}

static void UpdateGathering(GameState& s, float dt) {
    if (!s.gatheringResource.has_value()) return;
    s.gatherSecondsRemaining -= dt;
    if (s.gatherSecondsRemaining <= 0.0f) {
        const std::string type = *s.gatheringResource;
        int gained = 3 + (std::rand() % 3); // JS: 3 + Math.floor(Math.random()*3)
        float rawGain = s.autoGather ? 0.1f : RollGatherSkillGain(type == "wood" ? s.lumberjacking : s.mining);

        std::string gainNote;
        if (type == "wood") {
            float gain = GainSkillCapped(s.lumberjacking, rawGain, 120.0f);
            s.wood += gained;
            gainNote = gain > 0 ? " (Lumberjacking +" + std::to_string(gain).substr(0, 4) + ")" : "";
            s.logLine = "Gathered " + std::to_string(gained) + " wood." + gainNote;
        } else { // "ore"
            float gain = GainSkillCapped(s.mining, rawGain, 120.0f);
            s.ore += gained;
            gainNote = gain > 0 ? " (Mining +" + std::to_string(gain).substr(0, 4) + ")" : "";
            s.logLine = "Gathered " + std::to_string(gained) + " ore." + gainNote;
        }
        // JS maybeGainStat(): only manual gathering procs STR/DEX, not Auto-Gather.
        if (!s.autoGather) {
            MaybeGainStat(s, &GameState::str, 0.10f);
            MaybeGainStat(s, &GameState::dex, 0.10f);
        }
        s.gatheringResource.reset();
        s.pendingEncounterCheck = "gather"; // resolved in main(): ambush/innocent roll, then auto-gather chains
        AddWeeklyProgress(s, kGoalGather, gained);
    }
}


static void UpdateUpgrade(GameState& s, float dt) {
    if (!s.upgrading.has_value()) return;
    s.upgrading->secondsRemaining -= dt;
    if (s.upgrading->secondsRemaining <= 0.0f) {
        int idx = FindCraftBuildingIndex(s.upgrading->buildingKey);
        if (idx >= 0) {
            s.buildingLevel[idx] = std::min(5, s.buildingLevel[idx] + 1);
            s.logLine = kCraftBuildings[idx].name + " upgraded to level " +
                        std::to_string(s.buildingLevel[idx]) + "!";
        }
        s.upgrading.reset();
    }
}

// Attempt to start an upgrade for the given craftable building.
// Mirrors the affordability check before startUpgrade() in the JS.
static void TryStartUpgrade(GameState& s, const std::string& key) {
    if (s.upgrading.has_value()) { s.logLine = "Already upgrading a building."; return; }
    int idx = FindCraftBuildingIndex(key);
    if (idx < 0) return;
    const BuildingDef& def = kCraftBuildings[idx];
    int lvl = s.buildingLevel[idx];
    if (lvl >= 5) { s.logLine = def.name + " is already at max level."; return; }
    const BuildingLevel& next = def.levels[lvl]; // levels[0] is level 1, so levels[lvl] is the *next* tier
    int haveResource = (def.resource == Resource::Wood) ? s.wood
                      : (def.resource == Resource::Ore) ? s.ore
                      : (def.resource == Resource::Leather) ? s.leather : 0;
    if (s.gold < next.goldCost || (def.resource != Resource::None && haveResource < next.resourceCost)) {
        s.logLine = "Not enough resources to upgrade " + def.name + ".";
        return;
    }
    s.gold -= next.goldCost;
    if (def.resource == Resource::Wood) s.wood -= next.resourceCost;
    else if (def.resource == Resource::Ore) s.ore -= next.resourceCost;
    else if (def.resource == Resource::Leather) s.leather -= next.resourceCost;
    s.upgrading = UpgradeInProgress{ key, next.upgradeTimeSec };
    s.logLine = "Upgrading " + def.name + " to level " + std::to_string(lvl + 1) + "...";
}

// ---------------------------------------------------------------------
// Combat resolution — ported from getCombatPower()/winChanceAgainst()/
// playerAttackRoll()/monsterAttackRoll()/endCombatWin()/endCombatLoss().
// ---------------------------------------------------------------------

static Pet* ActivePet(GameState& s) {
    for (auto& p : s.pets) if (p.active && p.hp > 0) return &p;
    return nullptr;
}

// A pet's turn in combat: casts its best known offensive spell if it's a Caster with
// mana for one, otherwise a wrestling-style bite. Mirrors the pet branch of
// combatRound(). Called after the player's action and before the monster's counter.
static void ResolvePetTurn(GameState& s) {
    if (!s.combat.has_value()) return;
    Pet* pet = ActivePet(s);
    if (!pet) return;
    CombatState& c = *s.combat;

    if (pet->role == PetRole::Caster) {
        // JS bestPetSpell(): highest-circle known Offensive spell the pet can afford.
        const Spell* best = nullptr;
        for (auto& sp : kSpells) {
            if (sp.type == SpellType::Offensive && pet->magery >= sp.minSkill) {
                if (!best || sp.circle > best->circle) best = &sp;
            }
        }
        if (best && pet->mana >= best->manaCost) {
            float castChance = (pet->magery < best->minSkill) ? 1.0f
                : std::clamp(2.0f + (pet->magery - best->minSkill) / (float)(best->maxSkill - best->minSkill) * 98.0f, 1.0f, 100.0f);
            pet->mana -= best->manaCost;
            if (RandUnit() * 100.0f < castChance) {
                float evalMult = (pet->evalInt * 3.0f / 100.0f) + 1.0f;
                int dmg = std::max(1, (int)std::round(best->baseDamage * evalMult * (0.85f + RandUnit() * 0.3f)));
                c.monsterHP -= dmg;
                c.Log(pet->name + " casts " + best->name + " for " + std::to_string(dmg) + " damage");
            } else {
                c.Log(pet->name + "'s spell fizzles");
            }
            GainSkillCapped(pet->magery, RollGatherSkillGain(pet->magery), 120.0f);
            GainSkillCapped(pet->evalInt, RollGatherSkillGain(pet->evalInt), 120.0f);
            GainSkillCapped(pet->meditation, RollGatherSkillGain(pet->meditation), 120.0f);
            return;
        }
        // falls through to a physical bite if no affordable spell known
    }
    int petPower = std::max(1, (int)std::round(pet->wrestling / 6.0f + pet->str * 0.2f));
    float petHitChance = std::clamp(50.0f + (petPower - c.monster.level) * 4.0f + pet->wrestling * 0.2f, 5.0f, 95.0f);
    if (RandUnit() * 100.0f < petHitChance) {
        int dmg = std::max(1, (int)std::round(petPower * (0.85f + RandUnit() * 0.3f)));
        c.monsterHP -= dmg;
        c.Log(pet->name + " bites for " + std::to_string(dmg) + " damage");
    } else {
        c.Log(pet->name + " misses");
    }
    GainSkillCapped(pet->wrestling, RollGatherSkillGain(pet->wrestling), 120.0f);
    GainSkillCapped(pet->tactics, RollGatherSkillGain(pet->tactics), 120.0f);
    GainSkillCapped(pet->anatomy, RollGatherSkillGain(pet->anatomy), 120.0f);
}

// Live-combat counterpart of ResolvePetTurn above (2026-09-22, AI companion — "AI
// players" plan Part 2) — identical formulas (Caster spell-cast branch preferred,
// wrestling-bite fallback, same skill gains) but writes to s.logLine and a passed-in
// target hp/level instead of CombatState&, mirroring exactly how this session's earlier
// live-combat work adapted ApplyWeaponTraining/CheckMonsterDefeatedAndHandleWin into
// their Live* counterparts. The caller checks targetHp<=0 afterward and resolves the
// win the same way it already does for the player's own live attacks.
static void ResolvePetTurnLive(GameState& s, float& targetHp, int targetLevel) {
    Pet* pet = ActivePet(s);
    if (!pet) return;

    if (pet->role == PetRole::Caster) {
        const Spell* best = nullptr;
        for (auto& sp : kSpells) {
            if (sp.type == SpellType::Offensive && pet->magery >= sp.minSkill) {
                if (!best || sp.circle > best->circle) best = &sp;
            }
        }
        if (best && pet->mana >= best->manaCost) {
            float castChance = (pet->magery < best->minSkill) ? 1.0f
                : std::clamp(2.0f + (pet->magery - best->minSkill) / (float)(best->maxSkill - best->minSkill) * 98.0f, 1.0f, 100.0f);
            pet->mana -= best->manaCost;
            if (RandUnit() * 100.0f < castChance) {
                float evalMult = (pet->evalInt * 3.0f / 100.0f) + 1.0f;
                int dmg = std::max(1, (int)std::round(best->baseDamage * evalMult * (0.85f + RandUnit() * 0.3f)));
                targetHp -= dmg;
                s.logLine = pet->name + " casts " + best->name + " for " + std::to_string(dmg) + " damage";
            } else {
                s.logLine = pet->name + "'s spell fizzles";
            }
            GainSkillCapped(pet->magery, RollGatherSkillGain(pet->magery), 120.0f);
            GainSkillCapped(pet->evalInt, RollGatherSkillGain(pet->evalInt), 120.0f);
            GainSkillCapped(pet->meditation, RollGatherSkillGain(pet->meditation), 120.0f);
            return;
        }
        // falls through to a physical bite if no affordable spell known
    }
    int petPower = std::max(1, (int)std::round(pet->wrestling / 6.0f + pet->str * 0.2f));
    float petHitChance = std::clamp(50.0f + (petPower - targetLevel) * 4.0f + pet->wrestling * 0.2f, 5.0f, 95.0f);
    if (RandUnit() * 100.0f < petHitChance) {
        int dmg = std::max(1, (int)std::round(petPower * (0.85f + RandUnit() * 0.3f)));
        targetHp -= dmg;
        s.logLine = pet->name + " bites for " + std::to_string(dmg) + " damage";
    } else {
        s.logLine = pet->name + " misses";
    }
    GainSkillCapped(pet->wrestling, RollGatherSkillGain(pet->wrestling), 120.0f);
    GainSkillCapped(pet->tactics, RollGatherSkillGain(pet->tactics), 120.0f);
    GainSkillCapped(pet->anatomy, RollGatherSkillGain(pet->anatomy), 120.0f);
}

// Follows the player continuously on the Wilderness/dungeon screens — no leash-to-spawn
// needed (unlike monster AI), a companion never "loses interest". Snaps directly to
// position (rather than easing from wherever it happened to be) on first use or after a
// screen change (detected by an implausibly large jump), so it never visibly flies in
// from {0,0} or across the map.
static const float kCompanionFollowSpeed = 140.0f; // faster than kWildMonsterChaseSpeed (90) so it doesn't lag behind
static const float kCompanionFollowDistance = 36.0f;
// Moderate pace — a helper, not a second player dominating the fight.
static const float kCompanionAttackCooldown = 1.5f;
// The tactical opponent's ranged-strike cooldown (2026-09-22, "AI players" plan Part 3)
// — longer than a plain melee swing so alternating melee/ranged still feels paced, not
// spammy.
static const float kTacticalRangedCooldown = 3.0f;
static void UpdateCompanionFollow(GameState& s, Vector2 playerPos, Vector2 playerFacing, float dt) {
    Vector2 targetPos = { playerPos.x - playerFacing.x * kCompanionFollowDistance,
                            playerPos.y - playerFacing.y * kCompanionFollowDistance };
    float distFromPlayer = Dist(s.companionPos, playerPos);
    if (!s.companionFollowInitialized || distFromPlayer > 400.0f) {
        s.companionPos = targetPos;
        s.companionFollowInitialized = true;
        return;
    }
    Vector2 dir = { targetPos.x - s.companionPos.x, targetPos.y - s.companionPos.y };
    float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (dist > 4.0f) {
        dir.x /= dist; dir.y /= dist;
        float step = std::min(dist, kCompanionFollowSpeed * dt);
        s.companionPos.x += dir.x * step;
        s.companionPos.y += dir.y * step;
    }
}

static int CombatPower(const GameState& s) {
    // JS getCombatPower(): sum of equipped weapon(s), deduped so a 2h weapon occupying
    // both hands isn't counted twice; unarmed falls back to Wrestling/6. Tactics,
    // Anatomy, and Strength each add a real bonus % (all Echo-gated where applicable).
    // The Lumberjacking-axe-specific bonus from the JS is omitted (a narrow, flavor-only
    // case not worth the added complexity here).
    int basePower = 0;
    if (s.equipped.leftHand) basePower += s.equipped.leftHand->power;
    if (s.equipped.rightHand && (!s.equipped.leftHand || s.equipped.rightHand->id != s.equipped.leftHand->id))
        basePower += s.equipped.rightHand->power;
    if (basePower == 0) basePower = std::max(1, (int)std::round(EffectiveSkill(s, &GameState::wrestling) / 6.0f));

    float effTactics = EffectiveSkill(s, &GameState::tactics);
    float effAnatomy = EffectiveSkill(s, &GameState::anatomy);
    float tacticsBonusPct = effTactics / 1.6f + (effTactics >= 100.0f ? 6.25f : 0.0f);
    float anatomyBonusPct = effAnatomy / 2.0f + (effAnatomy >= 100.0f ? 5.0f : 0.0f);
    float strengthBonusPct = s.str * 0.3f + (s.str >= 100 ? 5.0f : 0.0f);
    float power = basePower * (1.0f + (tacticsBonusPct + anatomyBonusPct + strengthBonusPct) / 100.0f);

    if (s.shaken > 0) power *= 0.85f; // JS isShaken(): -15% combat power
    if (HasWeeklyBlessing(s)) power *= 1.1f; // JS hasWeeklyBlessing(): +10%
    return std::max(1, (int)std::round(power));
}

// JS totalDefense(): sum of power across all six armor slots.
static int TotalDefense(const GameState& s) {
    int def = 0;
    for (auto* slot : { &s.equipped.helmet, &s.equipped.gorget, &s.equipped.gloves,
                          &s.equipped.arms, &s.equipped.legs, &s.equipped.chest })
        if (slot->has_value()) def += (*slot)->power;
    return def;
}

// JS: winChanceAgainst() — used for the % shown on each "Hunt" button before you commit.
static float WinChancePreview(const GameState& s, int monsterLevel) {
    int power = CombatPower(s);
    float weaponSkillBonus = EffectiveSkill(s, ActiveWeaponSkillField(s)) * 0.2f;
    float chance = 50.0f + (power - monsterLevel) * 4.0f + weaponSkillBonus;
    return std::clamp(chance, 5.0f, 95.0f);
}

// JS: monsterHitChance() — 50 - dex*0.2, clamped 20-90.
static float MonsterHitChance(const GameState& s) {
    return std::clamp(50.0f - s.dex * 0.2f, 20.0f, 90.0f);
}

// ---------------------------------------------------------------------
// Notoriety, murderers & innocents — ported from notorietyTier()/
// regenNotoriety()/maybeTriggerAmbush()/maybeTriggerInnocentEncounter()/
// snoopInnocent()/stealFromInnocent()/spareInnocent()/murderInnocent()/
// endMurdererWin()/endMurdererLoss()/isShaken() and the guard-zone check
// in switchTab() in the JS.
//
// Simplifications from the original (flagged, not silently dropped):
//   - A failed Snoop away from town normally spins up a full "theft duo"
//     counter-fight (startTheftDuoFight()) in the JS — a whole separate
//     2-attacker combat variant. That's out of scope here, so a failed
//     field Snoop instead applies the same gold/item penalty as a failed
//     town Snoop, just without a fight.
//   - The Bloodstained Road (a separate weekly bounty path for fighting
//     murderers on your own terms) isn't ported — only the random ambush
//     encounters are.
//   - Being Shaken halves HP/mana regen in the JS; this scaffold has no
//     passive out-of-combat HP regen yet, so only the -15% combat power
//     half of the penalty applies.
//   - Vendor price surcharges and the Healer/Bank notoriety penalties
//     aren't ported — those tie to shop systems (Provisioner/Healer/Bank)
//     that don't exist yet in this scaffold.
// ---------------------------------------------------------------------

enum class NotorietyTier { Innocent, Criminal, Murderer };
static const float kNotorietyMurdererThreshold = 50.0f;
static const float kNotorietyDecaySecondsPerPoint = 15.0f;
static const float kNotorietyPerMurder = 25.0f;
static const float kAmbushChance = 0.07f;
static const float kInnocentEncounterChance = 0.05f;
static const float kLordFameThreshold = 250.0f;
static const float kLordKarmaThreshold = 60.0f;

static const std::array<std::string, 5> kMurdererNames = {
    "A cutthroat", "A bandit", "A rogue", "A highwayman", "A killer"
};
static const std::array<std::string, 4> kInnocentNames = {
    "A lone traveler", "A wandering pilgrim", "A tired merchant", "A humble farmer"
};

static NotorietyTier GetNotorietyTier(const GameState& s) {
    if (s.notoriety <= 0.0f) return NotorietyTier::Innocent;
    if (s.notoriety < kNotorietyMurdererThreshold) return NotorietyTier::Criminal;
    return NotorietyTier::Murderer;
}
static std::string NotorietyTierLabel(NotorietyTier t) {
    return t == NotorietyTier::Murderer ? "Murderer" : t == NotorietyTier::Criminal ? "Criminal" : "Innocent";
}
// JS ambushNotorietyMultiplier(): murderers get ambushed far more, criminals somewhat more.
static float AmbushNotorietyMultiplier(NotorietyTier t) {
    return t == NotorietyTier::Murderer ? 2.2f : t == NotorietyTier::Criminal ? 1.5f : 1.0f;
}

static bool IsShaken(const GameState& s) { return s.shaken > 0; }
static void ApplyShaken(GameState& s) { s.shaken = 3; }
static void DecrementShaken(GameState& s) {
    if (s.shaken > 0) {
        s.shaken -= 1;
        if (s.shaken == 0) s.logLine = "You shake off the last of your unease.";
    }
}

// JS gainFame()/gainKarma(): simple accumulators; Karma has no cap, Fame doesn't
// either, but hitting the Fame+Karma thresholds together permanently earns "Lord".
static void GainFame(GameState& s, float amount) {
    s.fame += amount;
    if (!s.titleLordEarned && s.fame >= kLordFameThreshold && s.karma >= kLordKarmaThreshold)
        s.titleLordEarned = true;
}
static void GainKarma(GameState& s, float amount) {
    s.karma += amount;
    if (!s.titleLordEarned && s.fame >= kLordFameThreshold && s.karma >= kLordKarmaThreshold)
        s.titleLordEarned = true;
}

// JS regenNotoriety(): decays continuously, 1 point per 15 seconds, called every frame.
static void RegenNotoriety(GameState& s, float dt) {
    if (s.notoriety > 0.0f) s.notoriety = std::max(0.0f, s.notoriety - dt / kNotorietyDecaySecondsPerPoint);
}

// JS rollMurdererLevel(): scaled to within ~10% of your current combat power either way.
static int RollMurdererLevel(const GameState& s) {
    int power = std::max(3, CombatPower(s));
    float variance = 0.9f + RandUnit() * 0.2f;
    return std::max(1, (int)std::round(power * variance));
}

// On hold (2026-09-23) — Mark reported getting "caught in a loop" a few times from
// these firing (7% base chance after nearly every action — gathering, every monster
// kill — stacking up fast over a normal play session; multiplied up to 2.2x at
// Murderer notoriety tier, which is also exactly when a string of ambushes is most
// punishing). No structural runaway bug found (every trigger site is gated behind "no
// encounter already pending"), so this reads as a frequency/feel problem rather than a
// bug to hunt down further — flip back to true to re-enable once retuned, rather than
// deleting the system.
static const bool kAmbushSystemEnabled = false;
// Returns true if an ambush was triggered (sets s.ambush). Called after gathering
// completes and after a dungeon fight ends — never while something else is pending.
static bool TryTriggerAmbush(GameState& s, const std::string& /*source*/) {
    if (!kAmbushSystemEnabled) return false;
    // Also guards against an active live fight (Wilderness/dungeon) — without this, a
    // live fight left running while the player tabbed to another screen (it pauses,
    // since updateEngaged*MonsterAI only runs inside its own Draw*Screen) could end up
    // coexisting with an ambush triggered elsewhere in the meantime.
    if (s.ambush.has_value() || s.combat.has_value() || s.wildEngaged.has_value() || s.dungeonEngaged.has_value()) return false;
    float chance = kAmbushChance * AmbushNotorietyMultiplier(GetNotorietyTier(s));
    if (RandUnit() >= chance) return false;
    int level = RollMurdererLevel(s);
    const std::string& name = kMurdererNames[std::rand() % kMurdererNames.size()];
    s.ambush = GameState::AmbushEncounter{ name, level };
    s.logLine = name + " blocks your path!";
    return true;
}
static bool TryTriggerInnocentEncounter(GameState& s, const std::string& source) {
    if (s.ambush.has_value() || s.innocentEncounter.has_value() || s.combat.has_value() ||
        s.wildEngaged.has_value() || s.dungeonEngaged.has_value()) return false;
    if (RandUnit() >= kInnocentEncounterChance) return false;
    const std::string& name = kInnocentNames[std::rand() % kInnocentNames.size()];
    int gold = 10 + (std::rand() % 41);
    s.innocentEncounter = GameState::InnocentEncounter{ name, gold, false, source };
    s.logLine = name + " passes by, unaware of you.";
    return true;
}
// JS endMurdererWin(): direct gold reward (no corpse/skinning step), notoriety eases,
// Fame and Karma both rise.
// The Bloodstained Road — ported from bloodstainedTargetFor()/
// fightBloodstainedTier()/graySnoopChoice()/grayStealChoice()/
// endBloodstainedWin() in the JS. Losing a Bloodstained fight (or
// breaking off mid-fight) uses the ordinary EndMurdererLoss() above —
// only winning is special-cased, matching endMurdererWin()'s dispatch.
//
// Simplification: no rarity-item corpse drop (rollCorpseDrop()) on a win
// — same "no rarity items" gap already flagged for ordinary murderers.
// ---------------------------------------------------------------------

static DungeonMonster BloodstainedTargetFor(GameState& s, int pathIdx) {
    const BloodstainedPathDef& path = kBloodstainedPaths[pathIdx];
    int tierIdx = s.bloodstainedProgress[pathIdx];
    const BloodstainedTier& tier = path.tiers[tierIdx];
    int loop = s.bloodstainedLoop[pathIdx];
    float loopScale = 1.0f + loop * 0.15f;
    int playerPower = std::max(3, CombatPower(s));
    int level = std::max(1, (int)std::round(playerPower * tier.mult * loopScale));
    std::string displayName = (tier.isBoss && loop > 0)
        ? tier.label + " (Return " + std::to_string(loop + 1) + ")" : tier.label;

    DungeonMonster m;
    m.name = displayName;
    m.level = level;
    m.baseGold = (int)std::round(level * (tier.isBoss ? 3.0f : 1.5f));
    m.baseLeather = 0;
    m.isMurderer = true;
    m.bloodstainedPathIdx = pathIdx;
    m.bloodstainedTierIdx = tierIdx;
    return m;
}
// JS endBloodstainedWin(): advances the ladder, pays path-specific Fame/Karma/
// Notoriety, and on a boss kill loops the path (tougher next time) and permanently
// unlocks that path's weekly bounty.
static void EndBloodstainedWin(GameState& s) {
    CombatState& c = *s.combat;
    int pathIdx = c.monster.bloodstainedPathIdx;
    int tierIdx = c.monster.bloodstainedTierIdx;
    const BloodstainedPathDef& path = kBloodstainedPaths[pathIdx];
    const BloodstainedTier& tier = path.tiers[tierIdx];

    int goldFound = c.monster.baseGold + (std::rand() % 10);
    s.gold += goldFound;
    AddWeeklyProgress(s, kGoalDefeat, 1);
    AddWeeklyProgress(s, pathIdx == kPathBlue ? kGoalBlueBounty : pathIdx == kPathGray ? kGoalGrayBounty : kGoalRedBounty, 1);
    if (s.bloodstainedProgress[pathIdx] == tierIdx) s.bloodstainedProgress[pathIdx] = tierIdx + 1;

    std::string extraNote;
    if (pathIdx == kPathBlue) { GainFame(s, 5.0f); GainKarma(s, 5.0f); extraNote = " Fame and Karma rise."; }
    else if (pathIdx == kPathRed) { s.notoriety += 8.0f; extraNote = " Your notoriety rises."; }

    std::string msg = "You defeat " + c.monster.name + ", taking " + std::to_string(goldFound) + " gold." + extraNote;
    if (tier.isBoss) {
        bool firstTime = !s.bloodstainedBossDefeated[pathIdx];
        s.bloodstainedBossDefeated[pathIdx] = true;
        s.bloodstainedLoop[pathIdx] += 1;
        s.bloodstainedProgress[pathIdx] = 0;
        msg += firstTime
            ? " You have broken the " + path.name + " path — a new weekly bounty opens at the Hearthmoot!"
            : " " + c.monster.name + " falls again — the " + path.name + " path resets, tougher than before.";
    }
    s.logLine = msg;
    s.combat.reset();
    DecrementShaken(s);
}

static void EndMurdererWin(GameState& s) {
    CombatState& c = *s.combat;
    if (c.monster.bloodstainedPathIdx >= 0) { // JS endMurdererWin(): dispatches to endBloodstainedWin() first
        EndBloodstainedWin(s);
        return;
    }
    int goldFound = c.monster.baseGold + (std::rand() % 10);
    s.gold += goldFound;
    bool redeemed = s.notoriety > 0.0f;
    s.notoriety = std::max(0.0f, s.notoriety - 10.0f);
    GainFame(s, 10.0f);
    GainKarma(s, 10.0f);
    s.logLine = "You defeat " + c.monster.name + " and take " + std::to_string(goldFound) + " gold." +
                 (redeemed ? " Your notoriety eases." : "");
    s.combat.reset();
    AddWeeklyProgress(s, kGoalDefeat, 1);
}
// JS endMurdererLoss(): outright defeat costs 40% gold + your whole backpack and
// applies Shaken; breaking off mid-fight (fled=true) costs a lighter 15%/15% with
// no HP loss and only eases Shaken by one.
static void EndMurdererLoss(GameState& s, bool fled) {
    CombatState& c = *s.combat;
    if (fled) {
        int goldLost = (int)std::round(s.gold * 0.15f);
        s.gold -= goldLost;
        int itemsToLose = (int)std::round(s.backpack.size() * 0.15f);
        int lostCount = 0;
        for (int i = 0; i < itemsToLose && !s.backpack.empty(); i++) {
            s.backpack.erase(s.backpack.begin() + (std::rand() % s.backpack.size()));
            lostCount++;
        }
        s.logLine = "You break off the fight with " + c.monster.name + ", dropping " +
                     std::to_string(goldLost) + " gold" + (lostCount > 0 ? " and " + std::to_string(lostCount) +
                     " item" + (lostCount == 1 ? "" : "s") : "") + " in your retreat.";
        DecrementShaken(s);
    } else {
        int goldLost = (int)std::round(s.gold * 0.4f);
        s.gold -= goldLost;
        int itemsLost = (int)s.backpack.size();
        s.backpack.clear();
        s.hp = std::max(1, (int)std::round(s.maxHp * 0.2f));
        s.logLine = c.monster.name + " strips you of " + std::to_string(goldLost) + " gold and " +
                     std::to_string(itemsLost) + " item" + (itemsLost == 1 ? "" : "s") + " before vanishing into the shadows.";
        ApplyShaken(s);
    }
    s.combat.reset();
}

// JS snoopInnocent(): reveals the gold and unlocks Steal on success; on failure, a
// town encounter costs a gold/item fine (and Shaken), a field one is simplified (see
// the header note above) to the same fine, without spawning a counter-fight.
static const float kSnoopBaseChance = 25.0f, kSnoopSkillScaling = 0.65f, kSnoopMaxChance = 92.0f;
static const float kStealBaseChance = 20.0f, kStealSkillScaling = 0.7f, kStealMaxChance = 90.0f;
static float SnoopChance(const GameState& s) {
    return std::clamp(kSnoopBaseChance + EffectiveSkill(s, &GameState::snooping) * kSnoopSkillScaling, 2.0f, kSnoopMaxChance);
}
static float StealChance(const GameState& s) {
    return std::clamp(kStealBaseChance + EffectiveSkill(s, &GameState::stealing) * kStealSkillScaling, 2.0f, kStealMaxChance);
}
static void SnoopInnocent(GameState& s) {
    if (!s.innocentEncounter.has_value()) return;
    GameState::InnocentEncounter enc = *s.innocentEncounter;
    bool succeeded = RandUnit() * 100.0f < SnoopChance(s);
    float gain = GainSkillCapped(s.snooping, RollGatherSkillGain(s.snooping), 120.0f);
    std::string gainNote = gain > 0 ? " (Snooping +" + std::to_string(gain).substr(0, 4) + ")" : "";
    if (succeeded) {
        s.innocentEncounter->canSteal = true;
        s.logLine = "You quietly check " + enc.name + "'s belongings — exactly " +
                     std::to_string(enc.gold) + " gold, and you've got a read on them now." + gainNote;
        return;
    }
    s.notoriety += 10.0f;
    s.innocentEncounter.reset();
    int goldLost = (int)std::round(s.gold * 0.3f);
    s.gold -= goldLost;
    int itemsLost = std::min((int)s.backpack.size(), 2);
    for (int i = 0; i < itemsLost && !s.backpack.empty(); i++)
        s.backpack.erase(s.backpack.begin() + (std::rand() % s.backpack.size()));
    ApplyShaken(s);
    s.logLine = enc.name + " catches you lingering too close and shouts for the guard — " +
                 std::to_string(goldLost) + " gold and " + std::to_string(itemsLost) +
                 " item" + (itemsLost == 1 ? "" : "s") + " confiscated." + gainNote;
}
static void StealFromInnocent(GameState& s) {
    if (!s.innocentEncounter.has_value() || !s.innocentEncounter->canSteal) return;
    GameState::InnocentEncounter enc = *s.innocentEncounter;
    bool succeeded = RandUnit() * 100.0f < StealChance(s);
    float gain = GainSkillCapped(s.stealing, RollGatherSkillGain(s.stealing), 120.0f);
    std::string gainNote = gain > 0 ? " (Stealing +" + std::to_string(gain).substr(0, 4) + ")" : "";
    s.innocentEncounter.reset();
    if (succeeded) {
        s.gold += enc.gold;
        GainKarma(s, -5.0f);
        s.notoriety += 10.0f;
        s.logLine = "You lift " + std::to_string(enc.gold) + " gold from " + enc.name +
                     " without them noticing." + gainNote;
    } else {
        s.logLine = "Your nerve fails you at the last second — you let " + enc.name +
                     " walk on, empty-handed but unnoticed." + gainNote;
    }
}
static void SpareInnocent(GameState& s) {
    if (!s.innocentEncounter.has_value()) return;
    bool reduced = s.notoriety > 0.0f;
    s.notoriety = std::max(0.0f, s.notoriety - 5.0f);
    GainKarma(s, 10.0f);
    s.logLine = "You let " + s.innocentEncounter->name + " pass unharmed." +
                 (reduced ? " Your conscience eases slightly." : "");
    s.innocentEncounter.reset();
}
static void MurderInnocent(GameState& s) {
    if (!s.innocentEncounter.has_value()) return;
    GameState::InnocentEncounter enc = *s.innocentEncounter;
    s.gold += enc.gold;
    s.notoriety += kNotorietyPerMurder;
    GainKarma(s, -30.0f);
    s.logLine = "You strike down " + enc.name + " and take " + std::to_string(enc.gold) +
                 " gold. Your notoriety rises.";
    s.innocentEncounter.reset();
}

// JS switchTab(): a Murderer-tier character gets confiscated and bounced out the
// moment they try to enter the crafting buildings — mapped here onto the Craft tab.
static void GuardZoneConfiscateIfMurderer(GameState& s, Screen& targetScreen) {
    if (targetScreen != Screen::Craft || GetNotorietyTier(s) != NotorietyTier::Murderer) return;
    int goldLost = s.gold;
    int itemsLost = (int)s.backpack.size();
    s.gold = 0;
    s.backpack.clear();
    s.logLine = "Town guards spot you at the gate and drive you out — you flee, dropping everything you carried (" +
                 std::to_string(goldLost) + " gold, " + std::to_string(itemsLost) + " items).";
    targetScreen = Screen::Town;
}

static void StartCombat(GameState& s, int dungeonIdx, const DungeonMonster& monster) {
    if (s.combat.has_value() || s.ambush.has_value() || s.innocentEncounter.has_value()) return;
    CombatState c;
    c.dungeonIdx = dungeonIdx;
    c.monster = monster;
    c.monsterMaxHP = std::max(1, (int)std::round(monster.level * 3.0f)); // JS: monsterMaxHP()
    c.monsterHP = c.monsterMaxHP;
    c.Log("You engage the " + monster.name + "!");
    s.combat = c;
    s.logLine = "You engage the " + monster.name + "!";
}

// Blue/Red fight immediately; Gray opens a Snoop/Steal/Fight choice instead.
static void FightBloodstainedTier(GameState& s, int pathIdx) {
    if (s.ambush.has_value() || s.innocentEncounter.has_value() || s.combat.has_value() || s.grayEncounter.has_value())
        return;
    DungeonMonster target = BloodstainedTargetFor(s, pathIdx);
    if (pathIdx == kPathGray) {
        s.grayEncounter = GameState::GrayEncounter{ target.name, target.level, target.baseGold, target.bloodstainedTierIdx, false, 0 };
        s.logLine = "A mercenary contract awaits: " + target.name + ".";
        return;
    }
    StartCombat(s, -1, target);
}

static void GrayFightChoice(GameState& s) {
    if (!s.grayEncounter.has_value()) return;
    GameState::GrayEncounter enc = *s.grayEncounter;
    s.grayEncounter.reset();
    DungeonMonster target;
    target.name = enc.name; target.level = enc.level; target.baseGold = enc.baseGold; target.baseLeather = 0;
    target.isMurderer = true; target.bloodstainedPathIdx = kPathGray; target.bloodstainedTierIdx = enc.tierIdx;
    StartCombat(s, -1, target);
}

// JS graySnoopChoice(): success reveals a discounted "Steal" payout with no fight;
// failure drags you into the fight anyway, with a free hit already landed on you.
static void GraySnoopChoice(GameState& s) {
    if (!s.grayEncounter.has_value()) return;
    GameState::GrayEncounter enc = *s.grayEncounter;
    bool success = RandUnit() * 100.0f < SnoopChance(s);
    float gain = GainSkillCapped(s.snooping, RollGatherSkillGain(s.snooping), 120.0f);
    std::string gainNote = gain > 0 ? " (Snooping +" + std::to_string(gain).substr(0, 4) + ")" : "";
    if (success) {
        s.grayEncounter->canSteal = true;
        s.grayEncounter->previewGold = (int)std::round(enc.baseGold * 0.65f);
        s.logLine = "You get a good read on " + enc.name + " — carrying about " +
                     std::to_string(s.grayEncounter->previewGold) + " gold." + gainNote;
        return;
    }
    s.grayEncounter.reset();
    DungeonMonster target;
    target.name = enc.name; target.level = enc.level; target.baseGold = enc.baseGold; target.baseLeather = 0;
    target.isMurderer = true; target.bloodstainedPathIdx = kPathGray; target.bloodstainedTierIdx = enc.tierIdx;
    StartCombat(s, -1, target);
    if (s.combat.has_value()) {
        if (RandUnit() * 100.0f < MonsterHitChance(s)) {
            float raw = target.level * (0.8f + RandUnit() * 0.6f);
            int dmg = std::max(1, (int)std::round(raw - TotalDefense(s) * 0.3f));
            s.hp = std::max(1, s.hp - dmg); // JS: floored at 1, can't die from the surprise hit itself
            s.combat->Log("Caught off guard — hit for " + std::to_string(dmg) + " damage");
        } else {
            s.combat->Log("Caught off guard — but they miss");
        }
    }
    s.logLine = enc.name + " spots you sizing them up and comes in swinging." + gainNote;
}
static void GrayStealChoice(GameState& s) {
    if (!s.grayEncounter.has_value() || !s.grayEncounter->canSteal) return;
    GameState::GrayEncounter enc = *s.grayEncounter;
    bool success = RandUnit() * 100.0f < StealChance(s);
    float gain = GainSkillCapped(s.stealing, RollGatherSkillGain(s.stealing), 120.0f);
    std::string gainNote = gain > 0 ? " (Stealing +" + std::to_string(gain).substr(0, 4) + ")" : "";
    s.grayEncounter.reset();
    if (success) {
        s.gold += enc.previewGold;
        s.logLine = "You slip away with " + std::to_string(enc.previewGold) + " gold from " + enc.name +
                     " without a fight." + gainNote;
    } else {
        s.logLine = "Your nerve fails you — you let " + enc.name + " pass, empty-handed but unnoticed." + gainNote;
    }
}


// Handles a monster win (corpse drop, dungeon XP) when called with monsterHP <= 0.
// Mirrors endCombatWin() — gold+leather sit on a corpse until skinned, rather than
// being granted immediately (rarity-item drops remain out of scope). Returns true if
// the monster was in fact defeated (and thus combat has ended).
// JS: both endCombatWin() and endCombatLoss() give a 25% chance at a small Magic
// Resistance gain, regardless of whether you were even casting.
static void MaybeGainMagicResist(GameState& s, CombatState& c) {
    if (RandUnit() >= 0.25f) return;
    float gain = GainSkillCapped(s.magicResist, RollGatherSkillGain(s.magicResist), 120.0f);
    if (gain > 0) c.Log("Magic Resistance +" + std::to_string(gain).substr(0, 4));
}

static bool CheckMonsterDefeatedAndHandleWin(GameState& s) {
    CombatState& c = *s.combat;
    if (c.monsterHP > 0) return false;
    if (c.monster.isMurderer) {
        EndMurdererWin(s);
        return true;
    }
    int goldFound = std::max(1, c.monster.baseGold + (std::rand() % 3) - 1);
    s.corpses.push_back({ c.monster.name, c.monster.baseLeather, goldFound });
    std::string msg = "Defeated the " + c.monster.name + "! Corpse left behind with leather and " +
                        std::to_string(goldFound) + " gold to loot.";
    // dungeonIdx < 0 means this isn't one of the 4 curated dungeons (Wilderness
    // monsters use -1, same sentinel as ambushes/Bloodstained) — no dungeon XP/boss
    // ladder to update, and indexing either array below with a negative index would be
    // out of bounds.
    if (!c.monster.isBoss && c.dungeonIdx >= 0) {
        s.dungeonXP[c.dungeonIdx] += c.monster.level;
        const DungeonDef& dungeon = kDungeons[c.dungeonIdx];
        if (s.dungeonXP[c.dungeonIdx] >= dungeon.bossUnlockXp &&
            s.dungeonXP[c.dungeonIdx] - c.monster.level < dungeon.bossUnlockXp) {
            msg += " " + dungeon.boss.name + " is now available!";
        }
    }
    MaybeGainMagicResist(s, c);
    s.logLine = msg;
    s.combat.reset();
    DecrementShaken(s); // JS: every dungeon win eases Shaken by one fight
    AddWeeklyProgress(s, kGoalDefeat, 1);
    if (!TryTriggerAmbush(s, "dungeon")) TryTriggerInnocentEncounter(s, "dungeon");
    return true;
}

// The monster's counter-attack, shared by every player action (physical, offensive
// spell, or heal spell) — mirrors monsterAttackRoll() plus the loss check from
// endCombatLoss(). Also updates playerWasHit for the next round's rollSpellDisrupted().
static void MonsterCounterAndMaybeEnd(GameState& s) {
    if (!s.combat.has_value()) return; // combat already ended (e.g. a win, above)
    CombatState& c = *s.combat;
    // JS: an active pet can draw the monster's attack instead of the player, at a
    // chance depending on its role (tank draws aggro most, caster least).
    Pet* pet = ActivePet(s);
    float petTargetChance = !pet ? 0.0f : (pet->role == PetRole::Tank ? 0.6f
                                          : pet->role == PetRole::Caster ? 0.2f : 0.4f);
    bool targetsPet = pet && RandUnit() < petTargetChance;
    TriggerMonsterAnim(c, CombatAnim::Attack1); // swings regardless of hit/miss, like the player's own attack

    if (RandUnit() * 100.0f < MonsterHitChance(s)) {
        float raw = c.monster.level * (0.8f + RandUnit() * 0.6f);
        int dmg = std::max(1, (int)std::round(raw - TotalDefense(s) * 0.3f));
        if (targetsPet) {
            pet->hp = std::max(0.0f, pet->hp - dmg);
            c.Log("The " + c.monster.name + " hits " + pet->name + " for " + std::to_string(dmg) + " damage");
        } else {
            s.hp -= dmg;
            c.Log("The " + c.monster.name + " hits you for " + std::to_string(dmg) + " damage");
            c.playerWasHit = true;
            TriggerCombatAnim(c, CombatAnim::Hurt);
        }
    } else {
        c.Log("The " + c.monster.name + " misses" + (targetsPet ? (std::string(" ") + pet->name) : std::string("")));
        if (!targetsPet) c.playerWasHit = false;
    }
    if (s.hp <= 0) {
        if (c.monster.isMurderer) {
            EndMurdererLoss(s, false);
            return;
        }
        // --- Loss: mirrors endCombatLoss() ---
        MaybeGainMagicResist(s, c);
        s.hp = std::max(1, (int)std::round(s.maxHp * 0.2f));
        s.logLine = "You were defeated by the " + c.monster.name + " — you retreat, battered.";
        s.combat.reset();
        ApplyShaken(s);
        if (!TryTriggerAmbush(s, "dungeon")) TryTriggerInnocentEncounter(s, "dungeon");
    }
}

// ---------------------------------------------------------------------
// Live Wilderness combat (GameState::ActiveMonster/wildEngaged, first slice of the
// real-time combat rework) — resolved separately from the panel-based state.combat
// system above rather than shoehorned into it, since CombatState's win/loss handlers
// are written around a scrolling combat log (CombatState::Log) that doesn't exist out
// on the map. These three mirror the exact same formulas/rewards as the functions
// above (hit chance, damage, corpse drop, loss retreat, ambush/innocent chaining) —
// only the "where does this state live and how is it presented" part differs. Dungeon
// monsters and ambushes are untouched, still resolved by CheckMonsterDefeatedAndHandleWin/
// MonsterCounterAndMaybeEnd/ResolveCombatRound above.
// ---------------------------------------------------------------------

// Same 25% chance / same formula as MaybeGainMagicResist, just with no combat-log line
// to write to.
static void LiveMaybeGainMagicResist(GameState& s) {
    if (RandUnit() >= 0.25f) return;
    GainSkillCapped(s.magicResist, RollGatherSkillGain(s.magicResist), 120.0f);
}

// Same three skill-gain rolls as ApplyWeaponTraining (active weapon category, Tactics,
// Anatomy), minus the combat-log lines — there's no scrolling log panel out on the map.
static void LiveApplyWeaponTraining(GameState& s) {
    float GameState::* skillField = ActiveWeaponSkillField(s);
    GainSkillCapped(s.*skillField, RollGatherSkillGain(s.*skillField), 120.0f);
    GainSkillCapped(s.tactics, RollGatherSkillGain(s.tactics), 120.0f);
    GainSkillCapped(s.anatomy, RollGatherSkillGain(s.anatomy), 120.0f);
    MaybeGainStat(s, &GameState::str, 0.06f);
    MaybeGainStat(s, &GameState::dex, 0.06f);
}

// Same reward shape as CheckMonsterDefeatedAndHandleWin's non-boss branch with
// dungeonIdx<0 (no dungeon XP/boss ladder — Wilderness monsters aren't part of any of
// the 4 curated dungeons' progression).
static void EndWildMonsterWin(GameState& s, const std::string& name, int baseGold, int baseLeather) {
    int goldFound = std::max(1, baseGold + (std::rand() % 3) - 1);
    s.corpses.push_back({ name, baseLeather, goldFound });
    s.logLine = "Defeated the " + name + "! Corpse left behind with leather and " +
                 std::to_string(goldFound) + " gold to loot.";
    LiveMaybeGainMagicResist(s);
    DecrementShaken(s);
    AddWeeklyProgress(s, kGoalDefeat, 1);
    s.wildEngaged.reset();
    if (!TryTriggerAmbush(s, "dungeon")) TryTriggerInnocentEncounter(s, "dungeon");
}

// Same shape as MonsterCounterAndMaybeEnd's loss branch (the ordinary-monster path —
// Wilderness monsters are never murderers).
static void EndWildMonsterLoss(GameState& s, const std::string& name) {
    LiveMaybeGainMagicResist(s);
    s.hp = std::max(1, (int)std::round(s.maxHp * 0.2f));
    s.logLine = "You were defeated by the " + name + " — you retreat, battered.";
    s.wildEngaged.reset();
    ApplyShaken(s);
    if (!TryTriggerAmbush(s, "dungeon")) TryTriggerInnocentEncounter(s, "dungeon");
}
// Harsher loss consequence once the Rival has proven it can beat the player (see
// GameState::rivalHasBeatenPlayer) — same stakes as the Notoriety system's
// EndMurdererLoss (40% gold + whole backpack on outright defeat) but built on
// EndWildMonsterLoss's live-combat foundation instead, since the real EndMurdererLoss
// needs a CombatState (state.combat) this live-combat path doesn't have — confirmed by
// reading its body, which reads combat.monster.* fields with no live-combat
// equivalent. See the "Rival hunts you" plan for why these can't just be merged.
static void EndWildMonsterMurdererLoss(GameState& s, const std::string& name) {
    LiveMaybeGainMagicResist(s);
    int goldLost = (int)std::round(s.gold * 0.4f);
    s.gold -= goldLost;
    int itemsLost = (int)s.backpack.size();
    s.backpack.clear();
    s.hp = std::max(1, (int)std::round(s.maxHp * 0.2f));
    s.logLine = name + " strips you of " + std::to_string(goldLost) + " gold and " +
                 std::to_string(itemsLost) + " item" + (itemsLost == 1 ? "" : "s") + " before vanishing into the wilds.";
    s.wildEngaged.reset();
    ApplyShaken(s);
    if (!TryTriggerAmbush(s, "dungeon")) TryTriggerInnocentEncounter(s, "dungeon");
}
// Dungeon counterparts of the two functions above (2026-09-22) — same reward shape as
// CheckMonsterDefeatedAndHandleWin/MonsterCounterAndMaybeEnd's ordinary-monster paths,
// just without a CombatState::Log to write to. Unlike Wilderness monsters, a dungeon
// monster's dungeonIdx is always valid, so a non-boss win also advances dungeonXP and
// can unlock the boss — mirrors CheckMonsterDefeatedAndHandleWin's dungeon branch
// exactly (main.cpp, CheckMonsterDefeatedAndHandleWin).
static void EndDungeonMonsterWin(GameState& s, int dungeonIdx, bool isBoss, const std::string& name,
                                    int level, int baseGold, int baseLeather) {
    int goldFound = std::max(1, baseGold + (std::rand() % 3) - 1);
    s.corpses.push_back({ name, baseLeather, goldFound });
    std::string msg = "Defeated the " + name + "! Corpse left behind with leather and " +
                        std::to_string(goldFound) + " gold to loot.";
    if (!isBoss) {
        s.dungeonXP[dungeonIdx] += level;
        const DungeonDef& dungeon = kDungeons[dungeonIdx];
        if (s.dungeonXP[dungeonIdx] >= dungeon.bossUnlockXp &&
            s.dungeonXP[dungeonIdx] - level < dungeon.bossUnlockXp) {
            msg += " " + dungeon.boss.name + " is now available!";
        }
    }
    LiveMaybeGainMagicResist(s);
    s.logLine = msg;
    s.dungeonEngaged.reset();
    DecrementShaken(s);
    AddWeeklyProgress(s, kGoalDefeat, 1);
    if (!TryTriggerAmbush(s, "dungeon")) TryTriggerInnocentEncounter(s, "dungeon");
}
static void EndDungeonMonsterLoss(GameState& s, const std::string& name) {
    LiveMaybeGainMagicResist(s);
    s.hp = std::max(1, (int)std::round(s.maxHp * 0.2f));
    s.logLine = "You were defeated by the " + name + " — you retreat, battered.";
    s.dungeonEngaged.reset();
    ApplyShaken(s);
    if (!TryTriggerAmbush(s, "dungeon")) TryTriggerInnocentEncounter(s, "dungeon");
}

// Resolves one full round of a physical attack: player swings, then the active pet
// (if any) takes its turn, then (if the monster survives) it swings back. Mirrors
// combatRound()'s core melee exchange plus the pet branch.
// JS applyWeaponTraining(): trains the active weapon-category skill (or Wrestling
// unarmed) plus Tactics and Anatomy, every round regardless of hit or miss. Training
// always writes the raw skill value — only usage (hit chance, CombatPower) is
// Echo-gated, matching the JS where benching never blocks training.
static void ApplyWeaponTraining(GameState& s, CombatState& c) {
    float GameState::* skillField = ActiveWeaponSkillField(s);
    float gain = GainSkillCapped(s.*skillField, RollGatherSkillGain(s.*skillField), 120.0f);
    if (gain > 0) c.Log(ActiveWeaponCategoryLabel(s) + " +" + std::to_string(gain).substr(0, 4));
    float tGain = GainSkillCapped(s.tactics, RollGatherSkillGain(s.tactics), 120.0f);
    if (tGain > 0) c.Log("Tactics +" + std::to_string(tGain).substr(0, 4));
    float aGain = GainSkillCapped(s.anatomy, RollGatherSkillGain(s.anatomy), 120.0f);
    if (aGain > 0) c.Log("Anatomy +" + std::to_string(aGain).substr(0, 4));
    if (MaybeGainStat(s, &GameState::str, 0.06f)) c.Log("STR +1");
    if (MaybeGainStat(s, &GameState::dex, 0.06f)) c.Log("DEX +1");
}

static void ResolveCombatRound(GameState& s) {
    if (!s.combat.has_value() || s.combat->phase != CombatPhase::PlayerTurn) return;
    CombatState& c = *s.combat;
    TriggerCombatAnim(c, CombatAnim::Attack1);
    int power = CombatPower(s);
    float weaponSkillBonus = EffectiveSkill(s, ActiveWeaponSkillField(s)) * 0.2f;
    ApplyWeaponTraining(s, c);

    float hitChance = std::clamp(50.0f + (power - c.monster.level) * 4.0f + weaponSkillBonus, 5.0f, 95.0f);
    if (RandUnit() * 100.0f < hitChance) {
        int dmg = std::max(1, (int)std::round(power * (0.85f + RandUnit() * 0.3f)));
        c.monsterHP -= dmg;
        c.Log("You hit for " + std::to_string(dmg) + " damage");
        TriggerMonsterAnim(c, CombatAnim::Hurt);
        if (s.weaponPoisonCharges > 0) { // JS applyWeaponPoisonOnHit(): consumed on every successful hit
            c.monsterHP -= s.weaponPoisonPotency;
            s.weaponPoisonCharges -= 1;
            c.Log("Your poisoned blade takes hold for " + std::to_string(s.weaponPoisonPotency) + " damage");
        }
    } else {
        c.Log("Your attack misses");
    }

    if (CheckMonsterDefeatedAndHandleWin(s)) return;
    ResolvePetTurn(s);
    if (CheckMonsterDefeatedAndHandleWin(s)) return;
    MonsterCounterAndMaybeEnd(s);
}

// Mirrors combatFlee(): a murderer fight routes to the lighter "broke off mid-fight"
// penalty; a dungeon monster gets one free swing at 40% odds, then you always drop
// to 20% HP regardless (matches the JS exactly, quirky as that is).
static void FleeCombat(GameState& s) {
    if (!s.combat.has_value()) return;
    CombatState& c = *s.combat;
    if (c.monster.isMurderer) {
        EndMurdererLoss(s, true);
        return;
    }
    if (RandUnit() < 0.4f) {
        if (RandUnit() * 100.0f < MonsterHitChance(s)) {
            float raw = c.monster.level * (0.8f + RandUnit() * 0.6f);
            int dmg = std::max(1, (int)std::round(raw - TotalDefense(s) * 0.3f));
            s.hp = std::max(0, s.hp - dmg);
            s.logLine = "You flee — the " + c.monster.name + " hits you for " + std::to_string(dmg) + " damage.";
        } else {
            s.logLine = "You flee safely.";
        }
    } else {
        s.logLine = "You flee safely.";
    }
    s.hp = std::max(1, (int)std::round(s.maxHp * 0.2f));
    s.combat.reset();
    DecrementShaken(s);
    if (!TryTriggerAmbush(s, "dungeon")) TryTriggerInnocentEncounter(s, "dungeon");
}

// JS ambushFight()/ambushFlee(): converts a pending ambush into a real fight (using
// the same combat system as dungeon monsters, tagged isMurderer), or lets it pass
// with zero cost.
static void AmbushFight(GameState& s) {
    if (!s.ambush.has_value()) return;
    DungeonMonster murderer;
    murderer.name = s.ambush->name;
    murderer.level = s.ambush->level;
    murderer.baseGold = (int)std::round(s.ambush->level * 1.5f);
    murderer.baseLeather = 0;
    murderer.isMurderer = true;
    s.ambush.reset();
    StartCombat(s, -1, murderer); // dungeonIdx unused for murderer fights
    // The combat panel only exists inside DrawHuntScreen, but an ambush can trigger from
    // any screen (gathering on Town, gathering/taming in the Wilderness, ending a
    // dungeon fight, ...). Without this, choosing Fight silently started combat in state
    // with no screen able to render it — looked exactly like "Fight did nothing".
    s.screen = Screen::Hunt;
}
static void AmbushFlee(GameState& s) {
    if (!s.ambush.has_value()) return;
    s.logLine = "You avoid " + s.ambush->name + " and continue on your way, unharmed.";
    s.ambush.reset();
}

// ---------------------------------------------------------------------
// Bandages & Healing — ported from craftBandages()/buyProvisionerItem()/
// useBandageOutOfCombat()/combatBandage() in the JS. Bandages are a plain
// consumable count (not a backpack item): the Tailor crafts 5 from 2
// leather for free (no skill required), and a Provisioner-stand-in
// purchase (same simplification as TryBuyReagents above) tops them up
// with gold at the JS's real prices. Using one — in or out of combat —
// always trains Healing (and a 30% chance of Anatomy) regardless of
// success, and counts as your turn in combat exactly like a heal spell.
// ---------------------------------------------------------------------

static void CraftBandages(GameState& s) {
    if (s.leather < 2) { s.logLine = "Not enough leather to craft bandages."; return; }
    s.leather -= 2;
    s.bandages += 5;
    s.logLine = "Crafted 5 bandages from 2 leather.";
}
static void TryBuyBandages(GameState& s, int amount, int cost) {
    if (s.gold < cost) { s.logLine = "Not enough gold to buy bandages."; return; }
    s.gold -= cost;
    s.bandages += amount;
    s.logLine = "Bought " + std::to_string(amount) + " bandages for " + std::to_string(cost) + " gold.";
}
// JS: successChance = clamp(20 + effectiveSkill('healing')*0.8, 10, 99).
static float BandageSuccessChance(const GameState& s) {
    return std::clamp(20.0f + EffectiveSkill(s, &GameState::healing) * 0.8f, 10.0f, 99.0f);
}
static int BandageHealAmount(const GameState& s) {
    return (int)std::round(5.0f + EffectiveSkill(s, &GameState::healing) * 0.15f +
                             EffectiveSkill(s, &GameState::anatomy) * 0.05f);
}
// Shared by both use-sites. Matches the JS's exact ordering: the success roll uses
// the skill value *before* this use's training gain, but the heal amount (computed
// only if it succeeded) uses the value *after* — quirky, but faithful to the original.
static std::string ApplyBandage(GameState& s) {
    s.bandages -= 1;
    bool succeeded = RandUnit() * 100.0f < BandageSuccessChance(s);
    float healGain = GainSkillCapped(s.healing, RollGatherSkillGain(s.healing), 120.0f);
    std::string note = healGain > 0 ? " (Healing +" + std::to_string(healGain).substr(0, 4) + ")" : "";
    if (RandUnit() < 0.3f) {
        float aGain = GainSkillCapped(s.anatomy, RollGatherSkillGain(s.anatomy), 120.0f);
        if (aGain > 0) note += " (Anatomy +" + std::to_string(aGain).substr(0, 4) + ")";
    }
    if (succeeded) {
        int healAmt = BandageHealAmount(s);
        s.hp = std::min(s.maxHp, s.hp + healAmt);
        return "Bandage successful — healed " + std::to_string(healAmt) + "." + note;
    }
    return "The bandage fails to help." + note;
}
static void UseBandageOutOfCombat(GameState& s) {
    if (s.combat.has_value()) return;
    if (s.bandages < 1) { s.logLine = "No bandages left."; return; }
    if (s.hp >= s.maxHp) { s.logLine = "Already at full HP."; return; }
    s.logLine = ApplyBandage(s);
}
// Mirrors combatBandage(): healing yourself still counts as your turn, so the pet
// and monster still take theirs afterward — same shape as CastHealSpell above.
static void UseBandageInCombat(GameState& s) {
    if (!s.combat.has_value() || s.combat->phase != CombatPhase::PlayerTurn) return;
    if (s.bandages < 1) { s.logLine = "No bandages left."; return; }
    CombatState& c = *s.combat;
    TriggerCombatAnim(c, CombatAnim::Defend);
    c.Log(ApplyBandage(s));
    ResolvePetTurn(s);
    if (CheckMonsterDefeatedAndHandleWin(s)) return;
    MonsterCounterAndMaybeEnd(s);
}

// ---------------------------------------------------------------------
// Magic / spellcasting logic — see the "Magic / spellcasting" data section
// above for what's simplified. Ported from evalIntMultiplier()/
// spellPowerFor()/spellSuccessChance()/rollSpellDisrupted()/
// applySpellTraining()/regenMana() and the spell branches of
// playerAttackRoll()/combatHealSpell()/startCast() in the JS.
// ---------------------------------------------------------------------

// Flat reagent cost for any spell cast in live combat (Wilderness or dungeons),
// overriding the panel system's per-circle Spell::reagentCost (1-4) for this system
// specifically — Mark's explicit ask, a deliberate simplification "for now" rather than
// scaling reagent cost by spell circle the way the turn-based panel does.
static const int kLiveCombatReagentCost = 1;
static float MaxMana(const GameState& s) { return (float)s.intStat; } // JS currentMaxMana()
static float EvalIntMultiplier(const GameState& s) { return (EffectiveSkill(s, &GameState::evalInt) * 3.0f / 100.0f) + 1.0f; }
static int SpellPowerFor(const GameState& s, const Spell& spell) {
    return (int)std::round(spell.baseDamage * EvalIntMultiplier(s));
}
// JS spellSuccessChance(): 1% below minSkill, then scales 2%-100% across [minSkill,maxSkill].
static float SpellSuccessChance(const GameState& s, const Spell& spell) {
    float magery = EffectiveSkill(s, &GameState::magery);
    if (magery < spell.minSkill) return 1.0f;
    float pct = 2.0f + (magery - spell.minSkill) / (float)(spell.maxSkill - spell.minSkill) * 98.0f;
    return std::clamp(pct, 1.0f, 100.0f);
}
// JS rollSpellDisrupted(): only checked if the player was hit last round, tapering
// down as Magic Resistance trains (Echo-gated, like every other capped skill here).
static bool RollSpellDisrupted(const GameState& s) {
    if (!s.combat.has_value() || !s.combat->playerWasHit) return false;
    return RandUnit() * 100.0f < std::max(5.0f, 30.0f - EffectiveSkill(s, &GameState::magicResist) * 0.2f);
}
// JS regenMana(): passive regen scaled by Meditation, called every frame like gathering.
static void RegenMana(GameState& s, float dt) {
    float regenPerSec = 0.15f + s.meditation * 0.004f;
    if (IsShaken(s)) regenPerSec *= 0.5f;
    if (HasWeeklyBlessing(s)) regenPerSec *= 1.25f;
    s.mana = std::min(MaxMana(s), s.mana + regenPerSec * dt);
}

// JS applySpellTraining(): every cast (combat or practice) trains Magery (with the
// same overshoot taper crafting uses), plus a small Eval Int and Meditation roll.
static void ApplySpellTraining(GameState& s, const Spell& spell, std::string& logOut) {
    float overshoot = std::max(0.0f, s.magery - spell.minSkill);
    float mageryBase = std::max(0.05f, 0.6f - overshoot * 0.03f + (RandUnit() * 0.2f - 0.1f));
    // Capped at 100 (classic UO convention), not the 120 every other skill here uses —
    // Mark's explicit call. Note: kSpells' two hardest entries (Inferno Strike maxSkill
    // 110, Summon Fiend maxSkill 120) can now never quite reach 100% success chance
    // (SpellSuccessChance scales toward maxSkill) — left as-is since only the training
    // cap was asked for, not a spell-balance pass; flagged to Mark, easy to adjust later
    // if he wants those two spells reachable at exactly 100%.
    float mageryGain = GainSkillCapped(s.magery, CraftGainTaper(s.magery, mageryBase), 100.0f);
    float evalGain = GainSkillCapped(s.evalInt, RollGatherSkillGain(s.evalInt), 120.0f);
    float medGain = GainSkillCapped(s.meditation, RollGatherSkillGain(s.meditation), 120.0f);
    std::vector<std::string> notes;
    if (mageryGain > 0) notes.push_back("Magery +" + std::to_string(mageryGain).substr(0, 4));
    if (evalGain > 0) notes.push_back("Eval Int +" + std::to_string(evalGain).substr(0, 4));
    if (medGain > 0) notes.push_back("Meditation +" + std::to_string(medGain).substr(0, 4));
    if (MaybeGainStat(s, &GameState::intStat, 0.06f)) notes.push_back("INT +1");
    for (size_t i = 0; i < notes.size(); i++) logOut += (i == 0 ? " (" : ", ") + notes[i] + (i + 1 == notes.size() ? ")" : "");
}

// Casts an Offensive spell in combat as an alternative to a physical attack. Mirrors
// the spell branch of playerAttackRoll() plus the shared win/counter-attack handling.
static void CastOffensiveSpell(GameState& s, int spellIdx) {
    if (!s.combat.has_value() || s.combat->phase != CombatPhase::PlayerTurn) return;
    const Spell& spell = kSpells[spellIdx];
    if (spell.type != SpellType::Offensive) return;
    if (s.mana < spell.manaCost || s.reagents < spell.reagentCost) {
        s.logLine = "Not enough mana or reagents for " + spell.name + ".";
        return;
    }
    CombatState& c = *s.combat;
    TriggerCombatAnim(c, CombatAnim::Attack2);
    bool disrupted = RollSpellDisrupted(s);
    s.mana -= spell.manaCost;
    s.reagents -= spell.reagentCost;
    std::string note;
    if (disrupted) {
        ApplySpellTraining(s, spell, note);
        c.Log("Your wounds disrupt the casting of " + spell.name + "!" + note);
    } else {
        bool success = RandUnit() * 100.0f < SpellSuccessChance(s, spell);
        ApplySpellTraining(s, spell, note);
        if (success) {
            int dmg = std::max(1, (int)std::round(SpellPowerFor(s, spell) * (0.85f + RandUnit() * 0.3f)));
            c.monsterHP -= dmg;
            c.Log(spell.name + " hits for " + std::to_string(dmg) + " damage" + note);
            TriggerMonsterAnim(c, CombatAnim::Hurt);
        } else {
            c.Log(spell.name + " fizzles!" + note);
        }
    }
    if (CheckMonsterDefeatedAndHandleWin(s)) return;
    ResolvePetTurn(s);
    if (CheckMonsterDefeatedAndHandleWin(s)) return;
    MonsterCounterAndMaybeEnd(s);
}

// Casts a heal spell (Mending Word / Greater Mending) in combat. Mirrors
// combatHealSpell() — healing yourself still counts as your turn, so the pet and
// monster still take theirs afterward.
static void CastHealSpell(GameState& s, int spellIdx) {
    if (!s.combat.has_value() || s.combat->phase != CombatPhase::PlayerTurn) return;
    const Spell& spell = kSpells[spellIdx];
    if (spell.type != SpellType::Utility) return;
    if (s.mana < spell.manaCost || s.reagents < spell.reagentCost) {
        s.logLine = "Not enough mana or reagents for " + spell.name + ".";
        return;
    }
    CombatState& c = *s.combat;
    TriggerCombatAnim(c, CombatAnim::Protect);
    bool disrupted = RollSpellDisrupted(s);
    s.mana -= spell.manaCost;
    s.reagents -= spell.reagentCost;
    std::string note;
    if (disrupted) {
        ApplySpellTraining(s, spell, note);
        c.Log("Your wounds disrupt the casting of " + spell.name + "!" + note);
    } else {
        bool success = RandUnit() * 100.0f < SpellSuccessChance(s, spell);
        ApplySpellTraining(s, spell, note);
        if (success) {
            int healAmt = SpellPowerFor(s, spell);
            s.hp = std::min(s.maxHp, s.hp + healAmt);
            c.Log(spell.name + " heals you for " + std::to_string(healAmt) + note);
        } else {
            c.Log(spell.name + " fizzles!" + note);
        }
    }
    ResolvePetTurn(s);
    if (CheckMonsterDefeatedAndHandleWin(s)) return;
    MonsterCounterAndMaybeEnd(s);
}

// Live-combat counterpart of CastHealSpell above (2026-09-22, spell hotbar) — shared by
// both Wilderness and dungeon live combat since it has no monster targeting at all, so
// unlike the offensive live-cast functions it needs no per-screen variant. The engaged
// monster's own attack cooldown keeps ticking independently regardless of what the
// player does, so there's nothing else to resolve here (no RollSpellDisrupted either,
// same simplification the offensive live casts already make — no equivalent state to
// read out on the map). Cooldown/affordability are checked by the caller (the hotbar
// row), same division of responsibility as tryCastSpellAtEngagedMonster's call sites.
static void CastLiveUtilitySpell(GameState& s, int spellIdx) {
    const Spell& spell = kSpells[spellIdx];
    s.mana -= spell.manaCost;
    s.reagents -= kLiveCombatReagentCost;
    std::string note;
    bool success = RandUnit() * 100.0f < SpellSuccessChance(s, spell);
    ApplySpellTraining(s, spell, note);
    if (success) {
        int healAmt = SpellPowerFor(s, spell);
        s.hp = std::min(s.maxHp, s.hp + healAmt);
        s.logLine = spell.name + " heals you for " + std::to_string(healAmt) + note;
    } else {
        s.logLine = spell.name + " fizzles!" + note;
    }
}

// Risk-free practice outside combat — mirrors startCast()/tickCast(): spends mana
// (not reagents) purely to train Magery/Eval Int/Meditation, no combat effect.
//
// 2026-09-21: Mark reported being able to practice a Circle 7 spell at only 10
// Magery, calling it out as a bug — the original JS this was ported from has the
// exact same gap (mana-only gate, no skill check at all), so this is a deliberate
// deviation from the port, not a fix to a porting mistake. Added the same hard
// skill gate combat casting already uses (EffectiveSkill >= spell.minSkill); this
// only works because Circle 1's minSkill was also dropped from 10 to 0 in kSpells
// below (a starting-Magery-0 character could never practice ANYTHING otherwise —
// found and flagged before implementing, not discovered after).
static bool CanPracticeSpell(const GameState& s, const Spell& spell) {
    return EffectiveSkill(s, &GameState::magery) >= (float)spell.minSkill;
}
static void TryPracticeSpell(GameState& s, int spellIdx) {
    const Spell& spell = kSpells[spellIdx];
    if (!CanPracticeSpell(s, spell)) {
        s.logLine = "Magery too low to attempt " + spell.name + " (needs " + std::to_string(spell.minSkill) + ").";
        return;
    }
    if (s.mana < spell.manaCost) {
        s.logLine = "Not enough mana to practice " + spell.name + " (needs " + std::to_string(spell.manaCost) + ").";
        return;
    }
    s.mana -= spell.manaCost;
    std::string note;
    ApplySpellTraining(s, spell, note);
    s.logLine = "Practiced " + spell.name + "." + note;
}

// Simplification stand-in for the Provisioner (not ported): buy reagents for gold
// so Magic doesn't get permanently stuck once the starting 5 run out.
static void TryBuyReagents(GameState& s, int amount) {
    int cost = amount; // 1 gold per reagent — arbitrary, flagged as a stand-in
    if (s.gold < cost) { s.logLine = "Not enough gold to buy reagents."; return; }
    s.gold -= cost;
    s.reagents += amount;
    s.logLine = "Bought " + std::to_string(amount) + " reagents for " + std::to_string(cost) + " gold.";
}

// JS meditate(): a single instant click, not a channeled/repeating action — the C++
// port previously had this wrong (a toggle that looped forever training a flat 5-6
// point Meditation gain every 8s and never restored any mana at all; see memory/commit
// history). One press: no-ops in combat, no-ops at full mana (just a log line, no skill
// roll spent), otherwise rolls the same small gathering-curve Meditation gain used
// everywhere else (RollGatherSkillGain) and restores round(8 + meditation*0.3) mana
// (using the just-updated skill value, matching the JS's gainSkill-then-read order),
// capped at max mana.
static void Meditate(GameState& s) {
    if (s.combat.has_value() || s.ambush.has_value() || s.innocentEncounter.has_value()) {
        s.logLine = "You can't meditate right now.";
        return;
    }
    if (s.mana >= MaxMana(s)) {
        s.logLine = "Already at full mana.";
        return;
    }
    float gain = GainSkillCapped(s.meditation, RollGatherSkillGain(s.meditation), 120.0f);
    float restoreAmt = std::round(8.0f + s.meditation * 0.3f);
    s.mana = std::min(MaxMana(s), s.mana + restoreAmt);
    std::string msg = "You meditate, restoring " + std::to_string((int)restoreAmt) + " mana";
    if (gain > 0.0f) msg += " (Meditation +" + std::to_string(gain).substr(0, 4) + ")";
    s.logLine = msg + ".";
}

// ---------------------------------------------------------------------
// Taming & pets logic — see the "Taming & pets" data section above for
// what's simplified. Ported from tameChance()/petSlotCapacity()/
// resolveTameAttempt()/healPet()/sellPet()/trainPetSkill() and the pet's
// turn inside combatRound() in the JS.
// ---------------------------------------------------------------------

static const float kTameSkillCushion = 10.0f;

// JS tameChance(): 0 below (difficulty - cushion) in both Taming and Lore (no cushion
// for apex creatures); otherwise a gentle slope with a hard ceiling that gets stricter
// for tougher creatures, so even a maxed-out tamer never has a sure thing on a dragon.
static float TameChance(const GameState& s, const WildCreature& creature) {
    float taming = EffectiveSkill(s, &GameState::animalTaming);
    float lore = EffectiveSkill(s, &GameState::animalLore);
    float required = creature.isApex ? (float)creature.difficulty
                                       : std::max(0.0f, creature.difficulty - kTameSkillCushion);
    if (taming < required || lore < required) return 0.0f;
    float raw = 50.0f + (taming - creature.difficulty) * 1.2f;
    float ceiling = std::max(50.0f, 95.0f - creature.difficulty * 0.35f);
    return std::clamp(raw, 2.0f, ceiling);
}

// JS petSlotCapacity(): 1-5 slots from Taming+Lore+Veterinary combined.
static int PetSlotCapacity(const GameState& s) {
    float total = EffectiveSkill(s, &GameState::animalTaming) + EffectiveSkill(s, &GameState::animalLore) +
                    EffectiveSkill(s, &GameState::veterinary);
    return std::clamp(1 + (int)(total / 60.0f), 1, 5);
}


static void RegenPetMana(Pet& pet, float dt) {
    float regenPerSec = 0.15f + pet.meditation * 0.004f;
    pet.mana = std::min(pet.maxMana, pet.mana + regenPerSec * dt);
}
static void RegenAllPetMana(GameState& s, float dt) {
    for (auto& p : s.pets) RegenPetMana(p, dt);
}

static int RollInRange(const std::array<int, 2>& range) {
    return range[0] + (std::rand() % (range[1] - range[0] + 1));
}

static void TryStartTameAttempt(GameState& s, int creatureIdx) {
    if (s.tamingAttempt.has_value() || s.ambush.has_value() || s.innocentEncounter.has_value()) return;
    const WildCreature& creature = kWildCreatures[creatureIdx];
    if (TameChance(s, creature) <= 0.0f) {
        s.logLine = "You need at least " + std::to_string(creature.difficulty) +
                     " Animal Taming and Animal Lore to attempt a " + creature.name + ".";
        return;
    }
    s.tamingAttempt = TamingAttempt{ creatureIdx, 4.0f }; // JS TAME_ATTEMPT_DURATION: 4s
    s.logLine = "Attempting to tame the " + creature.name + "...";
}

// JS resolveTameAttempt(): rolls success, trains Taming (with the same overshoot
// taper crafting/magic use) and a little Lore either way, then creates the pet.
static void ResolveTameAttempt(GameState& s) {
    if (!s.tamingAttempt.has_value()) return;
    const WildCreature& creature = kWildCreatures[s.tamingAttempt->creatureIdx];
    float chance = TameChance(s, creature);
    bool succeeded = RandUnit() * 100.0f < chance;

    float overshoot = std::max(0.0f, s.animalTaming - creature.difficulty);
    float tameBase = std::max(0.05f, 0.6f - overshoot * 0.03f + (RandUnit() * 0.2f - 0.1f));
    float gain = GainSkillCapped(s.animalTaming, CraftGainTaper(s.animalTaming, tameBase), 120.0f);
    float loreGain = GainSkillCapped(s.animalLore, RollGatherSkillGain(s.animalLore) * 0.5f, 120.0f);
    std::string gainNote;
    if (gain > 0) gainNote += " (Taming +" + std::to_string(gain).substr(0, 4) + ")";
    if (loreGain > 0) gainNote += " (Lore +" + std::to_string(loreGain).substr(0, 4) + ")";

    if (succeeded) {
        if (MaybeGainStat(s, &GameState::str, 0.08f)) gainNote += " (STR +1)";
        if ((int)s.pets.size() >= PetSlotCapacity(s)) {
            s.logLine = "You tame a " + creature.name + ", but your stable has no free slots — it wanders off." + gainNote;
        } else {
            int str = RollInRange(creature.strRange);
            int dex = RollInRange(creature.dexRange);
            int intV = RollInRange(creature.intRange);
            Pet pet;
            pet.id = s.nextPetId++;
            pet.name = creature.name;
            pet.role = creature.role;
            pet.str = str; pet.dex = dex; pet.intStat = intV;
            pet.maxHp = 50.0f + str; pet.hp = pet.maxHp;
            pet.maxMana = (float)intV; pet.mana = pet.maxMana;
            pet.active = s.pets.empty();
            s.pets.push_back(pet);
            s.logLine = "Tamed a " + creature.name + " (Str " + std::to_string(str) + ", Dex " +
                         std::to_string(dex) + ", Int " + std::to_string(intV) + ")!" + gainNote;
            AddWeeklyProgress(s, kGoalTame, 1);
        }
    } else {
        s.logLine = "The " + creature.name + " resists your taming attempt." + gainNote;
    }
}

static void UpdateTameAttempt(GameState& s, float dt) {
    if (!s.tamingAttempt.has_value()) return;
    s.tamingAttempt->secondsRemaining -= dt;
    if (s.tamingAttempt->secondsRemaining <= 0.0f) {
        ResolveTameAttempt(s);
        s.tamingAttempt.reset();
        s.pendingEncounterCheck = "tame"; // same ambush/innocent roll gathering gets, resolved in main()
    }
}

static void SetPetActive(GameState& s, int petId) {
    for (auto& p : s.pets) p.active = (p.id == petId);
}

static void ReleasePet(GameState& s, int petId) {
    auto it = std::find_if(s.pets.begin(), s.pets.end(), [&](const Pet& p) { return p.id == petId; });
    if (it == s.pets.end()) return;
    s.logLine = "Released " + it->name + " back into the wild.";
    s.pets.erase(it);
}

// JS sellPet(): value scales with stats and total trained skill.
static void SellPet(GameState& s, int petId) {
    auto it = std::find_if(s.pets.begin(), s.pets.end(), [&](const Pet& p) { return p.id == petId; });
    if (it == s.pets.end()) return;
    float skillTotal = it->wrestling + it->tactics + it->anatomy + it->magery + it->evalInt + it->meditation;
    int value = (int)std::round(it->str * 1.5f + it->dex + it->intStat + skillTotal * 0.5f);
    s.gold += value;
    s.logLine = "Sold " + it->name + " for " + std::to_string(value) + " gold.";
    s.pets.erase(it);
}

// JS healPet(): Veterinary-scaled success chance and heal amount.
static void HealPet(GameState& s, int petId) {
    auto it = std::find_if(s.pets.begin(), s.pets.end(), [&](Pet& p) { return p.id == petId; });
    if (it == s.pets.end() || it->hp >= it->maxHp) return;
    float successChance = std::clamp(20.0f + s.veterinary * 0.8f, 10.0f, 99.0f);
    bool succeeded = RandUnit() * 100.0f < successChance;
    float gain = GainSkillCapped(s.veterinary, RollGatherSkillGain(s.veterinary), 120.0f);
    float loreGain = GainSkillCapped(s.animalLore, RollGatherSkillGain(s.animalLore) * 0.5f, 120.0f);
    std::string note;
    if (gain > 0) note += " (Vet +" + std::to_string(gain).substr(0, 4) + ")";
    if (loreGain > 0) note += " (Lore +" + std::to_string(loreGain).substr(0, 4) + ")";
    if (succeeded) {
        int healAmt = (int)std::round(8.0f + s.veterinary * 0.2f);
        it->hp = std::min(it->maxHp, it->hp + healAmt);
        s.logLine = "Treated " + it->name + " for " + std::to_string(healAmt) + " HP." + note;
    } else {
        s.logLine = "Your treatment doesn't help " + it->name + " this time." + note;
    }
}

// JS trainPetSkill(): instantly train one pet skill to 30 for gold (TRAIN_COST_PER_POINT=8).
static const float kPetTrainTarget = 30.0f;
static const int kTrainCostPerPoint = 8;
static void TrainPetSkillGold(GameState& s, int petId, float Pet::* skillField, const std::string& label) {
    auto it = std::find_if(s.pets.begin(), s.pets.end(), [&](Pet& p) { return p.id == petId; });
    if (it == s.pets.end()) return;
    float current = (*it).*skillField;
    if (current >= kPetTrainTarget) return;
    int cost = (int)std::ceil((kPetTrainTarget - current) * kTrainCostPerPoint);
    if (s.gold < cost) { s.logLine = "Not enough gold to train that."; return; }
    s.gold -= cost;
    (*it).*skillField = kPetTrainTarget;
    s.logLine = "Trained " + it->name + "'s " + label + " to 30 for " + std::to_string(cost) + " gold.";
}

// ---------------------------------------------------------------------
// Save / load — ported from save()/load()/applyOfflineAutoGather() in the
// JS. The JS saves the whole state object as JSON to localStorage on a
// 2-second timer; this scaffold writes a plain key=value text file to
// disk on the same cadence (plus on quit), and loads it once at startup.
//
// Simplifications from the original (flagged, not silently dropped):
//   - No JSON — a hand-rolled key=value / pipe-delimited text format
//     covers every persistent field. It's not meant to be hand-edited,
//     just readable enough to debug.
//   - Transient/in-progress state isn't persisted: an active combat
//     fight, a pending ambush or innocent encounter, an in-flight taming
//     attempt, or a building mid-upgrade. Quitting mid-action loses just
//     that action; all durable progress (gold, resources, skills,
//     backpack, equipped gear, pets, corpses, dungeon XP) is preserved.
//   - Corrupt or missing fields fall back to GameState's own defaults
//     (mirroring the JS's Object.assign-with-defaults pattern) rather
//     than failing the whole load.
// ---------------------------------------------------------------------

// Web save persistence (2026-09-22) — Mark reported losing progress every time he
// reopened the browser. Root cause: Emscripten's default filesystem (MEMFS) is
// entirely in-memory and is wiped the instant the page unloads — SaveGame's plain
// std::ofstream writes were succeeding every autosave, they just never survived a
// reload, since nothing backed that filesystem with real browser storage. Desktop is
// unaffected (a real OS filesystem) and needed no changes.
// Fix: mount IndexedDB-backed storage (IDBFS) at /persist and keep the save file
// there instead. IDBFS's actual read/write to IndexedDB is asynchronous, so this
// needs two-sided glue: `syncfs(true, ...)` pulls last session's save down from
// IndexedDB into the in-memory FS — g_persistReady only flips once that completes,
// and UpdateDrawFrame (see its own comment) holds off calling LoadGame/starting the
// game until it does, rather than reading an empty directory. `syncfs(false, ...)`
// pushes the in-memory FS back up to IndexedDB — called from inside SaveGame itself
// (below) after every write, so nothing new to remember at future SaveGame call sites.
#ifdef __EMSCRIPTEN__
static const char* kSaveFilePath = "/persist/townforge_save.txt";
static bool g_persistReady = false;
EM_JS(void, JS_InitPersistence, (), {
    try {
        FS.mkdir('/persist');
    } catch (e) {}
    FS.mount(IDBFS, {}, '/persist');
    FS.syncfs(true, function(err) {
        if (err) console.error('Town Forge: IDBFS initial load failed', err);
        Module._TF_persistReady = 1;
    });
});
EM_JS(int, JS_PersistReady, (), {
    return (typeof Module._TF_persistReady !== 'undefined' && Module._TF_persistReady) ? 1 : 0;
});
EM_JS(void, JS_FlushPersistence, (), {
    FS.syncfs(false, function(err) {
        if (err) console.error('Town Forge: IDBFS save flush failed', err);
    });
});
#else
static const char* kSaveFilePath = "townforge_save.txt";
#endif

static std::vector<std::string> SplitStr(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) out.push_back(item);
    return out;
}

// Item <-> "id|name|type|slot|handed|power|category" (type: 0=Weapon,1=Armor,2=Potion)
static std::string ItemToLine(const Item& it) {
    return std::to_string(it.id) + "|" + it.name + "|" + std::to_string((int)it.type) + "|" +
            it.slot + "|" + it.handed + "|" + std::to_string(it.power) + "|" + it.category;
}
static std::optional<Item> ItemFromLine(const std::string& line) {
    auto parts = SplitStr(line, '|');
    if (parts.size() < 6) return std::nullopt;
    Item it;
    it.id = std::atoi(parts[0].c_str());
    it.name = parts[1];
    it.type = (ItemType)std::atoi(parts[2].c_str());
    it.slot = parts[3];
    it.handed = parts[4];
    it.power = std::atoi(parts[5].c_str());
    it.category = parts.size() > 6 ? parts[6] : ""; // old saves before category existed
    return it;
}
// Pet <-> "id|name|role|str|dex|int|hp|maxHp|mana|maxMana|wrestling|tactics|anatomy|magery|evalInt|meditation|active"
static std::string PetToLine(const Pet& p) {
    std::ostringstream o;
    o << p.id << "|" << p.name << "|" << (int)p.role << "|" << p.str << "|" << p.dex << "|" << p.intStat << "|"
      << p.hp << "|" << p.maxHp << "|" << p.mana << "|" << p.maxMana << "|" << p.wrestling << "|" << p.tactics << "|"
      << p.anatomy << "|" << p.magery << "|" << p.evalInt << "|" << p.meditation << "|" << (p.active ? 1 : 0);
    return o.str();
}
static std::optional<Pet> PetFromLine(const std::string& line) {
    auto parts = SplitStr(line, '|');
    if (parts.size() < 17) return std::nullopt;
    Pet p;
    p.id = std::atoi(parts[0].c_str());
    p.name = parts[1];
    p.role = (PetRole)std::atoi(parts[2].c_str());
    p.str = std::atoi(parts[3].c_str());
    p.dex = std::atoi(parts[4].c_str());
    p.intStat = std::atoi(parts[5].c_str());
    p.hp = (float)std::atof(parts[6].c_str());
    p.maxHp = (float)std::atof(parts[7].c_str());
    p.mana = (float)std::atof(parts[8].c_str());
    p.maxMana = (float)std::atof(parts[9].c_str());
    p.wrestling = (float)std::atof(parts[10].c_str());
    p.tactics = (float)std::atof(parts[11].c_str());
    p.anatomy = (float)std::atof(parts[12].c_str());
    p.magery = (float)std::atof(parts[13].c_str());
    p.evalInt = (float)std::atof(parts[14].c_str());
    p.meditation = (float)std::atof(parts[15].c_str());
    p.active = std::atoi(parts[16].c_str()) != 0;
    return p;
}
// Corpse <-> "monsterName|baseLeather|gold"
static std::string CorpseToLine(const Corpse& c) {
    return c.monsterName + "|" + std::to_string(c.baseLeather) + "|" + std::to_string(c.gold);
}
static std::optional<Corpse> CorpseFromLine(const std::string& line) {
    auto parts = SplitStr(line, '|');
    if (parts.size() < 3) return std::nullopt;
    return Corpse{ parts[0], std::atoi(parts[1].c_str()), std::atoi(parts[2].c_str()) };
}
// PotionStack <-> "name|effect|potency|count"
static std::string PotionToLine(const PotionStack& p) {
    return p.name + "|" + p.effect + "|" + std::to_string(p.potency) + "|" + std::to_string(p.count);
}
static std::optional<PotionStack> PotionFromLine(const std::string& line) {
    auto parts = SplitStr(line, '|');
    if (parts.size() < 4) return std::nullopt;
    return PotionStack{ parts[0], parts[1], std::atoi(parts[2].c_str()), std::atoi(parts[3].c_str()) };
}

static void WriteEquipSlot(std::ofstream& out, const char* key, const std::optional<Item>& slot) {
    out << key << "=" << (slot.has_value() ? ItemToLine(*slot) : std::string("none")) << "\n";
}
static void ReadEquipSlot(const std::string& value, std::optional<Item>& slot) {
    slot = (value == "none" || value.empty()) ? std::nullopt : ItemFromLine(value);
}

static int BackpackCap(const GameState& s) {
    return GameState::kBackpackCap + kHouseTiers[s.houseTierIdx].capBonus;
}

static void SaveGame(const GameState& s) {
    std::ofstream out(kSaveFilePath, std::ios::trunc);
    if (!out.is_open()) return; // JS: storage unavailable — keep playing without persistence
    out << "version=1\n";
    out << "characterName=" << s.characterName << "\n";
    out << "gold=" << s.gold << "\nwood=" << s.wood << "\nore=" << s.ore << "\nleather=" << s.leather << "\n";
    out << "bandages=" << s.bandages << "\n";
    out << "houseTierIdx=" << s.houseTierIdx << "\nhouseHue=" << s.houseHue << "\nhouseName=" << s.houseName << "\n";
    out << "houseModuleLevel=" << s.houseModuleLevel[0] << "," << s.houseModuleLevel[1] << "," << s.houseModuleLevel[2] << "," << s.houseModuleLevel[3] << "\n";
    out << "buildingLevel=" << s.buildingLevel[0] << "," << s.buildingLevel[1] << "," << s.buildingLevel[2] << "," << s.buildingLevel[3] << "\n";
    out << "buildingSkill=" << s.buildingSkill[0] << "," << s.buildingSkill[1] << "," << s.buildingSkill[2] << "," << s.buildingSkill[3] << "\n";
    out << "dungeonXP=" << s.dungeonXP[0] << "," << s.dungeonXP[1] << "," << s.dungeonXP[2] << "," << s.dungeonXP[3] << "," << s.dungeonXP[4] << "\n";
    out << "hp=" << s.hp << "\nmaxHp=" << s.maxHp << "\nstr=" << s.str << "\ndex=" << s.dex << "\n";
    out << "lumberjacking=" << s.lumberjacking << "\nmining=" << s.mining << "\nskinning=" << s.skinning << "\n";
    out << "autoGather=" << (s.autoGather ? 1 : 0) << "\n";
    out << "magery=" << s.magery << "\nevalInt=" << s.evalInt << "\nmeditation=" << s.meditation << "\n";
    out << "intStat=" << s.intStat << "\nmana=" << s.mana << "\nreagents=" << s.reagents << "\n";
    out << "animalTaming=" << s.animalTaming << "\nanimalLore=" << s.animalLore << "\nveterinary=" << s.veterinary << "\n";
    out << "stealing=" << s.stealing << "\nsnooping=" << s.snooping << "\n";
    out << "notoriety=" << s.notoriety << "\nfame=" << s.fame << "\nkarma=" << s.karma << "\n";
    out << "titleLordEarned=" << (s.titleLordEarned ? 1 : 0) << "\nshaken=" << s.shaken << "\n";
    out << "nextItemId=" << s.nextItemId << "\nnextPetId=" << s.nextPetId << "\n";
    out << "lastActiveEpoch=" << (long long)std::time(nullptr) << "\n";
    out << "rivalLevel=" << s.rivalLevel << "\nrivalPosX=" << s.rivalPos.x << "\nrivalPosY=" << s.rivalPos.y <<
           "\nrivalHasBeatenPlayer=" << (s.rivalHasBeatenPlayer ? 1 : 0) << "\n";
    out << "poisoning=" << s.poisoning << "\nweaponPoisonCharges=" << s.weaponPoisonCharges <<
           "\nweaponPoisonPotency=" << s.weaponPoisonPotency << "\n";
    out << "bankGold=" << s.bankGold << "\n";
    out << "weekStartEpoch=" << s.weekStartEpoch << "\nblessingClaimedThisWeek=" << (s.blessingClaimedThisWeek ? 1 : 0) <<
           "\nblessingUntilEpoch=" << s.blessingUntilEpoch << "\n";
    out << "weeklyProgress=";
    for (int i = 0; i < kWeeklyGoalCount; i++) out << s.weeklyProgress[i] << (i + 1 < kWeeklyGoalCount ? "," : "\n");
    out << "weeklyClaimed=";
    for (int i = 0; i < kWeeklyGoalCount; i++) out << (s.weeklyClaimed[i] ? 1 : 0) << (i + 1 < kWeeklyGoalCount ? "," : "\n");

    out << "swordsmanship=" << s.swordsmanship << "\nfencing=" << s.fencing << "\nmacing=" << s.macing <<
           "\narchery=" << s.archery << "\nwrestling=" << s.wrestling << "\n";
    out << "tactics=" << s.tactics << "\nanatomy=" << s.anatomy << "\nmagicResist=" << s.magicResist <<
           "\nhealing=" << s.healing << "\n";
    out << "skillActive=";
    for (size_t i = 0; i < s.skillActive.size(); i++) out << (s.skillActive[i] ? 1 : 0) << (i + 1 < s.skillActive.size() ? "," : "\n");

    out << "bloodstainedProgress=" << s.bloodstainedProgress[0] << "," << s.bloodstainedProgress[1] << "," << s.bloodstainedProgress[2] << "\n";
    out << "bloodstainedLoop=" << s.bloodstainedLoop[0] << "," << s.bloodstainedLoop[1] << "," << s.bloodstainedLoop[2] << "\n";
    out << "bloodstainedBossDefeated=" << (s.bloodstainedBossDefeated[0] ? 1 : 0) << "," <<
           (s.bloodstainedBossDefeated[1] ? 1 : 0) << "," << (s.bloodstainedBossDefeated[2] ? 1 : 0) << "\n";

    out << "combatHotbar=";
    for (size_t i = 0; i < s.combatHotbar.size(); i++) out << s.combatHotbar[i] << (i + 1 < s.combatHotbar.size() ? "," : "\n");

    WriteEquipSlot(out, "equipped.leftHand", s.equipped.leftHand);
    WriteEquipSlot(out, "equipped.rightHand", s.equipped.rightHand);
    WriteEquipSlot(out, "equipped.helmet", s.equipped.helmet);
    WriteEquipSlot(out, "equipped.gorget", s.equipped.gorget);
    WriteEquipSlot(out, "equipped.gloves", s.equipped.gloves);
    WriteEquipSlot(out, "equipped.arms", s.equipped.arms);
    WriteEquipSlot(out, "equipped.legs", s.equipped.legs);
    WriteEquipSlot(out, "equipped.chest", s.equipped.chest);

    out << "backpack.count=" << s.backpack.size() << "\n";
    for (size_t i = 0; i < s.backpack.size(); i++) out << "backpack." << i << "=" << ItemToLine(s.backpack[i]) << "\n";
    out << "pets.count=" << s.pets.size() << "\n";
    for (size_t i = 0; i < s.pets.size(); i++) out << "pets." << i << "=" << PetToLine(s.pets[i]) << "\n";
    out << "corpses.count=" << s.corpses.size() << "\n";
    for (size_t i = 0; i < s.corpses.size(); i++) out << "corpses." << i << "=" << CorpseToLine(s.corpses[i]) << "\n";
    out << "potions.count=" << s.potions.size() << "\n";
    for (size_t i = 0; i < s.potions.size(); i++) out << "potions." << i << "=" << PotionToLine(s.potions[i]) << "\n";
    out << "bankItems.count=" << s.bankItems.size() << "\n";
    for (size_t i = 0; i < s.bankItems.size(); i++) out << "bankItems." << i << "=" << ItemToLine(s.bankItems[i]) << "\n";
    out.close();
#ifdef __EMSCRIPTEN__
    // The write above only lands in the in-memory FS — flush it to IndexedDB so it
    // actually survives a reload. Covers every SaveGame call site automatically (the
    // periodic autosave and any future one) with nothing to remember at each site.
    JS_FlushPersistence();
#endif
}

// Simulates the Auto-Gather that would have run while the game was closed, capped at
// 1 real hour and 2.5 total skill points, exactly like the JS's
// OFFLINE_GATHER_MAX_MS/OFFLINE_GATHER_MAX_SKILL/OFFLINE_GATHER_TICK_MS.
static void ApplyOfflineAutoGather(GameState& s, long long elapsedSeconds) {
    if (!s.autoGather || elapsedSeconds < 15) return; // JS: ignore trivially short gaps
    long long cappedSeconds = std::min(elapsedSeconds, 3600LL);
    long long simulatedSeconds = 0;
    float totalSkillGained = 0.0f;
    int actionsCompleted = 0;
    while (simulatedSeconds + 8 <= cappedSeconds && totalSkillGained < 2.5f) {
        std::string type = NextAutoGatherType(s);
        if (type.empty()) break;
        int gained = 3 + (std::rand() % 3);
        if (type == "wood") { totalSkillGained += GainSkillCapped(s.lumberjacking, 0.1f, 120.0f); s.wood += gained; }
        else { totalSkillGained += GainSkillCapped(s.mining, 0.1f, 120.0f); s.ore += gained; }
        simulatedSeconds += 8;
        actionsCompleted++;
    }
    if (actionsCompleted > 0) {
        long long minutesSimulated = std::max(1LL, (long long)std::round(simulatedSeconds / 60.0));
        s.logLine = "While you were away, auto-gathering ran for ~" + std::to_string(minutesSimulated) +
                     " minute" + (minutesSimulated == 1 ? "" : "s") + ": " + std::to_string(actionsCompleted) +
                     " action" + (actionsCompleted == 1 ? "" : "s") + " completed, +" +
                     std::to_string(totalSkillGained).substr(0, 4) + " skill.";
    }
    if (NextAutoGatherType(s).empty()) s.autoGather = false;
}

// Returns true if a save file was found and loaded (whether or not it parsed cleanly —
// a partially-corrupt file still applies whatever fields it could read, matching the
// JS's per-field fallback-to-default approach).
static bool LoadGame(GameState& s) {
    std::ifstream in(kSaveFilePath);
    if (!in.is_open()) return false;

    long long lastActiveEpoch = 0;
    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "characterName") s.characterName = val;
        else if (key == "houseTierIdx") s.houseTierIdx = std::clamp(std::atoi(val.c_str()), 0, (int)kHouseTiers.size() - 1);
        else if (key == "houseHue") s.houseHue = std::atoi(val.c_str());
        else if (key == "houseName") s.houseName = val;
        else if (key == "houseModuleLevel") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < 4; i++) s.houseModuleLevel[i] = std::atoi(p[i].c_str()); }
        else if (key == "gold") s.gold = std::atoi(val.c_str());
        else if (key == "wood") s.wood = std::atoi(val.c_str());
        else if (key == "ore") s.ore = std::atoi(val.c_str());
        else if (key == "leather") s.leather = std::atoi(val.c_str());
        else if (key == "bandages") s.bandages = std::atoi(val.c_str());
        else if (key == "buildingLevel") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < 4; i++) s.buildingLevel[i] = std::atoi(p[i].c_str()); }
        else if (key == "buildingSkill") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < 4; i++) s.buildingSkill[i] = (float)std::atof(p[i].c_str()); }
        else if (key == "dungeonXP") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < 5; i++) s.dungeonXP[i] = std::atoi(p[i].c_str()); }
        else if (key == "hp") s.hp = std::atoi(val.c_str());
        else if (key == "maxHp") s.maxHp = std::atoi(val.c_str());
        else if (key == "str") s.str = std::atoi(val.c_str());
        else if (key == "dex") s.dex = std::atoi(val.c_str());
        else if (key == "lumberjacking") s.lumberjacking = (float)std::atof(val.c_str());
        else if (key == "mining") s.mining = (float)std::atof(val.c_str());
        else if (key == "skinning") s.skinning = (float)std::atof(val.c_str());
        else if (key == "autoGather") s.autoGather = std::atoi(val.c_str()) != 0;
        else if (key == "magery") s.magery = std::min(100.0f, (float)std::atof(val.c_str())); // retroactively clamp existing saves grown past the new 100 cap
        else if (key == "evalInt") s.evalInt = (float)std::atof(val.c_str());
        else if (key == "meditation") s.meditation = (float)std::atof(val.c_str());
        else if (key == "intStat") s.intStat = std::atoi(val.c_str());
        else if (key == "mana") s.mana = (float)std::atof(val.c_str());
        else if (key == "reagents") s.reagents = std::atoi(val.c_str());
        else if (key == "animalTaming") s.animalTaming = (float)std::atof(val.c_str());
        else if (key == "animalLore") s.animalLore = (float)std::atof(val.c_str());
        else if (key == "veterinary") s.veterinary = (float)std::atof(val.c_str());
        else if (key == "stealing") s.stealing = (float)std::atof(val.c_str());
        else if (key == "snooping") s.snooping = (float)std::atof(val.c_str());
        else if (key == "notoriety") s.notoriety = (float)std::atof(val.c_str());
        else if (key == "fame") s.fame = (float)std::atof(val.c_str());
        else if (key == "karma") s.karma = (float)std::atof(val.c_str());
        else if (key == "titleLordEarned") s.titleLordEarned = std::atoi(val.c_str()) != 0;
        else if (key == "shaken") s.shaken = std::atoi(val.c_str());
        else if (key == "nextItemId") s.nextItemId = std::atoi(val.c_str());
        else if (key == "nextPetId") s.nextPetId = std::atoi(val.c_str());
        else if (key == "lastActiveEpoch") lastActiveEpoch = std::atoll(val.c_str());
        else if (key == "rivalLevel") s.rivalLevel = (float)std::atof(val.c_str());
        else if (key == "rivalPosX") s.rivalPos.x = (float)std::atof(val.c_str());
        else if (key == "rivalPosY") s.rivalPos.y = (float)std::atof(val.c_str());
        else if (key == "rivalHasBeatenPlayer") s.rivalHasBeatenPlayer = std::atoi(val.c_str()) != 0;
        else if (key == "poisoning") s.poisoning = (float)std::atof(val.c_str());
        else if (key == "weaponPoisonCharges") s.weaponPoisonCharges = std::atoi(val.c_str());
        else if (key == "weaponPoisonPotency") s.weaponPoisonPotency = std::atoi(val.c_str());
        else if (key == "bankGold") s.bankGold = std::atoi(val.c_str());
        else if (key == "weekStartEpoch") s.weekStartEpoch = std::atoll(val.c_str());
        else if (key == "blessingClaimedThisWeek") s.blessingClaimedThisWeek = std::atoi(val.c_str()) != 0;
        else if (key == "blessingUntilEpoch") s.blessingUntilEpoch = std::atoll(val.c_str());
        else if (key == "weeklyProgress") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < (size_t)kWeeklyGoalCount; i++) s.weeklyProgress[i] = std::atoi(p[i].c_str()); }
        else if (key == "weeklyClaimed") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < (size_t)kWeeklyGoalCount; i++) s.weeklyClaimed[i] = std::atoi(p[i].c_str()) != 0; }
        else if (key == "swordsmanship") s.swordsmanship = (float)std::atof(val.c_str());
        else if (key == "fencing") s.fencing = (float)std::atof(val.c_str());
        else if (key == "macing") s.macing = (float)std::atof(val.c_str());
        else if (key == "archery") s.archery = (float)std::atof(val.c_str());
        else if (key == "wrestling") s.wrestling = (float)std::atof(val.c_str());
        else if (key == "tactics") s.tactics = (float)std::atof(val.c_str());
        else if (key == "anatomy") s.anatomy = (float)std::atof(val.c_str());
        else if (key == "magicResist") s.magicResist = (float)std::atof(val.c_str());
        else if (key == "healing") s.healing = (float)std::atof(val.c_str());
        else if (key == "skillActive") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < s.skillActive.size(); i++) s.skillActive[i] = std::atoi(p[i].c_str()) != 0; }
        else if (key == "bloodstainedProgress") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < 3; i++) s.bloodstainedProgress[i] = std::atoi(p[i].c_str()); }
        else if (key == "bloodstainedLoop") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < 3; i++) s.bloodstainedLoop[i] = std::atoi(p[i].c_str()); }
        else if (key == "bloodstainedBossDefeated") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < 3; i++) s.bloodstainedBossDefeated[i] = std::atoi(p[i].c_str()) != 0; }
        else if (key == "combatHotbar") { auto p = SplitStr(val, ','); for (size_t i = 0; i < p.size() && i < s.combatHotbar.size(); i++) s.combatHotbar[i] = std::atoi(p[i].c_str()); }
        else if (key == "equipped.leftHand") ReadEquipSlot(val, s.equipped.leftHand);
        else if (key == "equipped.rightHand") ReadEquipSlot(val, s.equipped.rightHand);
        else if (key == "equipped.helmet") ReadEquipSlot(val, s.equipped.helmet);
        else if (key == "equipped.gorget") ReadEquipSlot(val, s.equipped.gorget);
        else if (key == "equipped.gloves") ReadEquipSlot(val, s.equipped.gloves);
        else if (key == "equipped.arms") ReadEquipSlot(val, s.equipped.arms);
        else if (key == "equipped.legs") ReadEquipSlot(val, s.equipped.legs);
        else if (key == "equipped.chest") ReadEquipSlot(val, s.equipped.chest);
        else if (key == "backpack.count") { s.backpack.clear(); s.backpack.reserve(std::atoi(val.c_str())); }
        else if (key.rfind("backpack.", 0) == 0) { if (auto it = ItemFromLine(val)) s.backpack.push_back(*it); }
        else if (key == "pets.count") { s.pets.clear(); s.pets.reserve(std::atoi(val.c_str())); }
        else if (key.rfind("pets.", 0) == 0) { if (auto p = PetFromLine(val)) s.pets.push_back(*p); }
        else if (key == "corpses.count") { s.corpses.clear(); s.corpses.reserve(std::atoi(val.c_str())); }
        else if (key.rfind("corpses.", 0) == 0) { if (auto c = CorpseFromLine(val)) s.corpses.push_back(*c); }
        else if (key == "potions.count") { s.potions.clear(); s.potions.reserve(std::atoi(val.c_str())); }
        else if (key.rfind("potions.", 0) == 0) { if (auto p = PotionFromLine(val)) s.potions.push_back(*p); }
        else if (key == "bankItems.count") { s.bankItems.clear(); s.bankItems.reserve(std::atoi(val.c_str())); }
        else if (key.rfind("bankItems.", 0) == 0) { if (auto it = ItemFromLine(val)) s.bankItems.push_back(*it); }
    }

    if (lastActiveEpoch > 0) {
        long long elapsed = (long long)std::time(nullptr) - lastActiveEpoch;
        if (elapsed > 0) {
            ApplyOfflineAutoGather(s, elapsed);
            // The Rival's own small growth trickle while you were away (2026-09-23,
            // "Rival hunts you" plan) — "it's been out there hunting/gathering/
            // training" without literally simulating any of that. Reuses this same
            // elapsed-since-last-save value rather than tracking a second epoch just
            // for the Rival (see GameState::rivalLevel's comment). Capped at 3 days'
            // worth so leaving the game untouched for a long time can't make it
            // absurd; the per-fight nudge in RivalFightEnded is the main growth driver.
            float hours = std::min((float)elapsed / 3600.0f, 72.0f);
            float target = (float)CombatPower(s);
            float growthFactor = std::min(0.5f, hours * 0.01f);
            s.rivalLevel = std::clamp(s.rivalLevel + (target - s.rivalLevel) * growthFactor, 5.0f, 500.0f);
        }
    }
    // Retroactive fixup for saves written before maxHp switched from "50 + str" to a
    // literal "maxHp == str" (Mark's call) — recompute from the loaded str rather than
    // trusting the stale saved maxHp, which would otherwise carry the old +50 forever.
    // Preserves current damage (missing HP) rather than fully healing on the fixup.
    int missingHp = std::max(0, s.maxHp - s.hp);
    s.maxHp = s.str;
    s.hp = std::max(1, s.maxHp - missingHp);
    return true;
}

// JS's "reset prototype" link: wipe the save file and start over with a fresh state.
static void ResetGame(GameState& s) {
    std::remove(kSaveFilePath);
    s = GameState{};
    s.logLine = "Game reset.";
}

// ---------------------------------------------------------------------
// Crafting & equipment — ported from craftItem()/equipItem()/sellItem()/
// applyCraftGainTaper()/gainSkill() in the JS.
// ---------------------------------------------------------------------

// Attempt to craft `recipeIdx` from `buildingIdx` (0=smith,1=carpenter,2=tailor).
// Mirrors craftItem(): checks the workshop's skill cap, the resource cost, then
// rolls a skill gain and a quality tier for the finished item.
// capOverride (>= 0) is used by home-workshop crafting (see DrawHouseScreen) to cap
// skill at the wing's level instead of the real town building's — everything else
// (skill value, resource pool, recipe list) is shared with the town building since
// it's the same underlying craft, just capped lower at home.
static void TryCraftItem(GameState& s, int buildingIdx, int recipeIdx, int capOverride = -1) {
    const BuildingDef& b = kCraftBuildings[buildingIdx];
    if (recipeIdx < 0 || recipeIdx >= (int)b.recipes.size()) return;
    const Recipe& r = b.recipes[recipeIdx];

    if ((int)s.backpack.size() >= BackpackCap(s)) {
        s.logLine = "Your backpack is full (" + std::to_string(BackpackCap(s)) + " items).";
        return;
    }
    float skillVal = s.buildingSkill[buildingIdx];
    int buildingCap = capOverride >= 0 ? capOverride : b.levels[s.buildingLevel[buildingIdx] - 1].cap;
    float effectiveSkill = std::min(skillVal, (float)buildingCap);
    if (effectiveSkill < r.reqSkill) {
        if (buildingCap < r.reqSkill)
            s.logLine = "This workshop tops out at " + std::to_string(buildingCap) +
                         " — needs " + std::to_string(r.reqSkill) + " to craft a " + r.name + ".";
        else
            s.logLine = "Skill too low for a " + r.name + " (needs " + std::to_string(r.reqSkill) + ").";
        return;
    }
    int* resourcePool = (b.resource == Resource::Wood) ? &s.wood
                        : (b.resource == Resource::Ore) ? &s.ore
                        : (b.resource == Resource::Leather) ? &s.leather : nullptr;
    if (resourcePool && *resourcePool < r.cost) {
        s.logLine = "Not enough resources to craft a " + r.name + ".";
        return;
    }
    if (resourcePool) *resourcePool -= r.cost;

    float overshoot = std::max(0.0f, skillVal - r.reqSkill);
    float baseGain = std::max(0.05f, 0.6f - overshoot * 0.03f + (RandUnit() * 0.2f - 0.1f));
    float gain = CraftGainTaper(skillVal, baseGain);
    s.buildingSkill[buildingIdx] = std::min((float)buildingCap, skillVal + gain);

    auto [qualityLabel, qualityMult] = QualityFor(effectiveSkill);
    int finalPower = std::max(1, (int)std::round(r.power * qualityMult));
    Item item{ s.nextItemId++, qualityLabel + " " + r.name, r.type, r.slot, r.handed, finalPower, r.category };
    s.backpack.push_back(item);

    std::string gainNote = gain > 0 ? " (+" + std::to_string(gain).substr(0, 4) + " skill)" : "";
    // Smith/Carpenter train STR+DEX (heavy work); Tailor trains DEX+INT (fine work) —
    // same skill-to-stat pairing as the JS's (unwired, but still the intended) SKILL_STAT_MAP.
    if (buildingIdx == 0 || buildingIdx == 1) {
        if (MaybeGainStat(s, &GameState::str, 0.06f)) gainNote += " (STR +1)";
        if (MaybeGainStat(s, &GameState::dex, 0.06f)) gainNote += " (DEX +1)";
    } else if (buildingIdx == 2) {
        if (MaybeGainStat(s, &GameState::dex, 0.06f)) gainNote += " (DEX +1)";
        if (MaybeGainStat(s, &GameState::intStat, 0.06f)) gainNote += " (INT +1)";
    }
    s.logLine = "Crafted " + item.name + " (" + std::to_string(finalPower) +
                 (r.type == ItemType::Armor ? " defense)" : " power)") + gainNote;
    AddWeeklyProgress(s, kGoalCraft, 1);
}

// JS craftPotion(): unlike weapons/armor, potions consume reagents (not a resource
// pool) and their potency is NOT quality-scaled — baseAmount is baseAmount regardless
// of skill, only the *chance to brew it at all* depends on skill.
static void TryCraftPotion(GameState& s, int recipeIdx, int capOverride = -1) {
    const BuildingDef& alchemy = kCraftBuildings[3];
    if (recipeIdx < 0 || recipeIdx >= (int)alchemy.recipes.size()) return;
    const Recipe& r = alchemy.recipes[recipeIdx];
    float skillVal = s.buildingSkill[3];
    int buildingCap = capOverride >= 0 ? capOverride : alchemy.levels[s.buildingLevel[3] - 1].cap;
    float effectiveSkill = std::min(skillVal, (float)buildingCap);
    if (effectiveSkill < r.reqSkill) {
        s.logLine = "Your Alchemy isn't high enough for a " + r.name + " yet (needs " +
                     std::to_string(r.reqSkill) + ").";
        return;
    }
    if (s.reagents < r.cost) { s.logLine = "Not enough reagents to brew a " + r.name + "."; return; }
    s.reagents -= r.cost;

    float overshoot = std::max(0.0f, skillVal - r.reqSkill);
    float baseGain = std::max(0.05f, 0.6f - overshoot * 0.03f + (RandUnit() * 0.2f - 0.1f));
    float gain = GainSkillCapped(s.buildingSkill[3], CraftGainTaper(skillVal, baseGain), (float)buildingCap);

    auto stack = std::find_if(s.potions.begin(), s.potions.end(), [&](const PotionStack& p) { return p.name == r.name; });
    if (stack == s.potions.end()) {
        s.potions.push_back({ r.name, r.category, r.power, 1 });
    } else {
        stack->count += 1;
    }
    std::string gainNote = gain > 0 ? " (Alchemy +" + std::to_string(gain).substr(0, 4) + ")" : "";
    if (MaybeGainStat(s, &GameState::intStat, 0.06f)) gainNote += " (INT +1)";
    s.logLine = "Brewed " + r.name + "." + gainNote;
    AddWeeklyProgress(s, kGoalBrew, 1);
}

// ---------------------------------------------------------------------
// Vendor "Buy" tab, added 2026-09-21 — the 4 craft buildings selling pre-made gear
// alongside letting you craft your own (Mark's ask: "the blacksmith, tailor and other
// crafting building [should] sell gear as well"). Deliberately reuses
// kCraftBuildings[i].recipes directly (confirmed no separate recipe-unlock system
// exists to respect) rather than a second vendor-stock list, and skips every part of
// TryCraftItem/TryCraftPotion that represents the player's own effort: no skill check,
// no resource cost, no skill-gain/stat-gain roll. Gold is the only cost.
// ---------------------------------------------------------------------

// Recipe has no gold-price field (see kQualityTiersData's comment) — a vendor price
// has to be derived. Tuned to sit above the gold-equivalent of gathering the
// materials yourself, so crafting stays the cheaper path; a first-pass number, not a
// locked balance decision (see the plan's "Explicitly deferred" section).
static const float kVendorGoldPerResourceUnit = 3.0f;
static const float kVendorGoldPerPower = 4.0f;
static int VendorPriceFor(const Recipe& r) {
    return std::max(5, (int)std::round(r.cost * kVendorGoldPerResourceUnit + r.power * kVendorGoldPerPower));
}
// Always the bottom ("Standard") quality tier regardless of the player's skill —
// QualityFor(0) hits kQualityTiersData's first bucket unconditionally.
static void TryBuyPremadeItem(GameState& s, int buildingIdx, int recipeIdx) {
    const BuildingDef& b = kCraftBuildings[buildingIdx];
    if (recipeIdx < 0 || recipeIdx >= (int)b.recipes.size()) return;
    const Recipe& r = b.recipes[recipeIdx];
    if ((int)s.backpack.size() >= BackpackCap(s)) {
        s.logLine = "Your backpack is full (" + std::to_string(BackpackCap(s)) + " items).";
        return;
    }
    int price = VendorPriceFor(r);
    if (s.gold < price) { s.logLine = "Not enough gold for a Standard " + r.name + " (" + std::to_string(price) + "g)."; return; }
    s.gold -= price;
    auto [qualityLabel, qualityMult] = QualityFor(0.0f);
    int finalPower = std::max(1, (int)std::round(r.power * qualityMult));
    Item item{ s.nextItemId++, qualityLabel + " " + r.name, r.type, r.slot, r.handed, finalPower, r.category };
    s.backpack.push_back(item);
    s.logLine = "Bought " + item.name + " for " + std::to_string(price) + "g.";
}
// Alchemy's Buy tab: potions have no quality scaling at all (see TryCraftPotion's own
// comment), so this is just the same potion at a flat markup over its reagent cost —
// no "Standard" tier question to resolve here.
static void TryBuyPremadePotion(GameState& s, int recipeIdx) {
    const BuildingDef& alchemy = kCraftBuildings[3];
    if (recipeIdx < 0 || recipeIdx >= (int)alchemy.recipes.size()) return;
    const Recipe& r = alchemy.recipes[recipeIdx];
    int price = VendorPriceFor(r);
    if (s.gold < price) { s.logLine = "Not enough gold for a " + r.name + " (" + std::to_string(price) + "g)."; return; }
    s.gold -= price;
    auto stack = std::find_if(s.potions.begin(), s.potions.end(), [&](const PotionStack& p) { return p.name == r.name; });
    if (stack == s.potions.end()) s.potions.push_back({ r.name, r.category, r.power, 1 });
    else stack->count += 1;
    s.logLine = "Bought " + r.name + " for " + std::to_string(price) + "g.";
}

// JS usePotion(): heal potions restore HP anytime; stamina potions are brewable but
// have nothing to restore here (no stamina stat in this scaffold — see header note).
static void DrinkPotion(GameState& s, int potionIdx) {
    if (potionIdx < 0 || potionIdx >= (int)s.potions.size()) return;
    PotionStack& p = s.potions[potionIdx];
    if (p.effect == "poison" || p.effect == "damage") {
        s.logLine = p.name + " isn't meant to be drunk — use Poison Weapon or Throw instead.";
        return;
    }
    if (p.effect == "heal") {
        s.hp = std::min(s.maxHp, s.hp + p.potency);
        s.logLine = "Drank " + p.name + " — healed " + std::to_string(p.potency) + ".";
    } else { // "stamina"
        s.logLine = "Drank " + p.name + " (no stamina system in this scaffold to restore).";
    }
    p.count -= 1;
    if (p.count <= 0) s.potions.erase(s.potions.begin() + potionIdx);
}

// JS poisonWeapon(): coats the weapon for `potency` charges (hitsRemaining), consumed
// one per successful hit (see the weapon-poison hook in ResolveCombatRound).
static void PoisonWeapon(GameState& s, int potionIdx) {
    if (potionIdx < 0 || potionIdx >= (int)s.potions.size()) return;
    PotionStack& p = s.potions[potionIdx];
    if (p.effect != "poison") return;
    s.weaponPoisonCharges = p.potency;
    s.weaponPoisonPotency = p.potency;
    float gain = GainSkillCapped(s.poisoning, RollGatherSkillGain(s.poisoning), 120.0f);
    std::string gainNote = gain > 0 ? " (Poisoning +" + std::to_string(gain).substr(0, 4) + ")" : "";
    s.logLine = "You coat your weapon in poison — " + std::to_string(p.potency) + " charges." + gainNote;
    p.count -= 1;
    if (p.count <= 0) s.potions.erase(s.potions.begin() + potionIdx);
}

// JS throwExplosionPotion(): a free action in combat — deals damage but does NOT
// hand the turn to the monster (no MonsterCounterAndMaybeEnd() call here).
static void ThrowExplosionPotion(GameState& s, int potionIdx) {
    if (!s.combat.has_value() || potionIdx < 0 || potionIdx >= (int)s.potions.size()) return;
    PotionStack& p = s.potions[potionIdx];
    if (p.effect != "damage") return;
    int dmg = std::max(1, (int)std::round(p.potency * (0.85f + RandUnit() * 0.3f)));
    s.combat->monsterHP -= dmg;
    s.combat->Log("You hurl " + p.name + " for " + std::to_string(dmg) + " damage!");
    s.logLine = "You hurl " + p.name + " for " + std::to_string(dmg) + " damage!";
    p.count -= 1;
    if (p.count <= 0) s.potions.erase(s.potions.begin() + potionIdx);
    CheckMonsterDefeatedAndHandleWin(s);
}

// Mirrors equipItem(): a 2h weapon takes both hands (bumping whatever was there back
// to the backpack); a 1h weapon takes the right hand by default; armor goes to its slot.
static void EquipFromBackpack(GameState& s, int backpackIdx) {
    if (backpackIdx < 0 || backpackIdx >= (int)s.backpack.size()) return;
    Item item = s.backpack[backpackIdx];
    s.backpack.erase(s.backpack.begin() + backpackIdx);

    if (item.type == ItemType::Weapon) {
        if (item.handed == "2h") {
            if (s.equipped.leftHand) s.backpack.push_back(*s.equipped.leftHand);
            if (s.equipped.rightHand && (!s.equipped.leftHand || s.equipped.rightHand->id != s.equipped.leftHand->id))
                s.backpack.push_back(*s.equipped.rightHand);
            s.equipped.leftHand = item;
            s.equipped.rightHand = item;
        } else {
            if (s.equipped.rightHand) s.backpack.push_back(*s.equipped.rightHand);
            s.equipped.rightHand = item;
        }
    } else { // armor
        std::optional<Item>* target = item.slot == "helmet" ? &s.equipped.helmet
            : item.slot == "gorget" ? &s.equipped.gorget : item.slot == "gloves" ? &s.equipped.gloves
            : item.slot == "arms" ? &s.equipped.arms : item.slot == "legs" ? &s.equipped.legs
            : &s.equipped.chest;
        if (target->has_value()) s.backpack.push_back(**target);
        *target = item;
    }
    s.logLine = "Equipped " + item.name + ".";
}

// JS sellValueFor(): round(power * 2) gold, item removed from the backpack.
static void SellFromBackpack(GameState& s, int backpackIdx) {
    if (backpackIdx < 0 || backpackIdx >= (int)s.backpack.size()) return;
    Item item = s.backpack[backpackIdx];
    int value = std::max(1, (int)std::round(item.power * 2.0f));
    s.backpack.erase(s.backpack.begin() + backpackIdx);
    s.gold += value;
    s.logLine = "Sold " + item.name + " for " + std::to_string(value) + " gold.";
}

// JS skinCorpse(): yield scales from 50% to 200% of the corpse's base leather as
// Skinning goes from 0 to 100 (live UO's Forensic Evaluation formula). Gold sat on
// the corpse since the kill (see the combat-win code above) and is only collected now.
static void SkinCorpse(GameState& s, int corpseIdx) {
    if (corpseIdx < 0 || corpseIdx >= (int)s.corpses.size()) return;
    Corpse c = s.corpses[corpseIdx];
    s.corpses.erase(s.corpses.begin() + corpseIdx);

    float yieldMult = 0.5f + 1.5f * (s.skinning / 100.0f);
    int leatherGained = std::max(1, (int)std::round(c.baseLeather * yieldMult));
    s.leather += leatherGained;
    s.gold += c.gold;
    float gain = GainSkillCapped(s.skinning, RollGatherSkillGain(s.skinning), 120.0f);
    std::string gainNote = gain > 0 ? " (Skinning +" + std::to_string(gain).substr(0, 4) + ")" : "";
    if (MaybeGainStat(s, &GameState::dex, 0.06f)) gainNote += " (DEX +1)";
    s.logLine = "Skinned the " + c.monsterName + " corpse for " + std::to_string(leatherGained) +
                 " leather and " + std::to_string(c.gold) + " gold." + gainNote;
}

// ---------------------------------------------------------------------
// Banking (the Vaultkeep) — ported from the JS's state.bank. Deposited
// gold and items are completely safe from every danger system above: a
// murderer loss, a failed Snoop, or a guard-zone confiscation only ever
// touches what you're carrying, never the bank.
// ---------------------------------------------------------------------

static void DepositGold(GameState& s, int amount) {
    amount = std::min(amount, s.gold);
    if (amount <= 0) return;
    s.gold -= amount;
    s.bankGold += amount;
    s.logLine = "Deposited " + std::to_string(amount) + " gold.";
}
static void WithdrawGold(GameState& s, int amount) {
    amount = std::min(amount, s.bankGold);
    if (amount <= 0) return;
    s.bankGold -= amount;
    s.gold += amount;
    s.logLine = "Withdrew " + std::to_string(amount) + " gold.";
}
static void DepositItem(GameState& s, int backpackIdx) {
    if (backpackIdx < 0 || backpackIdx >= (int)s.backpack.size()) return;
    Item item = s.backpack[backpackIdx];
    s.backpack.erase(s.backpack.begin() + backpackIdx);
    s.bankItems.push_back(item);
    s.logLine = "Deposited " + item.name + " in the vault.";
}
static void WithdrawItem(GameState& s, int bankIdx) {
    if (bankIdx < 0 || bankIdx >= (int)s.bankItems.size()) return;
    if ((int)s.backpack.size() >= BackpackCap(s)) {
        s.logLine = "Your backpack is full — make room before withdrawing.";
        return;
    }
    Item item = s.bankItems[bankIdx];
    s.bankItems.erase(s.bankItems.begin() + bankIdx);
    s.backpack.push_back(item);
    s.logLine = "Withdrew " + item.name + " from the vault.";
}

// Player housing — ported from the JS's buyHouseTier()/setHouseHue()/setHouseName()/
// buildHouseModule()/upgradeHouseModule() (see kHouseTiers/kHouseHues/kHomeModuleDefs).
static void BuyHouseTier(GameState& s, int targetIdx) {
    if (targetIdx <= s.houseTierIdx || targetIdx >= (int)kHouseTiers.size()) return;
    int cost = kHouseTiers[targetIdx].cost - kHouseTiers[s.houseTierIdx].cost;
    if (s.gold < cost) return;
    s.gold -= cost;
    s.houseTierIdx = targetIdx;
    s.logLine = "Purchased a " + kHouseTiers[targetIdx].name + " for " + std::to_string(cost) + " gold.";
}
static void SetHouseHue(GameState& s, int hueIdx) {
    if (hueIdx < 0 || hueIdx >= kHouseTiers[s.houseTierIdx].hueOptions) return;
    s.houseHue = hueIdx;
}
static int BuiltHouseModuleCount(const GameState& s) {
    int n = 0;
    for (int lvl : s.houseModuleLevel) if (lvl > 0) n++;
    return n;
}
static void BuildHouseModule(GameState& s, int moduleIdx) {
    if (moduleIdx < 0 || moduleIdx >= (int)kHomeModuleDefs.size()) return;
    if (s.houseModuleLevel[moduleIdx] > 0) return;
    if (BuiltHouseModuleCount(s) >= kHouseTiers[s.houseTierIdx].moduleSlots) return;
    int cost = kHomeModuleLevels[0].cost;
    if (s.gold < cost) return;
    s.gold -= cost;
    s.houseModuleLevel[moduleIdx] = 1;
    s.logLine = "Built a " + kHomeModuleDefs[moduleIdx].label + " onto your home for " + std::to_string(cost) + " gold.";
}
static void UpgradeHouseModule(GameState& s, int moduleIdx) {
    if (moduleIdx < 0 || moduleIdx >= (int)kHomeModuleDefs.size()) return;
    int current = s.houseModuleLevel[moduleIdx];
    if (current <= 0 || current >= (int)kHomeModuleLevels.size()) return;
    int cost = kHomeModuleLevels[current].cost; // levels are 1-based; index by current = next level's cost entry
    if (s.gold < cost) return;
    s.gold -= cost;
    s.houseModuleLevel[moduleIdx] = current + 1;
    s.logLine = "Upgraded your " + kHomeModuleDefs[moduleIdx].label + " to level " + std::to_string(current + 1) +
                 " for " + std::to_string(cost) + " gold.";
}

// ---------------------------------------------------------------------
// Rendering: the top-down Town world ("render cells" now means "render
// walkable nodes"), a resource HUD, and a detail/upgrade panel that opens
// when the player walks up to a building and presses E.
// ---------------------------------------------------------------------

static Color TileColorFor(const std::string& key) {
    if (auto* b = FindCraftBuilding(key)) return b->color;
    for (auto& t : kStaticTiles) if (t.key == key) return t.color;
    return GRAY;
}
static std::string TileNameFor(const std::string& key) {
    if (auto* b = FindCraftBuilding(key)) return b->name;
    for (auto& t : kStaticTiles) if (t.key == key) return t.name;
    return "Vacant Lot";
}

// World positions for the Town's 9 buildings — a uniform 3x3 grid (Mark asked for "a
// perfect square... connected by roads" instead of the old loosely hand-placed
// arrangement) on 300-unit spacing (was 250 — Mark asked for more breathing room
// between buildings after the 15% visual-size bump made the grid feel cramped; see
// kTownVisualScale and kTownWorldSize above), centered on Townhall. Every other
// building lines up with Townhall on exactly one axis, so DrawRoadToPlaza draws it a
// plain straight road; only the 4 corner buildings need its L-bend. That's what
// actually produces the clean tic-tac-toe grid of roads, not anything special in the
// road-drawing code itself — see DrawRoadToPlaza, unchanged.
struct TownNodePos { std::string key; Vector2 pos; };
// House sits 100 units past Stable on the middle row, the same offset the Wilderness
// Gate uses 100 units past Bank on the middle column — same "just outside the grid"
// spacing, different edge, so it doesn't crowd Stable or break the 3x3 layout.
static const std::array<TownNodePos, 10> kTownNodePositions = {{
    {"smith", {200, 200}}, {"carpenter", {500, 200}}, {"tailor", {800, 200}},
    {"alchemy", {200, 500}}, {"townhall", {500, 500}}, {"stable", {800, 500}},
    {"healer", {200, 800}}, {"bank", {500, 800}}, {"provisioner", {800, 800}},
    {"house", {900, 500}},
}};
// The town's central plaza — sized to hold only Townhall's grid slot, so every other
// building (all 300 units out on the grid) is clearly outside it and gets a road.
static const Rectangle kTownPlaza = { 420, 420, 160, 160 };

// The Wilderness Gate — a Town-edge node like a building, but instead of an
// upgrade/craft panel, walking up and pressing E switches to Screen::Wilderness.
// Placed due south on the grid's own central column (shares Bank's and Townhall's x),
// so the same straight road that already reaches Bank continues on to the gate (see the
// dedicated DrawWallBand call for that extension in DrawTownScreen) — the most direct,
// obvious path out of town rather than a walk to a far corner.
static const Vector2 kWildernessGatePos = { 500, 900 };

// ---------------------------------------------------------------------
// The Wilderness — an open outdoor space (no interior walls, unlike the dungeons; just
// scattered gather/tame nodes on open ground, closer in spirit to Town's layout than a
// dungeon's) reached via kWildernessGatePos in Town. Shares kDungeonWorldSize (1800)
// for the same "real room to walk around" reasoning as the dungeons. Gather nodes call
// the same TryStartGather() the Town HUD buttons already use (so auto-gather, skill
// gain, and the existing gather-completion ambush check all just work unmodified);
// creature spots call the same TryStartTameAttempt() the Pets screen list already uses.
// ---------------------------------------------------------------------
// The extra NE (forest, x>1500/y<600) and W (mountain, x<450) entries below are the
// wilderness-overhaul density pass — same resources/monsters as everywhere else on the
// map, just more of them concentrated into the two zones a real forest and mountain
// range would have, per the reference concept image Mark shared. An earlier survey
// wrongly flagged 5 wild creature sprites as unused "free" additions — they turned out
// to already be wired in (see GameAssets.wildCreatureTex's original 5-of-11 coverage).
// The real gap (Panther/Bison/Horse/Sabertooth/Drake/Wyvern, difficulty 40-100, defined
// in kWildCreatures but never exposed) got closed properly: Mark generated all 6 with
// Gemini ("Medieval Animal Set"), so every kWildCreatures entry now has real art and a
// spot below, not just the original 5.
struct WildernessGatherNode { Vector2 pos; std::string resource; }; // "wood" or "ore"
static const std::array<WildernessGatherNode, 12> kWildernessGatherNodes = {{
    { {500, 1400}, "wood" }, { {1300, 1400}, "wood" }, { {900, 1100}, "wood" },
    { {400, 900}, "ore" },   { {1400, 900}, "ore" },   { {900, 600}, "ore" },
    { {1700, 150}, "wood" }, { {1650, 450}, "wood" }, // Dense Forest zone (NE)
    { {250, 300}, "ore" },   { {300, 1300}, "ore" },   // Dragontooth mountain zone (W)
    // The new stretch toward Saltmere (2026-09-22, "second town" plan) — a couple of
    // waypoints so the longer walk isn't completely empty, not an exhaustive re-scatter.
    { {2200, 1550}, "wood" }, { {2550, 1900}, "ore" },
}};
// Index into kWildCreatures — a spread of difficulties so there's an easy tame near the
// entrance and a real challenge (Forest Dragon) at the far end of the map.
// Roaming Innocent NPCs (2026-09-23) — Mark asked for "AI innocent players... ones you
// can attack, rob etc" as a proactive alternative to the old passive
// TryTriggerInnocentEncounter popup (which only ever fired as a random chance after
// gathering/combat, never something you could seek out). Deliberately reuses the
// entire existing GameState::InnocentEncounter/DrawInnocentPanel mechanism as-is
// (Murder/Steal/Snoop/Spare all already exist and already move notoriety/karma/gold
// correctly) — the only real gap was that nothing in the world could ever populate
// s.innocentEncounter except that random roll. Each spot just wanders in place
// (MonsterWanderOffset, same as monsters/Town NPCs) and, once resolved one way or
// another, sits empty for a while before a fresh traveler appears — same "respawns
// after a cooldown" shape as a gather node, not a one-time encounter.
struct WildernessInnocentSpot { Vector2 pos; };
static const std::array<WildernessInnocentSpot, 4> kWildernessInnocentSpots = {{
    { {450, 1150} }, { {1450, 1150} }, { {1150, 450} }, { {2100, 1350} }, // last one along the Saltmere stretch
}};
static const float kInnocentRespawnSeconds = 45.0f;
// Called once per frame from DrawWildernessScreen — rolls a fresh traveler (same
// kInnocentNames/gold-range as the old passive TryTriggerInnocentEncounter) into any
// spot that's currently empty once its respawn timer runs out.
static void UpdateInnocentSpots(GameState& s, float dt) {
    for (auto& spot : s.innocentSpots) {
        if (spot.present) continue;
        spot.respawnTimer -= dt;
        if (spot.respawnTimer > 0.0f) continue;
        spot.present = true;
        spot.name = kInnocentNames[std::rand() % kInnocentNames.size()];
        spot.gold = 10 + (std::rand() % 41);
    }
}
struct WildernessCreatureSpot { Vector2 pos; int creatureIdx; };
static const std::array<WildernessCreatureSpot, 17> kWildernessCreatureSpots = {{
    { {700, 1550}, 0 },  // Stray Dog (difficulty 0)
    { {1100, 1550}, 1 }, // Timber Wolf (difficulty 20)
    { {300, 500}, 2 },   // Grizzly Bear (difficulty 30)
    { {1500, 300}, 7 },  // Storm Griffin (difficulty 80)
    { {900, 150}, 10 },  // Forest Dragon (difficulty 100)
    { {1650, 600}, 1 },  // Timber Wolf #2 — Dense Forest zone (NE)
    { {700, 900}, 3 },   // Dire Panther (difficulty 40)
    { {1100, 900}, 4 },  // Plains Bison (difficulty 50)
    { {1100, 400}, 5 },  // War Horse (difficulty 60)
    { {700, 500}, 6 },   // Sabertooth Cat (difficulty 70)
    { {1300, 600}, 8 },  // Young Drake (difficulty 90)
    { {500, 200}, 9 },   // Elder Wyvern (difficulty 100)
    // The Saltmere corridor (2026-09-24) — had zero creatures/monsters at all before
    // this (only 2 gather nodes + 1 innocent NPC dotted the whole stretch); Mark asked
    // for monsters/animals "all thru the wilderness", and this was the one real gap.
    // Reuses the same 11 creature types (no new art needed) at a few waypoints along
    // the walk, roughly easy-to-moderate — the corridor is meant to feel like an
    // established trade road, not a second gauntlet on top of the original zone.
    { {2000, 1900}, 0 }, // Stray Dog
    { {2300, 1600}, 4 }, // Plains Bison
    { {2500, 1800}, 5 }, // War Horse — fits the "road" setting
    { {2700, 1950}, 1 }, // Timber Wolf
    { {2400, 1450}, 6 }, // Sabertooth Cat
}};
// wildCreatureTex is parallel to kWildCreatures (not kWildernessCreatureSpots) —
// creatureIdx indexes it directly. Was a 5-of-11-covered array needing a slot-mapping
// indirection (and, before that fix, an outright out-of-bounds read for Griffin(7)/
// Dragon(10)) until every creature got real art — see GameAssets.wildCreatureTex.
// Walk here and press E to head back to Town — placed just past the entrance so it's
// the first thing you see coming in, same as walking straight back out a real gate.
static const Vector2 kWildernessReturnGatePos = { 900, 1750 };

// Fightable Wilderness monsters — walk up and press E to engage, then E again in
// melee range to swing. Live/real-time (GameState::ActiveMonster/wildEngaged) — the
// first slice of the real-time combat rework, NOT the panel-based state.combat system
// ambushes/dungeons/Bloodstained still use. Levels/leather/gold are pitched around
// Emberveil Hollow's easier tiers (see kDungeons) since the Wilderness is reachable
// well before any dungeon. Art: single frames cropped from OpenGameArt.org animation
// sheets Mark downloaded (assets/wilderness/wild_*.png) — Bat: bagzie, OGA-BY 3.0.
// Goblin/Imp: Stephen "Redshrike" Challener & William.Thompsonj, CC-BY 4.0
// ("[LPC] Goblin"/"[LPC] Imp"). Wolf: Redshrike & Thompsonj, CC-BY 4.0 ("[LPC] Wolf
// Animation" — the howl.png frame, no cropping needed). Bandit: Calciumtrice, CC-BY
// 3.0 ("Animated Rogue").
// isTacticalOpponent was a trailing field here through 2026-09-22 for the one Rival
// Adventurer entry; removed 2026-09-23 when the Rival stopped being a static spot at
// all (see GameState::rivalLevel's comment and the "Rival hunts you" plan) — it's now a
// fully separate roaming entity, so every entry left in this array is an ordinary
// always-melee monster again, no per-entry AI-variant flag needed.
struct WildernessMonsterSpot { Vector2 pos; std::string name; int level; int baseLeather; int baseGold; int iconIdx; };
static const std::array<WildernessMonsterSpot, 12> kWildernessMonsterSpots = {{
    { {1150, 1250}, "Wild Bat", 2, 1, 2, 0 },
    { {600, 1000}, "Wandering Goblin", 5, 3, 4, 1 },
    { {1150, 700}, "Lone Wolf", 9, 5, 7, 2 },
    { {600, 350}, "Lesser Imp", 14, 7, 10, 3 },
    { {1300, 150}, "Highway Bandit", 20, 10, 15, 4 },
    { {300, 1550}, "Mountain Bandit", 20, 10, 15, 4 }, // same art/stats as Highway Bandit — Dragontooth zone (W)
    // Density pass + Saltmere corridor coverage (2026-09-24, Mark asked for monsters
    // "all thru the wilderness") — two fill in gaps in the original zone, four cover
    // the corridor east toward Saltmere (which had zero monster spots at all before
    // this). All reuse the existing 5 monster art types; no new assets needed.
    { {900, 1400}, "Wandering Goblin", 5, 3, 4, 1 },   // south-central gap, original zone
    { {1500, 900}, "Lesser Imp", 14, 7, 10, 3 },        // east-central gap, original zone
    { {2000, 1650}, "Wild Bat", 2, 1, 2, 0 },           // corridor, near the Town 1 side
    { {2300, 1750}, "Highway Bandit", 20, 10, 15, 4 },  // corridor, a real "road danger"
    { {2600, 1650}, "Wandering Goblin", 5, 3, 4, 1 },   // corridor
    { {2750, 1850}, "Lone Wolf", 9, 5, 7, 2 },          // corridor, near the Saltmere side
}};
// "As if it were a WildernessMonsterSpot" stats for whichever monster is currently
// engaged. The Rival no longer has a real kWildernessMonsterSpots row (see
// GameState::rivalLevel's comment) — everything that used to read
// kWildernessMonsterSpots[am.spotIdx] unconditionally now goes through this instead, so
// the Rival's stats come from its own persistent GameState::rivalLevel. Gold/leather
// scale off rivalLevel using roughly the same ratio the static Bandit/Highway entries
// already use (~0.75x/~0.5x of level).
struct EngagedMonsterStats { std::string name; int level; int baseGold; int baseLeather; };
static EngagedMonsterStats EngagedWildMonsterStats(const GameState& s, const GameState::ActiveMonster& am) {
    if (am.isRival) {
        int lvl = std::max(1, (int)std::round(s.rivalLevel));
        return { "Rival Adventurer", lvl, std::max(1, (int)std::round(lvl * 0.75f)), std::max(1, (int)std::round(lvl * 0.5f)) };
    }
    const WildernessMonsterSpot& spot = kWildernessMonsterSpots[am.spotIdx];
    return { spot.name, spot.level, spot.baseGold, spot.baseLeather };
}
// Called right before ending a fight against the Rival (win, loss, or a successful
// flee) — nudges its persistent level partway toward the player's current
// CombatPower() (same "roughly as strong as you" idea RollMurdererLevel already uses
// for ambushers, ± the same variance) rather than snapping instantly, so it escalates
// across repeated encounters instead of spiking. Also remembers where the fight ended
// so roaming resumes from there instead of snapping back to some old fixed spot.
static const float kRivalGrowthLerpPerFight = 0.25f;
static void RivalFightEnded(GameState& s, const GameState::ActiveMonster& am) {
    s.rivalPos = am.pos;
    float target = (float)CombatPower(s) * (0.9f + RandUnit() * 0.2f);
    s.rivalLevel = std::clamp(s.rivalLevel + (target - s.rivalLevel) * kRivalGrowthLerpPerFight, 5.0f, 500.0f);
    s.rivalActivity = GameState::RivalActivity::Patrol;
    s.rivalActivityTimer = 0.0f; // pick a fresh patrol target immediately rather than waiting out a stale timer
}
// Tuning for the live engagement above. Melee range (60) sits just past where
// ResolveCircleCollision already naturally separates two kNodeRadius*0.7 (35) circles
// plus kPlayerRadius (17) — i.e. collision alone settles a chasing monster at roughly
// sword's-reach, this just confirms "in range" once it does. Chase speed (90) is well
// under kPlayerSpeed (220) so the player can always outrun/kite one; the leash (250)
// keeps a chasing monster from wandering into a neighboring node's territory, and the
// disengage range (320) lets it "lose interest" if the player breaks away.
static const float kWildMeleeRange = 60.0f;
static const float kWildMonsterChaseSpeed = 90.0f;
static const float kWildMonsterLeashRange = 250.0f;
static const float kWildDisengageRange = 320.0f;

// The Rival Adventurer's out-of-combat roaming (2026-09-23, "Rival hunts you" plan) —
// see GameState::rivalLevel's comment for why it's a fully separate entity rather than
// a kWildernessMonsterSpots row. Patrol is slower than a chasing monster (it's not
// actually chasing anything); Hunting is faster than kWildMonsterChaseSpeed so it reads
// as a real escalation once it commits to coming after the player.
static const float kRivalPatrolSpeed = 55.0f;
static const float kRivalHuntSpeed = 110.0f;
static const float kRivalArrivalRadius = 24.0f; // "close enough" to a patrol waypoint to pause there
static const float kRivalPauseDuration = 4.0f;  // how long it lingers at a waypoint (the "gathering/fighting" beat)
static const float kRivalHuntCheckMin = 30.0f, kRivalHuntCheckMax = 90.0f; // how often it re-rolls whether to start hunting
static const float kRivalHuntChance = 0.5f;      // odds it actually commits to a hunt when the timer fires, vs. picking a new patrol target instead
static const float kRivalHuntGiveUpTime = 25.0f; // seconds of hunting without reaching the player before it gives up
// Called once per frame from DrawWildernessScreen, before the nearest-interactable
// search, so s.rivalPos is current for this frame's distance checks — only while it
// isn't already the thing you're fighting (updateTacticalOpponentAI owns movement then).
static void UpdateRivalRoaming(GameState& s, float dt) {
    if (s.wildEngaged.has_value() && s.wildEngaged->isRival) return;
    if (s.rivalActivity == GameState::RivalActivity::Hunting) {
        s.rivalActivityTimer -= dt;
        Vector2 dir = { s.wildernessPlayerPos.x - s.rivalPos.x, s.wildernessPlayerPos.y - s.rivalPos.y };
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.0001f) {
            s.rivalPos.x += dir.x / len * kRivalHuntSpeed * dt;
            s.rivalPos.y += dir.y / len * kRivalHuntSpeed * dt;
        }
        // Give up and go back to patrolling if it's been hunting too long without
        // reaching interact range — tryEngageRival (DrawWildernessScreen) is what
        // actually starts the fight once close enough, this just stops the chase.
        if (s.rivalActivityTimer <= 0.0f) s.rivalActivity = GameState::RivalActivity::Patrol;
        return;
    }
    // Patrolling: walk toward rivalPatrolTarget — the timer only counts down once
    // actually AT the target (not during travel, or a long walk would eat the whole
    // pause before it even arrives), then either pick a new patrol target or commit to
    // a hunt. Doubles as "time until next decision" in both the paused and hunting
    // cases — simplest thing that reads as intentional rather than literally
    // simulating gathering/fighting.
    Vector2 toTarget = { s.rivalPatrolTarget.x - s.rivalPos.x, s.rivalPatrolTarget.y - s.rivalPos.y };
    float distToTarget = std::sqrt(toTarget.x * toTarget.x + toTarget.y * toTarget.y);
    if (distToTarget > kRivalArrivalRadius) {
        s.rivalPos.x += toTarget.x / distToTarget * kRivalPatrolSpeed * dt;
        s.rivalPos.y += toTarget.y / distToTarget * kRivalPatrolSpeed * dt;
        return;
    }
    s.rivalActivityTimer -= dt;
    if (s.rivalActivityTimer > 0.0f) return; // arrived, still pausing ("gathering/fighting") here
    if (RandUnit() < kRivalHuntChance) {
        s.rivalActivity = GameState::RivalActivity::Hunting;
        s.rivalActivityTimer = kRivalHuntGiveUpTime;
        return;
    }
    // Pick a new patrol waypoint among the gather nodes and the other real monsters'
    // spots — the same set of "places" that sell the it's-out-there-doing-things
    // fiction without actually running the gather/combat systems against them.
    int totalSpots = (int)kWildernessGatherNodes.size() + (int)kWildernessMonsterSpots.size();
    int pick = std::rand() % totalSpots;
    s.rivalPatrolTarget = pick < (int)kWildernessGatherNodes.size()
        ? kWildernessGatherNodes[pick].pos
        : kWildernessMonsterSpots[pick - (int)kWildernessGatherNodes.size()].pos;
    s.rivalActivityTimer = kRivalPauseDuration;
}
static const float kWildPlayerAttackCooldown = 0.8f;
static const float kWildMonsterAttackCooldown = 1.3f;
// Flat (not DEX-scaled) cast time for Wilderness spellcasting — magic's rhythm is
// independent of swing speed, same reasoning as PlayerSwingCooldown existing at all.
static const float kWildSpellCastCooldown = 1.2f;
// How long the hero sprite holds its Attack animation after a swing. Originally a
// DrawRing arc, then (2026-09-22) a procedural rotation of the walk-cycle sprite since
// no dedicated attack-animation art existed for the knight sprite at the time; as of
// 2026-09-23 the hero sheet (assets/hero/hero_v2.png) has real per-direction attack
// frames, so this now just gates ActorAnim::Attack in DrawPlayer instead of driving a
// fake rotation effect.
static const float kSwingEffectDuration = 0.18f;
// Same idea as kSwingEffectDuration but for spellcasting, gating ActorAnim::Cast —
// longer than the swing window since a cast already has its own flat cooldown
// (kWildSpellCastCooldown) to read against, unlike the DEX-scaled swing timer.
static const float kCastEffectDuration = 0.35f;
// DEX-based swing speed (Mark's own design, not from the JS prototype — see the
// STR/DEX/INT growth entry near MaybeGainStat/kStatCapTotal above; the JS never gave
// DEX any effect on attack timing at all). Anchored so DEX 10 (the starting default)
// matches the old flat kWildPlayerAttackCooldown exactly — nobody's swing gets slower
// than today's baseline — scaling down to 0.4s (2x speed) at DEX 100
// (kStatCapIndividual, the individual stat cap), linear in between.
static float PlayerSwingCooldown(const GameState& s) {
    float t = std::clamp((s.dex - 10) / (float)(kStatCapIndividual - 10), 0.0f, 1.0f);
    return kWildPlayerAttackCooldown - t * (kWildPlayerAttackCooldown - 0.4f);
}

// Purely decorative scatter (no collision) — same role as Town's kFoliagePositions.
// CraftPix "Rocks & Bushes" (assets/wilderness/PNG/), same license as the tame-spot art.
// 3=tree (reuses GameAssets.wildTree, the same texture wood-gather nodes use) and
// 4=rock (reuses GameAssets.wildRock, ore nodes' fallback texture) are purely
// decorative scatter for the forest/mountain density zones — no new assets, since both
// textures are already loaded for their interactive-node role.
// 5-14 are the "Medieval Animal Set" decorative props (same Gemini batch as the new
// creatures) — pure flavor scatter, same no-collision role as everything else here.
struct WildernessFoliage { Vector2 pos; int variant; }; // 0=bush1,1=bush2,2=fern1,3=tree,
                                                          // 4=rock,5=water,6=deerskull,
                                                          // 7=chest,8=bush(new),9=rocks(new),
                                                          // 10=cactus,11=fence,12=grass,
                                                          // 13=haybale,14=plant
static const std::array<WildernessFoliage, 30> kWildernessFoliage = {{
    { {750, 1300}, 0 }, { {1000, 1450}, 2 }, { {250, 1150}, 1 }, { {1550, 1050}, 0 },
    { {800, 800}, 2 },  { {1250, 950}, 1 },  { {450, 550}, 0 },  { {1000, 250}, 2 },
    // Dense Forest zone (NE)
    { {1750, 300}, 3 }, { {1600, 200}, 3 }, { {1750, 550}, 3 }, { {1500, 500}, 3 },
    // Dragontooth mountain zone (W)
    { {200, 650}, 4 },  { {350, 1050}, 4 }, { {200, 1450}, 4 }, { {350, 200}, 4 },
    // Medieval Animal Set props, scattered for general flavor
    { {1000, 1600}, 5 }, { {1450, 1300}, 6 }, { {1200, 250}, 7 }, { {1600, 1300}, 8 },
    { {600, 1400}, 9 },  { {250, 1550}, 10 }, { {1000, 1650}, 11 }, { {800, 1650}, 12 },
    { {1100, 1700}, 13 }, { {1450, 700}, 14 },
    // The new stretch toward Saltmere — a light scatter, reusing existing variants
    // (2026-09-22, "second town" plan), not an exhaustive re-decoration.
    { {2000, 1650}, 0 }, { {2350, 1850}, 1 }, { {2650, 1600}, 2 }, { {2800, 1850}, 0 },
}};
static const Texture2D* WildFoliageIcon(int variant) {
    switch (variant) {
        case 0: return g_assets.wildBush1Ok ? &g_assets.wildBush1 : nullptr;
        case 1: return g_assets.wildBush2Ok ? &g_assets.wildBush2 : nullptr;
        case 2: return g_assets.wildFern1Ok ? &g_assets.wildFern1 : nullptr;
        case 3: return g_assets.wildTreeOk ? &g_assets.wildTree : nullptr;
        case 4: return g_assets.wildRockOk ? &g_assets.wildRock : nullptr;
        default: {
            int propIdx = variant - 5; // 5..14 -> wildPropTex[0..9]
            if (propIdx >= 0 && propIdx < (int)g_assets.wildPropTex.size() && g_assets.wildPropTexOk[propIdx])
                return &g_assets.wildPropTex[propIdx];
            return nullptr;
        }
    }
}

// Physical entrances to the 4 curated dungeons — walking up and pressing E does exactly
// what clicking that dungeon's tab on the Hunt screen already does
// (s.selectedDungeon = idx; s.screen = Screen::Hunt), just from out here on the map
// instead of a tab click. The Hunt tab's own dungeon picker is untouched — this is a
// second way in, not a replacement. One per far corner/edge of the map, clear of every
// gather/tame/monster/gate node already out here.
struct WildernessDungeonEntrance { Vector2 pos; int dungeonIdx; Color color; };
static const std::array<WildernessDungeonEntrance, 5> kWildernessDungeonEntrances = {{
    { {150, 900}, 0, Color{ 180, 70, 55, 255 } },   // Emberveil Hollow
    { {1650, 900}, 1, Color{ 130, 100, 60, 255 } }, // Bloodtusk Hold
    { {1650, 1650}, 2, Color{ 65, 95, 135, 255 } }, // The Sunken Crypt
    { {150, 150}, 3, Color{ 95, 115, 75, 255 } },   // Wyrmscar Depths
    { {150, 1650}, 4, Color{ 110, 100, 90, 255 } }, // The Hollow Warrens — remaining free corner
}};

// ---------------------------------------------------------------------
// Dungeon room layouts — real rooms and corridors, one distinct shape per dungeon
// (piloted on the Sunken Crypt alone as a single hub-and-spoke shape; once that held
// up, each other dungeon got its own shape instead of reusing it everywhere). All
// axis-aligned, like every other world layout in this file; anything not inside one of
// a dungeon's rectangles is solid wall, enforced as a simple hard movement stop with
// axis-slide fallback in DrawHuntScreen. Same order as kDungeons (0=Emberveil Hollow,
// 1=Bloodtusk Hold, 2=The Sunken Crypt, 3=Wyrmscar Depths).
//
//   Emberveil Hollow: a single winding chain of rooms, no hub at all — a "crawl
//     deeper" feel matching a caustic elemental hollow rather than a fortification.
//   Bloodtusk Hold: a big central courtyard with 4 chambers opening directly onto it
//     (no corridor for those — the rectangles just share a wall) plus 2 further
//     chambers — an open raider stronghold rather than a maze.
//   The Sunken Crypt: the original hub-and-spoke pilot layout, unchanged.
//   Wyrmscar Depths: an asymmetric branching tree (not a star or a chain) with varied
//     room sizes — a more organic cave-system feel than the other three's uniform rooms.
// ---------------------------------------------------------------------
// Every rectangle below is exactly 2x the original (pre-2026-09-19) coordinates, to
// match kDungeonWorldSize going from 900 to 1800 — doubling every room/corridor
// preserves all the touching-boundary adjacency between them exactly (linear scaling
// can't introduce a gap or overlap that wasn't already there), while giving 4x the
// floor area and much longer walks between rooms to actually explore.
static const std::vector<Rectangle> kDungeonRoomLayouts[5] = {
    // 0: Emberveil Hollow — winding chain
    {
        {730,1130,340,340},   // room (monster 0, spawn)
        {230,1130,340,340},   // room (monster 1)
        {570,1240,160,120},   // corridor
        {230,570,340,340},    // room (monster 2)
        {320,910,160,220},    // corridor
        {730,570,340,340},    // room (monster 3)
        {570,670,160,140},    // corridor
        {1230,570,340,340},   // room (monster 4)
        {1070,670,160,140},   // corridor
        {1230,140,340,340},   // boss room
        {1310,480,180,90},    // corridor
    },
    // 1: Bloodtusk Hold — fortress courtyard
    {
        {600,600,600,600},    // courtyard (hub, no monster)
        {700,1200,400,300},   // South chamber (monster 0, spawn)
        {700,300,400,300},    // North chamber (monster 1)
        {1200,700,300,400},   // East chamber (monster 2)
        {300,700,300,400},    // West chamber (monster 3)
        {1200,300,400,400},   // NE chamber (monster 4)
        {1200,1100,440,440},  // SE chamber (boss)
    },
    // 2: The Sunken Crypt — hub and spoke (the original pilot layout)
    {
        {760,760,280,280},    // hub
        {720,240,360,360},    // N room (monster 0)
        {810,600,180,160},    // N corridor (hub <-> N room)
        {720,1200,360,360},   // S room (monster 1)
        {810,1040,180,160},   // S corridor (hub <-> S room)
        {1200,720,360,360},   // E room (monster 2)
        {1040,810,160,180},   // E corridor (hub <-> E room)
        {240,720,360,360},    // W room (monster 3)
        {600,810,160,180},    // W corridor (hub <-> W room)
        {1300,160,320,320},   // NE room (monster 4)
        {1080,270,220,180},   // NE corridor (N room <-> NE room)
        {1260,1260,340,340},  // SE room (boss)
        {1080,1320,180,180},  // SE corridor (S room <-> SE room)
    },
    // 3: Wyrmscar Depths — asymmetric branching cave
    {
        {740,1160,360,320},   // entrance (monster 0, spawn)
        {260,1180,360,280},   // room (monster 1)
        {620,1250,120,140},   // corridor
        {760,680,320,320},    // room (monster 2)
        {840,1000,160,160},   // corridor
        {760,240,320,320},    // room (monster 3)
        {840,560,160,120},    // corridor
        {1200,680,340,320},   // room (monster 4)
        {1080,760,120,160},   // corridor
        {1200,1120,400,360},  // boss room
        {1300,1000,160,120},  // corridor
    },
    // 4: The Hollow Warrens — a vertical mine shaft with two side tunnels, boss at the
    // very top (deepest point). Rooms are directly adjacent/overlapping by a clean 30px
    // (Bloodtusk-style, no separate corridor rects) rather than connected by corridors
    // like the other dungeons — a different construction technique for a genuinely
    // different shape. Every adjacency below was hand-verified to overlap, not just touch.
    {
        {750,1150,300,300},  // entrance (monster 0, spawn) — contains {900,1300}
        {750,880,300,300},   // shaft room 2 (monster 1) — overlaps entrance's top edge by 30
        {360,890,420,280},   // west tunnel (monster 2) — overlaps shaft room 2's west edge by 30
        {1020,890,420,280},  // east tunnel (monster 3) — overlaps shaft room 2's east edge by 30
        {750,610,300,300},   // shaft room 3 (monster 4) — overlaps shaft room 2's top edge by 30
        {680,240,440,400},   // boss room — overlaps shaft room 3's top edge by 30
    },
};
static bool DungeonIsFloor(int dungeonIdx, Vector2 p) {
    for (const Rectangle& r : kDungeonRoomLayouts[dungeonIdx])
        if (p.x >= r.x && p.x <= r.x + r.width && p.y >= r.y && p.y <= r.y + r.height) return true;
    return false;
}
// Draws `tex` tiled within `worldRect` (converted to screen space via `camera`), phase-
// aligned to the world grid (not the rect's own corner) so adjacent rects tile
// seamlessly. Deliberately doesn't scissor to its own bounds — every caller already
// has an outer scissor active, and nesting scissor calls breaks it (raylib's
// EndScissorMode() disables scissoring entirely rather than restoring the outer one) —
// so tiles may overhang a rect's edge by a fraction of a tile into neighboring wall.
static void DrawTiledRect(const Texture2D* tex, Rectangle worldRect, Vector2 camera, float worldTileSize, Color fillColor) {
    Vector2 topLeft = WorldToScreen({ worldRect.x, worldRect.y }, camera);
    Rectangle screenRect = { topLeft.x, topLeft.y, worldRect.width, worldRect.height };
    if (!tex) { DrawRectangleRec(screenRect, fillColor); return; }
    float scale = worldTileSize / (float)tex->width;
    float startX = screenRect.x - std::fmod(worldRect.x, worldTileSize);
    float startY = screenRect.y - std::fmod(worldRect.y, worldTileSize);
    for (float y = startY; y < screenRect.y + screenRect.height; y += worldTileSize)
        for (float x = startX; x < screenRect.x + screenRect.width; x += worldTileSize)
            DrawTextureEx(*tex, { x, y }, 0.0f, scale, WHITE);
}

// World positions for a dungeon's monster nodes — the center of that dungeon's own
// rooms in kDungeonRoomLayouts above (indices 0-4 = monsters 0-4, index 5 = boss).
static Vector2 DungeonMonsterNodePos(int dungeonIdx, int idx) {
    // Doubled to match kDungeonRoomLayouts (see that array's comment).
    static const Vector2 kCenters[5][6] = {
        { {900,1300}, {400,1300}, {400,740}, {900,740}, {1400,740}, {1400,310} }, // Emberveil Hollow
        { {900,1350}, {900,450}, {1350,900}, {450,900}, {1400,500}, {1420,1320} }, // Bloodtusk Hold
        { {900,420}, {900,1380}, {1380,900}, {420,900}, {1460,320}, {1430,1430} }, // The Sunken Crypt
        { {920,1320}, {440,1320}, {920,840}, {920,400}, {1370,840}, {1400,1300} }, // Wyrmscar Depths
        { {900,1300}, {900,1030}, {570,1030}, {1230,1030}, {900,760}, {900,440} }, // The Hollow Warrens
    };
    return kCenters[dungeonIdx][std::clamp(idx, 0, 5)];
}

// Monsters wander in a small loop around their home spot rather than standing frozen —
// a stateless, time-driven offset (no per-monster position to save/track) so it works
// identically for rendering, interaction range, and collision with zero extra bookkeeping.
// Each slot gets its own phase/speed/radius so they don't all move in lockstep. The
// Sunken Crypt's rooms are at least 160 units across against a <=28-unit wander radius,
// so monsters stay comfortably inside their room without needing room-aware clamping.
static Vector2 MonsterWanderOffset(int slotIdx, float worldTime) {
    float phase = (float)slotIdx * 2.4f; // arbitrary per-slot offset so motion isn't synchronized
    float speed = 0.5f + 0.15f * (slotIdx % 3);   // slight per-slot speed variety
    float radius = 22.0f + 6.0f * (slotIdx % 2);   // small wander radius — stays near its spawn
    return { std::cos(worldTime * speed + phase) * radius, std::sin(worldTime * speed * 0.8f + phase) * radius };
}
// The analytical derivative of MonsterWanderOffset (same phase/speed constants), giving
// a real facing direction for anything drawn wandering by it — used for Town NPCs
// (2026-09-23) so they visibly turn to face their own wander motion instead of always
// facing Down, without needing a stored previous-position field to compute velocity
// from. Not used for dungeon-monster/creature wander (those stay a simpler static Idle,
// see the Carl-art integration phase 4 notes) — Town is the one place players actually
// stand around watching NPCs long enough for it to be worth the extra polish.
static Vector2 WanderFacing(int slotIdx, float worldTime) {
    float phase = (float)slotIdx * 2.4f;
    float speed = 0.5f + 0.15f * (slotIdx % 3);
    Vector2 vel = { -std::sin(worldTime * speed + phase) * speed, std::cos(worldTime * speed * 0.8f + phase) * speed * 0.8f };
    float len = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    return len > 0.0001f ? Vector2{ vel.x / len, vel.y / len } : Vector2{ 0, 1 };
}
static Vector2 DungeonMonsterLivePos(int dungeonIdx, int idx, float worldTime) {
    Vector2 home = DungeonMonsterNodePos(dungeonIdx, idx);
    Vector2 off = MonsterWanderOffset(idx, worldTime);
    return { home.x + off.x, home.y + off.y };
}
// Same wander treatment for Wilderness monster spots (added 2026-09-22 for live
// combat's bump-to-engage — see kWildernessMonsterSpots) — these previously stood
// completely still at kWildernessMonsterSpots[idx].pos until engaged.
static Vector2 WildernessMonsterLivePos(int idx, float worldTime) {
    Vector2 home = kWildernessMonsterSpots[idx].pos;
    Vector2 off = MonsterWanderOffset(idx, worldTime);
    return { home.x + off.x, home.y + off.y };
}
// Same idea for the roaming Innocent NPCs (kWildernessInnocentSpots) — a small wander
// in place, not a real patrol like the Rival, since these are meant to read as ordinary
// travelers rather than a threat with somewhere to be.
static Vector2 WildernessInnocentLivePos(int idx, float worldTime) {
    Vector2 home = kWildernessInnocentSpots[idx].pos;
    Vector2 off = MonsterWanderOffset(idx, worldTime);
    return { home.x + off.x, home.y + off.y };
}
// Same idea for Town's wandering NPCs (kTownNPCs) — Town's first-ever ambient motion.
static Vector2 TownNPCLivePos(int idx, float worldTime, int townIdx = 0) {
    Vector2 home = (townIdx == 0 ? kTownNPCs : kTown2NPCs)[idx].homePos;
    Vector2 off = MonsterWanderOffset(idx, worldTime);
    return { home.x + off.x, home.y + off.y };
}

// World positions for the Bloodstained Road's three ladders — one node per path (each
// node's target updates live as you climb, rather than one node per tier).
static Vector2 BloodstainedPathNodePos(int pathIdx) {
    static const Vector2 kPos[3] = { {220, 300}, {450, 550}, {680, 300} }; // blue, gray, red
    return kPos[std::clamp(pathIdx, 0, 2)];
}

static void DrawWorldNode(Vector2 screenPos, float radius, Color color, const std::string& label,
                            bool nearPlayer, const std::string& sublabel = "",
                            const Texture2D* icon = nullptr, Color iconTint = WHITE,
                            const Rectangle* iconSrcRect = nullptr) {
    bool onScreen = screenPos.x > kViewport.x - radius && screenPos.x < kViewport.x + kViewport.width + radius &&
                     screenPos.y > kViewport.y - radius && screenPos.y < kViewport.y + kViewport.height + radius;
    if (!onScreen) return;
    if (icon) {
        // Real art loaded (every dungeon monster) — show it directly with no colored
        // circle backdrop (a flat color disc behind the art was both visual clutter and
        // the actual source of the original red-on-red problem on Emberveil Hollow's
        // fire-red floor, since that disc used the monster's own color). A thin gold
        // ring still signals "in range", the same role it plays on buildings.
        if (nearPlayer) DrawCircleLines((int)screenPos.x, (int)screenPos.y, radius + 4, Fade(kColorSlate, 0.9f));
        // The neutral backing plate that used to sit here was removed 2026-09-23 at
        // Mark's request ("remove the circle behind all of the images") — it existed to
        // guarantee icon contrast against busy/similarly-colored floors (originally
        // fixed Emberveil Hollow's red fire-elemental icons blending into its red/near-
        // black lava floor). Now that most icons here are real art with their own
        // outlines/shading rather than flat single-color glyphs, that risk is lower, but
        // if a specific icon becomes hard to read against a specific floor again, this
        // is the spot to revisit rather than a new bug to chase elsewhere.
        // `iconSrcRect` (2026-09-23, animated monsters/creatures) crops one frame out of
        // a DirSpriteSheet via ActorSrcRect instead of showing the whole texture — same
        // ring/plate/label chrome either way.
        if (iconSrcRect) DrawIconCenteredRect(*icon, *iconSrcRect, screenPos, radius * 1.5f, iconTint);
        else DrawIconCentered(*icon, screenPos, radius * 1.5f, iconTint);
        int tw = MeasureUIText(label.c_str(), 13);
        DrawUIText(label.c_str(), (int)screenPos.x - tw / 2, (int)(screenPos.y + radius + 4), 13, kColorText);
        if (!sublabel.empty()) {
            int stw = MeasureUIText(sublabel.c_str(), 11);
            DrawUIText(sublabel.c_str(), (int)screenPos.x - stw / 2, (int)(screenPos.y + radius + 16), 11, Fade(kColorText, 0.85f));
        }
    } else {
        // No art loaded for this one (e.g. the Bloodstained Road's path markers, which
        // never pass an icon) — the colored circle is the only visual, so it still needs
        // the always-visible outline for contrast against similarly-colored ground.
        DrawCircleV(screenPos, radius, Fade(color, 0.85f));
        DrawCircleV(screenPos, radius, Fade(BLACK, nearPlayer ? 0.0f : 0.15f)); // subtle dim when out of range
        if (nearPlayer) {
            DrawCircleV(screenPos, radius + 3, Fade(kColorSlate, 0.9f));
            DrawCircleV(screenPos, radius, Fade(color, 0.9f));
        }
        DrawCircleLines((int)screenPos.x, (int)screenPos.y, radius, Fade(RAYWHITE, 0.9f));
        DrawCircleLines((int)screenPos.x, (int)screenPos.y, radius - 1, Fade(BLACK, 0.5f));
        int tw = MeasureUIText(label.c_str(), 13);
        DrawUIText(label.c_str(), (int)screenPos.x - tw / 2, (int)screenPos.y - 6, 13, RAYWHITE);
        if (!sublabel.empty()) {
            int stw = MeasureUIText(sublabel.c_str(), 11);
            DrawUIText(sublabel.c_str(), (int)screenPos.x - stw / 2, (int)screenPos.y + 7, 11, Fade(RAYWHITE, 0.85f));
        }
    }
}

// Simple clickable button helper.
static bool Button(Rectangle r, const std::string& label, bool enabled) {
    // A small (1.5px/side) outset on both the visual rect and the click/tap hit-test —
    // enough to feel a bit more generous without crowding neighboring buttons the way
    // a bigger outset plus a drop shadow did (both have been tried and back out).
    const float kOutset = 1.5f;
    Rectangle big = { r.x - kOutset, r.y - kOutset, r.width + kOutset * 2.0f, r.height + kOutset * 2.0f };
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, big);
    Color bg = !enabled ? Fade(GRAY, 0.4f) : (hover ? Fade(kColorSlate, 0.9f) : Fade(kColorSlate, 0.7f));
    DrawRectangleRounded(big, 0.25f, 6, bg);
    DrawRectangleRoundedLines(big, 0.25f, 6, Fade(BLACK, 0.4f));
    int tw = MeasureUIText(label.c_str(), 14);
    DrawUIText(label.c_str(), (int)(big.x + (big.width - tw) / 2.0f), (int)(big.y + (big.height - 14) / 2.0f),
              14, kColorText);
    return enabled && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// Tap-to-interact button — the touch equivalent of the [E] key, shown only while in
// range of something interactable (mirrors the virtual joystick's bottom-left corner
// on the opposite side of the screen). Called after the world/scissor for the same
// reason as DrawVirtualJoystick — so it isn't painted over and isn't affected by the
// scissor region.
static bool DrawInteractButton(const std::string& label) {
    Rectangle r = { kViewport.x + kViewport.width - 150.0f, kViewport.y + kViewport.height - 90.0f, 130.0f, 60.0f };
    return Button(r, label, true);
}

// Touch-drag scrolling (2026-09-22) — every scrollable list in the game (Craft/
// Provisioner recipe & backpack lists, Magic's spell list, Pets, Bank, House, Skills,
// the Character screen's backpack) only ever responded to a desktop mouse wheel; mobile
// has no wheel at all, so on a touch device none of them could be scrolled past the
// first screenful. raylib's Emscripten GLFW3 layer already maps a single touch to the
// left mouse button (the same convention the virtual joystick and tap-to-interact
// button already rely on elsewhere in this file), so drag-to-scroll uses the same
// IsMouseButtonDown/GetMousePosition APIs — no separate touch API needed. One shared
// pair of drag-tracking statics is enough since only one list can be actively dragged
// at a time (one input device, one screen visible at once) regardless of which list.
// Drop-in replacement for the old `GetMouseWheelMove() * 24.0f` expression at every
// call site: `scroll -= ScrollDelta(area);` replaces the old
// `if (CheckCollisionPointRec(...)) scroll -= GetMouseWheelMove() * 24.0f;` entirely
// (the area-hover check now lives inside this function), desktop wheel scrolling is
// completely unchanged.
static bool g_scrollDragging = false;
static float g_scrollDragLastY = 0.0f;
static float ScrollDelta(Rectangle area) {
    Vector2 mouse = GetMousePosition();
    bool overArea = CheckCollisionPointRec(mouse, area);
    float delta = overArea ? GetMouseWheelMove() * 24.0f : 0.0f;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && overArea) {
        g_scrollDragging = true;
        g_scrollDragLastY = mouse.y;
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && g_scrollDragging) {
        // Finger moves up (mouse.y decreases) -> reveal content below -> scroll
        // increases; matches the `scroll -= ScrollDelta(...)` sign convention every
        // call site already uses for the wheel term.
        delta += (mouse.y - g_scrollDragLastY);
        g_scrollDragLastY = mouse.y;
    } else {
        g_scrollDragging = false;
    }
    return delta;
}

// ---------------------------------------------------------------------
// Live-combat spell hotbar (2026-09-22) — shared by DrawWildernessScreen and
// DrawHuntScreen (dungeons), the two live-combat screens. Assignment only touches
// s.combatHotbar/s.hotbarPickerSlot, no screen-specific monster state, so it's a real
// shared abstraction rather than the deliberate per-screen duplication used for the
// combat math itself (see ActiveMonster vs ActiveDungeonMonster's own comments).
// ---------------------------------------------------------------------

// Draws the 5 hotbar slots at the fixed spot the old "first 3 known offensive spells"
// strip used to occupy (between the joystick and the interact button) and returns
// which slot was just tapped, or -1. The caller decides what a tap means: outside
// combat it opens the assignment picker, in combat it casts — see the call sites in
// DrawWildernessScreen/DrawHuntScreen.
static int DrawCombatHotbarRow(const GameState& s, bool inCombat, float spellCooldownRemaining,
                                  float x = 175.0f, float y = kViewport.y + kViewport.height - 90.0f) {
    int tapped = -1;
    for (int i = 0; i < (int)s.combatHotbar.size(); i++) {
        Rectangle r = { x + (float)i * 68.0f, y, 62.0f, 60.0f };
        int spellIdx = s.combatHotbar[i];
        std::string label = "+";
        bool enabled = true;
        if (spellIdx >= 0 && spellIdx < (int)kSpells.size()) {
            const Spell& sp = kSpells[spellIdx];
            label = sp.name;
            if (inCombat) {
                bool affordable = s.mana >= sp.manaCost && s.reagents >= kLiveCombatReagentCost;
                enabled = affordable && spellCooldownRemaining <= 0;
            }
        } else if (inCombat) {
            enabled = false; // empty slot, nothing to cast
        }
        if (Button(r, label, enabled)) tapped = i;
    }
    return tapped;
}

// Full-width overlay listing every known spell (offense + heal/cure) to assign to the
// open slot, plus a Clear/Cancel option — opened by tapping any hotbar slot while not
// engaged in a fight (see the DrawCombatHotbarRow call sites).
static void DrawHotbarPicker(GameState& s, int screenW, int screenH) {
    if (!s.hotbarPickerSlot.has_value()) return;
    int slot = *s.hotbarPickerSlot;
    Rectangle overlay = { 20, 140, (float)screenW - 40, (float)screenH - 260 };
    DrawRectangleRounded(overlay, 0.05f, 6, Fade(kColorPageBg, 0.97f));
    DrawRectangleRoundedLines(overlay, 0.05f, 6, Fade(BLACK, 0.5f));
    DrawUIText("Assign a spell to this slot:", (int)overlay.x + 12, (int)overlay.y + 10, 14, kColorHeading);
    float y = overlay.y + 36;
    if (Button({ overlay.x + 12, y, overlay.width - 24, 26 }, "Clear slot", true)) {
        s.combatHotbar[slot] = -1;
        s.hotbarPickerSlot.reset();
        return;
    }
    y += 34;
    std::vector<int> known;
    for (size_t i = 0; i < kSpells.size(); i++) {
        const Spell& sp = kSpells[i];
        if ((sp.type == SpellType::Offensive || sp.type == SpellType::Utility) && EffectiveSkill(s, &GameState::magery) >= sp.minSkill)
            known.push_back((int)i);
    }
    float listBottom = overlay.y + overlay.height - 40;
    if (known.empty()) DrawUIText("No spells known yet — practice on the Magic tab.", (int)overlay.x + 12, (int)y, 13, DARKGRAY);
    for (int idx : known) {
        if (y + 26 > listBottom) break; // no scrolling for now — a long known-spell list just truncates
        const Spell& sp = kSpells[idx];
        std::string tag = sp.type == SpellType::Offensive ? "[Attack] " : "[Heal] ";
        if (Button({ overlay.x + 12, y, overlay.width - 24, 24 }, tag + sp.name, true)) {
            s.combatHotbar[slot] = idx;
            s.hotbarPickerSlot.reset();
            return;
        }
        y += 28;
    }
    if (Button({ overlay.x + 12, overlay.y + overlay.height - 34, overlay.width - 24, 26 }, "Cancel", true))
        s.hotbarPickerSlot.reset();
}

// Persistent HP/Mana readout for live combat (2026-09-22) — Wilderness has no player
// stat display at all otherwise; the Hunt screen already shows HP unconditionally (see
// its own header) so it only needs a Mana bar added at its call site, not this whole
// pair. No Stamina bar — no such stat exists in this game (DrinkPotion's stamina-potion
// branch literally logs "no stamina system in this scaffold"), confirmed and explicitly
// deferred rather than guessed at.
static void DrawLiveCombatHud(const GameState& s, float x, float y) {
    DrawUIText(TextFormat("HP: %d / %d", s.hp, s.maxHp), (int)x, (int)y, 13, kColorText);
    Rectangle hpBg = { x, y + 18, 140, 9 };
    DrawRectangleRec(hpBg, Fade(BLACK, 0.25f));
    float hpPct = std::clamp((float)s.hp / s.maxHp, 0.0f, 1.0f);
    DrawRectangleRec({ hpBg.x, hpBg.y, hpBg.width * hpPct, hpBg.height },
                       hpPct > 0.3f ? Color{ 63, 94, 63, 255 } : Color{ 122, 46, 46, 255 });
    DrawUIText(TextFormat("Mana: %.0f / %.0f", s.mana, MaxMana(s)), (int)x + 150, (int)y, 13, kColorText);
    Rectangle manaBg = { x + 150, y + 18, 120, 9 };
    DrawRectangleRec(manaBg, Fade(BLACK, 0.25f));
    float manaPct = std::clamp(s.mana / MaxMana(s), 0.0f, 1.0f);
    DrawRectangleRec({ manaBg.x, manaBg.y, manaBg.width * manaPct, manaBg.height }, Color{ 63, 82, 122, 255 });
}

// Bandage + heal-potion quick-use row for live combat (2026-09-22) — shared by both
// screens, sits just above the spell hotbar row. Reuses UseBandageOutOfCombat and
// DrinkPotion directly with zero changes: both already operate purely on GameState (hp/
// bandages/potions), with no CombatState coupling at all, so no new "live" variant was
// actually needed here (unlike the offensive-cast/melee functions, which do need a
// monster to hit). UseBandageOutOfCombat's own `!s.combat.has_value()` guard is
// harmless here since a live engagement and the old panel's s.combat are never both
// active at once.
static void DrawLiveCombatQuickItems(GameState& s) {
    float y = kViewport.y + kViewport.height - 150.0f;
    if (Button({ 175.0f, y, 90.0f, 40.0f }, TextFormat("Bandage (%d)", s.bandages),
                 s.bandages > 0 && s.hp < s.maxHp)) {
        UseBandageOutOfCombat(s);
    }
    int shown = 0;
    for (size_t i = 0; i < s.potions.size() && shown < 2; i++) {
        if (s.potions[i].effect != "heal") continue;
        const PotionStack& p = s.potions[i];
        Rectangle r = { 271.0f + (float)shown * 96.0f, y, 90.0f, 40.0f };
        if (Button(r, TextFormat("%s x%d", p.name.c_str(), p.count), true)) DrinkPotion(s, (int)i);
        shown++;
    }
}

// ---------------------------------------------------------------------
// Town screen: a walkable town square. Gather/Auto-Gather stay as a small
// HUD strip (they're timed actions, not places to walk to), but every
// building is now a node in the world — walk up and press E to open its
// upgrade panel, which pauses movement until closed.
// ---------------------------------------------------------------------

static void DrawTownScreen(GameState& s, int screenW, int screenH) {
    bool canGather = !s.gatheringResource.has_value();
    // Second town (2026-09-22) — reuses Town 1's exact building positions/plaza/roads/
    // collision/foliage/props unchanged (see GameState::selectedTown's comment); only
    // the NPC flavor, ground texture, and building tint differ per town, selected here.
    const auto& activeNPCs = (s.selectedTown == 0) ? kTownNPCs : kTown2NPCs;

    // Find the nearest building within interact range (used for both the prompt and
    // the actual E-press action) — movement is paused while a panel is open. The
    // Wilderness Gate competes in the same nearest-node search as every building, but
    // pressing E on it switches screens instead of opening a detail panel.
    std::string nearestKey;
    float nearestDist = 1e9f;
    for (auto& node : kTownNodePositions) {
        float d = Dist(s.townPlayerPos, node.pos);
        if (d < nearestDist) { nearestDist = d; nearestKey = node.key; }
    }
    float gateDist = Dist(s.townPlayerPos, kWildernessGatePos);
    bool gateIsNearest = gateDist < nearestDist;
    if (gateIsNearest) nearestDist = gateDist;
    // Wandering townsfolk compete in the same nearest-interactable search, same pattern
    // as the Wilderness Gate above — see kTownNPCs.
    int nearestNPCIdx = -1;
    for (int i = 0; i < (int)activeNPCs.size(); i++) {
        float d = Dist(s.townPlayerPos, TownNPCLivePos(i, s.worldTime, s.selectedTown));
        if (d < nearestDist) { nearestDist = d; nearestNPCIdx = i; }
    }
    bool npcIsNearest = nearestNPCIdx >= 0;
    bool inRange = nearestDist < kNodeRadius + kInteractRange;
    std::string interactLabel = npcIsNearest ? "Greet " + activeNPCs[nearestNPCIdx].name
                                  : gateIsNearest ? "Wilderness" : TileNameFor(nearestKey);

    if (!s.selectedTile.has_value()) {
        UpdatePlayerMovement(s.townPlayerPos, s.playerFacing, GetFrameTime(), kTownWorldSize);
        for (auto& node : kTownNodePositions)
            ResolveCircleCollision(s.townPlayerPos, kPlayerRadius, node.pos, kNodeRadius);
        ResolveCircleCollision(s.townPlayerPos, kPlayerRadius, kWildernessGatePos, kNodeRadius);
        // Townsfolk have no collision — they're ambient dressing, not obstacles; walking
        // through one is fine (see kTownNPCs' own comment).
        s.townPlayerPos = ClampToWorld(s.townPlayerPos, kPlayerEdgeMargin, kTownWorldSize);
        if (inRange && IsKeyPressed(KEY_E)) {
            if (npcIsNearest) s.greetedNPC = (s.greetedNPC.has_value() && *s.greetedNPC == nearestNPCIdx)
                                               ? std::nullopt : std::make_optional(nearestNPCIdx);
            else if (gateIsNearest) {
                s.screen = Screen::Wilderness;
                s.wildernessPlayerPos = (s.selectedTown == 0) ? Vector2{ 900, 1650 } : Vector2{ 2900, 1650 };
            }
            else s.selectedTile = nearestKey;
        }
    }

    // --- World render, clipped to the viewport so nothing bleeds into the HUD ---
    BeginScissorMode((int)kViewport.x, (int)kViewport.y, (int)kViewport.width, (int)kViewport.height);
    Vector2 camera = CameraTopLeft(s.townPlayerPos, kTownWorldSize);
    // Town 2 uses the dirt/road texture as its primary ground (already loaded, no new
    // asset needed) instead of grass, for an immediately different first impression —
    // a "packed earth trading post" feel vs. Town 1's tended grass.
    const Texture2D* activeGroundTex = (s.selectedTown == 0)
        ? (g_assets.groundGrassOk ? &g_assets.groundGrass : nullptr)
        : (g_assets.groundDirtOk ? &g_assets.groundDirt : nullptr);
    Color activeGroundFallback = (s.selectedTown == 0) ? Color{ 210, 198, 168, 255 } : Color{ 176, 158, 132, 255 };
    DrawTiledGround(activeGroundTex, kViewport, camera, 48.0f * kTownVisualScale, activeGroundFallback);

    // Farmland patches — CraftPix "village" ground dressing (assets/village/farmland.png),
    // hand-placed clear of every building/road/plaza, same spirit as kFoliagePositions
    // below (just a rectangle of tiled ground instead of a scattered icon). Re-placed
    // for the 3x3 grid layout, tucked along the open west edge near Alchemy/Healer
    // (both now in the grid's west column) same as before — offsets kept the same
    // distance from Alchemy/Healer's own (now 300-unit-spaced) positions.
    static const std::array<Rectangle, 2> kFarmlandPatches = {{
        {20, 356, 100, 100},  // near the Alchemy garden
        {20, 632, 100, 100},  // near the Healer's kitchen garden
    }};
    // Skipped for Town 2 — farmland doesn't fit its coastal trade-port identity; no
    // equivalent dressing added this pass (see the plan's deferred list).
    if (s.selectedTown == 0 && g_assets.farmlandOk)
        for (const Rectangle& patch : kFarmlandPatches)
            DrawTiledRect(&g_assets.farmland, patch, camera, 48.0f * kTownVisualScale, Color{ 200, 150, 90, 255 });

    // Fence posts marking the town plaza — purely decorative (no collision), evenly
    // spaced along its 4 edges from the same "village" pack as the farmland above.
    if (g_assets.fencePostOk) {
        std::vector<Vector2> posts;
        for (float x = kTownPlaza.x; x <= kTownPlaza.x + kTownPlaza.width; x += 50.0f) {
            posts.push_back({ x, kTownPlaza.y });
            posts.push_back({ x, kTownPlaza.y + kTownPlaza.height });
        }
        for (float py = kTownPlaza.y; py <= kTownPlaza.y + kTownPlaza.height; py += 50.0f) {
            posts.push_back({ kTownPlaza.x, py });
            posts.push_back({ kTownPlaza.x + kTownPlaza.width, py });
        }
        for (auto& p : posts) {
            Vector2 screenPos = WorldToScreen(p, camera);
            if (screenPos.x < kViewport.x - 20 || screenPos.x > kViewport.x + kViewport.width + 20 ||
                screenPos.y < kViewport.y - 20 || screenPos.y > kViewport.y + kViewport.height + 20) continue;
            DrawIconCentered(g_assets.fencePost, screenPos, 26.0f * kTownVisualScale, WHITE);
        }
    }

    // Decorative foliage first, so buildings/roads/plaza always render on top of it —
    // purely visual, no collision, hand-placed clear of every building/road/plaza. Six
    // variants (see kFoliagePositions) reusing the Wilderness screen's tree/bush/fern
    // textures for variety instead of one repeated bush icon, plus one autumn-colored
    // bush from a different pack for a splash of warm color.
    for (const TownFoliage& f : kFoliagePositions) {
        const Texture2D* icon = f.variant == 0 ? (g_assets.foliageOk ? &g_assets.foliage : nullptr)
                                 : f.variant == 1 ? (g_assets.wildTreeOk ? &g_assets.wildTree : nullptr)
                                 : f.variant == 2 ? (g_assets.wildBush1Ok ? &g_assets.wildBush1 : nullptr)
                                 : f.variant == 3 ? (g_assets.wildBush2Ok ? &g_assets.wildBush2 : nullptr)
                                 : f.variant == 4 ? (g_assets.wildFern1Ok ? &g_assets.wildFern1 : nullptr)
                                 : (g_assets.townAutumnBushOk ? &g_assets.townAutumnBush : nullptr);
        if (!icon) continue;
        Vector2 screenPos = WorldToScreen(f.pos, camera);
        if (screenPos.x < kViewport.x - 30 || screenPos.x > kViewport.x + kViewport.width + 30 ||
            screenPos.y < kViewport.y - 30 || screenPos.y > kViewport.y + kViewport.height + 30) continue;
        DrawIconCentered(*icon, screenPos, 34.0f * kTownVisualScale, WHITE);
    }

    // A dirt plaza around the Town Hall / Provisioner cluster — the outer 6 buildings
    // sit on open grass, connected back to it by roads, rather than a uniform flat
    // ground. Town 2 tints its roads/plaza a cooler gray-blue (worn dock stone) instead
    // of Town 1's warm tan, on top of the same already-loaded dirt texture.
    const Texture2D* dirtTex = g_assets.groundDirtOk ? &g_assets.groundDirt : nullptr;
    Color roadTint = (s.selectedTown == 0) ? Color{ 196, 164, 100, 255 } : Color{ 140, 148, 156, 255 };
    DrawWallBand(kTownPlaza, camera, dirtTex, 48.0f * kTownVisualScale, roadTint);
    for (auto& node : kTownNodePositions) DrawRoadToPlaza(node.pos, kTownPlaza, camera, dirtTex);
    // Extends Bank's own straight road (same central column, x=450) on past it to the
    // Wilderness Gate — one continuous main street from the plaza straight out of town,
    // instead of the gate sitting unconnected off in a corner.
    DrawWallBand({ kWildernessGatePos.x - 14, kTownPlaza.y + kTownPlaza.height, 28,
                    kWildernessGatePos.y - (kTownPlaza.y + kTownPlaza.height) },
                  camera, dirtTex, 48.0f * kTownVisualScale, roadTint);

    // Town-flavor props (well/lamps/signage/stalls/clutter) — see kTownProps.
    for (const TownProp& p : kTownProps) {
        const Texture2D* icon = nullptr;
        switch (p.kind) {
            case 0: icon = g_assets.townFountainOk ? &g_assets.townFountain : nullptr; break;
            case 1: icon = g_assets.townLampOk ? &g_assets.townLamp : nullptr; break;
            case 2: icon = g_assets.townSignSmithOk ? &g_assets.townSignSmith : nullptr; break;
            case 3: icon = g_assets.townStall1Ok ? &g_assets.townStall1 : nullptr; break;
            case 4: icon = g_assets.townStall2Ok ? &g_assets.townStall2 : nullptr; break;
            case 5: icon = g_assets.townStall3Ok ? &g_assets.townStall3 : nullptr; break;
            case 6: icon = g_assets.townLumberpileOk ? &g_assets.townLumberpile : nullptr; break;
            case 7: icon = g_assets.townBarrelOk ? &g_assets.townBarrel : nullptr; break;
            case 8: icon = g_assets.townCrateOk ? &g_assets.townCrate : nullptr; break;
            case 9: icon = g_assets.townAnvilOk ? &g_assets.townAnvil : nullptr; break;
            case 10: icon = g_assets.townStatueOk ? &g_assets.townStatue : nullptr; break;
            case 11: icon = g_assets.townSheepOk ? &g_assets.townSheep : nullptr; break;
            case 12: icon = g_assets.townCowOk ? &g_assets.townCow : nullptr; break;
            case 13: icon = g_assets.townChickenOk ? &g_assets.townChicken : nullptr; break;
            case 14: icon = g_assets.townPotionPurpleOk ? &g_assets.townPotionPurple : nullptr; break;
            case 15: icon = g_assets.townPotionRedOk ? &g_assets.townPotionRed : nullptr; break;
            case 16: icon = g_assets.townChestOk ? &g_assets.townChest : nullptr; break;
            case 17: icon = g_assets.townBookshelfOk ? &g_assets.townBookshelf : nullptr; break;
        }
        if (!icon) continue;
        Vector2 screenPos = WorldToScreen(p.pos, camera);
        if (screenPos.x < kViewport.x - 30 || screenPos.x > kViewport.x + kViewport.width + 30 ||
            screenPos.y < kViewport.y - 30 || screenPos.y > kViewport.y + kViewport.height + 30) continue;
        // Soft warm ground-glow under each streetlamp (Gemini's "sell the illusion they're
        // actively lighting the paths" suggestion) — layered fading circles (not
        // DrawCircleGradient: its signature differs between the desktop raylib 6.0 build
        // and the web build's raylib 5.5 source, int-x/y vs Vector2 — this avoids the
        // mismatch entirely) drawn before the lamp sprite so the sprite sits on top of
        // its own light pool rather than the glow overlapping the post.
        if (p.kind == 1) {
            float glowR = p.size * kTownVisualScale * 1.1f;
            Color glow = { 255, 214, 130, 255 };
            DrawCircleV(screenPos, glowR, Fade(glow, 0.10f));
            DrawCircleV(screenPos, glowR * 0.66f, Fade(glow, 0.16f));
            DrawCircleV(screenPos, glowR * 0.33f, Fade(glow, 0.22f));
        }
        DrawIconCentered(*icon, screenPos, p.size * kTownVisualScale, WHITE);
    }

    for (auto& node : kTownNodePositions) {
        Vector2 screenPos = WorldToScreen(node.pos, camera);
        bool near = (node.key == nearestKey) && inRange;
        std::string sub;
        // Saltmere got its own real building art 2026-09-23 (Mark's "Carl" art drop,
        // assets/saltmere_buildings/) — before that, Town 2 had no dedicated art at all
        // and fell back to the generic wall+roof+door composite tinted slate-blue. Real
        // art always renders at its own true colors (bodyTint WHITE); the slate tint is
        // now only a defensive fallback for the rare case a given key's file is missing.
        const Texture2D* realTex = (s.selectedTown == 0) ? FindTownBuildingTexture(node.key)
                                                            : FindSaltmereBuildingTexture(node.key);
        Color bodyTint = realTex ? WHITE : Color{ 150, 170, 185, 255 };
        if (int idx = FindCraftBuildingIndex(node.key); idx >= 0)
            sub = "Lv " + std::to_string(s.buildingLevel[idx]);
        else if (node.key == "house") {
            sub = kHouseTiers[s.houseTierIdx].name;
            if (s.selectedTown == 0 && s.houseHue >= 0 && s.houseHue < (int)kHouseHues.size())
                bodyTint = kHouseHues[s.houseHue].color;
        }
        DrawBuildingNode(screenPos, TileColorFor(node.key), TileNameFor(node.key), near, sub,
                           FindBuildingTexture(node.key), DoorAnimForBuilding(node.key),
                           realTex, bodyTint, kTownVisualScale);
    }
    {
        // The Wilderness Gate — drawn with the generic wall+roof fallback (no CraftPix
        // building art fits "gate to the wilds"), colored sage to read as an outdoor
        // threshold rather than a shop.
        Vector2 gateScreenPos = WorldToScreen(kWildernessGatePos, camera);
        DrawBuildingNode(gateScreenPos, kColorPanelBg, "Wilderness Gate", gateIsNearest && inRange, "",
                           nullptr, nullptr, nullptr, WHITE, kTownVisualScale);
    }
    // Wandering townsfolk (2026-09-23: real animated art, Mark's "Carl" drop — used to
    // be a plain colored circle, see kTownNPCs' comment history) — faces its own wander
    // motion via WanderFacing rather than always Down, since Town is the one screen
    // players linger on long enough for that polish to actually read.
    const auto& activeNPCSheets = (s.selectedTown == 0) ? g_assets.townNPCSheets : g_assets.saltmereNPCSheets;
    for (int i = 0; i < (int)activeNPCs.size(); i++) {
        Vector2 screenPos = WorldToScreen(TownNPCLivePos(i, s.worldTime, s.selectedTown), camera);
        bool near = npcIsNearest && nearestNPCIdx == i && inRange;
        const DirSpriteSheet& sheet = activeNPCSheets[i];
        if (sheet.ok) {
            Vector2 facing = WanderFacing(i, s.worldTime);
            Rectangle src = ActorSrcRect(sheet, facing, ActorAnim::Walk, s.worldTime);
            DrawWorldNode(screenPos, kNodeRadius * 0.5f * kTownVisualScale, kColorPanelBg, activeNPCs[i].name, near,
                           "", &sheet.tex, WHITE, &src);
        } else {
            DrawWorldNode(screenPos, kNodeRadius * 0.5f * kTownVisualScale, kColorPanelBg, activeNPCs[i].name, near);
        }
    }
    DrawPlayer(s, WorldToScreen(s.townPlayerPos, camera), s.playerFacing,
                (inRange && !s.selectedTile.has_value()) ? "[E] " + interactLabel : "", kTownVisualScale);
    EndScissorMode();
    DrawVirtualJoystick();
    if (inRange && !s.selectedTile.has_value() && DrawInteractButton("[E] " + interactLabel)) {
        if (npcIsNearest) s.greetedNPC = (s.greetedNPC.has_value() && *s.greetedNPC == nearestNPCIdx)
                                           ? std::nullopt : std::make_optional(nearestNPCIdx);
        else if (gateIsNearest) {
            s.screen = Screen::Wilderness;
            s.wildernessPlayerPos = (s.selectedTown == 0) ? Vector2{ 900, 1650 } : Vector2{ 2900, 1650 };
        }
        else s.selectedTile = nearestKey;
    }

    // Greet popup — small heading+text panel modeled on DrawInnocentPanel's shape, minus
    // its action buttons (nothing to choose, just dismiss). Toggled by E, same as
    // selectedTile's building panels.
    if (s.greetedNPC.has_value()) {
        const TownNPC& npc = activeNPCs[*s.greetedNPC];
        Rectangle greetBg = { 20, 500, (float)(screenW - 40), 100 };
        DrawRectangleRounded(greetBg, 0.06f, 8, Fade(kColorPageBg, 0.97f));
        DrawRectangleRoundedLines(greetBg, 0.06f, 8, Fade(BLACK, 0.4f));
        DrawUIText(npc.name.c_str(), (int)greetBg.x + 16, (int)greetBg.y + 14, 16, kColorHeading);
        DrawUIText(npc.greeting.c_str(), (int)greetBg.x + 16, (int)greetBg.y + 42, 13, kColorText);
        if (Button({ greetBg.x + greetBg.width - 64, greetBg.y + greetBg.height - 38, 44, 26 }, "X", true))
            s.greetedNPC.reset();
    }

    // Gather HUD strip — drawn AFTER (not before) the world render, so the tiled ground
    // fill doesn't paint over it. kViewport starts at y=110, overlapping this strip's
    // y=120-156, so draw order here matters: whichever is drawn last wins the pixels.
    if (Button({ 20, 120, 130, 30 }, "Gather Wood [1]", canGather)) TryStartGather(s, "wood");
    if (Button({ 160, 120, 120, 30 }, "Gather Ore [2]", canGather)) TryStartGather(s, "ore");
    std::string autoLabel = s.autoGather ? "Auto-Gather: ON" : "Auto-Gather: OFF";
    if (Button({ 290, 120, 150, 30 }, autoLabel, true)) ToggleAutoGather(s);
    // Solid-backed (DrawInfoLine, not bare DrawUIText) and split across two short lines
    // instead of one concatenated one — 2026-09-22 fix: this text sits directly on the
    // tiled ground with nothing else guaranteeing contrast (same class of bug already
    // fixed on the vendor screens' backdrops), and the combined skills+gathering string
    // could run long enough to overflow the safe margin on some viewports.
    DrawInfoLine(TextFormat("Lumberjacking: %.1f   Mining: %.1f", s.lumberjacking, s.mining), 20, 156, 12);
    if (s.gatheringResource.has_value())
        DrawInfoLine(TextFormat("Gathering %s... %.1fs", s.gatheringResource->c_str(), s.gatherSecondsRemaining),
                       20, 176, 12);

    // --- Detail / upgrade panel — opened by walking up + E, closed with [X]/[ESC] ---
    if (s.selectedTile.has_value()) {
        std::string key = *s.selectedTile;
        Rectangle panelBg = { 20, 500, (float)(screenW - 40), 220 };
        DrawRectangleRounded(panelBg, 0.06f, 8, Fade(kColorPageBg, 0.97f));
        DrawRectangleRoundedLines(panelBg, 0.06f, 8, Fade(BLACK, 0.4f));
        int panelY = (int)panelBg.y + 16;

        DrawUIText(TileNameFor(key).c_str(), 36, panelY, 18, kColorText);
        if (Button({ (float)(screenW - 80), (float)panelY - 4, 44, 26 }, "X", true)) s.selectedTile.reset();

        if (int idx = FindCraftBuildingIndex(key); idx >= 0) {
            const BuildingDef& def = kCraftBuildings[idx];
            int lvl = s.buildingLevel[idx];
            std::string capLine = "Skill cap: " + std::to_string(def.levels[lvl - 1].cap);
            DrawUIText(capLine.c_str(), 36, panelY + 28, 13, DARKGRAY);

            if (lvl < 5) {
                const BuildingLevel& next = def.levels[lvl];
                std::string resName = def.resource == Resource::Wood ? "wood"
                                      : def.resource == Resource::Ore ? "ore"
                                      : def.resource == Resource::Leather ? "leather" : "";
                std::string costLine = "Upgrade cost: " + std::to_string(next.goldCost) + "g" +
                    (def.resource != Resource::None ? (" + " + std::to_string(next.resourceCost) + " " + resName) : "") +
                    "  (" + std::to_string((int)next.upgradeTimeSec) + "s)";
                DrawUIText(costLine.c_str(), 36, panelY + 48, 13, DARKGRAY);

                bool upgrading = s.upgrading.has_value();
                if (Button({ 36, (float)(panelY + 72), 160, 34 }, "Upgrade", !upgrading))
                    TryStartUpgrade(s, key);
                if (upgrading && s.upgrading->buildingKey == key) {
                    float pct = 1.0f - (s.upgrading->secondsRemaining / next.upgradeTimeSec);
                    Rectangle bar = { 36, (float)(panelY + 112), 200, 8 };
                    DrawRectangleRec(bar, Fade(BLACK, 0.25f));
                    DrawRectangleRec({ bar.x, bar.y, bar.width * std::clamp(pct, 0.0f, 1.0f), bar.height }, kColorSlate);
                }
            } else {
                DrawUIText("Max level reached.", 36, panelY + 48, 13, DARKGRAY);
            }
            // Step inside and actually use the place, instead of tabbing away to Craft
            // by hand — jumps straight to this building's own recipe tab.
            if (Button({ 220, (float)(panelY + 72), 160, 34 }, "Craft here", true)) {
                s.craftBuildingTab = idx;
                s.screen = Screen::Craft;
                s.selectedTile.reset();
            }
        } else {
            DrawUIText("(No upgrades — amenity building.)", 36, panelY + 28, 13, DARKGRAY);
            // Same "step inside and use it" idea as the craft buildings above, for the
            // amenities that have an actual screen of their own.
            struct AmenityLink { const char* label; Screen target; };
            std::optional<AmenityLink> link;
            if (key == "bank" || key == "townhall") link = AmenityLink{ "Manage the Vaultkeep", Screen::Bank };
            else if (key == "stable") link = AmenityLink{ "Visit the Wildkeep", Screen::Pets };
            else if (key == "healer") link = AmenityLink{ "Rest & bandage up", Screen::Character };
            else if (key == "house") link = AmenityLink{ "Go inside", Screen::House };
            else if (key == "provisioner") link = AmenityLink{ "Browse the wares", Screen::Provisioner };
            if (link) {
                if (Button({ 36, (float)(panelY + 56), 220, 34 }, link->label, true)) {
                    s.screen = link->target;
                    s.selectedTile.reset();
                }
            }
        }
        DrawUIText("Walk away or press [X] to close.", 36, (int)panelBg.y + 190, 13, Fade(DARKGRAY, 0.8f));
    } else {
        DrawUIText("WASD/arrows (or drag bottom-left) to move. Walk up to a building and press [E].", 20, screenH - 66, 13, Fade(DARKGRAY, 0.8f));
    }
}

// ---------------------------------------------------------------------
// The Wilderness — reached via the gate in Town (see kWildernessGatePos in
// DrawTownScreen). An open outdoor arena like Town, not a walled dungeon: gather
// nodes reuse TryStartGather (so auto-gather and the gather-completion ambush check
// just work unmodified), creature spots reuse TryStartTameAttempt/ResolveTameAttempt
// (with a "tame" pendingEncounterCheck added alongside "gather" for the same ambush
// roll — see UpdateTameAttempt), and a Town Gate node walks you back out.
// ---------------------------------------------------------------------

static void DrawWildernessScreen(GameState& s, int screenW, int screenH) {
    DrawUIText("The Wilderness — gather wood/ore or tame a creature. Watch for trouble.", 20, 112, 13, kColorAccent);

    UpdateRivalRoaming(s, GetFrameTime()); // before the nearest-search below, so rivalPos is current this frame
    UpdateInnocentSpots(s, GetFrameTime());

    // --- Nearest interactable: gather nodes, creature spots, monster spots, and the
    // return gate all compete in one search, same pattern as the Wilderness Gate vs.
    // buildings in Town.
    enum class WildNodeKind { Gather, Creature, Monster, Rival, Innocent, ReturnGate, DungeonEntrance, Town2Gate };
    WildNodeKind nearestKind = WildNodeKind::ReturnGate;
    int nearestIdx = -1;
    float nearestDist = 1e9f;
    for (size_t i = 0; i < kWildernessGatherNodes.size(); i++) {
        float d = Dist(s.wildernessPlayerPos, kWildernessGatherNodes[i].pos);
        if (d < nearestDist) { nearestDist = d; nearestKind = WildNodeKind::Gather; nearestIdx = (int)i; }
    }
    for (size_t i = 0; i < kWildernessCreatureSpots.size(); i++) {
        float d = Dist(s.wildernessPlayerPos, kWildernessCreatureSpots[i].pos);
        if (d < nearestDist) { nearestDist = d; nearestKind = WildNodeKind::Creature; nearestIdx = (int)i; }
    }
    bool wasEngaged = s.wildEngaged.has_value(); // captured before any of this frame's updates
    for (size_t i = 0; i < kWildernessMonsterSpots.size(); i++) {
        // The engaged slot's live (moving) position takes over from its wander loop
        // once you're fighting it — everything else still searches by its own wander
        // position (WildernessMonsterLivePos), not a fixed spawn point.
        Vector2 pos = (wasEngaged && s.wildEngaged->spotIdx == (int)i) ? s.wildEngaged->pos : WildernessMonsterLivePos((int)i, s.worldTime);
        float d = Dist(s.wildernessPlayerPos, pos);
        if (d < nearestDist) { nearestDist = d; nearestKind = WildNodeKind::Monster; nearestIdx = (int)i; }
    }
    {
        // The Rival Adventurer (2026-09-23) — no longer a kWildernessMonsterSpots row
        // (see GameState::rivalLevel's comment), so it gets its own dedicated distance
        // check here, same pattern as Town2Gate's addition alongside ReturnGate.
        Vector2 pos = (wasEngaged && s.wildEngaged->isRival) ? s.wildEngaged->pos : s.rivalPos;
        float d = Dist(s.wildernessPlayerPos, pos);
        if (d < nearestDist) { nearestDist = d; nearestKind = WildNodeKind::Rival; nearestIdx = -1; }
    }
    for (size_t i = 0; i < kWildernessInnocentSpots.size(); i++) {
        if (!s.innocentSpots[i].present) continue; // empty/respawning — not interactable
        float d = Dist(s.wildernessPlayerPos, WildernessInnocentLivePos((int)i, s.worldTime));
        if (d < nearestDist) { nearestDist = d; nearestKind = WildNodeKind::Innocent; nearestIdx = (int)i; }
    }
    for (size_t i = 0; i < kWildernessDungeonEntrances.size(); i++) {
        float d = Dist(s.wildernessPlayerPos, kWildernessDungeonEntrances[i].pos);
        if (d < nearestDist) { nearestDist = d; nearestKind = WildNodeKind::DungeonEntrance; nearestIdx = (int)i; }
    }
    {
        float d = Dist(s.wildernessPlayerPos, kWildernessReturnGatePos);
        if (d < nearestDist) { nearestDist = d; nearestKind = WildNodeKind::ReturnGate; nearestIdx = -1; }
    }
    {
        // Gate to Town 2 (2026-09-22, "second town" plan) — a genuine walk out from
        // the Town 1 area, same pattern as kWildernessDungeonEntrances.
        float d = Dist(s.wildernessPlayerPos, kWildernessTown2GatePos);
        if (d < nearestDist) { nearestDist = d; nearestKind = WildNodeKind::Town2Gate; nearestIdx = -1; }
    }
    bool inRange = nearestDist < kNodeRadius + kInteractRange;

    // Engaging a monster now starts a *live* fight (GameState::ActiveMonster) instead
    // of the panel-based state.combat ambushes/dungeons still use — see the big comment
    // above kWildernessMonsterSpots. Same monsterMaxHP() formula StartCombat uses
    // (level*3) for starting HP.
    auto tryEngageWildMonster = [&](int idx) {
        const WildernessMonsterSpot& spot = kWildernessMonsterSpots[idx];
        GameState::ActiveMonster am;
        am.spotIdx = idx;
        am.pos = WildernessMonsterLivePos(idx, s.worldTime); // wherever it currently wandered to, not a snap back to spawn
        am.spawnPos = spot.pos;
        am.maxHp = std::max(1.0f, spot.level * 3.0f);
        am.hp = am.maxHp;
        s.wildEngaged = am;
        s.logLine = "You engage the " + spot.name + "!";
    };
    auto tryEngageRival = [&]() {
        GameState::ActiveMonster am;
        am.spotIdx = -1;
        am.isRival = true;
        am.pos = s.rivalPos;
        am.spawnPos = s.rivalPos; // unused for the Rival (see updateTacticalOpponentAI's leash comment) but kept sane
        am.maxHp = std::max(1.0f, s.rivalLevel * 3.0f);
        am.hp = am.maxHp;
        s.wildEngaged = am;
        s.logLine = "The Rival Adventurer turns to face you!";
    };
    // Walking up to a roaming Innocent NPC and pressing E opens the exact same
    // DrawInnocentPanel (Murder/Steal/Snoop/Spare) the old random popup already used —
    // only the trigger is new, see kWildernessInnocentSpots' comment. The spot goes
    // empty and starts its respawn countdown immediately; which choice the player makes
    // in the panel doesn't change that (all four already call innocentEncounter.reset()
    // on their own, this just handles the world-spot side of it).
    auto tryEngageInnocentSpot = [&](int idx) {
        if (s.ambush.has_value() || s.innocentEncounter.has_value() || s.combat.has_value() ||
            s.wildEngaged.has_value() || s.dungeonEngaged.has_value()) return;
        GameState::InnocentSpotState& spot = s.innocentSpots[idx];
        s.innocentEncounter = GameState::InnocentEncounter{ spot.name, spot.gold, false, "wilderness_npc" };
        s.logLine = spot.name + " notices you approaching.";
        spot.present = false;
        spot.respawnTimer = kInnocentRespawnSeconds;
    };

    // --- Live combat with whichever monster is already engaged (if any) ---
    // AI: chase toward the player (leashed to spawn so it can't wander into a
    // neighboring node's territory), disengage if the player breaks far enough away,
    // and the monster's own attack lands on its own cooldown — all independent of
    // input, same as monster wander motion elsewhere. Uses the exact
    // MonsterCounterAndMaybeEnd hit-chance/damage formula (see EndWildMonsterLoss's
    // comment) even though it isn't calling that function directly (no CombatState to
    // hand it — see the big comment above LiveMaybeGainMagicResist).
    auto updateEngagedMonsterAI = [&]() {
        if (!s.wildEngaged.has_value()) return;
        GameState::ActiveMonster& am = *s.wildEngaged;
        const WildernessMonsterSpot& spot = kWildernessMonsterSpots[am.spotIdx];
        float dtF = GetFrameTime();

        // Stop advancing at kWildMeleeRange (60), not kWildMeleeRange*0.6 (36) as this
        // used to say: the player-vs-monster collision floor a few lines below
        // (kPlayerRadius + kNodeRadius*0.6 = 17+30 = 47) is *larger* than 36, so the
        // monster could never actually reach that old target distance — every frame it
        // computed "not close enough yet," stepped toward the player, and collision
        // immediately shoved the player back out to 47 to compensate. That fight-with-
        // itself was the reported "monster pushing the player back" bug. 60 sits safely
        // outside the 47-unit floor, so the monster can actually reach and hold its
        // stopping distance instead of perpetually overshooting into collision.
        float distNow = Dist(am.pos, s.wildernessPlayerPos);
        if (distNow > kWildMeleeRange) {
            Vector2 dir = { s.wildernessPlayerPos.x - am.pos.x, s.wildernessPlayerPos.y - am.pos.y };
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.0001f) {
                dir.x /= len; dir.y /= len;
                Vector2 newPos = { am.pos.x + dir.x * kWildMonsterChaseSpeed * dtF,
                                     am.pos.y + dir.y * kWildMonsterChaseSpeed * dtF };
                float fsx = newPos.x - am.spawnPos.x, fsy = newPos.y - am.spawnPos.y;
                float fsLen = std::sqrt(fsx * fsx + fsy * fsy);
                if (fsLen > kWildMonsterLeashRange) {
                    newPos.x = am.spawnPos.x + fsx / fsLen * kWildMonsterLeashRange;
                    newPos.y = am.spawnPos.y + fsy / fsLen * kWildMonsterLeashRange;
                }
                am.pos = newPos;
            }
        }

        if (Dist(am.pos, s.wildernessPlayerPos) > kWildDisengageRange) {
            s.logLine = "The " + spot.name + " loses interest.";
            s.wildEngaged.reset();
            return;
        }

        if (am.monsterAttackCooldown > 0) am.monsterAttackCooldown -= dtF;
        if (am.playerAttackCooldown > 0) am.playerAttackCooldown -= dtF;
        if (am.playerSpellCooldown > 0) am.playerSpellCooldown -= dtF;
        if (am.swingEffectTimer > 0) am.swingEffectTimer -= dtF;
        if (am.castEffectTimer > 0) am.castEffectTimer -= dtF;

        bool inMelee = Dist(am.pos, s.wildernessPlayerPos) < kWildMeleeRange;
        if (inMelee && am.monsterAttackCooldown <= 0) {
            am.monsterAttackCooldown = kWildMonsterAttackCooldown;
            std::string mname = spot.name;
            if (RandUnit() * 100.0f < MonsterHitChance(s)) {
                float raw = spot.level * (0.8f + RandUnit() * 0.6f);
                int dmg = std::max(1, (int)std::round(raw - TotalDefense(s) * 0.3f));
                s.hp -= dmg;
                s.logLine = "The " + mname + " hits you for " + std::to_string(dmg) + " damage";
            } else {
                s.logLine = "The " + mname + " misses";
            }
            if (s.hp <= 0) { EndWildMonsterLoss(s, mname); return; }
        }

        // AI companion's turn — autonomous, cooldown-driven (2026-09-22, "AI players"
        // plan Part 2), rather than reactive to the player's own action like the old
        // panel's pet turn was. Uses the exact same ResolvePetTurnLive formulas the
        // Hunt-screen dungeon fights also use.
        if (s.companionAttackCooldown > 0) s.companionAttackCooldown -= dtF;
        if (ActivePet(s) && s.companionAttackCooldown <= 0) {
            s.companionAttackCooldown = kCompanionAttackCooldown;
            std::string mname = spot.name; int mgold = spot.baseGold, mleather = spot.baseLeather;
            int level = spot.level;
            ResolvePetTurnLive(s, am.hp, level);
            if (am.hp <= 0) { EndWildMonsterWin(s, mname, mgold, mleather); return; }
        }
    };
    // The tactical opponent's own AI (2026-09-22, "AI players" plan Part 3) — confirmed
    // via research to be the first monster in the whole game with any real decision-
    // making at all (every other monster, in every combat system, always melees with one
    // fixed hit roll and never retreats). Called INSTEAD of updateEngagedMonsterAI (see
    // the call site below) when the engaged monster is flagged
    // GameState::ActiveMonster::isRival — a separate function rather than a branch
    // inside the normal one, since the decision logic is genuinely different, not a
    // tweak to it.
    auto updateTacticalOpponentAI = [&]() {
        if (!s.wildEngaged.has_value()) return;
        GameState::ActiveMonster& am = *s.wildEngaged;
        EngagedMonsterStats spot = EngagedWildMonsterStats(s, am);
        float dtF = GetFrameTime();

        if (am.monsterAttackCooldown > 0) am.monsterAttackCooldown -= dtF;
        if (am.monsterSpecialCooldown > 0) am.monsterSpecialCooldown -= dtF;
        if (am.swingEffectTimer > 0) am.swingEffectTimer -= dtF;
        if (am.castEffectTimer > 0) am.castEffectTimer -= dtF;

        // Below ~25% HP: flee directly away from the player for a few seconds instead of
        // fighting on — no existing monster has ever done this (Flee/FleeCombat are
        // player-only actions everywhere else in the game).
        if (am.isFleeing) {
            am.fleeTimer -= dtF;
            Vector2 away = { am.pos.x - s.wildernessPlayerPos.x, am.pos.y - s.wildernessPlayerPos.y };
            float len = std::sqrt(away.x * away.x + away.y * away.y);
            if (len > 0.0001f) {
                away.x /= len; away.y /= len;
                am.pos.x += away.x * kWildMonsterChaseSpeed * dtF;
                am.pos.y += away.y * kWildMonsterChaseSpeed * dtF;
            }
            if (am.fleeTimer <= 0.0f) am.isFleeing = false; // re-engages normally next frame
            return;
        }
        if (am.hp / am.maxHp < 0.25f) { am.isFleeing = true; am.fleeTimer = 3.0f; return; }

        // Not fleeing — chase into range exactly like a normal monster (same chase-then-
        // leash math as updateEngagedMonsterAI), except leashed to its *current* pos
        // rather than a spawn point — it has no fixed spawn anymore (see
        // GameState::rivalLevel's comment), so this just prevents runaway movement
        // within a single frame rather than enforcing a real territory.
        float distNow = Dist(am.pos, s.wildernessPlayerPos);
        if (distNow > kWildMeleeRange) {
            Vector2 dir = { s.wildernessPlayerPos.x - am.pos.x, s.wildernessPlayerPos.y - am.pos.y };
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.0001f) {
                dir.x /= len; dir.y /= len;
                am.pos.x += dir.x * kWildMonsterChaseSpeed * dtF;
                am.pos.y += dir.y * kWildMonsterChaseSpeed * dtF;
            }
        }
        if (Dist(am.pos, s.wildernessPlayerPos) > kWildDisengageRange) {
            s.logLine = "The " + spot.name + " loses interest.";
            RivalFightEnded(s, am); // persists its position/resumes roaming from here — no win/loss, so no growth nudge beyond that
            s.wildEngaged.reset();
            return;
        }

        // Tactical decision: prefer a ranged strike over melee whenever it's off
        // cooldown, rather than always closing to melee like every other monster — a
        // simple priority rule (not a full utility-AI), matching this project's existing
        // "small, honest simplifications" convention.
        bool inMelee = Dist(am.pos, s.wildernessPlayerPos) < kWildMeleeRange;
        if (am.monsterSpecialCooldown <= 0) {
            am.monsterSpecialCooldown = kTacticalRangedCooldown;
            std::string mname = spot.name;
            if (RandUnit() * 100.0f < MonsterHitChance(s)) {
                float raw = spot.level * (0.9f + RandUnit() * 0.5f); // slightly harder-hitting than a plain melee swing
                int dmg = std::max(1, (int)std::round(raw - TotalDefense(s) * 0.3f));
                s.hp -= dmg;
                s.logLine = "The " + mname + " strikes you from range for " + std::to_string(dmg) + " damage";
            } else {
                s.logLine = "The " + mname + "'s ranged strike misses";
            }
            if (s.hp <= 0) {
                bool wasAlreadyBeaten = s.rivalHasBeatenPlayer;
                RivalFightEnded(s, am);
                s.rivalHasBeatenPlayer = true;
                if (wasAlreadyBeaten) EndWildMonsterMurdererLoss(s, mname); else EndWildMonsterLoss(s, mname);
                return;
            }
        } else if (inMelee && am.monsterAttackCooldown <= 0) {
            am.monsterAttackCooldown = kWildMonsterAttackCooldown;
            std::string mname = spot.name;
            if (RandUnit() * 100.0f < MonsterHitChance(s)) {
                float raw = spot.level * (0.8f + RandUnit() * 0.6f);
                int dmg = std::max(1, (int)std::round(raw - TotalDefense(s) * 0.3f));
                s.hp -= dmg;
                s.logLine = "The " + mname + " hits you for " + std::to_string(dmg) + " damage";
            } else {
                s.logLine = "The " + mname + " misses";
            }
            if (s.hp <= 0) {
                bool wasAlreadyBeaten = s.rivalHasBeatenPlayer;
                RivalFightEnded(s, am);
                s.rivalHasBeatenPlayer = true;
                if (wasAlreadyBeaten) EndWildMonsterMurdererLoss(s, mname); else EndWildMonsterLoss(s, mname);
                return;
            }
        }

        // The AI companion still helps against this opponent too, same as any other
        // fight — no special-casing needed here.
        if (s.companionAttackCooldown > 0) s.companionAttackCooldown -= dtF;
        if (ActivePet(s) && s.companionAttackCooldown <= 0) {
            s.companionAttackCooldown = kCompanionAttackCooldown;
            std::string mname = spot.name; int mgold = spot.baseGold, mleather = spot.baseLeather;
            int level = spot.level;
            ResolvePetTurnLive(s, am.hp, level);
            if (am.hp <= 0) {
                bool wasMurdererTier = s.rivalHasBeatenPlayer;
                RivalFightEnded(s, am);
                EndWildMonsterWin(s, mname, mgold, mleather);
                if (wasMurdererTier) { GainFame(s, 10.0f); s.notoriety = std::max(0.0f, s.notoriety - 10.0f); }
                return;
            }
        }
    };
    // Action (not automatic AI) — the player's own swing, rate-limited by its own
    // cooldown. Called from both the keyboard (KEY_E) and touch (DrawInteractButton)
    // paths below, same dual-input pattern tryInteract() uses elsewhere on this screen.
    // Uses the exact ResolveCombatRound hit-chance/damage formula (see EndWildMonsterWin's
    // comment for why it's copied rather than shared via a CombatState).
    auto trySwingAtEngagedMonster = [&]() {
        if (!s.wildEngaged.has_value()) return;
        GameState::ActiveMonster& am = *s.wildEngaged;
        EngagedMonsterStats spot = EngagedWildMonsterStats(s, am);
        if (Dist(am.pos, s.wildernessPlayerPos) >= kWildMeleeRange || am.playerAttackCooldown > 0) return;
        am.playerAttackCooldown = PlayerSwingCooldown(s);
        am.swingEffectTimer = kSwingEffectDuration;
        int power = CombatPower(s);
        float weaponSkillBonus = EffectiveSkill(s, ActiveWeaponSkillField(s)) * 0.2f;
        float hitChance = std::clamp(50.0f + (power - spot.level) * 4.0f + weaponSkillBonus, 5.0f, 95.0f);
        LiveApplyWeaponTraining(s);
        std::string mname = spot.name; int mgold = spot.baseGold, mleather = spot.baseLeather;
        if (RandUnit() * 100.0f < hitChance) {
            int dmg = std::max(1, (int)std::round(power * (0.85f + RandUnit() * 0.3f)));
            am.hp -= dmg;
            s.logLine = "You hit the " + mname + " for " + std::to_string(dmg) + " damage";
            if (am.hp <= 0) {
                if (am.isRival) {
                    bool wasMurdererTier = s.rivalHasBeatenPlayer;
                    RivalFightEnded(s, am);
                    EndWildMonsterWin(s, mname, mgold, mleather);
                    if (wasMurdererTier) { GainFame(s, 10.0f); s.notoriety = std::max(0.0f, s.notoriety - 10.0f); }
                } else {
                    EndWildMonsterWin(s, mname, mgold, mleather);
                }
            }
        } else {
            s.logLine = "Your attack misses";
        }
    };
    // Magery in live Wilderness combat — ranged (no kWildMeleeRange check, unlike the
    // sword swing above), on its own flat cooldown (kWildSpellCastCooldown) so casting
    // doesn't share a clock with melee. Reuses the exact same success/damage/training
    // formulas as the panel-based CastOffensiveSpell (SpellSuccessChance/SpellPowerFor/
    // ApplySpellTraining) since it's the same underlying magic system — just applied to
    // s.wildEngaged's hp instead of s.combat's. Simplified vs. the panel version: no
    // RollSpellDisrupted check, since that reads s.combat->playerWasHit, which has no
    // equivalent in live combat — an intentional simplification, not an oversight.
    auto tryCastSpellAtEngagedMonster = [&](int spellIdx) {
        if (!s.wildEngaged.has_value()) return;
        GameState::ActiveMonster& am = *s.wildEngaged;
        if (am.playerSpellCooldown > 0) return;
        const Spell& spell = kSpells[spellIdx];
        if (spell.type != SpellType::Offensive) return;
        if (s.mana < spell.manaCost || s.reagents < kLiveCombatReagentCost) {
            s.logLine = "Not enough mana or reagents for " + spell.name + ".";
            return;
        }
        am.playerSpellCooldown = kWildSpellCastCooldown;
        am.castEffectTimer = kCastEffectDuration;
        s.mana -= spell.manaCost;
        s.reagents -= kLiveCombatReagentCost;
        EngagedMonsterStats spot = EngagedWildMonsterStats(s, am);
        std::string mname = spot.name; int mgold = spot.baseGold, mleather = spot.baseLeather;
        std::string note;
        bool success = RandUnit() * 100.0f < SpellSuccessChance(s, spell);
        ApplySpellTraining(s, spell, note);
        if (success) {
            int dmg = std::max(1, (int)std::round(SpellPowerFor(s, spell) * (0.85f + RandUnit() * 0.3f)));
            am.hp -= dmg;
            s.logLine = spell.name + " hits the " + mname + " for " + std::to_string(dmg) + " damage" + note;
            if (am.hp <= 0) {
                if (am.isRival) {
                    bool wasMurdererTier = s.rivalHasBeatenPlayer;
                    RivalFightEnded(s, am);
                    EndWildMonsterWin(s, mname, mgold, mleather);
                    if (wasMurdererTier) { GainFame(s, 10.0f); s.notoriety = std::max(0.0f, s.notoriety - 10.0f); }
                } else {
                    EndWildMonsterWin(s, mname, mgold, mleather);
                }
            }
        } else {
            s.logLine = spell.name + " fizzles!" + note;
        }
    };
    // Walking into a dungeon entrance does exactly what its Hunt-tab does today —
    // s.selectedDungeon is left as-is if you're re-entering the one you were already in
    // (mirrors the tab-click behavior: only a fresh spawn point on switching dungeons).
    auto tryEnterDungeon = [&](int idx) {
        if (!s.selectedDungeon.has_value() || *s.selectedDungeon != idx) s.dungeonPlayerPos = { 900, 1300 };
        s.selectedDungeon = idx;
        s.huntSubView = 0;
        s.screen = Screen::Hunt;
    };
    auto tryInteract = [&]() {
        if (nearestKind == WildNodeKind::Gather) TryStartGather(s, kWildernessGatherNodes[nearestIdx].resource, 5.0f);
        else if (nearestKind == WildNodeKind::Creature) TryStartTameAttempt(s, kWildernessCreatureSpots[nearestIdx].creatureIdx);
        else if (nearestKind == WildNodeKind::Monster) tryEngageWildMonster(nearestIdx);
        else if (nearestKind == WildNodeKind::Rival) tryEngageRival();
        else if (nearestKind == WildNodeKind::Innocent) tryEngageInnocentSpot(nearestIdx);
        else if (nearestKind == WildNodeKind::DungeonEntrance) tryEnterDungeon(kWildernessDungeonEntrances[nearestIdx].dungeonIdx);
        else if (nearestKind == WildNodeKind::Town2Gate) {
            s.selectedTown = 1;
            s.screen = Screen::Town;
            s.townPlayerPos = { 450, 830 }; // same relative spawn every town uses, just south of its own gate
        }
        else { s.selectedTown = 0; s.screen = Screen::Town; s.townPlayerPos = { 450, 830 }; } // just south of kWildernessGatePos
    };
    // Being engaged in a live fight takes over the prompt/E-press entirely — same
    // "combat blocks other actions" convention the old panel-based system already had
    // (see tabsEnabled in main()), just enforced here instead since this fight never
    // leaves the Wilderness screen.
    std::string prompt;
    if (wasEngaged) {
        // Melee is automatic now (see trySwingAtEngagedMonster's call site below) — no
        // button/prompt needed for it, just naming who you're fighting.
        prompt = "Fighting " + kWildernessMonsterSpots[s.wildEngaged->spotIdx].name;
    } else if (inRange) {
        if (nearestKind == WildNodeKind::Gather) prompt = "[E] Gather " + kWildernessGatherNodes[nearestIdx].resource;
        else if (nearestKind == WildNodeKind::Creature)
            prompt = "[E] Tame " + kWildCreatures[kWildernessCreatureSpots[nearestIdx].creatureIdx].name;
        else if (nearestKind == WildNodeKind::Monster) prompt = "[E] Fight " + kWildernessMonsterSpots[nearestIdx].name;
        else if (nearestKind == WildNodeKind::Rival) prompt = "[E] Fight Rival Adventurer";
        else if (nearestKind == WildNodeKind::Innocent) prompt = "[E] Approach " + s.innocentSpots[nearestIdx].name;
        else if (nearestKind == WildNodeKind::DungeonEntrance)
            prompt = "[E] Enter " + kDungeons[kWildernessDungeonEntrances[nearestIdx].dungeonIdx].name;
        else if (nearestKind == WildNodeKind::Town2Gate) prompt = "[E] Enter " + std::string(kTown2Name);
        else prompt = "[E] Return to Town";
    }

    UpdatePlayerMovement(s.wildernessPlayerPos, s.playerFacing, GetFrameTime(), kWildernessWorldSize);
    if (ActivePet(s)) UpdateCompanionFollow(s, s.wildernessPlayerPos, s.playerFacing, GetFrameTime());
    for (auto& node : kWildernessGatherNodes)
        ResolveCircleCollision(s.wildernessPlayerPos, kPlayerRadius, node.pos, kNodeRadius * 0.7f);
    for (auto& spot : kWildernessCreatureSpots)
        ResolveCircleCollision(s.wildernessPlayerPos, kPlayerRadius, spot.pos, kNodeRadius * 0.8f);
    for (size_t i = 0; i < kWildernessMonsterSpots.size(); i++) {
        // The engaged one collides against its live position (below); the other 4
        // wander in a small loop (WildernessMonsterLivePos) and auto-engage the player
        // on contact — bumping into one starts the fight, no E press required (walking
        // up and pressing E while in range still works too, via tryInteract).
        if (wasEngaged && s.wildEngaged->spotIdx == (int)i) continue;
        Vector2 livePos = WildernessMonsterLivePos((int)i, s.worldTime);
        if (!s.wildEngaged.has_value() && Dist(s.wildernessPlayerPos, livePos) < kPlayerRadius + kNodeRadius * 0.7f)
            tryEngageWildMonster((int)i);
        ResolveCircleCollision(s.wildernessPlayerPos, kPlayerRadius, livePos, kNodeRadius * 0.7f);
    }
    if (wasEngaged)
        ResolveCircleCollision(s.wildernessPlayerPos, kPlayerRadius, s.wildEngaged->pos, kNodeRadius * 0.6f);
    for (auto& entrance : kWildernessDungeonEntrances)
        ResolveCircleCollision(s.wildernessPlayerPos, kPlayerRadius, entrance.pos, kNodeRadius * 0.8f);
    ResolveCircleCollision(s.wildernessPlayerPos, kPlayerRadius, kWildernessReturnGatePos, kNodeRadius);
    ResolveCircleCollision(s.wildernessPlayerPos, kPlayerRadius, kWildernessTown2GatePos, kNodeRadius);
    s.wildernessPlayerPos = ClampToWorld(s.wildernessPlayerPos, kPlayerEdgeMargin, kWildernessWorldSize);

    // AI reacts to this frame's final (post-collision) player position; the player's
    // own swing is a separate action so it can also be triggered by touch, below. The
    // Rival Adventurer (ActiveMonster::isRival) gets its own AI function instead — see
    // updateTacticalOpponentAI's comment.
    if (s.wildEngaged.has_value() && s.wildEngaged->isRival)
        updateTacticalOpponentAI();
    else
        updateEngagedMonsterAI();
    // Auto-continuous melee: fires on its own cooldown every frame once engaged and in
    // range, no button press needed — mirrors updateEngagedMonsterAI's unconditional
    // per-frame check for the monster's own attack. trySwingAtEngagedMonster already
    // self-gates on range/cooldown/wildEngaged internally, so calling it unconditionally
    // here is safe.
    if (wasEngaged) {
        trySwingAtEngagedMonster();
    } else if (inRange && IsKeyPressed(KEY_E)) {
        tryInteract();
    }

    BeginScissorMode((int)kViewport.x, (int)kViewport.y, (int)kViewport.width, (int)kViewport.height);
    Vector2 camera = CameraTopLeft(s.wildernessPlayerPos, kWildernessWorldSize);
    DrawTiledGround(g_assets.groundGrassOk ? &g_assets.groundGrass : nullptr, kViewport, camera, 48.0f,
                      Color{ 170, 188, 148, 255 }); // a shade greener/wilder than Town's tended-grass tint

    // Dirt paths from the Return Gate to each dungeon entrance — same DrawWallBand/
    // outline treatment as Town's roads, so the map reads as a connected place instead
    // of open grass with icons scattered on it.
    {
        const Texture2D* wildDirtTex = g_assets.groundDirtOk ? &g_assets.groundDirt : nullptr;
        for (const WildernessDungeonEntrance& entrance : kWildernessDungeonEntrances)
            DrawWildPath(kWildernessReturnGatePos, entrance.pos, camera, wildDirtTex);
    }

    // Decorative bush/fern scatter — drawn first (no collision) so nodes layer on top of
    // any incidental overlap. CraftPix "Rocks & Bushes", same license as the tame-spot art.
    for (const WildernessFoliage& f : kWildernessFoliage) {
        const Texture2D* icon = WildFoliageIcon(f.variant);
        if (!icon) continue;
        Vector2 screenPos = WorldToScreen(f.pos, camera);
        if (screenPos.x < kViewport.x - 30 || screenPos.x > kViewport.x + kViewport.width + 30 ||
            screenPos.y < kViewport.y - 30 || screenPos.y > kViewport.y + kViewport.height + 30) continue;
        DrawIconCentered(*icon, screenPos, 34.0f, WHITE);
    }

    int oreSeen = 0;
    for (size_t i = 0; i < kWildernessGatherNodes.size(); i++) {
        const WildernessGatherNode& node = kWildernessGatherNodes[i];
        bool isWood = node.resource == "wood";
        const Texture2D* icon;
        if (isWood) icon = g_assets.wildTreeOk ? &g_assets.wildTree : nullptr;
        else {
            int oreIdx = oreSeen++;
            icon = g_assets.wildOreTexOk[oreIdx] ? &g_assets.wildOreTex[oreIdx] : (g_assets.wildRockOk ? &g_assets.wildRock : nullptr);
        }
        bool near = nearestKind == WildNodeKind::Gather && nearestIdx == (int)i && inRange;
        Vector2 screenPos = WorldToScreen(node.pos, camera);
        DrawWorldNode(screenPos, kNodeRadius * 0.6f, isWood ? Color{ 90, 110, 60, 255 } : Color{ 120, 116, 110, 255 },
                       isWood ? "Tree" : "Ore Vein", near, "", icon);
    }
    for (size_t i = 0; i < kWildernessCreatureSpots.size(); i++) {
        const WildernessCreatureSpot& spot = kWildernessCreatureSpots[i];
        const WildCreature& creature = kWildCreatures[spot.creatureIdx];
        const DirSpriteSheet& sheet = g_assets.wildCreatureTex[spot.creatureIdx];
        bool near = nearestKind == WildNodeKind::Creature && nearestIdx == (int)i && inRange;
        std::string sub = TextFormat("diff %d - %.0f%%", creature.difficulty, TameChance(s, creature));
        Vector2 screenPos = WorldToScreen(spot.pos, camera);
        // Tamable creatures don't wander (spot.pos is a fixed point, unlike Wilderness
        // monsters/dungeon monsters), so there's no motion to derive a facing from — but
        // they can still face the player, the same "notices you approaching" read the
        // Rival/engaged-monster blocks already use elsewhere (2026-09-23 wire-up).
        if (sheet.ok) {
            Vector2 toPlayer = { s.wildernessPlayerPos.x - spot.pos.x, s.wildernessPlayerPos.y - spot.pos.y };
            float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
            Vector2 facing = len > 0.001f ? Vector2{ toPlayer.x / len, toPlayer.y / len } : Vector2{ 0, 1 };
            Rectangle src = ActorSrcRect(sheet, facing, ActorAnim::Idle, s.worldTime);
            DrawWorldNode(screenPos, kNodeRadius * 0.8f, Color{ 96, 72, 54, 255 }, creature.name, near, sub, &sheet.tex, WHITE, &src);
        } else {
            DrawWorldNode(screenPos, kNodeRadius * 0.8f, Color{ 96, 72, 54, 255 }, creature.name, near, sub);
        }
    }
    for (size_t i = 0; i < kWildernessMonsterSpots.size(); i++) {
        // The engaged slot is drawn separately below, at its live position with an HP
        // bar, instead of here at its idle spawn spot. Checks s.wildEngaged fresh
        // (not the frame-start wasEngaged) since updateEngagedMonsterAI() above may
        // have just ended the fight this same frame — wasEngaged would still be true
        // then, and dereferencing an emptied optional is undefined behavior.
        if (s.wildEngaged.has_value() && !s.wildEngaged->isRival && s.wildEngaged->spotIdx == (int)i) continue;
        const WildernessMonsterSpot& spot = kWildernessMonsterSpots[i];
        const DirSpriteSheet& sheet = g_assets.wildMonsterTex[spot.iconIdx];
        bool near = nearestKind == WildNodeKind::Monster && nearestIdx == (int)i && inRange;
        std::string sub = TextFormat("lvl %d - %.0f%%", spot.level, WinChancePreview(s, spot.level));
        Vector2 screenPos = WorldToScreen(WildernessMonsterLivePos((int)i, s.worldTime), camera);
        if (sheet.ok) {
            Rectangle src = ActorSrcRect(sheet, { 0, 1 }, ActorAnim::Idle, s.worldTime);
            DrawWorldNode(screenPos, kNodeRadius * 0.7f, Color{ 122, 46, 46, 255 }, spot.name, near, sub, &sheet.tex, WHITE, &src);
        } else {
            DrawWorldNode(screenPos, kNodeRadius * 0.7f, Color{ 122, 46, 46, 255 }, spot.name, near, sub);
        }
    }
    if (s.wildEngaged.has_value()) {
        // Live, moving monster — "near" (the highlight ring) now means "close enough to
        // swing" instead of "close enough to engage", reusing DrawWorldNode's existing
        // ring rather than adding a second visual for the same idea.
        EngagedMonsterStats spot = EngagedWildMonsterStats(s, *s.wildEngaged);
        const DirSpriteSheet& sheet = s.wildEngaged->isRival ? g_assets.rivalAdventurerSheet : g_assets.wildMonsterTex[kWildernessMonsterSpots[s.wildEngaged->spotIdx].iconIdx];
        Vector2 screenPos = WorldToScreen(s.wildEngaged->pos, camera);
        bool inMelee = Dist(s.wildEngaged->pos, s.wildernessPlayerPos) < kWildMeleeRange;
        if (sheet.ok) {
            // Faces the player directly rather than tracking real per-frame velocity —
            // chasing monsters always move straight at the player anyway, so this reads
            // identically without needing a stored previous-position/facing field.
            Vector2 toPlayer = { s.wildernessPlayerPos.x - s.wildEngaged->pos.x, s.wildernessPlayerPos.y - s.wildEngaged->pos.y };
            float toPlayerLen = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
            Vector2 facing = toPlayerLen > 0.001f ? Vector2{ toPlayer.x / toPlayerLen, toPlayer.y / toPlayerLen } : Vector2{ 0, 1 };
            Rectangle src = ActorSrcRect(sheet, facing, ActorAnim::Walk, s.worldTime);
            DrawWorldNode(screenPos, kNodeRadius * 0.7f, Color{ 122, 46, 46, 255 }, spot.name, inMelee, "", &sheet.tex, WHITE, &src);
        } else {
            DrawWorldNode(screenPos, kNodeRadius * 0.7f, Color{ 122, 46, 46, 255 }, spot.name, inMelee, "");
        }
        float hpPct = std::clamp(s.wildEngaged->hp / s.wildEngaged->maxHp, 0.0f, 1.0f);
        Rectangle hpBg = { screenPos.x - 30, screenPos.y - kNodeRadius * 0.7f - 26, 60, 8 };
        DrawRectangleRec(hpBg, Fade(BLACK, 0.4f));
        DrawRectangleRec({ hpBg.x, hpBg.y, hpBg.width * hpPct, hpBg.height }, Color{ 122, 46, 46, 255 });
        DrawRectangleLinesEx(hpBg, 1.0f, Fade(RAYWHITE, 0.8f));
    } else {
        // Roaming, not currently fought — patrolling or actively hunting the player
        // (2026-09-23, "Rival hunts you" plan). Sub-label surfaces which, both for
        // legibility and because "Hunting..." is a genuinely useful warning.
        Vector2 screenPos = WorldToScreen(s.rivalPos, camera);
        bool near = nearestKind == WildNodeKind::Rival && inRange;
        std::string sub = s.rivalActivity == GameState::RivalActivity::Hunting ? "Hunting..." : "Patrolling";
        const DirSpriteSheet& sheet = g_assets.rivalAdventurerSheet;
        if (sheet.ok) {
            Vector2 dir = s.rivalActivity == GameState::RivalActivity::Hunting
                ? Vector2{ s.wildernessPlayerPos.x - s.rivalPos.x, s.wildernessPlayerPos.y - s.rivalPos.y }
                : Vector2{ s.rivalPatrolTarget.x - s.rivalPos.x, s.rivalPatrolTarget.y - s.rivalPos.y };
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            Vector2 facing = len > 0.001f ? Vector2{ dir.x / len, dir.y / len } : Vector2{ 0, 1 };
            Rectangle src = ActorSrcRect(sheet, facing, ActorAnim::Walk, s.worldTime);
            DrawWorldNode(screenPos, kNodeRadius * 0.7f, Color{ 122, 46, 46, 255 }, "Rival Adventurer", near, sub, &sheet.tex, WHITE, &src);
        } else {
            DrawWorldNode(screenPos, kNodeRadius * 0.7f, Color{ 122, 46, 46, 255 }, "Rival Adventurer", near, sub);
        }
    }
    for (size_t i = 0; i < kWildernessInnocentSpots.size(); i++) {
        if (!s.innocentSpots[i].present) continue;
        bool near = nearestKind == WildNodeKind::Innocent && nearestIdx == (int)i && inRange;
        Vector2 screenPos = WorldToScreen(WildernessInnocentLivePos((int)i, s.worldTime), camera);
        // No dedicated art for these — a neutral (not monster-red) colored circle reads
        // as "not a threat," same fallback DrawWorldNode already uses for icon-less
        // nodes elsewhere.
        DrawWorldNode(screenPos, kNodeRadius * 0.6f, Color{ 150, 140, 110, 255 }, s.innocentSpots[i].name, near);
    }
    for (size_t i = 0; i < kWildernessDungeonEntrances.size(); i++) {
        const WildernessDungeonEntrance& entrance = kWildernessDungeonEntrances[i];
        bool near = nearestKind == WildNodeKind::DungeonEntrance && nearestIdx == (int)i && inRange;
        const DungeonDef& dungeon = kDungeons[entrance.dungeonIdx];
        Vector2 screenPos = WorldToScreen(entrance.pos, camera);
        const Texture2D* icon = g_assets.wildEntranceTexOk[i] ? &g_assets.wildEntranceTex[i] : nullptr;
        DrawWorldNode(screenPos, kNodeRadius * 0.9f, entrance.color, dungeon.name, near, dungeon.theme, icon);
    }
    {
        bool near = nearestKind == WildNodeKind::ReturnGate && inRange;
        Vector2 screenPos = WorldToScreen(kWildernessReturnGatePos, camera);
        DrawWorldNode(screenPos, kNodeRadius * 0.8f, kColorPanelBg, "Town Gate", near);
    }
    {
        // Gate to Town 2 (2026-09-22) — same treatment as the Town 1 gate just above,
        // out in the newly added Wilderness space (kWildernessTown2GatePos).
        bool near = nearestKind == WildNodeKind::Town2Gate && inRange;
        Vector2 screenPos = WorldToScreen(kWildernessTown2GatePos, camera);
        DrawWorldNode(screenPos, kNodeRadius * 0.9f, Color{ 140, 148, 156, 255 }, kTown2Name, near, "Coastal trade port");
    }
    // AI companion (2026-09-23): now renders as a real animated creature instead of a
    // colored circle, via WildCreatureSheetForRole — see that function's comment for
    // why it's "a creature representative of this Pet's role", not its exact tamed
    // species (Pet doesn't record which of the 11 kWildCreatures it came from, only
    // its role/stats/name; adding that would mean touching the taming system, out of
    // scope for this art pass).
    if (Pet* companion = ActivePet(s)) {
        Vector2 companionScreenPos = WorldToScreen(s.companionPos, camera);
        const DirSpriteSheet& sheet = WildCreatureSheetForRole(companion->role);
        if (sheet.ok) {
            Vector2 toPlayer = { s.wildernessPlayerPos.x - s.companionPos.x, s.wildernessPlayerPos.y - s.companionPos.y };
            float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
            Vector2 facing = len > 0.001f ? Vector2{ toPlayer.x / len, toPlayer.y / len } : Vector2{ 0, 1 };
            Rectangle src = ActorSrcRect(sheet, facing, ActorAnim::Walk, s.worldTime);
            DrawWorldNode(companionScreenPos, kNodeRadius * 0.5f, Color{ 63, 94, 63, 255 }, companion->name, false, "", &sheet.tex, WHITE, &src);
        } else {
            DrawWorldNode(companionScreenPos, kNodeRadius * 0.5f, Color{ 63, 94, 63, 255 }, companion->name, false);
        }
    }

    // Weapon swing / spell cast (2026-09-23): the hero sheet now has real per-direction
    // Attack and Cast frames (see LoadGameAssets' heroSheet column-range comments), so
    // this picks the animation state instead of driving the old rotation-arc hack.
    // Attack takes priority if somehow both timers are live at once (shouldn't overlap
    // in practice — melee and magic are on separate cooldowns but not literally
    // exclusive). Reads `s.wildEngaged` fresh rather than the frame-start `wasEngaged`,
    // since updateEngagedMonsterAI() above can end the fight (and reset it) earlier in
    // this same frame (same stale-optional pitfall documented elsewhere in this function).
    ActorAnim wildCombatAnim = ActorAnim::Idle;
    if (s.wildEngaged.has_value()) {
        if (s.wildEngaged->swingEffectTimer > 0.0f) wildCombatAnim = ActorAnim::Attack;
        else if (s.wildEngaged->castEffectTimer > 0.0f) wildCombatAnim = ActorAnim::Cast;
    }
    DrawPlayer(s, WorldToScreen(s.wildernessPlayerPos, camera), s.playerFacing, prompt, 1.0f, wildCombatAnim);
    EndScissorMode();
    DrawVirtualJoystick();
    if (wasEngaged) {
        // Melee is fully automatic now (see the trySwingAtEngagedMonster call site
        // above) — no interact button needed here anymore for it.
        // Drawn after EndScissorMode (not before), same reason Town's gather HUD strip
        // is — kViewport starts at y=110 and the tiled ground fill would paint over
        // anything drawn here earlier in the frame.
        DrawLiveCombatHud(s, 20, 116);
        DrawLiveCombatQuickItems(s);
    } else if (inRange && DrawInteractButton(prompt)) {
        tryInteract();
    }

    // Spell hotbar — only while actually engaged (2026-09-22 fix: it used to also show
    // while just exploring "so it could be configured between fights," but that spot
    // (x:175-509) directly overlaps the interact button (x:390-520, same y) used for
    // "[E] Gather/Tame/Fight/Enter" prompts, silently covering it whenever nothing was
    // engaged — Mark caught this by trying to gather. Configuration now lives on the
    // Magic screen instead, which has real free space (see DrawMagicScreen).
    if (wasEngaged) {
        float cd = s.wildEngaged->playerSpellCooldown;
        int tapped = DrawCombatHotbarRow(s, true, cd);
        if (tapped >= 0) {
            int spellIdx = s.combatHotbar[tapped];
            if (spellIdx >= 0) {
                const Spell& sp = kSpells[spellIdx];
                if (sp.type == SpellType::Offensive) tryCastSpellAtEngagedMonster(spellIdx);
                else CastLiveUtilitySpell(s, spellIdx);
            }
        }
    }
    DrawHotbarPicker(s, screenW, screenH);

    // Status strip, mirrors Town's gather HUD — solid-backed and split across separate
    // short lines rather than one long concatenated string (2026-09-22 fix, same reason
    // as Town's: no contrast guarantee against the tiled ground, and the combined
    // skills+gathering+taming text could run past a safe margin on some viewports).
    DrawInfoLine(TextFormat("Lumberjacking: %.1f   Mining: %.1f   Taming: %.1f",
                              s.lumberjacking, s.mining, s.animalTaming), 20, 156, 12);
    int statusY = 176;
    if (s.gatheringResource.has_value()) {
        DrawInfoLine(TextFormat("Gathering %s... %.1fs", s.gatheringResource->c_str(), s.gatherSecondsRemaining),
                       20, statusY, 12);
        statusY += 20;
    }
    if (s.tamingAttempt.has_value()) {
        DrawInfoLine(TextFormat("Taming %s... %.1fs",
                                   kWildCreatures[s.tamingAttempt->creatureIdx].name.c_str(), s.tamingAttempt->secondsRemaining),
                       20, statusY, 12);
    }
}

// ---------------------------------------------------------------------
// Hunt screen: dungeon tabs, monster/boss hunt buttons with a live win-
// chance preview, a small weapon shop, and the combat panel once a fight
// starts. Mirrors the JS's #tab-hunt content (dungeon tabs + huntButtons +
// bossSection + combatPanel), scoped to the parts covered above.
// ---------------------------------------------------------------------

static void DrawHuntScreen(GameState& s, int screenW, int screenH) {
    // HP bar (always visible on this screen, like the Character tab's HP bar)
    DrawUIText(TextFormat("HP: %d / %d", s.hp, s.maxHp), 20, 116, 16, kColorText);
    Rectangle hpBg = { 20, 138, 200, 12 };
    DrawRectangleRec(hpBg, Fade(BLACK, 0.25f));
    float hpPct = std::clamp((float)s.hp / s.maxHp, 0.0f, 1.0f);
    DrawRectangleRec({ hpBg.x, hpBg.y, hpBg.width * hpPct, hpBg.height },
                       hpPct > 0.3f ? Color{ 63, 94, 63, 255 } : Color{ 122, 46, 46, 255 });

    // Equipped gear summary — now driven by real crafted items (equip from the Craft
    // screen). Power/defense computed the same way as CombatPower()/TotalDefense().
    std::string weaponLine = s.equipped.rightHand ? s.equipped.rightHand->name : "None (unarmed)";
    DrawUIText(("Weapon: " + weaponLine).c_str(), 240, 116, 13, DARKGRAY);
    DrawUIText(TextFormat("Power: %d   Defense: %d", CombatPower(s), TotalDefense(s)), 240, 134, 13, DARKGRAY);
    if (!s.equipped.rightHand && !s.equipped.leftHand)
        DrawUIText("Craft or equip gear on the Craft tab.", 240, 152, 13, Fade(DARKGRAY, 0.8f));

    // --- Corpses waiting to be skinned (leather/gold sit here until skinned — see
    // SkinCorpse()). Shown above the combat/dungeon panel, on both branches. ---
    int corpseBandHeight = 0;
    if (!s.corpses.empty()) {
        int y = 170;
        DrawUIText(TextFormat("Skinning: %.1f", s.skinning), 20, y, 12, kColorAccent);
        y += 16;
        int shown = std::min((int)s.corpses.size(), 3);
        for (int i = 0; i < shown; i++) {
            const Corpse& c = s.corpses[i];
            std::string line = c.monsterName + " corpse (" + std::to_string(c.baseLeather) +
                                 " leather, " + std::to_string(c.gold) + "g)";
            DrawUIText(line.c_str(), 20, y + 4, 12, kColorText);
            if (Button({ 320.0f, (float)y, 100, 22 }, "Skin", true)) SkinCorpse(s, i);
            y += 26;
        }
        if ((int)s.corpses.size() > shown) {
            DrawUIText(TextFormat("+%d more corpse(s)", (int)s.corpses.size() - shown), 20, y, 13, DARKGRAY);
            y += 18;
        }
        corpseBandHeight = y - 170 + 8;
    }

    // --- If mid-combat, show the combat panel instead of the dungeon list ---
    if (s.combat.has_value()) {
        CombatState& c = *s.combat;
        int y = 175 + corpseBandHeight;
        DrawUIText(("Fighting: " + c.monster.name + (c.monster.isBoss ? " (Boss)" : "")).c_str(),
                  20, y, 18, kColorHeading);
        y += 26;
        DrawUIText(TextFormat("Monster HP: %d / %d", c.monsterHP, c.monsterMaxHP), 20, y, 14, DARKGRAY);
        Rectangle mhpBg = { 20, (float)(y + 20), 300, 12 };
        DrawRectangleRec(mhpBg, Fade(BLACK, 0.25f));
        float mhpPct = std::clamp((float)c.monsterHP / c.monsterMaxHP, 0.0f, 1.0f);
        DrawRectangleRec({ mhpBg.x, mhpBg.y, mhpBg.width * mhpPct, mhpBg.height }, Color{ 122, 46, 46, 255 });
        y += 36;

        // Combat arena — the animated knight (see "Combat sprite animations" above)
        // vs. the monster, replacing what used to be blank space here with actual
        // visual feedback for every action taken. Only the Sunken Crypt (dungeonIdx 2)
        // has a matching animated monster (Skeleton Warrior) so far — everywhere else
        // still shows the monster's existing static icon.
        float arenaCenterY = (float)y + 42.0f;
        DrawSpriteFrame(KnightSheetFor(c.anim), c.animTime, { 90, arenaCenterY }, 84.0f);
        if (c.dungeonIdx == 2 && g_assets.skeletonIdle.ok) {
            DrawSpriteFrame(MonsterSheetFor(c.monsterAnim), c.monsterAnimTime, { 390, arenaCenterY }, 84.0f);
        } else if (c.monster.icon || c.monster.isBoss) {
            const Texture2D* monsterTex = c.monster.icon ? c.monster.icon : BossFamilyTexture(c.dungeonIdx);
            if (monsterTex) DrawIconCentered(*monsterTex, { 390, arenaCenterY }, 84.0f, WHITE);
            else DrawCircleV({ 390, arenaCenterY }, 40.0f, Fade(Color{ 122, 46, 46, 255 }, 0.5f));
        } else if (const DirSpriteSheet* sheet = MonsterFamilySheet(c.dungeonIdx); sheet && sheet->ok) {
            // Static Idle frame, facing Down — this turn-based panel has no movement/
            // facing concept of its own, unlike the live Wilderness/dungeon fights.
            Rectangle src = ActorSrcRect(*sheet, { 0, 1 }, ActorAnim::Idle, s.worldTime);
            DrawIconCenteredRect(sheet->tex, src, { 390, arenaCenterY }, 84.0f, WHITE);
        } else {
            DrawCircleV({ 390, arenaCenterY }, 40.0f, Fade(Color{ 122, 46, 46, 255 }, 0.5f));
        }
        y += 90;

        if (Pet* pet = ActivePet(s)) {
            DrawUIText(TextFormat("%s: %.0f / %.0f HP", pet->name.c_str(), pet->hp, pet->maxHp), 20, y, 12,
                       Color{ 63, 94, 63, 255 });
        }
        y += 14;

        if (Button({ 20, (float)y, 120, 36 }, "Attack [A]", true)) ResolveCombatRound(s);
        if (Button({ 150, (float)y, 100, 36 }, "Flee [F]", true)) FleeCombat(s);
        if (Button({ 260, (float)y, 110, 36 }, TextFormat("Bandage (%d) [B]", s.bandages), s.bandages > 0))
            UseBandageInCombat(s);
        y += 44;

        DrawUIText(TextFormat("Mana: %.0f / %.0f   Reagents: %d", s.mana, MaxMana(s), s.reagents),
                   20, y, 12, DARKGRAY);
        y += 16;

        // Scrollable list of known Offensive/Utility(heal) spells — mirrors the JS's
        // knownOffensiveSpells/knownHealSpells filters (magery >= spell.minSkill).
        int spellListTop = y;
        int spellListHeight = 90;
        Rectangle spellArea = { 0, (float)spellListTop, (float)screenW, (float)spellListHeight };
        c.spellScroll -= ScrollDelta(spellArea);

        std::vector<int> knownIdx;
        for (size_t i = 0; i < kSpells.size(); i++) {
            const Spell& sp = kSpells[i];
            if ((sp.type == SpellType::Offensive || sp.type == SpellType::Utility) && EffectiveSkill(s, &GameState::magery) >= sp.minSkill)
                knownIdx.push_back((int)i);
        }
        float maxSpellScroll = std::max(0.0f, (float)knownIdx.size() * 24.0f - spellListHeight);
        c.spellScroll = std::clamp(c.spellScroll, 0.0f, maxSpellScroll);

        BeginScissorMode(0, spellListTop, screenW, spellListHeight);
        if (knownIdx.empty()) {
            DrawUIText("No spells known yet — practice on the Magic tab.", 20, spellListTop + 4, 13, DARKGRAY);
        }
        for (size_t row = 0; row < knownIdx.size(); row++) {
            int idx = knownIdx[row];
            const Spell& sp = kSpells[idx];
            float rowY = spellListTop + (float)row * 24 - c.spellScroll;
            if (rowY < spellListTop - 24 || rowY > spellListTop + spellListHeight) continue;
            std::string line = TextFormat("%s (%dmp/%dreg, %.0f%%)", sp.name.c_str(), sp.manaCost,
                                            sp.reagentCost, SpellSuccessChance(s, sp));
            if (const Texture2D* icon = SpellIcon(idx))
                DrawIconCentered(*icon, { 30, rowY + 10 }, 20.0f, WHITE);
            DrawUIText(line.c_str(), 44, (int)rowY + 4, 13, kColorText);
            bool affordable = s.mana >= sp.manaCost && s.reagents >= sp.reagentCost;
            if (Button({ (float)(screenW - 80), rowY, 60, 20 }, "Cast", affordable)) {
                if (sp.type == SpellType::Offensive) CastOffensiveSpell(s, idx);
                else CastHealSpell(s, idx);
            }
        }
        EndScissorMode();
        y = spellListTop + spellListHeight + 6;

        // Throw damage potions — a free action (see ThrowExplosionPotion(): no
        // monster counter-attack follows), so just one compact row is enough.
        for (size_t i = 0; i < s.potions.size(); i++) {
            if (s.potions[i].effect != "damage") continue;
            const PotionStack& p = s.potions[i];
            std::string line = "Throw " + p.name + " x" + std::to_string(p.count);
            DrawUIText(line.c_str(), 20, y + 4, 13, kColorText);
            if (Button({ (float)(screenW - 80), (float)y, 60, 20 }, "Throw", true)) ThrowExplosionPotion(s, (int)i);
            y += 24;
            break; // one row is enough on this cramped a panel; brew fewer types at once
        }

        DrawUIText("Combat log:", 20, y, 13, kColorAccent);
        y += 18;
        for (auto& line : c.log) {
            DrawUIText(line.c_str(), 20, y, 12, DARKGRAY);
            y += 16;
        }
        return;
    }

    // --- Gray Bloodstained encounter: a real choice, not a combat panel ---
    if (s.grayEncounter.has_value()) {
        const GameState::GrayEncounter& enc = *s.grayEncounter;
        int y = 175 + corpseBandHeight;
        DrawUIText(("Mercenary contract: " + enc.name + " (level " + std::to_string(enc.level) + ")").c_str(),
                   20, y, 16, kColorHeading);
        y += 28;
        if (enc.canSteal) {
            DrawUIText(TextFormat("Carrying about %d gold.", enc.previewGold), 20, y, 13, DARKGRAY);
            y += 24;
            if (Button({ 20, (float)y, 100, 36 }, "Steal", true)) GrayStealChoice(s);
            if (Button({ 130, (float)y, 100, 36 }, "Fight", true)) GrayFightChoice(s);
        } else {
            DrawUIText(TextFormat("Snooping: %.1f   Snoop chance: %.0f%%", s.snooping, SnoopChance(s)),
                       20, y, 13, DARKGRAY);
            y += 24;
            if (Button({ 20, (float)y, 100, 36 }, "Snoop", true)) GraySnoopChoice(s);
            if (Button({ 130, (float)y, 100, 36 }, "Fight", true)) GrayFightChoice(s);
        }
        return;
    }

    // --- Dungeons / Bloodstained Road sub-tabs ---
    // Bloodstained Road hidden 2026-09-23 at Mark's request ("hide the bloodstained
    // road menu for now") — a single flip-back switch, not a deletion, same pattern as
    // kAmbushSystemEnabled. Forces huntSubView back to Dungeons and skips drawing the
    // tab bar entirely (rather than just disabling the second tab) since with only one
    // real destination left, a tab bar with one tab would be more confusing than none.
    static const bool kBloodstainedRoadEnabled = false;
    int subY = 175 + corpseBandHeight;
    if (!kBloodstainedRoadEnabled) {
        s.huntSubView = 0;
    } else {
        Rectangle dungeonSubTab = { 20, (float)subY, 100, 26 };
        Rectangle bloodSubTab = { 126, (float)subY, 150, 26 };
        DrawRectangleRounded(dungeonSubTab, 0.3f, 6, s.huntSubView == 0 ? kColorSlate : Fade(GRAY, 0.3f));
        DrawUIText("Dungeons", (int)dungeonSubTab.x + 14, (int)dungeonSubTab.y + 6, 13, BLACK);
        DrawRectangleRounded(bloodSubTab, 0.3f, 6, s.huntSubView == 1 ? kColorSlate : Fade(GRAY, 0.3f));
        DrawUIText("Bloodstained Road", (int)bloodSubTab.x + 6, (int)bloodSubTab.y + 6, 12, BLACK);
        if (CheckCollisionPointRec(GetMousePosition(), dungeonSubTab) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) s.huntSubView = 0;
        if (CheckCollisionPointRec(GetMousePosition(), bloodSubTab) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) s.huntSubView = 1;
    }

    if (s.huntSubView == 1) {
        // --- Bloodstained Road: one walkable node per path, target updates live ---
        int y = subY + 36;
        DrawUIText("Always-open ladders — walk up and press [E] to fight your way up.", 20, y, 13, Fade(DARKGRAY, 0.8f));
        y += 16;

        std::string nearestKey;
        float nearestDist = 1e9f;
        for (int i = 0; i < 3; i++) {
            float d = Dist(s.bloodstainedPlayerPos, BloodstainedPathNodePos(i));
            if (d < nearestDist) { nearestDist = d; nearestKey = std::to_string(i); }
        }
        bool inRange = nearestDist < kNodeRadius + kInteractRange;

        UpdatePlayerMovement(s.bloodstainedPlayerPos, s.playerFacing, GetFrameTime());
        for (int i = 0; i < 3; i++)
            ResolveCircleCollision(s.bloodstainedPlayerPos, kPlayerRadius, BloodstainedPathNodePos(i), kNodeRadius);
        s.bloodstainedPlayerPos = ClampToWorld(s.bloodstainedPlayerPos, kPlayerEdgeMargin);
        if (inRange && IsKeyPressed(KEY_E)) FightBloodstainedTier(s, std::stoi(nearestKey));

        Rectangle roadViewport = { kViewport.x, (float)y + 6, kViewport.width, kViewport.y + kViewport.height - (y + 6) };
        BeginScissorMode((int)roadViewport.x, (int)roadViewport.y, (int)roadViewport.width, (int)roadViewport.height);
        DrawTiledGround(g_assets.groundDirtOk ? &g_assets.groundDirt : nullptr, roadViewport,
                          CameraTopLeft(s.bloodstainedPlayerPos), 48.0f, Color{ 90, 70, 55, 255 }); // packed dirt road
        Vector2 camera = CameraTopLeft(s.bloodstainedPlayerPos);
        for (int i = 0; i < 3; i++) {
            const BloodstainedPathDef& path = kBloodstainedPaths[i];
            DungeonMonster target = BloodstainedTargetFor(s, i);
            const BloodstainedTier& tier = path.tiers[s.bloodstainedProgress[i]];
            Vector2 screenPos = WorldToScreen(BloodstainedPathNodePos(i), camera);
            bool near = nearestKey == std::to_string(i) && inRange;
            std::string sub = TextFormat("%s lvl %d - %.0f%%%s", target.name.c_str(), target.level,
                                           WinChancePreview(s, target.level), tier.isBoss ? " BOSS" : "");
            DrawWorldNode(screenPos, kNodeRadius, path.color, path.name, near, sub);
            if (s.bloodstainedBossDefeated[i]) {
                std::string loopNote = "x" + std::to_string(s.bloodstainedLoop[i]);
                int lw = MeasureUIText(loopNote.c_str(), 12);
                DrawUIText(loopNote.c_str(), (int)screenPos.x - lw / 2, (int)screenPos.y - (int)kNodeRadius - 14, 12, kColorSlate);
            }
        }
        std::string prompt;
        if (inRange) {
            int i = std::stoi(nearestKey);
            prompt = (i == kPathGray ? "[E] Approach " : "[E] Fight ") + kBloodstainedPaths[i].name;
        }
        DrawPlayer(s, WorldToScreen(s.bloodstainedPlayerPos, camera), s.playerFacing, prompt);
        EndScissorMode();
        DrawVirtualJoystick();
        if (inRange && DrawInteractButton(prompt)) FightBloodstainedTier(s, std::stoi(nearestKey));
        return;
    }

    // --- Dungeon tabs ---
    int y = subY + 36;
    DrawUIText("Choose a dungeon", 20, y, 14, kColorAccent);
    y += 20;
    float tabX = 20;
    for (size_t i = 0; i < kDungeons.size(); i++) {
        bool sel = s.selectedDungeon.has_value() && *s.selectedDungeon == (int)i;
        float w = (float)MeasureUIText(kDungeons[i].name.c_str(), 12) + 20;
        if (tabX + w > screenW - 20) { tabX = 20; y += 34; }
        Rectangle r = { tabX, (float)y, w, 28 };
        DrawRectangleRounded(r, 0.3f, 6, sel ? kColorSlate : Fade(GRAY, 0.3f));
        DrawRectangleRoundedLines(r, 0.3f, 6, Fade(BLACK, 0.4f));
        int tw = MeasureUIText(kDungeons[i].name.c_str(), 12);
        DrawUIText(kDungeons[i].name.c_str(), (int)(r.x + (r.width - tw) / 2), (int)(r.y + 7), 12, BLACK);
        if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (!s.selectedDungeon.has_value() || *s.selectedDungeon != (int)i)
                s.dungeonPlayerPos = { 900, 1300 }; // fresh spawn point when entering a (different) dungeon
            s.selectedDungeon = (int)i;
        }
        tabX += w + 8;
    }
    y += 44;

    if (!s.selectedDungeon.has_value()) {
        DrawUIText("Select a dungeon to see its monsters.", 20, y, 13, DARKGRAY);
        return;
    }

    const DungeonDef& dungeon = kDungeons[*s.selectedDungeon];
    DrawUIText(dungeon.theme.c_str(), 20, y, 12, DARKGRAY);
    y += 8;

    // Mana bar — only while actually engaged in a live fight; HP is already always
    // shown at the top of this screen (see the header above), so only Mana is missing.
    // Safe to draw here (unlike Wilderness's equivalent) since this whole header
    // region sits above where the world/ground actually renders on this screen.
    if (s.dungeonEngaged.has_value()) {
        y += 6;
        DrawUIText(TextFormat("Mana: %.0f / %.0f", s.mana, MaxMana(s)), 20, y, 13, kColorText);
        Rectangle manaBg = { 20, (float)y + 18, 140, 9 };
        DrawRectangleRec(manaBg, Fade(BLACK, 0.25f));
        float manaPct = std::clamp(s.mana / MaxMana(s), 0.0f, 1.0f);
        DrawRectangleRec({ manaBg.x, manaBg.y, manaBg.width * manaPct, manaBg.height }, Color{ 63, 82, 122, 255 });
        y += 34;
    }

    // --- Explorable dungeon arena: walk up to a monster and press E to fight ---
    int xp = s.dungeonXP[*s.selectedDungeon];
    bool bossUnlocked = xp >= dungeon.bossUnlockXp;

    // The engaged monster (if any) is drawn/handled separately below at its live
    // position with an HP bar, same as Wilderness — exclude it from this search.
    bool wasDungeonEngaged = s.dungeonEngaged.has_value();
    std::string nearestKey; // "0".."4" for regular monsters, "boss"
    float nearestDist = 1e9f;
    bool nearestIsBoss = false;
    bool nearestIsExit = false;
    for (int i = 0; i < 5; i++) {
        if (wasDungeonEngaged && !s.dungeonEngaged->isBoss && s.dungeonEngaged->monsterIdx == i) continue;
        float d = Dist(s.dungeonPlayerPos, DungeonMonsterLivePos(*s.selectedDungeon, i, s.worldTime));
        if (d < nearestDist) { nearestDist = d; nearestKey = std::to_string(i); nearestIsBoss = false; nearestIsExit = false; }
    }
    if (bossUnlocked && !(wasDungeonEngaged && s.dungeonEngaged->isBoss)) {
        float d = Dist(s.dungeonPlayerPos, DungeonMonsterLivePos(*s.selectedDungeon, 5, s.worldTime));
        if (d < nearestDist) { nearestDist = d; nearestIsBoss = true; nearestIsExit = false; }
    }
    // Same spot every dungeon spawns you at ({900,1300}, set on entry above and on the
    // Wilderness's physical entrances) — guaranteed floor in every kDungeonRoomLayouts
    // hub room since that's already where you're standing the moment you walk in.
    static const Vector2 kDungeonExitPos = { 900, 1300 };
    {
        float d = Dist(s.dungeonPlayerPos, kDungeonExitPos);
        if (d < nearestDist) { nearestDist = d; nearestIsBoss = false; nearestIsExit = true; }
    }
    bool inRange = nearestDist < kNodeRadius + kInteractRange;

    // --- Live dungeon combat (2026-09-22) — exact port of Wilderness's live-combat
    // pattern (see DrawWildernessScreen's tryEngageWildMonster/updateEngagedMonsterAI/
    // trySwingAtEngagedMonster/tryCastSpellAtEngagedMonster) onto dungeon monsters.
    // Reuses the same screen-agnostic math (MonsterHitChance/SpellSuccessChance/
    // SpellPowerFor/ApplySpellTraining/LiveApplyWeaponTraining/PlayerSwingCooldown) and
    // the same tuning constants (kWildMeleeRange etc. — not Wilderness-specific despite
    // the name). Simplification accepted deliberately, matching the plan: the chase AI
    // moves in a straight line toward the player with no room-wall awareness — dungeon
    // rooms are generous (160+ units) relative to the leash range, so this reads fine
    // without needing real pathfinding.
    auto tryEngageDungeonMonster = [&](int monsterIdx, bool isBoss) {
        const DungeonMonster& m = isBoss ? dungeon.boss : dungeon.monsters[monsterIdx];
        GameState::ActiveDungeonMonster am;
        am.monsterIdx = monsterIdx;
        am.isBoss = isBoss;
        am.pos = DungeonMonsterLivePos(*s.selectedDungeon, isBoss ? 5 : monsterIdx, s.worldTime); // wherever it wandered to, no snap
        am.spawnPos = DungeonMonsterNodePos(*s.selectedDungeon, isBoss ? 5 : monsterIdx);
        am.maxHp = std::max(1.0f, m.level * 3.0f);
        am.hp = am.maxHp;
        s.dungeonEngaged = am;
        s.logLine = "You engage the " + m.name + "!";
    };

    auto tryDungeonInteract = [&]() {
        if (nearestIsExit) s.screen = Screen::Wilderness;
        else if (nearestIsBoss) tryEngageDungeonMonster(5, true);
        else tryEngageDungeonMonster(std::stoi(nearestKey), false);
    };

    auto updateEngagedDungeonMonsterAI = [&]() {
        if (!s.dungeonEngaged.has_value()) return;
        GameState::ActiveDungeonMonster& am = *s.dungeonEngaged;
        const DungeonMonster& m = am.isBoss ? dungeon.boss : dungeon.monsters[am.monsterIdx];
        float dtF = GetFrameTime();

        float distNow = Dist(am.pos, s.dungeonPlayerPos);
        if (distNow > kWildMeleeRange) {
            Vector2 dir = { s.dungeonPlayerPos.x - am.pos.x, s.dungeonPlayerPos.y - am.pos.y };
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.0001f) {
                dir.x /= len; dir.y /= len;
                Vector2 newPos = { am.pos.x + dir.x * kWildMonsterChaseSpeed * dtF,
                                     am.pos.y + dir.y * kWildMonsterChaseSpeed * dtF };
                float fsx = newPos.x - am.spawnPos.x, fsy = newPos.y - am.spawnPos.y;
                float fsLen = std::sqrt(fsx * fsx + fsy * fsy);
                if (fsLen > kWildMonsterLeashRange) {
                    newPos.x = am.spawnPos.x + fsx / fsLen * kWildMonsterLeashRange;
                    newPos.y = am.spawnPos.y + fsy / fsLen * kWildMonsterLeashRange;
                }
                am.pos = newPos;
            }
        }

        if (Dist(am.pos, s.dungeonPlayerPos) > kWildDisengageRange) {
            s.logLine = m.name + " loses interest.";
            s.dungeonEngaged.reset();
            return;
        }

        if (am.monsterAttackCooldown > 0) am.monsterAttackCooldown -= dtF;
        if (am.playerAttackCooldown > 0) am.playerAttackCooldown -= dtF;
        if (am.playerSpellCooldown > 0) am.playerSpellCooldown -= dtF;
        if (am.swingEffectTimer > 0) am.swingEffectTimer -= dtF;
        if (am.castEffectTimer > 0) am.castEffectTimer -= dtF;

        bool inMelee = Dist(am.pos, s.dungeonPlayerPos) < kWildMeleeRange;
        if (inMelee && am.monsterAttackCooldown <= 0) {
            am.monsterAttackCooldown = kWildMonsterAttackCooldown;
            std::string mname = m.name;
            if (RandUnit() * 100.0f < MonsterHitChance(s)) {
                float raw = m.level * (0.8f + RandUnit() * 0.6f);
                int dmg = std::max(1, (int)std::round(raw - TotalDefense(s) * 0.3f));
                s.hp -= dmg;
                s.logLine = "The " + mname + " hits you for " + std::to_string(dmg) + " damage";
            } else {
                s.logLine = "The " + mname + " misses";
            }
            if (s.hp <= 0) { EndDungeonMonsterLoss(s, mname); return; }
        }

        // AI companion's turn — same autonomous cooldown-driven action as Wilderness
        // (2026-09-22, "AI players" plan Part 2).
        if (s.companionAttackCooldown > 0) s.companionAttackCooldown -= dtF;
        if (ActivePet(s) && s.companionAttackCooldown <= 0) {
            s.companionAttackCooldown = kCompanionAttackCooldown;
            std::string mname = m.name; int mgold = m.baseGold, mleather = m.baseLeather;
            bool wasBoss = am.isBoss; int dungeonIdx = *s.selectedDungeon; int level = m.level;
            ResolvePetTurnLive(s, am.hp, level);
            if (am.hp <= 0) { EndDungeonMonsterWin(s, dungeonIdx, wasBoss, mname, level, mgold, mleather); return; }
        }
    };

    auto trySwingAtEngagedDungeonMonster = [&]() {
        if (!s.dungeonEngaged.has_value()) return;
        GameState::ActiveDungeonMonster& am = *s.dungeonEngaged;
        const DungeonMonster& m = am.isBoss ? dungeon.boss : dungeon.monsters[am.monsterIdx];
        if (Dist(am.pos, s.dungeonPlayerPos) >= kWildMeleeRange || am.playerAttackCooldown > 0) return;
        am.playerAttackCooldown = PlayerSwingCooldown(s);
        am.swingEffectTimer = kSwingEffectDuration;
        int power = CombatPower(s);
        float weaponSkillBonus = EffectiveSkill(s, ActiveWeaponSkillField(s)) * 0.2f;
        float hitChance = std::clamp(50.0f + (power - m.level) * 4.0f + weaponSkillBonus, 5.0f, 95.0f);
        LiveApplyWeaponTraining(s);
        std::string mname = m.name; int mgold = m.baseGold, mleather = m.baseLeather;
        bool wasBoss = am.isBoss; int dungeonIdx = *s.selectedDungeon; int level = m.level;
        if (RandUnit() * 100.0f < hitChance) {
            int dmg = std::max(1, (int)std::round(power * (0.85f + RandUnit() * 0.3f)));
            am.hp -= dmg;
            s.logLine = "You hit the " + mname + " for " + std::to_string(dmg) + " damage";
            if (am.hp <= 0) EndDungeonMonsterWin(s, dungeonIdx, wasBoss, mname, level, mgold, mleather);
        } else {
            s.logLine = "Your attack misses";
        }
    };

    auto tryCastSpellAtEngagedDungeonMonster = [&](int spellIdx) {
        if (!s.dungeonEngaged.has_value()) return;
        GameState::ActiveDungeonMonster& am = *s.dungeonEngaged;
        if (am.playerSpellCooldown > 0) return;
        const Spell& spell = kSpells[spellIdx];
        if (spell.type != SpellType::Offensive) return;
        if (s.mana < spell.manaCost || s.reagents < kLiveCombatReagentCost) {
            s.logLine = "Not enough mana or reagents for " + spell.name + ".";
            return;
        }
        am.playerSpellCooldown = kWildSpellCastCooldown;
        am.castEffectTimer = kCastEffectDuration;
        s.mana -= spell.manaCost;
        s.reagents -= kLiveCombatReagentCost;
        const DungeonMonster& m = am.isBoss ? dungeon.boss : dungeon.monsters[am.monsterIdx];
        std::string mname = m.name; int mgold = m.baseGold, mleather = m.baseLeather;
        bool wasBoss = am.isBoss; int dungeonIdx = *s.selectedDungeon; int level = m.level;
        std::string note;
        bool success = RandUnit() * 100.0f < SpellSuccessChance(s, spell);
        ApplySpellTraining(s, spell, note);
        if (success) {
            int dmg = std::max(1, (int)std::round(SpellPowerFor(s, spell) * (0.85f + RandUnit() * 0.3f)));
            am.hp -= dmg;
            s.logLine = spell.name + " hits the " + mname + " for " + std::to_string(dmg) + " damage" + note;
            if (am.hp <= 0) EndDungeonMonsterWin(s, dungeonIdx, wasBoss, mname, level, mgold, mleather);
        } else {
            s.logLine = spell.name + " fizzles!" + note;
        }
    };

    Vector2 prevDungeonPos = s.dungeonPlayerPos; // wall-slide against this if the move ends in a wall
    UpdatePlayerMovement(s.dungeonPlayerPos, s.playerFacing, GetFrameTime(), kDungeonWorldSize);
    if (ActivePet(s)) UpdateCompanionFollow(s, s.dungeonPlayerPos, s.playerFacing, GetFrameTime());
    for (int i = 0; i < 5; i++) {
        // The engaged one collides against its live position (below); the other 4
        // wander (DungeonMonsterLivePos) and auto-engage the player on contact — same
        // bump-to-engage treatment as Wilderness. Walking up and pressing E still works
        // too, via tryDungeonInteract.
        if (wasDungeonEngaged && !s.dungeonEngaged->isBoss && s.dungeonEngaged->monsterIdx == i) continue;
        Vector2 livePos = DungeonMonsterLivePos(*s.selectedDungeon, i, s.worldTime);
        if (!s.dungeonEngaged.has_value() && Dist(s.dungeonPlayerPos, livePos) < kPlayerRadius + kNodeRadius * 0.8f)
            tryEngageDungeonMonster(i, false);
        ResolveCircleCollision(s.dungeonPlayerPos, kPlayerRadius, livePos, kNodeRadius * 0.8f);
    }
    if (!(wasDungeonEngaged && s.dungeonEngaged->isBoss)) {
        Vector2 bossLivePos = DungeonMonsterLivePos(*s.selectedDungeon, 5, s.worldTime);
        if (bossUnlocked && !s.dungeonEngaged.has_value() && Dist(s.dungeonPlayerPos, bossLivePos) < kPlayerRadius + kNodeRadius)
            tryEngageDungeonMonster(5, true);
        ResolveCircleCollision(s.dungeonPlayerPos, kPlayerRadius, bossLivePos, kNodeRadius); // boss, locked or not
    }
    if (wasDungeonEngaged)
        ResolveCircleCollision(s.dungeonPlayerPos, kPlayerRadius, s.dungeonEngaged->pos, kNodeRadius * 0.7f);
    ResolveCircleCollision(s.dungeonPlayerPos, kPlayerRadius, kDungeonExitPos, kNodeRadius * 0.6f);
    s.dungeonPlayerPos = ClampToWorld(s.dungeonPlayerPos, kPlayerEdgeMargin, kDungeonWorldSize);
    if (!DungeonIsFloor(*s.selectedDungeon, s.dungeonPlayerPos)) {
        // Moving diagonally into a wall used to revert the whole frame's move, which
        // could feel like getting stuck even right next to open floor. Try keeping just
        // the X move or just the Y move (i.e. sliding along whichever wall you clipped)
        // before giving up and reverting entirely.
        Vector2 slideX = { s.dungeonPlayerPos.x, prevDungeonPos.y };
        Vector2 slideY = { prevDungeonPos.x, s.dungeonPlayerPos.y };
        if (DungeonIsFloor(*s.selectedDungeon, slideX)) s.dungeonPlayerPos = slideX;
        else if (DungeonIsFloor(*s.selectedDungeon, slideY)) s.dungeonPlayerPos = slideY;
        else s.dungeonPlayerPos = prevDungeonPos;
    }

    // AI reacts to this frame's final (post-collision) player position, same ordering
    // as Wilderness. Melee is auto-continuous once engaged — no button needed.
    updateEngagedDungeonMonsterAI();
    if (wasDungeonEngaged) {
        trySwingAtEngagedDungeonMonster();
    } else if (inRange && IsKeyPressed(KEY_E)) {
        tryDungeonInteract();
    }

    Rectangle arenaViewport = { kViewport.x, (float)y + 14, kViewport.width, kViewport.y + kViewport.height - (y + 14) };
    BeginScissorMode((int)arenaViewport.x, (int)arenaViewport.y, (int)arenaViewport.width, (int)arenaViewport.height);
    Vector2 camera = CameraTopLeft(s.dungeonPlayerPos, kDungeonWorldSize);

    const Texture2D* wallTex = ThemedDungeonWall(*s.selectedDungeon);
    // Real rooms, not an open arena — see kDungeonRoomLayouts. Wall texture fills the
    // whole viewport as solid rock, then each room/corridor rectangle punches a
    // floor-textured hole in it, in one pass with no depth-buffer or masking trickery
    // (floor is simply drawn on top). Each dungeon keeps its own themed floor/wall art
    // (lava/volcanic for Emberveil, orc-camp for Bloodtusk, tomb for the Crypt, rock for
    // Wyrmscar), falling back to a generic dungeon look if a themed texture is missing.
    // Emberveil's wall and floor art are both mottled red/black lava-rock crops that read
    // as nearly identical — Mark could tell monsters apart fine after the plate fix, but
    // not walls from floor. Rather than source new art, darken just the wall tile via a
    // multiply tint so it reads as cooled obsidian rock against the floor's bright lava,
    // without touching the other 3 dungeons' walls.
    Color wallTint = (*s.selectedDungeon == 0) ? Color{ 95, 70, 65, 255 } : WHITE;
    DrawTiledGround(wallTex, arenaViewport, camera, 48.0f, Color{ 40, 40, 44, 255 }, wallTint);
    const Texture2D* floorTex = ThemedDungeonFloor(*s.selectedDungeon);
    for (const Rectangle& r : kDungeonRoomLayouts[*s.selectedDungeon])
        DrawTiledRect(floorTex, r, camera, 48.0f, Color{ 60, 50, 46, 255 });
    // The Sunken Crypt gets a water pool in its boss room — it's the one dungeon that's
    // actually a *flooded* tomb; see assets/dungeon_themed/sunkencrypt_water.png
    // (cropped from the same "Top down dungeon" pack's water-coast animation).
    if (*s.selectedDungeon == 2 && g_assets.sunkenCryptWaterOk) {
        DrawTiledRect(&g_assets.sunkenCryptWater, { 1260, 1260, 340, 340 }, camera, 32.0f, Color{ 55, 88, 143, 255 });
    }
    // Emberveil Hollow gets a few scattered braziers instead — a caustic elemental
    // hollow calls for fire, not a tiled floor overlay (this pack's fire art is a
    // standalone prop icon, not a floor texture like the water was); see
    // assets/dungeon_themed/emberveil_brazier.png.
    if (*s.selectedDungeon == 0 && g_assets.emberveilBrazierOk) {
        // {900,1300} dropped from this list — it's the shared dungeon spawn/exit point
        // (kDungeonExitPos below), and the brazier icon there collided with the new
        // exit node's icon+label.
        static const std::array<Vector2, 2> kBrazierSpots = {{ {900,740}, {1400,310} }};
        for (const Vector2& pos : kBrazierSpots)
            DrawIconCentered(g_assets.emberveilBrazier, WorldToScreen(pos, camera), 40.0f, WHITE);
    }
    // The Hollow Warrens gets its boss room floored with the fancy medallion-pattern
    // rug from the same "Dungeon Tileset" sheet the walls/floor came from (CC0, Buch on
    // OpenGameArt — see assets/dungeon/dungeon_tiles.png), plus a couple of scattered
    // lit torches along the shaft — same "tiled rect for a themed room, icons for scattered
    // props" split as the Sunken Crypt/Emberveil cases above.
    if (*s.selectedDungeon == 4 && g_assets.hollowWarrensRugOk) {
        DrawTiledRect(&g_assets.hollowWarrensRug, { 680, 240, 440, 400 }, camera, 48.0f, Color{ 40, 45, 60, 255 });
    }
    if (*s.selectedDungeon == 4 && g_assets.hollowWarrensTorchOk) {
        static const std::array<Vector2, 2> kWarrenTorchSpots = {{ {570,1030}, {1230,1030} }};
        for (const Vector2& pos : kWarrenTorchSpots)
            DrawIconCentered(g_assets.hollowWarrensTorch, WorldToScreen(pos, camera), 34.0f, WHITE);
    }

    // Dungeon monsters DO wander in place (DungeonMonsterLivePos already applies
    // MonsterWanderOffset, same as Town NPCs) — they just never had a facing to match
    // that motion until now (2026-09-23, "cheap wire-up" following the Carl-art
    // integration). WanderFacing is that motion's own analytical derivative, so this
    // costs nothing beyond what Town NPCs already do with it.
    const DirSpriteSheet* monsterSheet = MonsterFamilySheet(*s.selectedDungeon);
    for (int i = 0; i < 5; i++) {
        // The engaged slot is drawn separately below, at its live position with an HP
        // bar — same convention as Wilderness. Checks s.dungeonEngaged fresh (not
        // wasDungeonEngaged) since updateEngagedDungeonMonsterAI() above may have just
        // ended the fight this same frame.
        if (s.dungeonEngaged.has_value() && !s.dungeonEngaged->isBoss && s.dungeonEngaged->monsterIdx == i) continue;
        const DungeonMonster& m = dungeon.monsters[i];
        Vector2 screenPos = WorldToScreen(DungeonMonsterLivePos(*s.selectedDungeon, i, s.worldTime), camera);
        bool near = !nearestIsBoss && nearestKey == std::to_string(i) && inRange;
        std::string sub = TextFormat("lvl %d - %.0f%%", m.level, WinChancePreview(s, m.level));
        Rectangle monsterSrc{};
        if (monsterSheet && monsterSheet->ok) monsterSrc = ActorSrcRect(*monsterSheet, WanderFacing(i, s.worldTime), ActorAnim::Walk, s.worldTime);
        DrawWorldNode(screenPos, kNodeRadius * 0.8f, Color{ 122, 46, 46, 255 }, m.name, near, sub,
                       monsterSheet && monsterSheet->ok ? &monsterSheet->tex : nullptr, WHITE,
                       monsterSheet && monsterSheet->ok ? &monsterSrc : nullptr);
    }
    bool engagedIsBossNow = s.dungeonEngaged.has_value() && s.dungeonEngaged->isBoss;
    if (!engagedIsBossNow) {
        Vector2 bossScreenPos = WorldToScreen(DungeonMonsterLivePos(*s.selectedDungeon, 5, s.worldTime), camera);
        if (bossUnlocked) {
            const Texture2D* bossTex = BossFamilyTexture(*s.selectedDungeon);
            Rectangle bossFallbackSrc{};
            if (!bossTex && monsterSheet && monsterSheet->ok) bossFallbackSrc = ActorSrcRect(*monsterSheet, WanderFacing(5, s.worldTime), ActorAnim::Walk, s.worldTime);
            DrawWorldNode(bossScreenPos, kNodeRadius, kColorSlate, dungeon.boss.name, nearestIsBoss && inRange,
                           "BOSS lvl " + std::to_string(dungeon.boss.level),
                           bossTex ? bossTex : (monsterSheet && monsterSheet->ok ? &monsterSheet->tex : nullptr),
                           bossTex ? WHITE : kColorSlate, // distinct boss art if loaded; gold-tinted regular monster as fallback
                           bossTex ? nullptr : (monsterSheet && monsterSheet->ok ? &bossFallbackSrc : nullptr));
        } else {
            DrawWorldNode(bossScreenPos, kNodeRadius, Fade(GRAY, 0.6f), "???",
                           false, std::to_string(xp) + "/" + std::to_string(dungeon.bossUnlockXp) + " XP");
        }
    }
    {
        bool near = nearestIsExit && inRange;
        Vector2 screenPos = WorldToScreen(kDungeonExitPos, camera);
        DrawWorldNode(screenPos, kNodeRadius * 0.7f, kColorPanelBg, "Exit", near);
    }
    // Live, moving engaged monster + its own HP bar — same treatment as Wilderness's
    // s.wildEngaged draw block.
    if (s.dungeonEngaged.has_value()) {
        const GameState::ActiveDungeonMonster& am = *s.dungeonEngaged;
        const DungeonMonster& m = am.isBoss ? dungeon.boss : dungeon.monsters[am.monsterIdx];
        const Texture2D* bossTex = am.isBoss ? BossFamilyTexture(*s.selectedDungeon) : nullptr;
        const Texture2D* tex = bossTex ? bossTex : (monsterSheet && monsterSheet->ok ? &monsterSheet->tex : nullptr);
        // Faces the player during the actual fight — same "face the player directly"
        // trick Wilderness's engaged-monster block already uses, rather than the
        // wander-derived facing the not-yet-engaged loop above uses.
        Rectangle engagedSrc{};
        if (!bossTex && monsterSheet && monsterSheet->ok) {
            Vector2 toPlayer = { s.dungeonPlayerPos.x - am.pos.x, s.dungeonPlayerPos.y - am.pos.y };
            float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
            Vector2 facing = len > 0.001f ? Vector2{ toPlayer.x / len, toPlayer.y / len } : Vector2{ 0, 1 };
            engagedSrc = ActorSrcRect(*monsterSheet, facing, ActorAnim::Idle, s.worldTime);
        }
        const Rectangle* srcRect = bossTex ? nullptr : (monsterSheet && monsterSheet->ok ? &engagedSrc : nullptr);
        Vector2 screenPos = WorldToScreen(am.pos, camera);
        bool inMelee = Dist(am.pos, s.dungeonPlayerPos) < kWildMeleeRange;
        DrawWorldNode(screenPos, am.isBoss ? kNodeRadius : kNodeRadius * 0.8f, Color{ 122, 46, 46, 255 }, m.name, inMelee, "", tex, WHITE, srcRect);
        float hpPct = std::clamp(am.hp / am.maxHp, 0.0f, 1.0f);
        Rectangle hpBg = { screenPos.x - 30, screenPos.y - kNodeRadius * 0.8f - 26, 60, 8 };
        DrawRectangleRec(hpBg, Fade(BLACK, 0.4f));
        DrawRectangleRec({ hpBg.x, hpBg.y, hpBg.width * hpPct, hpBg.height }, Color{ 122, 46, 46, 255 });
        DrawRectangleLinesEx(hpBg, 1.0f, Fade(RAYWHITE, 0.8f));
    }
    std::string prompt;
    if (s.dungeonEngaged.has_value()) {
        // Melee is automatic now — just naming who you're fighting, no button needed.
        const DungeonMonster& m = s.dungeonEngaged->isBoss ? dungeon.boss : dungeon.monsters[s.dungeonEngaged->monsterIdx];
        prompt = "Fighting " + m.name;
    } else if (inRange) {
        if (nearestIsExit) prompt = "[E] Leave dungeon";
        else prompt = nearestIsBoss ? "[E] Fight " + dungeon.boss.name
                                      : "[E] Fight " + dungeon.monsters[std::stoi(nearestKey)].name;
    }
    if (Pet* companion = ActivePet(s)) {
        Vector2 companionScreenPos = WorldToScreen(s.companionPos, camera);
        const DirSpriteSheet& sheet = WildCreatureSheetForRole(companion->role);
        if (sheet.ok) {
            Vector2 toPlayer = { s.dungeonPlayerPos.x - s.companionPos.x, s.dungeonPlayerPos.y - s.companionPos.y };
            float len = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
            Vector2 facing = len > 0.001f ? Vector2{ toPlayer.x / len, toPlayer.y / len } : Vector2{ 0, 1 };
            Rectangle src = ActorSrcRect(sheet, facing, ActorAnim::Walk, s.worldTime);
            DrawWorldNode(companionScreenPos, kNodeRadius * 0.5f, Color{ 63, 94, 63, 255 }, companion->name, false, "", &sheet.tex, WHITE, &src);
        } else {
            DrawWorldNode(companionScreenPos, kNodeRadius * 0.5f, Color{ 63, 94, 63, 255 }, companion->name, false);
        }
    }
    // Weapon swing / spell cast — same real Attack/Cast animation states as Wilderness
    // (see its call site's comment).
    ActorAnim dungeonCombatAnim = ActorAnim::Idle;
    if (s.dungeonEngaged.has_value()) {
        if (s.dungeonEngaged->swingEffectTimer > 0.0f) dungeonCombatAnim = ActorAnim::Attack;
        else if (s.dungeonEngaged->castEffectTimer > 0.0f) dungeonCombatAnim = ActorAnim::Cast;
    }
    DrawPlayer(s, WorldToScreen(s.dungeonPlayerPos, camera), s.playerFacing, prompt, 1.0f, dungeonCombatAnim);
    EndScissorMode();
    DrawVirtualJoystick();
    if (wasDungeonEngaged) {
        // Melee is fully automatic now — no interact button needed here anymore for it.
        DrawLiveCombatQuickItems(s);
    } else if (inRange && DrawInteractButton(prompt)) {
        tryDungeonInteract();
    }

    // Spell hotbar — only while actually engaged (2026-09-22 fix, same reason as
    // Wilderness: this spot overlaps the interact button used for "[E] Fight/Leave"
    // prompts when not engaged). Configuration lives on the Magic screen instead.
    // s.combatHotbar is shared across both screens either way, so a slot assigned
    // there works here too.
    if (wasDungeonEngaged) {
        float cd = s.dungeonEngaged->playerSpellCooldown;
        int tapped = DrawCombatHotbarRow(s, true, cd);
        if (tapped >= 0) {
            int spellIdx = s.combatHotbar[tapped];
            if (spellIdx >= 0) {
                const Spell& sp = kSpells[spellIdx];
                if (sp.type == SpellType::Offensive) tryCastSpellAtEngagedDungeonMonster(spellIdx);
                else CastLiveUtilitySpell(s, spellIdx);
            }
        }
    }
    DrawHotbarPicker(s, screenW, screenH);
}

// ---------------------------------------------------------------------
// Craft screen: pick a workshop (Smith/Carpenter/Tailor/Alchemy), scroll
// its recipe list, craft what you can afford and are skilled enough for.
// Weapon/armor recipes equip or sell from the backpack; Alchemy recipes
// brew potions into a separate potion pouch with Drink/Poison Weapon
// actions (Throw lives on the Hunt screen's combat panel, combat-only).
// ---------------------------------------------------------------------

// Small pill-button tab strip shared by the Craft/Buy toggle below and the new
// Provisioner screen's Buy/Sell toggle — same visual/hit-test pattern as the workshop
// tabs just below (filled kColorSlate when selected, faded gray otherwise), pulled out
// since both call sites need it now instead of just one.
static void DrawPillTabs(const std::vector<std::string>& labels, int* selected, float x, float y, float height) {
    float tabX = x;
    for (size_t i = 0; i < labels.size(); i++) {
        bool sel = *selected == (int)i;
        float w = (float)MeasureUIText(labels[i].c_str(), 12) + 20;
        Rectangle r = { tabX, y, w, height };
        DrawRectangleRounded(r, 0.3f, 6, sel ? kColorSlate : Fade(GRAY, 0.3f));
        DrawRectangleRoundedLines(r, 0.3f, 6, Fade(BLACK, 0.4f));
        int tw = MeasureUIText(labels[i].c_str(), 12);
        DrawUIText(labels[i].c_str(), (int)(r.x + (r.width - tw) / 2), (int)(r.y + 6), 12, BLACK);
        if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) *selected = (int)i;
        tabX += w + 8;
    }
}

static void DrawCraftScreen(GameState& s, int screenW, int screenH) {
    // Themed backdrop, keyed by which workshop is open — see DrawInteriorBackdrop.
    DrawInteriorBackdrop(g_assets.craftWallThemedOk[s.craftBuildingTab] ? &g_assets.craftWallThemed[s.craftBuildingTab] : nullptr,
                           g_assets.craftFloorThemedOk[s.craftBuildingTab] ? &g_assets.craftFloorThemed[s.craftBuildingTab] : nullptr,
                           screenW, screenH, 110);

    // Workshop tabs — all 4 craftable buildings now that Alchemy has recipes too.
    int y = 116;
    float tabX = 20;
    for (int i = 0; i < 4; i++) {
        const BuildingDef& b = kCraftBuildings[i];
        bool sel = s.craftBuildingTab == i;
        float w = (float)MeasureUIText(b.name.c_str(), 12) + 20;
        Rectangle r = { tabX, (float)y, w, 26 };
        DrawRectangleRounded(r, 0.3f, 6, sel ? kColorSlate : Fade(GRAY, 0.3f));
        DrawRectangleRoundedLines(r, 0.3f, 6, Fade(BLACK, 0.4f));
        int tw = MeasureUIText(b.name.c_str(), 12);
        DrawUIText(b.name.c_str(), (int)(r.x + (r.width - tw) / 2), (int)(r.y + 6), 12, BLACK);
        if (CheckCollisionPointRec(GetMousePosition(), r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s.craftBuildingTab = i;
            s.craftScroll = 0;
        }
        tabX += w + 8;
    }
    y += 34;

    const BuildingDef& b = kCraftBuildings[s.craftBuildingTab];
    bool isAlchemy = s.craftBuildingTab == 3;

    // Craft/Buy toggle, added 2026-09-21 — Buy sells the same recipes pre-made (always
    // Standard quality, gold only) instead of letting you craft your own; see
    // TryBuyPremadeItem/TryBuyPremadePotion. Reuses `b`/`isAlchemy` just resolved above.
    {
        int mode = s.craftModeTab;
        DrawPillTabs({ "Craft", "Buy" }, &mode, 20, (float)y, 26);
        s.craftModeTab = mode;
    }
    y += 34;
    if (s.craftModeTab == 1) {
        DrawInfoLine(TextFormat("Gold: %d", s.gold), 20, y, 13, kColorAccent);
        y += 20;
        int listTop = y;
        int listHeight = screenH - listTop - 40;
        Rectangle listArea = { 0, (float)listTop, (float)screenW, (float)listHeight };
        s.craftScroll -= ScrollDelta(listArea);
        float maxScroll = std::max(0.0f, (float)b.recipes.size() * 30.0f - listHeight);
        s.craftScroll = std::clamp(s.craftScroll, 0.0f, maxScroll);
        BeginScissorMode(0, listTop, screenW, listHeight);
        for (size_t i = 0; i < b.recipes.size(); i++) {
            const Recipe& r = b.recipes[i];
            float rowY = listTop + (float)i * 30 - s.craftScroll;
            if (rowY < listTop - 30 || rowY > listTop + listHeight) continue;
            int price = VendorPriceFor(r);
            std::string line = isAlchemy
                ? TextFormat("%s  (%s %d)", r.name.c_str(), r.category.c_str(), r.power)
                : TextFormat("Standard %s  (%d %s)", r.name.c_str(), r.power, r.type == ItemType::Armor ? "def" : "pwr");
            DrawUIText(line.c_str(), 20, (int)rowY + 6, 12, kColorText);
            std::string priceLabel = TextFormat("Buy (%dg)", price);
            if (Button({ (float)(screenW - 110), rowY, 90, 24 }, priceLabel, s.gold >= price)) {
                if (isAlchemy) TryBuyPremadePotion(s, (int)i);
                else TryBuyPremadeItem(s, s.craftBuildingTab, (int)i);
            }
        }
        EndScissorMode();
        return;
    }

    int buildingCap = b.levels[s.buildingLevel[s.craftBuildingTab] - 1].cap;
    float skillVal = s.buildingSkill[s.craftBuildingTab];
    DrawInfoLine(TextFormat("Skill: %.1f  (workshop cap: %d)", skillVal, buildingCap), 20, y, 13,
              kColorText);
    y += 20;

    // --- Scrollable recipe list ---
    int listTop = y;
    int listHeight = 200;
    Rectangle listArea = { 0, (float)listTop, (float)screenW, (float)listHeight };
    s.craftScroll -= ScrollDelta(listArea);
    float maxScroll = std::max(0.0f, (float)b.recipes.size() * 30.0f - listHeight);
    s.craftScroll = std::clamp(s.craftScroll, 0.0f, maxScroll);

    BeginScissorMode(0, listTop, screenW, listHeight);
    for (size_t i = 0; i < b.recipes.size(); i++) {
        const Recipe& r = b.recipes[i];
        float rowY = listTop + (float)i * 30 - s.craftScroll;
        if (rowY < listTop - 30 || rowY > listTop + listHeight) continue;

        bool skillOk = std::min(skillVal, (float)buildingCap) >= r.reqSkill;
        if (isAlchemy) {
            std::string line = TextFormat("%s  (req %d, %d reagents, %s %d)", r.name.c_str(), r.reqSkill,
                                            r.cost, r.category.c_str(), r.power);
            DrawUIText(line.c_str(), 20, (int)rowY + 6, 12, kColorText);
            bool canAfford = s.reagents >= r.cost;
            if (Button({ (float)(screenW - 90), rowY, 70, 24 }, "Brew", skillOk && canAfford))
                TryCraftPotion(s, (int)i);
        } else {
            std::string resName = b.resource == Resource::Wood ? "wood"
                                  : b.resource == Resource::Ore ? "ore"
                                  : b.resource == Resource::Leather ? "leather" : "";
            std::string line = TextFormat("%s  (req %d, %d %s, %d %s)", r.name.c_str(), r.reqSkill,
                                            r.cost, resName.c_str(), r.power, r.type == ItemType::Armor ? "def" : "pwr");
            DrawUIText(line.c_str(), 20, (int)rowY + 6, 12, kColorText);
            int haveResource = b.resource == Resource::Wood ? s.wood : b.resource == Resource::Ore ? s.ore : s.leather;
            bool canAfford = haveResource >= r.cost;
            bool roomInBackpack = (int)s.backpack.size() < BackpackCap(s);
            if (Button({ (float)(screenW - 90), rowY, 70, 24 }, "Craft", skillOk && canAfford && roomInBackpack))
                TryCraftItem(s, s.craftBuildingTab, (int)i);
        }
    }
    EndScissorMode();
    y = listTop + listHeight + 8;
    DrawInfoLine("Scroll to see more recipes.", 20, y, 13, Fade(DARKGRAY, 0.8f));
    y += 20;

    if (s.craftBuildingTab == 2) { // Tailor also crafts Bandages — consumable, no skill required
        DrawInfoLine(TextFormat("Bandages: %d", s.bandages), 20, y, 13, kColorAccent);
        if (Button({ (float)(screenW - 160), (float)y - 4, 140, 24 }, "Craft 5 (2 leather)", s.leather >= 2))
            CraftBandages(s);
        y += 28;
    }

    if (isAlchemy) {
        // --- Potion pouch: Drink (heal/stamina) or Poison Weapon, instead of backpack ---
        DrawInfoLine(TextFormat("Reagents: %d   Poisoning: %.1f", s.reagents, s.poisoning), 20, y, 13,
                   kColorAccent);
        y += 20;
        int pouchTop = y;
        int pouchHeight = screenH - pouchTop - 40;
        Rectangle pouchArea = { 0, (float)pouchTop, (float)screenW, (float)pouchHeight };
        s.backpackScroll -= ScrollDelta(pouchArea);
        float maxPouchScroll = std::max(0.0f, (float)s.potions.size() * 28.0f - pouchHeight);
        s.backpackScroll = std::clamp(s.backpackScroll, 0.0f, maxPouchScroll);

        BeginScissorMode(0, pouchTop, screenW, pouchHeight);
        if (s.potions.empty()) DrawUIText("No potions brewed yet.", 20, pouchTop + 4, 12, DARKGRAY);
        for (size_t i = 0; i < s.potions.size(); i++) {
            const PotionStack& p = s.potions[i];
            float rowY = pouchTop + (float)i * 28 - s.backpackScroll;
            if (rowY < pouchTop - 28 || rowY > pouchTop + pouchHeight) continue;
            std::string line = p.name + " x" + std::to_string(p.count);
            if (const Texture2D* icon = FindItemIconTexture(p.effect)) {
                DrawIconCentered(*icon, { 30, rowY + 10 }, 20.0f, WHITE);
                DrawUIText(line.c_str(), 44, (int)rowY + 6, 12, kColorText);
            } else {
                DrawUIText(line.c_str(), 20, (int)rowY + 6, 12, kColorText);
            }
            if (p.effect == "heal" || p.effect == "stamina") {
                if (Button({ (float)(screenW - 90), rowY, 70, 22 }, "Drink", true)) DrinkPotion(s, (int)i);
            } else if (p.effect == "poison") {
                if (Button({ (float)(screenW - 120), rowY, 100, 22 }, "Poison Wpn", true)) PoisonWeapon(s, (int)i);
            } else { // "damage"
                DrawUIText("(throw in combat)", (float)(screenW - 130), rowY + 4, 12, Fade(DARKGRAY, 0.8f));
            }
        }
        EndScissorMode();
        return;
    }

    // --- Backpack + equipped summary (weapon/armor buildings only) ---
    DrawInfoLine(TextFormat("Backpack (%d/%d)   Weapon power: %d   Defense: %d", (int)s.backpack.size(),
                          BackpackCap(s), CombatPower(s), TotalDefense(s)),
               20, y, 13, kColorAccent);
    y += 20;

    int backpackTop = y;
    int backpackHeight = screenH - backpackTop - 40;
    Rectangle backpackArea = { 0, (float)backpackTop, (float)screenW, (float)backpackHeight };
    s.backpackScroll -= ScrollDelta(backpackArea);
    float maxBackpackScroll = std::max(0.0f, (float)s.backpack.size() * 28.0f - backpackHeight);
    s.backpackScroll = std::clamp(s.backpackScroll, 0.0f, maxBackpackScroll);

    BeginScissorMode(0, backpackTop, screenW, backpackHeight);
    if (s.backpack.empty()) {
        DrawUIText("Nothing crafted yet.", 20, backpackTop + 4, 12, DARKGRAY);
    }
    for (size_t i = 0; i < s.backpack.size(); i++) {
        const Item& item = s.backpack[i];
        float rowY = backpackTop + (float)i * 28 - s.backpackScroll;
        if (rowY < backpackTop - 28 || rowY > backpackTop + backpackHeight) continue;
        DrawItemIcon(item, 20, rowY + 1, 20);
        DrawUIText(item.name.c_str(), 44, (int)rowY + 6, 12, kColorText);
        if (Button({ (float)(screenW - 180), rowY, 80, 22 }, "Equip", true)) EquipFromBackpack(s, (int)i);
        if (Button({ (float)(screenW - 90), rowY, 70, 22 }, "Sell", true)) SellFromBackpack(s, (int)i);
    }
    EndScissorMode();
}

// ---------------------------------------------------------------------
// Provisioner screen (added 2026-09-21) — Buy (reagents/bandages/heal potions,
// a small fixed catalog, not derived from Alchemy's recipe list) and Sell
// (backpack items, reusing SellFromBackpack — this is also Alchemy's only
// sell path, since DrawCraftScreen's Alchemy branch returns early before
// reaching its own backpack list). See DrawPillTabs/DrawInteriorBackdrop.
// ---------------------------------------------------------------------

static const int kProvisionerHealPotionCost = 15;

static void TryBuyHealPotion(GameState& s) {
    if (s.gold < kProvisionerHealPotionCost) { s.logLine = "Not enough gold for a Heal Potion."; return; }
    s.gold -= kProvisionerHealPotionCost;
    auto stack = std::find_if(s.potions.begin(), s.potions.end(), [](const PotionStack& p) { return p.name == "Heal Potion"; });
    if (stack == s.potions.end()) s.potions.push_back({ "Heal Potion", "heal", 30, 1 });
    else stack->count += 1;
    s.logLine = "Bought a Heal Potion for " + std::to_string(kProvisionerHealPotionCost) + " gold.";
}

static void DrawProvisionerScreen(GameState& s, int screenW, int screenH) {
    DrawInteriorBackdrop(g_assets.provisionerWallOk ? &g_assets.provisionerWall : nullptr,
                           g_assets.provisionerFloorOk ? &g_assets.provisionerFloor : nullptr,
                           screenW, screenH, 110);

    int y = 116;
    DrawInfoLine("The Provisioner — everyday supplies, bought and sold.", 20, y, 13, kColorAccent);
    y += 30;

    {
        int mode = s.provisionerTab;
        DrawPillTabs({ "Buy", "Sell" }, &mode, 20, (float)y, 26);
        s.provisionerTab = mode;
    }
    y += 34;

    if (s.provisionerTab == 0) {
        DrawInfoLine(TextFormat("Gold: %d", s.gold), 20, y, 13, kColorAccent);
        y += 24;

        DrawInfoLine(TextFormat("Reagents: %d", s.reagents), 20, y + 6, 12, kColorText);
        if (Button({ (float)(screenW - 140), (float)y, 120, 26 }, "Buy 5 (5g)", s.gold >= 5)) TryBuyReagents(s, 5);
        y += 34;

        DrawInfoLine(TextFormat("Bandages: %d", s.bandages), 20, y + 6, 12, kColorText);
        if (Button({ (float)(screenW - 140), (float)y, 120, 26 }, "Buy 5 (40g)", s.gold >= 40)) TryBuyBandages(s, 5, 40);
        y += 34;

        int healCount = 0;
        auto healStack = std::find_if(s.potions.begin(), s.potions.end(), [](const PotionStack& p) { return p.name == "Heal Potion"; });
        if (healStack != s.potions.end()) healCount = healStack->count;
        DrawInfoLine(TextFormat("Heal Potions: %d", healCount), 20, y + 6, 12, kColorText);
        if (Button({ (float)(screenW - 140), (float)y, 120, 26 }, TextFormat("Buy 1 (%dg)", kProvisionerHealPotionCost),
                    s.gold >= kProvisionerHealPotionCost)) TryBuyHealPotion(s);
        y += 34;
        return;
    }

    // --- Sell tab: same backpack-list-with-Sell-button block as DrawCraftScreen ---
    DrawInfoLine(TextFormat("Backpack (%d/%d)", (int)s.backpack.size(), BackpackCap(s)), 20, y, 13, kColorAccent);
    y += 20;

    int backpackTop = y;
    int backpackHeight = screenH - backpackTop - 40;
    Rectangle backpackArea = { 0, (float)backpackTop, (float)screenW, (float)backpackHeight };
    s.backpackScroll -= ScrollDelta(backpackArea);
    float maxBackpackScroll = std::max(0.0f, (float)s.backpack.size() * 28.0f - backpackHeight);
    s.backpackScroll = std::clamp(s.backpackScroll, 0.0f, maxBackpackScroll);

    BeginScissorMode(0, backpackTop, screenW, backpackHeight);
    if (s.backpack.empty()) {
        DrawUIText("Nothing to sell.", 20, backpackTop + 4, 12, DARKGRAY);
    }
    for (size_t i = 0; i < s.backpack.size(); i++) {
        const Item& item = s.backpack[i];
        float rowY = backpackTop + (float)i * 28 - s.backpackScroll;
        if (rowY < backpackTop - 28 || rowY > backpackTop + backpackHeight) continue;
        DrawItemIcon(item, 20, rowY + 1, 20);
        DrawUIText(item.name.c_str(), 44, (int)rowY + 6, 12, kColorText);
        if (Button({ (float)(screenW - 180), rowY, 80, 22 }, "Equip", true)) EquipFromBackpack(s, (int)i);
        if (Button({ (float)(screenW - 90), rowY, 70, 22 }, "Sell", true)) SellFromBackpack(s, (int)i);
    }
    EndScissorMode();
}

// ---------------------------------------------------------------------
// Magic screen: mana/reagent HUD, and a scrollable Spellcraft practice list —
// every spell (including Debuff/Buff/Summon, shown but not castable — see
// the "Magic / spellcasting" header note) can be practiced here risk-free,
// spending only mana to train Magery/Eval Int/Meditation.
// ---------------------------------------------------------------------

static void DrawMagicScreen(GameState& s, int screenW, int screenH) {
    int y = 116;
    DrawUIText(TextFormat("Mana: %.1f / %.0f   Reagents: %d", s.mana, MaxMana(s), s.reagents),
               20, y, 15, kColorText);
    y += 22;
    DrawUIText(TextFormat("Magery: %.1f   Eval Int: %.1f   Meditation: %.1f", s.magery, s.evalInt, s.meditation),
               20, y, 13, DARKGRAY);
    y += 24;

    DrawUIText("Out of reagents? Buy more at the Provisioner.", 20, y + 7, 13, Fade(DARKGRAY, 0.8f));
    y += 40;

    bool canMeditate = !s.combat.has_value() && !s.ambush.has_value() && !s.innocentEncounter.has_value() &&
                         s.mana < MaxMana(s);
    if (Button({ 20, (float)y, 150, 28 }, "Meditate", canMeditate)) Meditate(s);
    DrawUIText("Restores mana instantly, small chance of +Meditation", 180, y + 7, 13, Fade(DARKGRAY, 0.8f));
    y += 40;

    // Combat hotbar setup (2026-09-22) — configuration moved here from the Wilderness/
    // Hunt screens, where the same on-screen spot overlapped their interact button when
    // not engaged in a fight. This is the only place to assign slots now; casting still
    // happens on whichever live-combat screen you're actually fighting on.
    DrawUIText("Combat hotbar — tap a slot to assign a spell for live fights:", 20, y, 12, kColorAccent);
    y += 26;
    {
        int tapped = DrawCombatHotbarRow(s, false, 0.0f, 20.0f, (float)y);
        if (tapped >= 0) s.hotbarPickerSlot = tapped;
    }
    y += 68;
    DrawHotbarPicker(s, screenW, screenH);

    DrawUIText("Spellcraft — practice trains Magery/Eval Int/Meditation, mana only:", 20, y, 12,
               kColorAccent);
    y += 20;

    int listTop = y;
    int listHeight = screenH - listTop - 40;
    Rectangle listArea = { 0, (float)listTop, (float)screenW, (float)listHeight };
    s.magicScroll -= ScrollDelta(listArea);
    float maxScroll = std::max(0.0f, (float)kSpells.size() * 30.0f - listHeight);
    s.magicScroll = std::clamp(s.magicScroll, 0.0f, maxScroll);

    BeginScissorMode(0, listTop, screenW, listHeight);
    for (size_t i = 0; i < kSpells.size(); i++) {
        const Spell& sp = kSpells[i];
        float rowY = listTop + (float)i * 30 - s.magicScroll;
        if (rowY < listTop - 30 || rowY > listTop + listHeight) continue;

        bool castableType = sp.type == SpellType::Offensive || sp.type == SpellType::Utility;
        std::string typeNote = castableType ? "" : " [not usable in combat yet]";
        std::string line = TextFormat("Circle %d: %s (%dmp, needs %d magery)%s", sp.circle, sp.name.c_str(),
                                        sp.manaCost, sp.minSkill, typeNote.c_str());
        if (const Texture2D* icon = SpellIcon((int)i))
            DrawIconCentered(*icon, { 32, rowY + 12 }, 26.0f, WHITE);
        DrawUIText(line.c_str(), 50, (int)rowY + 6, 12, kColorText);
        if (Button({ (float)(screenW - 100), rowY, 80, 24 }, "Practice", s.mana >= sp.manaCost && CanPracticeSpell(s, sp)))
            TryPracticeSpell(s, (int)i);
    }
    EndScissorMode();
}

// ---------------------------------------------------------------------
// Pets screen (the Wildkeep): a taming attempt panel listing every wild
// creature with a live tame-% preview, and a scrollable roster of tamed
// pets with Active/Heal/Train/Release/Sell actions. Mirrors the JS's
// stable/Wildkeep panel plus renderPetRoster().
// ---------------------------------------------------------------------

static void DrawPetsScreen(GameState& s, int screenW, int screenH) {
    int y = 116;
    DrawUIText(TextFormat("Taming: %.1f   Lore: %.1f   Veterinary: %.1f", s.animalTaming, s.animalLore, s.veterinary),
               20, y, 13, kColorText);
    y += 18;
    DrawUIText(TextFormat("Pet slots: %d / %d", (int)s.pets.size(), PetSlotCapacity(s)), 20, y, 13, DARKGRAY);
    y += 22;

    if (s.tamingAttempt.has_value()) {
        const WildCreature& creature = kWildCreatures[s.tamingAttempt->creatureIdx];
        DrawUIText(("Taming a " + creature.name + "...").c_str(), 20, y, 14, kColorAccent);
        Rectangle bar = { 20, (float)(y + 20), 300, 10 };
        DrawRectangleRec(bar, Fade(BLACK, 0.25f));
        float pct = 1.0f - (s.tamingAttempt->secondsRemaining / 4.0f);
        DrawRectangleRec({ bar.x, bar.y, bar.width * std::clamp(pct, 0.0f, 1.0f), bar.height }, kColorSlate);
        return; // wait for the attempt to resolve before showing the creature list
    }

    DrawUIText("Wild creatures — attempt a tame (4s):", 20, y, 13, kColorAccent);
    y += 20;
    int listTop = y;
    int listHeight = 160;
    Rectangle listArea = { 0, (float)listTop, (float)screenW, (float)listHeight };
    s.creatureScroll -= ScrollDelta(listArea);
    float maxCreatureScroll = std::max(0.0f, (float)kWildCreatures.size() * 24.0f - listHeight);
    s.creatureScroll = std::clamp(s.creatureScroll, 0.0f, maxCreatureScroll);

    BeginScissorMode(0, listTop, screenW, listHeight);
    for (size_t i = 0; i < kWildCreatures.size(); i++) {
        const WildCreature& creature = kWildCreatures[i];
        float rowY = listTop + (float)i * 24 - s.creatureScroll;
        if (rowY < listTop - 24 || rowY > listTop + listHeight) continue;
        float chance = TameChance(s, creature);
        std::string line = TextFormat("%s (diff %d) - %.0f%% to tame", creature.name.c_str(),
                                        creature.difficulty, chance);
        DrawUIText(line.c_str(), 20, (int)rowY + 4, 12, kColorText);
        if (Button({ (float)(screenW - 90), rowY, 70, 20 }, "Tame", chance > 0))
            TryStartTameAttempt(s, (int)i);
    }
    EndScissorMode();
    y = listTop + listHeight + 8;

    // --- Pet roster ---
    DrawUIText("Your pets:", 20, y, 14, kColorAccent);
    y += 20;
    int rosterTop = y;
    int rosterHeight = screenH - rosterTop - 40;
    Rectangle rosterArea = { 0, (float)rosterTop, (float)screenW, (float)rosterHeight };
    s.petsScroll -= ScrollDelta(rosterArea);
    float maxScroll = std::max(0.0f, (float)s.pets.size() * 70.0f - rosterHeight);
    s.petsScroll = std::clamp(s.petsScroll, 0.0f, maxScroll);

    BeginScissorMode(0, rosterTop, screenW, rosterHeight);
    if (s.pets.empty()) {
        DrawUIText("No pets tamed yet.", 20, rosterTop + 4, 12, DARKGRAY);
    }
    for (size_t i = 0; i < s.pets.size(); i++) {
        Pet& pet = s.pets[i];
        float rowY = rosterTop + (float)i * 70 - s.petsScroll;
        if (rowY < rosterTop - 70 || rowY > rosterTop + rosterHeight) continue;

        std::string roleName = pet.role == PetRole::Tank ? "Tank" : pet.role == PetRole::Caster ? "Caster" : "Melee";
        std::string header = pet.name + (pet.active ? " (active)" : "") + " - " + roleName;
        DrawUIText(header.c_str(), 20, (int)rowY, 13, kColorText);
        std::string stats = TextFormat("HP %.0f/%.0f  Str %d Dex %d Int %d", pet.hp, pet.maxHp,
                                         pet.str, pet.dex, pet.intStat);
        DrawUIText(stats.c_str(), 20, (int)rowY + 16, 13, DARKGRAY);

        if (Button({ 20, rowY + 34, 70, 22 }, "Active", !pet.active)) SetPetActive(s, pet.id);
        if (Button({ 96, rowY + 34, 60, 22 }, "Heal", pet.hp < pet.maxHp)) HealPet(s, pet.id);
        if (Button({ 162, rowY + 34, 90, 22 }, "Train Wrest.", pet.wrestling < kPetTrainTarget && s.gold >= 1))
            TrainPetSkillGold(s, pet.id, &Pet::wrestling, "Wrestling");
        if (Button({ (float)(screenW - 150), rowY + 34, 70, 22 }, "Release", true)) ReleasePet(s, pet.id);
        if (Button({ (float)(screenW - 74), rowY + 34, 60, 22 }, "Sell", true)) SellPet(s, pet.id);
    }
    EndScissorMode();
}

// ---------------------------------------------------------------------
// Ambush & innocent-encounter panels: rendered in place of whatever screen
// is active whenever one is pending, blocking every other action until
// it's resolved — mirrors how the JS's ambush/innocent panels take over
// the current tab. Notoriety/Fame/Karma/Shaken status also lives here as
// a shared footer line, drawn from main() on every screen.
// ---------------------------------------------------------------------

static void DrawAmbushPanel(GameState& s, int screenW) {
    const GameState::AmbushEncounter& amb = *s.ambush;
    DrawUIText("Ambushed!", 20, 140, 22, kColorHeading);
    std::string line = amb.name + " blocks your path! (level " + std::to_string(amb.level) + ")";
    DrawUIText(line.c_str(), 20, 172, 14, kColorText);
    DrawUIText("A hardened killer, closely matched to your strength.", 20, 192, 12, DARKGRAY);
    if (Button({ 20, 220, 120, 40 }, "Fight", true)) AmbushFight(s);
    if (Button({ 150, 220, 120, 40 }, "Flee", true)) AmbushFlee(s);
}

static void DrawInnocentPanel(GameState& s, int screenW) {
    GameState::InnocentEncounter& enc = *s.innocentEncounter;
    DrawUIText((enc.name + " passes by...").c_str(), 20, 140, 20, kColorHeading);
    if (enc.canSteal) {
        DrawUIText(("You know they're carrying " + std::to_string(enc.gold) + " gold.").c_str(),
                   20, 172, 13, DARKGRAY);
    } else {
        DrawUIText("You could Spare, Snoop, or take a darker path.", 20, 172, 13, DARKGRAY);
    }
    int y = 210;
    if (Button({ 20, (float)y, 100, 36 }, "Spare", true)) SpareInnocent(s);
    if (Button({ 130, (float)y, 100, 36 }, "Snoop", true)) SnoopInnocent(s);
    y += 46;
    if (Button({ 20, (float)y, 100, 36 }, "Steal", enc.canSteal)) StealFromInnocent(s);
    if (Button({ 130, (float)y, 100, 36 }, "Murder", true)) MurderInnocent(s);
    y += 50;
    DrawUIText("Snoop reveals what they're carrying and unlocks Steal.", 20, y, 13, Fade(DARKGRAY, 0.8f));
}

// Shared footer: Notoriety/Fame/Karma/Shaken, drawn on every screen when any of
// them are non-default so it doesn't clutter a fresh game.
static void DrawNotorietyFooter(const GameState& s, int screenW, int screenH) {
    if (s.notoriety <= 0.01f && s.fame <= 0.01f && s.karma == 0.0f && s.shaken == 0) return;
    NotorietyTier tier = GetNotorietyTier(s);
    Color tierColor = tier == NotorietyTier::Murderer ? Color{ 138, 30, 30, 255 }
                     : tier == NotorietyTier::Criminal ? Color{ 160, 103, 46, 255 }
                     : Color{ 90, 74, 52, 255 };
    std::string title = s.titleLordEarned ? "Lord" : "";
    std::string line = NotorietyTierLabel(tier) + " (" + std::to_string((int)std::ceil(s.notoriety)) + ")" +
                         "   Fame: " + std::to_string((int)s.fame) + "   Karma: " + std::to_string((int)s.karma) +
                         (title.empty() ? "" : "   " + title);
    if (s.shaken > 0) line += "   Shaken (" + std::to_string(s.shaken) + ")";
    DrawUIText(line.c_str(), 20, screenH - 48, 12, tierColor);
}

// ---------------------------------------------------------------------
// Bank screen (the Vaultkeep + the Hearthmoot's weekly goals sharing one
// tab, both being "town amenity" features): deposit/withdraw gold and
// items completely safe from every danger system, plus the 5 weekly
// goals and their gold rewards / Weekly Blessing.
// ---------------------------------------------------------------------

static void DrawBankScreen(GameState& s, int screenW, int screenH) {
    int y = 116;
    DrawUIText(TextFormat("The Vaultkeep — Bank gold: %d", s.bankGold), 20, y, 15, kColorText);
    y += 22;

    if (Button({ 20, (float)y, 70, 26 }, "Dep 10g", s.gold >= 10)) DepositGold(s, 10);
    if (Button({ 96, (float)y, 70, 26 }, "Dep 50g", s.gold >= 50)) DepositGold(s, 50);
    if (Button({ 172, (float)y, 80, 26 }, "Dep All", s.gold > 0)) DepositGold(s, s.gold);
    if (Button({ 258, (float)y, 80, 26 }, "With 50g", s.bankGold >= 50)) WithdrawGold(s, 50);
    if (Button({ 344, (float)y, 90, 26 }, "With All", s.bankGold > 0)) WithdrawGold(s, s.bankGold);
    y += 36;

    DrawUIText("Your backpack (Deposit):", 20, y, 12, kColorAccent);
    y += 18;
    int backTop = y;
    int backHeight = 90;
    Rectangle backArea = { 0, (float)backTop, (float)screenW, (float)backHeight };
    s.bankScroll -= ScrollDelta(backArea);
    float maxBackScroll = std::max(0.0f, (float)s.backpack.size() * 26.0f - backHeight);
    s.bankScroll = std::clamp(s.bankScroll, 0.0f, maxBackScroll);
    BeginScissorMode(0, backTop, screenW, backHeight);
    if (s.backpack.empty()) DrawUIText("Backpack is empty.", 20, backTop + 4, 13, DARKGRAY);
    for (size_t i = 0; i < s.backpack.size(); i++) {
        float rowY = backTop + (float)i * 26 - s.bankScroll;
        if (rowY < backTop - 26 || rowY > backTop + backHeight) continue;
        DrawItemIcon(s.backpack[i], 20, rowY, 18);
        DrawUIText(s.backpack[i].name.c_str(), 42, (int)rowY + 4, 13, kColorText);
        if (Button({ (float)(screenW - 90), rowY, 70, 20 }, "Deposit", true)) DepositItem(s, (int)i);
    }
    EndScissorMode();
    y = backTop + backHeight + 8;

    DrawUIText("Vault contents (Withdraw):", 20, y, 12, kColorAccent);
    y += 18;
    int vaultTop = y;
    int vaultHeight = 90;
    Rectangle vaultArea = { 0, (float)vaultTop, (float)screenW, (float)vaultHeight };
    s.bankItemsScroll -= ScrollDelta(vaultArea);
    float maxVaultScroll = std::max(0.0f, (float)s.bankItems.size() * 26.0f - vaultHeight);
    s.bankItemsScroll = std::clamp(s.bankItemsScroll, 0.0f, maxVaultScroll);
    BeginScissorMode(0, vaultTop, screenW, vaultHeight);
    if (s.bankItems.empty()) DrawUIText("Vault is empty.", 20, vaultTop + 4, 13, DARKGRAY);
    for (size_t i = 0; i < s.bankItems.size(); i++) {
        float rowY = vaultTop + (float)i * 26 - s.bankItemsScroll;
        if (rowY < vaultTop - 26 || rowY > vaultTop + vaultHeight) continue;
        DrawUIText(s.bankItems[i].name.c_str(), 20, (int)rowY + 4, 13, kColorText);
        if (Button({ (float)(screenW - 100), rowY, 80, 20 }, "Withdraw", true)) WithdrawItem(s, (int)i);
    }
    EndScissorMode();
    y = vaultTop + vaultHeight + 12;

    // --- The Hearthmoot's weekly goals ---
    CheckWeeklyReset(s);
    long long secondsLeft = std::max(0LL, (s.weekStartEpoch + kWeekSeconds) - (long long)std::time(nullptr));
    long long daysLeft = secondsLeft / 86400;
    DrawUIText(TextFormat("The Hearthmoot — resets in ~%lld day%s", daysLeft, daysLeft == 1 ? "" : "s"),
               20, y, 13, kColorAccent);
    y += 20;
    if (HasWeeklyBlessing(s))
        DrawUIText("Weekly Blessing active: +10% combat power!", 20, y, 12, kColorSlate), y += 18;

    for (int i = 0; i < kWeeklyGoalCount; i++) {
        const WeeklyGoalDef& goal = kWeeklyGoals[i];
        int progress = std::min(goal.target, s.weeklyProgress[i]);
        std::string line = std::string(goal.label) + ": " + std::to_string(progress) + "/" + std::to_string(goal.target);
        DrawUIText(line.c_str(), 20, y + 4, 12, kColorText);
        bool ready = s.weeklyProgress[i] >= goal.target && !s.weeklyClaimed[i];
        std::string label = s.weeklyClaimed[i] ? "Claimed" : "Claim";
        if (Button({ (float)(screenW - 90), (float)y, 70, 22 }, label, ready)) ClaimWeeklyGoal(s, i);
        y += 26;
    }
}

// ---------------------------------------------------------------------
// House screen — ported from the JS's House panel (HOUSE_TIERS/HOUSE_HUES/
// HOME_MODULE_DEF/HOME_MODULE_LEVELS, see those tables for the numbers and the
// porting-history comment above kHouseTiers). Phase 1 of a bigger housing feature
// Mark wants eventually (freeform decoration/placement, well beyond what the JS ever
// speced) — this slice is the faithful port: tiers grow backpack capacity, hues/name
// are the cosmetic angle, workshop wings are the "hub" that gives a real reason to
// come home instead of just visiting town.
// ---------------------------------------------------------------------

// Simple single-line text entry — captures typed characters and Backspace every frame
// this is called, up to maxLen. No focus/click management since only two screens
// (Character, House) ever call this, each on its own field — it's just "always live"
// whenever that screen is showing.
static void UpdateTextInput(std::string& text, size_t maxLen) {
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125 && text.size() < maxLen) text += (char)key;
        key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !text.empty()) text.pop_back();
}

static void DrawHouseScreen(GameState& s, int screenW, int screenH) {
    UpdateTextInput(s.houseName, 24);
    const HouseTier& tier = kHouseTiers[s.houseTierIdx];

    int y = 116;
    DrawUIText(("Your House — " + tier.name).c_str(), 20, y, 18, kColorHeading);
    y += 24;

    // Name entry — same always-live pattern as the Character screen's name field.
    Rectangle nameBox = { 20, (float)y, (float)(screenW - 40), 26 };
    DrawRectangleRec(nameBox, Fade(WHITE, 0.6f));
    DrawRectangleRoundedLines(nameBox, 0.15f, 4, Fade(BLACK, 0.4f));
    std::string nameShown = s.houseName.empty() ? "Name your house" : s.houseName;
    DrawUIText(nameShown.c_str(), (int)nameBox.x + 6, (int)nameBox.y + 6, 13,
               s.houseName.empty() ? Fade(DARKGRAY, 0.6f) : kColorText);
    if (std::fmod(GetTime(), 1.0) < 0.5) {
        int caretX = (int)nameBox.x + 6 + (s.houseName.empty() ? 0 : MeasureUIText(s.houseName.c_str(), 13));
        DrawRectangle(caretX + 1, (int)nameBox.y + 6, 2, 15, kColorText);
    }
    y += 32;

    DrawUIText(TextFormat("Backpack capacity: %d (base %d + %d from house)",
                            BackpackCap(s), GameState::kBackpackCap, tier.capBonus), 20, y, 12, kColorText);
    y += 22;

    // --- Tiers ---
    DrawUIText("Tiers:", 20, y, 13, kColorAccent);
    y += 18;
    for (size_t i = 1; i < kHouseTiers.size(); i++) {
        const HouseTier& t = kHouseTiers[i];
        bool owned = (int)i <= s.houseTierIdx;
        int cost = t.cost - kHouseTiers[s.houseTierIdx].cost;
        std::string line = TextFormat("%s  (+%d cap, %d hues, %d wing%s)", t.name.c_str(), t.capBonus,
                                        t.hueOptions, t.moduleSlots, t.moduleSlots == 1 ? "" : "s");
        DrawUIText(line.c_str(), 20, y + 5, 12, owned ? Fade(kColorText, 0.6f) : kColorText);
        std::string btnLabel = owned ? "Owned" : TextFormat("Buy (%dg)", cost);
        if (Button({ (float)(screenW - 110), (float)y, 90, 22 }, btnLabel, !owned && s.gold >= cost))
            BuyHouseTier(s, (int)i);
        y += 26;
    }
    y += 4;

    // --- Hue swatches ---
    if (tier.hueOptions > 0) {
        DrawUIText("Color:", 20, y, 13, kColorAccent);
        y += 18;
        for (int i = 0; i < tier.hueOptions; i++) {
            Rectangle sw = { (float)(20 + i * 30), (float)y, 24, 24 };
            DrawRectangleRec(sw, kHouseHues[i].color);
            DrawRectangleLinesEx(sw, s.houseHue == i ? 3.0f : 1.0f, s.houseHue == i ? kColorHeading : Fade(BLACK, 0.4f));
            if (CheckCollisionPointRec(GetMousePosition(), sw) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                SetHouseHue(s, i);
        }
        y += 32;
    }

    // --- Workshop wings (scrollable — 4 modules, each with its recipe list once built) ---
    DrawUIText(TextFormat("Home Workshop — %d/%d wings built (each stays one tier behind town)",
                            BuiltHouseModuleCount(s), tier.moduleSlots), 20, y, 12, kColorAccent);
    y += 20;
    if (tier.moduleSlots <= 0) {
        DrawUIText("Buy a house to unlock personal workshop wings.", 20, y, 12, DARKGRAY);
        return;
    }

    int listTop = y;
    int listHeight = screenH - listTop - 20;
    Rectangle listArea = { 0, (float)listTop, (float)screenW, (float)listHeight };
    s.houseScroll -= ScrollDelta(listArea);

    // Precompute each module's row height (recipe rows for built modules push it out)
    // so scrolling/clamping accounts for the real content height, not a flat row size.
    auto moduleRowHeight = [&](int idx) -> float {
        int level = s.houseModuleLevel[idx];
        if (level <= 0) return 56.0f;
        const BuildingDef& b = kCraftBuildings[kHomeModuleDefs[idx].buildingIdx];
        return 56.0f + (float)b.recipes.size() * 28.0f;
    };
    float totalHeight = 0;
    for (size_t i = 0; i < kHomeModuleDefs.size(); i++) totalHeight += moduleRowHeight((int)i) + 8;
    s.houseScroll = std::clamp(s.houseScroll, 0.0f, std::max(0.0f, totalHeight - listHeight));

    BeginScissorMode(0, listTop, screenW, listHeight);
    float rowY = listTop - s.houseScroll;
    for (size_t i = 0; i < kHomeModuleDefs.size(); i++) {
        const HomeModuleDef& def = kHomeModuleDefs[i];
        int level = s.houseModuleLevel[i];
        float rh = moduleRowHeight((int)i);
        if (rowY + rh >= listTop && rowY <= listTop + listHeight) {
            if (level <= 0) {
                DrawUIText((def.label + "  (not built)").c_str(), 20, (int)rowY + 4, 13, kColorText);
                bool canBuild = BuiltHouseModuleCount(s) < tier.moduleSlots && s.gold >= kHomeModuleLevels[0].cost;
                if (Button({ 20, rowY + 24, 200, 24 }, TextFormat("Build Wing (%dg)", kHomeModuleLevels[0].cost), canBuild))
                    BuildHouseModule(s, (int)i);
            } else {
                int cap = kHomeModuleLevels[level - 1].cap;
                DrawUIText(TextFormat("%s  (level %d/5, cap %d)", def.label.c_str(), level, cap),
                           20, (int)rowY + 4, 13, kColorText);
                if (level < (int)kHomeModuleLevels.size()) {
                    int upCost = kHomeModuleLevels[level].cost;
                    if (Button({ (float)(screenW - 190), rowY, 170, 22 }, TextFormat("Upgrade (%dg)", upCost), s.gold >= upCost))
                        UpgradeHouseModule(s, (int)i);
                }
                const BuildingDef& b = kCraftBuildings[def.buildingIdx];
                float skillVal = s.buildingSkill[def.buildingIdx];
                for (size_t r = 0; r < b.recipes.size(); r++) {
                    const Recipe& recipe = b.recipes[r];
                    float recipeY = rowY + 30 + (float)r * 28;
                    bool skillOk = std::min(skillVal, (float)cap) >= recipe.reqSkill;
                    std::string line = TextFormat("  %s  (req %d, %d %s)", recipe.name.c_str(), recipe.reqSkill,
                                                    recipe.cost, def.buildingIdx == 3 ? "reagents" : "resource");
                    DrawUIText(line.c_str(), 20, (int)recipeY + 4, 12, skillOk ? kColorText : Fade(DARKGRAY, 0.8f));
                    if (Button({ (float)(screenW - 90), recipeY, 70, 22 }, def.buildingIdx == 3 ? "Brew" : "Craft", skillOk)) {
                        if (def.buildingIdx == 3) TryCraftPotion(s, (int)r, cap);
                        else TryCraftItem(s, def.buildingIdx, (int)r, cap);
                    }
                }
            }
        }
        rowY += rh + 8;
    }
    EndScissorMode();
}

// ---------------------------------------------------------------------
// Skills screen (the Echo system): the 700-point active-skill budget bar,
// and every capped skill with its value and an Active/Bench toggle.
// Benching frees budget for something else without losing any progress —
// mirrors setSkillActive()'s all-or-nothing per-skill toggle.
// ---------------------------------------------------------------------

static void DrawSkillsScreen(GameState& s, int screenW, int screenH) {
    int y = 116;
    float activeTotal = ActiveSkillTotal(s);
    DrawUIText(TextFormat("Active build: %.1f / %.0f", activeTotal, kTotalSkillCap), 20, y, 15,
               kColorText);
    y += 20;
    Rectangle budgetBar = { 20, (float)y, (float)(screenW - 40), 12 };
    DrawRectangleRec(budgetBar, Fade(BLACK, 0.25f));
    float pct = std::clamp(activeTotal / kTotalSkillCap, 0.0f, 1.0f);
    DrawRectangleRec({ budgetBar.x, budgetBar.y, budgetBar.width * pct, budgetBar.height },
                       activeTotal >= kTotalSkillCap ? kColorSlate : Color{ 63, 94, 63, 255 });
    y += 24;
    DrawUIText("Every skill trains freely to its own cap regardless of Active/Benched —", 20, y, 13, Fade(DARKGRAY, 0.8f));
    y += 14;
    DrawUIText("benching just frees budget for something else without losing progress.", 20, y, 13, Fade(DARKGRAY, 0.8f));
    y += 20;

    int listTop = y;
    int listHeight = screenH - listTop - 40;
    Rectangle listArea = { 0, (float)listTop, (float)screenW, (float)listHeight };
    s.craftScroll -= ScrollDelta(listArea);
    float maxScroll = std::max(0.0f, (float)kCappedSkills.size() * 30.0f - listHeight);
    s.craftScroll = std::clamp(s.craftScroll, 0.0f, maxScroll);

    BeginScissorMode(0, listTop, screenW, listHeight);
    for (size_t i = 0; i < kCappedSkills.size(); i++) {
        float rowY = listTop + (float)i * 30 - s.craftScroll;
        if (rowY < listTop - 30 || rowY > listTop + listHeight) continue;
        float val = s.*(kCappedSkills[i].field);
        std::string line = TextFormat("%s: %.1f", kCappedSkills[i].label, val);
        DrawUIText(line.c_str(), 20, (int)rowY + 6, 13, kColorText);
        bool active = s.skillActive[i];
        std::string label = active ? "Active" : "Benched";
        if (Button({ (float)(screenW - 100), rowY, 80, 24 }, label, true))
            SetSkillActive(s, (int)i, !active);
    }
    EndScissorMode();
}

// ---------------------------------------------------------------------
// Character screen — the page missing from the original port. Name entry,
// the title/vocation display, HP/Mana (Stamina is not shown — there's no
// stamina stat in this scaffold, flagged back in the Magic section),
// STR/DEX/INT, the 8-slot equipment grid, weapon power/defense totals,
// core resources, and the real "Use Bandage" action. Mirrors the JS's
// #tab-character, minus the "Use Refresh Potion" half of its Meditate
// button, since Refresh potions have nothing to restore without a
// stamina stat.
// ---------------------------------------------------------------------

// Dotted horizontal rule — used by the Character screen's ornate equipment card to
// separate rows (2026-09-23 redesign) instead of a solid line, matching the reference
// layout Mark supplied. Plain small filled rectangles rather than a dashed line style
// (raylib has no built-in dashed-line primitive), spaced by eye to read as "dotted" at
// this line thickness.
static void DrawDottedLineH(float x1, float x2, float y, Color color) {
    for (float x = x1; x < x2; x += 6.0f) DrawRectangle((int)x, (int)y, 3, 2, color);
}

static void DrawCharacterScreen(GameState& s, int screenW, int screenH) {
    UpdateTextInput(s.characterName, 24);

    int y = 116;
    float overallSkill = OverallSkill(s);
    DrawUIText(("Character  " + TitleFor(overallSkill)).c_str(), 20, y, 18, kColorHeading);
    y += 24;

    // Name entry — this is the only text field in the whole game (no click-to-focus:
    // typing works any time this screen is open), which wasn't obvious with no visible
    // cursor — a blinking caret after the text makes it read as "live" the way a normal
    // text box would, instead of looking like inert label text.
    Rectangle nameBox = { 20, (float)y, (float)(screenW - 40), 26 };
    DrawRectangleRec(nameBox, Fade(WHITE, 0.6f));
    DrawRectangleRoundedLines(nameBox, 0.15f, 4, Fade(BLACK, 0.4f));
    std::string nameShown = s.characterName.empty() ? "Name your character" : s.characterName;
    DrawUIText(nameShown.c_str(), (int)nameBox.x + 6, (int)nameBox.y + 6, 13,
               s.characterName.empty() ? Fade(DARKGRAY, 0.6f) : kColorText);
    if (std::fmod(GetTime(), 1.0) < 0.5) {
        int caretX = (int)nameBox.x + 6 + (s.characterName.empty() ? 0 : MeasureUIText(s.characterName.c_str(), 13));
        DrawRectangle(caretX + 1, (int)nameBox.y + 6, 2, 15, kColorText);
    }
    y += 32;

    // Subtitle — bumped from plain DARKGRAY to the accent color/a slightly bigger size
    // so naming a character reads as more consequential than inert label text (2026-09-22
    // UI polish pass; the name box itself already had a bordered card, just this line
    // was flat).
    DrawUIText(CharacterDisplayName(s).c_str(), 20, y, 14, kColorAccent);
    y += 22;

    if (s.shaken > 0) { DrawUIText(TextFormat("Shaken (%d)", s.shaken), 20, y, 12, Color{ 122, 46, 46, 255 }); y += 16; }
    NotorietyTier tier = GetNotorietyTier(s);
    if (tier != NotorietyTier::Innocent) {
        Color tc = tier == NotorietyTier::Murderer ? Color{ 138, 30, 30, 255 } : Color{ 160, 103, 46, 255 };
        DrawUIText(NotorietyTierLabel(tier).c_str(), 20, y, 12, tc);
        y += 16;
    }
    y += 6;

    // Resources (2026-09-23 redesign) — Mark asked for gold/resources to be listed on
    // this page again (they were trimmed to just Reagents in the 2026-09-22 pass, on
    // the reasoning that Gold/Wood/Ore/Leather already show in the top bar on every
    // screen — that reasoning still holds, this is deliberate duplication he asked for
    // back, not a regression of that earlier decision).
    DrawUIText(TextFormat("Gold: %d   Wood: %d   Ore: %d   Leather: %d   Reagents: %d",
                            s.gold, s.wood, s.ore, s.leather, s.reagents), 20, y, 13, kColorAccent);
    y += 26;

    // Equipment card redesign (2026-09-23) — Mark supplied a reference layout (ornate
    // parchment/wood-frame card, single-column icon+label+value rows, a "DERIVED STATS"
    // footer) and asked to match its look exactly, replacing the paperdoll + 2-column
    // equipment grid this screen used before. Colors are local to this one card on
    // purpose — the rest of the game deliberately moved off a gold/parchment palette
    // some time ago (see the kColorText/kColorPanelBg block's own comment on that), so
    // this is a one-screen departure, not a reversion of that earlier decision, unless
    // asked to spread it further.
    {
        const Color kWoodBorder = { 92, 58, 30, 255 };
        const Color kWoodBorderLight = { 168, 116, 62, 255 };
        const Color kParchmentBg = { 232, 209, 171, 255 };
        const Color kBannerBg = { 64, 40, 22, 255 };
        const Color kBannerText = { 232, 205, 140, 255 };
        const Color kRivet = { 78, 78, 84, 255 };

        struct EquipRow { const char* label; const std::optional<Item>* item; };
        const std::array<EquipRow, 8> rows = {{
            { "HELMET", &s.equipped.helmet }, { "GORGET", &s.equipped.gorget },
            { "LEFT HAND", &s.equipped.leftHand }, { "RIGHT HAND", &s.equipped.rightHand },
            { "GLOVES", &s.equipped.gloves }, { "CHEST", &s.equipped.chest },
            { "ARMS", &s.equipped.arms }, { "LEGS", &s.equipped.legs },
        }};
        const float rowH = 42.0f;
        const float cardX = 16.0f, cardW = (float)(screenW - 32);
        const float cardH = 40.0f + 20.0f + rows.size() * rowH + 54.0f + 24.0f; // banner + subtitle + rows + footer + padding
        Rectangle card = { cardX, (float)y, cardW, cardH };

        // Outer wood frame + parchment fill, corner rivets.
        DrawRectangleRounded(card, 0.06f, 8, kWoodBorder);
        Rectangle cardInner = { card.x + 5, card.y + 5, card.width - 10, card.height - 10 };
        DrawRectangleRounded(cardInner, 0.06f, 8, kParchmentBg);
        for (Vector2 corner : { Vector2{card.x + 14, card.y + 14}, Vector2{card.x + card.width - 14, card.y + 14},
                                  Vector2{card.x + 14, card.y + card.height - 14}, Vector2{card.x + card.width - 14, card.y + card.height - 14} }) {
            DrawCircleV(corner, 5.0f, kRivet);
            DrawCircleV({ corner.x - 1, corner.y - 1 }, 1.6f, Fade(WHITE, 0.6f));
        }

        int cy = y + 14;
        // Embossed title banner.
        Rectangle banner = { card.x + 30, (float)cy, card.width - 60, 34 };
        DrawRectangleRounded(banner, 0.35f, 6, kWoodBorder);
        Rectangle bannerInner = { banner.x + 3, banner.y + 3, banner.width - 6, banner.height - 6 };
        DrawRectangleRounded(bannerInner, 0.35f, 6, kBannerBg);
        const char* title = "EQUIPMENT - LOADOUT";
        int titleW = MeasureUIText(title, 15);
        DrawUIText(title, (int)(banner.x + banner.width / 2.0f - titleW / 2.0f), (int)banner.y + 9, 15, kBannerText);
        cy += 34 + 10;

        DrawUIText("Town Forge - Character Equipment", (int)card.x + 20, cy, 11, Fade(kWoodBorder, 0.85f));
        cy += 10;
        DrawDottedLineH(card.x + 20, card.x + card.width - 20, (float)cy + 10, Fade(kWoodBorder, 0.6f));
        cy += 16;

        // One row per equipment slot: icon thumbnail, then "LABEL: item name" (or
        // "- Empty -"), a dotted rule beneath each — DrawItemIcon is the exact same
        // per-item icon lookup the backpack list below already uses.
        for (const EquipRow& row : rows) {
            const std::optional<Item>& item = *row.item;
            Rectangle iconBox = { card.x + 20, (float)cy, 34, 34 };
            DrawRectangleRounded(iconBox, 0.2f, 4, Fade(WHITE, 0.55f));
            DrawRectangleRoundedLines(iconBox, 0.2f, 4, kWoodBorder);
            if (item.has_value()) DrawItemIcon(*item, iconBox.x + 3, iconBox.y + 3, 28);
            float textX = iconBox.x + 34 + 14;
            std::string labelPart = std::string(row.label) + ": ";
            DrawUIText(labelPart.c_str(), (int)textX, cy + 9, 13, kColorHeading);
            int labelW = MeasureUIText(labelPart.c_str(), 13);
            DrawUIText(item.has_value() ? item->name.c_str() : "- Empty -", (int)(textX + labelW), cy + 9, 13,
                        item.has_value() ? kWoodBorderLight : Fade(DARKGRAY, 0.65f));
            cy += (int)rowH;
            DrawDottedLineH(card.x + 20, card.x + card.width - 20, (float)cy - 8, Fade(kWoodBorder, 0.35f));
        }

        // Derived stats footer — Weapon Power and Total Defense are real, computed
        // stats (CombatPower/TotalDefense, same formulas used everywhere else in the
        // game); the reference layout's Load/Weight/set-bonus line was left out on
        // purpose rather than faked — this game has no encumbrance or item-set-bonus
        // system to report a real number for, and showing invented values that don't
        // affect anything would be misleading, not just decorative.
        cy += 12;
        Rectangle footer = { card.x + 20, (float)cy, card.width - 40, 40 };
        DrawRectangleRoundedLines(footer, 0.15f, 6, kWoodBorder);
        // "DERIVED STATS" label straddling the top border, same badge-on-the-line look
        // as the reference image rather than a plain heading above the box.
        const char* footerLabel = "DERIVED STATS";
        int footerLabelW = MeasureUIText(footerLabel, 11);
        Rectangle footerBadge = { footer.x + footer.width / 2.0f - footerLabelW / 2.0f - 8, footer.y - 8, (float)footerLabelW + 16, 16 };
        DrawRectangleRec(footerBadge, kParchmentBg);
        DrawUIText(footerLabel, (int)(footer.x + footer.width / 2.0f - footerLabelW / 2.0f), (int)footer.y - 6, 11, kWoodBorder);
        std::string statsLine = TextFormat("Weapon Power: %d   Total Defense: %d", CombatPower(s), TotalDefense(s));
        int statsW = MeasureUIText(statsLine.c_str(), 13);
        DrawUIText(statsLine.c_str(), (int)(footer.x + footer.width / 2.0f - statsW / 2.0f), (int)footer.y + 14, 13, kColorHeading);

        y += (int)card.height + 12;
    }

    // Backpack — was previously only visible on the Craft screen (where Sell also makes
    // sense); shown here too as a quick "what am I carrying" reference, with just Equip
    // (this page is about the character, not the shop). Shares s.backpackScroll with the
    // Craft screen's list — one scroll-position field, not worth a second for this.
    DrawUIText(TextFormat("Backpack (%d/%d)", (int)s.backpack.size(), BackpackCap(s)), 20, y, 13, kColorAccent);
    y += 18;
    int packTop = y;
    int packHeight = screenH - packTop - 20;
    Rectangle packArea = { 0, (float)packTop, (float)screenW, (float)packHeight };
    s.backpackScroll -= ScrollDelta(packArea);
    float maxPackScroll = std::max(0.0f, (float)s.backpack.size() * 28.0f - packHeight);
    s.backpackScroll = std::clamp(s.backpackScroll, 0.0f, maxPackScroll);
    BeginScissorMode(0, packTop, screenW, packHeight);
    if (s.backpack.empty()) {
        DrawUIText("Nothing in your backpack.", 20, packTop + 4, 12, DARKGRAY);
    }
    for (size_t i = 0; i < s.backpack.size(); i++) {
        const Item& item = s.backpack[i];
        float rowY = packTop + (float)i * 28 - s.backpackScroll;
        if (rowY < packTop - 28 || rowY > packTop + packHeight) continue;
        DrawItemIcon(item, 20, rowY, 20);
        DrawUIText(item.name.c_str(), 44, (int)rowY + 4, 12, kColorText);
        if (Button({ (float)(screenW - 90), rowY, 70, 22 }, "Equip", true)) EquipFromBackpack(s, (int)i);
    }
    EndScissorMode();
}

// Hoisted out of main() so UpdateDrawFrame() (a plain function pointer, called every
// frame either by the desktop while-loop below or by emscripten_set_main_loop on web —
// see main()) can reach them; there's exactly one of each for the process's lifetime
// either way, so file-scope statics cost nothing bar naming a global.
static GameState g_state;
static const int kScreenW = 540;
static const int kScreenH = 900;
static float g_autosaveTimer = 0.0f;
static float g_resetArmedTimer = 0.0f; // >0 while the Reset button is armed, waiting for a confirm click

// Desktop-only "zoom the whole view" — Mark asked for the game to look ~10-15% bigger.
// Every draw call in this file is an absolute pixel coordinate tuned for the fixed
// 540x900 internal resolution, so scaling *that* directly would mean re-tuning hundreds
// of Rectangle/font-size constants. Instead: keep every existing draw call completely
// untouched, rendering into a RenderTexture2D still sized 540x900, then present that
// texture scaled up to fill a proportionally bigger actual window — see UpdateDrawFrame
// and main(). SetMouseScale compensates so GetMousePosition()/Button() hit-tests keep
// seeing the original 540x900 coordinate space, unaware anything changed.
// **Not applied to the web build** — the canvas there is already independently scaled
// to fit the browser viewport by shell.html's CSS (unrelated to this constant), and
// raylib's Emscripten layer has its own canvas-buffer-to-CSS-size mouse mapping; adding
// SetMouseScale on top of that risked double-compensating and breaking tap targets in a
// way that's hard to verify without a real device. Web's equivalent zoom is a separate
// CSS change in shell.html instead (see its comment).
static const float kZoom = 1.125f;
#ifndef __EMSCRIPTEN__
static RenderTexture2D g_zoomTarget;
#endif

// One frame's worth of update+draw. Split out of main() so it can be handed to
// emscripten_set_main_loop on web — a blocking `while(!WindowShouldClose())` loop only
// "works" there via -s ASYNCIFY around WindowShouldClose()'s internal emscripten_sleep(),
// which turned out to be fragile (it hung indefinitely partway through startup once this
// build grew past some threshold) — emscripten_set_main_loop is the robust, standard
// pattern raylib's own web examples use instead, with the browser's requestAnimationFrame
// driving each call rather than a C++-side blocking sleep.
static void UpdateDrawFrame() {
#ifdef __EMSCRIPTEN__
    // Hold off on everything else until the async IndexedDB load (kicked off by
    // JS_InitPersistence in main()) has actually landed — reading the save file before
    // then would see an empty directory and silently start a fresh game even when a
    // real save exists on this device. Desktop has no equivalent wait (LoadGame already
    // ran synchronously in main(), before the loop even started, since its filesystem
    // is real and synchronous). Runs once; `loaded` latches true and this whole block
    // is skipped on every frame after.
    static bool loaded = false;
    if (!loaded) {
        if (!JS_PersistReady()) {
            BeginDrawing();
            ClearBackground(kColorPageBg);
            DrawUIText("Loading save data...", kScreenW / 2 - 90, kScreenH / 2, 18, kColorText);
            EndDrawing();
            return;
        }
        loaded = true;
        bool hadSave = LoadGame(g_state);
        if (hadSave && !g_state.logLine.empty() && g_state.logLine == "Welcome to Town Forge.")
            g_state.logLine = "Welcome back.";
    }
#endif
    GameState& state = g_state;
    int screenW = kScreenW, screenH = kScreenH;
    float& autosaveTimer = g_autosaveTimer;
    float& resetArmedTimer = g_resetArmedTimer;
    {
        float dt = GetFrameTime();

        // --- Update ---
        UpdateGathering(state, dt);
        UpdateUpgrade(state, dt);
        RegenMana(state, dt);
        RegenAllPetMana(state, dt);
        UpdateTameAttempt(state, dt);
        RegenNotoriety(state, dt);
        state.worldTime += dt;
        UpdateCombatAnim(state, dt);

        // JS: autosave every 2 seconds (setInterval(() => { render(); save(); }, 2000)).
        autosaveTimer += dt;
        if (autosaveTimer >= 2.0f) {
            autosaveTimer = 0.0f;
            SaveGame(state);
        }
        if (resetArmedTimer > 0.0f) {
            resetArmedTimer -= dt;
            if (resetArmedTimer <= 0.0f) state.logLine = "Reset cancelled.";
        }

        // JS: after a gather completes, roll for an ambush/innocent encounter, then
        // (if nothing triggered) let auto-gather chain into its next action.
        if (!state.pendingEncounterCheck.empty()) {
            std::string src = state.pendingEncounterCheck;
            state.pendingEncounterCheck.clear();
            bool triggered = TryTriggerAmbush(state, src);
            if (!triggered) triggered = TryTriggerInnocentEncounter(state, src);
            if (triggered) {
                state.autoGather = false;
            } else if (state.autoGather && src == "gather") {
                std::string next = NextAutoGatherType(state);
                if (next.empty()) {
                    state.autoGather = false;
                    state.logLine = "Auto-gather complete — Mining and Lumberjacking both reached 100.";
                } else {
                    TryStartGather(state, next);
                }
            }
        }

        bool encounterPending = state.ambush.has_value() || state.innocentEncounter.has_value();

        // --- Input: keyboard shortcuts for gathering (1/2/3) and closing the building
        // panel (ESC); movement (WASD/arrows) and interaction (E) are handled inside
        // DrawTownScreen itself, since they need the frame's nearest-node lookup. ---
        if (!encounterPending && state.screen == Screen::Town) {
            if (IsKeyPressed(KEY_ONE))   TryStartGather(state, "wood");
            if (IsKeyPressed(KEY_TWO))   TryStartGather(state, "ore");
            if (IsKeyPressed(KEY_THREE)) ToggleAutoGather(state);
            if (IsKeyPressed(KEY_ESCAPE)) state.selectedTile.reset();
        }
        // --- Input: combat shortcuts, only meaningful on Hunt while fighting ---
        if (!encounterPending && state.screen == Screen::Hunt && state.combat.has_value()) {
            if (IsKeyPressed(KEY_A)) ResolveCombatRound(state);
            if (IsKeyPressed(KEY_F)) FleeCombat(state);
            if (IsKeyPressed(KEY_B)) UseBandageInCombat(state);
        }
        // --- Input: switch screens with Tab (cycles Character -> Town -> Hunt -> Craft -> Magic -> Pets -> Bank -> House -> Skills -> Character) ---
        if (!encounterPending && IsKeyPressed(KEY_TAB)) {
            if (!(state.screen == Screen::Hunt && state.combat.has_value())) { // don't tab away mid-fight
                Screen next = (state.screen == Screen::Character) ? Screen::Town
                            : (state.screen == Screen::Town) ? Screen::Hunt
                            : (state.screen == Screen::Hunt) ? Screen::Craft
                            : (state.screen == Screen::Craft) ? Screen::Magic
                            : (state.screen == Screen::Magic) ? Screen::Pets
                            : (state.screen == Screen::Pets) ? Screen::Bank
                            : (state.screen == Screen::Bank) ? Screen::House
                            : (state.screen == Screen::House) ? Screen::Skills : Screen::Character;
                GuardZoneConfiscateIfMurderer(state, next); // JS switchTab(): Murderer tier gets bounced from Craft
                state.screen = next;
            }
        }

        // --- Draw ---
#ifndef __EMSCRIPTEN__
        BeginTextureMode(g_zoomTarget); // desktop: draw at the original 540x900, upscaled below
#else
        BeginDrawing();
#endif
        ClearBackground(kColorPageBg); // parchment background

        DrawUIText("TOWN FORGE", 20, 16, 22, kColorHeading);
        DrawUIText("Your power comes from what you build", 20, 40, 13, DARKGRAY);

        // Reset (top-right corner): first click arms it, a second click within 3s
        // confirms — a small safety net the JS's single-click reset link didn't have.
        bool resetArmed = resetArmedTimer > 0.0f;
        std::string resetLabel = resetArmed ? "Confirm?" : "Reset";
        if (Button({ (float)(screenW - 78), 16, 58, 22 }, resetLabel, !encounterPending && !state.combat.has_value())) {
            if (resetArmed) { ResetGame(state); resetArmedTimer = 0.0f; }
            else resetArmedTimer = 3.0f;
        }

        // Resource HUD (mirrors .resources pill row in the HTML) — shown on all screens
        std::string hud = TextFormat("Gold: %d   Wood: %d   Ore: %d   Leather: %d",
                                       state.gold, state.wood, state.ore, state.leather);
        DrawUIText(hud.c_str(), 20, 60, 15, kColorText);

        // Top tab bar (mirrors the JS bottom-nav tabs), on its own row now that
        // there are 9 of them.
        bool tabsEnabled = !state.combat.has_value() && !encounterPending;
        // Hunt tab hidden 2026-09-23 at Mark's request — now that every dungeon has a
        // real Wilderness entrance you can walk to (see kWildernessDungeonEntrances),
        // the tab was just a redundant "teleport straight to a dungeon picker"
        // shortcut, and looting/skinning already work identically in live Wilderness
        // combat (EndWildMonsterWin already pushes a corpse with real leather, same as
        // dungeons). The Hunt *screen* itself is untouched and still fully reachable by
        // walking into a Wilderness dungeon entrance (tryEnterDungeon still sets
        // Screen::Hunt) — only this tab-bar shortcut is gone. Single switch, not a
        // deletion, same pattern as kAmbushSystemEnabled/kBloodstainedRoadEnabled.
        // Remaining tabs shift left to fill the gap (via `tabX` not advancing past
        // Hunt's slot) rather than leaving an empty space in the bar.
        static const bool kHuntTabEnabled = false;
        float tabX = 20.0f;
        Rectangle charTab   = { tabX, 84, 53, 26 }; tabX += 56;
        Rectangle townTab   = { tabX, 84, 53, 26 }; tabX += 56;
        Rectangle craftTab  = { tabX, 84, 53, 26 }; tabX += 56;
        Rectangle huntTab   = { tabX, 84, 53, 26 }; if (kHuntTabEnabled) tabX += 56;
        Rectangle magicTab  = { tabX, 84, 53, 26 }; tabX += 56;
        Rectangle petsTab   = { tabX, 84, 53, 26 }; tabX += 56;
        Rectangle bankTab   = { tabX, 84, 53, 26 }; tabX += 56;
        Rectangle houseTab  = { tabX, 84, 53, 26 }; tabX += 56;
        Rectangle skillsTab = { tabX, 84, 53, 26 };
        if (Button(charTab, "Char", tabsEnabled)) state.screen = Screen::Character;
        if (Button(townTab, "Town", tabsEnabled)) state.screen = Screen::Town;
        if (Button(craftTab, "Craft", tabsEnabled)) {
            Screen target = Screen::Craft;
            GuardZoneConfiscateIfMurderer(state, target);
            state.screen = target;
        }
        if (kHuntTabEnabled && Button(huntTab, "Hunt", tabsEnabled)) state.screen = Screen::Hunt;
        if (Button(magicTab, "Magic", tabsEnabled)) state.screen = Screen::Magic;
        if (Button(petsTab, "Pets", tabsEnabled)) state.screen = Screen::Pets;
        if (Button(bankTab, "Bank", tabsEnabled)) state.screen = Screen::Bank;
        if (Button(houseTab, "House", tabsEnabled)) state.screen = Screen::House;
        if (Button(skillsTab, "Skills", tabsEnabled)) state.screen = Screen::Skills;

        if (state.ambush.has_value()) {
            DrawAmbushPanel(state, screenW);
        } else if (state.innocentEncounter.has_value()) {
            DrawInnocentPanel(state, screenW);
        } else if (state.screen == Screen::Character) {
            DrawCharacterScreen(state, screenW, screenH);
        } else if (state.screen == Screen::Town) {
            DrawTownScreen(state, screenW, screenH);
        } else if (state.screen == Screen::Hunt) {
            DrawHuntScreen(state, screenW, screenH);
        } else if (state.screen == Screen::Craft) {
            DrawCraftScreen(state, screenW, screenH);
        } else if (state.screen == Screen::Magic) {
            DrawMagicScreen(state, screenW, screenH);
        } else if (state.screen == Screen::Pets) {
            DrawPetsScreen(state, screenW, screenH);
        } else if (state.screen == Screen::Bank) {
            DrawBankScreen(state, screenW, screenH);
        } else if (state.screen == Screen::House) {
            DrawHouseScreen(state, screenW, screenH);
        } else if (state.screen == Screen::Skills) {
            DrawSkillsScreen(state, screenW, screenH);
        } else if (state.screen == Screen::Provisioner) {
            // No tab-bar button and not in the Tab-key cycle below — same treatment as
            // Wilderness, reached only by walking to the building (see DrawTownScreen's
            // AmenityLink for Provisioner). The tab bar's 9 buttons already fill the row.
            DrawProvisionerScreen(state, screenW, screenH);
        } else {
            DrawWildernessScreen(state, screenW, screenH);
        }

        // Log line (mirrors the JS log panel) — shown on all screens
        DrawUIText(state.logLine.c_str(), 20, screenH - 30, 13, Color{ 90, 74, 52, 255 });
        DrawNotorietyFooter(state, screenW, screenH);

#ifndef __EMSCRIPTEN__
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK); // letterbox color; shouldn't actually show since the aspect ratio matches exactly
        // RenderTexture2D textures are Y-flipped relative to a normal draw — negative
        // source height corrects it (the standard raylib render-to-texture pattern).
        DrawTexturePro(g_zoomTarget.texture,
                         { 0, 0, (float)kScreenW, -(float)kScreenH },
                         { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() },
                         { 0, 0 }, 0.0f, WHITE);
#endif
        EndDrawing();
    }
}

static void CleanupAndClose() {
    GameState& state = g_state;
    SaveGame(state); // final save on quit
    if (g_assets.uiFontOk) UnloadFont(g_assets.uiFont);
    if (g_assets.playerOk) UnloadTexture(g_assets.player);
    for (int i = 0; i < 10; i++) if (g_assets.buildingOk[i]) UnloadTexture(g_assets.building[i].second);
    for (int i = 0; i < 10; i++) if (g_assets.townBuildingOk[i]) UnloadTexture(g_assets.townBuilding[i].second);
    for (int i = 0; i < 5; i++) if (g_assets.monsterFamily[i].ok) UnloadTexture(g_assets.monsterFamily[i].tex);
    for (int i = 0; i < 5; i++) if (g_assets.bossFamilyOk[i]) UnloadTexture(g_assets.bossFamily[i]);
    for (int i = 0; i < 5; i++) if (g_assets.dungeonFloorThemedOk[i]) UnloadTexture(g_assets.dungeonFloorThemed[i]);
    for (int i = 0; i < 5; i++) if (g_assets.dungeonWallThemedOk[i]) UnloadTexture(g_assets.dungeonWallThemed[i]);
    if (g_assets.hollowWarrensRugOk) UnloadTexture(g_assets.hollowWarrensRug);
    if (g_assets.hollowWarrensTorchOk) UnloadTexture(g_assets.hollowWarrensTorch);
    for (int i = 0; i < 4; i++) if (g_assets.craftFloorThemedOk[i]) UnloadTexture(g_assets.craftFloorThemed[i]);
    for (int i = 0; i < 4; i++) if (g_assets.craftWallThemedOk[i]) UnloadTexture(g_assets.craftWallThemed[i]);
    if (g_assets.provisionerFloorOk) UnloadTexture(g_assets.provisionerFloor);
    if (g_assets.provisionerWallOk) UnloadTexture(g_assets.provisionerWall);
    if (g_assets.groundGrassOk) UnloadTexture(g_assets.groundGrass);
    if (g_assets.groundDirtOk) UnloadTexture(g_assets.groundDirt);
    if (g_assets.dungeonWallOk) UnloadTexture(g_assets.dungeonWall);
    if (g_assets.dungeonFloorOk) UnloadTexture(g_assets.dungeonFloor);
    if (g_assets.buildingDoorOk) UnloadTexture(g_assets.buildingDoor);
    for (size_t i = 0; i < g_assets.paperdollTex.size(); i++) if (g_assets.paperdollOk[i]) UnloadTexture(g_assets.paperdollTex[i]);
    for (size_t i = 0; i < g_assets.itemIconTex.size(); i++) if (g_assets.itemIconOk[i]) UnloadTexture(g_assets.itemIconTex[i]);
    for (const SpriteSheet* sheet : { &g_assets.knightIdle, &g_assets.knightAttack1, &g_assets.knightAttack2,
                                        &g_assets.knightHurt, &g_assets.knightDefend, &g_assets.knightProtect,
                                        &g_assets.skeletonIdle, &g_assets.skeletonAttack1, &g_assets.skeletonHurt })
        if (sheet->ok) UnloadTexture(sheet->tex);
    if (g_assets.heroSheet.ok) UnloadTexture(g_assets.heroSheet.tex);
    for (const SpriteSheet* sheet : { &g_assets.doorSmith, &g_assets.doorCarpenter, &g_assets.doorTailor, &g_assets.doorAlchemy })
        if (sheet->ok) UnloadTexture(sheet->tex);
    if (g_assets.fencePostOk) UnloadTexture(g_assets.fencePost);
    if (g_assets.farmlandOk) UnloadTexture(g_assets.farmland);
    if (g_assets.townFountainOk) UnloadTexture(g_assets.townFountain);
    if (g_assets.townStatueOk) UnloadTexture(g_assets.townStatue);
    if (g_assets.townAutumnBushOk) UnloadTexture(g_assets.townAutumnBush);
    if (g_assets.spellIconOffensiveOk) UnloadTexture(g_assets.spellIconOffensive);
    if (g_assets.spellIconDebuffOk) UnloadTexture(g_assets.spellIconDebuff);
    if (g_assets.spellIconBuffOk) UnloadTexture(g_assets.spellIconBuff);
    if (g_assets.spellIconUtilityOk) UnloadTexture(g_assets.spellIconUtility);
    for (int i = 0; i < (int)g_assets.spellIconPerSpell.size(); i++) if (g_assets.spellIconPerSpellOk[i]) UnloadTexture(g_assets.spellIconPerSpell[i]);
    if (g_assets.gearIconSwordOk) UnloadTexture(g_assets.gearIconSword);
    if (g_assets.gearIconShieldOk) UnloadTexture(g_assets.gearIconShield);
    if (g_assets.gearIconShield2Ok) UnloadTexture(g_assets.gearIconShield2);
    if (g_assets.gearIconHelmetOk) UnloadTexture(g_assets.gearIconHelmet);
    if (g_assets.gearIconGauntletOk) UnloadTexture(g_assets.gearIconGauntlet);
    if (g_assets.gearIconAmuletOk) UnloadTexture(g_assets.gearIconAmulet);
    if (g_assets.townLampOk) UnloadTexture(g_assets.townLamp);
    if (g_assets.townSignSmithOk) UnloadTexture(g_assets.townSignSmith);
    if (g_assets.townStall1Ok) UnloadTexture(g_assets.townStall1);
    if (g_assets.townStall2Ok) UnloadTexture(g_assets.townStall2);
    if (g_assets.townStall3Ok) UnloadTexture(g_assets.townStall3);
    if (g_assets.townLumberpileOk) UnloadTexture(g_assets.townLumberpile);
    if (g_assets.townBarrelOk) UnloadTexture(g_assets.townBarrel);
    if (g_assets.townCrateOk) UnloadTexture(g_assets.townCrate);
    if (g_assets.townAnvilOk) UnloadTexture(g_assets.townAnvil);
    if (g_assets.sunkenCryptWaterOk) UnloadTexture(g_assets.sunkenCryptWater);
    if (g_assets.emberveilBrazierOk) UnloadTexture(g_assets.emberveilBrazier);
    if (g_assets.wildTreeOk) UnloadTexture(g_assets.wildTree);
    if (g_assets.wildRockOk) UnloadTexture(g_assets.wildRock);
    for (int i = 0; i < (int)g_assets.wildCreatureTex.size(); i++) if (g_assets.wildCreatureTex[i].ok) UnloadTexture(g_assets.wildCreatureTex[i].tex);
    for (int i = 0; i < (int)g_assets.wildPropTex.size(); i++) if (g_assets.wildPropTexOk[i]) UnloadTexture(g_assets.wildPropTex[i]);
    for (int i = 0; i < 3; i++) if (g_assets.wildOreTexOk[i]) UnloadTexture(g_assets.wildOreTex[i]);
    if (g_assets.wildBush1Ok) UnloadTexture(g_assets.wildBush1);
    if (g_assets.wildBush2Ok) UnloadTexture(g_assets.wildBush2);
    if (g_assets.wildFern1Ok) UnloadTexture(g_assets.wildFern1);
    for (int i = 0; i < 5; i++) if (g_assets.wildMonsterTex[i].ok) UnloadTexture(g_assets.wildMonsterTex[i].tex);
    if (g_assets.rivalAdventurerSheet.ok) UnloadTexture(g_assets.rivalAdventurerSheet.tex);
    for (int i = 0; i < 5; i++) if (g_assets.wildEntranceTexOk[i]) UnloadTexture(g_assets.wildEntranceTex[i]);
#ifndef __EMSCRIPTEN__
    UnloadRenderTexture(g_zoomTarget);
#endif
    CloseWindow();
}

int main() {
    std::srand((unsigned)std::time(nullptr));
#ifndef __EMSCRIPTEN__
    InitWindow((int)(kScreenW * kZoom), (int)(kScreenH * kZoom), "Town Forge");
    SetMouseScale(1.0f / kZoom, 1.0f / kZoom); // see kZoom's comment
    g_zoomTarget = LoadRenderTexture(kScreenW, kScreenH);
#else
    InitWindow(kScreenW, kScreenH, "Town Forge");
#endif
    SetTargetFPS(60);
    LoadGameAssets(); // must come after InitWindow — texture loading needs a graphics context

#ifdef __EMSCRIPTEN__
    // Kick off the async IndexedDB mount+load now; LoadGame itself is deferred to
    // inside UpdateDrawFrame until JS_PersistReady() confirms the load landed, rather
    // than called here synchronously — see the comments above kSaveFilePath and at the
    // top of UpdateDrawFrame.
    JS_InitPersistence();
    // The browser's requestAnimationFrame drives each call; no blocking loop, no
    // WindowShouldClose() polling, no ASYNCIFY needed — see UpdateDrawFrame's comment.
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    bool hadSave = LoadGame(g_state); // applies offline Auto-Gather catch-up internally
    if (hadSave && !g_state.logLine.empty() && g_state.logLine == "Welcome to Town Forge.")
        g_state.logLine = "Welcome back.";
    while (!WindowShouldClose()) UpdateDrawFrame();
    CleanupAndClose();
#endif
    return 0;
}
