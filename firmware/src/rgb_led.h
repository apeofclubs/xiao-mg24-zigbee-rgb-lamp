/**
 * @file rgb_led.h
 * @brief RGB LED output: PWM, colour conversion, transitions (Phase 2).
 */
#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>

/** Configure the three LED pins as push-pull outputs at their OFF level. */
void rgb_led_init(void);

/** Phase 2: set the three channels, 0..255 each, before brightness scaling. */
void rgb_led_set(uint8_t red, uint8_t green, uint8_t blue);

/** All channels off. */
void rgb_led_off(void);

#endif
