/**
 * @file zigbee.c
 * @brief Start-up and Zigbee network callbacks.
 *
 * Phase 3: endpoint 1 is an Extended Color Light (zigbee_light.c). The
 * firmware never joins a network on its own: joining, leaving and the factory
 * reset are console commands (zigbee_cli.c); pairing is also a 5 s hold on the
 * D9 button (button.c), see docs/zigbee.md. After a reboot a
 * device that was joined rejoins its network (that is the stack, not a new
 * join).
 */
#include "app/framework/include/af.h"
#include "app/framework/plugin/network-steering/network-steering.h"
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
#include "button.h"
#include "fault.h"

static uint8_t last_lqi;
static int8_t last_rssi;
static uint32_t last_rx_ms;
static bool have_rx;
static bool steering;   /* network steering running (started by us) */
static bool leaving_to_pair;   /* zigbee_app_pair(): waiting for the leave */
static uint32_t leave_ms;
static sl_zigbee_af_event_t pair_event;

static void pair_handler(sl_zigbee_af_event_t *event);

void sl_zigbee_af_main_init_cb(void)
{
  fault_init();           // crash details on RTT from here on
  power_manager_init();   // first: boot window and escape pin
  sl_zigbee_af_event_init(&pair_event, pair_handler);
  storage_init();
  rgb_led_init();          // LED pins at their OFF level before anything else
  lamp_init();             // balance and fade from NVM3, LED stays off
  zigbee_light_init();     // last light state restored on the first loop pass
  lamp_cli_init();         // `lamp ...` commands on the RTT console
  zigbee_cli_init();       // `zigbee ...` commands
  battery_init();
  button_init();           // D9 on/off and pairing button

  sl_zigbee_app_debug_println("%s %s %s (Phase 3: Zigbee light, console on RTT)",
                              APP_MANUFACTURER_NAME,
                              APP_MODEL_IDENTIFIER,
                              APP_FIRMWARE_VERSION_STRING);
  sl_zigbee_app_debug_println("Recovery: awake for %u ms after reset%s",
                              (unsigned)APP_BOOT_AWAKE_WINDOW_MS,
                              power_manager_escape_active()
                              ? ", escape pin LOW: sleep disabled until reset" : "");
  fault_report();          // the last crash, if the last reset was one
  sl_zigbee_app_debug_println("Not joining on its own: `zigbee join` or holding D9 for 5 s pairs, "
                              "`zigbee status` shows the network");
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
  steering = false;
  button_join_finished(status == SL_STATUS_OK);
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

bool zigbee_app_is_joining(void)
{
  return steering || leaving_to_pair
         || sl_zigbee_af_network_state() == SL_ZIGBEE_JOINING_NETWORK;
}

sl_status_t zigbee_app_join(void)
{
  if (sl_zigbee_af_network_state() != SL_ZIGBEE_NO_NETWORK || steering || leaving_to_pair) {
    return SL_STATUS_INVALID_STATE;
  }
  sl_status_t s = sl_zigbee_af_network_steering_start();
  steering = (s == SL_STATUS_OK);
  return s;
}

/* Waits for the leave started by zigbee_app_pair(), then starts steering.
 * Polled with an application event (no busy wait, the lamp may sleep). */
static void pair_handler(sl_zigbee_af_event_t *event)
{
  (void)event;
  if (!leaving_to_pair) {
    return;
  }
  if (sl_zigbee_af_network_state() == SL_ZIGBEE_NO_NETWORK) {
    leaving_to_pair = false;
    sl_status_t s = zigbee_app_join();
    sl_zigbee_app_debug_println("Pair: old network left, join %s (0x%02lX)",
                                s == SL_STATUS_OK ? "searching" : "could not start",
                                (unsigned long)s);
    if (s != SL_STATUS_OK) {
      button_join_finished(false);
    }
  } else if ((halCommonGetInt32uMillisecondTick() - leave_ms) >= APP_PAIR_LEAVE_TIMEOUT_MS) {
    leaving_to_pair = false;
    sl_zigbee_app_debug_println("Pair: the leave did not complete in %u ms, pairing abandoned",
                                (unsigned)APP_PAIR_LEAVE_TIMEOUT_MS);
    button_join_finished(false);
  } else {
    sl_zigbee_af_event_set_delay_ms(&pair_event, APP_PAIR_LEAVE_POLL_MS);
  }
}

sl_status_t zigbee_app_pair(void)
{
  if (zigbee_app_is_joining()) {
    return SL_STATUS_INVALID_STATE;
  }
  sl_zigbee_network_status_t st = sl_zigbee_af_network_state();
  if (st == SL_ZIGBEE_NO_NETWORK) {
    return zigbee_app_join();
  }
  if (st != SL_ZIGBEE_LEAVING_NETWORK) {
    sl_status_t s = sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_NO_OPTION);
    if (s != SL_STATUS_OK) {
      return s;
    }
  }
  /* Only what belongs to the old network: its link figures and the bindings
   * (they point at the old coordinator; Zigbee2MQTT binds again at the
   * interview). Light state, reporting configuration, groups, scenes, balance
   * and fade are kept; `zigbee reset` clears them. */
  zigbee_app_forget_link();
  (void)sl_zigbee_clear_binding_table();
  leaving_to_pair = true;
  leave_ms = halCommonGetInt32uMillisecondTick();
  sl_zigbee_af_event_set_delay_ms(&pair_event, APP_PAIR_LEAVE_POLL_MS);
  sl_zigbee_app_debug_println("Pair: leaving the current network first");
  return SL_STATUS_OK;
}

sl_status_t zigbee_app_factory_reset(void)
{
  sl_status_t s = SL_STATUS_OK;
  if (leaving_to_pair) {
    leaving_to_pair = false;
    sl_zigbee_af_event_set_inactive(&pair_event);
    button_join_finished(false);
  }
  if (steering) {
    /* Calls sl_zigbee_af_network_steering_complete_cb() with a failure. */
    (void)sl_zigbee_af_network_steering_stop();
    steering = false;
  }
  if (sl_zigbee_af_network_state() != SL_ZIGBEE_NO_NETWORK) {
    s = sl_zigbee_leave_network(SL_ZIGBEE_LEAVE_NWK_WITH_NO_OPTION);
  }
  zigbee_app_forget_link();
  (void)sl_zigbee_clear_binding_table();
  sl_zigbee_af_clear_report_table_cb();
  zigbee_light_factory_reset();
  sl_zigbee_app_debug_println("Factory reset: network left (0x%02lX), bindings, reporting, "
                              "groups, scenes and last state cleared; balance kept",
                              (unsigned long)s);
  return s;
}
