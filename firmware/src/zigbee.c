/**
 * @file zigbee.c
 * @brief Start-up and Zigbee network callbacks.
 *
 * Phase 3: endpoint 1 is an Extended Color Light (zigbee_light.c). The
 * firmware never joins a network on its own: joining, leaving and the factory
 * reset are console commands (zigbee_cli.c, docs/zigbee.md). After a reboot a
 * device that was joined rejoins its network (that is the stack, not a new
 * join).
 */
#include "app/framework/include/af.h"
#include "app_config.h"
#include "zigbee_app.h"
#include "rgb_led.h"
#include "battery.h"
#include "power_manager.h"
#include "storage.h"
#include "lamp.h"
#include "lamp_cli.h"
#include "zigbee_light.h"
#include "zigbee_cli.h"

static uint8_t last_lqi;
static int8_t last_rssi;
static uint32_t last_rx_ms;
static bool have_rx;

void sl_zigbee_af_main_init_cb(void)
{
  power_manager_init();   // first: boot window and escape pin
  storage_init();
  rgb_led_init();          // LED pins at their OFF level before anything else
  lamp_init();             // balance and fade from NVM3, LED stays off
  zigbee_light_init();     // last light state restored on the first loop pass
  lamp_cli_init();         // `lamp ...` commands on the RTT console
  zigbee_cli_init();       // `zigbee ...` commands
  battery_init();

  sl_zigbee_app_debug_println("%s %s %s (Phase 3: Zigbee light, console on RTT)",
                              APP_MANUFACTURER_NAME,
                              APP_MODEL_IDENTIFIER,
                              APP_FIRMWARE_VERSION_STRING);
  sl_zigbee_app_debug_println("Recovery: awake for %u ms after reset%s",
                              (unsigned)APP_BOOT_AWAKE_WINDOW_MS,
                              power_manager_escape_active()
                              ? ", escape pin LOW: sleep disabled until reset" : "");
  sl_zigbee_app_debug_println("Not joining on its own: `zigbee join` pairs, `zigbee status` shows the network");
}

static void configure_polling(void)
{
  sl_zigbee_af_set_short_poll_interval_ms_cb(APP_POLL_SHORT_MS);
  sl_zigbee_af_set_long_poll_interval_ms_cb(APP_POLL_LONG_MS);
#if APP_CONSOLE_KEEP_AWAKE
  /* Bench build on USB: always short-poll, a command arrives within
   * APP_POLL_SHORT_MS. */
  sl_zigbee_af_add_to_current_app_tasks(SL_ZIGBEE_AF_FORCE_SHORT_POLL);
#endif
}

void sl_zigbee_af_stack_status_cb(sl_status_t status)
{
  if (status == SL_STATUS_NETWORK_UP) {
    configure_polling();
    sl_zigbee_app_debug_println("Network up: node 0x%04X, parent 0x%04X",
                                sl_zigbee_af_get_node_id(),
                                sl_zigbee_get_parent_node_id());
  } else if (status == SL_STATUS_NETWORK_DOWN) {
    sl_zigbee_app_debug_println("Network down");
  }
}

void sl_zigbee_af_network_steering_complete_cb(sl_status_t status,
                                               uint8_t totalBeacons,
                                               uint8_t joinAttempts,
                                               uint8_t finalState)
{
  (void)finalState;
  sl_zigbee_app_debug_println("Join %s: status 0x%02X, %u beacons, %u attempts",
                              status == SL_STATUS_OK ? "done" : "failed",
                              status, totalBeacons, joinAttempts);
}

/* Every APS message a sleepy end device receives comes through its parent:
 * the last hop is the parent. Kept for `zigbee status`. */
bool sl_zigbee_af_pre_message_received_cb(sl_zigbee_af_incoming_message_t *incomingMessage)
{
  last_lqi = incomingMessage->lastHopLqi;
  last_rssi = incomingMessage->lastHopRssi;
  last_rx_ms = halCommonGetInt32uMillisecondTick();
  have_rx = true;
  return false;   // the framework processes the message as usual
}

bool zigbee_app_is_joined(void)
{
  return sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK;
}

bool zigbee_app_last_link(uint8_t *lqi, int8_t *rssi, uint32_t *age_ms)
{
  if (!have_rx) {
    return false;
  }
  *lqi = last_lqi;
  *rssi = last_rssi;
  *age_ms = halCommonGetInt32uMillisecondTick() - last_rx_ms;
  return true;
}

void zigbee_app_forget_link(void)
{
  have_rx = false;
}
