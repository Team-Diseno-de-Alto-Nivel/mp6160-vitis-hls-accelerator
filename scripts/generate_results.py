#!/usr/bin/env python3
"""Write live pipeline results into README.md between `<!-- RESULTS:* -->`
markers, so the numbers, logs and images shown there are never hand-copied.

Each *stage* touches only its own markers, so the fast model/HLS workflow and
the heavy gem5 workflow can update the README independently without clobbering
each other's blocks:

    python scripts/generate_results.py model --run-log sim.log   --check-log check.log
    python scripts/generate_results.py hls   --run-log hls.log   --check-log check.log
    python scripts/generate_results.py gem5  --run-log gem5.log  --check-log check.log
    python scripts/generate_results.py image                     # embeds output JPGs

The `image` stage assumes the JPGs already exist (produced by
scripts/raw_to_jpg.py); every other stage degrades gracefully to a clear
placeholder when its inputs are missing, so a partial CI run still commits
something meaningful.
"""

import argparse
import re
from pathlib import Path

WIDTH = 1920
HEIGHT = 1080
PIXELS = WIDTH * HEIGHT

INPUT_RAW = Path("images/input/image.raw")
README = Path("README.md")

# Which RAW each stage produces, and how check_output.py labels it. The label
# is what we grep for in the check log to pull that stage's verdict line.
STAGE_OUTPUT = {
    "model": ("images/output/output.raw", "standalone SystemC model"),
    "hls": ("images/output/output_hls.raw", "HLS co-simulation"),
    "gem5": ("images/output/output_gem5.raw", "gem5 virtual prototype"),
}


def replace_marker(text: str, name: str, content: str) -> str:
    """Swap the body between <!-- RESULTS:name:START --> and :END markers."""
    pattern = re.compile(
        rf"(<!-- RESULTS:{name}:START -->)(.*?)(<!-- RESULTS:{name}:END -->)",
        re.DOTALL,
    )
    if not pattern.search(text):
        raise SystemExit(f"Marker RESULTS:{name} not found in {README}")
    return pattern.sub(lambda m: f"{m.group(1)}\n{content}\n{m.group(3)}", text)


def code_block(body: str) -> str:
    return "```\n" + body.strip("\n") + "\n```"


def check_line(check_log: Path | None, output_raw: str) -> str | None:
    """The check_output.py verdict line that mentions this stage's output."""
    if not check_log or not check_log.exists():
        return None
    for line in check_log.read_text().splitlines():
        if output_raw in line:
            return line.rstrip()
    return None


# ── model ─────────────────────────────────────────────────────────────────────


def data_volume_table() -> str:
    """Bytes actually moved, read from the real files (not parsed from a log)."""
    in_bytes = INPUT_RAW.stat().st_size if INPUT_RAW.exists() else 0
    out_raw = Path(STAGE_OUTPUT["model"][0])
    out_bytes = out_raw.stat().st_size if out_raw.exists() else 0
    rows = [
        ("Disk → RAM (input)", PIXELS * 3, in_bytes),
        ("RAM → Disk (output)", PIXELS, out_bytes),
    ]
    lines = ["| Transfer | Expected | Actual | Match |", "|---|---|---|---|"]
    for label, expected, actual in rows:
        match = "✅" if expected == actual else "❌"
        lines.append(f"| {label} | {expected:,} B | {actual:,} B | {match} |")
    return "\n".join(lines)


def stage_model(args) -> None:
    text = README.read_text()
    text = replace_marker(text, "MODEL-DATA", data_volume_table())
    line = check_line(args.check_log, STAGE_OUTPUT["model"][0])
    body = line if line else "Model output not verified in this run."
    text = replace_marker(text, "MODEL-CHECK", code_block(body))
    README.write_text(text)
    print("Updated RESULTS:MODEL-DATA and RESULTS:MODEL-CHECK")


# ── hls (host functional co-simulation) ───────────────────────────────────────


def stage_hls(args) -> None:
    parts = []
    if args.run_log and args.run_log.exists():
        for line in args.run_log.read_text().splitlines():
            s = line.strip()
            if "TEST PASSED" in s or ("Processed" in s and "pixels" in s):
                parts.append(s)
    line = check_line(args.check_log, STAGE_OUTPUT["hls"][0])
    if line:
        parts.append(line)
    if not parts:
        parts = ["HLS host co-simulation not run in this workflow."]
    text = replace_marker(README.read_text(), "HLS-HOST", code_block("\n".join(parts)))
    README.write_text(text)
    print("Updated RESULTS:HLS-HOST")


# ── gem5 ──────────────────────────────────────────────────────────────────────


def stage_gem5(args) -> None:
    excerpt = ""
    if args.run_log and args.run_log.exists():
        lines = args.run_log.read_text().splitlines()
        # Keep the driver's own trace (load/configure/start/done/save) plus any
        # gem5 banner/tick lines; drop the rest to keep the README readable.
        kept = [l.rstrip() for l in lines
                if l.startswith("driver:") or "Exiting @ tick" in l
                or l.startswith("gem5") or "Global frequency" in l]
        excerpt = "\n".join(kept[:40])
    line = check_line(args.check_log, STAGE_OUTPUT["gem5"][0])
    body = excerpt or "gem5 run produced no captured output."
    if line:
        body += "\n" + line
    text = replace_marker(README.read_text(), "GEM5", code_block(body))
    README.write_text(text)
    print("Updated RESULTS:GEM5")


# ── output image ──────────────────────────────────────────────────────────────


def stage_image(args) -> None:
    """Embed the input and produced grayscale side by side (HTML table renders
    on GitHub). Uses whichever grayscale JPG exists, preferring the model's."""
    candidates = [
        ("images/output/output.jpg", "Grayscale (SystemC model)"),
        ("images/output/output_hls.jpg", "Grayscale (HLS host)"),
        ("images/output/output_gem5.jpg", "Grayscale (gem5)"),
    ]
    gray = next(((p, c) for p, c in candidates if Path(p).exists()), None)
    if gray is None:
        body = "_No grayscale JPG was produced in this run._"
    else:
        gray_path, gray_caption = gray
        body = (
            "<table>\n<tr>\n"
            '<td align="center"><img src="images/input/image.jpg" width="380"><br>Input RGB</td>\n'
            f'<td align="center"><img src="{gray_path}" width="380"><br>{gray_caption}</td>\n'
            "</tr>\n</table>"
        )
    text = replace_marker(README.read_text(), "OUTPUT-IMAGE", body)
    README.write_text(text)
    print("Updated RESULTS:OUTPUT-IMAGE")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="stage", required=True)

    p_model = sub.add_parser("model")
    p_model.add_argument("--run-log", type=Path)
    p_model.add_argument("--check-log", type=Path)
    p_model.set_defaults(func=stage_model)

    p_hls = sub.add_parser("hls")
    p_hls.add_argument("--run-log", type=Path)
    p_hls.add_argument("--check-log", type=Path)
    p_hls.set_defaults(func=stage_hls)

    p_gem5 = sub.add_parser("gem5")
    p_gem5.add_argument("--run-log", type=Path)
    p_gem5.add_argument("--check-log", type=Path)
    p_gem5.set_defaults(func=stage_gem5)

    p_image = sub.add_parser("image")
    p_image.set_defaults(func=stage_image)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
