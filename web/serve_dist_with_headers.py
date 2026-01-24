#!/usr/bin/env python3

from __future__ import annotations

import argparse
import http.server
import os
import socketserver


class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Required for SharedArrayBuffer / Emscripten pthreads.
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")

        # Helpful defaults for local dev.
        self.send_header("Cross-Origin-Resource-Policy", "same-origin")
        self.send_header("Cache-Control", "no-store")

        super().end_headers()


def main() -> None:
    parser = argparse.ArgumentParser(description="Serve OpenStarbound web dist/ with COOP/COEP headers.")
    parser.add_argument("--dir", default=None)
    parser.add_argument("--pthreads", action="store_true", help="Serve the pthread-enabled web build folder.")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()

    if args.dir:
        directory = os.path.abspath(args.dir)
    else:
        base_dir = "dist-web-pthreads" if args.pthreads else "dist-web"
        directory = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", base_dir))
    os.chdir(directory)

    with socketserver.TCPServer((args.host, args.port), Handler) as httpd:
        print(f"Serving {directory} at http://{args.host}:{args.port}/")
        httpd.serve_forever()


if __name__ == "__main__":
    main()
