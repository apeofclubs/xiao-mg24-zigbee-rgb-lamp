/**
 * @file main.c
 * @brief Start-up. Same super-loop as the Simplicity SDK Zigbee SoC samples
 *        (sl_main, bare metal, no RTOS). Application start-up hooks live in
 *        zigbee.c (sl_zigbee_af_main_init_cb).
 */
#include "sl_component_catalog.h"
#include "sl_main_init.h"
#include "sl_main_process_action.h"
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif

int main(void)
{
  sl_main_init();   // device, clocks, NVM3, stack, then sl_zigbee_af_main_init_cb()
  app_init();

  while (1) {
    sl_main_process_action();
    app_process_action();
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    // Sleeps only to the deepest mode currently allowed. EM2 is blocked during
    // the boot window and while the escape pin was held (power_manager.c).
    // This firmware never requests EM4.
    sl_power_manager_sleep();
#endif
  }
}
