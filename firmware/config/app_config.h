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
#define APP_FIRMWARE_VERSION_STRING  "0.1.0-phase1"

/* ---------------------------------------------------------------------------
 * RGB LED (conventional 4-pin LED, one resistor per channel)
 * Pads D0 / D2 / D3 (PC00 / PC02 / PC03). Driven by TIMER0 CC0..CC2 from
 * Phase 2 on; in Phase 1 the pins are only set to their "off" level.
 * ------------------------------------------------------------------------- */
#define APP_LED_RED_PORT             gpioPortC   /* pad D0 */
#define APP_LED_RED_PIN              0
#define APP_LED_GREEN_PORT           gpioPortC   /* pad D2 */
#define APP_LED_GREEN_PIN            2
#define APP_LED_BLUE_PORT            gpioPortC   /* pad D3 */
#define APP_LED_BLUE_PIN             3

/* 1 = common cathode (shared pin to GND, a channel is ON when its GPIO is HIGH).
 * 0 = common anode   (shared pin to 3V3, a channel is ON when its GPIO is LOW).
 * Must match the LED actually wired: see docs/hardware-assumptions.md (A1). */
#define APP_LED_COMMON_CATHODE       1

/* Upper limit applied to every PWM duty cycle, in percent (battery life). */
#define APP_LED_MAX_DUTY_PERCENT     100
#define APP_LED_PWM_FREQUENCY_HZ     1000

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
 * Pins the firmware must never drive (see docs/pinout.md)
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
