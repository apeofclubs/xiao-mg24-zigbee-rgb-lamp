/**
 * @file power_manager.c
 * @brief Keeps the board recoverable.
 *
 * - After every reset, an EM1 requirement blocks EM2/EM3 for
 *   APP_BOOT_AWAKE_WINDOW_MS, so a debugger can always attach.
 * - If the escape pad (D1 = PC01) is LOW at boot, the requirement is never
 *   released: the chip does not sleep until the next reset.
 * - EM4 is never used by this firmware.
 */
#include "power_manager.h"
#include "app_config.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_power_manager.h"
#include "sl_sleeptimer.h"

static sl_sleeptimer_timer_handle_t boot_window_timer;
static bool escape_active;

static void boot_window_elapsed(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;
  sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
}

void power_manager_init(void)
{
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);

  CMU_ClockEnable(cmuClock_GPIO, true);
  // Input with pull-up (DOUT = 1 selects pull-up), filter on.
  GPIO_PinModeSet(APP_ESCAPE_PORT, APP_ESCAPE_PIN, gpioModeInputPullFilter, 1);
  // Let the pull-up settle before sampling.
  sl_sleeptimer_delay_millisecond(2);
  escape_active = (GPIO_PinInGet(APP_ESCAPE_PORT, APP_ESCAPE_PIN) == 0);
  // Pin back to disabled to save the pull-up current.
  GPIO_PinModeSet(APP_ESCAPE_PORT, APP_ESCAPE_PIN, gpioModeDisabled, 0);

  if (!escape_active) {
    sl_sleeptimer_start_timer_ms(&boot_window_timer,
                                 APP_BOOT_AWAKE_WINDOW_MS,
                                 boot_window_elapsed,
                                 NULL,
                                 0,
                                 0);
  }
}

bool power_manager_escape_active(void)
{
  return escape_active;
}
