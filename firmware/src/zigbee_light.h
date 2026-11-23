/**
 * @file zigbee_light.h
 * @brief Endpoint 1 light: ZCL attributes -> lamp, Identify, last-state
 *        save (debounced) and restore.
 */
#ifndef ZIGBEE_LIGHT_H
#define ZIGBEE_LIGHT_H

#include <stdint.h>

/** Call from sl_zigbee_af_main_init_cb(), after lamp_init(). The last state is
 *  restored on the first pass of the main loop. */
void zigbee_light_init(void);

/** Factory reset of the light: groups, scenes, attributes to their defaults,
 *  last state erased from NVM3 (the balance is kept), LED off. */
void zigbee_light_factory_reset(void);

/** Last-state NVM3 writes since boot (bench check of the debounce). */
uint32_t zigbee_light_nvm_write_count(void);

#endif
