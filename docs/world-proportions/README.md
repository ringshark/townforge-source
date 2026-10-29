# World proportions around the 76px player

Connected-world buildings now fit a 200px wide / 228px high art envelope (previously 96 / 94), preserving each asset's aspect ratio. Trees are 148–188px tall, town lamps are 94px, and human NPCs are 68px. The player retains the current 76px height and connected walking rig.

Each town has room for the enlarged facades. Emberhold has more depth and a narrow west boundary protecting Southfen housing; Saltmere uses a long coastal street with parallel building rows; Frostmere and Cragmoor have wider compounds. Axis-aligned 2D gate approaches fit the coastal map limits. The separate 3D view retains its road-derived orientation.

Wider paved streets and door approaches match the buildings. Solid building bases use rectangles, while roof overhangs remain walk-behind and fade when they obscure the player. Shop exits return directly to a clear point outside the correct building. Entering a shop clears the tracking overlay.

Interior floor areas remain 340–420px square. Furniture artwork is 35% larger, standard door openings widen to 88px, and NPCs use the same outdoor human proportions. Dungeon edge furniture and light fixtures are larger; dungeon floor layouts stay connected.

Validation: production web build; native landscape/navigation suite, including gate passage, flood-fill access to every shop using the new collision bases, all housing plot corners outside compounds, coordinate round trips, compact-room station access, shop exit clearance and save round trips. Chromium desktop and phone review covers all four towns, six dungeons and thirteen interiors (46 scenes), room movement, and GPU transparency checks on all ten Frostmere/Cragmoor building textures per viewport. Screenshots and browser-results.json are alongside this file.
