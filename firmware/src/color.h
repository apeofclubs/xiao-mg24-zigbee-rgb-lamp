/**
 * @file color.h
 * @brief Colour conversions for the lamp: HSV, CIE 1931 xy and colour
 *        temperature to an RGB colour (0..255 per channel, brightest channel
 *        at 255; brightness is the level's job, lamp.c).
 */
#ifndef COLOR_H
#define COLOR_H

#include <stdint.h>
#include "lamp.h"

/** hue 0..359 degrees, saturation 0..100, value 0..100. */
lamp_rgb_t color_from_hsv(uint16_t hue, uint8_t sat, uint8_t val);

/** Zigbee CurrentHue / CurrentSaturation (0..254 each, hue 254 = 360 degrees). */
lamp_rgb_t color_from_zcl_hs(uint8_t hue, uint8_t sat);

/** Zigbee CurrentX / CurrentY (value / 65536 = CIE 1931 x, y). */
lamp_rgb_t color_from_zcl_xy(uint16_t x, uint16_t y);

/** Colour temperature in mireds (1 000 000 / kelvin). */
lamp_rgb_t color_from_mireds(uint16_t mireds);

#endif
