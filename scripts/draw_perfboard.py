#!/usr/bin/env python3
"""Draw the lamp's perfboard (docs/assembly.md section 3) as two to-scale SVG images.

    scripts/draw_perfboard.py            # writes docs/img/perfboard-parts-side.svg
                                         #    and docs/img/perfboard-copper-side.svg

Python standard library only. The layout below is the single source for the drawings and must match the tables in
docs/assembly.md section 3 (parts side, copper links, wires). Change both together.
"""

import os
import sys

PITCH = 2.54                      # mm
COLS = "ABCDEFGHIJKL"             # west -> east, seen from the parts side
ROWS = 4                          # 1 = north (button side) ... 4 = south (XIAO side)
BOARD_W, BOARD_H = 30.5, 10.2     # mm
EDGE_X = (BOARD_W - (len(COLS) - 1) * PITCH) / 2
EDGE_Y = (BOARD_H - (ROWS - 1) * PITCH) / 2
S = 26.0                          # px per mm
FONT = "DejaVu Sans, Arial, Helvetica, sans-serif"

# --- the layout (docs/assembly.md 3.1-3.3) ---------------------------------------------------------------------
RESISTORS = [  # (lead hole, lead hole, label, colour bands)
    ("A2", "E2", "100 Ω  (red)", ["#7b4a1e", "#111111", "#7b4a1e", "#c9a227"]),
    ("G1", "K1", "220 Ω  (green)", ["#c62828", "#c62828", "#7b4a1e", "#c9a227"]),
    ("H3", "L3", "100 Ω  (blue)", ["#7b4a1e", "#111111", "#7b4a1e", "#c9a227"]),
]
LED_LEGS = {"F2": "R", "G2": "G", "F3": "C", "G3": "B"}
WIRES = [  # (hole, wire colour, short name, destination)
    ("A4", "#d32f2f", "0", "XIAO D0"),
    ("E4", "#f5f5f5", "S", "button pin 1"),
    ("F4", "#212121", "N", "XIAO GND"),
    ("J4", "#2e7d32", "2", "XIAO D2"),
    ("L4", "#1565c0", "3", "XIAO D3"),
]
LINKS = [  # copper-side links between neighbouring holes
    ("F2", "E2"), ("A2", "A3"), ("A3", "A4"),                               # red, D0
    ("G2", "G1"), ("K1", "J1"), ("J1", "J2"), ("J2", "J3"), ("J3", "J4"),  # green, D2
    ("G3", "H3"), ("L3", "L4"),                                             # blue, D3
    ("F3", "F4"), ("E4", "F4"),                                             # ground
]

STAGGER = {"E4": 40}            # px: wire S label moved down, clear of its neighbour N

BOARD_FILL = "#e6d7a8"
PAD_COPPER = "#c8743a"
SOLDER = "#b9bec4"
SOLDER_EDGE = "#7d848c"


def hole_mm(name, mirrored):
    col = COLS.index(name[0])
    row = int(name[1:]) - 1
    x = EDGE_X + col * PITCH
    if mirrored:
        x = BOARD_W - x
    return x, EDGE_Y + row * PITCH


class Svg:
    def __init__(self, width, height, ox, oy, mirrored):
        self.w, self.h, self.ox, self.oy, self.mirrored = width, height, ox, oy, mirrored
        self.items = []

    def p(self, name):
        x, y = hole_mm(name, self.mirrored)
        return self.ox + x * S, self.oy + y * S

    def add(self, s):
        self.items.append(s)

    def text(self, x, y, s, size=13, anchor="middle", weight="normal", fill="#222", extra=""):
        self.add(f'<text x="{x:.1f}" y="{y:.1f}" font-family="{FONT}" font-size="{size}" '
                 f'text-anchor="{anchor}" font-weight="{weight}" fill="{fill}" {extra}>{s}</text>')

    def line(self, x1, y1, x2, y2, stroke, width, extra=""):
        self.add(f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" stroke="{stroke}" '
                 f'stroke-width="{width:.1f}" stroke-linecap="round" {extra}/>')

    def circle(self, x, y, r, fill, stroke="none", width=0, extra=""):
        self.add(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r:.1f}" fill="{fill}" stroke="{stroke}" '
                 f'stroke-width="{width:.1f}" {extra}/>')

    def save(self, path):
        with open(path, "w", encoding="utf-8") as f:
            f.write(f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" '
                    f'viewBox="0 0 {self.w} {self.h}">\n')
            f.write(f'<rect width="{self.w}" height="{self.h}" fill="#ffffff"/>\n')
            f.write("\n".join(self.items))
            f.write("\n</svg>\n")


def frame(svg, title, subtitle):
    """Board outline, grid labels, compass, scale bar."""
    ox, oy = svg.ox, svg.oy
    svg.text(svg.w / 2, 34, title, size=20, weight="bold")
    svg.text(svg.w / 2, 58, subtitle, size=13, fill="#555")
    svg.add(f'<rect x="{ox:.1f}" y="{oy:.1f}" width="{BOARD_W * S:.1f}" height="{BOARD_H * S:.1f}" rx="{S:.1f}" '
            f'fill="{BOARD_FILL}" stroke="#8a7a4a" stroke-width="2"/>')
    for c in COLS:
        x, _ = svg.p(c + "1")
        svg.text(x, oy - 30, c, size=14, weight="bold", fill="#444")
    for r in range(1, ROWS + 1):
        _, y = svg.p("A%d" % r)
        svg.text(ox - 16, y + 5, str(r), size=14, weight="bold", fill="#444")
        svg.text(ox + BOARD_W * S + 16, y + 5, str(r), size=14, weight="bold", fill="#444")
    # compass, left of the title area
    cx, cy = 60, 120
    svg.line(cx, cy + 22, cx, cy - 18, "#333", 2.5)
    svg.add(f'<polygon points="{cx},{cy - 28} {cx - 7},{cy - 14} {cx + 7},{cy - 14}" fill="#333"/>')
    svg.text(cx, cy - 34, "N", size=15, weight="bold")
    svg.text(svg.ox + BOARD_W * S / 2, oy - 58, "north edge: row 1, button side (north wall)", size=12, fill="#555")
    # west / east
    west_x, east_x = (ox + BOARD_W * S + 50, ox - 50) if svg.mirrored else (ox - 50, ox + BOARD_W * S + 50)
    svg.text(west_x, oy + BOARD_H * S / 2 + 5, "W", size=15, weight="bold", fill="#777")
    svg.text(east_x, oy + BOARD_H * S / 2 + 5, "E", size=15, weight="bold", fill="#777")
    # scale bar 5 mm
    sx, sy = svg.w - 40 - 5 * S, 120
    svg.line(sx, sy, sx + 5 * S, sy, "#333", 2)
    svg.line(sx, sy - 5, sx, sy + 5, "#333", 2)
    svg.line(sx + 5 * S, sy - 5, sx + 5 * S, sy + 5, "#333", 2)
    svg.text(sx + 2.5 * S, sy - 9, "5 mm", size=12)
    svg.text(svg.w - 40, sy + 22, "board 30.5 × 10.2 mm, pitch 2.54 mm", size=11, anchor="end", fill="#555")


def parts_side(path):
    svg = Svg(1000, 720, 110, 200, mirrored=False)
    frame(svg, "Perfboard, PARTS side (seen from above, LED up)",
          "Holes A–L × 1–4. Leads go down through the board; all soldering is on the copper side.")
    used = set(LED_LEGS) | {h for w in WIRES for h in w[:1]} | {h for r in RESISTORS for h in r[:2]}
    for c in COLS:
        for r in range(1, ROWS + 1):
            x, y = svg.p(f"{c}{r}")
            svg.circle(x, y, 0.5 * S, "#6b5a2e" if f"{c}{r}" not in used else "#3a2f14")
    # A1 marker
    x, y = svg.p("A1")
    svg.circle(x, y, 0.95 * S, "none", "#d81b60", 2.5)
    svg.text(x + 28, y - 18, "write “A1” here", size=11, anchor="start", fill="#d81b60")
    # resistors, lying flat
    for a, b, label, bands in RESISTORS:
        (x1, y1), (x2, y2) = svg.p(a), svg.p(b)
        svg.line(x1, y1, x2, y2, "#9e9e9e", 0.5 * S)
        svg.circle(x1, y1, 0.35 * S, "#9e9e9e")
        svg.circle(x2, y2, 0.35 * S, "#9e9e9e")
        bl, bh = 6.0 * S, 2.3 * S
        bx, by = (x1 + x2) / 2 - bl / 2, y1 - bh / 2
        svg.add(f'<rect x="{bx:.1f}" y="{by:.1f}" width="{bl:.1f}" height="{bh:.1f}" rx="{bh / 2:.1f}" '
                f'fill="#d9c28f" stroke="#6d5d36" stroke-width="1.5"/>')
        for i, col in enumerate(bands):
            fx = bx + bl * (0.2 + 0.14 * i) if i < 3 else bx + bl * 0.8
            svg.add(f'<rect x="{fx - 0.25 * S:.1f}" y="{by + 1:.1f}" width="{0.5 * S:.1f}" height="{bh - 2:.1f}" '
                    f'fill="{col}"/>')
        svg.text((x1 + x2) / 2, y1 - bh / 2 - 6 if a[1] != "3" else y1 + bh / 2 + 16, label, size=12, weight="bold")
    # LED: flange Ø 5.8, body Ø 5, centred on F2/G2/F3/G3
    (fx2, fy2), (gx3, gy3) = svg.p("F2"), svg.p("G3")
    cx, cy = (fx2 + gx3) / 2, (fy2 + gy3) / 2
    svg.circle(cx, cy, 2.9 * S, "#eeeeee", "#8d8d8d", 2, 'fill-opacity="0.75"')
    svg.circle(cx, cy, 2.5 * S, "#ffffff", "#bdbdbd", 1.5, 'fill-opacity="0.6"')
    for hole, leg in LED_LEGS.items():
        x, y = svg.p(hole)
        col = {"R": "#d32f2f", "G": "#2e7d32", "B": "#1565c0", "C": "#212121"}[leg]
        svg.circle(x, y, 0.42 * S, col)
        svg.text(x + (-15 if hole[0] == "F" else 15), y + 5, leg, size=13, weight="bold", fill=col)
    # wire holes (wires enter from the copper side)
    for hole, colour, name, dest in WIRES:
        x, y = svg.p(hole)
        svg.circle(x, y, 0.62 * S, "none", "#555" if colour == "#f5f5f5" else colour, 4)
    # labels of the wire holes, below the board
    by = svg.oy + BOARD_H * S
    for hole, colour, name, dest in WIRES:
        x, y = svg.p(hole)
        dy = STAGGER.get(hole, 0)
        svg.line(x, y + 0.7 * S, x, by + 30 + dy, "#777", 1, 'stroke-dasharray="3,3"')
        svg.text(x, by + 46 + dy, f"wire {name}", size=12, weight="bold")
        svg.text(x, by + 62 + dy, f"→ {dest}", size=11, fill="#444")
    svg.text(svg.ox + BOARD_W * S / 2, by + 132, "south edge: row 4, XIAO side. Wires come in from the copper side "
             "(ringed holes): only their trimmed tips show here.", size=12, fill="#555")
    # legend
    ly = 640
    svg.text(110, ly - 20, "Circle in the middle: the 5 mm RGB LED (flange Ø 5.8 mm), sitting on its flange, centred on the board and the base.", size=12, anchor="start")
    svg.text(110, ly, "LED legs (check yours with the diode test first): R = red, G = green, B = blue, "
             "C = common cathode (longest leg).", size=12, anchor="start")
    svg.text(110, ly + 20, "Resistors lie flat, body on the board, 4 holes apart. Never below 100 Ω on red. "
             "Unused holes stay empty.", size=12, anchor="start")
    svg.text(110, ly + 40, "Source: docs/assembly.md §3.1 (generated by scripts/draw_perfboard.py).", size=11,
             anchor="start", fill="#777")
    svg.save(path)


def copper_side(path):
    svg = Svg(1000, 820, 110, 200, mirrored=True)
    frame(svg, "Perfboard, COPPER side (board turned over left-to-right, seen from below)",
          "Columns now read L … A from left to right. North (row 1) is still at the top.")
    used = set(LED_LEGS) | {w[0] for w in WIRES} | {h for r in RESISTORS for h in r[:2]} | \
        {h for link in LINKS for h in link}
    for c in COLS:
        for r in range(1, ROWS + 1):
            x, y = svg.p(f"{c}{r}")
            svg.circle(x, y, 0.85 * S, PAD_COPPER)
            svg.circle(x, y, 0.45 * S, "#4a3a1a")
    # links (solder / lead offcut) between neighbouring holes
    for a, b in LINKS:
        (x1, y1), (x2, y2) = svg.p(a), svg.p(b)
        svg.line(x1, y1, x2, y2, SOLDER_EDGE, 0.95 * S)
        svg.line(x1, y1, x2, y2, SOLDER, 0.7 * S)
    for hole in sorted(used):
        x, y = svg.p(hole)
        svg.circle(x, y, 0.8 * S, SOLDER, SOLDER_EDGE, 1.5)
    # LED legs: must NOT be joined to each other
    for hole, leg in LED_LEGS.items():
        x, y = svg.p(hole)
        col = {"R": "#d32f2f", "G": "#2e7d32", "B": "#1565c0", "C": "#212121"}[leg]
        svg.text(x, y + 5, leg, size=13, weight="bold", fill=col)
    # resistor leads (labels only)
    for a, b, label, _ in RESISTORS:
        for h in (a, b):
            x, y = svg.p(h)
            svg.text(x, y + 4, "r", size=11, fill="#555")
    # wires leaving south
    by = svg.oy + BOARD_H * S
    for hole, colour, name, dest in WIRES:
        x, y = svg.p(hole)
        stroke = "#9e9e9e" if colour == "#f5f5f5" else colour
        dy = STAGGER.get(hole, 0)
        svg.line(x, y, x, by + 50 + dy, "#555", 0.75 * S + 2)
        svg.line(x, y, x, by + 50 + dy, colour, 0.75 * S)
        svg.circle(x, y, 0.42 * S, SOLDER, SOLDER_EDGE, 1.2)
        svg.text(x, by + 70 + dy, f"wire {name}", size=12, weight="bold", fill="#222" if stroke == "#9e9e9e" else stroke)
        svg.text(x, by + 86 + dy, f"→ {dest}", size=11, fill="#444")
    svg.text(svg.ox + BOARD_W * S / 2, by + 150, "south edge: row 4, XIAO side", size=12, fill="#555")
    # net table
    ly = 660
    nets = [
        ("#d32f2f", "red (D0)", "F2–E2 (LED R → resistor);  A2–A3–A4 (resistor → wire 0)"),
        ("#2e7d32", "green (D2)", "G2–G1 (LED G → resistor);  K1–J1–J2–J3–J4 (resistor → wire 2; J1, J3 are free pads)"),
        ("#1565c0", "blue (D3)", "G3–H3 (LED B → resistor);  L3–L4 (resistor → wire 3)"),
        ("#212121", "ground", "F3–F4 (LED cathode → wire N);  E4–F4 (button wire S)"),
    ]
    for i, (col, net, holes) in enumerate(nets):
        y = ly + 20 * i
        svg.add(f'<rect x="110" y="{y - 11}" width="14" height="14" fill="{col}"/>')
        svg.text(132, y, f"<tspan font-weight=\"bold\">{net}</tspan>: {holes}", size=12, anchor="start")
    notes = [
        "Grey = solder link between neighbouring pads (bent lead or tinned offcut). r = resistor lead. "
        "Every other pad stays bare.",
        "R, G, C, B are neighbours: never bridge them. Trim leads ≤ 0.5 mm, then one layer of Kapton tape "
        "over the whole side (it faces the cell).",
        "Source: docs/assembly.md §3.2–3.3 (generated by scripts/draw_perfboard.py).",
    ]
    for i, n in enumerate(notes):
        svg.text(110, ly + 90 + 20 * i, n, size=12 if i < 2 else 11, anchor="start", fill="#222" if i < 2 else "#777")
    svg.save(path)


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    out = os.path.join(root, "docs", "img")
    os.makedirs(out, exist_ok=True)
    parts_side(os.path.join(out, "perfboard-parts-side.svg"))
    copper_side(os.path.join(out, "perfboard-copper-side.svg"))
    print("wrote", out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
