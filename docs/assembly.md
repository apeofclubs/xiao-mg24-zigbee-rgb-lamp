# Final assembly: perfboard and base (Ø 36.5 × 19.5 mm)

> Validation level: **written**. Nobody on the agent side can build or fit this. Every dimension below comes from
> the operator's measurement of the base (inner Ø 36.5 mm, inner height 19.5 mm, 2026-11-24), from datasheet or
> typical part sizes, or from estimates marked "≈". **Do the paper mock-up in §5.1 before cutting or gluing
> anything.** The electrical design is unchanged: same pins, same resistors, same rules as `wiring.md`.

Read first: `wiring.md` (resistors, button, **Battery connection** steps 1–5), `hardware-assumptions.md` H1, H2, H8,
H11, H14. Pins: `pinout.md`.

## 1. Choice of placement: XIAO upright, USB-C through the floor

Sizes used (check yours with calipers, §5.1):

| Part | Size | Source |
|---|---|---|
| Base, inside | Ø 36.5 mm, 19.5 mm high (from the inner floor to the rim) | operator, measured |
| XIAO MG24 | 21 × 17.8 mm PCB [S1]; ≈ 1.2 mm PCB; ≈ 3.2 mm USB-C receptacle on the component side | Seeed wiki; receptacle: typical mid-size USB-C |
| Cell Liter Energy 902025 | 9 × 20 × 25 mm (thickness × width × length), leads from a 20 mm end | part number; measure (the PCM end can add 1–2 mm) |
| 5 mm RGB LED | ≈ 8.6 mm tall with its flange, flange Ø ≈ 5.8 mm | typical 5 mm LED |
| ¼ W resistor | body ≈ 6.3 × 2.5 mm | typical |
| Tactile switch | 6 × 6 mm, body 3.5 mm thick | typical |
| USB-C cable plug | overmould ≤ 12.35 × 6.5 mm (USB-C specification maximum; most cables are smaller) | |

Three ways to place the XIAO were compared:

| | Upright, USB-C down through the floor (**recommended**) | Tilted | Flat, USB-C through a side slot |
|---|---|---|---|
| Height | 21.0 mm: **1.5 mm above the rim** | 21 cos θ + t sin θ ≤ 19.5 mm needs θ ≈ 33° (bare board, t ≈ 3.5 mm) to ≈ 45° (with wires on the pads, t ≈ 6 mm); a small tilt (≤ 15°) gains nothing (21.3 mm at 10°) | ≈ 3.5 mm thick, fits under the rim |
| Floor footprint | 17.8 × ≈ 6 mm along the wall | 14–19 mm deep instead of 6 | 21 × 17.8 mm |
| Room left for the cell (20 × 25 mm flat) | yes, **≈ 1 mm to the wall at its two far corners**, 3 mm from the XIAO | no: 6 → 14+ mm of depth leaves less than the cell's 20 mm | no side by side (17.8 + 20 = 37.8 mm > 36.5 mm). Only stacked: cell on the floor, XIAO on a deck at ≈ 10.5–14 mm, perfboard on top at ≥ 15 mm: LED tip ≈ 25 mm (5.5 mm above the rim), antenna sandwiched between the cell and the perfboard |
| Antenna | end opposite the USB-C, at the top, above the rim: best of the three | | between the cell and the perfboard: worst |
| USB-C | from underneath (window in the floor) | at an angle through the floor | side slot at mid-height |
| Verdict | **chosen** | rejected (cell no longer fits) | possible but 3 layers high and poorer radio |

**Female headers: no.** A header strip adds at least 8.5 mm and needs a carrier board under the XIAO. Neither the
upright nor the flat layout has that height. The XIAO is wired directly on its castellated pads.

Consequences of the upright choice, to accept or refuse (questions at the end):

* The XIAO's top edge is ≈ 1.5 mm above the rim, the LED's tip ≈ 2.2 mm. Whatever sits on the base (shade,
  diffuser, lid) needs that much room. A recess in the lid over the XIAO can also hold it down (§5.5).
* Charging and flashing: the cable plugs in **from underneath**. Lay the lamp on its side or on a stand while it
  charges.

## 2. Top view of the base (to scale)

Coordinates in mm, origin at the centre of the base, **south = the XIAO side, north = the button side**, z = height
above the inner floor. Each character is 0.5 mm wide and each line 1 mm high, so the drawings are to scale. The marks
on the top and bottom rulers are every 5 mm (−20 … +20); the labels on the left are y.

### 2.1 Level 1: floor to z ≈ 11 mm (cell, XIAO, connector, wall ledges)

```
     |         |         |         |         |         |         |         |         |
+20

                                   ooooooooooooooooooooo
                              ooooJJJJJJJJJJJJJJJJJJJJJJJoooo
                          oooo    JJJJJJJJJJJJJJJJJJJJJJJ    oooo
+15                    oooo     ~ JJJJJJJJJJJJJJJJJJJJJJJ       oooo
                     ooo ~        JJJJJJJJJJJJJJJJJJJJJJJ          ooo
                   *o             JJJJJJJJJJJJJJJJJJJJJJJ             oo
                 oo CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC oo
               ooo  C                                                 C  ooo
+10           oo    C                                                 C    oo
             oo   * C                                                 C     oo
            oo   ~  C                                                 C      oo
           oo       C                                                 C       oo
          oo      * C                                                 C        oo
 +5       LLLLLLLLL C                                                 C LLLLLLLLL
         oLLLLL~LLL C                                                 C LLLLLLLLLo
         oLLLLLLLLL*C                                                 C LLLLLLLLLo
         oLLLLLLLLL C                                                 C LLLLLLLLLo
         oLLLLLLLLL C                                                 C LLLLLLLLLo
 +0      oLLLL~LLLL C                        +                        C LLLLLLLLLo
         o          C                                                 C          o
         o          C                                                 C          o
         o    ~     C                                                 C          o
         oo         C                                                 C         oo
 -5       o         C                                                 C         o
          oo        C                                                 C        oo
           oo   ~   C                                                 C       oo
            oo      CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC      oo
             oo                                                             oo
-10           oo     ~    ~     ===========================                oo
               ooo              ===========================              ooo
                 oo        XXXXXXXXXUUUUUUUUUUUUUUUUUUUXXXXXXXXX        oo
                   oo      XXXXXXXXXUUUUUUUUUUUUUUUUUUUXXXXXXXXX      oo
                     ooo   XXXXXXXXXUUUUUUUUUUUUUUUUUUUXXXXXXXXX   ooo
-15                    ooooXXXXXXXXXUUUUUUUUUUUUUUUUUUUXXXXXXXXXoooo
                          oXXXXXXXXXUUUUUUUUUUUUUUUUUUUXXXXXXXXXo
                              oo=ooo                   ooo=oo
                                ===========================

-20
     |         |         |         |         |         |         |         |         |
     x: -20                -10                0                 +10                +20 mm
```

| Mark | What | Where (x, y in mm) | z (mm) |
|---|---|---|---|
| `o` | base wall, inner Ø 36.5 | r = 18.25 | 0–19.5 |
| `X` / `U` | XIAO upright; `U` = its USB-C receptacle. Component side towards the wall | x −8.9…+8.9; component face y ≈ −15.5, PCB y −12.3…−11.1 | 0–21.0 |
| `=` | window in the floor for the USB-C plug, **13 × 7.5 mm**, centred on the receptacle | x −6.5…+6.5, y −17.65…−10.15 | through the floor |
| `C` | cell, flat on the floor, leads/PCM end to the **west** | x −12.5…+12.5, y −8.1…+11.9 | 0–9.0 |
| `J` | JST pair (pigtail + cell plug, mated), standing on its edge | x −5.5…+5.5, y +12.9…+17.0 | 0–6 |
| `L` | two wall ledges carrying the perfboard (4 mm deep, 5 mm wide) | x ±13.5…±17.5, y 0…+5 | **top at 11.5**, bottom ≈ 7.5 |
| `~` | pigtail path: XIAO underside BAT pads → west gap → north to `J`, at floor level, under the west ledge | | 0–6 |
| `*` | cell leads: from the cell's west end → north to `J` | | 0–9 |
| `+` | centre of the base | 0, 0 | |

The window touches the foot of the wall at its two southern corners (by ≈ 0.6 mm): cut it through the floor only;
the wall above stays whole.

### 2.2 Level 2: z ≈ 11.5 mm to the top (perfboard, LED, button)

```
     |         |         |         |         |         |         |         |         |
+20
                                             ^
                                   ooooSSSSSSSSSSSSSoooo
                              oooooo   SSSSSSSSSSSSS   oooooo
                          oooo         SSSSSSSSSSSSS         oooo
+15                    oooo            SSSSSSSSSSSSS            oooo
                     ooo               SSSSSSSSSSSSS               ooo
                   oo                                                 oo
                 oo                                                     oo
               ooo                                                       ooo
+10           oo                                                           oo
             oo                                                             oo
            oo                                                               oo
           oo                                                                 oo
          oo                                                                   oo
 +5       o    PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPggggggggggggggPPPPPPPPPPP    o
         oo    P                                   gggggggggggggg          P    oo
         o     P                          OOOOOOO  gggggggggggggg          P     o
         o     P     rrrrrrrrrrrrr      OOOOOOOOOOO                        P     o
         o     P     rrrrrrrrrrrrr     OOOOOOOOOOOOO                       P     o
 +0      o     P     rrrrrrrrrrrrr     OOOOOOOOOOOOO    bbbbbbbbbbbbbb     P     o
         o     P                       OOOOOOOOOOOOO    bbbbbbbbbbbbbb     P     o
         o     P                        OOOOOOOOOOO     bbbbbbbbbbbbbb     P     o
         o     P                          OOOOOOO                          P     o
         oo    P 0                   s    N                    2         3 P    oo
 -5       o    PPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPPP    o
          oo                                                                   oo
           oo                                                                 oo
            oo                                                               oo
             oo                                                             oo
-10           oo                                                           oo
               ooo                                                       ooo
                 oo        XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX        oo
                   oo      XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX      oo
                     ooo   XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX   ooo
-15                    ooooXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXoooo
                          oXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXo
                              oooooo                   oooooo
                                   ooooooooooooooooooooo

-20
     |         |         |         |         |         |         |         |         |
     x: -20                -10                0                 +10                +20 mm
```

| Mark | What | Where | z (mm) |
|---|---|---|---|
| `P` | perfboard outline, 30.5 × 10.2 mm, resting on the two ledges | x −15.24…+15.24, y −5.08…+5.08 | 11.5–13.1 |
| `O` | LED, **centred on the base** | 0, 0 | 13.1–≈ 21.7 |
| `r` `g` `b` | resistors lying flat: red 100 Ω, green 220 Ω, blue 100 Ω | rows 2, 1, 3 of §3 | 13.1–15.6 |
| `0` `2` `3` `N` `s` | wire exits (south edge): to D0, D2, D3, XIAO GND (`N`); `s` = to the button | row 4 of §3 | |
| `S` | button, body glued to the inside of the **north wall**, plunger through a Ø 4 mm hole (`^`) | x −3…+3, y +14.5…+18 | 12–18, plunger axis at z = 15 |
| `X` | XIAO (upper part, antenna end) | as level 1 | up to 21.0 |

Wire paths (all at board level or along the XIAO, never lying on the cell):

* **The four board wires (`0`, `N`, `2`, `3`)** leave the board's south edge, cross the 6 mm gap to the XIAO at
  z ≈ 11–13 (2 mm above the cell's top), then run down the XIAO's inner face to its edge pads. Seen from the
  centre of the base, looking at the XIAO (you then see its underside): **the D0–D6 edge is on your left (east)**,
  the 5V / GND / 3V3 / D10 / D9 edge is on your right (west). The red wire (`0`, west end of the board) crosses to
  the east edge: route it along the board's south edge.
* **Button**: the D9 wire runs along the west wall above the ledge to the XIAO's D9 pad; the button's ground wire runs
  over the board's parts side (west half, away from the LED) to hole E4.

Approximate pad heights on the upright XIAO (castellated pads at 2.54 mm, assumed centred on the 21 mm board):
pad 1 (D0 / 5V) z ≈ 3, pad 2 (D1 / GND) ≈ 5.5, pad 3 (D2 / 3V3) ≈ 8, pad 4 (D3 / D10) ≈ 10.5, pad 5 (D4 / D9) ≈ 13.

### 2.3 Side section (north–south through the centre, not to scale)

```
 z mm  south (XIAO)                                                north (button)
 21.7                                ▲ LED tip
 21.0   ┌─┐  XIAO top / antenna      │
 19.5 ──┤X├───────────────── rim ────┼───────────────────────────────── rim ──
        │X│                         ╭┴╮                              ┌─┐
        │X│   resistors 13.1–15.6   │O│                          [S]═│ │ Ø4 hole, z 15
 13.1   │X│    ════════════ perfboard ═══════════                  │ │
 11.5   │X│    (rests on the east/west wall ledges)                │ │
        │X│ ≥ 1.9 mm free above the cell (swelling)                │ │
  9.0   │X│  ┌────────────── cell 902025, 9 mm ─────────────┐      │ │
        │X│  │                                              │ [J]  │ │
  0   ──┴U┴──┴──────────────────────────────────────────────┴──────┘ │
         ▼ USB-C plug from underneath, through the 13 × 7.5 mm window
```

### 2.4 Clearances

| Between | Gap |
|---|---|
| XIAO PCB underside ↔ cell (south edge) | 3.0 mm (the pigtail's two wires pass here) |
| cell top ↔ lowest point under the perfboard (joints trimmed to ≤ 0.5 mm + Kapton) | ≥ 1.9 mm |
| cell far corners (±12.5, +11.9) ↔ wall | ≈ 1 mm |
| cell ends ↔ ledges | 1 mm |
| perfboard corners ↔ wall | ≥ 1.9 mm |
| perfboard south edge ↔ XIAO PCB | 6 mm (wire crossing) |
| XIAO top above the rim | 1.5 mm |
| LED tip above the rim | ≈ 2.2 mm (less if your LED is shorter) |

## 3. Perfboard layout, hole by hole

**Board**: single-sided FR4 prototype board, 2.54 mm pitch, round pads, 1.6 mm thick. Keep **12 columns × 4 rows**
of holes (A–L × 1–4): **30.5 × 10.2 mm**. To cut it: count 14 × 6 holes, score both faces with a knife along the
outer row and column of holes on each side (through the hole centres), several passes against a steel ruler, snap
over a table edge, file the edges flat. Then round the four corners with a file (≈ 1 mm), wear a mask: FR4 dust
is an irritant. Do not cut with scissors (it splits).

Orientation used everywhere: **row 1 north (button side), row 4 south (XIAO side), column A west, L east**, parts
side up. Mark "A1" on the parts side with a pen before starting.

### 3.1 Parts side (seen from above)

```
      A    B    C    D    E    F    G    H    I    J    K    L
 1    ·    ·    ·    ·    ·    ·    g━━━━━[ 220 Ω ]━━━━━g    ·
 2    r━━━━━[ 100 Ω ]━━━━━r    R    G    ·    ·    ·    ·    ·
 3    ·    ·    ·    ·    ·    C    B    b━━━━━[ 100 Ω ]━━━━━b
 4    0    ·    ·    ·    S    N    ·    ·    ·    2    ·    3
```

| Hole | What goes in it |
|---|---|
| F2 | LED **R** leg |
| G2 | LED **G** leg |
| F3 | LED **common cathode** (longest leg) |
| G3 | LED **B** leg |
| A2, E2 | red resistor **100 Ω** (lying flat, body over B2–D2) |
| G1, K1 | green resistor **220 Ω** (body over H1–J1) |
| H3, L3 | blue resistor **100 Ω** (body over I3–K3) |
| A4 | wire `0` → XIAO **D0** (red wire) |
| J4 | wire `2` → XIAO **D2** (green wire) |
| L4 | wire `3` → XIAO **D3** (blue wire) |
| F4 | wire `N` → XIAO **GND** pad (black wire) |
| E4 | wire `S` → one diagonal pin of the button (black or white wire) |

The LED: a 5 mm RGB LED has its 4 legs in a line at ≈ 1.27 mm, in the order **R – C – G – B** (C = cathode, the
longest leg). Check yours first with the multimeter's diode test (black probe on the long leg, red probe on each
other leg: that colour glows faintly; `hardware-assumptions.md` H3). Then bend the legs into a 2 × 2 square: hold
the LED with the legs in a line from west to east (R west), bend **R and G slightly north** (into row 2) and **C
and B slightly south** (into row 3), legs straight down otherwise. The LED then sits on its flange, centred between
F2, G2, F3 and G3, which is the centre of the board and of the base.

The resistors lie flat, 4 holes apart (10.16 mm), body touching the board. Same values as the breadboard: **never
below 100 Ω on red** (`wiring.md`).

### 3.2 Copper side

Every link (`━`, `┃`) is between **neighbouring** holes. Make it by bending the lead of one part flat onto the next
pad and soldering both, or with a piece of tinned wire (a resistor lead offcut) for the longer runs. No solder blob
across two pads that are not linked here.

```
COPPER SIDE seen THROUGH the board from above (X-ray, same orientation as the parts view)
      A    B    C    D    E    F    G    H    I    J    K    L
 1    ·    ·    ·    ·    ·    ·    ●    ·    ·    ●━━━━●    ·
                                    ┃              ┃
 2    ●    ·    ·    ·    ●━━━━●    ●    ·    ·    ●    ·    ·
      ┃                                            ┃
 3    ●    ·    ·    ·    ·    ●    ●━━━━●    ·    ●    ·    ●
      ┃                        ┃                   ┃         ┃
 4    ●    ·    ·    ·    ●━━━━●    ·    ·    ·    ●    ·    ●

COPPER SIDE seen from BELOW (board turned over left-to-right: columns read L ... A)
      L    K    J    I    H    G    F    E    D    C    B    A
 1    ·    ●━━━━●    ·    ·    ●    ·    ·    ·    ·    ·    ·
                ┃              ┃
 2    ·    ·    ●    ·    ·    ●    ●━━━━●    ·    ·    ·    ●
                ┃                                            ┃
 3    ●    ·    ●    ·    ●━━━━●    ●    ·    ·    ·    ·    ●
      ┃         ┃                   ┃                        ┃
 4    ●    ·    ●    ·    ·    ·    ●━━━━●    ·    ·    ·    ●
```

| Net | Holes joined on the copper side |
|---|---|
| red (D0) | F2–E2 (LED R → resistor), A2–A3–A4 (resistor → wire `0`) |
| green (D2) | G2–G1 (LED G → resistor), K1–J1–J2–J3–J4 (resistor → wire `2`; J1 and J3 are free pads under resistor bodies) |
| blue (D3) | G3–H3 (LED B → resistor), L3–L4 (resistor → wire `3`) |
| ground | F3–F4 (LED cathode → wire `N`), E4–F4 (button wire `S`) |

Every other pad stays bare. The four LED legs are neighbours of each other by construction (F2, G2, F3, G3): they
must **not** be joined. Trim every lead and link to **≤ 0.5 mm** above its joint (the copper side faces the cell),
then cover the whole copper side with one layer of **Kapton tape**.

### 3.3 Wires on the board

Insert each wire from the **copper side** (stripped 2 mm, tinned), insulation against the pad, solder on the copper
side, trim the copper that pokes out on the parts side. Lengths, generous on purpose so the board can be lifted out
and turned over later:

| Wire | Hole → end | Length | Colour |
|---|---|---|---|
| `0` | A4 → XIAO D0 | 60 mm | red |
| `2` | J4 → XIAO D2 | 50 mm | green |
| `3` | L4 → XIAO D3 | 50 mm | blue |
| `N` | F4 → XIAO GND | 50 mm | black |
| `S` | E4 → button, diagonal pin 1 | 60 mm | white |
| D9 | button, diagonal pin 2 → XIAO D9 | 80 mm | yellow |

That is **5 wires to the XIAO** (D0, D2, D3, D9, GND) and one ground for the LED and the button (the board's F4),
**never BAT−**.

### 3.4 Why the button is not on the board

On a flat board the button's plunger would point up, into whatever sits on the base, and could not be pressed from
outside. So the 6 × 6 mm button is glued inside the **north wall** (opposite the USB-C), plunger through a hole,
and wired with two wires: one to the board's ground (E4), one straight to D9. The electrical wiring is exactly the
one in `wiring.md` (two diagonal pins, no resistor, no capacitor).

What the base needs:

* a **Ø 4 mm hole** in the north wall, centre **15 mm above the inner floor** (the hole stays 2.5 mm below the rim);
* a switch whose plunger sticks out of the wall by ≈ 1 mm: total switch height ≈ 3.5 mm (body) + wall thickness +
  1 mm. Standard heights: 4.3, 5, 6, 7, 8, 9.5, 11, 13 mm. Example: a 2 mm wall → a **6 × 6 × 7 mm** switch. If the
  plunger is too short, glue a small cap or a printed pin in the hole as an actuator. A 6 × 6 switch has a Ø 3.5 mm
  round plunger: the 4 mm hole leaves it free.
* the switch body glued square against the wall (two dots of hot glue on its sides, **none on the plunger** or in
  the hole). Its four legs point inwards: bend the two unused ones flat, keep the two diagonal ones for the wires.

## 4. Bill of materials (final build)

| Qty | Part | Exact spec |
|---|---|---|
| 1 | Seeed Studio XIAO MG24 | the one already flashed (H12: standard, not Sense) |
| 1 | LiPo Liter Energy 902025 | 1 cell, 3.7 V, 400 mAh, **with PCM** (H1), charge ≥ 200 mA (H2) |
| 1 | 2-pin JST pigtail | **same series as the cell's plug**, after measuring the pitch (`wiring.md` table: JST-PH 2.0 mm most likely; PicoBlade 1.25, ZH 1.5, SH 1.0). Wires ≤ 60 mm, 26–28 AWG |
| 1 | RGB LED, 5 mm, 4 legs, **common cathode** | the Elegoo LED used on the bench |
| 2 | resistor 100 Ω, ¼ W, through-hole | red and blue (5 % or better) |
| 1 | resistor 220 Ω, ¼ W, through-hole | green |
| 1 | tactile switch 6 × 6 mm, 4 pins, through-hole | height to suit the wall, §3.4 (e.g. 6 × 6 × 7 mm) |
| 1 | prototype board | **single-sided FR4**, 2.54 mm pitch, round pads, 1.6 mm; at least 14 × 6 holes. Not stripboard, not paper (FR2) |
| 1 m | silicone wire **28 AWG** (≈ 0.9 mm outside) | red, green, blue, black, white, yellow if possible; 30 AWG also works |
| 10 cm | heat-shrink **Ø 1.5 mm** (2:1) | over the two button-leg joints and any wire-to-wire splice |
| 10 cm | heat-shrink **Ø 3 mm** (2:1) | optional, to sleeve the 4 board wires together near the XIAO |
| 1 roll | **Kapton** (polyimide) tape, 10 mm | copper side of the board, pigtail joints on the BAT pads, insulation |
| – | hot glue (low-temperature if possible) | XIAO, board on ledges, button, wire strain relief. **Never on the cell** |
| 2 | wall ledges, 4 × 5 mm, 4 mm tall (top at 11.5 mm) | printed with the base, or plastic offcuts glued to the wall |
| – | thin double-sided tape (≤ 0.2 mm, removable) | under the cell, optional |
| 0 | female headers | **none**: no height for them (§1) |

## 5. Assembly procedure

Tools: fine-tip soldering iron (≈ 330 °C), flux, multimeter (continuity, Ω, diode, DC V), flush cutters, calipers,
knife and steel ruler, file, hot glue gun. USB unplugged and cell unplugged unless a step says otherwise.

### 5.1 Measure and mock up (nothing soldered, nothing cut)

1. Measure with calipers and write down: the cell (thickness, width, length **including** the PCM end and the
   lead exit), the XIAO's length from the USB-C face to the far edge, the LED's height with its flange, the base's
   wall and floor thicknesses, and the JST pitch (`wiring.md`, "Which JST").
2. Draw §2's two levels at 1:1 on paper (a Ø 36.5 mm circle) and lay the real cell, the XIAO (standing) and a cut
   card of 30.5 × 10.2 mm on it. If the cell is bigger than 20 × 25 mm, or a corner touches the wall, stop and
   report the numbers: the layout must change before anything is cut.
3. Check what will sit on the base: is there room for the XIAO 1.5 mm above the rim and the LED ≈ 2.2 mm above it?

### 5.2 Perfboard (no XIAO yet)

4. Cut the board (§3). Mark A1.
5. Identify the LED legs (diode test, §3.1). Bend them into the square, insert the LED in F2/G2/F3/G3, flange
   flat on the board. Solder **one** leg, check that the LED is upright and seated, then the other three.
6. Insert the three resistors flat (A2–E2 100 Ω, G1–K1 220 Ω, H3–L3 100 Ω). Read the colour bands, or measure each
   one before inserting: 100 Ω = brown-black-brown, 220 Ω = red-red-brown (4-band code; 5-band: brown-black-black-black, red-red-black-black).
7. Make the links of §3.2 on the copper side.
8. Solder the five board wires (§3.3) into A4, J4, L4, F4, E4.
9. Button: solder wire `S` (from E4) to one leg and the yellow D9 wire to the **diagonally opposite** leg. Heat-shrink
   Ø 1.5 mm on both joints.
10. Trim everything to ≤ 0.5 mm on the copper side. Do not apply the Kapton yet (the tests use the pads).

**Check 1: board alone** (multimeter, nothing powered):

| Test | Probes | Expected |
|---|---|---|
| a. red resistor | wire `0` end ↔ LED R leg (top of F2, parts side) | **100 Ω** (95–105) |
| b. green resistor | wire `2` end ↔ LED G leg (G2) | **220 Ω** (209–231) |
| c. blue resistor | wire `3` end ↔ LED B leg (G3) | **100 Ω** (95–105) |
| d. ground | wire `N` ↔ LED cathode (F3), and wire `N` ↔ wire `S` | beep (continuity) |
| e. LEDs | diode mode, red probe on wire `0` / `2` / `3`, black on wire `N` | red / green / blue glows faintly, reading ≈ 1.7–2.3 V / 2.4–2.9 V / 2.6–3.0 V. Blue can read "OL" on a meter whose test voltage is under 3 V: then check it at the first power-up instead |
| f. reverse | the same with the probes swapped | OL (no reading) |
| g. no bridge between colours | `0` ↔ `2`, `0` ↔ `3`, `2` ↔ `3`, both polarities | OL |
| h. button | continuity, wire `N` ↔ yellow D9 wire end | **open at rest, beep while pressed**. A beep at rest: the two legs used are not diagonal (`wiring.md`, button step 4) |

Then cover the copper side with Kapton.

### 5.3 Wires to the XIAO (USB unplugged, no cell, no pigtail yet)

11. Tin the five XIAO pads: **D0, D2, D3** (D0–D6 edge, pads 1, 3, 4 from the USB-C end), **GND** and **D9** (the
    other edge, pads 2 and 5 from the USB-C end). Check the silkscreen of your board: these are the same pads as on
    the breadboard.
12. Solder the wires to the pads from the **underside** face (the one that will face the inside of the base), short
    stripped ends (1.5 mm), no solder running onto the neighbouring pad. `0` → D0, `2` → D2, `3` → D3, `N` → GND,
    yellow → D9.
13. Recommended for recovery: solder a 40 mm wire to **D1**, close its free end with a piece of heat-shrink folded
    over (no bare copper), and leave it coiled near the USB-C window. It lets you hold D1 to GND at boot later (the
    escape that keeps the lamp awake, `flashing.md`) without unsoldering anything. It does nothing otherwise (the
    firmware reads D1 once at boot, then disables it).
14. A dot of hot glue on each wire 3–5 mm from its pad (strain relief), not over the joint itself.

**Check 2: XIAO wired** (multimeter, nothing powered):

| Test | Probes | Expected |
|---|---|---|
| a. ground | XIAO GND pad ↔ LED cathode (F3) | beep |
| b. channels | D0 pad ↔ LED R leg; D2 pad ↔ LED G leg; D3 pad ↔ LED B leg | 100 Ω; 220 Ω; 100 Ω |
| c. button | D9 pad ↔ GND pad | open (or > 100 kΩ) at rest, beep pressed |
| d. no short 3V3–GND | 3V3 pad ↔ GND pad, Ω range | **not 0 Ω** (the reading may rise slowly as the capacitors charge; anything above ≈ 1 kΩ is fine) |
| e. no short 5V–GND | 5V pad ↔ GND pad | same |
| f. no short BAT+–BAT− | underside BAT+ ↔ BAT− pads | same, **not 0 Ω** |
| g. neighbours | D0↔D1, D1↔D2, D2↔D3, D3↔D4, 5V↔GND, GND↔3V3, D10↔D9, D9↔D8 | no beep |
| h. nothing on 5V, 3V3, BAT+, BAT− | look | no wire on them yet |

### 5.4 First power-up on USB, without the cell

15. Lay everything on a non-conductive surface, nothing touching. Plug the XIAO into the PC by USB-C. The cell is
    **not** connected; the BAT pads have nothing on them.
16. Multimeter DC V: 3V3 pad ↔ GND **3.2–3.4 V**. If it is lower, or the board gets warm, unplug and recheck
    Check 2.
17. Open the RTT console (`flashing.md` §4.1, or §6.5 on Windows). `lamp status`, then `lamp rgb 255 0 0`,
    `lamp rgb 0 255 0`, `lamp rgb 0 0 255`, `lamp rgb 255 255 255`, `lamp off`: each colour lights alone, white
    is white-ish (balance as on the bench), off is fully dark.
18. Short press on the button: the lamp toggles, the console prints `Button: toggle`.
19. Without a cell, the XIAO's charge LED may blink or flicker: ignore it, nothing must be connected to BAT+/BAT−.
20. Unplug USB.

### 5.5 Battery pigtail (JST)

21. Follow `wiring.md`, **"Battery connection", steps 1–5, exactly** (polarity found with the real cell, pigtail
    soldered on BAT+/BAT− with USB and cell unplugged, continuity check, first plug-in with a positive reading on
    the pads). Cut the pigtail to ≈ 50 mm before soldering: enough to reach the north of the base along the west
    side (`~` path, §2.1).
22. Cover the two BAT joints with Kapton; a dot of hot glue on the pigtail 5 mm from the pads (on the wires, not on
    the cell).

**Check 3: battery**

| Test | How | Expected |
|---|---|---|
| a. polarity | cell plugged, USB unplugged: red probe BAT+, black BAT− | **positive**, the cell's voltage (≈ 3.7–4.2 V). Negative: unplug at once (`wiring.md` step 5) |
| b. runs on battery | RTT console, `lamp rgb 255 255 255` then `lamp off` | lights, then dark |
| c. charge | USB plugged with the cell in, on a non-flammable surface | charge LED on; after 10 min the cell is at most lukewarm |

Unplug the cell (pull on the plug's housing) before fitting into the base.

### 5.6 Fitting into the base

23. Base preparation (nothing inside yet): cut the **USB-C window** 13 × 7.5 mm in the floor (§2.1; check that your
    cable's plug overmould passes through it), drill the **Ø 4 mm button hole** in the north wall at z = 15 mm, and
    fit the **two ledges** (top exactly 11.5 mm above the inner floor, at the east and west walls, y 0…+5 mm).
    If the base is printed: add all three to the model instead; two vertical guide rails 1.4 mm apart (for the
    1.2 mm PCB) at the XIAO's ends make the XIAO's mounting much stronger.
24. Glue the button in the north wall (§3.4). Check its click from outside.
25. Stand the XIAO in place: component side towards the south wall, USB-C mouth over the window, PCB edge on the
    floor. Plug a USB-C cable from underneath to check the alignment, then unplug it. Hot glue: one dot at each
    upper corner of the XIAO against the wall (on the unused D6 / D7 pads' area, **not on the antenna end's
    middle, not on wired pads**), one bead along the bottom edge on the inner side, beside the window. Plugging the
    cable pushes the board upwards: the glue (or the lid, if it rests on the XIAO's top edge) must take that.
26. Route the pigtail along the `~` path (west side, at floor level, under the west ledge) to the north.
27. Lay the cell flat in its place, leads end to the west, ≥ 1 mm from the XIAO's underside and from the ledges,
    optionally on thin double-sided tape. Route its leads north to the connector (`*`).
28. Plug the cell into the pigtail; stand the mated JST on its edge in the north gap (`J`), away from the cell's
    corners and below the button.
29. Lay the perfboard on the two ledges, LED up, row 4 towards the XIAO, LED centred. A dot of hot glue on each
    ledge, at the board's ends.
30. Dress the wires: the four board wires along the board's south edge, then down the XIAO; the D9 wire along the
    west wall above the ledge; the button's ground wire over the board's west half. Nothing lies on the cell.

**Check 4: in the base, before closing**

| Test | How | Expected |
|---|---|---|
| a. lamp | USB unplugged; short press | toggles on/off |
| b. pairing | if it is paired already, nothing to do; otherwise `bench-test.md` §3.9 | as on the bench |
| c. charging | plug USB from underneath | charge LED on, lamp keeps working |
| d. radio | Zigbee2MQTT: the lamp's LQI from its parent, compared with the bench | similar (the antenna is above the rim, nothing metal over it) |

## 6. Before closing the base

* [ ] **No pressure on the cell**: it moves slightly when pushed with a finger; nothing rests on it; the board is
  ≥ 1.5 mm above it (look from the side, or slide a 1.5 mm drill shank between them).
* [ ] **No wire on the cell**, no wire pinched under the board, the ledges or the XIAO.
* [ ] No hot glue on the cell, the antenna end of the XIAO or the button's plunger.
* [ ] Strain relief: glue dots on the wires near the XIAO pads, the board exits and the BAT pads; the JST pair is
  not pulling on the pads.
* [ ] Kapton on the board's copper side and on the BAT pads; no bare copper can touch the cell's pouch or PCM.
* [ ] The XIAO is held (try plugging and unplugging the USB-C cable twice: it must not move).
* [ ] USB-C reachable from underneath, button pressable from outside, the D1 lead is reachable when the base is open.
* [ ] The cell is a 1-cell LiPo, plugged the right way (Check 3a done).
* [ ] The lamp passed Check 4.

## 7. Open questions for the operator

1. What sits on top of the base (shade, diffuser, lid), and does it leave 1.5 mm above the rim for the XIAO and
   ≈ 2.2 mm for the LED? Can it have a recess that also holds the XIAO down?
2. How is the base made (3D-printed, bought), and how thick are its wall and floor? (Window, button hole and switch
   height depend on it; a printed base can include the ledges, the window and the XIAO rails.)
3. Is the button on the north wall, opposite the USB-C, where you want it?
4. Charging from underneath means laying the lamp on its side or on a stand while it charges: acceptable?
5. The measurements of §5.1 step 1, especially the cell's real length with its PCM and the JST pitch.
