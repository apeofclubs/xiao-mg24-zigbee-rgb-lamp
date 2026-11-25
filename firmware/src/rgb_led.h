/**
 * @file rgb_led.h
 * @brief Low-level RGB LED output: TIMER0 PWM on the three LED pins.
 *
 * Colour, gamma, balance, fades and effects are in lamp.c; this module only
 * turns three linear duty values into PWM and manages the energy mode.
 */
#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>

#define RGB_LED_CHANNELS   3u
#define RGB_LED_DUTY_FULL  65535u   /* duty scale used by rgb_led_write() */

/** Configure the three LED pins as push-pull outputs at their OFF level. */
void rgb_led_init(void);

/**
 * Set the three channels (red, green, blue), each 0..RGB_LED_DUTY_FULL
 * (linear PWM duty). A channel at 0 is disconnected from the timer and held at
 * its OFF level. While any channel is above 0 TIMER0 runs and an EM1
 * requirement is held (TIMER0 stops in EM2); when all are 0 the timer stops and
 * the requirement is released.
 */
void rgb_led_write(const uint16_t duty[RGB_LED_CHANNELS]);

/** All channels off (pins at their OFF level, timer stopped, EM1 released). */
void rgb_led_off(void);

/** Current duty of a channel in timer counts, and the timer period (TOP + 1). */
uint32_t rgb_led_duty_counts(uint8_t channel);
uint32_t rgb_led_period_counts(void);

#endif
