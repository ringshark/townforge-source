# Button and touch routing

The Me screen's Play button occupies the same header space as Chat on the world
screen. Previously Play changed the screen, then Chat processed that same mouse
press later in the frame. The touch TARGET button also overlapped the quick-item
belt, allowing a target tap to use an item.

Shared buttons, gump buttons, close buttons, item selectors and spell slots now
consume one press through `UIClick`. Navigation cannot reuse that press on another
control. The UI retains ownership through the gesture's release, so a disappearing
button cannot turn into a world pick, walk destination or camera drag. Disabled
controls and visible panel shields also protect the world behind them.

TARGET is above the belt, with a gap between the visible bounds. Chat is skipped
when the rendered screen changes. World orbit and virtual joystick input check UI
ownership before beginning a gesture. Text fields require their own short press
and release inside the same field; scrolling or arriving from another screen
does not open a text prompt.

Drawing and input use the same scissor stack. Hidden portions of list buttons are
not clickable, and nested scissor scopes restore the parent clip. Paperdoll item
popovers block the controls beneath them. The body rotation area excludes its page
tabs, and world-only button padding is reduced to three pixels.

`python tests/ui-input.py` compiles and tests the production routing functions,
including navigation versus Chat, disabled controls, press/release ownership,
clipped lists, nested clips, panel shields, text-field gestures and TARGET bounds.
Browser QA checks the Me-to-world transition, deliberate Chat, touch targeting
without spending bandages, and enabled button layouts on Town, Wilderness,
Dungeon, Me, Craft, Magic, Skills, House, Bank and Pets.
