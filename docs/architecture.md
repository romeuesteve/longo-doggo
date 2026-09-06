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

**The solid map must stay in lockstep with the chain.** Occupancy is
keyed by cell and the tail exception probes `dog_part_is_solid()` by
part index, so every chain mutation (step, grow, shrink) re-stamps the
whole chain with fresh indices. Clearing only the moved cells leaves
ghost solids (a cell a part vacated stays solid forever) or stale
indices (the tail resolves to the wrong part's flags).

**Presentation substitutions are deliberate and documented.** These
departures from the recovered presentation exist because the literal
port reads badly at modern window sizes:
- *Text*: the recovered fonts are an 8px "DejaVu Sans" bitmap atlas
  that turns to mush inside the 304x208 surface, so UI text replays in
  the present pass at window resolution with Renogare (repo
  `assets/fonts`, converted to TrueType outlines and rasterized at the
  drawn pixel size with point filtering — no blur). The in-world house
  counter keeps the recovered `FontDigits` bitmap font on purpose: it
  should stay pixelated with the game. Alignment still follows the
  recovered draw state (font 0 bold = left-aligned, fonts 1/2 =
  centered).
- *Dialogue panel*: the original scales the whole 24x24
  `sprDialogueBox`; the reconstruction renders a 9-slice instead
  (corners native, sides and centre stretched) over the same rect.
- *Ground layer*: the `Tiles_1`/`GroundTileSet` layer is removed fully
  (data, draw call, texture) — it only stamped sparse dirt decals over
  the `sprTile` background.
- *Unpressed buttons* animate through all 9 `sprButton` frames on the
  shared 8 fps clock; the pressed state holds the single-frame
  `sprButtonPressed`.

**Known deviations from the recovered collision data.** Two places
where the port intentionally diverges from what the dumped geometry
produced, because the literal behaviour read as a bug in play:
- *Buttons*: the recovered box mask is the fully opaque 16x20 sprite,
  so a box one cell below a button pressed it through a 4px lid
  overlap. Boxes now press only from the button's own cell.
- *House*: the recovered `sprHouse` has a full-image automatic mask
  (64x64 from the roof peak). The solid footprint follows the house
  walls only (48x32, centred on the anchor from the stored cell down —
  the sprite's walls span x 9..54) so the roof and eaves overhang stay
  walkable background.

**Simulation constants come from the GML, not from taste.** Cooldowns
(`alarm[1] = 2`), lerp factors (0.2 dog, 0.25 box/door), pulse counts
(200 stepping by 4), bark cadence (alarm 25, 20% chance) are all
recovered values; keep them named next to the rule they serve.

## Testing

`tests/game_smoke.c` runs the sim headlessly through the real room
data: title flow, dialogue gating, press cadence + cooldown, chain
follow, the tail exception (a length-3 dog looping on four cells),
scaled-wall stamping, pear sprites, hole rendering + the filled frame,
the 9-slice bubble, house anchors, view snap, box push/hole fill,
button/door counting, the win transition and retry. View-level asserts
inspect the pushed `ViewItem`s directly — no renderer needed.
`tests/playthrough.c` replays a timed input script and prints states
for manual comparison against the original build.
