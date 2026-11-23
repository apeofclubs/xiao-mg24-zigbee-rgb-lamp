/**
 * @file lamp.c
 * @brief Colour pipeline: colour (0..255) x brightness (0..100 %) in a
 *        perceptual scale, then gamma 2.2, then per-channel balance, then the
 *        duty cap, then PWM (rgb_led.c).
 *
 * Fades interpolate in the perceptual scale, so they look even. Fades and
 * effects run on one Zigbee application-framework event (APP_LED_TICK_MS),
 * active only while something moves. No dynamic allocation.
 */
#include "lamp.h"
#include <string.h>
#include "app/framework/include/af.h"
#include "app_config.h"
#include "rgb_led.h"
#include "storage.h"
#include "color.h"

/* Perceptual value of a channel: colour (0..255) x level (0..254, the Zigbee
 * CurrentLevel scale; the console's 0..100 % is converted on the way in). */
#define LEVEL_MAX  254u
#define P_MAX      (255u * LEVEL_MAX)

/* x^2.2 for x = i/256, i = 0..256, scaled to 0..65535. Generated with
 * round(65535 * (i / 256) ** 2.2). */
static const uint16_t gamma_table[257] = {
      0,     0,     2,     4,     7,    11,    17,    24,
     32,    41,    52,    64,    78,    93,   110,   128,
    147,   168,   191,   215,   240,   267,   296,   327,
    359,   392,   428,   465,   504,   544,   586,   630,
    676,   723,   772,   823,   875,   930,   986,  1044,
   1104,  1165,  1229,  1294,  1361,  1430,  1501,  1574,
   1648,  1725,  1803,  1884,  1966,  2050,  2136,  2224,
   2314,  2406,  2500,  2595,  2693,  2793,  2895,  2998,
   3104,  3212,  3322,  3433,  3547,  3663,  3781,  3900,
   4022,  4146,  4272,  4400,  4530,  4663,  4797,  4933,
   5072,  5212,  5355,  5499,  5646,  5795,  5946,  6099,
   6255,  6412,  6572,  6733,  6897,  7063,  7231,  7402,
   7574,  7749,  7926,  8105,  8286,  8469,  8655,  8843,
   9033,  9225,  9419,  9616,  9815, 10016, 10219, 10425,
  10632, 10842, 11054, 11269, 11486, 11705, 11926, 12149,
  12375, 12603, 12833, 13066, 13301, 13538, 13777, 14019,
  14263, 14509, 14758, 15009, 15262, 15517, 15775, 16035,
  16298, 16563, 16830, 17099, 17371, 17645, 17922, 18201,
  18482, 18765, 19051, 19339, 19630, 19923, 20218, 20516,
  20816, 21119, 21424, 21731, 22040, 22352, 22667, 22984,
  23303, 23624, 23949, 24275, 24604, 24935, 25269, 25605,
  25943, 26284, 26628, 26973, 27322, 27672, 28026, 28381,
  28739, 29100, 29462, 29828, 30196, 30566, 30939, 31314,
  31692, 32072, 32454, 32840, 33227, 33617, 34010, 34405,
  34802, 35202, 35605, 36010, 36417, 36827, 37240, 37655,
  38072, 38493, 38915, 39340, 39768, 40198, 40631, 41066,
  41503, 41944, 42387, 42832, 43280, 43730, 44183, 44639,
  45097, 45557, 46020, 46486, 46954, 47425, 47899, 48374,
  48853, 49334, 49818, 50304, 50793, 51284, 51778, 52275,
  52774, 53276, 53780, 54287, 54796, 55308, 55823, 56341,
  56860, 57383, 57908, 58436, 58966, 59499, 60035, 60573,
  61114, 61657, 62203, 62752, 63303, 63857, 64414, 64973,
  65535,
};

typedef struct {
  const char *name;
  lamp_rgb_t color;
} preset_t;

static const preset_t presets[] = {
  { "warmwhite", { 255, 140, 40 } },
  { "white",     { 255, 255, 255 } },
  { "red",       { 255, 0, 0 } },
  { "green",     { 0, 255, 0 } },
  { "blue",      { 0, 0, 255 } },
  { "orange",    { 255, 80, 0 } },
  { "yellow",    { 255, 180, 0 } },
  { "purple",    { 140, 0, 255 } },
  { "cyan",      { 0, 255, 255 } },
  { "pink",      { 255, 40, 120 } },
};
#define PRESET_COUNT  (sizeof(presets) / sizeof(presets[0]))


static storage_lamp_settings_t settings;
static lamp_rgb_t color = { 255, 140, 40 };   /* warm white until told otherwise */
static uint8_t level = LEVEL_MAX;              /* last non-zero level, 1..254 */
static bool on;

/* Output in the perceptual scale, and the running fade. */
static uint16_t p_now[3];
static uint16_t p_from[3];
static uint16_t p_to[3];
static uint32_t fade_total_ticks;
static uint32_t fade_tick;

static lamp_effect_t effect;
static uint32_t effect_ms;

static sl_zigbee_af_event_t tick_event;

/* ------------------------------------------------------------------------- */

static uint16_t gamma_of(uint16_t p)
{
  uint32_t pos = (uint32_t)p * 256u;
  uint32_t i = pos / P_MAX;
  uint32_t frac = pos % P_MAX;
  if (i >= 256u) {
    return gamma_table[256];
  }
  uint32_t a = gamma_table[i];
  uint32_t b = gamma_table[i + 1u];
  return (uint16_t)(a + ((b - a) * frac + P_MAX / 2u) / P_MAX);
}

static void output(const uint16_t p[3])
{
  uint16_t duty[3];
  for (uint8_t i = 0; i < 3u; i++) {
    uint32_t d = gamma_of(p[i]);
    d = d * settings.balance[i] / 100u;
    d = d * APP_LED_MAX_DUTY_PERCENT / 100u;
    duty[i] = (uint16_t)d;
  }
  rgb_led_write(duty);
}

static void target_from_state(uint16_t p[3])
{
  uint8_t lvl = on ? level : 0u;
  p[0] = (uint16_t)(color.r * lvl);
  p[1] = (uint16_t)(color.g * lvl);
  p[2] = (uint16_t)(color.b * lvl);
}

static void start_fade_ms(uint16_t fade_ms)
{
  target_from_state(p_to);
  memcpy(p_from, p_now, sizeof(p_from));
  fade_total_ticks = fade_ms / APP_LED_TICK_MS;
  fade_tick = 0;
  if (fade_total_ticks == 0u) {
    memcpy(p_now, p_to, sizeof(p_now));
    output(p_now);
    sl_zigbee_af_event_set_inactive(&tick_event);
    return;
  }
  sl_zigbee_af_event_set_delay_ms(&tick_event, APP_LED_TICK_MS);
}

static void start_fade(void)
{
  start_fade_ms(settings.fade_ms);
}

static void stop_effect(void)
{
  effect = LAMP_EFFECT_NONE;
  effect_ms = 0;
}

static void effect_step(void)
{
  effect_ms += APP_LED_TICK_MS;
  uint16_t p[3];
  if (effect == LAMP_EFFECT_BLINK) {
    /* Identify: the current colour (white if none) at full brightness,
     * APP_EFFECT_BLINK_PERIOD_MS on/off, whatever the on/off state. */
    effect_ms %= APP_EFFECT_BLINK_PERIOD_MS;
    lamp_rgb_t c = color;
    if (c.r == 0u && c.g == 0u && c.b == 0u) {
      c.r = c.g = c.b = 255u;
    }
    bool lit = effect_ms < (APP_EFFECT_BLINK_PERIOD_MS / 2u);
    p[0] = lit ? (uint16_t)(c.r * LEVEL_MAX) : 0u;
    p[1] = lit ? (uint16_t)(c.g * LEVEL_MAX) : 0u;
    p[2] = lit ? (uint16_t)(c.b * LEVEL_MAX) : 0u;
  } else if (effect == LAMP_EFFECT_RAINBOW) {
    effect_ms %= APP_EFFECT_RAINBOW_PERIOD_MS;
    uint16_t hue = (uint16_t)(effect_ms * 360u / APP_EFFECT_RAINBOW_PERIOD_MS);
    color = color_from_hsv(hue, 100u, 100u);
    target_from_state(p);
  } else {   /* LAMP_EFFECT_BREATHE */
    /* Breathe: triangle wave on the brightness of the current colour. */
    effect_ms %= APP_EFFECT_BREATHE_PERIOD_MS;
    uint32_t half = APP_EFFECT_BREATHE_PERIOD_MS / 2u;
    uint32_t pos = (effect_ms < half) ? effect_ms : (APP_EFFECT_BREATHE_PERIOD_MS - effect_ms);
    uint32_t scale = APP_EFFECT_BREATHE_MIN_PERCENT
                     + (100u - APP_EFFECT_BREATHE_MIN_PERCENT) * pos / half;   /* 0..100 */
    target_from_state(p);
    for (uint8_t i = 0; i < 3u; i++) {
      p[i] = (uint16_t)((uint32_t)p[i] * scale / 100u);
    }
  }
  memcpy(p_now, p, sizeof(p_now));
  output(p_now);
}

static void tick_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  if (effect != LAMP_EFFECT_NONE) {
    effect_step();
    sl_zigbee_af_event_set_delay_ms(&tick_event, APP_LED_TICK_MS);
    return;
  }
  fade_tick++;
  if (fade_tick >= fade_total_ticks) {
    memcpy(p_now, p_to, sizeof(p_now));
    output(p_now);
    return;   /* fade done: event stays inactive */
  }
  for (uint8_t i = 0; i < 3u; i++) {
    int32_t from = p_from[i];
    int32_t to = p_to[i];
    p_now[i] = (uint16_t)(from + (to - from) * (int32_t)fade_tick / (int32_t)fade_total_ticks);
  }
  output(p_now);
  sl_zigbee_af_event_set_delay_ms(&tick_event, APP_LED_TICK_MS);
}

/* ------------------------------------------------------------------------- */

void lamp_init(void)
{
  settings.balance[0] = APP_LED_BALANCE_RED_DEFAULT;
  settings.balance[1] = APP_LED_BALANCE_GREEN_DEFAULT;
  settings.balance[2] = APP_LED_BALANCE_BLUE_DEFAULT;
  settings.fade_ms = APP_LED_FADE_DEFAULT_MS;
  (void)storage_load_lamp_settings(&settings);   /* keeps the defaults if none stored */

  sl_zigbee_af_event_init(&tick_event, tick_handler);
  on = false;
  memset(p_now, 0, sizeof(p_now));
  rgb_led_off();
}

void lamp_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
  stop_effect();
  color.r = r;
  color.g = g;
  color.b = b;
  on = true;
  start_fade();
}

void lamp_set_hsv(uint16_t hue, uint8_t sat, uint8_t val)
{
  lamp_rgb_t c = color_from_hsv(hue, sat, val);
  lamp_set_rgb(c.r, c.g, c.b);
}

void lamp_set_level(uint8_t percent)
{
  stop_effect();
  if (percent > 100u) {
    percent = 100u;
  }
  if (percent == 0u) {
    on = false;
  } else {
    level = (uint8_t)(((uint32_t)percent * LEVEL_MAX + 50u) / 100u);
    on = true;
  }
  start_fade();
}

void lamp_apply(bool new_on, uint8_t new_level, lamp_rgb_t new_color, uint16_t fade_ms)
{
  stop_effect();
  if (new_level > LEVEL_MAX) {
    new_level = LEVEL_MAX;
  }
  if (new_level > 0u) {
    level = new_level;
  }
  on = new_on && (new_level > 0u);
  color = new_color;
  start_fade_ms(fade_ms);
}

void lamp_off(void)
{
  stop_effect();
  on = false;
  start_fade();
}

bool lamp_set_preset(const char *name)
{
  for (uint32_t i = 0; i < PRESET_COUNT; i++) {
    if (strcmp(name, presets[i].name) == 0) {
      lamp_set_rgb(presets[i].color.r, presets[i].color.g, presets[i].color.b);
      return true;
    }
  }
  return false;
}

const char *lamp_preset_name(uint32_t index)
{
  return (index < PRESET_COUNT) ? presets[index].name : NULL;
}

void lamp_set_effect(lamp_effect_t new_effect)
{
  stop_effect();
  if (new_effect == LAMP_EFFECT_NONE) {
    /* Settle on the colour the effect left. */
    start_fade();
    return;
  }
  effect = new_effect;
  on = true;
  sl_zigbee_af_event_set_delay_ms(&tick_event, APP_LED_TICK_MS);
}

bool lamp_set_balance(uint8_t r, uint8_t g, uint8_t b)
{
  settings.balance[0] = (r > 100u) ? 100u : r;
  settings.balance[1] = (g > 100u) ? 100u : g;
  settings.balance[2] = (b > 100u) ? 100u : b;
  if (effect == LAMP_EFFECT_NONE && !sl_zigbee_af_event_is_scheduled(&tick_event)) {
    output(p_now);   /* show the new balance at once */
  }
  return storage_save_lamp_settings(&settings);
}

void lamp_get_balance(uint8_t balance[3])
{
  memcpy(balance, settings.balance, 3u);
}

bool lamp_set_fade_ms(uint16_t ms)
{
  settings.fade_ms = (ms > APP_LED_FADE_MAX_MS) ? APP_LED_FADE_MAX_MS : ms;
  return storage_save_lamp_settings(&settings);
}

uint16_t lamp_get_fade_ms(void)
{
  return settings.fade_ms;
}

void lamp_get_state(lamp_state_t *state)
{
  state->on = on;
  state->level = (uint8_t)(((uint32_t)level * 100u + LEVEL_MAX / 2u) / LEVEL_MAX);
  state->level_raw = level;
  state->color = color;
  state->effect = effect;
  state->fading = (effect == LAMP_EFFECT_NONE) && sl_zigbee_af_event_is_scheduled(&tick_event);
}
