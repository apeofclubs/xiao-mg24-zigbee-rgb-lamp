/**
 * @file power_manager.h
 * @brief Sleep policy: boot window, escape pin, later polling profiles and
 *        low-battery behaviour (Phase 5).
 */
#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <stdbool.h>

/** Block EM2 for APP_BOOT_AWAKE_WINDOW_MS, or until reset if the escape pin is LOW. */
void power_manager_init(void);

/** True if the escape pin was held LOW at boot. */
bool power_manager_escape_active(void);

#endif
