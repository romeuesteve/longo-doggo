# Longo Doggo build

The rewrite uses C99, raylib, and CMake. raylib is fetched automatically from its
pinned upstream 6.0 archive on the first configure, so no separate raylib install
is required. The first configure needs network access.

## Native Windows build (GCC/MinGW)

Use a fresh build directory for the native target:

```powershell
cmake -S . -B build/native -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_C_COMPILER=gcc `
  -DLONGO_DOGGO_PLATFORM=native
cmake --build build/native --parallel
```

The `MinGW Makefiles` generator selects the installed GCC/MinGW toolchain. The
game executable is `build/native/longo_doggo.exe` (or the equivalent path for
the build directory you choose). The runtime assets are copied beside
it at `assets/exported-assets/`, so launching from the build directory works
without extra setup.

## Web build (Emscripten HTML5/WebAssembly)

An Emscripten SDK shell must be active so both `emcc` and `emcmake` are on `PATH`.
Configure the web target in a separate build directory:

```powershell
emcmake cmake -S . -B build/web `
  -DCMAKE_BUILD_TYPE=Release `
  -DLONGO_DOGGO_PLATFORM=web
cmake --build build/web --parallel
```

This produces `longo_doggo.html` together with its `.js` and `.wasm` files in
`build/web`. The runtime assets are copied beside the generated shell
under `build/web/assets/exported-assets/`. Serve that directory over HTTP for
browser testing; opening the HTML file directly can prevent WebAssembly or asset
loading in some browsers.

Use separate build directories for native and web builds. CMake cannot change the
compiler or platform of an already-configured build tree.
