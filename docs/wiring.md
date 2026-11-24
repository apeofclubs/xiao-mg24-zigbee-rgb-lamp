# Wiring

> Final soldered build in the base (perfboard layout, assembly order, checks): `assembly.md`.
>
> Read `hardware-assumptions.md` before soldering. LED polarity (H3), the cell's protection circuit (H1), its
> charge-current rating (H2) and the battery connector polarity (H8) must be confirmed first.

## Parts

| Qty | Part | Notes |
|---|---|---|
| 1 | Seeed Studio XIAO MG24 (or MG24 Sense) | |
| 1 | RGB LED, 4 pins, conventional (not WS2812) | Elegoo kit LED. **Common cathode** (H3 confirmed on the bench, 2026-11-22); common anode supported |
| 2 | 100 Ω resistor, ¼ W or 0603/0805 | red and blue channels (as wired and measured on the bench, 2026-11-23); **never below 100 Ω on red** |
| 1 | 220 Ω resistor | green channel |
| 1 | LiPo cell Liter Energy 902025, 3.7 V 400 mAh, red/black leads, **1 cell** (3.7 V nominal; 3.98 V measured 2026-11-24) | **with a protection circuit** (H1), plugged through a JST connector |
| 1 | 2-pin JST pigtail matching the cell's plug (most likely JST-PH, 2.0 mm pitch: measure, table below) | soldered on BAT+ / BAT− |
| 1 | Push button, 4-pin tactile switch (e.g. 6 × 6 mm) | on/off and pairing, on D9; two **diagonal** pins used |
| – | thin silicone wire (e.g. 28–30 AWG), heat-shrink | |

No MOSFET, no external divider, no second charger, no resistor or capacitor on the button (internal pull-up and
filter).

## Diagram: common-cathode LED (default, `APP_LED_COMMON_CATHODE = 1`)

```
                    Seeed XIAO MG24 (top view, USB-C at the top)
                   ┌──────────────[ USB-C ]──────────────┐
      R 100 Ω      │ D0  PC00                     5V     │  ← nothing connected
 LED R ──/\/\/\────┤                                     │
                   │ D1  PC01  (escape: GND at boot      │
                   │            = never sleep)   GND ────┼──────────┬───────────┐
      R 220 Ω      │ D2  PC02                            │          │           │
 LED G ──/\/\/\────┤                             3V3     │          │           │
      R 100 Ω      │ D3  PC03                            │          │      ┌────┴────┐
 LED B ──/\/\/\────┤                                     │          │      │ button  │ 4-pin tactile,
                   │ D4 … D8   free             D9 PA04 ─┼──────────┼──────┤ (2 pins │ two diagonal
                   │                            D10 free │          │      │diagonal)│ pins used
                   │                                     │          │      └─────────┘
                   │      underside:  BAT+   BAT-        │          │
                   └────────────────────┬──────┬─────────┘          │
                                        │      │  pigtail           │
                                  red ──┘      └── black            │
                                     ┌──┴──────┴──┐                 │
                                     │ JST socket │                 │
                                     └──┬──────┬──┘                 │
                                     ┌──┴──────┴──┐  check polarity │
                                     │  JST plug  │  before plug-in │
                                     └──┬──────┬──┘                 │
                         ┌──────────────┴──────┴─────┐              │
                         │ LiPo 902025 400 mAh + PCM │  1 cell only │
                         └───────────────────────────┘              │
 LED common (cathode, longest leg) ─────────────────────────────────┘
```

The LED cathode and the button share the XIAO's **GND pad** (both wires on that pad, or joined first and then one
wire to the pad). **BAT− is never used as a ground** for the LED or the button.

## Diagram: common-anode LED (set `APP_LED_COMMON_CATHODE = 0` in `firmware/config/app_config.h`)

The same three resistors and pins. The common pin goes to **3V3** instead of GND. A channel then lights when its pin is
driven LOW, and the firmware inverts the PWM.

```
 LED common (anode, longest leg) ──────── XIAO 3V3
 LED R ── 100 Ω ── D0     LED G ── 220 Ω ── D2     LED B ── 100 Ω ── D3
```

Connect the common anode to **3V3**, never to `5V` or `BAT+`. With a higher voltage on the anode, an "off" GPIO at
3.3 V would no longer turn the LED off, and current would flow into the pin.

## Resistor calculation

Supply: the XIAO's 3.3 V rail (buck U2, [S2] sheet 03). The GPIO output is close to the rail at a few mA. Ignoring
the pin's voltage drop makes the estimate slightly high, so it errs on the safe side.

`I = (3.3 V − Vf) / R`

### Measured on the bench (operator, 2026-11-22/23)

Phase 2 firmware, `lamp balance 100 100 100`, one channel alone at 100 % duty, voltage measured across the resistor
(the current is that voltage divided by R; the LED's Vf is 3.3 V minus it):

| Channel | R | V across R | I | Vf of the LED at that current |
|---|---|---|---|---|
| Red | 100 Ω | 1.00 V | **10 mA** | 2.30 V |
| Green | 220 Ω | 0.60 V | **2.7 mA** | 2.70 V |
| Blue | 100 Ω | 0.33 V | **3.3 mA** | 2.97 V |

* Full white (all three at 100 %, balance 100/100/100): about 16 mA in total, 10 mA of it on D0.
* **Blue limits the white**: its Vf (2.97 V) leaves only 0.33 V across the resistor, so even 100 Ω gives 3.3 mA.
  A lower blue resistor buys little (and nothing once the rail sags on battery); the white point is set with
  `lamp balance` (lower red and green), not with resistors.
* **Design margin: 10 mA per pin.** Red is at that margin with 100 Ω: **never go below 100 Ω on red**. If the red LED
  shorted, 100 Ω would let 33 mA through the pin: a fault case, not a working point.
* Every pin stays at or under 10 mA, far below normal microcontroller GPIO limits; the EFR32MG24 datasheet figures
  are item H9.

### Earlier estimates (first plan, before the measurement)

| Channel | Vf assumed at 1–2 mA | R | I (estimate) |
|---|---|---|---|
| Red | 1.8–2.0 V | 220 Ω | 5.9–6.8 mA |
| Green | 2.6–2.9 V | 330 Ω | 1.2–2.1 mA |
| Blue | 2.6–2.9 V | 330 Ω | 1.2–2.1 mA |

The measured Vf are higher than assumed (red 2.30 V, blue 2.97 V), which is why the operator went down to
100 Ω / 220 Ω / 100 Ω to get usable brightness.

* **Low current on purpose**: a few mA through a 5 mm LED is clearly visible indoors and costs far less battery than
  20 mA. Colour balance is corrected in firmware with per-channel balance coefficients (`lamp balance`,
  `docs/bench-test.md`), not by changing resistors again.
* **Green and blue have little headroom**: 3.3 V − Vf is only 0.3–0.6 V. When the battery falls below about 3.4 V,
  the buck can no longer hold 3.3 V, and blue then green dim first, then go out. The low-battery policy (Phase 4)
  accounts for this.

## On/off and pairing button (D9)

A standard 4-pin tactile switch has its pins connected in two pairs inside: the two pins of a pair are always
connected, the button connects one pair to the other. Using two **diagonally opposite** pins always goes through the
switch, whichever way the part is turned, so no need to know which pins are paired.

1. One diagonal pin → **D9** (PA04). The other diagonal pin → **GND** (the same GND pad as the LED cathode, or a
   junction with it). The two other pins stay unconnected.
2. Nothing else: no resistor, no capacitor. The firmware turns on D9's internal pull-up and input filter.
3. Never wire the button to 3V3, 5V or BAT+: pressed, it would short that supply to D9 or to GND.
4. Check before power: multimeter in continuity mode between D9 and GND: **open** at rest, **closed** while pressed.
   If it beeps at rest, the two pins are on the same side of a pair, not diagonal: move one wire.

What it does (`zigbee.md` §8): **short press** = light on/off (as a Zigbee Toggle command). **Hold 5 s** = pairing
(leaving the current network first if joined): the lamp breathes blue while it searches, then shows green (joined) or
amber (failed). No factory reset on the button (`zigbee reset` on the console). D1 stays the recovery escape; D9 is
not an escape.

## Battery connection

The cell is plugged permanently through a **2-pin JST connector**: a short pigtail (socket + two wires) is soldered on
the XIAO's BAT+ / BAT− pads, and the cell's own plug goes into it. Use **only a 1-cell LiPo** (3.7 V nominal, 4.2 V
full): the XIAO's charger (SGM40567-4.2) is a single-cell charger. **Never a 2-cell pack** (7.4 V; it often has a
second, 3-pin balance plug): it would destroy the charger and the board.

### Which JST: measure the pitch

Pitch = distance between the centres of the two pins (or of the two holes in the socket). Calipers, or a ruler
against both connectors side by side. The pigtail must be the same series as the cell's plug: different series do not
mate properly and can make an intermittent or shorted contact.

| Series | Pitch | Notes |
|---|---|---|
| **JST-PH** | **2.0 mm** | most common on small LiPo cells (Adafruit, many sellers): the most likely one here |
| Molex PicoBlade ("JST 1.25") | 1.25 mm | common on very small cells and drones |
| JST-ZH | 1.5 mm | |
| JST-SH | 1.0 mm | |
| JST-XH | 2.5 mm | usually balance plugs of multi-cell packs: if the cell has this, check it really is 1 cell |

The lamp draws a few tens of mA and the charger 200 mA: any of these connectors carries that easily.

### Polarity is not standard: check it before the first plug-in

Sellers wire the same JST plug both ways (red on the left or on the right). A reversed cell on BAT+ / BAT− destroys
the charger and can damage the cell. Never trust the colour of the pigtail's wires or the position of the pins: check
with the real cell, in this order.

1. **Cell alone**, multimeter in DC volts: probes on the two pins of the cell's plug. Note which pin gives a
   **positive** reading with the red probe on it (about +3.7…4.2 V; 3.98 V measured): that is the cell's +.
2. **Before soldering the pigtail**: plug the cell into the pigtail (pigtail not connected to anything else), measure
   between the two free ends of the pigtail. The end that reads **positive** under the red probe goes to **BAT+**,
   the other to **BAT−**, whatever their colour. If the pigtail's red wire reads negative, the cell and the pigtail
   are wired opposite: mark the wires (or swap the pins in the housing, with the cell unplugged) so that the cell's
   red wire reaches BAT+. Unplug the cell.
3. The XIAO's battery pads are on the **underside**. Seeed: "The negative terminal of the power supply should be the
   side closest to the USB port, and the positive terminal … is the side away from the USB port" [S1]. Check against
   the silkscreen of your board. Solder the pigtail with the XIAO **unplugged from USB** and the cell **unplugged**,
   one wire at a time, heat-shrink on each joint, the two wires never touching.
4. **Continuity check, cell still unplugged**: multimeter in continuity mode between the **BAT+ pad** and the socket
   contact that mated with the cell's + pin in step 2: it must beep. Same for BAT− and the other contact. No beep
   between the two contacts (no short).
5. First plug-in, USB unplugged: measure on the pads, red probe on BAT+, black on BAT−: **positive**, the cell's
   voltage. If it reads negative, **unplug at once** and recheck steps 2–4.

Then, as before:

* Do not connect the cell anywhere else: not to `5V`, not to `3V3`, not to the LED, not to the button. BAT− is not
  used as a ground.
* Strain-relieve the pigtail (a dot of hot glue on the wires next to the pads, not on the cell) so that plugging and
  unplugging does not pull on the pads. A LiPo must not be pierced, crushed, bent or heated. Leave a little room
  around it in the enclosure; a cell can swell with age.
* Unplug the cell by pulling on the plug's housing, not on the wires.

## USB-C charging from underneath

The XIAO's own USB-C connector does everything: charging (SGM40567, ≈ 200 mA), power and flashing. Keep a single
charging path.

**Option 1 (preferred): direct access to the XIAO's connector.**
Mount the XIAO so its USB-C receptacle faces a hole in the base of the lamp. The board is about 21 × 17.8 mm [S1].
Standing upright with the connector pointing down, it is 21 mm tall: 1.5 mm above the rim of the measured base
(inner Ø 36.5 × 19.5 mm, item H11). That is the placement chosen for the final build, with the plug inserted from
underneath through a window in the floor: see `assembly.md` (comparison with tilted and flat, clearances, perfboard
layout and assembly procedure).

**Option 2: a short internal USB-C extension.**
Use a **complete** USB-C extension (male plug into the XIAO, female panel-mount socket in the base), with all wires
including CC and the data lines. That keeps USB flashing working. It adds no circuit: the XIAO still does the
charging.

**Do not** fit a bare USB-C receptacle wired to the battery or to the BAT pads. That bypasses the charger: no
current limit, no 4.2 V termination, and a fire risk. Do not add a second charger module either.

Leave the cable's plug space clear inside the base: a USB-C plug overmould is about 12 × 7 mm and 20 mm long.

## Battery safety summary

* Use a cell with a protection circuit (PCM), item H1. The firmware's low-battery shutdown is not a substitute.
* Charge only through the XIAO's USB-C, at ≈ 200 mA, which must be within the cell's rating (H2).
* Charge on a non-flammable surface the first few times, and feel the cell: it must stay cool.
* Stop using a cell that swells, smells or gets hot.
