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
| USART to the SAMD11 (USB serial) | (internal, D12 / D11 pads) | PA08 / PA09 | unused from Phase 2 | The console moved to SEGGER RTT over SWD (`flashing.md` §4.1); the pins are left in their reset state |

All three PWM channels come from **one timer** (TIMER0 has 3 compare channels [S5]), so they share one frequency
and stay in phase. On Series 2, TIMER outputs are routed to pins through the GPIO routing registers. **TIMER0 can
reach port C on this part**: the SDK pin-tool data [S5] (`EFR32MG24B220F1536IM48/PORTIO.portio`) lists locations
32–41, i.e. PC00–PC09, for each of TIMER0 CC0, CC1 and CC2 (checked for Phase 2, 2026-11-23).

The firmware sets D0/D2/D3 to push-pull at the "off" level for the configured LED polarity at boot; from Phase 2,
TIMER0 drives a pin only while its channel is above 0. It reads D1 once at boot and then disables it. It does not
touch PD03/PD04 (see `hardware-assumptions.md`, item H4).

### Check against the pins Seeed reserves (Seeed wiki, page updated 2024-10-17)

The operator's reference lists: PD4 = `ADC_BAT`, PD3 = battery-divider enable, PA6 = external-flash CS, PB5 = RF
switch, PB1 = IMU power, PB0 = mic power, PC1 = Seeed's recovery pin.

* **The LED pins PC00 / PC02 / PC03 are on none of them.** Phase 2 drives only these three pins (and reads PC01 once
  at boot, as an input with pull-up, which is how Seeed's own escape uses it).
* PD3/PD4, PA6, PB0/PB1, PB5 are not touched by the Phase 2 firmware. The external flash is never sent any command,
  so never Deep Power Down (`0xB9`).
* **Discrepancy to resolve before anyone uses PB0/PB1:** the wiki's list gives PB1/PB0 as IMU/mic power, while the
  schematic v1.1 [S2] and the Arduino variant [S3] (table below) give PB00/PB01 as SPI to the external flash and
  PD05/PC08 as IMU power/mic enable on the Sense. No phase of this project uses these pins, so it has no effect now.

## Full pad table

From [S1] (pin map), cross-checked against the schematic net names [S2] and the Arduino variant [S3].

| Pad | EFR32 | Schematic net | On-board connection | Use in this project |
|---|---|---|---|---|
| D0 | PC00 | `PC00/A0/D0` | none | **LED red** |
| D1 | PC01 | `PC01/A1/D1` | none (Arduino escape pin) | **Escape input** |
| D2 | PC02 | `PC02/A2/D2` | none | **LED green** |
| D3 | PC03 | `PC03/A3/D3` | none | **LED blue** |
| D4 | PC04 | `PC04/A4/D4/SDA` | none (I2C SDA in Arduino) | free |
| D5 | PC05 | `PC05/A5/D5/SCL` | none (I2C SCL in Arduino) | free |
| D6 | PC06 | `PC06/A6/D6/TX` | none (Serial1 TX in Arduino) | free |
| D7 | PC07 | `PC07/A7/D7/RX` | none (Serial1 RX in Arduino) | free |
| D8 | PA03 | `PA03/A8/D8/SCLK` | none | free; see note on PA03 |
| D9 | PA04 | `PA04/A9/D9/MISO` | none | free; proposed optional pairing button (port A: can wake the chip from EM2), `zigbee.md` §8 |
| D10 | PA05 | `PA05/A10/D10/MOSI` | none | free |
| 3V3 | – | `+3V3` | output of buck U2 (TPS628438) | LED common anode (if anode variant) |
| GND | – | `GND` | | LED common cathode |
| 5V | – | `VBUS` | USB-C VBUS | **never connect anything** |
| BAT+ / BAT- (underside pads) | – | `VBAT` | charger U1 (SGM40567-4.2) | LiPo cell |

## Pins to avoid

| EFR32 pin | Why | Source |
|---|---|---|
| **PA01 / PA02** | SWCLK / SWDIO to the on-board SAMD11 debugger. Reconfiguring them removes the only way to reflash. **Never touched.** | [S2] sheets 04/05 (nets `SWCLK`, `SWDIO`) |
| PA08 / PA09 | USART to the SAMD11 (USB serial); unused from Phase 2 (console on RTT) | [S1][S2] |
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

Measured on the operator's bench (`wiring.md`, 2026-11-23): red 10 mA (100 Ω), green 2.7 mA (220 Ω), blue 3.3 mA
(100 Ω) at 100 % duty. The design margin is **10 mA per pin**, reached by red: never under 100 Ω on red. The exact
EFR32MG24 figures (maximum current per pin, total I/O current, VOH/VOL at a given load) are in the datasheet [S6],
sections "Absolute Maximum Ratings" and "GPIO Pins". **I could not extract text from that PDF on the build host**, so
the figures are not quoted here. Checking them against the datasheet tables is open item H9 in `hardware-assumptions.md`,
and the 10 mA margin is to be checked against them.

## Energy-mode notes that constrain the pins

* TIMER0..4 are high-frequency peripherals: they stop in EM2. Only LETIMER0 keeps counting in EM2, and it has two
  outputs (OUT0, OUT1) [S5]. Three EM2 PWM channels are therefore impossible. See `feasibility.md` §2.
* The reference manual [S7] (GPIO chapter) restricts peripheral pin functions in EM2/EM3 to ports A and B. Static GPIO
  output levels are kept on every port. The LED pins (port C) therefore keep their "off" level while the chip sleeps,
  but they cannot carry a peripheral signal in EM2. This port A/B rule comes from the reference manual chapter, and
  the exact wording should be re-checked (item H9).
