/**
 * @file storage.c
 * @brief Lamp settings in the default NVM3 instance, under one object in the
 *        application (user) key domain:
 *        - APP_NVM3_KEY_LAMP_SETTINGS (balance, fade): written only on an
 *          explicit console command;
 *        - APP_NVM3_KEY_LIGHT_STATE (last on/off, level, colour, start-up
 *          attributes): written by zigbee_light.c only after the state has been
 *          stable for APP_STATE_SAVE_DELAY_MS, at most once per
 *          APP_STATE_SAVE_MIN_INTERVAL_MS, and only when it differs from the
 *          stored copy.
 *        The Zigbee stack's objects (0x10000-0x1FFFF), the manufacturing tokens
 *        and the user-data page are never touched.
 */
#include "storage.h"
#include "app_config.h"
#include <string.h>
#include "nvm3_default.h"

#define SETTINGS_VERSION  1u

typedef struct {
  uint8_t version;
  uint8_t balance[3];
  uint16_t fade_ms;
  uint16_t reserved;
} stored_settings_t;

void storage_init(void)
{
  /* The default NVM3 instance is opened by sl_main_init() (token manager). */
}

bool storage_load_lamp_settings(storage_lamp_settings_t *settings)
{
  uint32_t type = 0;
  size_t len = 0;
  if (nvm3_getObjectInfo(nvm3_defaultHandle, APP_NVM3_KEY_LAMP_SETTINGS, &type, &len)
      != SL_STATUS_OK
      || type != NVM3_OBJECTTYPE_DATA
      || len != sizeof(stored_settings_t)) {
    return false;
  }
  stored_settings_t s;
  if (nvm3_readData(nvm3_defaultHandle, APP_NVM3_KEY_LAMP_SETTINGS, &s, sizeof(s))
      != SL_STATUS_OK
      || s.version != SETTINGS_VERSION
      || s.balance[0] > 100u || s.balance[1] > 100u || s.balance[2] > 100u
      || s.fade_ms > APP_LED_FADE_MAX_MS) {
    return false;
  }
  for (uint8_t i = 0; i < 3u; i++) {
    settings->balance[i] = s.balance[i];
  }
  settings->fade_ms = s.fade_ms;
  return true;
}

bool storage_save_lamp_settings(const storage_lamp_settings_t *settings)
{
  stored_settings_t s = {
    .version = SETTINGS_VERSION,
    .balance = { settings->balance[0], settings->balance[1], settings->balance[2] },
    .fade_ms = settings->fade_ms,
    .reserved = 0,
  };
  return nvm3_writeData(nvm3_defaultHandle, APP_NVM3_KEY_LAMP_SETTINGS, &s, sizeof(s))
         == SL_STATUS_OK;
}

#define LIGHT_STATE_VERSION  1u

typedef struct {
  uint8_t version;
  uint8_t reserved[3];
  storage_light_state_t state;
} stored_light_state_t;

bool storage_load_light_state(storage_light_state_t *state)
{
  uint32_t type = 0;
  size_t len = 0;
  if (nvm3_getObjectInfo(nvm3_defaultHandle, APP_NVM3_KEY_LIGHT_STATE, &type, &len)
      != SL_STATUS_OK
      || type != NVM3_OBJECTTYPE_DATA
      || len != sizeof(stored_light_state_t)) {
    return false;
  }
  stored_light_state_t s;
  if (nvm3_readData(nvm3_defaultHandle, APP_NVM3_KEY_LIGHT_STATE, &s, sizeof(s))
      != SL_STATUS_OK
      || s.version != LIGHT_STATE_VERSION
      || s.state.on_off > 1u
      || s.state.level == 0u || s.state.level > 254u
      || s.state.color_mode > 2u
      || s.state.hue > 254u || s.state.saturation > 254u) {
    return false;
  }
  *state = s.state;
  return true;
}

bool storage_save_light_state(const storage_light_state_t *state)
{
  stored_light_state_t s;
  memset(&s, 0, sizeof(s));
  s.version = LIGHT_STATE_VERSION;
  s.state = *state;
  return nvm3_writeData(nvm3_defaultHandle, APP_NVM3_KEY_LIGHT_STATE, &s, sizeof(s))
         == SL_STATUS_OK;
}

bool storage_erase_light_state(void)
{
  sl_status_t st = nvm3_deleteObject(nvm3_defaultHandle, APP_NVM3_KEY_LIGHT_STATE);
  return st == SL_STATUS_OK || st == SL_STATUS_NOT_FOUND;
}
