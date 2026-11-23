# Technical feasibility

**Verdict: feasible on the Seeed XIAO MG24 with no extra active parts.** The lamp is a Zigbee 3.0 *sleepy end device*
(SED) with three resistors and one RGB LED. There are two real limits. The LED cannot be PWM-dimmed while the chip is
in EM2, so the chip stays in EM1 while the LED is lit. And a sleepy device only hears commands when it polls, so
command latency equals the poll interval.

Source references ([S1]…[S7]) are listed in `pinout.md`.

> **Measured vs. estimated.** Nothing in this document was measured. There is no hardware in this project yet.
> Every number marked *estimate* is a back-of-the-envelope figure, to be replaced by the measurements of Phase 5.
> Numbers quoted from Seeed or Silicon Labs are their figures, not ours.

## 1. Board facts this assessment relies on

| Fact | Source |
|---|---|
| MCU `EFR32MG24B220F1536IM48`, 1536 kB flash, 256 kB RAM, 39 MHz HFXO, 32.768 kHz LFXO | [S1], [S2] sheet 05 |
| On-board debugger: ATSAMD11D14A wired to SWCLK/SWDIO/RESET of the EFR32; it also bridges USART0 (PA08/PA09) to USB | [S2] sheet 04 |
| No BOOT button and no BOOT recovery method | [S1] |
| Charger U1 `SGM40567-4.2`, charge current set by R9 = 120 kΩ: the schematic reads "ICharge = 24000/120K = 200mA" (and "Imax = 1A") | [S2] sheet 03 |
| 3.3 V rail from buck U2 `TPS628438`. A separate LDO U10 `SGM2040-3.3` (250 mA) feeds the SAMD11 from USB | [S2] sheet 03 |
| Battery measurement: net `PD03/VBAT_CTL` switches a divider read on `PD04/VBAT_ADC`; Seeed's sketch drives PD3 HIGH and computes `raw * (2 * 3.3 / 4095)` | [S1], [S2] sheet 03 |
| RF switch U9 `FM8625H` between the chip antenna and the U.FL connector, controlled by PB04 (select: "0: RF1, 1: RF2") and PB05 (power) | [S1], [S2] sheet 05 |
| Known brick: EM4 deep sleep "fails to wake up properly, preventing new sketches from being uploaded"; flash Deep Power Down to be avoided | [S1] |

## 2. PWM while sleeping

* The 3-channel PWM needs a TIMER (TIMER0 has three compare channels [S5]). TIMERs are clocked from the
  high-frequency clock tree, which stops in EM2. **No TIMER output in EM2.**
* LETIMER0 keeps running in EM2 from the 32 kHz LFXO, but it has **only two outputs** (OUT0, OUT1 [S5]). In EM2,
  peripheral pin functions are limited to ports A and B [S7]. It cannot drive three channels.
* Software PWM from a sleep timer would wake the CPU hundreds of times per second, with visible flicker at any useful
  low-power rate. Rejected.

**Decision: while any LED channel is lit, the power manager holds an EM1 requirement.** The CPU still sleeps between
events (EM1 = core clock stopped, peripherals running) and TIMER0 keeps the PWM. When the LED is off, the requirement
is released and the chip goes down to EM2 between polls. In EM2 the LED pins hold their static "off" level.

*Measured* cost of the light (operator's bench, 2026-11-22/23, `wiring.md` "Measured on the bench"): with the
resistors now wired (red 100 Ω, green 220 Ω, blue 100 Ω) one channel alone at 100 % duty draws **red 10 mA, green
2.7 mA, blue 3.3 mA**, so full white at balance 100/100/100 is about **16 mA**. Blue limits the white point, so the
tuned balance lowers red and green and the real white is lower (to be re-measured once the operator has re-tuned
`lamp balance`). Design margin: 10 mA per pin, reached by red: never under 100 Ω on red. Add the EM1 current of the
chip, which is in the low-mA range. The exact EM1 figure is in the datasheet [S6]
"Current consumption" tables (to be quoted, item H9). Seeed's own figure for a running board is "Normal (Typ.)
6.71mA" [S1], with unstated conditions. Either way, a lit lamp lasts **days, not months**, on 400 mAh. Standby is
where the sleepy design matters.

A consequence worth using: while the LED is lit, the chip is already awake in EM1, so polling the parent faster costs
little. Phase 5 can use a short poll interval while lit and a long one while off.

## 3. Sleepy end device vs. command latency

A SED's radio is off between polls. Its parent router (or the coordinator) keeps messages for it and hands them over
when the SED polls. Consequences:

1. **Latency = up to one poll interval** (plus a few ms of radio time). A 1 s poll means a command lands within about
   1 s.
2. **The poll interval must stay well below the parent's buffering time.** Zigbee parents hold an indirect message for
   about 7.7 s (the IEEE 802.15.4 / Zigbee PRO "transaction persistence time"). Commands sent to a SED polling less
   often than that are **lost**. The SDK's default *long* poll of 300 s (`end-device-support-config.h`) is fine for a
   sensor and wrong for a lamp. Item H10 checks this against Zigbee2MQTT behaviour.
3. Zigbee2MQTT sends light commands like to any other device, and the parent buffers them. Group commands
   (broadcasts) are not buffered for sleepy children the same way, so **Z2M groups may not reach a sleepy lamp**.
   Phase 3 tests it.

Proposed profiles, exposed as `APP_POWER_PROFILE` in `config/app_config.h` (Phase 5 implements them):

| Profile | Poll while LED off | Poll while LED on | Expected latency |
|---|---|---|---|
| Responsive | 0.25 s | 0.25 s | ≤ 0.25 s |
| **Balanced (default)** | 1 s | 0.25 s | ≤ 1 s when off |
| Maximum battery | 5 s | 1 s | ≤ 5 s when off |

*Estimate* of standby current, with stated assumptions to be replaced in Phase 5:
`I_avg ≈ I_sleep + Q_poll / T_poll`.

* Assume `I_sleep` = 5 µA. EM2 chip current plus board leakage: buck quiescent current, charger, divider off. The
  datasheet gives the chip part, but the board part is unknown. Seeed quotes "Low Power (Typ.) 1.95μA" [S1] without
  conditions.
* Assume `Q_poll` = 10 µC per poll: wake-up, radio on, data request, acknowledgement, a few ms at several mA. Seeed
  quotes "RX current 4.6mA/TX current 5mA (0dBm)" [S1].

| Profile (LED off) | T_poll | I_avg (estimate) | 400 mAh standby (estimate) |
|---|---|---|---|
| Responsive | 0.25 s | ≈ 45 µA | ≈ 1 year |
| Balanced | 1 s | ≈ 15 µA | ≈ 3 years |
| Maximum battery | 5 s | ≈ 7 µA | ≈ 6 years |

These figures ignore LiPo self-discharge, which dominates over such periods. In practice, **standby will be limited
by the cell, not by the firmware**, once polling is ≥ 1 s. Phase 5 replaces this table with measured values.

## 4. Charger and power path

* The charger is a linear single-cell charger, `SGM40567-4.2` (4.2 V termination). It is set to **≈ 200 mA**
  (R9 = 120 kΩ, the schematic's own note [S2]). That is 0.5 C for a 400 mAh cell. Many small LiPo cells allow 0.5–1 C
  charge, but **the 902025 datasheet has to confirm it** (item H2).
* The power sheet also carries a Schottky diode D1 (`PSBD2FD40V1H`) and a load switch U6 (`TPS22916`). These are the
  usual parts of a "USB powers the load, battery charges" arrangement. The text extraction of the schematic gives the
  parts but **not their connections**, so load sharing while charging is **not confirmed** (item H5). The brief
  rightly warns not to assume ideal power-path behaviour. Until it is confirmed, the low-battery logic (Phase 4) must
  tolerate a battery voltage that jumps up when USB is plugged in. Battery-percentage readings while charging are
  meaningless and are flagged as such.
* The red CHARGE LED is driven from VBUS by hardware [S1]. Firmware has no control over it.
* **No discharge protection is visible on the board.** The charger does not cut the load at low voltage. Over-discharge
  protection must come from the cell's own protection circuit (PCM), item H1. The firmware's critical-battery shutdown
  is a second line of defence, never the first.

## 5. Battery measurement (`ADC_BAT`)

**Available for a later phase (Phase 4), not used in Phase 2.** The Seeed wiki [S1] (page updated 2024-10-17) lists
PD4 as `ADC_BAT` and PD3 as the battery-divider enable (HIGH to measure, LOW in sleep), with the sample
`analogRead(PD4) * 2 * 3.3 / 4095`. The Phase 2 firmware never drives PD3 or PD4.

* PD04 (`VBAT_ADC`) reads the battery through a divider. PD03 (`VBAT_CTL`) switches the divider on: Seeed's sketch
  drives PD3 HIGH in `setup()` [S1]. The ×2 in Seeed's formula implies a 1:2 divider. The resistor values sit on
  sheet 03 (R6 100 kΩ, R17/R18 10 kΩ, transistor Q1 `LP0404N3T5G` are on that sheet), but the extracted text does
  not show which of them form the divider.
* **Conflict to resolve before Phase 4 drives PD3:** the Arduino core labels pin 25 (PD3) "Battery charger enable"
  [S3], while the schematic names the net `VBAT_CTL` [S2]. If PD3 did touch the charger, toggling it could stop
  charging. Phase 1 therefore **never drives PD3** (item H4).
* Using the on-board divider means **no external resistor divider** (brief §2). The firmware enables it only for each
  measurement (a few ms), so its current while off is that of the switch's leakage.
* The IADC reads port D pins through the analog bus. Phase 4 will check the ABUS allocation in the reference manual
  [S7] and calibrate against a multimeter reading (test plan in `roadmap.md`).
* Percentage: a piecewise-linear LiPo discharge table (for example 4.20 V = 100 %, 3.70 V ≈ 50 %, 3.50 V ≈ 10 %,
  3.30 V = 0 %; to be tuned on the real cell), not a linear voltage-to-percent rule. It is reported in ZCL
  `BatteryPercentageRemaining` (0x0021, unit 0.5 %) and `BatteryVoltage` (0x0020, unit 100 mV). Readings under LED
  load or while charging are approximate, and the docs will say so.

## 6. Antenna path (RF switch)

U9 selects the chip antenna or the U.FL connector, controlled by PB04/PB05 [S1][S2]. The Silicon Labs Arduino core,
whose Zigbee examples run on this board, **does not drive PB04/PB05 at all**. Its variant file only lists them [S3].
Phase 1 does the same: both pins stay in their reset state (disabled). Whether the on-board antenna is selected in
that state, or whether PB05 must be HIGH and PB04 LOW, is item H6. Phase 3 checks it with a link-quality comparison.

What the sources say: the Seeed wiki pin map names PB04 "RF Switch Port Select" ("Switch onboard antenna and the UFL
antenna") and PB05 "RF Switch Power" [S1]; the schematic marks the select input "0: RF1, 1: RF2" [S2]. **No source we
have says which of RF1/RF2 is the chip antenna.** So Phase 3 does not guess: the bench command `zigbee rfswitch
reset|0|1` (`zigbee.md` §5) lets the operator compare the three states without reflashing. Default at every boot: the
reset state (both pins disabled), unchanged from Phases 1–2.

Why the command is safe for the radio front end:

* Both pins only drive the switch's logic inputs: PB05 is its supply (a GPIO-powered switch draws well under a mA)
  and PB04 its select input, both at the 3.3 V I/O level the switch is designed for on this board. Neither pin is a
  debug, flash or USB-bridge pin.
* Whichever position is selected, the radio sees an antenna path: the chip antenna, or the U.FL connector. With
  nothing on the U.FL the radio transmits into an open connector at the stack's +3 dBm (2 mW, the network-steering
  default, shown by `zigbee status`). The reflected power is at most those 2 mW, far below the part's own maximum
  output (+10 dBm on the B220); the expected effect is a weak or lost link, not damage. This is a reasoned estimate,
  not a Silicon Labs mismatch rating (not available offline). If you want no doubt at all, fit an antenna on the U.FL
  before trying the state that turns out to select it.
* Nothing is saved: a reset or power cycle returns to the reset state, so a state that loses the link is undone by
  pressing RESET.

## 7. Recoverability (never brick the board)

* Image layout: application linked at `0x08006000`, above the Gecko bootloader area `0x08000000–0x08005FFF`. This is
  the same layout as the Arduino core's XIAO MG24 Zigbee variant (`linkerfile.ld`: `FLASH ORIGIN = 0x8006000`) [S4].
  Flashing it **does not write the bootloader**. NVM3 (Zigbee keys, settings) is at `0x08176000–0x0817DFFF`.
* After every reset the firmware blocks EM2 for **10 s**. With pad **D1 tied to GND at reset**, it never sleeps until
  the next reset. **EM4 is never used.** The debug pins PA01/PA02 are never reconfigured. The external flash is never
  put in Deep Power Down.
* Recovery when the board stops answering is in `flashing.md`.

## 8. What is not feasible, or not without changes

* PWM in EM2 with three channels: not possible (see §2).
* A sub-second response while also polling every 5 s or more: not possible. Pick a profile.
* Reliable reception of Z2M **group** commands by a sleepy lamp: doubtful, to be tested (§3).
* Accurate state of charge during charging or under LED load: not possible with a voltage-only gauge. The value is an
  estimate.
