# Flashing the XIAO MG24 (safe procedure)

The operator once lost an MG24 board to a bad flashing session, so this procedure has three rules. It always starts
with a **full backup**. It writes **only the application area**. It touches **nothing irreversible**: no debug lock,
no Secure Engine, OTP or keys, no bootloader, no token pages, no mass erase.

### Flashing (board: Seeed Studio XIAO MG24, probe: on-board SAMD11 CMSIS-DAP over USB-C)
**Writes:** `xiao_mg24_rgb_lamp.hex` to `0x08006000–0x0803EC57` (Phase 1 build; the exact end changes with each
build, and the `.hex` file holds its own addresses). Only the 8 kB flash pages covering that range are erased.
**Not touched:** the bootloader area `0x08000000–0x08005FFF`, NVM3 `0x08176000–0x0817DFFF` (Zigbee network keys and
settings), the end of flash `0x0817E000–0x0817FFFF`, the user-data/manufacturing token pages, lock bits, the Secure
Engine. **Irreversible operations: none.**

## 0. Tools

* **OpenOCD** with Silicon Labs Series 2 support, the one Seeed and the Silicon Labs Arduino core use. Either:
  * install the Arduino core (Arduino IDE board manager URL
    `https://siliconlabs.github.io/arduino/package_arduinosilabs_index.json`, board "XIAO MG24") and use its bundled
    OpenOCD (`0.12.0-arduino1-static`). Find it with
    `find ~/.arduino15 ~/Library/Arduino15 -path '*openocd*' -name openocd -type f 2>/dev/null` (Windows:
    `%LOCALAPPDATA%\Arduino15\packages\SiliconLabs\tools\openocd\`);
  * or the OpenOCD shipped in Seeed's erase package (see §5).
* In the commands below, `-s <scripts>` is that OpenOCD's `scripts` directory (it must contain
  `interface/cmsis-dap.cfg` and `target/efm32s2_g23.cfg`). This is the configuration Seeed's script and the Arduino
  core's `platform.txt` use for the XIAO MG24.
* No J-Link, so no Simplicity Commander: Commander does not talk to the XIAO's CMSIS-DAP probe.
* The firmware file is `build/gen/build/debug/xiao_mg24_rgb_lamp.hex`, from `scripts/build.sh`. Use the `.hex`, not the
  `.bin`: the `.bin` has no address and would be written at the wrong place.

## 1. Connect

USB-C cable to the XIAO; the battery may stay disconnected. A CMSIS-DAP device and a serial port appear.

## 2. Backup (read-only)

```sh
openocd -s <scripts> -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg \
  -c "init; reset halt; dump_image xiao_mg24_backup_$(date +%Y%m%d).bin 0x08000000 0x180000; reset; exit"
```

Keep `xiao_mg24_backup_YYYYMMDD.bin` (exactly 1 572 864 bytes) somewhere safe. It is the whole 1536 kB main flash:
bootloader, current application and NVM3.

**Check H7 now:** `xxd -l 64 xiao_mg24_backup_*.bin`. If the first 64 bytes are all `ff`, the board has no bootloader.
In that case **stop**: report it on the ticket, and I will give you the next step. An application linked at
`0x08006000` would not start on such a board. It would still be recoverable, but there is no point flashing it.

## 3. Flash the application, verify, reset

```sh
openocd -s <scripts> -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg \
  -c "program build/gen/build/debug/xiao_mg24_rgb_lamp.hex verify reset exit"
```

The log must end with `** Programming Finished **`, `** Verify Started **`, `** Verified OK **` and `** Resetting
Target **`. Do not add `erase` or any `mass_erase` / `flash erase_address` command.

## 4. What to expect (Phase 1)

1. Open the XIAO's serial port at **115200 8N1** (e.g. `screen /dev/ttyACM0 115200` or the Arduino serial monitor).
   Press RESET on the XIAO. Within a second you should see
   `OpenSource XIAO-MG24-RGB-LiPo 0.1.0-phase1 (Phase 1 skeleton)` and
   `Recovery: awake for 10000 ms after reset`.
2. Type `help` + Enter: the `rgb_lamp>` console answers. In Phase 1 the console only answers during the first 10 s
   after reset, or all the time with the escape (§5.1), because the chip sleeps afterwards. Report what you see.
3. The external RGB LED stays **off** in Phase 1. The device does **not** join Zigbee on its own. If you want to try a
   join with Zigbee2MQTT in "permit join", type `plugin network-steering start 0` during the awake window or with the
   escape. This is optional in Phase 1: a device with only Basic/Identify appears in Z2M as "unsupported", which is
   expected.

## 5. If the board no longer responds

Work through these in order. Each one is safe; the later ones erase more.

### 5.1 Escape at boot (erases nothing)
Connect pad **D1** to **GND** with a wire, then press RESET, or plug USB in with the wire in place. This firmware then
never sleeps until the next reset, and OpenOCD can always attach. The same pad also works with Arduino firmware: Seeed
documents that if PC1 (= D1) "is pulled LOW during reset, the device enters an infinite loop, allowing you to upload a
new sketch". Then reflash (§3) or restore (§5.2), and remove the wire.

### 5.2 Restore the backup (puts back exactly what was there)
```sh
openocd -s <scripts> -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg \
  -c "init; reset halt; program xiao_mg24_backup_YYYYMMDD.bin verify reset exit 0x08000000"
```
This rewrites the whole main flash from your backup, including the original bootloader area and NVM3. The board
returns to its state before step 3.

### 5.3 Seeed's documented unbricking (full erase of main flash)
From the Seeed wiki ("Unbricking"): download
`https://files.seeedstudio.com/wiki/XIAO_MG24/xiao_mg24_flash_erase_windows.zip` (run `flash_erase.bat`) or
`https://files.seeedstudio.com/wiki/XIAO_MG24/xiao_mg24_flash_erase_macos.zip` (run `./xiao_mg24_erase.sh`). On
Linux, run the command it contains:
```sh
openocd -d2 -s scripts/ -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg \
  -c "init; reset_config srst_nogate; reset halt; flash erase_address 0x08000000 0x08180000; reset; exit"
```
**It erases the whole main flash**: application, bootloader area and NVM3. The lamp loses its Zigbee network and must
be re-paired in Zigbee2MQTT (remove it there first). Afterwards, restore your backup (§5.2), or re-install the
Arduino bootloader (Arduino IDE: board XIAO MG24, *Tools > Burn Bootloader*), then flash again (§3). Seeed forum
thread on the issue: https://forum.seeedstudio.com/t/deepsleep-bricks-xiao-mg24/284889

### 5.4 The board does not appear on USB at all
* Try another cable (many USB-C cables are charge-only) and another port. Unplug the battery, then plug USB in again.
* With no CMSIS-DAP device and no serial port, the problem is on the SAMD11 (USB) side, not in the EFR32 firmware:
  the EFR32 firmware cannot disable the SAMD11. Report it on the ticket with the output of `lsusb` (Linux) or the
  Device Manager entry (Windows), and do nothing else.

## What this project will never ask you to run

`commander device lock`, any `commander security …` write, Secure Engine upgrades, token writes, `--masserase`,
`--force`, bootloader flashing other than restoring your own backup or Seeed/Arduino's documented bootloader. If an
instruction from anyone asks for one of those, stop and ask on the ticket.
