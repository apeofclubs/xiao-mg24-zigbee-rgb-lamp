/**
 * @file button.c
 * @brief Pairing / factory-reset push button on pad D9 (PA04) to GND.
 *
 *   short press (released before APP_BUTTON_WARN_MS)
 *       not on a network: start pairing (same as `zigbee join`); the lamp
 *         blinks blue while it searches, then shows solid green for
 *         APP_BUTTON_FEEDBACK_MS when joined, or solid amber if the join failed
 *       already joined: solid green for APP_BUTTON_FEEDBACK_MS, nothing else
 *         (a short press never leaves a network: hold 10 s for that)
 *       joining, or joined with the parent lost (rejoining): ignored
 *   held APP_BUTTON_WARN_MS: the lamp blinks red; letting go before
 *       APP_BUTTON_FACTORY_RESET_MS cancels (no action, the lamp comes back)
 *   held APP_BUTTON_FACTORY_RESET_MS: factory reset (same as `zigbee reset`),
 *       then solid red for APP_BUTTON_FEEDBACK_MS, then off (factory state)
 *
 * The edge interrupt (GPIO driver, port A works in EM2) only wakes the main
 * loop; the level is read APP_BUTTON_DEBOUNCE_MS after the last edge and the
 * hold time is timed with application events: no busy wait, no sleep change.
 * The feedback colours go through zigbee_light_feedback(): they are never
 * written to the ZCL attributes or to NVM3.
 */
#include "button.h"
#include "app/framework/include/af.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_gpio.h"
#include "app_config.h"
#include "lamp.h"
#include "zigbee_app.h"
#include "zigbee_light.h"

typedef enum {
  BTN_IDLE = 0,
  BTN_PRESSED,         /* down, shorter than APP_BUTTON_WARN_MS */
  BTN_WARNING,         /* down, red blinking, reset at APP_BUTTON_FACTORY_RESET_MS */
  BTN_WAIT_RELEASE,    /* action done (or held at boot): ignore until released */
} button_state_t;

static const lamp_rgb_t COLOR_JOINING = { 0u, 0u, 255u };     /* blue, blinking */
static const lamp_rgb_t COLOR_OK      = { 0u, 255u, 0u };     /* green */
static const lamp_rgb_t COLOR_FAILED  = { 255u, 96u, 0u };    /* amber */
static const lamp_rgb_t COLOR_RESET   = { 255u, 0u, 0u };     /* red */

static sl_zigbee_af_event_t edge_event;      /* ISR event: activated by the interrupt */
static sl_zigbee_af_event_t button_event;    /* debounce and hold timing */
static sl_zigbee_af_event_t feedback_event;  /* end of a solid-colour feedback */

static button_state_t state;
static uint32_t press_ms;
static bool join_feedback;     /* a join started by the button is running */
static bool timed_feedback;    /* a solid colour shows until feedback_event */

static bool button_down(void)
{
  return GPIO_PinInGet(APP_BUTTON_PORT, APP_BUTTON_PIN) == 0u;
}

/* ---- LED feedback -------------------------------------------------------- */

static void show_blink(lamp_rgb_t c)
{
  zigbee_light_feedback(true);
  lamp_apply(true, 254u, c, 0u);
  lamp_set_effect(LAMP_EFFECT_BLINK);
}

static void show_solid(lamp_rgb_t c)
{
  zigbee_light_feedback(true);
  lamp_apply(true, 254u, c, 0u);
  timed_feedback = true;
  sl_zigbee_af_event_set_delay_ms(&feedback_event, APP_BUTTON_FEEDBACK_MS);
}

/* Back to what should show when no button feedback is due. */
static void feedback_done(void)
{
  timed_feedback = false;
  sl_zigbee_af_event_set_inactive(&feedback_event);
  if (join_feedback) {
    show_blink(COLOR_JOINING);
  } else {
    zigbee_light_feedback(false);
  }
}

static void feedback_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  feedback_done();
}

/* ---- actions --------------------------------------------------------------- */

static void short_press(void)
{
  sl_zigbee_network_status_t st = sl_zigbee_af_network_state();
  if (st == SL_ZIGBEE_JOINED_NETWORK) {
    sl_zigbee_app_debug_println("Button: already joined (hold 10 s to factory reset)");
    show_solid(COLOR_OK);
  } else if (st != SL_ZIGBEE_NO_NETWORK || zigbee_app_is_joining()) {
    sl_zigbee_app_debug_println("Button: busy (joining or rejoining), ignored");
    feedback_done();
  } else {
    sl_status_t s = zigbee_app_join();
    sl_zigbee_app_debug_println("Button: join %s (0x%02lX)",
                                s == SL_STATUS_OK ? "searching" : "could not start",
                                (unsigned long)s);
    if (s == SL_STATUS_OK) {
      join_feedback = true;
      timed_feedback = false;
      sl_zigbee_af_event_set_inactive(&feedback_event);
      show_blink(COLOR_JOINING);
    } else {
      show_solid(COLOR_FAILED);
    }
  }
}

static void factory_reset(void)
{
  sl_zigbee_app_debug_println("Button: held %u s, factory reset",
                              (unsigned)(APP_BUTTON_FACTORY_RESET_MS / 1000u));
  join_feedback = false;              /* steering is stopped by the reset */
  (void)zigbee_app_factory_reset();   /* LED off, attributes to defaults */
  show_solid(COLOR_RESET);            /* then off: feedback_done() */
}

/* ---- state machine -------------------------------------------------------- */

static void button_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  bool down = button_down();
  uint32_t held = halCommonGetInt32uMillisecondTick() - press_ms;

  switch (state) {
    case BTN_IDLE:
      if (down) {
        press_ms = halCommonGetInt32uMillisecondTick();
        state = BTN_PRESSED;
        sl_zigbee_af_event_set_delay_ms(&button_event, APP_BUTTON_TICK_MS);
      }
      break;

    case BTN_PRESSED:
      if (!down) {
        state = BTN_IDLE;
        short_press();
      } else if (held >= APP_BUTTON_WARN_MS) {
        state = BTN_WARNING;
        sl_zigbee_app_debug_println("Button: held, factory reset in %u s unless released",
                                    (unsigned)((APP_BUTTON_FACTORY_RESET_MS - APP_BUTTON_WARN_MS)
                                               / 1000u));
        timed_feedback = false;
        sl_zigbee_af_event_set_inactive(&feedback_event);
        show_blink(COLOR_RESET);
        sl_zigbee_af_event_set_delay_ms(&button_event, APP_BUTTON_TICK_MS);
      } else {
        sl_zigbee_af_event_set_delay_ms(&button_event, APP_BUTTON_TICK_MS);
      }
      break;

    case BTN_WARNING:
      if (!down) {
        state = BTN_IDLE;
        sl_zigbee_app_debug_println("Button: released, factory reset cancelled");
        feedback_done();
      } else if (held >= APP_BUTTON_FACTORY_RESET_MS) {
        state = BTN_WAIT_RELEASE;
        factory_reset();
      } else {
        sl_zigbee_af_event_set_delay_ms(&button_event, APP_BUTTON_TICK_MS);
      }
      break;

    case BTN_WAIT_RELEASE:
    default:
      if (!down) {
        state = BTN_IDLE;
      }
      break;
  }
}

static void edge_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  /* Read the level once it has been stable for the debounce time. While the
   * button is held, the hold ticks keep running from button_handler(). */
  sl_zigbee_af_event_set_delay_ms(&button_event, APP_BUTTON_DEBOUNCE_MS);
}

static void on_edge(uint8_t int_no, void *context)
{
  (void)int_no;
  (void)context;
  sl_zigbee_af_event_set_active(&edge_event);   /* ISR-safe: ISR event */
}

/* ---- public ---------------------------------------------------------------- */

void button_init(void)
{
  sl_zigbee_af_isr_event_init(&edge_event, edge_handler);
  sl_zigbee_af_event_init(&button_event, button_handler);
  sl_zigbee_af_event_init(&feedback_event, feedback_handler);

  CMU_ClockEnable(cmuClock_GPIO, true);
  /* Input, pull-up (DOUT = 1), glitch filter. No current flows unless the
   * button is pressed (about 3.3 V / 40 kOhm = 80 uA while held). */
  GPIO_PinModeSet(APP_BUTTON_PORT, APP_BUTTON_PIN, gpioModeInputPullFilter, 1);

  /* Held at boot (or shorted): never acts until it has been released. */
  state = button_down() ? BTN_WAIT_RELEASE : BTN_IDLE;

  const sl_gpio_t pin = { .port = (sl_gpio_port_t)APP_BUTTON_PORT, .pin = APP_BUTTON_PIN };
  int32_t int_no = SL_GPIO_INTERRUPT_UNAVAILABLE;
  sl_status_t s = sl_gpio_configure_external_interrupt(&pin, &int_no,
                                                       SL_GPIO_INTERRUPT_RISING_FALLING_EDGE,
                                                       on_edge, NULL);
  sl_zigbee_app_debug_println("Button D9: %s%s",
                              s == SL_STATUS_OK ? "ready" : "interrupt not available",
                              state == BTN_WAIT_RELEASE ? ", held at boot: ignored until released" : "");
}

void button_join_finished(bool joined)
{
  if (!join_feedback) {
    return;   /* not started by the button (`zigbee join`), or reset since */
  }
  join_feedback = false;
  if (state == BTN_WARNING) {
    return;   /* the red warning keeps the LED; feedback_done() restores it */
  }
  show_solid(joined ? COLOR_OK : COLOR_FAILED);
}
