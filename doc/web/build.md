# Web Build (Emscripten)

## Requirements
- Emscripten SDK (emsdk) installed and activated.
- CMake 3.23+ and Ninja.
- Python 3 (used by font conversion scripts).

## Configure + Build
From the repo root:

1) Configure using the preset:
- Emscripten (single-threaded):
  - Configure preset: `emscripten-release`
  - Build preset: `emscripten-release`
- Emscripten (PThreads):
  - Configure preset: `emscripten-pthreads-release`
  - Build preset: `emscripten-pthreads-release`

2) Run CMake in the source directory:
- `cmake --preset=emscripten-release`
- `cmake --build --preset=emscripten-release`

For PThreads:
- `cmake --preset=emscripten-pthreads-release`
- `cmake --build --preset=emscripten-pthreads-release`

The output will be placed in:
- `build/emscripten-release/` or
- `build/emscripten-pthreads-release/`

## Output Files
Typical output includes:
- `starbound.html`
- `starbound.js`
- `starbound.wasm`
- `starbound.data`
- `preload.config`

These files are the full web distribution. Copy them into a hosting folder as-is.

## Notes
- The web shell template is [web/emscripten_shell.html](../../web/emscripten_shell.html). Emscripten uses it to generate `starbound.html`.
- The post-build patcher [web/patch_emscripten_package_name.py](../../web/patch_emscripten_package_name.py) fixes `PACKAGE_NAME` and adds a runtime guard for proxied async calls.
- Fonts in `assets/opensb/font/*.woff2` are converted to TTF for web builds (see [web/convert_woff2_fonts.py](../../web/convert_woff2_fonts.py)).
