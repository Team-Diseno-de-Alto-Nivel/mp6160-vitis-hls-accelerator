#!/usr/bin/env python3
"""Convert a headerless RAW file back to JPG (or any PIL-supported format).

Usage:
    python scripts/raw_to_jpg.py <input.raw> <output.jpg> [--width W] [--height H] [--gray]

Defaults: 1920x1080 RGB. Pass --gray for 1-byte/pixel grayscale RAW (accelerator output).
"""

import argparse
import sys
from pathlib import Path
from PIL import Image


def convert(src: Path, dst: Path, width: int, height: int, gray: bool) -> None:
    channels = 1 if gray else 3
    expected = width * height * channels
    raw = src.read_bytes()

    if len(raw) != expected:
        sys.exit(
            f"Error: expected {expected} bytes for {width}x{height} "
            f"{'gray' if gray else 'RGB'}, got {len(raw)}"
        )

    mode = "L" if gray else "RGB"
    img = Image.frombytes(mode, (width, height), raw)
    dst.parent.mkdir(parents=True, exist_ok=True)
    img.save(dst)
    print(f"Saved {width}x{height} {mode} → {dst}")


def main() -> None:
    parser = argparse.ArgumentParser(description="RAW → JPG converter")
    parser.add_argument("input",  type=Path, help="Source .raw file")
    parser.add_argument("output", type=Path, help="Destination image (JPG, PNG, …)")
    parser.add_argument("--width",  type=int, default=1920)
    parser.add_argument("--height", type=int, default=1080)
    parser.add_argument("--gray", action="store_true",
                        help="1 byte/pixel grayscale (accelerator output)")
    args = parser.parse_args()

    if not args.input.exists():
        sys.exit(f"Error: {args.input} not found")

    convert(args.input, args.output, args.width, args.height, args.gray)


if __name__ == "__main__":
    main()
