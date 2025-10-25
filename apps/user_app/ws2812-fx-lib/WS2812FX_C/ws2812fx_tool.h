#ifndef ws2812fx_tool_h
#define ws2812fx_tool_h

// mode helper functions
uint16_t
  WS2812FX_blink(uint32_t, uint32_t, uint8_t strobe),
  WS2812FX_color_wipe(uint32_t, uint32_t, uint8_t),
  WS2812FX_twinkle(uint32_t, uint32_t),
  WS2812FX_twinkle_fade(uint32_t),
  WS2812FX_sparkle(uint32_t, uint32_t),
  WS2812FX_chase(uint32_t, uint32_t, uint32_t),
  WS2812FX_chase_flash(uint32_t, uint32_t),
  WS2812FX_running(uint32_t, uint32_t),
  WS2812FX_fireworks(uint32_t),
  WS2812FX_fire_flicker(int),
  WS2812FX_tricolor_chase(uint32_t, uint32_t, uint32_t),
  WS2812FX_scan(uint32_t, uint32_t, uint8_t);

uint32_t
  WS2812FX_color_blend(uint32_t, uint32_t, uint8_t);

#endif