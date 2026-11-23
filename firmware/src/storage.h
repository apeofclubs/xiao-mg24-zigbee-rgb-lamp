/**
 * @file storage.h
 * @brief Persistent lamp settings in NVM3 (application key domain only).
 *        ZCL attributes with NVM storage are handled by the framework.
 */
#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint8_t balance[3];   /* red, green, blue: percent 0..100 */
  uint16_t fade_ms;     /* default transition time */
} storage_lamp_settings_t;

/* Last light state, as the Zigbee attributes hold it (src/zigbee_light.c). */
typedef struct {
  uint8_t on_off;             /* OnOff 0/1 */
  uint8_t level;              /* CurrentLevel 1..254 */
  uint8_t color_mode;         /* ColorMode: 0 HS, 1 XY, 2 colour temperature */
  uint8_t hue;                /* CurrentHue 0..254 */
  uint8_t saturation;         /* CurrentSaturation 0..254 */
  uint8_t start_up_on_off;    /* StartUpOnOff 0, 1, 2, 0xFF */
  uint8_t start_up_level;     /* StartUpCurrentLevel 0..254, 0xFF */
  uint8_t on_level;           /* OnLevel 1..254, 0xFF */
  uint16_t x;                 /* CurrentX */
  uint16_t y;                 /* CurrentY */
  uint16_t mireds;            /* ColorTemperatureMireds */
  uint16_t start_up_mireds;   /* StartUpColorTemperatureMireds, 0xFFFF = previous */
  uint16_t on_off_transition; /* OnOffTransitionTime, 1/10 s */
} storage_light_state_t;

void storage_init(void);

/** Fill *settings from NVM3. Returns false (and leaves *settings untouched)
 *  when nothing valid is stored yet. */
bool storage_load_lamp_settings(storage_lamp_settings_t *settings);

/** Write the settings to NVM3. Returns true on success. */
bool storage_save_lamp_settings(const storage_lamp_settings_t *settings);

/** Fill *state from NVM3 (key APP_NVM3_KEY_LIGHT_STATE). False if none valid. */
bool storage_load_light_state(storage_light_state_t *state);

/** Write the last light state. Callers debounce (zigbee_light.c). */
bool storage_save_light_state(const storage_light_state_t *state);

/** Delete the last light state (factory reset). The balance is kept. */
bool storage_erase_light_state(void);

#endif
