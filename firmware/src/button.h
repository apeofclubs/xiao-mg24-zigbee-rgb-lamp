/**
 * @file button.h
 * @brief Pairing / factory-reset button on pad D9 (PA04) to GND (button.c,
 *        docs/zigbee.md section 8).
 */
#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

/** Configure D9 (input, pull-up, filter, edge interrupt). Call from
 *  sl_zigbee_af_main_init_cb(), after zigbee_light_init(). */
void button_init(void);

/** Called when network steering ends; shows the result if the button started it. */
void button_join_finished(bool joined);

#endif
