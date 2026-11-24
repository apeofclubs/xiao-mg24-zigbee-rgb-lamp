/**
 * @file zigbee_light.h
 * @brief Endpoint 1 light: ZCL attributes -> lamp, Identify, last-state
 *        save (debounced) and restore.
 */
#ifndef ZIGBEE_LIGHT_H
#define ZIGBEE_LIGHT_H

#include <stdbool.h>
#include <stdint.h>

/** Call from sl_zigbee_af_main_init_cb(), after lamp_init(). The last state is
 *  restored on the first pass of the main loop. */
void zigbee_light_init(void);

/** Factory reset of the light: groups, scenes, attributes to their defaults,
 *  last state erased from NVM3 (the balance is kept), LED off. */
void zigbee_light_factory_reset(void);

/** Toggle the light exactly as a ZCL On/Off Toggle command received on
 *  endpoint 1 would (button short press). Works joined or not. Turning on
 *  restores the colour and level held in the attributes. */
bool zigbee_light_toggle(void);

/** true: the button feedback (button.c) drives the LED; Zigbee changes still
 *  update the attributes and the saved state but are not shown. false: the
 *  LED goes back to Identify or to the attribute state. Never saved. */
void zigbee_light_feedback(bool active);

/** Last-state NVM3 writes since boot (bench check of the debounce). */
uint32_t zigbee_light_nvm_write_count(void);

#endif
