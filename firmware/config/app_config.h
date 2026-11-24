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
#define APP_FIRMWARE_VERSION_STRING  "0.3.0-phase3"   /* also Basic SWBuildID in the .zap */

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
 * Defaults from the operator's bench (2026-11-23): with red 100 ohm, green
 * 220 ohm and blue 100 ohm, 100/100/100 gives a good white. Re-tune with
 * `lamp white` and `lamp balance`; the tuned values are kept in NVM3 and win
 * over these defaults. */
#define APP_LED_BALANCE_RED_DEFAULT    100u
#define APP_LED_BALANCE_GREEN_DEFAULT  100u
#define APP_LED_BALANCE_BLUE_DEFAULT   100u

/* Colour transitions: default fade time and the tick that drives fades and
 * effects (a Zigbee application-framework event, so it wakes the main loop). */
#define APP_LED_FADE_DEFAULT_MS      400u
#define APP_LED_FADE_MAX_MS          10000u
#define APP_LED_TICK_MS              20u
#define APP_EFFECT_RAINBOW_PERIOD_MS 12000u   /* one full hue turn */
#define APP_EFFECT_BREATHE_PERIOD_MS 4000u    /* dim -> bright -> dim */
#define APP_EFFECT_BREATHE_MIN_PERCENT 8u     /* lowest point of "breathe" */
#define APP_EFFECT_BLINK_PERIOD_MS   1000u    /* Identify: 0.5 s on, 0.5 s off */

/* Zigbee control. The SDK On/Off, Level Control and Color Control servers run
 * the ZCL transitions themselves (attribute steps every 100 ms or less); the
 * lamp follows each step with this short fade so the steps do not show. */
#define APP_ZIGBEE_FOLLOW_FADE_MS    100u

/* Colour temperature range announced in ColorTempPhysicalMin/Max (.zap):
 * 153 mireds = 6500 K, 500 mireds = 2000 K. Rendered with the RGB LED (no white
 * channel), so it is an approximation, tuned by the balance. */
#define APP_COLOR_TEMP_MIN_MIREDS    153u
#define APP_COLOR_TEMP_MAX_MIREDS    500u

/* ---------------------------------------------------------------------------
 * Persistent settings: default NVM3 instance, application (user) key domain
 * 0x00000-0x0FFFF only. The Zigbee stack's objects live in 0x10000-0x1FFFF
 * (NVM3KEY_DOMAIN_ZIGBEE, SDK zigbee/stack/config/token-stack.h) and are never
 * read or written by the application.
 * ------------------------------------------------------------------------- */
#define APP_NVM3_KEY_LAMP_SETTINGS   0x0A100u   /* balance, fade: kept by `zigbee reset` */
#define APP_NVM3_KEY_LIGHT_STATE     0x0A101u   /* last on/off, level, colour: erased by `zigbee reset` */

/* Last-state writes: only once the light state has not changed for
 * APP_STATE_SAVE_DELAY_MS, never more often than APP_STATE_SAVE_MIN_INTERVAL_MS,
 * and only if it differs from what is stored. Worst case 120 writes per hour
 * (docs/zigbee.md "Flash wear"). */
#define APP_STATE_SAVE_DELAY_MS        5000u
#define APP_STATE_SAVE_MIN_INTERVAL_MS 30000u

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
 * On/off and pairing button (operator, 2026-11-24, AUR-462; replaces AUR-459)
 * Push button between pad D9 (PA04, schematic net PA04/A9/D9/MISO) and GND,
 * internal pull-up with the input glitch filter, no external part. Port A
 * keeps its GPIO interrupts in EM2, so a press wakes the sleeping lamp.
 * PA04 is not a debug, flash or USB-bridge pin (docs/pinout.md).
 *   short press (released before APP_BUTTON_PAIR_MS): toggle the light, as a
 *       ZCL On/Off Toggle command
 *   held APP_BUTTON_PAIR_MS: pairing (leaves the current network first if
 *       joined); the LED breathes until the join ends
 * No factory reset on the button: `zigbee reset` on the console.
 * A button already held at boot is ignored until it is released.
 * ------------------------------------------------------------------------- */
#define APP_BUTTON_PORT              gpioPortA   /* pad D9 */
#define APP_BUTTON_PIN               4
#define APP_BUTTON_DEBOUNCE_MS       30u     /* level read this long after the last edge */
#define APP_BUTTON_TICK_MS           100u    /* hold timing while pressed */
#define APP_BUTTON_PAIR_MS           5000u   /* hold this long to pair; shorter = toggle */
#define APP_BUTTON_FEEDBACK_MS       2000u   /* green / amber after a pairing */

/* Re-pairing a joined lamp: the leave is asynchronous; steering starts once
 * the stack reports no network, checked every APP_PAIR_LEAVE_POLL_MS, and
 * the pairing fails if that takes longer than APP_PAIR_LEAVE_TIMEOUT_MS. */
#define APP_PAIR_LEAVE_POLL_MS       200u
#define APP_PAIR_LEAVE_TIMEOUT_MS    10000u

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
 *   PA07  user LED; PD05, PC08, PC09 Sense only
 *   PB04 / PB05 RF switch: left in their reset state, except by the bench
 *   command `zigbee rfswitch` below (not saved, back to reset state on reboot).
 * ------------------------------------------------------------------------- */

/* RF switch U9 (FM8625H) between the radio, the chip antenna and the U.FL
 * connector (Seeed wiki pin map: PB04 "RF Switch Port Select", PB05 "RF Switch
 * Power"; schematic v1.1 sheet 05: select "0: RF1, 1: RF2"). Which of RF1/RF2
 * is the chip antenna is not stated in any source we have: item H6, measured
 * with `zigbee rfswitch` (docs/zigbee.md). */
#define APP_RF_SWITCH_SELECT_PORT    gpioPortB
#define APP_RF_SWITCH_SELECT_PIN     4
#define APP_RF_SWITCH_POWER_PORT     gpioPortB
#define APP_RF_SWITCH_POWER_PIN      5

/* ---------------------------------------------------------------------------
 * Polling. A sleepy end device only receives a command when it polls its
 * parent; the parent keeps a pending command about 7.7 s, so every poll
 * interval below stays under 7 s.
 *   Bench build (APP_CONSOLE_KEEP_AWAKE = 1): the device stays in short-poll
 *   mode all the time: a command arrives within APP_POLL_SHORT_MS.
 *   Battery build (APP_CONSOLE_KEEP_AWAKE = 0, tuned in Phase 5): short poll
 *   for a few seconds after each message (the SDK end-device-support wake
 *   timeout), then the profile's long poll interval APP_POLL_LONG_MS.
 * ------------------------------------------------------------------------- */
#define APP_POLL_SHORT_MS            250u

#define APP_POWER_PROFILE_RESPONSIVE 0
#define APP_POWER_PROFILE_BALANCED   1
#define APP_POWER_PROFILE_MAX_BATTERY 2
#define APP_POWER_PROFILE            APP_POWER_PROFILE_BALANCED

/* Long poll per profile, as in docs/feasibility.md section 3. */
#if APP_POWER_PROFILE == APP_POWER_PROFILE_RESPONSIVE
#define APP_POLL_LONG_MS             250u
#elif APP_POWER_PROFILE == APP_POWER_PROFILE_BALANCED
#define APP_POLL_LONG_MS             1000u
#else
#define APP_POLL_LONG_MS             5000u
#endif

#define APP_DEBUG_LOG                1   /* 0 for production low-power builds */

#endif /* APP_CONFIG_H */
