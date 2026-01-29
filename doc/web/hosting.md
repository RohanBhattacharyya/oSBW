# Web Hosting & Headers

## Required Headers (PThreads)
PThreads builds require **crossOriginIsolated**, which requires the following headers:

- `Cross-Origin-Opener-Policy: same-origin`
- `Cross-Origin-Embedder-Policy: require-corp`

Without them, SharedArrayBuffer is disabled and the PThreads build will fail at runtime.

## Local Dev Server
Use the included server to apply COOP/COEP headers locally:

- [web/serve_dist_with_headers.py](../../web/serve_dist_with_headers.py)

Examples:
- Serve a non-PThreads build folder:
  - `python3 web/serve_dist_with_headers.py --dir /path/to/build/emscripten-release`
- Serve the default PThreads folder:
  - `python3 web/serve_dist_with_headers.py --pthreads`

The server also disables caching (`Cache-Control: no-store`) which helps with fast iteration.

## Hosting Checklist
- Serve over HTTPS for best browser compatibility.
- Ensure the response MIME types are correct:
  - `.html` → `text/html`
  - `.js` → `text/javascript`
  - `.wasm` → `application/wasm`
  - `.data` → `application/octet-stream`
- Keep the output files together in the same directory.

## Files to Deploy
Copy all outputs from the build directory (e.g., `build/emscripten-pthreads-release/`) to the hosted folder:

- `starbound.html`
- `starbound.js`
- `starbound.wasm`
- `starbound.data`
- `preload.config`
