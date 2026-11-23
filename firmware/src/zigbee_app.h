/**
 * @file zigbee_app.h
 * @brief Zigbee network helpers (zigbee.c).
 */
#ifndef ZIGBEE_APP_H
#define ZIGBEE_APP_H

#include <stdbool.h>
#include <stdint.h>

/** True once the device is joined to a network. */
bool zigbee_app_is_joined(void);

/** LQI and RSSI of the last message received (the parent is the last hop of
 *  every message a sleepy end device receives), and its age. False if none
 *  since boot or since the last leave. */
bool zigbee_app_last_link(uint8_t *lqi, int8_t *rssi, uint32_t *age_ms);

/** Forget the last link figures (after a leave). */
void zigbee_app_forget_link(void);

#endif
