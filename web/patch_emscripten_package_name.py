#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
from pathlib import Path


def patch_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    changed = False

    package_pattern = re.compile(r'(var\s+PACKAGE_NAME\s*=\s*")([^"]*?starbound\.data)(";)', re.M)
    if package_pattern.search(text):
        text = package_pattern.sub(r'\1starbound.data\3', text, count=1)
        changed = True

    datafile_pattern = re.compile(r'datafile_[^\"\)]+?starbound\.data')
    if datafile_pattern.search(text):
        text = datafile_pattern.sub('datafile_starbound.data', text)
        changed = True

    # Emscripten PThreads: When a proxied-to-main-thread JS call is marked async,
    # Emscripten awaits it via `rtn.then(...)`. Some proxied call sites can return
    # a non-Promise value (e.g. an integer error code) which makes Firefox throw:
    #   TypeError: can't access property "then", rtn is undefined
    # Treat any return value as a thenable via Promise.resolve(...).
    if "rtn.then(rtn=>__emscripten_run_js_on_main_thread_done(ctx,ctxArgs,rtn));" in text and \
       "Promise.resolve(rtn).then(rtn=>__emscripten_run_js_on_main_thread_done(ctx,ctxArgs,rtn));" not in text:
        text = text.replace(
            "rtn.then(rtn=>__emscripten_run_js_on_main_thread_done(ctx,ctxArgs,rtn));",
            "Promise.resolve(rtn).then(rtn=>__emscripten_run_js_on_main_thread_done(ctx,ctxArgs,rtn));",
            1,
        )
        changed = True

    if changed:
        path.write_text(text, encoding="utf-8")
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Patch generated Emscripten JS output (PACKAGE_NAME + web runtime fixes)."
    )
    parser.add_argument("--js", required=True, help="Path to the generated starbound.js")
    args = parser.parse_args()

    js_path = Path(args.js)
    if not js_path.exists():
        print(f"[patch_package_name] JS not found: {js_path}")
        return 0

    if patch_file(js_path):
        print(f"[patch_package_name] Patched generated JS in {js_path}")
    else:
        print(f"[patch_package_name] No PACKAGE_NAME found in {js_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())