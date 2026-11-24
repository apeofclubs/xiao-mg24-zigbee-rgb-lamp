/**
 * @file zigbee_cli.c
 * @brief Network commands on the RTT console (docs/zigbee.md):
 *
 *   zigbee join            start network steering (open the network in Z2M first);
 *                          same as a short press on the D9 button
 *   zigbee leave           leave the network (the light keeps its state)
 *   zigbee status          network state, PAN, channel, node, parent, LQI/RSSI
 *   zigbee reset           factory reset: leave, clear bindings, reporting,
 *                          groups, scenes, light attributes and the saved last
 *                          state. The balance (NVM3 key 0x0A100) is kept.
 *                          Same as holding the D9 button for 10 s.
 *   zigbee rfswitch [reset|0|1]
 *                          H6 bench aid: show or set the RF switch pins
 *                          PB04/PB05. Not saved: every reboot returns to the
 *                          reset state (both pins disabled).
 */
#include "zigbee_cli.h"
#include <stdio.h>
#include <string.h>
#include "app/framework/include/af.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_cli.h"
#include "app_config.h"
#include "zigbee_app.h"
#include "zigbee_light.h"

typedef enum {
  RF_SWITCH_RESET = 0,   /* both pins disabled: reset state, as the Arduino core */
  RF_SWITCH_SELECT_0,    /* PB05 HIGH (switch powered), PB04 LOW */
  RF_SWITCH_SELECT_1,    /* PB05 HIGH (switch powered), PB04 HIGH */
} rf_switch_state_t;

static rf_switch_state_t rf_switch_state = RF_SWITCH_RESET;

static const char *rf_switch_name(rf_switch_state_t s)
{
  switch (s) {
    case RF_SWITCH_SELECT_0: return "0 (PB05 HIGH, PB04 LOW)";
    case RF_SWITCH_SELECT_1: return "1 (PB05 HIGH, PB04 HIGH)";
    default:                 return "reset (PB04, PB05 disabled)";
  }
}

static const char *network_state_name(sl_zigbee_network_status_t s)
{
  switch (s) {
    case SL_ZIGBEE_NO_NETWORK:              return "no network";
    case SL_ZIGBEE_JOINING_NETWORK:         return "joining";
    case SL_ZIGBEE_JOINED_NETWORK:          return "joined";
    case SL_ZIGBEE_JOINED_NETWORK_NO_PARENT: return "joined, parent lost (rejoining)";
    case SL_ZIGBEE_LEAVING_NETWORK:         return "leaving";
    default:                                return "unknown";
  }
}

static void print_eui64(const uint8_t *eui)
{
  /* Little-endian in memory; printed most significant byte first, as Z2M. */
  printf("0x");
  for (int i = 7; i >= 0; i--) {
    printf("%02x", eui[i]);
  }
}

static void cmd_join(sl_cli_command_arg_t *args)
{
  (void)args;
  sl_zigbee_network_status_t st = sl_zigbee_af_network_state();
  if (st != SL_ZIGBEE_NO_NETWORK) {
    printf("already %s: `zigbee leave` (or `zigbee reset`) first\r\n",
           network_state_name(st));
    return;
  }
  if (zigbee_app_is_joining()) {
    printf("already searching: wait for \"Join done\" or \"Join failed\"\r\n");
    return;
  }
  sl_status_t s = zigbee_app_join();
  printf("join: %s (0x%02lX). Result in about 10-60 s: \"Join done\" or \"Join failed\"\r\n",
         s == SL_STATUS_OK ? "searching" : "could not start", (unsigned long)s);
}

static void cmd_leave(sl_cli_command_arg_t *args)
{
  (void)args;
  if (sl_zigbee_af_network_state() == SL_ZIGBEE_NO_NETWORK) {
    printf("not on a network\r\n");
    return;
  }
  sl_status_t s = sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_NO_OPTION);
  zigbee_app_forget_link();
  printf("leave: 0x%02lX%s\r\n", (unsigned long)s,
         s == SL_STATUS_OK ? " (the light keeps its state; `zigbee join` to pair again)" : "");
}

static void cmd_status(sl_cli_command_arg_t *args)
{
  (void)args;
  sl_zigbee_network_status_t st = sl_zigbee_af_network_state();
  uint8_t eui[8];
  sl_zigbee_af_get_eui64(eui);
  printf("%s %s, IEEE ", APP_MODEL_IDENTIFIER, APP_FIRMWARE_VERSION_STRING);
  print_eui64(eui);
  printf("\r\nnetwork: %s\r\n", network_state_name(st));
  if (st == SL_ZIGBEE_JOINED_NETWORK || st == SL_ZIGBEE_JOINED_NETWORK_NO_PARENT) {
    sl_zigbee_node_type_t type;
    sl_zigbee_network_parameters_t p;
    if (sl_zigbee_get_network_parameters(&type, &p) == SL_STATUS_OK) {
      printf("pan 0x%04X, extended pan ", p.panId);
      print_eui64(p.extendedPanId);
      printf(", channel %u, tx power %d dBm\r\n", p.radioChannel, p.radioTxPower);
    }
    printf("node 0x%04X, parent 0x%04X ", sl_zigbee_af_get_node_id(),
           sl_zigbee_get_parent_node_id());
    print_eui64(sl_zigbee_get_parent_eui64());
    printf("\r\n");
  }
  uint8_t lqi;
  int8_t rssi;
  uint32_t age;
  if (zigbee_app_last_link(&lqi, &rssi, &age)) {
    printf("parent link (last message received): LQI %u, RSSI %d dBm, %lu s ago\r\n",
           lqi, rssi, (unsigned long)(age / 1000u));
  } else {
    printf("parent link: no message received yet\r\n");
  }
  printf("poll: %lu ms now (short %u ms%s, long %lu ms)\r\n",
         (unsigned long)sl_zigbee_af_get_current_poll_interval_ms_cb(),
         (unsigned)APP_POLL_SHORT_MS,
         APP_CONSOLE_KEEP_AWAKE ? ", forced: bench build" : "",
         (unsigned long)APP_POLL_LONG_MS);
  printf("rf switch: %s\r\n", rf_switch_name(rf_switch_state));
  printf("last-state NVM3 writes since boot: %lu\r\n",
         (unsigned long)zigbee_light_nvm_write_count());
}

static void cmd_reset(sl_cli_command_arg_t *args)
{
  (void)args;
  sl_status_t s = zigbee_app_factory_reset();   /* same as holding D9 for 10 s */
  printf("leave: 0x%02lX\r\n", (unsigned long)s);
  printf("factory reset done: network left, bindings, reporting, groups, scenes and "
         "last state cleared; balance kept. `zigbee join` to pair again\r\n");
}

static void rf_switch_apply(rf_switch_state_t s)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  if (s == RF_SWITCH_RESET) {
    /* Select first, then power, back to the reset state (disabled). */
    GPIO_PinModeSet(APP_RF_SWITCH_SELECT_PORT, APP_RF_SWITCH_SELECT_PIN, gpioModeDisabled, 0);
    GPIO_PinModeSet(APP_RF_SWITCH_POWER_PORT, APP_RF_SWITCH_POWER_PIN, gpioModeDisabled, 0);
  } else {
    /* Power the switch, then drive its select input. */
    GPIO_PinModeSet(APP_RF_SWITCH_POWER_PORT, APP_RF_SWITCH_POWER_PIN, gpioModePushPull, 1);
    GPIO_PinModeSet(APP_RF_SWITCH_SELECT_PORT, APP_RF_SWITCH_SELECT_PIN, gpioModePushPull,
                    s == RF_SWITCH_SELECT_1 ? 1u : 0u);
  }
  rf_switch_state = s;
}

static void cmd_rfswitch(sl_cli_command_arg_t *args)
{
  if (sl_cli_get_argument_count(args) == 0) {
    printf("rf switch: %s\r\n", rf_switch_name(rf_switch_state));
    return;
  }
  const char *a = sl_cli_get_argument_string(args, 0);
  rf_switch_state_t s;
  if (strcmp(a, "reset") == 0) {
    s = RF_SWITCH_RESET;
  } else if (strcmp(a, "0") == 0) {
    s = RF_SWITCH_SELECT_0;
  } else if (strcmp(a, "1") == 0) {
    s = RF_SWITCH_SELECT_1;
  } else {
    printf("error: zigbee rfswitch [reset|0|1]\r\n");
    return;
  }
  rf_switch_apply(s);
  printf("rf switch: %s (not saved: reset state after reboot). Wait for a few "
         "messages, then `zigbee status` for the LQI/RSSI\r\n", rf_switch_name(s));
}

static const sl_cli_command_info_t info_join = SL_CLI_COMMAND(
  cmd_join, "Join a network (permit join open in Z2M)", "", { SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_leave = SL_CLI_COMMAND(
  cmd_leave, "Leave the network", "", { SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_status = SL_CLI_COMMAND(
  cmd_status, "Network, parent, LQI/RSSI, poll, RF switch", "", { SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_reset = SL_CLI_COMMAND(
  cmd_reset, "Factory reset: leave, clear Zigbee data and last state (balance kept)", "",
  { SL_CLI_ARG_END, });
static const sl_cli_command_info_t info_rfswitch = SL_CLI_COMMAND(
  cmd_rfswitch, "H6 bench aid: RF switch pins PB04/PB05, not saved", "reset|0|1 (none: show)",
  { SL_CLI_ARG_STRINGOPT, SL_CLI_ARG_END, });

static const sl_cli_command_entry_t zigbee_table[] = {
  { "join", &info_join, false },
  { "leave", &info_leave, false },
  { "status", &info_status, false },
  { "reset", &info_reset, false },
  { "rfswitch", &info_rfswitch, false },
  { NULL, NULL, false },
};

static const sl_cli_command_info_t info_zigbee = SL_CLI_COMMAND_GROUP(
  zigbee_table, "Zigbee network commands");

static const sl_cli_command_entry_t top_table[] = {
  { "zigbee", &info_zigbee, false },
  { NULL, NULL, false },
};

static sl_cli_command_group_t zigbee_group = {
  { NULL },
  false,
  top_table,
};

void zigbee_cli_init(void)
{
  sl_cli_command_add_command_group(sl_cli_default_handle, &zigbee_group);
}
