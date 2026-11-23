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

void storage_init(void);

/** Fill *settings from NVM3. Returns false (and leaves *settings untouched)
 *  when nothing valid is stored yet. */
bool storage_load_lamp_settings(storage_lamp_settings_t *settings);

/** Write the settings to NVM3. Returns true on success. */
bool storage_save_lamp_settings(const storage_lamp_settings_t *settings);

#endif
