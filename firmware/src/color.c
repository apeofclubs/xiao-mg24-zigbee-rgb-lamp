/**
 * @file color.c
 * @brief Colour conversions (see color.h).
 *
 * xy -> RGB uses the sRGB primaries and D65 white (IEC 61966-2-1): the LED's
 * real primaries are unknown, and Zigbee2MQTT / Home Assistant compute xy from
 * sRGB colours, so a colour picked in HA comes back as the same RGB triple.
 * The linear result is gamma-encoded (1/2.2) because lamp.c applies gamma 2.2
 * to the colour x level before the PWM: one gamma, in one place.
 *
 * Colour temperature -> xy: Kim et al. cubic spline of the Planckian locus
 * (1667 K..25000 K), the approximation used by most open-source light
 * firmware. Then the same xy -> RGB path.
 */
#include "color.h"
#include <math.h>

lamp_rgb_t color_from_hsv(uint16_t hue, uint8_t sat, uint8_t val)
{
  hue %= 360u;
  if (sat > 100u) {
    sat = 100u;
  }
  if (val > 100u) {
    val = 100u;
  }
  uint32_t v = (uint32_t)val * 255u / 100u;
  uint32_t s = sat;
  uint32_t region = hue / 60u;
  uint32_t rem = (uint32_t)(hue % 60u) * 255u / 60u;       /* 0..250 */
  uint8_t p = (uint8_t)(v * (100u - s) / 100u);
  uint8_t q = (uint8_t)(v * (100u * 255u - s * rem) / (100u * 255u));
  uint8_t t = (uint8_t)(v * (100u * 255u - s * (255u - rem)) / (100u * 255u));
  lamp_rgb_t c;
  switch (region) {
    case 0:  c.r = (uint8_t)v; c.g = t; c.b = p; break;
    case 1:  c.r = q; c.g = (uint8_t)v; c.b = p; break;
    case 2:  c.r = p; c.g = (uint8_t)v; c.b = t; break;
    case 3:  c.r = p; c.g = q; c.b = (uint8_t)v; break;
    case 4:  c.r = t; c.g = p; c.b = (uint8_t)v; break;
    default: c.r = (uint8_t)v; c.g = p; c.b = q; break;
  }
  return c;
}

lamp_rgb_t color_from_zcl_hs(uint8_t hue, uint8_t sat)
{
  if (hue > 254u) {
    hue = 254u;
  }
  if (sat > 254u) {
    sat = 254u;
  }
  uint16_t deg = (uint16_t)(((uint32_t)hue * 360u + 127u) / 254u);
  uint8_t pct = (uint8_t)(((uint32_t)sat * 100u + 127u) / 254u);
  return color_from_hsv(deg, pct, 100u);
}

static lamp_rgb_t rgb_from_xy_float(float x, float y)
{
  const lamp_rgb_t white = { 255, 255, 255 };
  if (y < 0.001f || x < 0.0f || x + y > 1.0f) {
    return white;
  }
  /* XYZ with luminance Y = 1. */
  float X = x / y;
  float Z = (1.0f - x - y) / y;
  float lin[3] = {
     3.2406f * X - 1.5372f - 0.4986f * Z,
    -0.9689f * X + 1.8758f + 0.0415f * Z,
     0.0557f * X - 0.2040f + 1.0570f * Z,
  };
  float max = 0.0f;
  for (int i = 0; i < 3; i++) {
    if (lin[i] < 0.0f) {
      lin[i] = 0.0f;   /* outside the sRGB gamut: nearest displayable colour */
    }
    if (lin[i] > max) {
      max = lin[i];
    }
  }
  if (max <= 0.0f) {
    return white;
  }
  uint8_t out[3];
  for (int i = 0; i < 3; i++) {
    float v = powf(lin[i] / max, 1.0f / 2.2f);
    out[i] = (uint8_t)(v * 255.0f + 0.5f);
  }
  lamp_rgb_t c = { out[0], out[1], out[2] };
  return c;
}

lamp_rgb_t color_from_zcl_xy(uint16_t x, uint16_t y)
{
  return rgb_from_xy_float((float)x / 65536.0f, (float)y / 65536.0f);
}

lamp_rgb_t color_from_mireds(uint16_t mireds)
{
  if (mireds < 40u) {
    mireds = 40u;       /* 25000 K */
  } else if (mireds > 600u) {
    mireds = 600u;      /* 1667 K */
  }
  float t = 1.0e6f / (float)mireds;
  float t2 = t * t;
  float t3 = t2 * t;
  float x;
  if (t <= 4000.0f) {
    x = -0.2661239e9f / t3 - 0.2343589e6f / t2 + 0.8776956e3f / t + 0.179910f;
  } else {
    x = -3.0258469e9f / t3 + 2.1070379e6f / t2 + 0.2226347e3f / t + 0.240390f;
  }
  float x2 = x * x;
  float x3 = x2 * x;
  float y;
  if (t <= 2222.0f) {
    y = -1.1063814f * x3 - 1.34811020f * x2 + 2.18555832f * x - 0.20219683f;
  } else if (t <= 4000.0f) {
    y = -0.9549476f * x3 - 1.37418593f * x2 + 2.09137015f * x - 0.16748867f;
  } else {
    y = 3.0817580f * x3 - 5.87338670f * x2 + 3.75112997f * x - 0.37001483f;
  }
  return rgb_from_xy_float(x, y);
}
