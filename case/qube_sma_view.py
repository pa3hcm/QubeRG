"""
Exploded view: how the Si5351 board hangs from the top part on the SMA
bulkhead (F-F) and the M-M adaptor.
(c) 2026 PA3HCM - CC BY-NC 4.0

Called from qube_case.py --preview. Writes docs/sma_mount.png.
Connector sizes are typical values, for illustration only.
"""

import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import proj3d

import qube_case as qc
from qube_case import Manifold, box, cyl_z
from qube_preview import draw, tris, CASE_COLOR2

GOLD = (0.88, 0.70, 0.28)
NICKEL = (0.78, 0.78, 0.80)
WHITE = (0.95, 0.95, 0.92)
PCB = (0.45, 0.15, 0.55)
BLACK = (0.08, 0.08, 0.08)

X, Y = qc.SMA_X, qc.SMA_Y
R_THREAD = 3.17            # 1/4"-36 thread
R_HEX = 4.0 / 0.866        # 8 mm across flats


def hex_z(z0, h, r=R_HEX):
    return Manifold.cylinder(h, r, r, 6).translate((X, Y, z0))


def female_end(z0, h, up):
    """Threaded female end with dielectric and centre contact, opening up or down."""
    body = cyl_z(X, Y, z0, h, R_THREAD)
    zc = z0 + h - 4.5 if up else z0
    body = body - cyl_z(X, Y, zc - 0.01 if not up else zc, 4.5 + 0.01, 2.25)
    diel = cyl_z(X, Y, zc + (0 if up else 0.6), 3.9, 2.25) - cyl_z(X, Y, zc - 1, 7, 0.55)
    pin = cyl_z(X, Y, zc + (0 if up else 0.5), 4.0, 0.45)
    return body, diel, pin


def parts():
    """All parts in assembled position: list of (name, manifold, colour)."""
    top_in = qc.IN1             # inner surface of the top
    top_out = qc.S
    p = []
    # --- SMA F-F bulkhead: flange on the outside, nut + washer on the inside
    p.append(("bulkhead", hex_z(top_out, 2.0), GOLD))
    b, d, pin = female_end(top_out + 2.0, 6.5, up=True)
    p += [("bulkhead", b, GOLD), ("bulkhead", d, WHITE), ("bulkhead", pin, GOLD)]
    p.append(("bulkhead", cyl_z(X, Y, 79.0, top_out - 79.0 + 0.01, R_THREAD), GOLD))
    b, d, pin = female_end(79.0 - 0.01, 3.0, up=False)
    p += [("bulkhead", d, WHITE)]
    p.append(("washer", cyl_z(X, Y, top_in - 0.6, 0.6, 4.6) - cyl_z(X, Y, top_in - 1, 2, 3.3), NICKEL))
    p.append(("nut", hex_z(top_in - 3.1, 2.5) - cyl_z(X, Y, top_in - 4, 4, R_THREAD), GOLD))
    # --- M-M adaptor: two coupling nuts and a body
    a_top = 83.9
    p.append(("adaptor", hex_z(a_top - 5.0, 5.0) - cyl_z(X, Y, a_top - 4.6, 5, R_THREAD), GOLD))
    p.append(("adaptor", cyl_z(X, Y, a_top - 7.6, 2.61, 3.4), GOLD))
    p.append(("adaptor", hex_z(a_top - 12.6, 5.0) - cyl_z(X, Y, a_top - 13, 4.6, R_THREAD), GOLD))
    # --- Si5351 board with three edge-launch SMA jacks
    st = qc.SI_TOP
    for k in range(3):    # CLK0 (on the adaptor), CLK1, CLK2
        xx = qc.SI_XC + qc.SI_CLK0_OFF + k * qc.SI_SMA_PITCH
        jack = box(xx - 3.25, Y - 3.25, st - 0.2, xx + 3.25, Y + 3.25, st + 4.5)
        jack += cyl_z(xx, Y, st + 4.5, 6.1 if k == 0 else 6.5, R_THREAD)
        jack += box(xx - 3.25, Y - 3.25, st - 3.5, xx - 2.0, Y - 0.85, st)     # legs around the PCB
        jack += box(xx + 2.0, Y - 3.25, st - 3.5, xx + 3.25, Y - 0.85, st)
        jack += box(xx - 3.25, Y + 0.85, st - 3.5, xx - 2.0, Y + 3.25, st)
        jack += box(xx + 2.0, Y + 0.85, st - 3.5, xx + 3.25, Y + 3.25, st)
        p.append(("board", jack, GOLD))
    xc = qc.SI_XC
    p.append(("board", box(xc - qc.SI_W / 2, Y - qc.SI_T / 2, qc.SI_BOT, xc + qc.SI_W / 2, Y + qc.SI_T / 2, st), PCB))
    p.append(("board", box(xc - 2.5, Y - qc.SI_T / 2 - 1.0, qc.SI_BOT + 9, xc + 2.5, Y - qc.SI_T / 2, qc.SI_BOT + 14), BLACK))
    p.append(("board", box(xc + 6, Y - qc.SI_T / 2 - 1.2, qc.SI_BOT + 8, xc + 9.2, Y - qc.SI_T / 2, qc.SI_BOT + 13), NICKEL))
    for i in range(4):  # header pins on the bottom edge
        px = xc - 3.81 + i * 2.54
        p.append(("board", box(px - 0.32, Y - 0.32, qc.SI_BOT - 6, px + 0.32, Y + 0.32, qc.SI_BOT), GOLD))
    p.append(("board", box(xc - 5.1, Y - 1.27, qc.SI_BOT - 2.5, xc + 5.1, Y + 1.27, qc.SI_BOT), BLACK))
    return p


EXPLODE = {"lid": 0, "bulkhead": 22, "washer": -9, "nut": -14, "adaptor": -24, "board": -38}
LABELS = {
    "bulkhead": "1. SMA F-F bulkhead:\n    through the hole from the outside",
    "washer": "2. washer",
    "nut": "3. nut: tighten from the inside",
    "adaptor": "4. M-M adaptor:\n    screw onto the bulkhead",
    "board": "5. Si5351 board:\n    screw its CLK0 jack\n    onto the adaptor",
}


def fig_points(fig, ax, m):
    """Project a manifold's vertices to figure coordinates (0..1)."""
    import numpy as np
    v = np.asarray(m.to_mesh().vert_properties)[:, :3]
    x2, y2, _ = proj3d.proj_transform(v[:, 0], v[:, 1], v[:, 2], ax.get_proj())
    disp = ax.transData.transform(np.column_stack([x2, y2]))
    return fig.transFigure.inverted().transform(disp)


def anchor(pts, side):
    """Point on the left/right silhouette of a part, at its mid height."""
    import numpy as np
    xe = pts[:, 0].max() if side == "right" else pts[:, 0].min()
    near = pts[np.abs(pts[:, 0] - xe) < 0.004]
    return xe, float(np.median(near[:, 1]))


def place_labels(fig, entries, x_text, side, fontsize=12):
    """Labels in a column at x_text; pushed apart vertically so they never overlap."""
    line_h = fontsize * 1.3 / (fig.get_figheight() * 72)
    gap = 0.012
    entries = sorted(entries, key=lambda e: -e[0][1])
    prev_bottom = None
    for (ax_, ay), text in entries:
        n = text.count("\n") + 1
        h = n * line_h
        ty = ay
        if prev_bottom is not None and ty + h / 2 > prev_bottom - gap:
            ty = prev_bottom - gap - h / 2
        prev_bottom = ty - h / 2
        pad = 0.006 if side == "right" else -0.006
        fig.add_artist(plt.Line2D([ax_ + (0.004 if side == "right" else -0.004), x_text - pad], [ay, ty],
                                  transform=fig.transFigure, color="#777", lw=0.9))
        fig.text(x_text, ty, text, fontsize=fontsize, color="#222", va="center",
                 ha="left" if side == "right" else "right")


def fit(ax, lo, hi, zoom=1.0):
    """Tight, non-cubic 3D limits so the parts fill the axes."""
    ax.set_xlim(lo[0], hi[0])
    ax.set_ylim(lo[1], hi[1])
    ax.set_zlim(lo[2], hi[2])
    ax.set_box_aspect((hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]), zoom=zoom)


def render(top, outdir):
    import numpy as np
    lid = top ^ box(-1, Y, -1, qc.S + 1, qc.S + 1, qc.S + 1)    # back half of the top part (cut view)
    lid = lid ^ box(X - 16, -1, -1, X + 16, Y + 16, qc.S + 1)   # strip around the SMA hole
    ps = parts()
    fig = plt.figure(figsize=(9, 10), dpi=120)

    # ---- exploded view (geometry on the left of its half, labels to the right)
    ax = fig.add_axes([0.05, 0.0, 0.50, 1.0], projection="3d")
    items = [tris(lid, CASE_COLOR2, 4)]
    groups = {"lid": [lid]}
    for name, m, col in ps:
        mm = m.translate((0, 0, EXPLODE[name]))
        items.append(tris(mm, col, 1.5))
        groups.setdefault(name, []).append(mm)
    lim = ((X - 30, Y - 30, 15), (X + 30, Y + 30, 125))
    draw(ax, items, elev=12, azim=-72, light=(-0.3, -0.8, 0.6), lim=lim)
    fit(ax, (X - 18, Y - 8, qc.SI_BOT + EXPLODE["board"] - 9), (qc.SI_XC + qc.SI_W / 2 + 2, Y + 16, qc.S + 33), zoom=1.05)
    ax.plot([X, X], [Y, Y], [qc.SI_TOP - 40, qc.S + 32], color="#888", ls="--", lw=1)
    fig.canvas.draw()
    pts = {n: np.concatenate([fig_points(fig, ax, m) for m in ms]) for n, ms in groups.items()}
    right = max(p[:, 0].max() for p in pts.values())
    texts = dict(LABELS)
    texts["lid"] = "top part (cut open)"
    place_labels(fig, [(anchor(pts[n], "right"), texts[n]) for n in texts], right + 0.03, "right")
    clk0 = [m for n, m, c in ps if n == "board"][0].translate((0, 0, EXPLODE["board"]))
    c0 = fig_points(fig, ax, clk0)
    left = min(p[:, 0].min() for p in pts.values())
    place_labels(fig, [(anchor(c0, "left"), "CLK0")], left - 0.03, "left")
    top_y = max(p[:, 1].max() for p in pts.values())
    fig.text((left + right) / 2 + 0.12, top_y + 0.05, "Mounting the Si5351 on the top part",
             fontsize=15, ha="center", weight="bold")

    out = os.path.join(outdir, "sma_mount.png")
    fig.savefig(out, facecolor="white", bbox_inches="tight", pad_inches=0.3)
    plt.close(fig)
    print("written", os.path.abspath(out))
