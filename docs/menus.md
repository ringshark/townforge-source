# Town Forge menus

The MENU opens Character, Crafting, Guild, Journal and Settings. Return to World restores the current town, wilderness or dungeon. Combat, ghost and death-animation navigation restrictions still apply; Settings remains accessible.

Character has Gear, Pack and Skills tabs. Pack expands the existing inventory and links to Bank storage and Home/settlement. Skills links to Magic and Pets. The paperdoll and equipped items still use the same character renderer and inventory logic.

Crafting separates workshop selection from Craft, Buy, Orders, Sell and Recycle. Dye is enabled at the Tailor. Recipe cards show power/effect, materials, skill requirements and a concrete reason when unavailable. Crafting and buying use their existing functions, resource costs and quality rules. Alchemy retains its potion pouch; bandage crafting remains at the Tailor. Sell and Recycle list only backpack weapons and armor and retain the existing sale and recovery formulas.

Guild opens the physical city. Buildings open their existing actions; countdowns appear on queued buildings. Aid, Hall and Warband captions point to available help, gifts and activity rewards. The summary and navigation badges derive from server hub eligibility: claimed, failed, unjoined and ongoing activities do not advertise rewards. Help excludes own requests, already-helped requests and full requests. When a construction countdown expires, the city requests fresh server state rather than granting an upgrade locally. Zoom controls do not also pick a building.

Journal combines Quests, Weekly goals, Maps and Events. Events show the existing adventure journal, newest first. Quest badges count completed bounties and town tasks. Settings contains readable, scrollable preferences, plus Help, cloud Save and the existing two-press Reset confirmation. Closing Guild, Journal or Settings returns to the world.

Input routing, scissor clipping and press/release ownership remain enforced. Validation includes the Emscripten build, production input/movement/guild tests, `tests/menu-status.py`, and browser checks of navigation, sale/recycling, event history and menu button intersections. Browser guild screens use a simulated server hub; this update requires no database migration or economy change.
