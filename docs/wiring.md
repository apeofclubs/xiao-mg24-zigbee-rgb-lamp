# Wiring

> Read `hardware-assumptions.md` before soldering. LED polarity (H3), the cell's protection circuit (H1), its
> charge-current rating (H2) and the battery connector polarity (H8) must be confirmed first.

## Parts

| Qty | Part | Notes |
|---|---|---|
| 1 | Seeed Studio XIAO MG24 (or MG24 Sense) | |
| 1 | RGB LED, 4 pins, conventional (not WS2812) | Elegoo kit LED. **Common cathode** (H3 confirmed on the bench, 2026-11-22); common anode supported |
| 2 | 100 Ω resistor, ¼ W or 0603/0805 | red and blue channels (as wired and measured on the bench, 2026-11-23); **never below 100 Ω on red** |
| 1 | 220 Ω resistor | green channel |
| 1 | LiPo cell Liter Energy 902025, 3.7 V 400 mAh, red/black leads | **with a protection circuit** (H1) |
| – | thin silicone wire (e.g. 28–30 AWG), heat-shrink | |

No MOSFET, no external divider, no second charger.

## Diagram: common-cathode LED (default, `APP_LED_COMMON_CATHODE = 1`)

```
                    Seeed XIAO MG24 (top view, USB-C at the top)
                   ┌──────────────[ USB-C ]──────────────┐
      R 100 Ω      │ D0  PC00                     5V     │  ← nothing connected
 LED R ──/\/\/\────┤                                     │
                   │ D1  PC01  (escape: GND at boot      │
                   │            = never sleep)   GND ────┼──────────┐
      R 220 Ω      │ D2  PC02                            │          │
 LED G ──/\/\/\────┤                             3V3     │          │
      R 100 Ω      │ D3  PC03                            │          │
 LED B ──/\/\/\────┤                                     │          │
                   │ D4 … D10   free                     │          │
                   │                                     │          │
                   │      underside:  BAT+   BAT-        │          │
                   └────────────────────┬──────┬─────────┘          │
                                        │      │                    │
                           red ─────────┘      └──────── black      │
                         ┌───────────────────────────┐              │
                         │ LiPo 902025 400 mAh + PCM │              │
                         └───────────────────────────┘              │
 LED common (cathode, longest leg) ─────────────────────────────────┘
```

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

## Battery connection

1. **Check the cell first** (H1, H2, H8): protection circuit present, charge current ≥ 200 mA allowed, connector
   polarity. Never trust wire colours alone. Measure with a multimeter: red lead positive, about 3.7–4.2 V.
2. The XIAO MG24's battery pads are on the **underside**. Seeed: "The negative terminal of the power supply should be
   the side closest to the USB port, and the positive terminal … is the side away from the USB port" [S1]. Check
   against the silkscreen of your board before soldering.
3. Solder the leads with the XIAO **unplugged from USB**. Insulate each joint with heat-shrink. Never let the two
   leads touch. Cut and solder one lead at a time.
4. Do not connect the cell anywhere else: not to `5V`, not to `3V3`, and not to the LED.
5. Strain-relieve the leads (a dot of hot glue on the wires, not on the cell). A LiPo must not be pierced, crushed,
   bent or heated. Leave a little room around it in the enclosure; a cell can swell with age.

## USB-C charging from underneath

The XIAO's own USB-C connector does everything: charging (SGM40567, ≈ 200 mA), power and flashing. Keep a single
charging path.

**Option 1 (preferred): direct access to the XIAO's connector.**
Mount the XIAO so its USB-C receptacle faces a hole in the base of the lamp. The board is about 21 × 17.8 mm [S1].
Standing upright with the connector pointing down, it needs about 21 mm of height plus the plug's clearance under the
base. That is at the limit of the 20 mm available (item H11). Alternative: lay the board flat on the base near the
edge, with the connector facing outwards through a slot in the side wall, close to the bottom.

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
