#!/usr/bin/env python3
"""Check generated grayscale images against the BT.601 reference.

Recomputes the reference on the host from images/input/image.raw using the same
arithmetic as src/model/utils/conversion.h (float32
weights, round-half-away-from-zero) and compares every output found in
images/output/ byte for byte.

Usage:
    python scripts/check_output.py [--input images/input/image.raw]
"""

import argparse
import sys
from pathlib import Path

import numpy as np

# Which outputs to check, and which stage produced them.
OUTPUTS = {
    "output.raw": "standalone SystemC model",
    "output_gem5.raw": "gem5 virtual prototype",
    "output_hls.raw": "HLS co-simulation",
}


def reference(rgb_path: Path) -> np.ndarray:
    rgb = np.fromfile(rgb_path, dtype=np.uint8)
    if rgb.size % 3:
        sys.exit(f"{rgb_path}: {rgb.size} bytes is not a whole number of RGB pixels")
    rgb = rgb.reshape(-1, 3).astype(np.float32)
    gray = (
        np.float32(0.299) * rgb[:, 0]
        + np.float32(0.587) * rgb[:, 1]
        + np.float32(0.114) * rgb[:, 2]
    )
    return np.floor(gray.astype(np.float64) + 0.5).astype(np.uint8)


def compare(path: Path, ref: np.ndarray, label: str) -> bool:
    got = np.fromfile(path, dtype=np.uint8)
    if got.size != ref.size:
        print(f"FAIL  {path}  ({label}): expected {ref.size} bytes, got {got.size}")
        return False

    bad = np.flatnonzero(got != ref)
    if bad.size:
        first = bad[0]
        print(
            f"FAIL  {path}  ({label}): {bad.size} of {ref.size} pixels differ; "
            f"first at {first} (expected {ref[first]}, got {got[first]})"
        )
        return False

    print(f"OK    {path}  ({label}): {ref.size} pixels match BT.601")
    return True


def main() -> None:
    parser = argparse.ArgumentParser(description="Verify grayscale output against BT.601")
    parser.add_argument("--input", type=Path, default=Path("images/input/image.raw"))
    parser.add_argument("--output-dir", type=Path, default=Path("images/output"))
    args = parser.parse_args()

    if not args.input.exists():
        sys.exit(f"{args.input} not found — run scripts/prepare_input.py first")

    ref = reference(args.input)

    checked = [
        (args.output_dir / name, label)
        for name, label in OUTPUTS.items()
        if (args.output_dir / name).exists()
    ]
    if not checked:
        sys.exit(
            f"No outputs found in {args.output_dir}. "
            "Run 'make run-model' and/or 'make run-vp' first."
        )

    if not all(compare(path, ref, label) for path, label in checked):
        sys.exit(1)


if __name__ == "__main__":
    main()
