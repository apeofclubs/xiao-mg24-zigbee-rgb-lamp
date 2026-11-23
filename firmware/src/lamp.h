/**
 * @file lamp.h
 * @brief Lamp state: colour, brightness, per-channel balance, fades and
 *        effects. Phase 2: driven from the console only (lamp_cli.c); the
 *        Zigbee clusters call the same functions from Phase 3 on.
 */
#ifndef LAMP_H
#define LAMP_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  LAMP_EFFECT_NONE = 0,
  LAMP_EFFECT_RAINBOW,
  LAMP_EFFECT_BREATHE,
} lamp_effect_t;

typedef struct {
  uint8_t r, g, b;
} lamp_rgb_t;

/** Load the balance and fade time from NVM3, LED off. Call once at start-up. */
void lamp_init(void);

/* Every setter below stops a running effect first. */

/** Set the colour (0..255 per channel) and turn the lamp on, with a fade. */
void lamp_set_rgb(uint8_t r, uint8_t g, uint8_t b);

/** Set the colour from hue 0..359, saturation 0..100, value 0..100. */
void lamp_set_hsv(uint16_t hue, uint8_t sat, uint8_t val);

/** Brightness 0..100 %, gamma-corrected. 0 turns the lamp off (and keeps the
 *  previous non-zero level for the next "on"). */
void lamp_set_level(uint8_t percent);

/** Fade to off. */
void lamp_off(void);

/** Set a named colour. Returns false if the name is unknown. */
bool lamp_set_preset(const char *name);

/** Name of preset number index, or NULL past the last one. */
const char *lamp_preset_name(uint32_t index);

/** Start an effect (or LAMP_EFFECT_NONE to stop it, the lamp keeps its colour). */
void lamp_set_effect(lamp_effect_t effect);

/** Per-channel balance 0..100 %, applied after gamma. Saved to NVM3. */
bool lamp_set_balance(uint8_t r, uint8_t g, uint8_t b);
void lamp_get_balance(uint8_t balance[3]);

/** Default fade time in ms (0 = instant). Saved to NVM3. */
bool lamp_set_fade_ms(uint16_t ms);
uint16_t lamp_get_fade_ms(void);

typedef struct {
  bool on;
  uint8_t level;
  lamp_rgb_t color;
  lamp_effect_t effect;
  bool fading;
} lamp_state_t;

void lamp_get_state(lamp_state_t *state);

#endif
