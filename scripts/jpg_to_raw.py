#!/usr/bin/env python3
"""Convert a JPG (or any PIL-supported format) to headerless RAW RGB.

Usage:
    python scripts/jpg_to_raw.py <input.jpg> <output.raw> [--width W] [--height H]

Defaults: 1920x1080 (project spec). The image is resized if dimensions differ.
Output: W*H*3 bytes, row-major RGB, no header.
"""

import argparse
import sys
from pathlib import Path
from PIL import Image


def convert(src: Path, dst: Path, width: int, height: int) -> None:
    img = Image.open(src).convert("RGB")
    if img.size != (width, height):
        img = img.resize((width, height), Image.LANCZOS)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(img.tobytes())
    print(f"Saved {width}x{height} RAW RGB → {dst}  ({dst.stat().st_size} bytes)")


def main() -> None:
    parser = argparse.ArgumentParser(description="JPG → RAW RGB converter")
    parser.add_argument("input",  type=Path, help="Source image (JPG, PNG, …)")
    parser.add_argument("output", type=Path, help="Destination .raw file")
    parser.add_argument("--width",  type=int, default=1920)
    parser.add_argument("--height", type=int, default=1080)
    args = parser.parse_args()

    if not args.input.exists():
        sys.exit(f"Error: {args.input} not found")

    convert(args.input, args.output, args.width, args.height)


if __name__ == "__main__":
    main()
