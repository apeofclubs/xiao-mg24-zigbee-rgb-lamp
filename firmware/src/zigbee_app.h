/**
 * @file zigbee_app.h
 * @brief Zigbee network helpers (zigbee.c).
 */
#ifndef ZIGBEE_APP_H
#define ZIGBEE_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "sl_status.h"

/** True once the device is joined to a network. */
bool zigbee_app_is_joined(void);

/** LQI and RSSI of the last message received (the parent is the last hop of
 *  every message a sleepy end device receives), and its age. False if none
 *  since boot or since the last leave. */
bool zigbee_app_last_link(uint8_t *lqi, int8_t *rssi, uint32_t *age_ms);

/** Forget the last link figures (after a leave). */
void zigbee_app_forget_link(void);

/** True while network steering runs (started by `zigbee join` or the button)
 *  or the stack is joining. */
bool zigbee_app_is_joining(void);

/** Start network steering. SL_STATUS_INVALID_STATE if already on a network
 *  (joined, parent lost, leaving) or already joining. The lamp never calls
 *  this on its own: only `zigbee join` and a short press on the button. */
sl_status_t zigbee_app_join(void);

/** Factory reset: stop steering, leave the network, clear bindings,
 *  reporting, groups, scenes, light attributes and the saved last state (NVM3
 *  key APP_NVM3_KEY_LIGHT_STATE). The balance (APP_NVM3_KEY_LAMP_SETTINGS) is
 *  kept. Returns the leave status (SL_STATUS_OK if there was no network). */
sl_status_t zigbee_app_factory_reset(void);

#endif
