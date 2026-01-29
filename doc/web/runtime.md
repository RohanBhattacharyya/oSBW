# Web Runtime Specifics

This describes the major differences and constraints for the web build.

## Filesystem & Persistence
- Saves/mods are stored in `IDBFS` under `/storage` and `/mods`.
- The web shell installs sync hooks to persist writes; sync runs on a timer and on tab visibility changes.
- The Save Manager UI exports/imports `/storage` and `/mods` via a `.zip` file (JSZip).

## UI Shell
- The runtime HTML UI lives in [web/emscripten_shell.html](../../web/emscripten_shell.html).
- The shell adds a console viewer and a Save Manager, and injects fullscreen handling.
- The canvas size is synced to the viewport with device pixel ratio for crisp rendering.

## Networking Limitations
- Web builds cannot accept raw TCP or platform P2P connections.
- For web builds, `clientIPJoinable` and `clientP2PJoinable` are forced off.

## WebGL
- WebGL2 only (`-sUSE_WEBGL2=1`, min/max WebGL version set to 2).

## Audio
- Emscripten uses browser audio; short sounds can be decompressed in memory.
- Some browsers require user interaction before audio can start.

## HTTP
- HTTP requests from C++ are disabled on web builds and will return an error.

## Fonts
- WOFF2 fonts are converted to TTF for web builds because Emscripten’s FreeType build may not support WOFF2.

## Threading
- PThreads builds require COOP/COEP headers and a cross-origin isolated context.
- Single-threaded builds do not require those headers but can be slower.
