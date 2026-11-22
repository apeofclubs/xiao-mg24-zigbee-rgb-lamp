/**
 * @file zigbee_app.h
 * @brief Zigbee network, clusters and reporting (Phase 3 fills this in).
 */
#ifndef ZIGBEE_APP_H
#define ZIGBEE_APP_H

#include <stdbool.h>

/** True once the device is joined to a network. */
bool zigbee_app_is_joined(void);

#endif
