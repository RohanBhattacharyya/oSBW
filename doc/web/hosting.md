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

## Multiplayer (WebSocket Proxy)
Web browsers cannot open raw TCP sockets. To host multiplayer for web clients, run the WebSocket proxy and give players a `wss://` URL.

### Proxy Usage
The proxy lives at [web/opensb_wss_proxy.js](../../web/opensb_wss_proxy.js) and forwards WebSocket traffic to your TCP server.

1) Install dependencies in the web folder:
  - `cd web`
  - `npm install`
2) Run the proxy:
  - `node opensb_wss_proxy.js --listen 0.0.0.0:8080 --target 127.0.0.1:21025 --path /opensb`
3) In the web client’s Multiplayer address box, enter:
  - `ws://localhost:8080/opensb` (local/dev)
  - `ws://YOUR_HOST:8080/opensb` (LAN/dev)
  - `wss://YOUR_HOST/opensb` (production with TLS)

Notes:
- `0.0.0.0` is a bind address and cannot be used by clients. Use `localhost` or a real host name/IP.
- If the page is served over HTTPS, browsers require `wss://` (secure WebSocket).

Optional auth token:
- Start the proxy with `--token YOUR_TOKEN`.
- Players connect using `wss://YOUR_HOST/opensb?token=YOUR_TOKEN`.

### TLS / WSS
For production, terminate TLS with a reverse proxy (Nginx/Caddy) or run the proxy behind a HTTPS load balancer. Browsers require `wss://` on secure pages.

### Server Compression Setting (Web Clients)
Web builds do not support Zstd network stream compression. On the server, set:

- `connectionSettings.compression` → `None`

If the server advertises Zstd, web clients will refuse to connect.

## Files to Deploy
Copy all outputs from the build directory (e.g., `build/emscripten-pthreads-release/`) to the hosted folder:

- `starbound.html`
- `starbound.js`
- `starbound.wasm`
- `starbound.data`
- `preload.config`
