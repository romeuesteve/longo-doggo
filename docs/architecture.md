# Architecture notes

How the reconstruction is layered, and the recovery conventions the
ports follow. The short version lives in the [README](../README.md).

## Layers

```
main.c        OS glue only: window, input edges, audio playback
render.c      raylib replay of view items; owns all API conversion
core/view.c   draw-item kernel + shared animation clocks
core/world.c  room load (instances -> solid map + entities), tick order
core/solid.c  the one collision vocabulary: cell -> occupant kind
src/objects/  one script per GameMaker object; owns state, rules, view
level_data.*  rooms dumped from data.win (source of truth, never edited)
```

Data flows one way per frame: the front-end translates OS events into a
`SimInput` (pressed edges, one tick wide), `sim_tick()` runs the rules,
object scripts push draw items tagged with depth + order, and the
backend replays them. Rules never touch raylib; the renderer never
decides gameplay.

## Recovery conventions

These are the rules of thumb that keep the ports literal, plus the
failure modes they prevent. Each one exists because ignoring it
produced a real bug.

**Collision derives from instance geometry, centrally.** In data.win,
`oHole`, `oBox`, `oDoor`, `oGoal` and `oDogPart` are all children of
`oBlock`, and rooms stamp `oBlock` *scaled* (`xscale`/`yscale` are cell
counts). `load_room()` rasterises every such bbox into the solid map
through one helper, `mark_footprint()` (GameMaker's strict-overlap rule:
touching edges do not collide). Rendering a single cell per instance
instead made most walls walkable — the map must be stamped from the
full scaled bbox. Per-kind semantics stay with the owning script
(`solid.c` decides what a kind *means* for a probe; `hole.c` decides
when a hole stops being solid).

**Input is press edges.** The original dog step reads
`keyboard_check_pressed` and gates repeats with `key_cooldown`
(`alarm[1] = 2`). Movement is therefore one cell per physical press;
"held" state is reserved for consumers that genuinely need it. Feeding
held keys with a repeat timer instead changed the game's feel and
let the dog slide continuously.

**View state is born at placement.** A placed entity snaps its eased
view position to its cell (`view_snap()` in each script). The original
lerps (`xx/yy`, 0.2/0.25) exist to smooth *movement*; initialising the
eased value at zero made every object glide in from (0,0) on room load.

**Sprite identity comes from the recovered object table.**
`recovered/analysis/ground-truth.txt` records each object's
`sprite=` from data.win. Names lie: `oSkull` draws `sprPear` (8 frames
@ 16 fps), and the `sprSkull` asset is unreferenced. When a drawn
object looks wrong, check the object table before the sprite table.

**The backend converts, the scripts speak GameMaker.** View items carry
GameMaker conventions — angles in degrees, positive = counterclockwise
on screen; positions at the sprite origin; `draw_sprite_part_ext`
semantics (source rect clipped, top-left destination, no origin). The
raylib replay negates the rotation once, at the single
`DrawTexturePro` call site. Converting at push sites instead mirrored
every rotated sprite (the bark).

**Multi-source draws must be ported whole.** The house is two draw
sources in the original: `oGoal` draws the full `sprHouse` (64x64,
origin (32,64), so the anchor is the bottom-centre), then `oGoalUp`
redraws the top 44 rows with the squash pulse. Porting only the crop
or mis-deriving the anchor left the house floating and half-drawn.
Instance positions in `level_data.h` are exact multiples of the cell
size, so cell-packing round-trips losslessly when the stored cell and
the offset (+8/+32 here) are derived from the spawn math, not guessed.

**Simulation constants come from the GML, not from taste.** Cooldowns
(`alarm[1] = 2`), lerp factors (0.2 dog, 0.25 box/door), pulse counts
(200 stepping by 4), bark cadence (alarm 25, 20% chance) are all
recovered values; keep them named next to the rule they serve.

## Testing

`tests/game_smoke.c` runs the sim headlessly through the real room
data: title flow, dialogue gating, press cadence + cooldown, chain
follow, scaled-wall stamping, pear sprites, house anchors, view snap,
box push/hole fill, button/door counting, the win transition and retry.
View-level asserts inspect the pushed `ViewItem`s directly — no
renderer needed. `tests/playthrough.c` replays a timed input script
and prints states for manual comparison against the original build.
