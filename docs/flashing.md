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
    `%LOCALAPPDATA%\Arduino15\packages\SiliconLabs\tools\openocd\`, see §6.1);
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

On Windows, use the PowerShell commands in §6 instead.

```sh
openocd -s <scripts> -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg \
  -c "init; reset halt; dump_image xiao_mg24_backup_$(date +%Y%m%d).bin 0x08000000 0x180000; reset; exit"
```

Keep `xiao_mg24_backup_YYYYMMDD.bin` (exactly 1 572 864 bytes) somewhere safe. It is the whole 1536 kB main flash:
bootloader, current application and NVM3.

**Check H7 now:** `xxd -l 64 xiao_mg24_backup_*.bin`. If the first 64 bytes are all `ff`, the board has no bootloader.
In that case **stop**: report it on the ticket, and I will give you the next step. An application linked at
`0x08006000` would not start on such a board. It would still be recoverable, but there is no point flashing it.

### 2.1 What the backup tells us about this board (operator's board, 2026-11-22)

Reference backup: **`xiao_mg24_backup_20261122_b.bin`** (1 572 864 bytes). Keep it; §5.2 / §6.4 restore it.

| Range | Content |
| --- | --- |
| `0x08000000–0x08005FFF` | Bootloader (H7 confirmed: SP `0x20001008`, reset `0x08000171`). Never written by §3. |
| `0x08006000–…` | Stock application, vector table at `0x08006000`: where our `.hex` is linked. |
| around `0x081747CC–0x08174842` | Data written by the stock application at runtime (two reads differ in 106 bytes there, nowhere else). Most likely the stock application's own NVM3. |

Both reads were taken with the CPU halted (`reset halt` before `dump_image`), so each one is a consistent snapshot;
they differ only because the stock application ran (and wrote its data) between the two sessions. Either file is a
valid restore image; `_b` is the reference.

### 2.2 How the NVM3 area is handled

* **Our image's NVM3** is `0x08176000–0x0817DFFF` (32 kB, `NVM3_DEFAULT_NVM_SIZE = 32768` in
  `firmware/xiao_mg24_rgb_lamp.slcp`; placed by the linker, not stored in the `.hex`). It holds the Zigbee stack's
  tokens (network key, PAN, …) and, from Phase 2, the lamp's own settings under the application's keys.
* **§3 never erases or writes NVM3, nor any page outside the `.hex` segments.** `program … verify reset exit` erases
  only the 8 kB pages that cover the `.hex` segments, which end far below NVM3 (Phase 1:
  `0x0803EC57`; `scripts/check_image.py` from AUR-412 is meant to reject any segment outside the application area).
  So:
  * flashing a new build of the lamp over an older one **keeps** the lamp's settings and its Zigbee network (no
    re-pairing);
  * flashing the lamp over the stock firmware the first time leaves the stock application's data in place (it is
    simply not used by our image, except in the case below).
* **Open point (does not block Phase 2's console tests):** the stock data sits at `0x08174xxx`, just below our NVM3
  start. If the stock NVM3 is larger than 8 kB (for example 40 kB, `0x08174000–0x0817DFFF`, the SDK default size),
  its pages overlap ours, and our image would open the stock application's NVM3 pages instead of a blank area. I have
  not verified how NVM3 and the Zigbee stack react to that, so the read-only scan below settles it first. It reads the
  **backup file only**, nothing on the board. It lists every 8 kB page that starts with an NVM3 page header (first
  word `0xB29A0001`, bytes `01 00 9A B2`, from `nvm3_page.c` in the Simplicity SDK):
  ```powershell
  Set-Location "$HOME\xiao_mg24_backups"
  $a = [IO.File]::ReadAllBytes("$PWD\xiao_mg24_backup_20261122_b.bin")
  for ($o = 0; $o -lt $a.Length; $o += 0x2000) {
    if ($a[$o] -eq 0x01 -and $a[$o+1] -eq 0x00 -and $a[$o+2] -eq 0x9A -and $a[$o+3] -eq 0xB2) { "NVM3 page at 0x{0:X8}" -f (0x08000000 + $o) }
  }
  ```
  (Linux: `for o in $(seq 0 8192 1572863); do [ "$(xxd -s $o -l 4 -p xiao_mg24_backup_20261122_b.bin)" = 01009ab2 ] && printf 'NVM3 page at 0x%08X\n' $((0x08000000+o)); done`.)
  * **No page at or above `0x08176000`:** no overlap. §3 is enough; nothing else to do.
  * **Pages at or above `0x08176000`:** report the list on the ticket before the first flash of the lamp. I will then
    propose, for your explicit approval, a one-time erase of **only** `0x08176000–0x0817DFFF` (our NVM3, 4 pages),
    with the exact command. It is reversible from this backup (§5.2), but it is not part of the default procedure,
    so do not erase anything on your own.

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
returns to its state before step 3. In detail:
* `program` of a `.bin` at `0x08000000` erases and rewrites every 8 kB page of the 1536 kB main flash (the backup
  covers all of it, `FF` pages included), then `verify` reads it all back and compares. Expect about a minute.
* NVM3 comes back **as it was in the backup**: the stock application's data from that day returns, and the **lamp's
  own NVM3 (balance, Zigbee network) is lost**. After a restore, if the lamp had joined a network, remove it in
  Zigbee2MQTT; it must be re-paired after the lamp firmware is flashed again.
* The bootloader area is rewritten with the same bytes it already holds. This is the only command in this document
  that writes there; use it only for recovery, and only with a backup of this board whose size is exactly
  1 572 864 bytes and whose first bytes match §2.1 (`08 10 00 20 71 01 00 08`).
* Token pages (user data, manufacturing), lock bits and the Secure Engine are outside main flash and are not touched.
* For this board use `xiao_mg24_backup_20261122_b.bin`.

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

## 6. Windows (PowerShell) variant of §0, §2, §3 and §5.2

Same operations, same addresses, nothing more is written. Open **PowerShell** (not `cmd`) and keep it open: the
variables below are lost when the window closes, so set them again in a new window.

### 6.1 Find OpenOCD (read-only)
```powershell
$root = "$env:LOCALAPPDATA\Arduino15\packages\SiliconLabs\tools\openocd"
Get-ChildItem $root -Recurse -Filter openocd.exe | Select-Object -ExpandProperty FullName
Get-ChildItem $root -Recurse -Filter efm32s2_g23.cfg | Select-Object -ExpandProperty FullName
```
Each command should print one path. If it prints several (several OpenOCD versions), pick the same version folder for
both. Then set the two variables (replace the paths with the ones printed above):
```powershell
$openocd = "C:\...\openocd.exe"
$scripts = "C:\...\scripts"   # the parent of the "target" folder printed above
Test-Path "$scripts\interface\cmsis-dap.cfg"; Test-Path "$scripts\target\efm32s2_g23.cfg"   # both must print True
```
If `$root` does not exist, the Silicon Labs Arduino core is not installed (§0) or the Arduino IDE stores its data
elsewhere: report it on the ticket.

### 6.2 Backup (read-only)
Work in a dedicated folder and use a **bare file name**, with no `C:\…` path. OpenOCD reads the text after `-c` as
Tcl, and Tcl treats `\` as an escape character, so a Windows path in there would be mangled.
```powershell
New-Item -ItemType Directory -Force "$HOME\xiao_mg24_backups" | Out-Null
Set-Location "$HOME\xiao_mg24_backups"
$backup = "xiao_mg24_backup_$(Get-Date -Format yyyyMMdd).bin"
& $openocd -s $scripts -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg -c "init; reset halt; dump_image $backup 0x08000000 0x180000; reset; exit"
```
The `&` is required because `$openocd` is a path. `$(Get-Date -Format yyyyMMdd)` replaces Linux's `$(date +%Y%m%d)`.
Check the result:
```powershell
(Get-Item $backup).Length                      # must print 1572864
Format-Hex -Path $backup | Select-Object -First 4   # first 64 bytes (replaces xxd -l 64)
```
* A size other than **1572864**: the backup is incomplete. Do not go on to §6.3; report the OpenOCD log.
* **H7**: if those first 64 bytes are all `FF`, stop and report it on the ticket (see §2).
* Copy the `.bin` somewhere other than this PC as well (USB stick, cloud).
* `Error: unable to find a CMSIS-DAP device`: check the cable (data, not charge-only), close the Arduino IDE and any
  serial monitor, and try again. Report the full log if it persists.

### 6.3 Flash the application, verify, reset
Copy `xiao_mg24_rgb_lamp.hex` (from `scripts/build.sh` or the CI artifact) into the same folder, then:
```powershell
Set-Location "$HOME\xiao_mg24_backups"
& $openocd -s $scripts -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg -c "program xiao_mg24_rgb_lamp.hex verify reset exit"
```
Same checks as §3: the log ends with `** Verified OK **` and `** Resetting Target **`. Do not add `erase`.

### 6.4 Restore the backup (§5.2)
```powershell
Set-Location "$HOME\xiao_mg24_backups"
& $openocd -s $scripts -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg -c "program xiao_mg24_backup_YYYYMMDD.bin verify reset exit 0x08000000"
```
Replace `YYYYMMDD` with your backup's date. The address `0x08000000` is required for a `.bin`.

### 6.5 Serial console on Windows
The XIAO's serial port appears in Device Manager under **Ports (COM et LPT)** as *Périphérique série USB (COMx)*.
Open it at **115200 8N1** with the Arduino IDE serial monitor (line ending: *Newline* or *Both NL & CR*) or PuTTY
(*Serial*, `COMx`, 115200). Close it before running OpenOCD if OpenOCD cannot open the probe.

## What this project will never ask you to run

`commander device lock`, any `commander security …` write, Secure Engine upgrades, token writes, `--masserase`,
`--force`, bootloader flashing other than restoring your own backup or Seeed/Arduino's documented bootloader. If an
instruction from anyone asks for one of those, stop and ask on the ticket.
