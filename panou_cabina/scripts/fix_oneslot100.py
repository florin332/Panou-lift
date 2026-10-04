#!/usr/bin/env python3
"""
Regenerates the GFXglyph table in Fonts/oneslot100.h.

Bug: glyphs for 'A'..'Z', '?', '!', etc. were generated with a POSITIVE
yOffset (+119), while digits '0'..'9' use the correct Adafruit GFX
convention (negative yOffset = glyph top relative to the text baseline).
Result: uppercase letters render ~240px below the baseline -> off-screen
on the 240x320 panel (e.g. 'P' for Parter never appears).

Rules applied (baseline convention matching digits '0'-'9' with
yOffset = -128, i.e. digit top at baseline-128, bottom on baseline):
  - GFXglyph.yOffset is int8_t, so the minimum representable value is
    -128. All corrected glyphs are therefore TOP-ALIGNED with the digits
    at yOffset = -128 (a 138px cap extends 9px below the baseline; at
    the baselines used by PanelRenderer (140/224/300) this still fits
    the 320px screen: worst case rows 172..309).
  - glyphs already correct (digits, ',', '-', '.', ':', ';', '<', '=',
    '>', '+', '&', '~', '_', space, lowercase) are left untouched.
"""
import re
import sys
from pathlib import Path

FONT = Path(__file__).resolve().parent.parent / "Fonts" / "oneslot100.h"

CAP_TOP = -128  # int8_t minimum; aligns glyph tops with digit tops

TOP_MARKS = {0x22, 0x27, 0x2A, 0x5E, 0x60}          # " ' * ^ `
DESCENDERS = {0x28, 0x29, 0x51, 0x5B, 0x5D, 0x7B, 0x7C, 0x7D}  # ( ) Q [ ] { | }

# Glyphs that are already correct and must not be touched
KEEP = set(range(0x30, 0x3A))          # 0-9 (yOffset already -128)
KEEP |= {0x20}                          # space
KEEP |= {0x2B, 0x2C, 0x2D, 0x2E}        # + , - .
KEEP |= {0x3A, 0x3B, 0x3C, 0x3D, 0x3E}  # : ; < = >
KEEP |= {0x26}                          # &
KEEP |= {0x5F, 0x7E}                    # _ ~
KEEP |= set(range(0x61, 0x7B))          # a-z (already negative; bitmap
                                        # overlaps for h-z are a separate
                                        # issue, not touched here)

LINE_RE = re.compile(
    r"^(\s*\{)\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(-?\d+),\s*(-?\d+)\s*(\}.*)$"
)


def fix_line(line: str) -> tuple[str, bool]:
    m = LINE_RE.match(line)
    if not m:
        return line, False
    open_b, off, w, h, adv, xo, yo, tail = m.groups()
    off, w, h, adv, xo, yo = map(int, (off, w, h, adv, xo, yo))

    cm = re.search(r"0x([0-9A-Fa-f]{2})", tail)
    code = int(cm.group(1), 16) if cm else -1

    if h == 0 or code in KEEP:
        new_yo = yo
    else:
        # yOffset is int8_t: -(h-1) would underflow for h >= 130, so we
        # top-align every non-KEEP glyph with the digits at -128.
        new_yo = CAP_TOP

    if new_yo == yo:
        return line, False

    new_line = f"{open_b} {off:6d}, {w:3d}, {h:3d}, {adv:3d}, {xo:4d}, {new_yo:4d} {tail}"
    return new_line, True


def main() -> int:
    text = FONT.read_text(encoding="utf-8")
    lines = text.splitlines(keepends=True)

    changed = 0
    out = []
    in_table = False
    for line in lines:
        if "oneslot100Glyphs[]" in line:
            in_table = True
        elif in_table and "GFXfont oneslot100" in line:
            in_table = False
        if in_table:
            new_line, did = fix_line(line.rstrip("\n"))
            if did:
                changed += 1
                out.append(new_line + "\n")
                continue
        out.append(line)

    FONT.write_text("".join(out), encoding="utf-8")
    print(f"oneslot100.h: {changed} glyph yOffset(s) corrected")
    return 0


if __name__ == "__main__":
    sys.exit(main())
