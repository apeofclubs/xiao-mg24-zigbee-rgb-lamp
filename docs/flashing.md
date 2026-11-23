# Flashing the XIAO MG24 (safe procedure)

The operator once lost an MG24 board to a bad flashing session, so this procedure has three rules. It always starts
with a **full backup**. It writes **only the application area**. It touches **nothing irreversible**: no debug lock,
no Secure Engine, OTP or keys, no bootloader, no token pages, no mass erase.

### Flashing (board: Seeed Studio XIAO MG24, probe: on-board SAMD11 CMSIS-DAP over USB-C)
**Writes:** `xiao_mg24_rgb_lamp.hex` to `0x08006000–0x080488DB` (Phase 3 build on the agents' host, two segments
`0x08006000–0x0800627B` and `0x08006280–0x080488DB`, about 266 kB; the exact end changes with each build and the CI
build differs by a few bytes; the `.hex` file holds its own addresses). Only the 8 kB flash pages covering that range
are erased. Margin to NVM3: about 1.18 MB (`0x080488DC` to `0x08176000`).
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
* The firmware file is `xiao_mg24_rgb_lamp.hex`. Get it one of two ways:
  * **CI artifact** (no tools to install): CI runs on pull requests to `main` (and on pushes to `main`). Open the
    pull request for the task branch, then on GitHub *Actions* > workflow *firmware* > the run for its latest commit >
    *Artifacts* > download the zip (kept 14 days) and extract `xiao_mg24_rgb_lamp.hex`. The run must be green: it
    means the build passed and `scripts/check_image.py` accepted the image.
  * **Local build**: `scripts/build.sh`, then `scripts/check_image.py`; the file is
    `build/gen/build/debug/xiao_mg24_rgb_lamp.hex`.

  Use the `.hex`, not the `.bin`: the `.bin` has no address and would be written at the wrong place.

## 1. Connect

USB-C data cable to the XIAO; keep the battery disconnected in Phase 2. A CMSIS-DAP device appears (Windows:
"Périphérique USB composite", `USB\VID_2886&PID_0062`). The board's USB serial port ("Virtual COM-Port") is **not
used** by this firmware: on Windows 8.1 it has no driver, and that is fine. Do not install a driver for it.

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
  tokens (network key, PAN, …) and, from Phase 2, the lamp's own settings (balance, fade time) in one object under
  the application's key `0x0A100` (user domain `0x00000–0x0FFFF`; the stack's objects are in `0x10000–0x1FFFF`).
  From Phase 3 a second application object, `0x0A101`, holds the last light state (on/off, level, colour, start-up
  attributes); `zigbee reset` deletes only that one and the stack's network data, never `0x0A100`.
* **First boot of the Phase 3 image over Phase 2:** the balance and fade time (`0x0A100`) are read as before and
  kept. There is no `0x0A101` yet, so the lamp starts **off** and not joined (Phase 2 never joined). The stack finds
  no network in its tokens and waits for `zigbee join`. Nothing in NVM3 is erased or rewritten by the update.
* **§3 never erases or writes NVM3, nor any page outside the `.hex` segments.** `program … verify reset exit` erases
  only the 8 kB pages that cover the `.hex` segments, which end far below NVM3 (Phase 3:
  `0x080488DB`; `scripts/check_image.py` from AUR-412 is meant to reject any segment outside the application area).
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

**Backup first.** The backup of §2 stays valid as long as the bootloader area has not changed; our images never write
it. To check it without a new full backup (read-only, Linux; Windows in §6.2a), read the first 24 kB and compare them
with the backup:
```sh
openocd -s <scripts> -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg \
  -c "init; reset halt; dump_image boot_now.bin 0x08000000 0x6000; reset; exit"
cmp boot_now.bin <(head -c 24576 xiao_mg24_backup_20261122_b.bin) && echo "bootloader unchanged"
```
`bootloader unchanged` = keep the existing backup. Any difference: stop, take a new full backup (§2) and report it.

```sh
openocd -s <scripts> -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg \
  -c "program build/gen/build/debug/xiao_mg24_rgb_lamp.hex verify reset exit"
```

The log must end with `** Programming Finished **`, `** Verify Started **`, `** Verified OK **` and `** Resetting
Target **`. Do not add `erase` or any `mass_erase` / `flash erase_address` command.

## 4. What to expect (Phase 3)

1. First boot of the Phase 3 image: the external RGB LED stays **off**. Later boots restore the last light state
   (or what the start-up attributes ask for, `zigbee.md` §4).
2. Open the console over RTT (§4.1). Type `help` + Enter: the `rgb_lamp>` prompt answers; `lamp status` prints the
   lamp's state and `zigbee status` the network state. The bench tests are in `docs/bench-test.md` §3.
3. The boot messages (`OpenSource XIAO-MG24-RGB-LiPo 0.3.0-phase3 …`, `Recovery: awake for 10000 ms after reset`) are
   printed before OpenOCD attaches, so you usually will not see them. They are not needed.
4. The device does **not** join Zigbee on its own: `zigbee join` after opening permit-join in Zigbee2MQTT
   (`zigbee.md` §2). Once joined, it rejoins the same network by itself after a reset.

### 4.1 Console over SEGGER RTT (Linux; Windows in §6.5)
The console goes through the same CMSIS-DAP link as flashing: no serial port, no driver. OpenOCD reads and writes
the firmware's RTT buffers in RAM while the firmware runs. It **does not reset or halt the board and writes nothing to
flash**.
```sh
openocd -s <scripts> -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg \
  -c "init; rtt setup 0x20000000 0x10000 {SEGGER RTT}; rtt start; rtt server start 19021 0"
```
* The log must show `rtt: Control block found at 0x2000….` (Phase 2 local build: `0x200040AC`; the CI build can
  differ slightly, which is why OpenOCD searches the first 64 kB of RAM for it).
* In a second terminal: `nc 127.0.0.1 19021` (or `telnet 127.0.0.1 19021`). Type the commands there.
* Stop with `Ctrl+C` in the OpenOCD terminal; the lamp keeps running. **Only one program can use the probe**: stop the
  RTT session before flashing (§3) or backing up (§2), and start it again afterwards.
* The bench build holds EM1 (`APP_CONSOLE_KEEP_AWAKE = 1` in `firmware/config/app_config.h`), so the probe can
  always read the buffers. It never uses EM2 sleep, and **never EM4**.

## 5. If the board no longer responds

Work through these in order. Each one is safe; the later ones erase more.

### 5.1 Escape at boot (erases nothing)
Connect pad **D1** (PC1) to **GND** with a wire, then press RESET, or plug USB in with the wire in place.
* **With this project's firmware**: the firmware itself reads D1 at boot (`src/power_manager.c`). If it is LOW, the
  chip never sleeps until the next reset, so OpenOCD can always attach. The Phase 2 bench build never sleeps anyway;
  the escape matters from Phase 5 (battery builds) on.
* **With Arduino firmware** (for example after restoring the stock backup): Seeed documents that if PC1 "is pulled LOW
  during reset, the device enters an infinite loop, allowing you to upload a new sketch". That loop is code in the
  Arduino core, **not** a chip or bootloader feature: it does not exist in our Simplicity SDK firmware, which uses
  the same pad for its own escape described above.

Then reflash (§3) or restore (§5.2), and remove the wire. The board has no BOOT button.

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
**It erases the whole main flash** (`0x08000000–0x0817FFFF`): application, bootloader area and NVM3. The lamp loses
its settings (balance, fade time) and its Zigbee network and must be re-paired in Zigbee2MQTT (remove it there first).
It does not touch lock bits, tokens outside main flash or the Secure Engine. Use it only if §5.1 and §5.2 fail. Afterwards, restore your backup (§5.2), or re-install the
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

### 6.2a Check that the bootloader area is unchanged (read-only, before §6.3)
```powershell
Set-Location "$HOME\xiao_mg24_backups"
& $openocd -s $scripts -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg -c "init; reset halt; dump_image boot_now.bin 0x08000000 0x6000; reset; exit"
$a = [IO.File]::ReadAllBytes("$PWD\boot_now.bin"); $b = [IO.File]::ReadAllBytes("$PWD\xiao_mg24_backup_20261122_b.bin")
$diff = 0; for ($i = 0; $i -lt 0x6000; $i++) { if ($a[$i] -ne $b[$i]) { $diff++ } }; "length $($a.Length), differing bytes: $diff"
```
`length 24576, differing bytes: 0` = the bootloader is unchanged and the existing backup is still the one to keep.
Anything else: stop, take a new full backup (§6.2) and report it.

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

### 6.5 Console on Windows (SEGGER RTT, no COM port, no driver)
Same as §4.1, with the OpenOCD found in §6.1. On the operator's Windows 8.1 the XIAO's "Virtual COM-Port" has no
driver: leave it as it is, it is not used.
1. With the lamp firmware running (§6.3 done, at least 2 s after reset), in PowerShell:
   ```powershell
   & $openocd -s $scripts -f interface/cmsis-dap.cfg -f target/efm32s2_g23.cfg -c "init; rtt setup 0x20000000 0x10000 {SEGGER RTT}; rtt start; rtt server start 19021 0"
   ```
   It attaches without resetting or halting the board and writes nothing to flash. Leave this window open. The log
   must show `rtt: Control block found at 0x2000….`; if it says the block was not found, wait 2 s and run it again,
   then report the log.
2. Open a terminal on `127.0.0.1`, port `19021`: **PuTTY** (the standalone `putty.exe` needs no installation),
   *Connection type* **Raw**, Host `127.0.0.1`, Port `19021`. Windows' own Telnet client also works
   (`telnet 127.0.0.1 19021`) if the *Telnet Client* Windows feature is enabled.
3. Type `help`, `lamp status`, `lamp rgb 255 0 0`, … (`docs/bench-test.md`).
4. To stop: close PuTTY, then `Ctrl+C` in the OpenOCD window. The lamp keeps running. **Stop OpenOCD before flashing
   (§6.3) or backing up (§6.2)**: only one program can use the probe at a time.

## What this project will never ask you to run

`commander device lock`, any `commander security …` write, Secure Engine upgrades, token writes, `--masserase`,
`--force`, bootloader flashing other than restoring your own backup or Seeed/Arduino's documented bootloader. If an
instruction from anyone asks for one of those, stop and ask on the ticket.
