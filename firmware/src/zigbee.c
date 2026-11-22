/**
 * @file zigbee.c
 * @brief Zigbee application callbacks.
 *
 * Phase 1: the stack runs as a sleepy end device but the firmware never starts
 * network steering on its own. Joining for a bench test is done from the
 * console: `plugin network-steering start 0`. On/Off, Level Control, Color
 * Control and Power Configuration servers arrive in Phases 3 and 4.
 */
#include "app/framework/include/af.h"
#include "app_config.h"
#include "zigbee_app.h"
#include "rgb_led.h"
#include "battery.h"
#include "power_manager.h"
#include "storage.h"

void sl_zigbee_af_main_init_cb(void)
{
  power_manager_init();   // first: boot window and escape pin
  storage_init();
  rgb_led_init();
  battery_init();

  sl_zigbee_app_debug_println("%s %s %s (Phase 1 skeleton)",
                              APP_MANUFACTURER_NAME,
                              APP_MODEL_IDENTIFIER,
                              APP_FIRMWARE_VERSION_STRING);
  sl_zigbee_app_debug_println("Recovery: awake for %u ms after reset%s",
                              (unsigned)APP_BOOT_AWAKE_WINDOW_MS,
                              power_manager_escape_active()
                              ? ", escape pin LOW: sleep disabled until reset" : "");
}

void sl_zigbee_af_stack_status_cb(sl_status_t status)
{
  if (status == SL_STATUS_NETWORK_UP) {
    sl_zigbee_app_debug_println("Network up");
  } else if (status == SL_STATUS_NETWORK_DOWN) {
    sl_zigbee_app_debug_println("Network down");
  }
}

void sl_zigbee_af_network_steering_complete_cb(sl_status_t status,
                                               uint8_t totalBeacons,
                                               uint8_t joinAttempts,
                                               uint8_t finalState)
{
  (void)totalBeacons;
  (void)joinAttempts;
  (void)finalState;
  sl_zigbee_app_debug_println("Join complete: 0x%02X", status);
}

bool zigbee_app_is_joined(void)
{
  return sl_zigbee_af_network_state() == SL_ZIGBEE_JOINED_NETWORK;
}
