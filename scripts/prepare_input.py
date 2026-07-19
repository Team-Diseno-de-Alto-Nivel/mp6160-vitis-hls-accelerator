#!/usr/bin/env python3
"""Prepare images/input/image.raw for the simulation.

Looks for an existing image (JPG, PNG, …) in images/input/.
If one is found, converts it to RAW. Otherwise generates a synthetic
1920×1080 RGB gradient and saves it as test.jpg + image.raw.

Usage:
    python scripts/prepare_input.py [--width W] [--height H]
"""

import argparse
import sys
from pathlib import Path

from PIL import Image

SUPPORTED = {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".webp"}
INPUT_DIR = Path("images/input")
OUTPUT_RAW = INPUT_DIR / "image.raw"


def find_existing_image() -> Path | None:
    for p in sorted(INPUT_DIR.iterdir()):
        if p.suffix.lower() in SUPPORTED:
            return p
    return None


def generate_test_image(dst: Path, width: int, height: int) -> None:
    try:
        import numpy as np
        arr = __import__("numpy").zeros((height, width, 3), dtype="uint8")
        arr[:, :, 0] = np.linspace(0, 255, width, dtype="uint8")
        arr[:, :, 1] = np.linspace(0, 255, height, dtype="uint8")[:, None]
        arr[:, :, 2] = 128
        img = Image.fromarray(arr)
    except ImportError:
        img = Image.new("RGB", (width, height), (255, 128, 64))

    dst.parent.mkdir(parents=True, exist_ok=True)
    img.save(dst)
    print(f"Generated synthetic test image → {dst}")


def to_raw(src: Path, dst: Path, width: int, height: int) -> None:
    img = Image.open(src).convert("RGB")
    if img.size != (width, height):
        img = img.resize((width, height), Image.LANCZOS)
    dst.parent.mkdir(parents=True, exist_ok=True)
    dst.write_bytes(img.tobytes())
    print(f"Converted {src} → {dst}  ({dst.stat().st_size} bytes)")


def main() -> None:
    parser = argparse.ArgumentParser(description="Prepare RAW input for simulation")
    parser.add_argument("--width",  type=int, default=1920)
    parser.add_argument("--height", type=int, default=1080)
    args = parser.parse_args()

    INPUT_DIR.mkdir(parents=True, exist_ok=True)

    src = find_existing_image()
    if src:
        print(f"Found existing image: {src}")
    else:
        print("No image found in images/input/ — generating synthetic test image")
        src = INPUT_DIR / "test.jpg"
        generate_test_image(src, args.width, args.height)

    to_raw(src, OUTPUT_RAW, args.width, args.height)


if __name__ == "__main__":
    main()
