/*
移植WS2812FX库

该库实现了比Adafruit_NeoPixel库更丰富的显示效果，且该库的实现是建立在Adafruit_NeoPixel库的基础上实现的。这里的移植是基于STM32 Keil MDK实现的（其他平台未测试）。

移植原则

移植需要用到的变量，这里全部声明为static，外部函数需要使用其变量时必须通过函数的方式访问。
为所有向外提供使用的函数统一添加Adafruit_NeoPixel_的前缀，既便于识别又不会和其他文件函数产生重名冲突。
当函数存在重载情形时将重载函数的不一致参数组合作为函数名的后缀。
最大限度的尊重源库的命名，方便后续升级
移植过程

WS2812FX库对应的WS2812FX.cpp文件中有1600多行代码。贸然直接复制过来修改的话会有太多的工作量。我们根据具体的内容将代码分为两部分移植—基础函数和功能函数。

keil MDK 编译器不支持二进制表示，因此首先要实现binary.h文件（直接从arduino库中复制出来）。
实现库中需要用到的宏函数和一些不存在类型的typedef
移植基础函数部分（暂时屏蔽掉一些不重要的辅助函数）
移植功能函数部分
未移植自定义显示效果部分的函数；屏蔽掉了显示效果名称部分的定义（节省部分flash空间）
————————————————
版权声明：本文为CSDN博主「顶点元」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/xiaoyuanwuhui/article/details/108708674

 */


/* 注意
移植该库需要建立在移植好Adafruit_NeoPixel库的基础上进行，该库会用到很多Adafruit_NeoPixel库中的接口。

移植该库需要提供获取系统运行时间的函数。此处利用的是STM32HAL库提供的HAL_GetTick()函数。

#define millis HAL_GetTick
为减小rom内存的占用，这里屏蔽掉了各个模式名称的字符串存储。

对于twinkle()函数需要用到的random(min,max)函数尚未实现，暂且使用WS2812_random16_lim()函数替代。
————————————————
版权声明：本文为CSDN博主「顶点元」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/xiaoyuanwuhui/article/details/108708674
*/

#include "WS2812FX.h"
// #include "bsp_delay.h"
#include "Adafruit_NeoPixel.h"
#include <stdlib.h>
#include <string.h>

#include "led_strip_sys.h"


static uint16_t _rand16seed;
static void (*customShow)(void) = NULL;
uint8_t _running, _triggered;
static Segment _segments[MAX_NUM_SEGMENTS];                 // array of segments (20 bytes per element)
static Segment_runtime  _segment_runtimes[MAX_NUM_ACTIVE_SEGMENTS]; // array of segment runtimes (16 bytes per element)
static uint8_t _active_segments[MAX_NUM_ACTIVE_SEGMENTS];          // array of active segments (1 bytes per element)
static uint8_t _segments_len = 0;          // size of _segments array
static uint8_t _active_segments_len = 0;   // size of _segments_runtime and _active_segments arrays
static uint8_t _num_segments = 0;          // number of configured segments in the _segments array
 Segment* _seg;                      // currently active segment (20 bytes)
 Segment_runtime* _seg_rt;           // currently active segment runtime (16 bytes)
 uint16_t _seg_len;                  // num LEDs in the currently active segment




/**
 * @brief ws2812的初始化
 *
 * @param num_leds 灯珠的数量，一条灯带有10个灯珠，参数就为10
 * @param type 颜色顺序，一般是RGB
 */
void WS2812FX_init(uint16_t num_leds, uint8_t type) {
  #ifdef MY_DEBUG
  printf("WS2812FX_init\n");
  #endif
  Adafruit_NeoPixel_init(num_leds, type); //该函数要放在起始位置，放在后面可能会导致内存申请失败
  _running = false;
  _segments_len = MAX_NUM_SEGMENTS;
  _active_segments_len = MAX_NUM_ACTIVE_SEGMENTS;

  // create all the segment arrays and init to zeros
  /* _segments = (Segment *)malloc(_segments_len * sizeof(Segment)); */
 /*  _active_segments = (uint8_t *)malloc(_active_segments_len * sizeof(uint8_t)); */
/*   _segment_runtimes = (Segment_runtime *)malloc(_active_segments_len * sizeof(Segment_runtime)); */

  WS2812FX_resetSegments();
  // WS2812FX_setSegment_colorOptions(0, 0, num_leds - 1, DEFAULT_MODE, DEFAULT_COLOR, DEFAULT_SPEED, NO_OPTIONS);

  WS2812FX_resetSegmentRuntimes();

  Adafruit_NeoPixel_setBrightness(DEFAULT_BRIGHTNESS + 1);
}

/*
ws218运行
配合定时
*/
extern u8 ws2811fx_set_cycle;
extern unsigned long get_syn_time(void);
void WS2812FX_service()
{
  if(_running || _triggered) {

    unsigned long now = 0;

    now = millis(); // Be aware, millis() rolls over every 49 days

    uint8_t doShow = false;
    for(uint8_t i=0; i < _active_segments_len; i++) {
      if(_active_segments[i] != INACTIVE_SEGMENT) {
        _seg     = &_segments[_active_segments[i]];
        _seg_len = (uint16_t)(_seg->stop - _seg->start + 1);
        _seg_rt  = &_segment_runtimes[i];
        CLR_FRAME_CYCLE;

        if(now >= _seg_rt->next_time || _triggered) {
          SET_FRAME;
          doShow = true;

          uint16_t delay = _seg->mode();
          _seg_rt->next_time = now + max(delay, SPEED_MIN);
          _seg_rt->counter_mode_call++;
        }

      }
    }
    if(doShow) {
    //  delay(1); // for ESP32 (see https://forums.adafruit.com/viewtopic.php?f=47&t=117327)

      WS2812FX_show();
    }
    _triggered = false;
  }
}

// 设置颜色数量
void WS2812FX_set_coloQty(uint8_t seg, uint8_t n)
{
  if(n > MAX_NUM_COLORS) n = MAX_NUM_COLORS;
  _segments[seg].c_n = n;
}

// overload setPixelColor() functions so we can use gamma correction
// (see https://learn.adafruit.com/led-tricks-gamma-correction/the-issue)
void WS2812FX_setPixelColor(uint16_t n, uint32_t c) {
  uint8_t w = (c >> 24) & 0xFF;
  uint8_t r = (c >> 16) & 0xFF;
  uint8_t g = (c >>  8) & 0xFF;
  uint8_t b =  c        & 0xFF;
  WS2812FX_setPixelColor_rgbw(n, r, g, b, w);
}

void WS2812FX_setPixelColor_rgb(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  WS2812FX_setPixelColor_rgbw(n, r, g, b, 0);
}

void WS2812FX_setPixelColor_rgbw(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  if(IS_GAMMA)
  {
    Adafruit_NeoPixel_setPixelColor_rgbw(n, Adafruit_NeoPixel_gamma8(r), Adafruit_NeoPixel_gamma8(g), Adafruit_NeoPixel_gamma8(b), Adafruit_NeoPixel_gamma8(w));
  }
  else
  {
    Adafruit_NeoPixel_setPixelColor_rgbw(n, r, g, b, w);
  }
}

void WS2812FX_copyPixels(uint16_t dest, uint16_t src, uint16_t count) {
  uint8_t *pixels = Adafruit_NeoPixel_getPixels();
  uint8_t bytesPerPixel = WS2812FX_getNumBytesPerPixel(); // 3=RGB, 4=RGBW

  memmove(pixels + (dest * bytesPerPixel), pixels + (src * bytesPerPixel), count * bytesPerPixel);
}

// change the underlying Adafruit_NeoPixel pixels pointer (use with care)
//void WS2812FX_setPixels(uint16_t num_leds, uint8_t* ptr) {
//  free(Adafruit_NeoPixel::pixels); // free existing data (if any)
//  Adafruit_NeoPixel::pixels = ptr;
//  Adafruit_NeoPixel::numLEDs = num_leds;
//  Adafruit_NeoPixel::numBytes = num_leds * ((wOffset == rOffset) ? 3 : 4);
//}

// overload show() functions so we can use custom show()
void WS2812FX_show(void) {
  customShow == NULL ? Adafruit_NeoPixel_show() : customShow();
}

void WS2812FX_start() {
  WS2812FX_resetSegmentRuntimes();
  _running = true;
}

// 给内部的 _running 设置为 true
void WS2812FX_set_running(void)
{
    _running = true;
}

void WS2812FX_stop() {
  _running = false;
  // WS2812FX_strip_off();

}

void WS2812FX_play(void)
{
  _running = true;
}

void WS2812FX_pause() {
  _running = false;
}

void WS2812FX_resume() {
  WS2812FX_resetSegments();
  _running = true;
}

void WS2812FX_trigger() {
  _triggered = true;
}

void WS2812FX_setMode(mode_ptr m) {
  WS2812FX_setMode_seg(0, m);
}

void WS2812FX_setMode_seg(uint8_t seg, mode_ptr m) {
  WS2812FX_resetSegmentRuntime(seg);
  _segments[seg].mode = m;
}

void WS2812FX_setOptions(uint8_t seg, uint8_t o) {
  _segments[seg].options = o;
}

void WS2812FX_setSpeed(uint16_t s) {
  WS2812FX_setSpeed_seg(0, s);
}

void WS2812FX_setSpeed_seg(uint8_t seg, uint16_t s) {
  _segments[seg].speed = constrain(s, SPEED_MIN, SPEED_MAX);
}

void WS2812FX_increaseSpeed(uint8_t s) {
  uint16_t newSpeed = constrain(_seg->speed + s, SPEED_MIN, SPEED_MAX);
  WS2812FX_setSpeed(newSpeed);
}

void WS2812FX_decreaseSpeed(uint8_t s) {
  uint16_t newSpeed = constrain(_seg->speed - s, SPEED_MIN, SPEED_MAX);
  WS2812FX_setSpeed(newSpeed);
}

void WS2812FX_setColor_rgb(uint8_t r, uint8_t g, uint8_t b) {
  WS2812FX_setColor(((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
}

void WS2812FX_setColor_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  WS2812FX_setColor((((uint32_t)w << 24)| ((uint32_t)r << 16) | ((uint32_t)g << 8)| ((uint32_t)b)));
}

void WS2812FX_setColor(uint32_t c) {
  WS2812FX_setColor_seg(0, c);
}

void WS2812FX_setColor_seg(uint8_t seg, uint32_t c) {
  _segments[seg].colors[0] = c;
}

void WS2812FX_setColors(uint8_t seg, uint32_t* c) {
  for(uint8_t i=0; i<MAX_NUM_COLORS; i++) {
    _segments[seg].colors[i] = c[i];
  }
}

void WS2812FX_setBrightness(uint8_t b) {
  b = constrain(b, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  Adafruit_NeoPixel_setBrightness(b);
  WS2812FX_show();
}

void WS2812FX_increaseBrightness(uint8_t s) {
  s = constrain(Adafruit_NeoPixel_getBrightness() + s, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  WS2812FX_setBrightness(s);
}

void WS2812FX_decreaseBrightness(uint8_t s) {
  s = constrain(Adafruit_NeoPixel_getBrightness() - s, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
  WS2812FX_setBrightness(s);
}

//void WS2812FX::setLength(uint16_t b) {
//  resetSegmentRuntimes();
//  if (b < 1) b = 1;

//  // Decrease numLEDs to maximum available memory
//  do {
//      Adafruit_NeoPixel::updateLength(b);
//      b--;
//  } while(!Adafruit_NeoPixel::numLEDs && b > 1);

//  _segments[0].start = 0;
//  _segments[0].stop = Adafruit_NeoPixel::numLEDs - 1;
//}

//void WS2812FX_increaseLength(uint16_t s) {
//  uint16_t seglen = _segments[0].stop - _segments[0].start + 1;
//  setLength(seglen + s);
//}

//void WS2812FX_decreaseLength(uint16_t s) {
//  uint16_t seglen = _segments[0].stop - _segments[0].start + 1;
//  fill(BLACK, _segments[0].start, seglen);
//  show();

//  if (s < seglen) setLength(seglen - s);
//}

uint8_t WS2812FX_isRunning() {
  return _running;
}

uint8_t WS2812FX_isTriggered() {
  return _triggered;
}

uint8_t WS2812FX_isFrame() {
  return WS2812FX_isFrame_seg(0);
}

uint8_t WS2812FX_isFrame_seg(uint8_t seg) {
  uint8_t* ptr = (uint8_t*)memchr(_active_segments, seg, _active_segments_len);
  if(ptr == NULL) return false; // segment not active
  return (_segment_runtimes[ptr - _active_segments].aux_param2 & FRAME);
}

uint8_t WS2812FX_isCycle() {
  return WS2812FX_isCycle_seg(0);
}

uint8_t WS2812FX_isCycle_seg(uint8_t seg) {
  uint8_t* ptr = (uint8_t*)memchr(_active_segments, seg, _active_segments_len);
  if(ptr == NULL) return false; // segment not active
  return (_segment_runtimes[ptr - _active_segments].aux_param2 & CYCLE);
}

void WS2812FX_setCycle() {
  SET_CYCLE;
}




uint16_t WS2812FX_getSpeed(void) {
  return WS2812FX_getSpeed_seg(0);
}

uint16_t WS2812FX_getSpeed_seg(uint8_t seg) {
  return _segments[seg].speed;
}

uint8_t WS2812FX_getOptions(uint8_t seg) {
  return _segments[seg].options;
}

uint16_t WS2812FX_getLength(void) {
  return Adafruit_NeoPixel_numPixels();
}

uint16_t WS2812FX_getNumBytes(void) {
  return Adafruit_NeoPixel_getNumBytes();
}

uint8_t WS2812FX_getNumBytesPerPixel(void) {
  return Adafruit_NeoPixel_getNumBytesPerPixel();
}

uint8_t WS2812FX_getModeCount(void) {
  return MODE_COUNT;
}

uint8_t WS2812FX_getNumSegments(void) {
  return _num_segments;
}

void WS2812FX_setNumSegments(uint8_t n) {
  _num_segments = n;
}

uint32_t WS2812FX_getColor(void) {
  return WS2812FX_getColor_seg(0);
}

uint32_t WS2812FX_getColor_seg(uint8_t seg) {
  return _segments[seg].colors[0];
}

uint32_t* WS2812FX_getColors(uint8_t seg) {
  return _segments[seg].colors;
}

Segment* WS2812FX_getSegment(void) {
  return _seg;
}

Segment* WS2812FX_getSegment_seg(uint8_t seg) {
  return &_segments[seg];
}

Segment* WS2812FX_getSegments(void) {
  return _segments;
}

Segment_runtime* WS2812FX_getSegmentRuntime(void) {
  return _seg_rt;
}

Segment_runtime* WS2812FX_getSegmentRuntime_seg(uint8_t seg) {
  uint8_t* ptr = (uint8_t*)memchr(_active_segments, seg, _active_segments_len);
  if(ptr == NULL) return NULL; // segment not active
  return &_segment_runtimes[ptr - _active_segments];
}

Segment_runtime* WS2812FX_getSegmentRuntimes(void) {
  return _segment_runtimes;
}

uint8_t* WS2812FX_getActiveSegments(void) {
  return _active_segments;
}

//const __FlashStringHelper* WS2812FX_getModeName(uint8_t m) {
//  if(m < MODE_COUNT) {
//    return _names[m];
//  } else {
//    return "";//F("");
//  }
//}

void WS2812FX_setIdleSegment(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t color, uint16_t speed, uint8_t options) {
  uint32_t colors[] = {color, 0, 0};
  WS2812FX_setIdleSegment_colors(n, start, stop, mode, colors, speed, options);
}

void WS2812FX_setIdleSegment_colors(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t colors[], uint16_t speed, uint8_t options) {
  WS2812FX_setSegment_colorsOptions(n, start, stop, mode, colors, speed, options);
  if(n < _active_segments_len) WS2812FX_removeActiveSegment(n);;
}

void WS2812FX_setSegment_colorReverse(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t color, uint16_t speed, uint8_t reverse) {
  uint32_t colors[] = {color, 0, 0};
  WS2812FX_setSegment_colorsOptions(n, start, stop, mode, colors, speed, (uint8_t)(reverse ? REVERSE : NO_OPTIONS));
}

void WS2812FX_setSegment_colorsReverse(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t colors[], uint16_t speed, uint8_t reverse) {
  WS2812FX_setSegment_colorsOptions(n, start, stop, mode, colors, speed, (uint8_t)(reverse ? REVERSE : NO_OPTIONS));
}

void WS2812FX_setSegment_colorOptions(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t color, uint16_t speed, uint8_t options) {
  uint32_t colors[] = {color, 0, 0};
  WS2812FX_setSegment_colorsOptions(n, start, stop, mode, colors, speed, options);
}

void WS2812FX_setSegment_colorsOptions(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t colors[], uint16_t speed, uint8_t options) {
  if(n < _segments_len) {
    if(n + 1 > _num_segments) _num_segments = n + 1;
    _segments[n].start = start;
    _segments[n].stop = stop;
    _segments[n].mode = mode;
    _segments[n].speed = speed;
    _segments[n].options = options;

    WS2812FX_setColors(n, (uint32_t*)colors);

    if(n < _active_segments_len) WS2812FX_addActiveSegment(n);
  }
}


void WS2812FX_addActiveSegment(uint8_t seg) {
  uint8_t* ptr = (uint8_t*)memchr(_active_segments, seg, _active_segments_len);
  if(ptr != NULL) return; // segment already active
  for(uint8_t i=0; i<_active_segments_len; i++) {
    if(_active_segments[i] == INACTIVE_SEGMENT) {
      _active_segments[i] = seg;
      WS2812FX_resetSegmentRuntime(seg);
      break;
    }
  }
}

void WS2812FX_removeActiveSegment(uint8_t seg) {
  for(uint8_t i=0; i<_active_segments_len; i++) {
    if(_active_segments[i] == seg) {
      _active_segments[i] = INACTIVE_SEGMENT;
    }
  }
}

void WS2812FX_swapActiveSegment(uint8_t oldSeg, uint8_t newSeg) {
  uint8_t* ptr = (uint8_t*)memchr(_active_segments, newSeg, _active_segments_len);
  if(ptr != NULL) return; // if newSeg is already active, don't swap
  for(uint8_t i=0; i<_active_segments_len; i++) {
    if(_active_segments[i] == oldSeg) {
      _active_segments[i] = newSeg;

      // reset all runtime parameters EXCEPT next_time,
      // allowing the current animation frame to complete
      Segment_runtime seg_rt = _segment_runtimes[i];
      seg_rt.counter_mode_step = 0;
      seg_rt.counter_mode_call = 0;
      seg_rt.aux_param = 0;
      seg_rt.aux_param2 = 0;
      seg_rt.aux_param3 = 0;
      break;
    }
  }
}

uint8_t WS2812FX_isActiveSegment(uint8_t seg) {
  uint8_t* ptr = (uint8_t*)memchr(_active_segments, seg, _active_segments_len);
  if(ptr != NULL) return true;
  return false;
}

/* 调用改函数要重新初始化一次WS2812 */
void WS2812FX_resetSegments() {
  WS2812FX_resetSegmentRuntimes();
  memset(_segments, 0, _segments_len * sizeof(Segment));
  memset(_active_segments, INACTIVE_SEGMENT, _active_segments_len);
  _num_segments = 0;
}

/* 设置所以段无效 */
void WS2812FX_setSegmentsInactive(void)
{
  memset(_active_segments, INACTIVE_SEGMENT, _active_segments_len);
}

void WS2812FX_resetSegmentRuntimes() {
  memset(_segment_runtimes, 0, _active_segments_len * sizeof(Segment_runtime));
}

void WS2812FX_resetSegmentRuntime(uint8_t seg) {
  uint8_t* ptr = (uint8_t*)memchr(_active_segments, seg, _active_segments_len);
  if(ptr == NULL) return; // segment not active
  memset(&_segment_runtimes[ptr - _active_segments], 0, sizeof(Segment_runtime));
}

/*
 * Turns everything off. Doh.
 */
void WS2812FX_strip_off() {
  Adafruit_NeoPixel_clear();
  WS2812FX_show();
}

/*
 * Put a value 0 to 255 in to get a color value.
 * The colours are a transition r -> g -> b -> back to r
 * Inspired by the Adafruit examples.
 * 输入数值0-255，输出颜色从红-》黄-》绿-》青-》蓝-》紫-》回到红
 */
uint32_t WS2812FX_color_wheel(uint8_t pos) {
  pos = 255 - pos;
  if(pos < 85) {
    return ((uint32_t)(255 - pos * 3) << 16) | ((uint32_t)(0) << 8) | (pos * 3);
  } else if(pos < 170) {
    pos -= 85;
    return ((uint32_t)(0) << 16) | ((uint32_t)(pos * 3) << 8) | (255 - pos * 3);
  } else {
    pos -= 170;
    return ((uint32_t)(pos * 3) << 16) | ((uint32_t)(255 - pos * 3) << 8) | (0);
  }
}

/*
 * Returns a new, random wheel index with a minimum distance of 42 from pos.
 输入pos，返回的数据距离pos最小42
 */
uint8_t WS2812FX_get_random_wheel_index(uint8_t pos) {
  uint8_t r = 0;
  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t d = 0;

  while(d < 42) {
    r = WS2812FX_random8();
    x = abs(pos - r);
    y = 255 - x;
    d = min(x, y);
  }

  return r;
}

// fast 8-bit random number generator shamelessly borrowed from FastLED
uint8_t WS2812FX_random8() {
    _rand16seed = (_rand16seed * 2053) + 13849;
    return (uint8_t)((_rand16seed + (_rand16seed >> 8)) & 0xFF);
}

// note random8(lim) generates numbers in the range 0 to (lim -1)
uint8_t WS2812FX_random8_lim(uint8_t lim) {
    uint8_t r = WS2812FX_random8();
    r = ((uint16_t)r * lim) >> 8;
    return r;
}

uint16_t WS2812FX_random16() {
    return (uint16_t)WS2812FX_random8() * 256 + WS2812FX_random8();
}

// note random16(lim) generates numbers in the range 0 to (lim - 1)
uint16_t WS2812FX_random16_lim(uint16_t lim) {
    uint16_t r = WS2812FX_random16();
    r = ((uint32_t)r * lim) >> 16;
    return r;
}

































