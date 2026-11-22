# Pin assignment: Seeed Studio XIAO MG24

The firmware takes every pin below from `firmware/config/app_config.h` (and the console pins from
`firmware/config/sl_iostream_usart_vcom_config.h`). Nothing else in the code names a pin.

## Sources

| Ref | Document | URL |
|---|---|---|
| [S1] | Seeed wiki, *Getting Started with Seeed Studio XIAO MG24*: pin map table, battery sketch, deep-sleep warning | https://wiki.seeedstudio.com/xiao_mg24_getting_started/ |
| [S2] | Seeed XIAO MG24 schematic, KiCad project v1.1 (sheets 03 Power, 04 SAMD11, 05 EFR32MG24B220, 06 Peripheral) | https://files.seeedstudio.com/wiki/XIAO_MG24/Getting_Start/XIAO_MGM240S_KICAD_Prj.pdf |
| [S3] | Silicon Labs Arduino core 5.0.0, XIAO MG24 variant pin table | https://github.com/SiliconLabsSoftware/arduino/blob/main/variants/xiao_mg24/arduino_variant.cpp |
| [S4] | Silicon Labs Arduino core 5.0.0, XIAO MG24 Zigbee variant configuration (crystal tuning, USART pins) | https://github.com/SiliconLabsSoftware/arduino/tree/main/variants/xiao_mg24/zigbee/config |
| [S5] | Simplicity SDK 2026.6.1, pin-tool data for `EFR32MG24B220F1536IM48` (TIMER0..4 with 3 channels each, LETIMER0 with OUT0/OUT1 only) | `platform_core/platform/hwconf_data/pin_tool/efr32mg24/` in the SDK |
| [S6] | Silicon Labs EFR32MG24 datasheet (copy hosted by Seeed) | https://files.seeedstudio.com/wiki/XIAO_MG24/Getting_Start/mg24-group-datasheet.PDF |
| [S7] | Silicon Labs EFR32xG24 reference manual (copy hosted by Seeed) | https://files.seeedstudio.com/wiki/XIAO_MG24/Getting_Start/efr32xg24_rm.pdf |

The MCU on the schematic is `EFR32MG24B220F1536IM48-B` (U5, sheet 05 [S2]). That is the part the firmware targets.

## Chosen pins

| Function | XIAO pad | EFR32 pin | Peripheral (from Phase 2) | Why this pin |
|---|---|---|---|---|
| LED red | **D0** | PC00 | TIMER0 CC0 (PWM) | Edge pad, plain GPIO, no on-board load [S2] |
| LED green | **D2** | PC02 | TIMER0 CC1 (PWM) | Same |
| LED blue | **D3** | PC03 | TIMER0 CC2 (PWM) | Same |
| Recovery escape (input, pull-up) | **D1** | PC01 | GPIO | Same pad as the Arduino core's `DEEP_SLEEP_ESCAPE_PIN` [S3], so one habit works for both firmwares |
| Battery divider enable | (internal) | PD03 `VBAT_CTL` | GPIO out | Fixed by the board [S2] |
| Battery voltage | (internal) | PD04 `VBAT_ADC` | IADC | Fixed by the board [S1][S2] |
| Console TX (to SAMD11 → USB) | (internal, D12 pad) | PA08 | USART0 TX | Fixed by the board [S2][S4] |
| Console RX (from SAMD11) | (internal, D11 pad) | PA09 | USART0 RX | Fixed by the board [S2][S4] |

All three PWM channels come from **one timer** (TIMER0 has 3 compare channels [S5]), so they share one frequency
and stay in phase. On Series 2, TIMER outputs are routed to pins through the GPIO routing registers. Whether TIMER0
can reach port C on this part is expected but **not yet checked against the datasheet's alternate-function table**
(item H9). Phase 2's generation with the SDK pin tool will confirm it. If not, the fallback is TIMER1 (also 3
channels [S5]) or moving the LED to D8–D10 (port A).

Phase 1 sets D0/D2/D3 to push-pull at the "off" level for the configured LED polarity. It reads D1 once at boot
and then disables it. It does not touch PD03/PD04 (see `hardware-assumptions.md`, item H4).

## Full pad table

From [S1] (pin map), cross-checked against the schematic net names [S2] and the Arduino variant [S3].

| Pad | EFR32 | Schematic net | On-board connection | Use in this project |
|---|---|---|---|---|
| D0 | PC00 | `PC00/A0/D0` | none | **LED red** |
| D1 | PC01 | `PC01/A1/D1` | none (Arduino escape pin) | **Escape input** |
| D2 | PC02 | `PC02/A2/D2` | none | **LED green** |
| D3 | PC03 | `PC03/A3/D3` | none | **LED blue** |
| D4 | PC04 | `PC04/A4/D4/SDA` | none (I2C SDA in Arduino) | free (future button) |
| D5 | PC05 | `PC05/A5/D5/SCL` | none (I2C SCL in Arduino) | free |
| D6 | PC06 | `PC06/A6/D6/TX` | none (Serial1 TX in Arduino) | free |
| D7 | PC07 | `PC07/A7/D7/RX` | none (Serial1 RX in Arduino) | free |
| D8 | PA03 | `PA03/A8/D8/SCLK` | none | free; see note on PA03 |
| D9 | PA04 | `PA04/A9/D9/MISO` | none | free |
| D10 | PA05 | `PA05/A10/D10/MOSI` | none | free |
| 3V3 | – | `+3V3` | output of buck U2 (TPS628438) | LED common anode (if anode variant) |
| GND | – | `GND` | | LED common cathode |
| 5V | – | `VBUS` | USB-C VBUS | **never connect anything** |
| BAT+ / BAT- (underside pads) | – | `VBAT` | charger U1 (SGM40567-4.2) | LiPo cell |

## Pins to avoid

| EFR32 pin | Why | Source |
|---|---|---|
| **PA01 / PA02** | SWCLK / SWDIO to the on-board SAMD11 debugger. Reconfiguring them removes the only way to reflash. **Never touched.** | [S2] sheets 04/05 (nets `SWCLK`, `SWDIO`) |
| PA08 / PA09 | USART to the SAMD11 (USB serial); used for the console | [S1][S2] |
| RESET | `~MG24_RESET`, driven by the SAMD11 | [S2] |
| PA06 | `FLASH_CS` of the 4 MB SPI flash U8 (P25Q32SH). Never send it Deep Power Down (`0xB9`) | [S2] sheet 06; [S1] warning |
| PA00, PB00, PB01 | SPI1 SCK/MOSI/MISO, shared with the external flash; bottom pads D17/D15/D16 | [S2][S3] |
| PA07 | `USER_RED_LED` (on-board LED, active LOW per [S3]) | [S2][S3] |
| PB04 / PB05 | RF switch U9 (FM8625H): port select / power. Antenna path, see `feasibility.md` §6 | [S1][S2] |
| PD03 / PD04 | battery divider control / ADC, reserved for Phase 4 | [S1][S2] |
| PD05, PC08, PC09 | IMU power, mic enable, mic ADC (XIAO MG24 **Sense** only) | [S2][S3] |
| PB02, PB03 | bottom pads D13/D14, I2C of the IMU on the Sense | [S2][S3] |
| PD02 | bottom pad D18 | [S1] |
| PA03 | edge pad D8. It is also the EFR32 SWV/trace output when trace is enabled (not enabled here); keep it for later use rather than the LED | datasheet [S6], alternate functions |

## GPIO drive limits

The design keeps every pin at **a few mA** (see `wiring.md`: about 1.5 mA red, about 2 mA green/blue, worst case
under 10 mA even with a shorted LED), far below what microcontroller GPIOs are rated for. The exact EFR32MG24 figures
(maximum current per pin, total I/O current, VOH/VOL at a given load) are in the datasheet [S6], sections
"Absolute Maximum Ratings" and "GPIO Pins". **I could not extract text from that PDF on the build host**, so the
figures are not quoted here. Checking them against the datasheet tables is open item H9 in `hardware-assumptions.md`,
and the resistor values stay valid whatever those figures are.

## Energy-mode notes that constrain the pins

* TIMER0..4 are high-frequency peripherals: they stop in EM2. Only LETIMER0 keeps counting in EM2, and it has two
  outputs (OUT0, OUT1) [S5]. Three EM2 PWM channels are therefore impossible. See `feasibility.md` §2.
* The reference manual [S7] (GPIO chapter) restricts peripheral pin functions in EM2/EM3 to ports A and B. Static GPIO
  output levels are kept on every port. The LED pins (port C) therefore keep their "off" level while the chip sleeps,
  but they cannot carry a peripheral signal in EM2. This port A/B rule comes from the reference manual chapter, and
  the exact wording should be re-checked (item H9).
