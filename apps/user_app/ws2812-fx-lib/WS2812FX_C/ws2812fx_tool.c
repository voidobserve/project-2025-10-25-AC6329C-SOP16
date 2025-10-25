/* 实现效果用到的工具 */
#include "WS2812FX.h"
#include "Adafruit_NeoPixel.h"
#include <stdlib.h>
#include <string.h>
extern  Segment* _seg; 
extern  uint16_t _seg_len;
extern Segment_runtime* _seg_rt;
extern uint8_t _running, _triggered;
/*
 * Color wipe function，流水效果
 * LEDs are turned on (color1) in sequence, then turned off (color2) in sequence.
 * if (uint8_t rev == true) then LEDs are turned off in reverse order
 * 颜色擦除功能，用color1依次点亮LED,再用color2依次擦除
 * rev = 0:两个颜色同向， =1color2反方向擦除
 * IS_REVERSE:流水方向
 */
uint16_t WS2812FX_color_wipe(uint32_t color1, uint32_t color2, uint8_t rev) {
  if(_seg_rt->counter_mode_step < _seg_len) {
    uint32_t led_offset = _seg_rt->counter_mode_step;
    if(IS_REVERSE) {
      WS2812FX_setPixelColor(_seg->stop - led_offset, color1);
    } else {
      WS2812FX_setPixelColor(_seg->start + led_offset, color1);
    }
  } else {
    uint32_t led_offset = _seg_rt->counter_mode_step - _seg_len;
    if((IS_REVERSE && !rev) || (!IS_REVERSE && rev)) {
      WS2812FX_setPixelColor(_seg->stop - led_offset, color2);
    } else {
      WS2812FX_setPixelColor(_seg->start + led_offset, color2);
    }
  }

  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % (_seg_len * 2);

  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;

  return (_seg->speed / (_seg_len * 2));
}




/*
 * scan function - runs a block of pixels back and forth.
  dual = 0:一个颜色块来回运动
 dual = 1:两个颜色块来开合运动
 */
uint16_t WS2812FX_scan(uint32_t color1, uint32_t color2, uint8_t dual) {
  int8_t dir = _seg_rt->aux_param ? -1 : 1;//决定运动方向
  uint8_t size = 1 << SIZE_OPTION;

  Adafruit_NeoPixel_fill(color2, _seg->start, _seg_len);

  for(uint8_t i = 0; i < size; i++) {
    if(IS_REVERSE || dual) {
      WS2812FX_setPixelColor(_seg->stop - _seg_rt->counter_mode_step - i, color1);
    }
    if(!IS_REVERSE || dual) {
      WS2812FX_setPixelColor(_seg->start + _seg_rt->counter_mode_step + i, color1);
    }
  }

  _seg_rt->counter_mode_step += dir;
  if(_seg_rt->counter_mode_step == 0) {
    _seg_rt->aux_param = 0;
    SET_CYCLE;
  }
  if(_seg_rt->counter_mode_step >= (uint16_t)(_seg_len - size)) _seg_rt->aux_param = 1;

  return (_seg->speed );
}



/*
 * Tricolor chase function //三色追逐函数
 */
uint16_t WS2812FX_tricolor_chase(uint32_t color1, uint32_t color2, uint32_t color3) {
  uint8_t sizeCnt = 1 << SIZE_OPTION;
  uint8_t sizeCnt2 = sizeCnt + sizeCnt;
  uint8_t sizeCnt3 = sizeCnt2 + sizeCnt;
  uint16_t index = _seg_rt->counter_mode_step % sizeCnt3;
  for(uint16_t i=0; i < _seg_len; i++, index++) {
    index = index % sizeCnt3;

    uint32_t color = color3;
    if(index < sizeCnt) color = color1;
    else if(index < sizeCnt2) color = color2;

    if(IS_REVERSE) {
      WS2812FX_setPixelColor(_seg->start + i, color);
    } else {
      WS2812FX_setPixelColor(_seg->stop - i, color);
    }
  }

  _seg_rt->counter_mode_step++;
  if(_seg_rt->counter_mode_step % _seg_len == 0) SET_CYCLE;

  return (_seg->speed / _seg_len);
}


/*
 * twinkle function
 */
uint16_t WS2812FX_twinkle(uint32_t color1, uint32_t color2) {
  if(_seg_rt->counter_mode_step == 0) {
    Adafruit_NeoPixel_fill(color2, _seg->start, _seg_len);
    uint16_t min_leds = (_seg_len / 4) + 1; // make sure, at least one LED is on
    _seg_rt->counter_mode_step = WS2812FX_random16_lim(min_leds * 2);//random(min_leds, min_leds * 2);
    SET_CYCLE;
  }

  WS2812FX_setPixelColor(_seg->start + WS2812FX_random16_lim(_seg_len), color1);

  _seg_rt->counter_mode_step--;
  return (_seg->speed / _seg_len);
}



/*
 * fade out functions
 灯带颜色淡出，设置FADE_RATE和_seg->colors[1]，可以改变淡出颜色
 */
void WS2812FX_fade_out() {
  /*return*/ WS2812FX_fade_out_targetColor(_seg->colors[1]);
}

void WS2812FX_fade_out_targetColor(uint32_t targetColor) {
  static const uint8_t rateMapH[] = {0, 1, 1, 1, 2, 3, 4, 6};
  static const uint8_t rateMapL[] = {0, 2, 3, 8, 8, 8, 8, 8};

  uint8_t rate  = FADE_RATE;
  uint8_t rateH = rateMapH[rate];
  uint8_t rateL = rateMapL[rate];

  uint32_t color = targetColor;
  int w2 = (color >> 24) & 0xff;
  int r2 = (color >> 16) & 0xff;
  int g2 = (color >>  8) & 0xff;
  int b2 =  color        & 0xff;

  for(uint16_t i=_seg->start; i <= _seg->stop; i++) {
    color = Adafruit_NeoPixel_getPixelColor(i); // current color
    if(rate == 0) { // old fade-to-black algorithm
      WS2812FX_setPixelColor(i, (color >> 1) & 0x7F7F7F7F);
    } else { // new fade-to-color algorithm
      int w1 = (color >> 24) & 0xff;
      int r1 = (color >> 16) & 0xff;
      int g1 = (color >>  8) & 0xff;
      int b1 =  color        & 0xff;

      // calculate the color differences between the current and target colors
      int wdelta = w2 - w1;
      int rdelta = r2 - r1;
      int gdelta = g2 - g1;
      int bdelta = b2 - b1;

      // if the current and target colors are almost the same, jump right to the target
      // color, otherwise calculate an intermediate color. (fixes rounding issues)
      wdelta = abs(wdelta) < 3 ? wdelta : (wdelta >> rateH) + (wdelta >> rateL);
      rdelta = abs(rdelta) < 3 ? rdelta : (rdelta >> rateH) + (rdelta >> rateL);
      gdelta = abs(gdelta) < 3 ? gdelta : (gdelta >> rateH) + (gdelta >> rateL);
      bdelta = abs(bdelta) < 3 ? bdelta : (bdelta >> rateH) + (bdelta >> rateL);

      WS2812FX_setPixelColor_rgbw(i, r1 + rdelta, g1 + gdelta, b1 + bdelta, w1 + wdelta);
    }
  }
}


/*
 * color blend function
 颜色混合功能，渐变过程
 */
uint32_t WS2812FX_color_blend(uint32_t color1, uint32_t color2, uint8_t blend) {
  if(blend == 0)   return color1;
  if(blend == 255) return color2;

  uint8_t w1 = (color1 >> 24) & 0xff;
  uint8_t r1 = (color1 >> 16) & 0xff;
  uint8_t g1 = (color1 >>  8) & 0xff;
  uint8_t b1 =  color1        & 0xff;

  uint8_t w2 = (color2 >> 24) & 0xff;
  uint8_t r2 = (color2 >> 16) & 0xff;
  uint8_t g2 = (color2 >>  8) & 0xff;
  uint8_t b2 =  color2        & 0xff;

  uint32_t w3 = ((w2 * blend) + (w1 * (255U - blend))) / 256U;
  uint32_t r3 = ((r2 * blend) + (r1 * (255U - blend))) / 256U;
  uint32_t g3 = ((g2 * blend) + (g1 * (255U - blend))) / 256U;
  uint32_t b3 = ((b2 * blend) + (b1 * (255U - blend))) / 256U;

  return ((w3 << 24) | (r3 << 16) | (g3 << 8) | (b3));
}



/*
 * twinkle_fade function
 */
uint16_t WS2812FX_twinkle_fade(uint32_t color) {
  WS2812FX_fade_out();
  /* 修改后更多闪点 */
  if(WS2812FX_random8_lim(2) == 0) 
  {
    uint8_t size = 1 << SIZE_OPTION; /* 像素大小 */
    uint16_t index = _seg->start + WS2812FX_random16_lim(_seg_len - size);
    Adafruit_NeoPixel_fill(color, index, size);
    SET_CYCLE;
  }
  return (_seg->speed / 8);
}




/*
 * Sparkle function
 * color1 = background color
 * color2 = sparkle color
 */
uint16_t WS2812FX_sparkle(uint32_t color1, uint32_t color2) {
  if(_seg_rt->counter_mode_step == 0) {
    Adafruit_NeoPixel_fill(color1, _seg->start, _seg_len);
  }

  uint8_t size = 1 << SIZE_OPTION;
  Adafruit_NeoPixel_fill(color1, _seg->start + _seg_rt->aux_param3, size);

  _seg_rt->aux_param3 = WS2812FX_random16_lim(_seg_len - size); // aux_param3 stores the random led index
  Adafruit_NeoPixel_fill(color2, _seg->start + _seg_rt->aux_param3, size);

  SET_CYCLE;
  return (_seg->speed / 32);
}




/*
 * color chase function.
 * color1 = background color
 * color2 and color3 = colors of two adjacent leds
 */
uint16_t WS2812FX_chase(uint32_t color1, uint32_t color2, uint32_t color3) {
  uint8_t size = 1 << SIZE_OPTION;
  for(uint8_t i=0; i<size; i++) {
    uint16_t a = (_seg_rt->counter_mode_step + i) % _seg_len;
    uint16_t b = (a + size) % _seg_len;
    uint16_t c = (b + size) % _seg_len;
    if(IS_REVERSE) {
      WS2812FX_setPixelColor(_seg->stop - a, color1);
      WS2812FX_setPixelColor(_seg->stop - b, color2);
      WS2812FX_setPixelColor(_seg->stop - c, color3);
    } else {
      WS2812FX_setPixelColor(_seg->start + a, color1);
      WS2812FX_setPixelColor(_seg->start + b, color2);
      WS2812FX_setPixelColor(_seg->start + c, color3);
    }
  }

  if(_seg_rt->counter_mode_step + (size * 3) == _seg_len) SET_CYCLE;

  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % _seg_len;
  return (_seg->speed / _seg_len);
}


/*
 * running white flashes function.
 * color1 = background color
 * color2 = flash color
 */
uint16_t WS2812FX_chase_flash(uint32_t color1, uint32_t color2) {
  const static uint8_t flash_count = 4;
  uint8_t flash_step = _seg_rt->counter_mode_call % ((flash_count * 2) + 1);

  if(flash_step < (flash_count * 2)) {
    uint32_t color = (flash_step % 2 == 0) ? color2 : color1;
    uint16_t n = _seg_rt->counter_mode_step;
    uint16_t m = (_seg_rt->counter_mode_step + 1) % _seg_len;
    if(IS_REVERSE) {
      WS2812FX_setPixelColor(_seg->stop - n, color);
      WS2812FX_setPixelColor(_seg->stop - m, color);
    } else {
      WS2812FX_setPixelColor(_seg->start + n, color);
      WS2812FX_setPixelColor(_seg->start + m, color);
    }
    return 30;
  } else {
    _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % _seg_len;
    if(_seg_rt->counter_mode_step == 0) {
      // update aux_param so mode_chase_flash_random() will select the next color
      _seg_rt->aux_param = WS2812FX_get_random_wheel_index(_seg_rt->aux_param);
      SET_CYCLE;
    }
  }
  return (_seg->speed / _seg_len);
}


/*
 * Alternating pixels running function.
 */
uint16_t WS2812FX_running(uint32_t color1, uint32_t color2) {
  uint8_t size = 2 << SIZE_OPTION;
  uint32_t color = (_seg_rt->counter_mode_step & size) ? color1 : color2;

  if(IS_REVERSE) {
    WS2812FX_copyPixels(_seg->start, _seg->start + 1, _seg_len - 1);
    WS2812FX_setPixelColor(_seg->stop, color);
  } else {
    WS2812FX_copyPixels(_seg->start + 1, _seg->start, _seg_len - 1);
    WS2812FX_setPixelColor(_seg->start, color);
  }

  _seg_rt->counter_mode_step = (_seg_rt->counter_mode_step + 1) % _seg_len;
  if(_seg_rt->counter_mode_step == 0) SET_CYCLE;
  return (_seg->speed / _seg_len);
}



/*
 * Fireworks function.
 */
uint16_t WS2812FX_fireworks(uint32_t color) {
  WS2812FX_fade_out();

// for better performance, manipulate the Adafruit_NeoPixels pixels[] array directly
  uint8_t *pixels = Adafruit_NeoPixel_getPixels();
  uint8_t bytesPerPixel = Adafruit_NeoPixel_getNumBytesPerPixel(); // 3=RGB, 4=RGBW
  uint16_t startPixel = _seg->start * bytesPerPixel + bytesPerPixel;
  uint16_t stopPixel = _seg->stop * bytesPerPixel ;
  for(uint16_t i=startPixel; i <stopPixel; i++) {
    uint16_t tmpPixel = (pixels[i - bytesPerPixel] >> 2) +
      pixels[i] +
      (pixels[i + bytesPerPixel] >> 2);
    pixels[i] =  tmpPixel > 255 ? 255 : tmpPixel;
  }

  uint8_t size = 2 << SIZE_OPTION;
  if(!_triggered) {
    for(uint16_t i=0; i<max(1, _seg_len/20); i++) {
      if(WS2812FX_random8_lim(10) == 0) {
        uint16_t index = _seg->start + WS2812FX_random16_lim(_seg_len - size);
        Adafruit_NeoPixel_fill(color, index, size);
        SET_CYCLE;
      }
    }
  } else {
    for(uint16_t i=0; i<max(1, _seg_len/10); i++) {
      uint16_t index = _seg->start + WS2812FX_random16_lim(_seg_len - size);
      Adafruit_NeoPixel_fill(color, index, size);
      SET_CYCLE;
    }
  }

  return (_seg->speed / _seg_len);
}




/*
 * Fire flicker function
 */
uint16_t WS2812FX_fire_flicker(int rev_intensity) {
  uint8_t w = (_seg->colors[0] >> 24) & 0xFF;
  uint8_t r = (_seg->colors[0] >> 16) & 0xFF;
  uint8_t g = (_seg->colors[0] >>  8) & 0xFF;
  uint8_t b = (_seg->colors[0]        & 0xFF);
  uint8_t lum = max(w, max(r, max(g, b))) / rev_intensity;

  
  for(uint16_t i=_seg->start; i <= _seg->stop; i++) {
    int flicker = WS2812FX_random8_lim(lum);

    WS2812FX_setPixelColor_rgbw(i, max(r - flicker, 0), max(g - flicker, 0), max(b - flicker, 0), max(w - flicker, 0));
  }

  SET_CYCLE;
  return (_seg->speed / _seg_len);
}

/*
 * Blink/strobe function
 * Alternate between color1 and color2
 * if(strobe == true) then create a strobe effect
 */
uint16_t WS2812FX_blink(uint32_t color1, uint32_t color2, uint8_t strobe) {
  if(_seg_rt->counter_mode_call & 1) {
    uint32_t color = (IS_REVERSE) ? color1 : color2; // off
    Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);
    SET_CYCLE;
    return strobe ? _seg->speed - 20 : (_seg->speed / 2);
  } else {
    uint32_t color = (IS_REVERSE) ? color2 : color1; // on
    Adafruit_NeoPixel_fill(color, _seg->start, _seg_len);
    return strobe ? 20 : (_seg->speed / 2);
  }
}

