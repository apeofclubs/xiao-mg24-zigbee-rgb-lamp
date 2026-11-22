/**
 * @file rgb_led.c
 * @brief Phase 1: pins only. The LED is held OFF; no PWM yet.
 *
 * Phase 2 routes TIMER0 CC0..CC2 to these pins. TIMER0 does not run in EM2,
 * so the power manager must hold EM1 while any channel is lit
 * (docs/feasibility.md, "PWM and sleep").
 */
#include "rgb_led.h"
#include "app_config.h"
#include "em_cmu.h"
#include "em_gpio.h"

/* Logic level that turns a channel OFF for the configured LED polarity. */
#define LED_OFF_LEVEL  (APP_LED_COMMON_CATHODE ? 0u : 1u)

void rgb_led_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  GPIO_PinModeSet(APP_LED_RED_PORT, APP_LED_RED_PIN, gpioModePushPull, LED_OFF_LEVEL);
  GPIO_PinModeSet(APP_LED_GREEN_PORT, APP_LED_GREEN_PIN, gpioModePushPull, LED_OFF_LEVEL);
  GPIO_PinModeSet(APP_LED_BLUE_PORT, APP_LED_BLUE_PIN, gpioModePushPull, LED_OFF_LEVEL);
}

void rgb_led_set(uint8_t red, uint8_t green, uint8_t blue)
{
  /* Phase 2. Until then the LED stays off whatever is asked. */
  (void)red;
  (void)green;
  (void)blue;
  rgb_led_off();
}

void rgb_led_off(void)
{
  if (LED_OFF_LEVEL) {
    GPIO_PinOutSet(APP_LED_RED_PORT, APP_LED_RED_PIN);
    GPIO_PinOutSet(APP_LED_GREEN_PORT, APP_LED_GREEN_PIN);
    GPIO_PinOutSet(APP_LED_BLUE_PORT, APP_LED_BLUE_PIN);
  } else {
    GPIO_PinOutClear(APP_LED_RED_PORT, APP_LED_RED_PIN);
    GPIO_PinOutClear(APP_LED_GREEN_PORT, APP_LED_GREEN_PIN);
    GPIO_PinOutClear(APP_LED_BLUE_PORT, APP_LED_BLUE_PIN);
  }
}
