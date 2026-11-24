# Zigbee: pairing, reset, Zigbee2MQTT and Home Assistant

Phase 3 firmware (`0.3.0-phase3`). Validation level of everything below: **compiled**. It becomes
*verified in Zigbee2MQTT* only with your test (`bench-test.md` §3).

**Tested target: not yet tested.** The converter and this page were checked (by reading the published packages, and
by loading the converter with zigbee-herdsman-converters 26.125.0 outside Zigbee2MQTT) against your install, as shown
on 2026-11-24:

| | Version |
|---|---|
| Zigbee2MQTT | **2.14.3** (frontend 2.15.1), Home Assistant OS add-on, MQTT broker `core-mosquitto` |
| zigbee-herdsman-converters | **26.125.0** |
| zigbee-herdsman | 11.2.0 (adapter `ember`) |
| Node.js | v24.22.1 |
| Coordinator firmware | Silicon Labs **EmberZNet 7.4.4 [GA]** |
| Lamp's expected parent (H13) | a mains-powered Zigbee lamp next to the device, probably a router: confirm in Z2M's **Map** tab after pairing (`bench-test.md` §3.2) |

## 1. What the lamp is on the network

| | |
|---|---|
| Device type | Zigbee 3.0 **sleepy end device** (it never routes) |
| Endpoint 1 | **Extended Color Light** (`0x010D`), profile Home Automation (`0x0104`) |
| Manufacturer / model | `OpenSource` / `XIAO-MG24-RGB-LiPo` (Basic cluster), SW build id `0.3.0-phase3` |
| Server clusters | Basic, Identify, Groups, Scenes, On/Off, Level Control, Color Control |
| On/Off | On, Off, Toggle; `StartUpOnOff` (power-on behaviour) |
| Level Control | 1–254, with transition time and the "with on/off" commands; `OnLevel`, `OnOffTransitionTime`, `StartUpCurrentLevel` |
| Color Control | **hue/saturation** (normal hue, 0–254), **CIE xy**, **colour temperature** 153–500 mireds (6500–2000 K). `ColorCapabilities = 0x0019` |
| Groups, Scenes | Add/view/remove/get membership (groups and scenes), store/recall scene (8 scenes) |
| Identify | Identify (the LED blinks the current colour, white if none, 0.5 s on / 0.5 s off), Identify Query |
| Not implemented | Enhanced hue commands and colour loop (the Simplicity SDK colour server does not implement them, so `ColorCapabilities` does not claim them); Identify `TriggerEffect`; OffWithEffect, OnWithRecallGlobalScene, OnWithTimedOff; enhanced/copy scene. They are not declared in the ZCL configuration; the lamp answers "unsupported command" |

The SDK's cluster servers run the commands and transitions; the firmware turns the attributes into the LED colour
through the same path as the bench console (gamma 2.2, then your `lamp balance`, then PWM). The colour temperature is
made with the RGB LED (no white LED), so it is an approximation, tuned by the balance.

The `lamp ...` console commands still work but are **local only**: Z2M does not see them, they are not saved as the
last state, and the next Zigbee command replaces them.

## 2. Pairing with Zigbee2MQTT

The lamp **never joins on its own**. Pairing is a console command (RTT console, `flashing.md` §6.5) or a short
press on the D9 button (§8).

1. Optional but recommended: install the external converter first (§6), so the lamp is recognised at once.
2. In Zigbee2MQTT: **Permit join (All)** (top bar), or permit join on the router that should become the lamp's
   parent. Leave it open for the next step.
3. On the console: `zigbee join` (or a short press on the button: the lamp blinks blue while it searches, then shows
   green 2 s when joined, amber 2 s on failure). It answers `join: searching`. Within about 10–60 s the console prints
   `Join done: status 0x00 …` then `Network up: node 0x…, parent 0x…`. On failure it prints `Join failed: …`.
4. In Z2M the lamp appears, the interview runs (a few seconds to a minute for a sleepy device) and ends with
   "Successfully interviewed". Close permit join.
5. `zigbee status` shows the network, the parent and the parent's link quality (§7).

**With your EmberZNet 7.4.4 coordinator (`ember` adapter):** nothing to configure. The lamp joins as a standard
Zigbee 3.0 device: while permit join is open the coordinator accepts joins with the well-known preconfigured link
key (`ZigBeeAlliance09`), then the lamp asks the trust center for its own unique link key (the SDK's
`zigbee_update_tc_link_key` component); zigbee-herdsman 11.2.0 sets the coordinator to answer these requests
(`TC_KEY_REQUEST_POLICY = ALLOW_TC_KEY_REQUESTS_AND_SEND_CURRENT_KEY`). No install code is needed. If the lamp's
parent should be a router, use "Permit join" on that router (or *All*); the lamp only joins where permit join is open.
A failed key exchange shows as `Join failed` on the console and nothing in Z2M: then close and reopen permit join,
`zigbee reset`, `zigbee join` again, and send me the Z2M log of that minute (log level *debug*).

After a reset or a power cut, a joined lamp **rejoins the same network by itself** (its network keys are in NVM3):
no new pairing. If its parent disappears (coordinator restart, router unplugged), it tries to rejoin every 10 s until
the parent is back (no limit on the number of attempts in this bench build).

## 3. Leaving, factory reset, re-pairing

| Console | What it does | What it keeps |
|---|---|---|
| `zigbee leave` | Leaves the network (sends a leave notice: Z2M removes the device) | Light state, groups, scenes, balance |
| `zigbee reset`, or **hold the D9 button 10 s** | **Factory reset**: leaves the network, clears the binding table, the reporting configuration, groups, scenes, all light attributes back to their defaults (off, warm white), and deletes the saved last state (NVM3 key `0x0A101`) | Your `lamp balance` and `lamp fade` (NVM3 key `0x0A100`) |

`zigbee reset` only uses the Zigbee stack's own leave and table functions and deletes one application NVM3 object.
It never erases flash pages, the bootloader, lock bits, OTP, the Secure Engine, the user-data page or the
manufacturing tokens. A Basic "Reset to factory defaults" command sent from Z2M resets the attributes only (it does
not leave the network).

**Re-pairing** (for example after moving the lamp to another coordinator):
1. `zigbee reset` on the console or hold the button 10 s (or, in Z2M, *Remove* the device; if Z2M cannot reach it, tick *Force remove*, then
   `zigbee reset`).
2. Permit join in Z2M, then `zigbee join` or a short press (§2).

## 4. Last state after a power cut, start-up behaviour

The lamp saves its light state in NVM3 (application key `0x0A101`): on/off, level, colour mode and colour, and the
start-up settings (`StartUpOnOff`, `StartUpCurrentLevel`, `StartUpColorTemperatureMireds`, `OnLevel`,
`OnOffTransitionTime`). At boot it reads it back, then applies the start-up attributes as the ZCL defines them:

| Z2M setting (attribute) | Effect at power-up |
|---|---|
| `power_on_behavior` (`StartUpOnOff`): `off` / `on` / `toggle` / `previous` (default) | Lamp off / on / the opposite of before / as before |
| `current_level_startup` (`StartUpCurrentLevel`): `minimum` / 1–254 / `previous` (default) | That level |
| `color_temp_startup` (`StartUpColorTemperatureMireds`): 153–500 / `previous` (default) | That colour temperature (colour mode becomes colour temperature) |

The first boot of a fresh lamp, and the first boot after `zigbee reset`, start **off**.

### Flash wear (debounced writes)

The light state is never written during a fade or a series of commands. A write happens only when:
* the state has not changed for **5 s** (`APP_STATE_SAVE_DELAY_MS`), and
* at least **30 s** have passed since the previous write (`APP_STATE_SAVE_MIN_INTERVAL_MS`), and
* the state differs from what is stored (a lamp switched on and off again in between writes nothing).

**Worst-case write rate: 1 write per 30 s = 120 per hour**, and only if something changes the light every 30 s,
day and night. A change made less than 30 s after the previous save is saved 30 s after it; a power cut inside
that window loses only that last change.

What that means for the flash (estimate): one write uses about 32 bytes of the 32 kB NVM3 area, so NVM3 erases an
8 kB page about every 250 writes and each of its 4 pages roughly every 1 000 writes. With the flash endurance of
about 10 000 erase cycles (datasheet figure to be quoted, item H9) that is in the order of **10 million writes**:
**more than 10 years at the worst-case 120 writes/hour**, and far more in normal use (a few changes a day). The
Zigbee stack also stores its own data (frame counters) in the same area; it is designed for that.

`zigbee status` prints `last-state NVM3 writes since boot` so you can watch the debounce on the bench.

## 5. RF switch (H6 bench aid)

`zigbee rfswitch` shows the state of the antenna switch pins; `zigbee rfswitch reset | 0 | 1` sets them:

| State | PB05 (switch power) | PB04 (select) |
|---|---|---|
| `reset` (default at every boot, as in Phases 1–2 and the Arduino core) | disabled | disabled |
| `0` | HIGH | LOW ("RF1" on the schematic) |
| `1` | HIGH | HIGH ("RF2") |

The setting is **not saved**: RESET or a power cut returns to `reset`. Which of RF1/RF2 is the chip antenna is not
documented anywhere I could read (`feasibility.md` §6), which is why you compare the link quality in each state
(`bench-test.md` §3.6). Why it is safe for the radio: `feasibility.md` §6. If a state loses the link, press RESET.

## 6. Zigbee2MQTT external converter

File: `zigbee2mqtt/xiao_mg24_rgb_lamp.mjs` (modern extends: `light` with xy + hs colour and colour temperature,
`identify`).

Checked against your versions (zigbee-herdsman-converters 26.125.0, Zigbee2MQTT 2.14.3): loaded with that converter
library, it exposes on/off, brightness, `color_temp` and `color_temp_startup` 153–500, colour xy and hue/saturation,
`level_config` (`on_off_transition_time`, `execute_if_off`, `on_level`, `current_level_startup`: the attributes the
lamp has), `power_on_behavior` (off/on/toggle/previous) and `identify` (marked as a sleepy device), no effect, no OTA.

**Install (Zigbee2MQTT 2.x, checked in 2.14.3):**
1. Copy `xiao_mg24_rgb_lamp.mjs` into the folder `external_converters` inside the Zigbee2MQTT data folder (the folder
   that holds Z2M's `configuration.yaml`; create `external_converters` if it does not exist). With the Home
   Assistant add-on that data folder is usually `/config/zigbee2mqtt` or `/addon_configs/45df7312_zigbee2mqtt`.
   Nothing to add to `configuration.yaml`: Z2M 2.x loads every `.mjs`, `.js` or `.cjs` file in that folder. The file
   is an ES module (`export default`), which is what the `.mjs` extension tells Node.js. Z2M itself creates a
   `node_modules` link in that folder (so the `import` works from outside its install folder): leave it.
2. **Restart Zigbee2MQTT** (the folder is read only at start). The log shows
   `Loaded external converter 'xiao_mg24_rgb_lamp.mjs'.`, followed by Z2M's standard warning that external converters
   are meant for testing. If the file is invalid, the log says `Invalid external converter ... was ignored` and Z2M
   renames it to `xiao_mg24_rgb_lamp.mjs.invalid`: send me that log line.
3. If the lamp was paired before the converter: on its Z2M page, click **Reconfigure** (or remove and pair again).
   The bench build polls every 250 ms (§7), so Reconfigure reaches it at any time.

**Without the converter:** Z2M 2.x builds an automatic ("generated") definition from the clusters it finds during
the interview. For this lamp it should give on/off, brightness, colour and colour temperature, and the lamp should
already work from Home Assistant, but it is listed as "not supported", has a generic name, and may lack the
power-on behaviour / start-up settings and identify. If Z2M chose *enhanced* hue commands, the hue/saturation colour
would be refused by the lamp (the xy colour that HA normally sends still works); the converter forces normal hue.
This is what I expect from Z2M's documented behaviour, not something tested: tell me what the *About* and *Exposes*
tabs show without and with the converter (`bench-test.md` §3.2).

## 7. Response time (sleepy end device)

A sleepy end device only receives a command when it asks its parent ("polls"). The parent keeps a pending command
about 7.7 s.

* **Bench build** (`APP_CONSOLE_KEEP_AWAKE = 1`, USB power, this Phase 3 build): the lamp polls every **250 ms**
  all the time (`APP_POLL_SHORT_MS`, forced short poll). A command from HA should show within about 0.25–0.5 s.
* **Battery build** (Phase 5, `APP_CONSOLE_KEEP_AWAKE = 0`): long poll of the profile (`APP_POLL_LONG_MS`:
  Responsive 0.25 s, **Balanced 1 s** (default), Maximum battery 5 s), with short polls for a few seconds after each
  message. Every profile stays under the parent's 7.7 s.

`zigbee status` prints the current poll interval.

## 8. Pairing / factory-reset button (D9)

A push button between **D9 (PA04)** and **GND** (wiring: `wiring.md`, "Pairing / reset button"). Internal pull-up
and input filter, no other part. Port A keeps its GPIO interrupt in EM2, so a press wakes the sleeping lamp. Code:
`firmware/src/button.c`; pin and timings in `firmware/config/app_config.h` (`APP_BUTTON_*`). The console commands
all stay.

| Action | Lamp state | What happens | LED feedback |
|---|---|---|---|
| **Short press** (released before 3 s) | no network | Pairing starts, exactly as `zigbee join` | blinks **blue** while searching (10–60 s), then **green** 2 s (joined) or **amber** 2 s (failed: open permit join and press again) |
| Short press | joined | Nothing changes: a short press **never leaves** a network (an accidental press must not unpair the lamp) | **green** 2 s ("I am paired") |
| Short press | joining, or joined with its parent lost (rejoining) | Ignored | none |
| **Hold 3 s** | any | Warning: factory reset in 7 s | blinks **red**. **Let go to cancel**: nothing happens, the lamp shows its state again |
| **Hold 10 s** | any | **Factory reset**, exactly as `zigbee reset` (§3): steering stopped, network left, bindings, reporting, groups, scenes, attributes and saved last state cleared; balance and fade kept | solid **red** 2 s, then off (factory state). Release the button; it does nothing until released |

* To move the lamp to another network: hold 10 s, then (permit join open) short press.
* A button already held (or shorted) at boot is ignored until released: it cannot trigger a reset at power-up. It is
  **not** a recovery escape: that stays **D1** to GND at reset (`flashing.md` §5).
* The feedback colours are never written to the ZCL attributes nor to NVM3: when the feedback ends the lamp shows
  its Zigbee state again (including a running Identify). Zigbee commands received during the feedback are applied
  and saved as usual, and shown when it ends.
* The reset writes nothing new: it uses the stack's leave and table functions and deletes the application NVM3 object
  `0x0A101`, like `zigbee reset`. No flash erase, no bootloader, token, lock or Secure Engine access.
* Power: no current at rest (the pull-up only conducts while pressed, about 80 µA). While the button is held, the
  firmware wakes every 100 ms to time the hold; nothing when released.

## 9. Troubleshooting

| Symptom | Check |
|---|---|
| `Join failed` | Permit join open in Z2M (it closes after a few minutes)? Lamp within a few metres of the coordinator or a router for the first test. `zigbee status` must say `no network` before `zigbee join` (else `zigbee reset` first). Try again; then H6 (§5). |
| Short press does nothing | The lamp is joining or rejoining (wait), or the button is not reaching D9/GND: `wiring.md` continuity check. The console prints `Button: …` on every press |
| `already joined` on `zigbee join` | It is still on a (maybe old) network: `zigbee reset`, then pair again. |
| Joined, but Z2M shows no interview / "interview failed" | Sleepy devices interview slowly: wait 1 min. On the lamp's Z2M page: *Interview* again, then *Reconfigure*. |
| Z2M shows "not supported" | The converter is not loaded (§6): check the file is in `external_converters`, restart Z2M, *Reconfigure*. |
| Commands arrive late or are lost | `zigbee status`: poll 250 ms? parent link LQI/RSSI (below about LQI 100 / RSSI −85 dBm the link is weak)? Move the lamp closer or add a router, then H6. |
| The lamp ignores Z2M after a coordinator restart | Wait up to a minute: it rejoins every 10 s. `zigbee status` shows `joined, parent lost (rejoining)` meanwhile. Report it if it stays that way. |
| Colours look wrong (white tinted) | That is the balance: `lamp white` then `lamp balance r g b` (bench-test Phase 2 §2.3). Blue is the weakest channel. |
| `parent link: no message received yet` | Normal until Z2M sends something; toggle the lamp once from Z2M. |
| The board stops responding | `flashing.md` §5 (escape pin D1, then restore the backup). |

## 10. Home Assistant automation examples

Replace `light.xiao_mg24_rgb_lamp` with the entity id HA shows for the lamp (it follows the Z2M friendly name).

**Warm light at sunset, off at 23:30**

```yaml
alias: Lamp warm at sunset
triggers:
  - trigger: sun
    event: sunset
    offset: "-00:15:00"
  - trigger: time
    at: "23:30:00"
actions:
  - choose:
      - conditions:
          - condition: time
            after: "23:00:00"
        sequence:
          - action: light.turn_off
            target:
              entity_id: light.xiao_mg24_rgb_lamp
            data:
              transition: 5
    default:
      - action: light.turn_on
        target:
          entity_id: light.xiao_mg24_rgb_lamp
        data:
          color_temp_kelvin: 2700
          brightness_pct: 60
          transition: 3
mode: single
```

**Red alert for two minutes when a door opens at night, then back to off**

```yaml
alias: Lamp red on door at night
triggers:
  - trigger: state
    entity_id: binary_sensor.front_door_contact
    to: "on"
conditions:
  - condition: time
    after: "22:00:00"
    before: "06:30:00"
actions:
  - action: light.turn_on
    target:
      entity_id: light.xiao_mg24_rgb_lamp
    data:
      rgb_color: [255, 0, 0]
      brightness_pct: 40
  - delay: "00:02:00"
  - action: light.turn_off
    target:
      entity_id: light.xiao_mg24_rgb_lamp
mode: restart
```

(`binary_sensor.front_door_contact` is an example: any door or motion sensor works. Older HA versions write
`trigger:` / `platform:` / `service:` instead of `triggers:` / `trigger:` / `action:`; the automation editor
converts it.)
