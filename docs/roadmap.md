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

* TIMER0 CC0..CC2 PWM at about 1 kHz on D0/D2/D3, polarity from `APP_LED_COMMON_CATHODE`.
* Gamma-corrected brightness 0–100 %, per-channel limits for colour balance, smooth transitions on a timer.
* EM1 requirement held while any channel is lit, released when all are off.
* A console command (`lamp rgb <r> <g> <b>`, `lamp level <0-100>`) for bench tests.

**Needs from you before Phase 3:** LED wired per `wiring.md`; for each of red, green, blue, white at 100 %: the current
per channel (multimeter in series, mA) and whether colours and brightness look right; confirmation the lamp switches
off fully.

## Phase 3: Zigbee RGB light

* Endpoint 1 becomes an Extended Color Light (0x010D): On/Off (0x0006), Level Control (0x0008), Color Control
  (0x0300; XY and hue/saturation), Groups, Scenes, Identify; manufacturer `OpenSource`, model `XIAO-MG24-RGB-LiPo`.
* Pairing on demand (button or console), factory reset procedure, last-state restore from NVM3, start-up behaviour
  attribute.
* Zigbee2MQTT: check whether the generic Zigbee light definition works; if not, an external converter in
  `zigbee2mqtt/`.
* Docs: pairing, factory reset, re-pairing, Z2M troubleshooting, Home Assistant automation examples.

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
