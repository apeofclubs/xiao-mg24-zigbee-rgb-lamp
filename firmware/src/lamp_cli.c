/**
 * @file lamp_cli.c
 * @brief Bench console commands for Phase 2 (docs/bench-test.md):
 *
 *   lamp rgb <r> <g> <b>          colour 0..255 each, lamp on
 *   lamp hsv <h> <s> <v>          hue 0..359, saturation and value 0..100
 *   lamp level <0..100>           brightness (0 = off)
 *   lamp off
 *   lamp white                    balanced white, to judge the balance
 *   lamp balance [<r> <g> <b>]    show, or set and save (0..100 % each)
 *   lamp preset <name>            named colour (lamp preset with no name lists them)
 *   lamp fade <ms>                default transition time, saved (0 = instant)
 *   lamp effect rainbow|breathe|off
 *   lamp status
 *
 * Any command other than `lamp status` and `lamp balance` (show) stops a
 * running effect. The console runs over SEGGER RTT (iostream_rtt); RTT has no
 * receive interrupt, so an application event wakes the main loop every
 * APP_CONSOLE_POLL_MS to let the CLI read it.
 */
#include "lamp_cli.h"
#include <stdio.h>
#include <string.h>
#include "app/framework/include/af.h"
#include "sl_cli.h"
#include "app_config.h"
#include "lamp.h"
#include "rgb_led.h"

static sl_zigbee_af_event_t console_poll_event;

static void console_poll_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  /* Nothing to do: waking the main loop lets sl_cli read the RTT buffer. */
  sl_zigbee_af_event_set_delay_ms(&console_poll_event, APP_CONSOLE_POLL_MS);
}

static const char *effect_name(lamp_effect_t e)
{
  switch (e) {
    case LAMP_EFFECT_RAINBOW: return "rainbow";
    case LAMP_EFFECT_BREATHE: return "breathe";
    default:                  return "off";
  }
}

static void print_balance(void)
{
  uint8_t b[3];
  lamp_get_balance(b);
  printf("balance r=%u%% g=%u%% b=%u%%\r\n", b[0], b[1], b[2]);
}

static void cmd_rgb(sl_cli_command_arg_t *args)
{
  lamp_set_rgb(sl_cli_get_argument_uint8(args, 0),
               sl_cli_get_argument_uint8(args, 1),
               sl_cli_get_argument_uint8(args, 2));
  printf("ok\r\n");
}

static void cmd_hsv(sl_cli_command_arg_t *args)
{
  uint16_t h = sl_cli_get_argument_uint16(args, 0);
  uint8_t s = sl_cli_get_argument_uint8(args, 1);
  uint8_t v = sl_cli_get_argument_uint8(args, 2);
  if (h > 359u || s > 100u || v > 100u) {
    printf("error: h 0-359, s 0-100, v 0-100\r\n");
    return;
  }
  lamp_set_hsv(h, s, v);
  printf("ok\r\n");
}

static void cmd_level(sl_cli_command_arg_t *args)
{
  uint8_t l = sl_cli_get_argument_uint8(args, 0);
  if (l > 100u) {
    printf("error: level 0-100\r\n");
    return;
  }
  lamp_set_level(l);
  printf("ok\r\n");
}

static void cmd_off(sl_cli_command_arg_t *args)
{
  (void)args;
  lamp_off();
  printf("ok\r\n");
}

static void cmd_white(sl_cli_command_arg_t *args)
{
  (void)args;
  lamp_set_rgb(255, 255, 255);
  print_balance();
}

static void cmd_balance(sl_cli_command_arg_t *args)
{
  int n = sl_cli_get_argument_count(args);
  if (n == 0) {
    print_balance();
    return;
  }
  if (n != 3) {
    printf("error: lamp balance <r> <g> <b> (0-100 each), or no argument to show\r\n");
    return;
  }
  uint8_t r = sl_cli_get_argument_uint8(args, 0);
  uint8_t g = sl_cli_get_argument_uint8(args, 1);
  uint8_t b = sl_cli_get_argument_uint8(args, 2);
  if (r > 100u || g > 100u || b > 100u) {
    printf("error: balance 0-100 each\r\n");
    return;
  }
  bool saved = lamp_set_balance(r, g, b);
  print_balance();
  printf("%s\r\n", saved ? "saved" : "error: NVM3 write failed (applied until reset)");
}

static void cmd_preset(sl_cli_command_arg_t *args)
{
  if (sl_cli_get_argument_count(args) == 1
      && lamp_set_preset(sl_cli_get_argument_string(args, 0))) {
    printf("ok\r\n");
    return;
  }
  printf("presets:");
  for (uint32_t i = 0; lamp_preset_name(i) != NULL; i++) {
    printf(" %s", lamp_preset_name(i));
  }
  printf("\r\n");
}

static void cmd_fade(sl_cli_command_arg_t *args)
{
  uint16_t ms = sl_cli_get_argument_uint16(args, 0);
  if (ms > APP_LED_FADE_MAX_MS) {
    printf("error: fade 0-%u ms\r\n", (unsigned)APP_LED_FADE_MAX_MS);
    return;
  }
  bool saved = lamp_set_fade_ms(ms);
  printf("fade %u ms, %s\r\n", lamp_get_fade_ms(), saved ? "saved" : "NVM3 write failed");
}

static void cmd_effect(sl_cli_command_arg_t *args)
{
  const char *name = sl_cli_get_argument_string(args, 0);
  if (strcmp(name, "rainbow") == 0) {
    lamp_set_effect(LAMP_EFFECT_RAINBOW);
  } else if (strcmp(name, "breathe") == 0) {
    lamp_set_effect(LAMP_EFFECT_BREATHE);
  } else if (strcmp(name, "off") == 0) {
    lamp_set_effect(LAMP_EFFECT_NONE);
  } else {
    printf("error: lamp effect rainbow|breathe|off\r\n");
    return;
  }
  printf("ok\r\n");
}

static void cmd_status(sl_cli_command_arg_t *args)
{
  (void)args;
  lamp_state_t s;
  lamp_get_state(&s);
  printf("%s %s\r\n", APP_MODEL_IDENTIFIER, APP_FIRMWARE_VERSION_STRING);
  printf("target: %s rgb=%u,%u,%u level=%u%% effect=%s%s\r\n",
         s.on ? "on" : "off", s.color.r, s.color.g, s.color.b, s.level,
         effect_name(s.effect), s.fading ? " (fading)" : "");
  print_balance();
  printf("fade %u ms, LED %s\r\n", lamp_get_fade_ms(),
         APP_LED_COMMON_CATHODE ? "common cathode" : "common anode");
  uint32_t period = rgb_led_period_counts();
  if (period == 0u) {
    printf("pwm: stopped, all pins at OFF level (r=0.0%% g=0.0%% b=0.0%%)\r\n");
    return;
  }
  const char names[3] = { 'r', 'g', 'b' };
  printf("pwm: %u Hz, period %lu counts:", (unsigned)APP_LED_PWM_FREQUENCY_HZ,
         (unsigned long)period);
  for (uint8_t i = 0; i < 3u; i++) {
    uint32_t c = rgb_led_duty_counts(i);
    uint32_t permille = (uint32_t)(((uint64_t)c * 1000u + period / 2u) / period);
    printf(" %c=%lu.%lu%% (%lu)", names[i], (unsigned long)(permille / 10u),
           (unsigned long)(permille % 10u), (unsigned long)c);
  }
  printf("\r\n");
}

static const sl_cli_command_info_t info_rgb = SL_CLI_COMMAND(
  cmd_rgb, "Colour, lamp on", "red 0-255" SL_CLI_UNIT_SEPARATOR "green 0-255"
  SL_CLI_UNIT_SEPARATOR "blue 0-255",
  { SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_hsv = SL_CLI_COMMAND(
  cmd_hsv, "Colour from hue/saturation/value", "hue 0-359" SL_CLI_UNIT_SEPARATOR
  "saturation 0-100" SL_CLI_UNIT_SEPARATOR "value 0-100",
  { SL_CLI_ARG_UINT16, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_level = SL_CLI_COMMAND(
  cmd_level, "Brightness in %, 0 = off", "0-100",
  { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_off = SL_CLI_COMMAND(
  cmd_off, "Fade to off", "", { SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_white = SL_CLI_COMMAND(
  cmd_white, "Balanced white (255,255,255) to judge the balance", "", { SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_balance = SL_CLI_COMMAND(
  cmd_balance, "Show, or set and save, the per-channel balance",
  "r g b, 0-100 each (none: show)",
  // One optional type, last in the list: the SDK CLI repeats it for every
  // token (0..n uint8) and rejects an optional type that is not last.
  // cmd_balance accepts exactly 0 or 3 values.
  { SL_CLI_ARG_UINT8OPT, SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_preset = SL_CLI_COMMAND(
  cmd_preset, "Named colour (no name: list)", "name",
  { SL_CLI_ARG_STRINGOPT, SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_fade = SL_CLI_COMMAND(
  cmd_fade, "Default transition time, saved", "ms 0-10000",
  { SL_CLI_ARG_UINT16, SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_effect = SL_CLI_COMMAND(
  cmd_effect, "Demo effect", "rainbow|breathe|off",
  { SL_CLI_ARG_STRING, SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_status = SL_CLI_COMMAND(
  cmd_status, "Target, balance and PWM duty per channel", "", { SL_CLI_ARG_END, });

static const sl_cli_command_entry_t lamp_table[] = {
  { "rgb", &info_rgb, false },
  { "hsv", &info_hsv, false },
  { "level", &info_level, false },
  { "off", &info_off, false },
  { "white", &info_white, false },
  { "balance", &info_balance, false },
  { "preset", &info_preset, false },
  { "fade", &info_fade, false },
  { "effect", &info_effect, false },
  { "status", &info_status, false },
  { NULL, NULL, false },
};

static const sl_cli_command_info_t info_lamp = SL_CLI_COMMAND_GROUP(
  lamp_table, "RGB lamp bench commands");

static const sl_cli_command_entry_t top_table[] = {
  { "lamp", &info_lamp, false },
  { NULL, NULL, false },
};

static sl_cli_command_group_t lamp_group = {
  { NULL },
  false,
  top_table,
};

void lamp_cli_init(void)
{
  /* The CLI instance ("example") is initialised by sl_main_init(), before this. */
  sl_cli_command_add_command_group(sl_cli_default_handle, &lamp_group);
  sl_zigbee_af_event_init(&console_poll_event, console_poll_handler);
  sl_zigbee_af_event_set_delay_ms(&console_poll_event, APP_CONSOLE_POLL_MS);
}
