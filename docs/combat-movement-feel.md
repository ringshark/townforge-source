# Combat and movement feel

The shared keyboard, arrow-key, joystick and tap movement path now ramps to full
speed in about 63 ms. Releasing stops immediately and reversing starts in the new
direction immediately. Keyboard diagonals remain normalized. Joystick speed rises
continuously out of its dead zone, instead of jumping to 20% speed. Tap destinations
remain bounded, and movement resets after leaving a view or pausing in a panel.
Movement delta is capped at 100 ms to limit jumps after a stalled browser frame
while retaining full walking pace down to 10 fps.

The player's rendered facing eases along the shortest angle, with faster turning
during attacks. Gameplay facing remains immediate. Automatic melee faces the target
at swing start, so the cleave arc agrees with the attack rather than the last walk.
Locomotion speed settles promptly when stopped. Moving attacks and casts retain a
continuous walking/running stride beneath the upper-body action.

Automatic wilderness and dungeon attacks now start a 320 ms animation, then resolve
the existing damage roll at contact: 128 ms for ordinary attacks, 205 ms for the
bow's release pose. DEX cooldowns, damage formulas, resource costs and rewards remain
the same. Leaving melee range during windup produces an out-of-reach miss. Death or
target switching cancels the pending strike; switching retains attack recovery.
The player animation completes before the fastest 350 ms attack cooldown.
NPC weapon-specific animation durations remain intact.

Impact pause is 28 ms for ordinary hits and 45 ms for damage of 20 or more, with
a smaller camera kick. Sparks, damage numbers, hit sounds and enemy flashes still
fire on the damage event. Enemy attack resolution remains the existing AI behavior.

Validation:

```sh
g++ -std=c++17 tests/combat-motion.cpp -o /tmp/tf-combat-test
/tmp/tf-combat-test
python tests/player-movement.py
```

The tests cover response, immediate stop/reversal, angular wrap, frame-independent
turning, one impact per windup, range/death cancellation, DEX cadence at 30/60/120 fps,
and the production movement function with keyboard, analog stick, tap arrival,
diagonal normalization, a stalled frame and switching position owners.
