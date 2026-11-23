/**
 * @file storage.c
 * @brief Lamp settings in the default NVM3 instance, under one object in the
 *        application (user) key domain: APP_NVM3_KEY_LAMP_SETTINGS. The Zigbee
 *        stack's objects (0x10000-0x1FFFF), the manufacturing tokens and the
 *        user-data page are never touched. Written only on an explicit console
 *        command, so flash wear is negligible.
 */
#include "storage.h"
#include "app_config.h"
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
