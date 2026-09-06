# Architecture notes

How the game is layered, and the conventions it follows. The short
version lives in the [README](../README.md).

## Layers

```
main.c        OS glue only: window, input edges, audio playback
render.c      raylib replay of view items; owns all API conversion
core/view.c   draw-item kernel + shared animation clocks
core/world.c  room load (objects -> solid map + entities), tick order
core/solid.c  the one collision vocabulary: cell -> occupant kind
core/undo.c   the move history: verbatim board snapshots
core/rng.h    one xorshift per consumer (sim, fx, butterfly, house)
core/sim_math.h  shared float math (lengthdir/point_direction/lerp)
src/objects/  one script per game object; owns state, rules, view
level_data.*  room tables (source of truth, never edited)
```

Data flows one way per frame: the front-end translates OS events into a
`SimInput` (pressed edges, one tick wide), `sim_tick()` runs the rules,
object scripts push draw items tagged with depth — the fixed per-object
`VIEW_DEPTH_*` constants; push order breaks ties — and the backend
replays them. Rules never touch raylib; the renderer never decides
gameplay.

## Design rules

These are the rules of thumb that keep the simulation correct, plus the
failure modes they prevent. Each one exists because ignoring it
produced a real bug.

**Collision derives from footprint geometry, centrally.** Walls, holes,
doors, the goal and the dog's body all stamp their footprint into the
solid map through one helper, `mark_footprint()`, and rooms stamp walls
*scaled* (`xscale`/`yscale` are cell counts). The overlap rule is
strict: touching edges do not collide. Rendering a single cell per
object instead made most walls walkable — the map must be stamped from
the full scaled footprint. Per-kind semantics stay with the owning
script (`solid.c` decides what a kind *means* for a probe; `hole.c`
decides when a hole stops being solid).

**Input is press edges.** Movement is one cell per physical press and
repeats are gated with a two-tick `key_cooldown`. `SimInput` carries no
held state at all. Feeding held keys with a repeat timer instead
changed the game's feel and let the dog slide continuously.

**View state is born at placement.** A placed object snaps its eased
view position to its cell (`view_snap()` in each script). The eased
values exist to smooth *movement*; initialising them at zero made every
object glide in from (0,0) on room load.

**Sprite identity comes from the object table, not the name.** Names
lie: the skull item draws `sprPear` (8 frames @ 16 fps), and the
`sprSkull` asset is unreferenced. When a drawn object looks wrong,
check the object-to-sprite mapping before the sprite table.

**The backend converts, the scripts speak the view vocabulary.** View
items carry the view conventions — angles in degrees, positive =
counterclockwise on screen; positions at the sprite origin;
sprite-part semantics (source rect clipped, top-left destination, no
origin). The raylib replay negates the rotation once, at the single
`DrawTexturePro` call site. Converting at push sites instead mirrored
every rotated sprite (the bark).

**Multi-source draws must be recreated whole.** The house is two draw
sources: the full `sprHouse` (64x64, origin (32,64), so the anchor is
the bottom-centre), then a second pass redraws the top 44 rows with the
squash pulse. Porting only the crop or mis-deriving the anchor left the
house floating and half-drawn. Object positions in `level_data.h` are
exact multiples of the cell size, so cell-packing round-trips
losslessly when the stored cell and the offset (+8/+32 here) are
derived from the spawn math, not guessed.

**The solid map must stay in lockstep with the chain.** Occupancy is
keyed by cell and the tail exception probes `dog_part_is_solid()` by
part index, so every chain mutation (step, grow, shrink) re-stamps the
whole chain with fresh indices. Clearing only the moved cells leaves
ghost solids (a cell a part vacated stays solid forever) or stale
indices (the tail resolves to the wrong part's flags).

**Undo is a verbatim time-machine, captured at the one board-changing
site.** Every gameplay script owns its snapshot (a state struct plus
`*_capture`/`*_restore`, so adding a field automatically joins the
history); `core/undo.c` owns the stack. The capture wraps `try_step()`
in dog.c — the only place a move mutates the board — before the first
mutation, and commits only when the step actually moves, so strained
attempts leave no phantom undo points. The solid map is captured
verbatim instead of re-stamped from the owners on restore: replaying
exactly what was recorded keeps undo free of stamp-order questions.
Everything that is not board state — rng, fx, view easing, dialogue,
the transition — keeps running through an undo (a restored dog snaps
its eased view position, like a fresh placement), and a room load
drops the history.

**Presentation substitutions are deliberate and documented.** These
departures from the literal sprite data exist because it reads badly at
modern window sizes:
- *Text*: a bitmap font at the 304x208 surface turns to mush, so UI
  text replays in the present pass at window resolution with Renogare
  (repo `assets/fonts`, converted to TrueType outlines and rasterized
  at the drawn pixel size with point filtering — no blur). The in-world
  house counter keeps the pixel digits bitmap font on purpose: it
  should stay pixelated with the game. Alignment: font 0 bold =
  left-aligned, fonts 1/2 = centered.
- *Dialogue panel*: the `sprDialogueBox` sheet is a 24x24 frame; the
  reconstruction renders a 9-slice instead (corners native, sides and
  centre stretched) over the same rect.
- *Ground layer*: the `Tiles_1`/`GroundTileSet` layer is removed fully
  (data, draw call, texture) — it only stamped sparse dirt decals over
  the `sprTile` background.
- *Unpressed buttons* animate through all 9 `sprButton` frames on the
  shared 8 fps clock; the pressed state holds the single-frame
  `sprButtonPressed`.

**Deliberate collision simplifications.** Two places where the
collision geometry is simplified, because the literal behaviour read as
a bug in play:
- *Buttons*: the box sprite is 16x20 — one cell tall plus a 4px lid —
  so a box one cell below a button visually overlaps it. Boxes press
  only from the button's own cell.
- *House*: `sprHouse` is a full 64x64 image from the roof peak. The
  solid footprint follows the house walls only (48x32, centred on the
  anchor from the stored cell down — the sprite's walls span x 9..54)
  so the roof and eaves overhang stay walkable background.

**Simulation constants are part of the rules.** Cooldowns (2 ticks),
lerp factors (0.2 dog, 0.25 box/door), pulse counts (200 stepping by
4), bark cadence (25 ticks, 20% chance) are game data, not taste; keep
them named next to the rule they serve.

## Testing

`tests/game_smoke.c` runs the sim headlessly through the real room
data: title flow, dialogue gating, press cadence + cooldown, chain
follow, the tail exception (a length-3 dog looping on four cells),
scaled-wall stamping, pear sprites, hole rendering + the filled frame,
the 9-slice bubble, house anchors, view snap, box push/hole fill,
button/door counting, the win transition, retry, and undo (steps, box
pushes into holes, eaten apples, a fatal pear, and history dropped on
a room load). View-level asserts inspect the pushed `ViewItem`s
directly — no renderer needed.
`tests/playthrough.c` replays a timed input script and prints states
for manual review.
