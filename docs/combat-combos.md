# Combo moves

Two moves in quick succession can make a third. Dash and then attack, and the swing becomes a lunge. Attack and then dash, and the dash cuts everything it passes. The order matters: the same two keys pressed the other way round are a different combo.

This file is the design. The code is `code/sim/player_abilities/combos.cpp`, and the tests are `code/tests/player_combo_tests.cpp`.

## What fluid means here

Combos exist to make fighting feel like one long motion instead of a row of separate button presses. Every rule below follows from that.

1. **No new keys.** A combo is made of moves the player already has. Nobody learns a button. They find out that dashing into a swing does something new.
2. **The window starts when the first move starts.** A dash takes about a quarter second, and attacking anywhere inside it, or a little after, is the lunge. Players press early, so a window that starts when a move ends feels unresponsive.
3. **The combo never makes the player wait.** A lunge keeps the dash's speed and does not root the player the way a plain swing does. A cutting dash is still a full dash. A combo replaces or adds to a move but never adds a wind-up.
4. **Combos chain.** The trail records a combo under the move it ended on: a lunge counts as an attack and a cutting dash counts as a dash. So attack, dash, attack, attack, attack runs cutting dash, lunge, second swing, finisher, without a pause.
5. **Every combo shows its name.** The move gets a burst of its own and its name pops over the player's head for half a second, so a player who found one by accident can find it again.

## The combos

| Combo | Moves | Window | What it does |
|---|---|---|---|
| Lunge | dash, attack | 0.35 s | A long narrow thrust instead of the swing: about twice the sword's reach in a 50 degree slice, one and a half times the damage, and a shove along the aim. No root, so the dash carries the player through. It opens the sword chain, so the next two swings are the second hit and the finisher. |
| Skewer | jump, dash, attack | 0.45 s | The lunge done from the air. It throws everything it hits straight up and stuns it, which sets up a juggle. It beats the lunge because the longer match wins. |
| Cutting dash | attack, dash | 0.4 s | The dash spins the blade around the player: everything within 40 units of the dash's path is hit, thrown to the side and stunned for a moment. |
| Flame fan | dash, cast | 0.35 s | Three fireballs in a 30 degree fan instead of one. |
| Long jump | dash, jump | 0.3 s | The jump keeps the dash's speed (at least 380) and the air drag drops to a fifth until the player lands, so the jump goes about twice as far. It is the way across a gap. |
| Ambush | blink, attack | 0.5 s | The swing after a blink is the finisher straight away (the throw up and the stun), whatever the chain was. |

## The system

### The move trail

Each player keeps its last four moves, newest first, and the seconds since each one started (`ComboTrail` and `ComboTrailAge` in `player_fields.inc`). A move is one of jump, dash, blink, slam, attack or cast (`combo_move` in `player.h`). Walking is not a move, and area abilities are not moves yet.

A move is recorded when it actually happens, not when its key goes down. An attack that waits in the action queue for the previous swing to end is recorded when it swings. The window is measured between the moves themselves, so the key timing doesn't matter.

### The combo table

`PlayerCombos` in `combos.cpp` has one row per combo:

- the name drawn over the player,
- up to three moves in order,
- the window: the most seconds allowed between one move and the next,
- the burst drawn when it fires (`sim_burst`, `events.h`),
- whether it moves only the player (see prediction below),
- the function that does it.

### When a combo fires

Every move goes through one call, `RunPlayerCombo(Player, Move)`, at the moment it starts. It looks for the row that ends in this move and whose earlier moves are the newest entries of the trail, each within the window. The longest match wins. Then it records the move in the trail, whether or not a combo fired.

What a found combo does depends on the kind of move it ends in. The rule has no exceptions:

- **It ends in an attack or a cast.** The combo replaces the swing or the fireball. The move's timing (root, interval, sound) comes from its spawn action row as usual. The combo decides what gets spawned.
- **It ends in a dash, blink, slam or jump.** The move happens as normal, and the combo runs right after it. The cutting dash is still a dash, and the long jump is still a jump.

So no ability needs to know which combos exist. Each one makes a single `RunPlayerCombo` call.

### Prediction and online play

Online, the client moves its own player before the server answers (`client/prediction.cpp`): jump and the movement abilities, but not attacks or casts. For combos, this means:

- The trail is part of what a replay restores (`predicted_body`), like the jumps spent.
- Attack and cast presses are not run while predicting. They are still marked in the trail, so the client and the server agree on what came before a dash.
- A combo that moves only the player (the long jump) runs in the prediction, and its burst is drawn by the client itself. A combo that touches anyone else (the cutting dash's hits) runs only on the server. While predicting it only records the move.
- Combos that end in an attack or a cast already run only on the server, like every swing.

The combo bursts are new `sim_burst` values. The snapshot already carries bursts as one byte each, so the network protocol does not change.

### What it looks like

Each combo has a burst row in `client/fx_bursts.cpp`. Two new shapes are added: a thrust (a line of dots out along the aim, for the lunge and the skewer) and a spin (dots circling a centre that travels along the dash, for the cutting dash). When a burst that belongs to a combo arrives, `fx_bursts.cpp` also shows the combo's name over that player's head.

## Adding a combo

1. Add a row to `PlayerCombos`: name, moves, window, burst, and whether it moves only the player.
2. Write its function next to the others in `combos.cpp`. If it is a new sword cut, add a row to `SwordCuts` in `sword.cpp` and call `SwingSwordCut`.
3. If it needs its own look, add a `sim_burst` at the end of the list in `events.h` and its row in `BurstLooks`.
4. Add a test to `player_combo_tests.cpp` that presses the moves and checks what happened.

A new kind of move, such as an area ability, needs a `combo_move` value and one `RunPlayerCombo` call where that move starts.

## Later

- Area abilities as moves (push, then dash).
- A short glow on the player while a combo window is open, as a hint that one is available.
- A list of the combos in the controls panel.
