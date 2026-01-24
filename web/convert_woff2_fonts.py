#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


def convert_one(woff2_path: Path, src_root: Path, dst_root: Path, tool: str) -> None:
    rel = woff2_path.relative_to(src_root)
    dst_woff2 = dst_root / rel
    dst_ttf = dst_woff2.with_suffix(".ttf")

    dst_woff2.parent.mkdir(parents=True, exist_ok=True)

    # Skip if we already have a newer .ttf
    try:
        if dst_ttf.exists():
            if dst_ttf.stat().st_mtime >= woff2_path.stat().st_mtime:
                return
    except OSError:
        pass

    shutil.copy2(woff2_path, dst_woff2)

    # woff2_decompress writes a .ttf next to the input and returns non-zero on failure.
    proc = subprocess.run([tool, str(dst_woff2)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if proc.returncode != 0:
        raise RuntimeError(
            f"woff2_decompress failed for {woff2_path} -> {dst_woff2}\n"
            f"stdout:\n{proc.stdout}\n\n"
            f"stderr:\n{proc.stderr}\n"
        )

    if not dst_ttf.exists() or dst_ttf.stat().st_size == 0:
        raise RuntimeError(f"woff2_decompress did not produce a valid TTF: {dst_ttf}")

    # Remove the copied .woff2 to keep the preload small and avoid accidental WOFF2 selection.
    try:
        dst_woff2.unlink(missing_ok=True)
    except TypeError:
        # Python < 3.8 compatibility
        if dst_woff2.exists():
            dst_woff2.unlink()


def main() -> int:
    parser = argparse.ArgumentParser(description="Convert OpenSB .woff2 fonts to .ttf for web builds")
    parser.add_argument("--src", required=True, help="Source font directory (expects .woff2 inside)")
    parser.add_argument("--dst", required=True, help="Destination root (will create font/.../*.ttf)")
    parser.add_argument("--tool", default="woff2_decompress", help="Path to woff2_decompress")
    args = parser.parse_args()

    src_root = Path(args.src).resolve()
    dst_root = Path(args.dst).resolve()

    if not src_root.exists():
        raise SystemExit(f"Source directory does not exist: {src_root}")

    tool = shutil.which(args.tool) or args.tool
    if not shutil.which(tool) and not Path(tool).exists():
        raise SystemExit(f"woff2_decompress not found: {args.tool}")

    woff2_files = sorted(src_root.rglob("*.woff2"))
    if not woff2_files:
        # Nothing to do.
        return 0

    for woff2 in woff2_files:
        convert_one(woff2, src_root, dst_root, tool)

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as e:
        print(f"[convert_woff2_fonts] ERROR: {e}", file=sys.stderr)
        returncode = 1
        raise SystemExit(returncode)
