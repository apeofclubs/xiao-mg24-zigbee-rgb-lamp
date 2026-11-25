/**
 * @file battery.c
 * @brief Phase 1 stub. PD03 (VBAT_CTL) and PD04 (VBAT_ADC) are not touched
 *        until the divider circuit is confirmed (docs/hardware-assumptions.md).
 */
#include "battery.h"

void battery_init(void)
{
}

uint16_t battery_voltage_mv(void)
{
  return 0;
}

uint8_t battery_percentage_half(uint16_t millivolts)
{
  (void)millivolts;
  return 0xFF;   /* "invalid / unknown" in ZCL BatteryPercentageRemaining */
}
