/**
 * @file main.c
 * @brief Start-up. Same super-loop as the Simplicity SDK Zigbee SoC samples
 *        (sl_main, bare metal, no RTOS). Application start-up hooks live in
 *        zigbee.c (sl_zigbee_af_main_init_cb).
 *
 * Derived from the Simplicity SDK sample zigbee_app/z3/zigbee_z3_switch/main.c:
 *   Copyright 2025 Silicon Laboratories Inc. www.silabs.com
 *   SPDX-License-Identifier: Zlib
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software. Permission is granted to anyone to use this software
 * for any purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software in a
 *    product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * Modified for this project: sl_main_custom_main super-loop without RTOS.
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
