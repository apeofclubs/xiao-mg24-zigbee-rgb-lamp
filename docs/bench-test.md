# Bench tests

Phase 2 (local RGB from the console): §0–§2. Phase 3 (Zigbee light): §3. Troubleshooting for both: §4.

## Phase 2: local RGB from the console

Goal: check that the firmware drives the LED correctly (colours, balance, fades, fully off) and measure the current
per channel. This is the roadmap's Phase 2 gate. Zigbee is not involved yet.

## 0. Before you start (safety)

* **USB power only, battery disconnected.** The Phase 2 bench build never sleeps (it holds EM1 so the console always
  answers); that is fine on USB and not meant for the battery.
* Wiring as in `wiring.md`: red **100 Ω** on D0, green 220 Ω on D2, blue 100 Ω on D3 (as measured on 2026-11-23),
  common (long leg) on **GND**.
  Every LED leg goes through its resistor: never connect an LED leg straight to a pin.
* Flash as in `flashing.md`: backup first (already done: `xiao_mg24_backup_20261122_b.bin`), then §6.3 on Windows.
  Only the application area is written; nothing irreversible.
* Open the console as in `flashing.md` §6.5 (Windows) or §4.1 (Linux): OpenOCD with RTT, then PuTTY *Raw* on
  `127.0.0.1:19021`. No COM port and no driver are needed.

## 1. Console commands

Type each command and press Enter. Every command except `lamp status` and `lamp balance` (with no argument) stops a
running effect.

| Command | What it does |
|---|---|
| `lamp status` | Prints the target (on/off, colour, level, effect), the balance, the fade time and the PWM duty per channel in % and in timer counts. `pwm: stopped` = timer off, all pins at their OFF level. |
| `lamp rgb <r> <g> <b>` | Colour, 0–255 per channel; turns the lamp on. Example: `lamp rgb 255 0 0`. |
| `lamp hsv <h> <s> <v>` | Colour from hue 0–359, saturation 0–100, value 0–100. Example: `lamp hsv 300 100 100` (magenta). |
| `lamp level <0-100>` | Brightness in %, gamma-corrected (50 % looks about half as bright). `0` turns off and keeps the previous level for the next colour command. |
| `lamp off` | Fades to off. |
| `lamp white` | Balanced white (255, 255, 255 through the balance) and prints the balance: use it to judge the balance. |
| `lamp balance` | Shows the balance (default red 100 %, green 100 %, blue 100 %: a good white with 100/220/100 Ω). |
| `lamp balance <r> <g> <b>` | Sets the balance, 0–100 % per channel, applies it at once and **saves it** (kept across resets and reflashing). |
| `lamp preset <name>` | Named colour: `warmwhite`, `white`, `red`, `green`, `blue`, `orange`, `yellow`, `purple`, `cyan`, `pink`. `lamp preset` alone lists them. |
| `lamp fade <ms>` | Transition time for the next changes, 0–10000 ms (0 = instant); default 400 ms; **saved**. |
| `lamp effect rainbow` | Slow hue cycle (12 s per turn) at the current level. |
| `lamp effect breathe` | Current colour slowly dims and brightens (4 s). |
| `lamp effect off` | Stops the effect; the lamp keeps the colour it was showing. |
| `help` | Lists every console command (including the Zigbee stack's own, not needed here). |

How the colour is computed: colour × level, then gamma 2.2, then the balance of each channel. So `lamp balance` only
scales one colour against the others; `lamp level` dims all three together.

## 2. Tests to run and what to report

### 2.1 LED fully off (boot and `off`)
1. Right after flashing, and after pressing RESET: the LED must be **completely dark** (look closely in a dim room).
2. `lamp rgb 255 255 255`, then `lamp off`, wait 1 s, then `lamp status`: it must print `target: off` and
   `pwm: stopped`, and the LED must be completely dark.

Report: dark at boot yes/no; dark after `lamp off` yes/no; the `lamp status` output.

### 2.2 Current per channel at 100 % (multimeter)
Measure one channel at a time. **Set the multimeter to the mA range (DC) before connecting it**, and put it **in
series**: unsolder or unplug the wire between the channel's resistor and its pad (D0, D2 or D3), then connect the
meter between the resistor and the pad. Never put the meter's probes directly across a pin and GND or 3V3 on the mA
range: that is a short circuit.

1. `lamp balance 100 100 100` (full duty on every channel for this measurement) and `lamp fade 0`.
2. Red: `lamp rgb 255 0 0`, read the meter. Green: `lamp rgb 0 255 0`. Blue: `lamp rgb 0 0 255`. For each one, also
   type `lamp status`: the measured channel must show `100.0%`.
3. White, all three together: `lamp rgb 255 255 255` (measure each channel in turn, or the common leg to GND).
4. Put back the balance you prefer (see 2.3) and `lamp fade 400`.

Expected (estimates from `wiring.md`, 3.3 V rail): **red ≈ 6 mA** (5.9–6.8 mA), **green ≈ 1–2 mA**, **blue ≈ 1–2 mA**.
Stop and report if any channel is above **10 mA**, or if a channel reads 0 mA while its LED is lit (meter wiring).

Report: the four readings in mA (red, green, blue, and white per channel or total).

### 2.3 Balance and colours
1. `lamp white`. If the white looks tinted, adjust and look again, for example:
   * greenish → lower green: `lamp balance 100 30 70`;
   * bluish → lower blue: `lamp balance 100 40 55`;
   * pinkish / reddish → raise green and blue, or lower red: `lamp balance 85 45 75`.

   Each `lamp balance r g b` is applied at once and saved.
2. Then try: `lamp preset warmwhite`, `orange`, `yellow`, `purple`, `cyan`, `pink`, and a few hues:
   `lamp hsv 30 100 100` (orange), `lamp hsv 180 100 100` (cyan), `lamp hsv 270 100 100` (violet),
   `lamp hsv 330 60 100` (pastel pink).
3. Brightness: `lamp preset warmwhite`, then `lamp level 100`, `50`, `20`, `5`, `1`. Each step should look clearly
   dimmer, with no visible flicker. `lamp level 0` turns off.
4. Fades and effects: `lamp fade 2000`, then `lamp preset blue`, `lamp preset orange` (a slow, even cross-fade);
   `lamp effect rainbow` for about 30 s; `lamp effect breathe`; `lamp effect off`; back to `lamp fade 400`.

Report: the final `lamp balance` values; which colours look right or wrong (and how); any flicker, step or flash
during fades and effects; the output of `lamp status`.

### 2.4 Settings kept after a reset
Press RESET (or unplug and replug USB), restart the RTT session (`flashing.md` §6.5) and type `lamp balance`: it must
show the values you saved. The LED must be off after the reset.

Report: the balance after the reset.

## 3. Phase 3: Zigbee light

Goal: the roadmap's Phase 3 gate ("Needs from you before Phase 4"). Firmware `0.3.0-phase3`, bench build: USB power
only, battery disconnected (it never sleeps, so the console always answers). Commands and background: `zigbee.md`.

### 3.0 Flash
1. Bootloader unchanged, so the existing backup is still valid: `flashing.md` §6.2a must print
   `length 24576, differing bytes: 0`. Anything else: stop and report.
2. Flash the CI `.hex` with `flashing.md` §6.3 (`program … verify reset exit`, no `erase`).
3. Open the RTT console (`flashing.md` §6.5). The LED is **off**. `lamp balance` shows the balance you saved in
   Phase 2 (it is kept). `zigbee status` shows `network: no network`.

Report: the §6.2a line, the end of the OpenOCD log, `lamp balance` and `zigbee status` outputs.

### 3.1 Re-tune the white (blue is the limit)
Blue is the weakest channel (3.3 mA at 100 %): set blue to 100 % and lower the others, for example
`lamp white`, then `lamp balance 60 70 100` and adjust by eye. Report the final `lamp balance`.

### 3.2 Pair and check Zigbee2MQTT
1. Your Zigbee2MQTT 2.14.3 / converters 26.125.0 / EmberZNet 7.4.4 install is already noted (`zigbee.md`, top);
   tell me only if it changed. Say whether mains-powered Zigbee routers are near the lamp (H13).
2. If you can, pair once **without** the converter first: permit join in Z2M, `zigbee join` on the console, wait for
   `Join done` and the end of the interview, then copy the Z2M **About** and **Exposes** tabs (screenshot or text).
3. Install the converter (`zigbee.md` §6), restart Z2M, *Reconfigure* the lamp (or skip step 2 and pair now with the
   converter). Copy **About** and **Exposes** again.

Report: the console lines from `zigbee join` to `Network up`, the Z2M log lines for the join and interview, the
About/Exposes tabs (without and with the converter), and `zigbee status`.

### 3.3 Control from Home Assistant (with the delay)
For each action, note: works yes/no, and the delay between the click in HA and the LED (a stopwatch or a phone
video is enough).
1. ON, OFF, toggle a few times.
2. Brightness: 100 %, 50 %, 10 %, 1 %; and a change with a transition (HA *light.turn_on* with `transition: 3`).
3. Colour: red, green, blue, a few picker colours, white (colour picker → xy).
4. Colour temperature: coldest, warmest, middle.
5. In Z2M's *Exposes* tab: hue/saturation colour (if shown), `power_on_behavior`, and the *Identify* button: the LED
   must blink for a few seconds and then go back to its state.

Report: yes/no and the delay for each; anything that looks wrong (colour, brightness steps, flicker).

### 3.4 Last state after a lamp reset
1. Set a colour and brightness from HA. Wait **40 s** (the save waits until the state is stable, `zigbee.md` §4).
2. Press RESET on the XIAO (or unplug/replug USB). The lamp must come back **on, with the same colour and
   brightness**, within about 1 s, and HA must control it again **without re-pairing**.
3. In Z2M set `power_on_behavior` to `off`, wait 40 s, press RESET: the lamp must stay **off**. Set it back to
   `previous`.
4. `zigbee status`: note `last-state NVM3 writes since boot` after a burst of 10 quick changes from HA (it should
   grow by 1, not by 10).

Report: what the lamp did after each reset, and the write counter.

### 3.5 Coordinator restart
Restart Zigbee2MQTT (and, if your coordinator allows it, power-cycle the coordinator). Then, every 15 s, toggle the
lamp from HA until it reacts. Meanwhile `zigbee status` on the console.

Report: how long until the lamp reacted again, and the `network:` lines seen in `zigbee status` meanwhile.

### 3.6 H6: link quality in the three RF-switch states
Keep the lamp at a fixed place (a few metres from its parent, same orientation) for the whole test. For each of
`zigbee rfswitch reset`, `zigbee rfswitch 0`, `zigbee rfswitch 1`:
1. Type the command, then toggle the lamp 5 times from HA.
2. `zigbee status`: note the `parent link` LQI and RSSI. Also note the LQI Z2M shows for the lamp.
3. If the lamp stops reacting in a state: note it, press RESET (back to `reset`) and go on.

Nothing is saved: after a RESET the switch is back in `reset`. With nothing on the U.FL connector this is safe
(`feasibility.md` §6).

Report: a small table: state, LQI, RSSI, Z2M LQI, reacts yes/no.

### 3.7 H10: reaction time and a Z2M group
1. Toggle from HA 10 times and note the typical delay (3.3 already gives it).
2. In Z2M, create a group, add the lamp, and toggle the **group** from HA: works yes/no, delay.

Report: the delays, direct and through the group.

### 3.8 Factory reset and re-pairing
1. `zigbee reset`: the console prints `factory reset done …`, the LED goes off, Z2M shows the lamp leaving (or not
   responding). `lamp balance` still shows your balance. `zigbee status` shows `no network`.
2. Re-pair: permit join in Z2M, `zigbee join` (`zigbee.md` §3). HA controls it again (the entity may need to be
   re-enabled if Z2M renamed it).

Report: the console output of `zigbee reset` and `lamp balance`, and whether re-pairing worked.

### What to send back (Phase 4 gate)
1. §3.0 checks, routers near the lamp (H13), and your Z2M versions only if they changed.
2. Final `lamp balance` (§3.1).
3. About/Exposes without and with the converter (§3.2).
4. ON/OFF, brightness, colour, colour temperature, identify from HA: yes/no + delay (§3.3).
5. Behaviour after a lamp reset (§3.4) and after a coordinator restart (§3.5).
6. H6 table (§3.6); H10 delays incl. the group (§3.7).
7. Factory reset and re-pairing (§3.8).

## 4. If something goes wrong

* **The console does not answer** (OpenOCD says the RTT control block is not found): wait 2 s after reset and start
  OpenOCD again; report its log.
* **OpenOCD cannot open the probe**: close any other OpenOCD window (only one program can use the probe), then retry.
* **The LED stays on after `lamp off`**, or a colour is wrong at boot: stop, report the `lamp status` output and a
  photo of the wiring.
* **The board no longer responds at all**: `flashing.md` §5 (escape, restore the backup, then Seeed's erase).
