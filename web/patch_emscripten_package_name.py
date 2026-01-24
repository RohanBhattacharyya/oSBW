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

    if changed:
        path.write_text(text, encoding="utf-8")
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description="Patch Emscripten PACKAGE_NAME to a relative path.")
    parser.add_argument("--js", required=True, help="Path to the generated starbound.js")
    args = parser.parse_args()

    js_path = Path(args.js)
    if not js_path.exists():
        print(f"[patch_package_name] JS not found: {js_path}")
        return 0

    if patch_file(js_path):
        print(f"[patch_package_name] Patched PACKAGE_NAME in {js_path}")
    else:
        print(f"[patch_package_name] No PACKAGE_NAME found in {js_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())