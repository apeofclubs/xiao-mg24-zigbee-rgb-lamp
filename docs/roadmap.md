# Roadmap

Each phase starts only when the previous one's **gate** is met: the hardware results listed under "Needs from you",
reported on the ticket. Validation levels are written as **written / compiled / flashed / physically tested /
verified in Zigbee2MQTT**. Agents can reach *compiled* at most. Everything above that comes from your tests.

## Phase 1: hardware and SDK validation (this branch)

Delivered: `feasibility.md`, `framework.md`, `pinout.md`, `wiring.md`, `hardware-assumptions.md`, `flashing.md`, this
roadmap, and a firmware skeleton that builds from the command line (`scripts/build.sh`). The skeleton is a sleepy end
device stack with Basic/Identify, a 10 s awake window, the D1 escape, the LED held off, and no auto-join.

Status: **compiled**.

**Needs from you before Phase 2:**
1. H1, H2, H3, H8 answered (cell protection, charge current, LED polarity and Vf, battery polarity).
2. H12, H13 (board variant, Zigbee parent).
3. Optional but useful: the backup from `flashing.md` §2 and the H7 check, then the Phase 1 flash and the console
   banner (§4). This proves the flashing path before any real feature depends on it.

## Phase 2: local RGB (no Zigbee control yet)

* TIMER0 CC0..CC2 PWM at 1 kHz on D0/D2/D3, polarity from `APP_LED_COMMON_CATHODE`; pins at their OFF level at boot
  and whenever a channel is 0 (`src/rgb_led.c`).
* Gamma 2.2 brightness 0–100 %, per-channel balance saved in NVM3 (application key `0x0A100`), fades and two effects
  on an application event (`src/lamp.c`).
* EM1 requirement held while any channel is lit, released when all are off.
* Bench console over SEGGER RTT through the on-board CMSIS-DAP (`src/lamp_cli.c`, `flashing.md` §4.1/§6.5): `lamp rgb`,
  `hsv`, `level`, `off`, `white`, `balance`, `preset`, `fade`, `effect`, `status` (`bench-test.md`). The bench build
  holds EM1 all the time (`APP_CONSOLE_KEEP_AWAKE = 1`), on USB power only.

Status: **compiled** (AUR-417).

**Needs from you before Phase 3:** LED wired per `wiring.md`; for each of red, green, blue, white at 100 %: the current
per channel (multimeter in series, mA) and whether colours and brightness look right; confirmation the lamp switches
off fully.

## Phase 3: Zigbee light

* Endpoint 1 is an Extended Color Light (0x010D): On/Off (0x0006), Level Control (0x0008, transitions and "with
  on/off"), Color Control (0x0300: hue/saturation, XY, colour temperature 153–500 mireds; `ColorCapabilities`
  0x0019, no enhanced hue or colour loop because the SDK server does not implement them), Groups, Scenes, Identify
  (LED blinks); manufacturer `OpenSource`, model `XIAO-MG24-RGB-LiPo` (`config/zcl/zcl_config.zap`).
* The SDK cluster servers drive the Phase 2 lamp through one path: attributes → `lamp_apply()` → gamma, balance, PWM
  (`src/zigbee_light.c`, conversions in `src/color.c`).
* Pairing on demand only, from the console: `zigbee join | leave | status | reset | rfswitch` (`src/zigbee_cli.c`).
  No button wired; an optional one on D9 is proposed (`zigbee.md` §8).
* Factory reset (`zigbee reset`): leave, bindings, reporting, groups, scenes, attributes, saved last state; the
  balance (`0x0A100`) is kept. No flash erase.
* Last state in NVM3 (`0x0A101`) with `StartUpOnOff`, `StartUpCurrentLevel`, `StartUpColorTemperatureMireds`; written
  only after 5 s of stable state, at most once per 30 s (worst case 120 writes/hour, `zigbee.md` §4).
* Bench build keeps the RTT console and EM1; forced short poll 250 ms (a command arrives within 250 ms); the
  profiles' long poll (0.25 / 1 / 5 s) is for Phase 5. Rejoin attempts never stop after a parent loss.
* H6 bench aid `zigbee rfswitch reset|0|1` (not saved). Zigbee2MQTT 2.x external converter
  `zigbee2mqtt/xiao_mg24_rgb_lamp.mjs`. Docs: `zigbee.md`, `bench-test.md` §3; measured LED currents in `wiring.md`.

Status: **compiled** (AUR-437).

**Needs from you before Phase 4:** the Z2M "About" / "Exposes" tabs for the lamp (screenshot or text); ON/OFF,
brightness and colour working from HA (yes/no each, with the delay you notice); behaviour after a lamp reset and a
coordinator restart; H6 (LQI with the two RF-switch states); H10 (reaction time, Z2M groups).

## Phase 4: battery

* IADC reading of PD04 with PD03 switched only during the measurement (after H4), with a calibration constant.
* LiPo voltage-to-percentage table; Power Configuration (0x0001): `BatteryVoltage` (0x0020),
  `BatteryPercentageRemaining` (0x0021), low-battery alarm/flag; configurable reporting interval.
* Low battery (about 3.5 V, to validate): report it, cap brightness, stop effects. Critical (about 3.3 V): LED off,
  long sleep, no restart loops.

**Needs from you before Phase 5:** battery voltage from a multimeter vs. the value in Z2M at three charge levels; the
H4 and H5 tests; behaviour while charging (red CHARGE LED, readings); the cell temperature after a full charge.

## Phase 5: power optimisation

* Poll profiles (Responsive / Balanced / Maximum battery) with fast polling while lit; reporting intervals; logs
  switched off in the production build (`APP_DEBUG_LOG 0`); unused peripherals off.
* Measured current replaces the estimates in `feasibility.md` §3.

**Needs from you before Phase 6:** current measurements (a µCurrent/Nordic PPK2/Joulescope-class meter in the battery
lead, or a multimeter on its µA range for the average) in: LED off idle, LED on white 100 %, during a command; the
command delay in each profile; the lamp still working after 24 h of sleep/wake cycles.

## Phase 6: release

* Complete docs (README, troubleshooting, test plan and validation checklist), reproducible build in CI (DevOps
  engineer), a tagged release with the `.hex` built by CI from the tagged source.
* Pairing/reconnect tests repeated on the release image.

**Needs from you:** the final checklist ticked on your hardware, and the go to publish.
