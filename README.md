# Longo Doggo reconstruction

A C99 + raylib reconstruction of the recovered GameMaker build of
[Longo Doggo](https://romeuski.itch.io/longo-doggo), aiming for a 1:1
behavioral match with the original. CMake drives native and Emscripten
targets from one codebase.

The simulation core (`src/game.c`) is a direct port of the decompiled GML
events in `recovered/exported-assets/code`: the same instance model (creation
order ids, depth, alarms, bbox collisions with GameMaker's strict-overlap
semantics), the same object events (`oDog` movement with the two-frame
`key_cooldown` alarm, `oDogPart` chain following, `oBox` push/block states,
`oButton`/`oDoor` press counting, `oHole` fills, `oGoalUp` remain counting,
`oTutorial` dialogue gating, `oTransition` wipes, the persistent
`oTransition`/`obj_bloom_appsrf`), and the same per-sprite animation playback
speeds recovered from `data.win`.

The renderer (`src/render.c`) reproduces the original Draw events: the
304x208 application surface, background/tile layers and instances merged in
depth order, the `oShadows` shadow surface composited at 0.2 alpha, the
`obj_bloom_appsrf` bloom (threshold 0.8, range 0.3, 5-step blur, 0.6
intensity), the GUI pass with the `oTutorial` dialogue boxes and
`oTransition` level wipes, and the exported bitmap fonts.

Rooms come from `src/level_data.h`, regenerated straight from `data.win` by
`recovered/analysis/DumpRoomData.csx`: the 11 rooms in the original runtime
order (title, tutorial, level6, level5, level4, level2, level1, level3,
credits, editor, levelbase — `GeneralInfo.RoomOrder`, so the game starts on
the title screen and `room_goto_next()` follows the table), with instances in
stored creation order, float scales, and instance-layer depths.

## Build and run on Windows

From PowerShell:

```powershell
cmake -S . -B build -G Ninja -DLONGO_DOGGO_PLATFORM=native
cmake --build build --parallel
ctest --test-dir build --output-on-failure
.\build\longo_doggo.exe
```

The build copies recovered runtime assets to `build/assets/exported-assets/`.
Controls match the original: arrow keys or WASD move (OS key repeat included),
`R` retries the room, Space barks, Enter/E/Space advance dialogues, any key
starts from the title. The editor room (unreachable in the normal flow, like
the original) uses the mouse: wheel cycles the palette, left click places,
right click deletes, Enter toggles edit/play.

`tests/game_smoke.c` exercises the simulation headlessly: title → tutorial
flow, dialogue gating, movement cadence, apple/pear length changes, box push,
hole fill, simultaneous button/door logic, the win transition order, and
retry.

## Web export

Install and activate the Emscripten SDK, then run:

```powershell
emcmake cmake -S . -B build/web -DLONGO_DOGGO_PLATFORM=web -DCMAKE_BUILD_TYPE=Release
cmake --build build/web --parallel
```

This emits `longo_doggo.html`, JavaScript, WebAssembly, and copied assets in
`build/web`. Serve that directory over HTTP for browser testing. See
[`docs/build.md`](docs/build.md) for the expanded platform notes.

## Recovery material

The original ZIP, extracted GameMaker data, exported PNG/WAV/GML content,
ground-truth dumps, room manifest, and UndertaleModTool CLI are preserved
under [`recovered`](recovered). `recovered/analysis` holds the dump scripts
and their outputs (asset tables, runtime room order, per-sprite animation
speeds, room instance data) used to drive the reconstruction.
