# xiao-mg24-zigbee-rgb-lamp

Open-source Zigbee RGB battery lamp firmware for the Seeed Studio XIAO MG24 (EFR32MG24), for Home Assistant through
Zigbee2MQTT. No Wi-Fi, no cloud.

The target device: a small rechargeable decorative lamp. It has one conventional RGB LED with PWM dimming, Zigbee
ON/OFF, brightness and colour, battery voltage and percentage reporting, and USB-C charging through the XIAO's own
charger. It runs as a Zigbee **sleepy end device** for low standby consumption.

## Status

**Phase 1 of 6: hardware plan and buildable skeleton.** The firmware **compiles** (Simplicity SDK 2026.6.1). It has
**not been flashed or tested on hardware yet.** It does not drive the LED and does not join Zigbee by itself. See the
[roadmap](docs/roadmap.md).

| Feature | Implemented | Compiled | Tested on hardware | Verified in Zigbee2MQTT |
|---|---|---|---|---|
| Build from the command line | yes | yes | – | – |
| Recovery window + D1 escape | yes | yes | no | – |
| RGB PWM | no (Phase 2) | | | |
| Zigbee light (On/Off, Level, Color) | no (Phase 3) | | | |
| Battery reporting | no (Phase 4) | | | |
| Power profiles | no (Phase 5) | | | |

## Documentation

* [Feasibility](docs/feasibility.md): sleepy end device vs. PWM, latency, charger, battery measurement
* [Framework choice](docs/framework.md): Simplicity SDK vs. Arduino core
* [Pinout](docs/pinout.md): pins used, pins to avoid
* [Wiring](docs/wiring.md): LED, resistors, battery, USB-C
* [Hardware assumptions to confirm](docs/hardware-assumptions.md)
* [Flashing, backup and recovery](docs/flashing.md): read before flashing
* [Roadmap](docs/roadmap.md)

## Build

```sh
scripts/build.sh
```

Needs Simplicity SDK 2026.6.1, slc-cli 6.0.23 and the Arm GNU Toolchain 14.2.Rel1. Details are in
[CLAUDE.md](CLAUDE.md#build-and-check).

## Recovery in one line

If the board stops answering, connect pad **D1 to GND** and press RESET: the firmware then never sleeps and can be
reflashed. Full procedure: [docs/flashing.md](docs/flashing.md#5-if-the-board-no-longer-responds).

## Licence

MIT, see [LICENSE](LICENSE). Third-party components: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
