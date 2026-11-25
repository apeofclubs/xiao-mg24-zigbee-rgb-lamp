/**
 * @file rgb_led.c
 * @brief TIMER0 CC0..CC2 PWM on D0 / D2 / D3 (PC00 / PC02 / PC03).
 *
 * - One timer for the three channels: same frequency, in phase.
 * - Polarity from APP_LED_COMMON_CATHODE: for a common anode LED the compare
 *   outputs are inverted and the idle (OFF) pin level is HIGH.
 * - A channel at 0 is unrouted from the timer, so its pin is plain GPIO at the
 *   OFF level: no glitch, the LED is fully off.
 * - TIMER0 is a high-frequency peripheral and stops in EM2, so an EM1
 *   requirement is held while any channel is lit (docs/feasibility.md section 2).
 *   The timer is fully re-initialised each time the LED goes from off to on.
 * - EM4 is never used.
 */
#include "rgb_led.h"
#include "app_config.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "sl_power_manager.h"

#define LED_TIMER      TIMER0
#define LED_TIMER_IDX  0   /* GPIO->TIMERROUTE[0] is TIMER0 */

/* Logic level that turns a channel OFF for the configured LED polarity. */
#define LED_OFF_LEVEL  (APP_LED_COMMON_CATHODE ? 0u : 1u)

typedef struct {
  GPIO_Port_TypeDef port;
  uint8_t pin;
} led_pin_t;

static const led_pin_t led_pins[RGB_LED_CHANNELS] = {
  { APP_LED_RED_PORT, APP_LED_RED_PIN },
  { APP_LED_GREEN_PORT, APP_LED_GREEN_PIN },
  { APP_LED_BLUE_PORT, APP_LED_BLUE_PIN },
};

static const uint32_t route_enable_bit[RGB_LED_CHANNELS] = {
  GPIO_TIMER_ROUTEEN_CC0PEN,
  GPIO_TIMER_ROUTEEN_CC1PEN,
  GPIO_TIMER_ROUTEEN_CC2PEN,
};

static bool running;           /* timer running and EM1 requirement held */
static uint32_t top;           /* timer TOP: period = top + 1 counts */
static uint32_t duty_counts[RGB_LED_CHANNELS];

static void pins_to_off_level(void)
{
  for (uint8_t i = 0; i < RGB_LED_CHANNELS; i++) {
    GPIO_PinModeSet(led_pins[i].port, led_pins[i].pin, gpioModePushPull, LED_OFF_LEVEL);
  }
}

static void timer_start(void)
{
  CMU_ClockEnable(cmuClock_TIMER0, true);

  uint32_t freq = CMU_ClockFreqGet(cmuClock_TIMER0);
  top = (freq / APP_LED_PWM_FREQUENCY_HZ) - 1u;

  TIMER_InitCC_TypeDef cc = TIMER_INITCC_DEFAULT;
  cc.mode = timerCCModePWM;
  cc.outInvert = (APP_LED_COMMON_CATHODE ? false : true);
  for (uint8_t i = 0; i < RGB_LED_CHANNELS; i++) {
    TIMER_InitCC(LED_TIMER, i, &cc);
  }

  /* Pin locations; outputs are enabled per channel in rgb_led_write(). */
  GPIO->TIMERROUTE[LED_TIMER_IDX].ROUTEEN = 0;
  GPIO->TIMERROUTE[LED_TIMER_IDX].CC0ROUTE =
    ((uint32_t)APP_LED_RED_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
    | ((uint32_t)APP_LED_RED_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[LED_TIMER_IDX].CC1ROUTE =
    ((uint32_t)APP_LED_GREEN_PORT << _GPIO_TIMER_CC1ROUTE_PORT_SHIFT)
    | ((uint32_t)APP_LED_GREEN_PIN << _GPIO_TIMER_CC1ROUTE_PIN_SHIFT);
  GPIO->TIMERROUTE[LED_TIMER_IDX].CC2ROUTE =
    ((uint32_t)APP_LED_BLUE_PORT << _GPIO_TIMER_CC2ROUTE_PORT_SHIFT)
    | ((uint32_t)APP_LED_BLUE_PIN << _GPIO_TIMER_CC2ROUTE_PIN_SHIFT);

  TIMER_Init_TypeDef init = TIMER_INIT_DEFAULT;
  init.enable = true;
  init.prescale = timerPrescale1;
  TIMER_Init(LED_TIMER, &init);
  TIMER_TopSet(LED_TIMER, top);
  for (uint8_t i = 0; i < RGB_LED_CHANNELS; i++) {
    TIMER_CompareSet(LED_TIMER, i, 0);
  }

  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
  running = true;
}

/* Stops the counter only. The TIMER0 clock stays enabled and the timer is not
 * reset (AUR-464): after a stop no code touches TIMER0 until timer_start(),
 * which re-initialises it; an access to a peripheral whose clock is off is a
 * BusFault on Series 2. The clock costs nothing in EM2 (high-frequency
 * peripheral clocks stop there). */
static void timer_stop(void)
{
  GPIO->TIMERROUTE[LED_TIMER_IDX].ROUTEEN = 0;
  pins_to_off_level();
  TIMER_Enable(LED_TIMER, false);
  for (uint8_t i = 0; i < RGB_LED_CHANNELS; i++) {
    duty_counts[i] = 0;
  }
  if (running) {
    running = false;
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
  }
}

void rgb_led_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  pins_to_off_level();
  running = false;
}

void rgb_led_write(const uint16_t duty[RGB_LED_CHANNELS])
{
  bool any = false;
  for (uint8_t i = 0; i < RGB_LED_CHANNELS; i++) {
    if (duty[i] != 0) {
      any = true;
    }
  }
  if (!any) {
    rgb_led_off();
    return;
  }
  if (!running) {
    timer_start();
  }

  uint32_t routeen = 0;
  for (uint8_t i = 0; i < RGB_LED_CHANNELS; i++) {
    uint32_t period = top + 1u;
    uint32_t counts = (uint32_t)(((uint64_t)duty[i] * period
                                  + (RGB_LED_DUTY_FULL / 2u)) / RGB_LED_DUTY_FULL);
    if (duty[i] != 0 && counts == 0) {
      counts = 1;   /* smallest visible step rather than off */
    }
    duty_counts[i] = counts;
    if (counts != 0) {
      routeen |= route_enable_bit[i];
      /* counts == period (> TOP) never matches: output stays on (100 %). */
      TIMER_CompareBufSet(LED_TIMER, i, counts);
    }
  }
  /* Channels at 0 are unrouted: their pin goes back to GPIO at OFF level. */
  GPIO->TIMERROUTE[LED_TIMER_IDX].ROUTEEN = routeen;
}

void rgb_led_off(void)
{
  if (running) {
    timer_stop();
  } else {
    pins_to_off_level();
  }
}

uint32_t rgb_led_duty_counts(uint8_t channel)
{
  return (channel < RGB_LED_CHANNELS) ? duty_counts[channel] : 0;
}

uint32_t rgb_led_period_counts(void)
{
  return running ? (top + 1u) : 0;
}
