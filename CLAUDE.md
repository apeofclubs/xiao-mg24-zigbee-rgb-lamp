# CLAUDE.md

Open-source Zigbee 3.0 RGB battery lamp for the **Seeed Studio XIAO MG24** (EFR32MG24B220F1536IM48): one conventional
4-pin RGB LED on three PWM pins, a 400 mAh LiPo charged by the XIAO's own USB-C charger, integrated with Home Assistant
through Zigbee2MQTT. It is a **sleepy end device**. The spec is the operator's brief, quoted in the parent task. The
current state and next steps are in `docs/roadmap.md`.

## Layout

```
firmware/
  xiao_mg24_rgb_lamp.slcp          slc project: components, configuration, sources
  config/app_config.h              ALL pins, polarity, thresholds, profiles (no pin elsewhere)
  config/sl_iostream_usart_vcom_config.h   console pins (pin-tool override)
  config/zcl/zcl_config.zap        ZCL endpoints/clusters/attributes (ZAP)
  src/main.c                       start-up and super-loop
  src/zigbee.c, zigbee_app.h       network, clusters, reporting
  src/rgb_led.[ch]                 PWM, colour conversion, transitions
  src/battery.[ch]                 ADC, voltage -> percentage
  src/power_manager.[ch]           boot window, escape pin, sleep policy, low battery
  src/storage.[ch]                 persistent settings (NVM3)
scripts/build.sh                   generate + build from the command line
docs/                              feasibility, framework, pinout, wiring, assumptions, flashing, roadmap
build/                             generated project and binaries (git-ignored, never committed)
```

## Pinned versions

| Tool | Version | Path on the agents' host |
|---|---|---|
| Simplicity SDK (EmberZNet, ZCL framework) | **2026.6.1** (`sdk: simplicity_sdk 2026.6.1` in the `.slcp`) | `/opt/silabs/installs/conan/p/simpleca33d691c539/p` |
| Arm GNU Toolchain | **14.2.Rel1** (`arm-none-eabi-gcc 14.2.1`) | `/opt/silabs/installs/conan/p/gcc-a469753773d4f2/p/bin` |
| slc-cli | **6.0.23** | `~/.local/opt/slc_cli/slc` |
| Java (for slc) | 21 | provided with slc |
| Target part | `EFR32MG24B220F1536IM48` (no board file: pins in `firmware/config/`) | |

`/opt/silabs` is read-only. Never install into it.

## Build and check

```sh
scripts/build.sh
```

It runs:

```sh
export PATH=/opt/silabs/installs/conan/p/gcc-a469753773d4f2/p/bin:$PATH
slc generate -p firmware/xiao_mg24_rgb_lamp.slcp -d build/gen --with EFR32MG24B220F1536IM48 -o makefile
make -C build/gen -j"$(nproc)" -f xiao_mg24_rgb_lamp.Makefile
```

Output: `build/gen/build/debug/xiao_mg24_rgb_lamp.{hex,s37,bin,out}`, about 40 s on this host. The build is correct
when `make` exits 0 and `arm-none-eabi-size` shows `.text` starting at `0x08006000` (check with
`arm-none-eabi-objdump -h build/gen/build/debug/xiao_mg24_rgb_lamp.out | grep -E 'vectors|\.text'`).

On another machine: install Simplicity SDK 2026.6.1, slc-cli 6.0.23 and the Arm GNU Toolchain 14.2.Rel1; run
`slc configuration --sdk <sdk path>` and `slc signature trust --sdk <sdk path>` once; then set
`SLC=<path to slc>` and `GCC_BIN=<toolchain bin>` for `scripts/build.sh`. CI (DevOps) does the same.

Expected, harmless warnings: `RAIL PTI peripheral not configured` (PTI is off on purpose), `-Wstringop-overflow` in
the prebuilt stack's `neighbor.c` (LTO), and unused `cli_cmd_zcl_*` variables in the generated `zap-cli.c`.

## Flashing safety (read `docs/flashing.md` and the `safe-flashing` skill)

* Agents never flash. The operator flashes, with OpenOCD over the XIAO's on-board CMSIS-DAP, after a full backup.
* The image is linked at `0x08006000`, above the bootloader. Never produce an image or a command that writes
  `0x08000000–0x08005FFF`, the NVM3/token area, lock bits, OTP or the Secure Engine. No `--force`, no mass erase.
* Firmware must stay recoverable: keep the 10 s awake window after reset (`APP_BOOT_AWAKE_WINDOW_MS`) and the D1
  escape. **Never use EM4.** Never reconfigure PA01/PA02 (SWCLK/SWDIO) or PA08/PA09. Never send the external flash
  Deep Power Down (`0xB9`). No reset loop without a delay.
* Every hand-off gives the validation level: written / compiled / flashed / physically tested / verified in
  Zigbee2MQTT. Agents reach *compiled* at most.

## Conventions

* Simplicity SDK APIs only (`sl_*`, emlib, `sl_zigbee_af_*`). No Arduino APIs, no mixing.
* Pins and thresholds only in `firmware/config/app_config.h`; logic in `src/`.
* No dynamic allocation in application code; event-driven (`sl_zigbee_af_event_*`, sleeptimer), no busy loops.
* Commit generated output? **No**: commit the `.slcp`, `config/` and `src/`, never `build/`.
* Commit messages: one line saying what the lamp or its user gets, e.g.
  `Lamp restores its last colour after a power cut`.
