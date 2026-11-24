/**
 * @file button.c
 * @brief On/off and pairing push button on pad D9 (PA04) to GND
 *        (operator, 2026-11-24, AUR-462).
 *
 *   short press (released before APP_BUTTON_PAIR_MS)
 *       toggle the light, as a ZCL On/Off Toggle command (zigbee_light_toggle()):
 *       the OnOff attribute changes, so a joined lamp reports it, and the lamp
 *       works the same when it is not on a network. No colour feedback. During
 *       a pairing the attribute still toggles; the LED shows it once the
 *       pairing indication ends.
 *   held APP_BUTTON_PAIR_MS: pairing (zigbee_app_pair()); a joined lamp
 *       leaves its network first. The LED breathes blue from that moment until
 *       the join ends, then shows solid green (joined) or amber (failed) for
 *       APP_BUTTON_FEEDBACK_MS, then the light state. Letting go is not needed
 *       for the pairing to run; nothing more happens on release.
 *       Hold while a pairing (button or `zigbee join`) runs: ignored, the
 *       running pairing goes on.
 *   No factory reset on the button: `zigbee reset` on the console.
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
  BTN_PRESSED,         /* down, shorter than APP_BUTTON_PAIR_MS */
  BTN_WAIT_RELEASE,    /* pairing started (or held at boot): ignore until released */
} button_state_t;

static const lamp_rgb_t COLOR_JOINING = { 0u, 0u, 255u };     /* blue, breathing */
static const lamp_rgb_t COLOR_OK      = { 0u, 255u, 0u };     /* green */
static const lamp_rgb_t COLOR_FAILED  = { 255u, 96u, 0u };    /* amber */

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

static void show_breathe(lamp_rgb_t c)
{
  zigbee_light_feedback(true);
  lamp_apply(true, 254u, c, 0u);
  lamp_set_effect(LAMP_EFFECT_BREATHE);
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
    show_breathe(COLOR_JOINING);
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
  bool ok = zigbee_light_toggle();
  sl_zigbee_app_debug_println("Button: toggle%s", ok ? "" : " failed");
}

static void long_press(void)
{
  if (zigbee_app_is_joining()) {
    sl_zigbee_app_debug_println("Button: a pairing is already running, hold ignored");
    return;
  }
  bool was_on_network = sl_zigbee_af_network_state() != SL_ZIGBEE_NO_NETWORK;
  /* Breathe first, so the user sees at once that the hold was taken. */
  join_feedback = true;
  timed_feedback = false;
  sl_zigbee_af_event_set_inactive(&feedback_event);
  show_breathe(COLOR_JOINING);
  sl_status_t s = zigbee_app_pair();
  sl_zigbee_app_debug_println("Button: held %u s, pairing %s%s (0x%02lX)",
                              (unsigned)(APP_BUTTON_PAIR_MS / 1000u),
                              s == SL_STATUS_OK ? "started" : "could not start",
                              was_on_network ? ", leaving the current network first" : "",
                              (unsigned long)s);
  if (s != SL_STATUS_OK) {
    join_feedback = false;
    show_solid(COLOR_FAILED);
  }
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
      } else if (held >= APP_BUTTON_PAIR_MS) {
        state = BTN_WAIT_RELEASE;
        long_press();
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
    return;   /* not started by the button (`zigbee join`) */
  }
  join_feedback = false;
  show_solid(joined ? COLOR_OK : COLOR_FAILED);
}
