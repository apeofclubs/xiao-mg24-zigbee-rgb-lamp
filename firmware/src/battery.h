/**
 * @file battery.h
 * @brief Battery voltage (IADC on PD04, divider enabled by PD03) and
 *        voltage-to-percentage conversion (Phase 4).
 */
#ifndef BATTERY_H
#define BATTERY_H

#include <stdbool.h>
#include <stdint.h>

void battery_init(void);

/** Measured battery voltage in mV, or 0 when not measured (Phase 1). */
uint16_t battery_voltage_mv(void);

/** Remaining charge, 0..200 in half-percent (ZCL 0x0021 encoding), or 0xFF unknown. */
uint8_t battery_percentage_half(uint16_t millivolts);

#endif
