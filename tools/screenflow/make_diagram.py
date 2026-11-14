#!/usr/bin/env python3
"""
Builds docs/screens.svg (and docs/screens.png when Inkscape is available):
all QubeRG screens with arrows showing which button leads where.

The screen images are produced by build.sh, which runs the real firmware
(src/main.cpp) on the PC with simulated buttons.

(c) 2026 PA3HCM - CC BY-NC 4.0
"""

import base64
import os
import shutil
import struct
import subprocess
import sys
import zlib

SCALE = 2
FG = (150, 235, 255)      # OLED pixel colour
BG = (8, 10, 16)

C_MODE = "#d35400"
C_DIGIT = "#1e8449"
C_UPDN = "#1f5fbf"
C_TEXT = "#222"


def read_pgm(path):
    with open(path, "rb") as f:
        data = f.read()
    parts = data.split(b"\n", 3)
    w, h = map(int, parts[1].split())
    return w, h, parts[3]


def png_data_uri(path):
    w, h, px = read_pgm(path)
    rows = []
    for y in range(h):
        row = bytearray([0])
        for x in range(w):
            c = FG if px[y * w + x] else BG
            row += bytes(c) * SCALE
        rows.append(bytes(row) * SCALE)
    raw = b"".join(rows)

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w * SCALE, h * SCALE, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    return "data:image/png;base64," + base64.b64encode(png).decode()


CW, CH = 272, 144            # card size (screen 256 x 128 + bezel)
COL = [40, 370, 700, 1030, 1360]
R0, R1, R2, R3 = 80, 340, 640, 920
W, H = 1720, 1160


def card(svg, imgdir, name, x, y, title, sub=None, dashed=False):
    dash = ' stroke-dasharray="8 5"' if dashed else ""
    svg.append(f'<text x="{x + CW / 2}" y="{y - 12}" text-anchor="middle" class="t">{title}</text>')
    if sub:
        svg.append(f'<text x="{x + CW / 2}" y="{y + CH + 20}" text-anchor="middle" class="s">{sub}</text>')
    svg.append(f'<rect x="{x}" y="{y}" width="{CW}" height="{CH}" rx="10" fill="#1d2330" stroke="#555" stroke-width="2"{dash}/>')
    svg.append(f'<image x="{x + 8}" y="{y + 8}" width="256" height="128" '
               f'href="{png_data_uri(os.path.join(imgdir, name + ".pgm"))}" style="image-rendering:pixelated"/>')


def arrow(svg, pts, color, label=None, lx=None, ly=None, anchor="middle"):
    d = "M" + " L".join(f"{x},{y}" for x, y in pts)
    svg.append(f'<path d="{d}" fill="none" stroke="{color}" stroke-width="3" marker-end="url(#a{color[1:]})"/>')
    if label:
        for i, line in enumerate(label.split("\n")):
            svg.append(f'<text x="{lx}" y="{ly + i * 17}" text-anchor="{anchor}" class="b" fill="{color}">{line}</text>')


def hmove(svg, i, j, y, color, label):
    """Horizontal arrow from card in column i to card in column j at row y."""
    x0, x1 = COL[i] + CW + 4, COL[j] - 6
    arrow(svg, [(x0, y + CH / 2), (x1, y + CH / 2)], color, label, (x0 + x1) / 2, y + CH / 2 - 10)


def build(imgdir, out_svg):
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}" '
           'font-family="DejaVu Sans, Arial, sans-serif">',
           '<style>.t{font-size:17px;font-weight:bold;fill:#222}.s{font-size:13px;fill:#555}'
           '.b{font-size:15px;font-weight:bold}.h{font-size:24px;font-weight:bold;fill:#222}'
           '.l{font-size:15px;fill:#222}</style>', '<defs>']
    for c in (C_MODE, C_DIGIT, C_UPDN, "#888"):
        svg.append(f'<marker id="a{c[1:]}" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="5" markerHeight="5" '
                   f'orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" fill="{c}"/></marker>')
    svg.append('</defs>')
    svg.append(f'<rect width="{W}" height="{H}" fill="#fff"/>')
    svg.append(f'<text x="{W / 2}" y="40" text-anchor="middle" class="h">QubeRG – screens and buttons</text>')

    # ---- start-up
    card(svg, imgdir, "splash", COL[0], R0, "Start-up")
    arrow(svg, [(COL[0] + 70, R0 + CH + 4), (COL[0] + 70, R1 - 34)], "#888", "after 2 s", COL[0] + 82, R0 + CH + 52, "start")

    # ---- main loop
    svg.append(f'<text x="{COL[1]}" y="{R1 - 75}" class="l" font-weight="bold">Main loop – press MODE for the next mode</text>')
    card(svg, imgdir, "off", COL[0], R1, "OFF (power-up)", "no output")
    card(svg, imgdir, "cw", COL[1], R1, "CW", "continuous carrier")
    card(svg, imgdir, "cwkeyed", COL[2], R1, "CW-keyed", "-.-.-. at 12 WPM")
    card(svg, imgdir, "fsk", COL[3], R1, "FSK", "f and f + shift, 500 ms each")
    card(svg, imgdir, "beacon", COL[4], R1, "Beacon", "only when CALLSIGN is set", dashed=True)
    for i in range(4):
        hmove(svg, i, i + 1, R1, C_MODE, "MODE")
    # Beacon -> OFF, over the top
    yb = R1 - 48
    arrow(svg, [(COL[4] + CW / 2, R1 - 30), (COL[4] + CW / 2, yb), (COL[0] + 200, yb), (COL[0] + 200, R1 - 34)],
          C_MODE, "MODE (back to OFF; without CALLSIGN: FSK → OFF)", (COL[2] + COL[3]) / 2 + 100, yb - 8)

    # ---- shift setting
    yh = R1 + CH + 56
    arrow(svg, [(COL[3] + CW / 2, R1 + CH + 28), (COL[3] + CW / 2, yh), (COL[1] + CW / 2, yh), (COL[1] + CW / 2, R2 - 34)],
          C_MODE, "hold MODE 1 s: FSK shift setting (output keeps running)", (COL[1] + COL[3]) / 2 + CW / 2, yh - 8)
    card(svg, imgdir, "shift170", COL[1], R2, "Shift 170 Hz")
    card(svg, imgdir, "shift425", COL[2], R2, "Shift 425 Hz")
    card(svg, imgdir, "shift850", COL[3], R2, "Shift 850 Hz")
    card(svg, imgdir, "shiftvar", COL[4], R2, "Shift VAR (1 – 10000 Hz)")
    for i in range(1, 4):
        hmove(svg, i, i + 1, R2, C_MODE, "MODE")
    yv = R2 + CH + 40
    arrow(svg, [(COL[4] + 100, R2 + CH + 4), (COL[4] + 100, yv), (COL[1] + CW / 2, yv), (COL[1] + CW / 2, R2 + CH + 6)],
          C_MODE, "MODE", (COL[2] + COL[3]) / 2 + CW / 2, yv - 8)
    # UP/DOWN on VAR
    svg.append(f'<text x="{COL[4] + CW + 6}" y="{R2 + 50}" class="b" fill="{C_UPDN}">▲ ▼</text>')
    svg.append(f'<text x="{COL[4] + CW + 6}" y="{R2 + 70}" class="s">±1 Hz</text>')
    svg.append(f'<text x="{COL[4] + CW + 6}" y="{R2 + 88}" class="s">hold:</text>')
    svg.append(f'<text x="{COL[4] + CW + 6}" y="{R2 + 106}" class="s">10 Hz</text>')
    svg.append(f'<text x="{COL[4] + CW + 6}" y="{R2 + 124}" class="s">steps</text>')

    # back to normal operation
    card(svg, imgdir, "fskvar", COL[0], R2, "Back in the main loop", "FSK with the chosen shift")
    x0, x1 = COL[1] - 6, COL[0] + CW + 6
    arrow(svg, [(x0, R2 + CH / 2), (x1, R2 + CH / 2)], C_MODE, "hold\nMODE", (x0 + x1) / 2, R2 + CH / 2 - 26)
    svg.append(f'<text x="{(x0 + x1) / 2}" y="{R2 + CH / 2 + 22}" text-anchor="middle" class="s">any</text>')
    svg.append(f'<text x="{(x0 + x1) / 2}" y="{R2 + CH / 2 + 38}" text-anchor="middle" class="s">shift</text>')

    # save
    card(svg, imgdir, "saved", COL[4], R3, "Shift saved", "kept after power off")
    arrow(svg, [(COL[4] + 220, R2 + CH + 4), (COL[4] + 220, R3 - 34)], C_DIGIT, "hold DIGIT 1 s", COL[4] + 214, R3 - 52, "end")

    # ---- normal operation keys
    card(svg, imgdir, "digitup", COL[0], R3, "Setting the frequency", "cursor on the 10 kHz digit, ▲ pressed")
    lx, ly = COL[1] + 10, R3 + 4
    lines = [
        ("In the main loop (every mode):", None),
        ("▲ UP / ▼ DOWN", C_UPDN, "change the digit above the cursor (19 → 20), hold to repeat"),
        ("D DIGIT", C_DIGIT, "move the cursor to the next digit (wraps around)"),
        ("hold D DIGIT", C_DIGIT, "save the FSK shift (also works in the shift setting)"),
        ("M MODE", C_MODE, "next mode"),
        ("hold M MODE", C_MODE, "in FSK: open / close the FSK shift setting"),
        ("", None),
        ("In the shift setting:", None),
        ("M MODE", C_MODE, "next shift: 170 → 425 → 850 → VAR"),
        ("▲ UP / ▼ DOWN", C_UPDN, "VAR only: ±1 Hz, hold for 10 Hz steps (about 100 Hz/s)"),
    ]
    for i, item in enumerate(lines):
        y = ly + i * 22
        if item[1] is None:
            svg.append(f'<text x="{lx}" y="{y}" class="l" font-weight="bold">{item[0]}</text>')
        else:
            svg.append(f'<text x="{lx + 10}" y="{y}" class="b" fill="{item[1]}">{item[0]}</text>')
            svg.append(f'<text x="{lx + 150}" y="{y}" class="l">{item[2]}</text>')
    svg.append("</svg>")

    with open(out_svg, "w") as f:
        f.write("\n".join(svg))
    print("written", out_svg)
    if shutil.which("inkscape"):
        out_png = os.path.splitext(out_svg)[0] + ".png"
        subprocess.run(["inkscape", out_svg, "--export-type=png", f"--export-filename={out_png}"],
                       check=True, capture_output=True)
        print("written", out_png)


if __name__ == "__main__":
    build(sys.argv[1], sys.argv[2])
