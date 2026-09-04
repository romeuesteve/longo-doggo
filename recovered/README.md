# Longo Doggo recovery

This folder contains a recovery copy of the Windows build downloaded from:

<https://romeuski.itch.io/longo-doggo>

## Original build

- `LongoDoggo.zip` — 10,216,219 bytes
- `LongoDoggo/data.win` — 8,827,602 bytes
- SHA-256 of `data.win`: `330AF56730DE5B70A7B15AEF3796F95F4CA6B7944BD47FD8141134B7872B7C4`
- GameMaker Studio 2 VM build, bytecode version 17, not YYC

## Recovered content

`exported-assets/` contains:

- 30 sprites / 112 PNG animation frames
- 7 WAV sound effects
- 3 font atlases with glyph CSV metadata
- 2 background/tileset PNGs
- 108 decompiled root GML entries
- `strings.json` with the game's embedded strings

`analysis/room-manifest.txt` contains the room dimensions, layers, and placed object instances. The build has 11 rooms: the title screen, tutorial, six numbered levels, credits, the level editor, and a shared level-base room.

## Tooling

The data was opened and exported with UndertaleModTool's CLI v0.9.2.0, kept in `utmt-cli/`. The official project is <https://github.com/UnderminersTeam/UndertaleModTool>.

## Important limitation

The original GameMaker project files (`.yyp`, object editor metadata, source asset files, and project settings) are not present in the download. The compiled data preserves enough to reproduce the game, but a clean editable project will need to be rebuilt. The decompiled GML is a strong reference, not guaranteed to be byte-for-byte identical to the original source.

## Reconstruction target

This workspace now uses a fresh C99 + raylib implementation with CMake. The
same code can target a native executable and, when Emscripten is installed, an
HTML/WebAssembly build. It was chosen for low-level control, small runtime
size, and one codebase for desktop and web.

## Other candidate targets

### Godot 4 — recommended if the new engine should be different

Best balance for this small 2D grid game. The recovered PNGs/WAVs can be used directly, the 16-pixel movement grid maps naturally to a tilemap or data-driven board, and the GML mechanics can be rewritten into a small set of scripts. It also gives a straightforward desktop and HTML5 path for itch.io.

### A fresh GameMaker project — shortest path to visual parity

This is not a different engine, but it is likely the fastest and most faithful restoration. The extracted GML and GameMaker-style object model can be adapted with comparatively little translation.

### MonoGame/FNA with C# — most control

Good if the goal is a code-first, dependency-light project, but it requires more work for room editing, animation, rendering, audio, and packaging.

### Phaser with TypeScript — browser-first

Good for an itch.io web build, but the room/editor workflow, pixel-perfect scaling, and runtime audio need more custom infrastructure than Godot.

Unity is possible, but it is heavier than this game needs.

## Implementation note

The reconstruction in the repository covers the title/tutorial, all six
numbered levels, credits, and the recovered core mechanics. It also renders
the original room tile layers from `analysis/room-tiles.txt`, the bitmap font
atlases, and the exported sprite/audio assets. Sprite frames preserve the
GameMaker padding/origins; the object named `oSkull` uses the original
`sprPear` art, and the dialogue panel uses a 9-slice renderer. It is
intentionally not byte-for-byte identical to the original GameMaker runtime;
the decompiled GML and room data were used as behavioral references.

Check the original licenses for the sound effects before redistributing a rebuilt version; the game's own credits mention freesound.org.
