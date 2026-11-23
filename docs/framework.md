# Framework choice

**Decision: Silicon Labs Simplicity SDK 2026.6.1 (EmberZNet Zigbee stack + ZCL application framework), generated with
`slc` and built with GNU Make + Arm GNU Toolchain 14.2.Rel1.** The project does not use the Arduino core, and the
two APIs are not mixed.

## The two candidates

### A. Silicon Labs Arduino core (`SiliconLabsSoftware/arduino`), release 5.0.0

Checked against the upstream repository (https://github.com/SiliconLabsSoftware/arduino):

* 5.0.0 introduces "the all new Zigbee library" (`libraries/Zigbee`, wrapping EmberZNet) and lists the Seeed XIAO
  MG24 among the supported boards. It is selected with *Tools > Protocol stack > Zigbee*.
* Example paths exist as named in the brief: `libraries/Zigbee/examples/zigbee_lightbulb`, `zigbee_lightbulb_color`,
  `zigbee_lightbulb_dimmable`, `zigbee_power_source`, `zigbee_end_device` (plus switch, sensor, thermostat and other
  examples). There is no `zigbee_sleepy` example.
* APIs (library `readme.md`): `ZigbeeColorLightbulb` ("adds Color Control server support": `set_hue()`,
  `set_saturation()`, `set_rgb()`, `get_rgb()`…), `ZigbeeDimmableLightbulb` (`set_level()`,
  `set_brightness_percent()`), `ZigbeePowerSource` ("a standalone Power Configuration endpoint":
  `set_battery_percent()`, `set_reporting_interval()`), `Zigbee.setDeviceType(ZIGBEE_DEVICE_TYPE_END_DEVICE)`,
  `Zigbee.setPowerSource(ZIGBEE_POWER_SOURCE_TYPE_BATTERY)`.
* **Sleepy end device:** the readme describes an end-device "wake, read, report, sleep" workflow but **names no sleep
  or poll-control API**. The stack and its configuration (poll intervals, sleep policy) come **precompiled** in
  `variants/xiao_mg24/zigbee/` and cannot be changed from a sketch.
* The `zigbee_lightbulb_color` readme snippet uses `LED_BUILTIN_1`/`LED_BUILTIN_2`, which the XIAO MG24 variant does
  not define, so it targets another board.
* PWM is `analogWrite()`. Its timer allocation lives in `cores/gecko`, not in the variant, and nothing ties it to the
  power manager's EM1 requirement.
* Build: needs `arduino-cli` + the core (about 1 GB of downloads) outside this host's installed tools.

### B. Simplicity SDK + ZCL framework (`slc`)

* Installed on this host and pinned: Simplicity SDK **2026.6.1**, slc-cli **6.0.23**, Arm GNU Toolchain
  **14.2.Rel1** (see `CLAUDE.md`).
* Sleepy end device is a first-class configuration:
  `SLI_ZIGBEE_PRIMARY_NETWORK_DEVICE_TYPE = SLI_ZIGBEE_NETWORK_DEVICE_TYPE_SLEEPY_END_DEVICE`, `zigbee_pro_leaf_stack`,
  `zigbee_end_device_support` (short/long poll intervals, wake timeout), with the stack's EM2 sleep through the Power
  Manager. The Silicon Labs sample `zigbee_z3_switch` is a sleepy device built this way.
* Standard ZCL servers as components: `zigbee_on_off`, `zigbee_level_control`, `zigbee_color_control_server`,
  `zigbee_reporting`, `zigbee_basic`, `zigbee_identify`, `zigbee_poll_control_server`. The Power Configuration
  cluster (0x0001) is declared in the ZAP file with its attributes, and the application writes them. The Silicon Labs
  sample `zigbee_z3_light` provides an Extended Color Light endpoint (device 0x010D) to start from.
* Full control of the pieces that matter for this lamp: the EM1 requirement while lit
  (`sl_power_manager_add_em_requirement()`), poll intervals per profile, TIMER0 PWM, the IADC, and NVM3.
* Command-line build verified on this host: `scripts/build.sh` generates and builds in about 40 s.

## Trade-offs

| Need (brief) | Arduino 5.0.0 | Simplicity SDK |
|---|---|---|
| Sleepy end device with tunable polling | not exposed | yes (`zigbee_end_device_support`) |
| EM1 while lit, EM2 while off | not controllable from a sketch | yes (Power Manager requirements) |
| On/Off, Level, Color Control | yes (`ZigbeeColorLightbulb`) | yes (ZCL server components) |
| Power Configuration (voltage, %) | `ZigbeePowerSource` (percent API) | yes (ZAP attributes, written by the app) |
| Reproducible command-line build here | needs `arduino-cli` + core download | yes, tools pinned and installed |
| Code size / complexity | small sketch | more files (slcp, ZAP, config) |
| Board support | XIAO MG24 variant (pins, crystal tuning) | none: part-level project, every pin set by hand |

## Why B

The two requirements that decide the design, a sleepy end device with controlled polling and EM1-only-while-lit, are
exactly what the Arduino library hides. The Arduino core is useful for one thing here: its XIAO MG24 variant is the
reference for board-specific settings. This project takes from it (and cites in `THIRD_PARTY_NOTICES.md`) the
**crystal tuning** (HFXO CTUNE 95, LFXO enabled with CTUNE 44), the USART pins (unused since Phase 2: the console is on RTT), and the **application start address
0x08006000** above the bootloader.

**No mixing.** No Arduino headers, libraries or `analogWrite()` are used. The firmware calls only Simplicity SDK APIs
(`sl_*`, emlib `GPIO_*`/`CMU_*`, `sl_zigbee_af_*`).
