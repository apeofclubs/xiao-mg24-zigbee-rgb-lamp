# Wiring

> Read `hardware-assumptions.md` before soldering. LED polarity (H3), the cell's protection circuit (H1), its
> charge-current rating (H2) and the battery connector polarity (H8) must be confirmed first.

## Parts

| Qty | Part | Notes |
|---|---|---|
| 1 | Seeed Studio XIAO MG24 (or MG24 Sense) | |
| 1 | RGB LED, 4 pins, conventional (not WS2812) | Elegoo kit LED. Common cathode assumed, common anode supported |
| 1 | 680 Ω resistor, ¼ W or 0603/0805 | red channel |
| 2 | 330 Ω resistor | green and blue channels |
| 1 | LiPo cell Liter Energy 902025, 3.7 V 400 mAh, red/black leads | **with a protection circuit** (H1) |
| – | thin silicone wire (e.g. 28–30 AWG), heat-shrink | |

No MOSFET, no external divider, no second charger.

## Diagram: common-cathode LED (default, `APP_LED_COMMON_CATHODE = 1`)

```
                    Seeed XIAO MG24 (top view, USB-C at the top)
                   ┌──────────────[ USB-C ]──────────────┐
      R 680 Ω      │ D0  PC00                     5V     │  ← nothing connected
 LED R ──/\/\/\────┤                                     │
                   │ D1  PC01  (escape: GND at boot      │
                   │            = never sleep)   GND ────┼──────────┐
      R 330 Ω      │ D2  PC02                            │          │
 LED G ──/\/\/\────┤                             3V3     │          │
      R 330 Ω      │ D3  PC03                            │          │
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
 LED R ── 680 Ω ── D0     LED G ── 330 Ω ── D2     LED B ── 330 Ω ── D3
```

Connect the common anode to **3V3**, never to `5V` or `BAT+`. With a higher voltage on the anode, an "off" GPIO at
3.3 V would no longer turn the LED off, and current would flow into the pin.

## Resistor calculation

Supply: the XIAO's 3.3 V rail (buck U2, [S2] sheet 03). The GPIO output is close to the rail at a few mA. Ignoring
the pin's voltage drop makes the estimate slightly high, so it errs on the safe side.

`I = (3.3 V − Vf) / R`

| Channel | Vf assumed at 1–2 mA | R | I (estimate) | I if the LED were shorted (worst case for the pin) |
|---|---|---|---|---|
| Red | 1.8–2.0 V | 680 Ω | 1.9–2.2 mA | 4.9 mA |
| Green | 2.6–2.9 V | 330 Ω | 1.2–2.1 mA | 10 mA |
| Blue | 2.6–2.9 V | 330 Ω | 1.2–2.1 mA | 10 mA |

* The Vf values are typical for 5 mm RGB LEDs at low current. They are **assumed**. Green and blue LEDs are often
  specified at about 3.0–3.2 V at 20 mA, and their Vf drops at 1–2 mA. Item H3 measures the real LED.
* Total: about 6 mA for full white, a few mA per pin. Every pin stays far below normal microcontroller GPIO limits.
  The EFR32MG24 datasheet figures are item H9.
* **Low current on purpose**: a 1–2 mA 5 mm LED is clearly visible indoors and costs far less battery than 20 mA.
  Colour balance (green usually looks brighter) is corrected in firmware with per-channel duty limits in Phase 2, not
  by changing resistors.
* **Green and blue have little headroom**: 3.3 V − Vf is only a few tenths of a volt. When the battery falls below
  about 3.4 V, the buck can no longer hold 3.3 V, and green/blue dim before red. The low-battery policy (Phase 4)
  accounts for this.
* If the lamp is too dim after the Phase 2 test, go down in steps (red 470 Ω, green/blue 220 Ω) and measure each
  channel's current with a multimeter in series. Keep each channel under 5 mA.

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
