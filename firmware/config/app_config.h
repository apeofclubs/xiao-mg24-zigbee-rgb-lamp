/**
 * @file app_config.h
 * @brief Hardware mapping and tunables for the XIAO MG24 RGB lamp.
 *
 * Every pin and threshold lives here, apart from the logic. Sources for each
 * pin: Seeed XIAO MG24 schematic v1.1 (XIAO_MGM240S_KICAD_Prj.pdf) and the
 * Silicon Labs Arduino core variant file variants/xiao_mg24/arduino_variant.cpp.
 * See ../../docs/pinout.md.
 */
#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "em_gpio.h"

/* ---------------------------------------------------------------------------
 * Identity (Basic cluster values are set in config/zcl/zcl_config.zap)
 * ------------------------------------------------------------------------- */
#define APP_MANUFACTURER_NAME        "OpenSource"
#define APP_MODEL_IDENTIFIER         "XIAO-MG24-RGB-LiPo"
#define APP_ENDPOINT                 1
#define APP_FIRMWARE_VERSION_STRING  "0.2.0-phase2"

/* ---------------------------------------------------------------------------
 * RGB LED (conventional 4-pin LED, one resistor per channel)
 * Pads D0 / D2 / D3 (PC00 / PC02 / PC03), driven by TIMER0 CC0..CC2 in PWM
 * mode. TIMER0 can reach port C: the SDK pin-tool data for this part lists
 * locations 32..41 (PC00..PC09) for TIMER0 CC0/CC1/CC2 (item H9).
 * ------------------------------------------------------------------------- */
#define APP_LED_RED_PORT             gpioPortC   /* pad D0 */
#define APP_LED_RED_PIN              0
#define APP_LED_GREEN_PORT           gpioPortC   /* pad D2 */
#define APP_LED_GREEN_PIN            2
#define APP_LED_BLUE_PORT            gpioPortC   /* pad D3 */
#define APP_LED_BLUE_PIN             3

/* 1 = common cathode (shared pin to GND, a channel is ON when its GPIO is HIGH).
 * 0 = common anode   (shared pin to 3V3, a channel is ON when its GPIO is LOW).
 * Must match the LED actually wired. H3 confirmed by the operator on
 * 2026-11-22: common (long) leg on GND, so common cathode. */
#define APP_LED_COMMON_CATHODE       1

/* Upper limit applied to every PWM duty cycle, in percent (battery life). */
#define APP_LED_MAX_DUTY_PERCENT     100
#define APP_LED_PWM_FREQUENCY_HZ     1000

/* Per-channel balance, percent of full PWM for that channel (0..100).
 * Defaults estimated from the operator's bench test (no firmware, 3V3 through
 * the resistors): red 220 ohm (about 6 mA) looked the dimmest, then blue, and
 * green 330 ohm (about 1-1.5 mA) the brightest. Starting point only: tune with
 * `lamp white` and `lamp balance`; the tuned values are kept in NVM3. */
#define APP_LED_BALANCE_RED_DEFAULT    100u
#define APP_LED_BALANCE_GREEN_DEFAULT  40u
#define APP_LED_BALANCE_BLUE_DEFAULT   70u

/* Colour transitions: default fade time and the tick that drives fades and
 * effects (a Zigbee application-framework event, so it wakes the main loop). */
#define APP_LED_FADE_DEFAULT_MS      400u
#define APP_LED_FADE_MAX_MS          10000u
#define APP_LED_TICK_MS              20u
#define APP_EFFECT_RAINBOW_PERIOD_MS 12000u   /* one full hue turn */
#define APP_EFFECT_BREATHE_PERIOD_MS 4000u    /* dim -> bright -> dim */
#define APP_EFFECT_BREATHE_MIN_PERCENT 8u     /* lowest point of "breathe" */

/* ---------------------------------------------------------------------------
 * Persistent settings: default NVM3 instance, application (user) key domain
 * 0x00000-0x0FFFF only. The Zigbee stack's objects live in 0x10000-0x1FFFF
 * (NVM3KEY_DOMAIN_ZIGBEE, SDK zigbee/stack/config/token-stack.h) and are never
 * read or written by the application.
 * ------------------------------------------------------------------------- */
#define APP_NVM3_KEY_LAMP_SETTINGS   0x0A100u

/* ---------------------------------------------------------------------------
 * Bench console over SEGGER RTT (read by OpenOCD through the on-board
 * CMSIS-DAP, docs/flashing.md section 6.5). RTT has no receive interrupt, so
 * the CLI is polled: an application event wakes the main loop every
 * APP_CONSOLE_POLL_MS. APP_CONSOLE_KEEP_AWAKE = 1 also holds EM1 forever so the
 * probe can read the RTT buffers at any time (bench builds on USB power only;
 * Phase 5 sets it to 0 for battery builds).
 * ------------------------------------------------------------------------- */
#define APP_CONSOLE_KEEP_AWAKE       1
#define APP_CONSOLE_POLL_MS          50u

/* ---------------------------------------------------------------------------
 * Recovery: boot window and escape pin (docs/flashing.md, safe-flashing rules)
 * ------------------------------------------------------------------------- */
/* After every reset the chip stays in EM0/EM1 (no EM2, never EM4) this long,
 * so the on-board debugger can always attach. */
#define APP_BOOT_AWAKE_WINDOW_MS     10000u

/* Escape pin: pad D1 (PC01), internal pull-up. If it reads LOW at boot
 * (pad shorted to GND), the firmware never sleeps until the next reset.
 * Same pad as the Arduino core's DEEP_SLEEP_ESCAPE_PIN. */
#define APP_ESCAPE_PORT              gpioPortC
#define APP_ESCAPE_PIN               1

/* ---------------------------------------------------------------------------
 * Battery measurement (Phase 4). Schematic nets PD03/VBAT_CTL and
 * PD04/VBAT_ADC. Phase 1 does not touch these pins.
 * ------------------------------------------------------------------------- */
#define APP_VBAT_CTL_PORT            gpioPortD   /* HIGH enables the divider (Seeed wiki sketch) */
#define APP_VBAT_CTL_PIN             3
#define APP_VBAT_ADC_PORT            gpioPortD
#define APP_VBAT_ADC_PIN             4
#define APP_VBAT_DIVIDER_RATIO       2.0f        /* Seeed wiki formula raw * (2 * 3.3 / 4095) */

/* Starting points for investigation, not validated limits (brief §6). */
#define APP_BATTERY_LOW_MV           3500u
#define APP_BATTERY_CRITICAL_MV      3300u
#define APP_BATTERY_REPORT_INTERVAL_S 3600u

/* ---------------------------------------------------------------------------
 * Pins the firmware must never drive (see docs/pinout.md and the Seeed wiki)
 *   PA01 / PA02  SWCLK / SWDIO (on-board SAMD11 debugger)
 *   PA08 / PA09  USART to the SAMD11 (console)
 *   PA06, PA00, PB00, PB01  external SPI flash (CS, SCK, MOSI, MISO)
 *   PA07  user LED; PB04 / PB05 RF switch select / power; PD05, PC08, PC09 Sense only
 * ------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Power profiles (Phase 5). Short poll interval in ms.
 * ------------------------------------------------------------------------- */
#define APP_POWER_PROFILE_RESPONSIVE 0
#define APP_POWER_PROFILE_BALANCED   1
#define APP_POWER_PROFILE_MAX_BATTERY 2
#define APP_POWER_PROFILE            APP_POWER_PROFILE_BALANCED

#define APP_DEBUG_LOG                1   /* 0 for production low-power builds */

#endif /* APP_CONFIG_H */
