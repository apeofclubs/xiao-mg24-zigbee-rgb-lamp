# Third-party notices

This repository's own code is under the MIT licence (`LICENSE`). It contains, or was derived from, the following
third-party material.

## Silicon Laboratories Simplicity SDK 2026.6.1

* `firmware/src/main.c`: derived from the SDK sample `zigbee_app/z3/zigbee_z3_switch/main.c`.
  Copyright 2025 Silicon Laboratories Inc., **Zlib** licence. The notice is kept in the file header, and the file is
  marked as modified.
* `firmware/config/zcl/zcl_config.zap`: derived from the ZAP configurations of the SDK samples `zigbee_z3_switch` and
  `zigbee_z3_light` (Silicon Laboratories Inc.). The endpoint was reduced to Basic and Identify, and the device type,
  manufacturer and model were changed.
* `firmware/config/sl_iostream_usart_vcom_config.h`: the SDK's iostream USART configuration template (Silicon
  Laboratories Inc., Zlib; licence header kept in the file), with the XIAO MG24 pins filled in.

The SDK itself is **not** in this repository. It is fetched or installed separately (see `CLAUDE.md`). Its licensing
is in the SDK's `License.txt`: the default licence is the Silicon Labs **Master Software License Agreement (MSLA)**,
with Zlib/other exceptions listed there. **A firmware image built from this repository links Silicon Labs code
(EmberZNet stack libraries, RAIL, platform) under those terms.** Anyone distributing built images must respect them.
The MSLA permits use on Silicon Labs devices. No such image is committed here.

## Silicon Labs Arduino core 5.0.0 (`SiliconLabsSoftware/arduino`)

Board-specific values for the XIAO MG24 were taken from the core's variant files (MIT licence, Copyright 2025 Silicon
Laboratories Inc.): https://github.com/SiliconLabsSoftware/arduino/tree/main/variants/xiao_mg24

* HFXO CTUNE 95, LFXO enabled with CTUNE 44, HFXO/LFXO as default clock sources
  (`zigbee/config/sl_clock_manager_oscillator_config.h`, `sl_clock_manager_tree_config.h`);
* USART0 console on PA08/PA09 (`zigbee/config/sl_iostream_usart_xiao_mg24_config.h`);
* application start address 0x08006000 (`zigbee/linkerfile.ld`);
* the pin roles in `docs/pinout.md` (`arduino_variant.cpp`).

These are configuration values, not code. No Arduino source is copied.

## Seeed Studio

Pin roles, the battery measurement formula and the recovery procedure come from the Seeed wiki and the XIAO MG24
schematic (KiCad project, CC BY-SA 4.0 per its title block). They are cited, not copied. The `docs/` files link to the
sources.
