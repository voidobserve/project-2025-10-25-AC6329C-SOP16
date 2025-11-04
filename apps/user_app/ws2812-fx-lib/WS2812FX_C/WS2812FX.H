#ifndef __WS2812FX_H
#define __WS2812FX_H
#define WS2812FX_VERSION 0x00000001
#include "adafruit_typedef.h"

/*
  移植该库需要建立在移植好Adafruit_NeoPixel库的基础上来实现
  1.keil MDK编译器不支持二进制表示，而该库用到了二进制表示，需要用到binary.h文件
  2.为了减少库文件本身内容的修改，定义了一些方便移植的宏。
  3.当前版本移植匆忙未完全测试
  4.该库需要实现获取系统运行时间的函数，这里直接利用了HAL库提供的HAL_GetTick函数。
 */

#include <stdint.h>
#include "ws2812_bsp.h"
#include "binary.h"


/* 为方便移植定义的宏 */
typedef char __FlashStringHelper;
#define millis HAL_GetTick
#define PROGMEM
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define constrain(amt,low,high) ((amt)<=(low)?(low):((amt)>=(high)?(high):(amt)))

#define FSH(x) (__FlashStringHelper*)(x)

//#include <Adafruit_NeoPixel.h>

#define DEFAULT_BRIGHTNESS (uint8_t)50
#define DEFAULT_MODE       (uint8_t)0
#define DEFAULT_SPEED      (uint16_t)1000
#define DEFAULT_COLOR      (uint32_t)0xFF0000
#define DEFAULT_COLORS     { RED, GREEN, BLUE }
#define COLORS(...)        (const uint32_t[]){__VA_ARGS__}

#if defined(ESP8266) || defined(ESP32)
  //#pragma message("Compiling for ESP")
  #define SPEED_MIN (uint16_t)2
#else
  //#pragma message("Compiling for Arduino")
  #define SPEED_MIN (uint16_t)10
#endif
#define SPEED_MAX (uint16_t)65535

#define BRIGHTNESS_MIN (uint8_t)0
#define BRIGHTNESS_MAX (uint8_t)255

/* each segment uses 36 bytes of SRAM memory, so if you're compile fails
  because of insufficient flash memory, decreasing MAX_NUM_SEGMENTS may help */
#define MAX_NUM_SEGMENTS         10
#define MAX_NUM_ACTIVE_SEGMENTS  10
#define INACTIVE_SEGMENT        255 /* max uint_8 */
#define MAX_NUM_COLORS            8 /* number of colors per segment */
#define MAX_CUSTOM_MODES          8

// some common colors
#define RED        (uint32_t)0x00FF0000
#define GREEN      (uint32_t)0x0000FF00
#define BLUE       (uint32_t)0x000000FF
#define WHITE      (uint32_t)0xFFFFFFFF // （流星灯需要使用到这份数据，如果修改了成0xFF000000，会导致流星灯不亮）
#define PURE_WHITE (uint32_t)0xFF000000 // 七彩灯使用到的纯白色
// #define WHITE      (uint32_t)0xFF000000
#define BLACK      (uint32_t)0x00000000
#define YELLOW     (uint32_t)0x00FFFF00
#define CYAN       (uint32_t)0x0000FFFF
#define MAGENTA    (uint32_t)0x00FF00FF
#define PURPLE     (uint32_t)0x00400080
#define ORANGE     (uint32_t)0x00FF3000
#define PINK       (uint32_t)0x00FF1493
#define GRAY       (uint32_t)0x00101010
#define ULTRAWHITE (uint32_t)0xFFFFFFFF
#define DIM(c)     (uint32_t)((c >> 2) & 0x3f3f3f3f) // color at 25% intensity
#define DARK(c)    (uint32_t)((c >> 4) & 0x0f0f0f0f) // color at  6% intensity


// segment options
// bit    7: reverse animation
// bits 4-6: fade rate (0-7)
// bit    3: gamma correction
// bits 1-2: size
// bits   0: TBD
#define NO_OPTIONS   (uint8_t)B00000000
#define REVERSE      (uint8_t)B10000000                         //控制正反方向
#define IS_REVERSE   ((_seg->options & REVERSE) == REVERSE)     //判断是反方向
#define FADE_XFAST   (uint8_t)B00010000                         //FADE系列，拖尾（长度）的数量，搭配 WS2812FX_fade_out(); 使用
#define FADE_FAST    (uint8_t)B00100000
#define FADE_MEDIUM  (uint8_t)B00110000
#define FADE_SLOW    (uint8_t)B01000000
#define FADE_XSLOW   (uint8_t)B01010000
#define FADE_XXSLOW  (uint8_t)B01100000
#define FADE_GLACIAL (uint8_t)B01110000
#define FADE_RATE    ((_seg->options >> 4) & 7)
#define GAMMA        (uint8_t)B00001000
#define IS_GAMMA     ((_seg->options & GAMMA) == GAMMA)
#define SIZE_SMALL   (uint8_t)B00000000 //1                     // SIZE 系列  特别的效果才用上    
#define SIZE_MEDIUM  (uint8_t)B00000010 //3                      //3个灯为一组
#define SIZE_LARGE   (uint8_t)B00000100 //5
#define SIZE_XLARGE  (uint8_t)B00000110 //7
#define SIZE_OPTION  ((_seg->options >> 1) & 3)

// segment runtime options (aux_param2)
#define FRAME           ((uint8_t)B10000000)
#define SET_FRAME       (_seg_rt->aux_param2 |=  FRAME)
#define CLR_FRAME       (_seg_rt->aux_param2 &= ~FRAME)
#define CYCLE           ((uint8_t)B01000000)
#define SET_CYCLE       (_seg_rt->aux_param2 |=  CYCLE)
#define CLR_CYCLE       (_seg_rt->aux_param2 &= ~CYCLE)
#define CLR_FRAME_CYCLE (_seg_rt->aux_param2 &= ~(FRAME | CYCLE))

#define MODE_COUNT 63 //(sizeof(_names)/sizeof(_names[0]))





typedef uint16_t (*mode_ptr)(void);

// segment parameters
typedef struct Segment { // 20 bytes
  uint16_t start;
  uint16_t stop;
  uint16_t speed;
  mode_ptr  mode;
  uint8_t  options;
  uint8_t  c_n;     //有效颜色数量
  uint32_t colors[MAX_NUM_COLORS];
} Segment;

// segment runtime parameters
typedef struct Segment_runtime { // 16 bytes
  unsigned long next_time;
  uint32_t counter_mode_step;
  uint32_t counter_mode_call;
  uint8_t aux_param;   // auxilary param (usually stores a color_wheel index)
  uint8_t aux_param2;  // auxilary param (usually stores bitwise options)
  uint16_t aux_param3; // auxilary param (usually stores a segment index)
} Segment_runtime;



//    timer(void),
void  WS2812FX_init(uint16_t num_leds, uint8_t type);
void
  WS2812FX_service(void),
  WS2812FX_start(void),
  WS2812FX_stop(void),
  WS2812FX_pause(void),
  WS2812FX_resume(void),
  WS2812FX_strip_off(void),
  WS2812FX_fade_out(void),
  WS2812FX_fade_out_targetColor(uint32_t),
  WS2812FX_setMode(mode_ptr m),
  WS2812FX_setMode_seg(uint8_t seg, mode_ptr m),
  WS2812FX_setOptions(uint8_t seg, uint8_t o),
  WS2812FX_setCustomMode(uint16_t (*p)()),
  WS2812FX_setCustomShow(void (*p)()),
  WS2812FX_setSpeed(uint16_t s),
  WS2812FX_setSpeed_seg(uint8_t seg, uint16_t s),
  WS2812FX_increaseSpeed(uint8_t s),
  WS2812FX_decreaseSpeed(uint8_t s),
  WS2812FX_setColor_rgb(uint8_t r, uint8_t g, uint8_t b),
  WS2812FX_setColor_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w),
  WS2812FX_setColor(uint32_t c),
  WS2812FX_set_coloQty(uint8_t seg, uint8_t n),
  WS2812FX_setColor_seg(uint8_t seg, uint32_t c),
  WS2812FX_setColors(uint8_t seg, uint32_t* c),
  WS2812FX_setBrightness(uint8_t b),
  WS2812FX_increaseBrightness(uint8_t s),
  WS2812FX_decreaseBrightness(uint8_t s),
  WS2812FX_setLength(uint16_t b),
  WS2812FX_increaseLength(uint16_t s),
  WS2812FX_decreaseLength(uint16_t s),
  WS2812FX_trigger(void),
  WS2812FX_setCycle(void),
  WS2812FX_setNumSegments(uint8_t n),
  WS2812FX_setSegment_colorReverse(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t color,          uint16_t speed, uint8_t reverse),
  WS2812FX_setSegment_colorOptions(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t color,          uint16_t speed, uint8_t options),
  WS2812FX_setSegment_colorsReverse(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t colors[], uint16_t speed, uint8_t reverse),
  WS2812FX_setSegment_colorsOptions(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t colors[], uint16_t speed, uint8_t options),
  WS2812FX_setIdleSegment(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t color,          uint16_t speed, uint8_t options),
  WS2812FX_setIdleSegment_colors(uint8_t n, uint16_t start, uint16_t stop, mode_ptr mode, uint32_t colors[], uint16_t speed, uint8_t options),
  WS2812FX_addActiveSegment(uint8_t seg),
  WS2812FX_removeActiveSegment(uint8_t seg),
  WS2812FX_swapActiveSegment(uint8_t oldSeg, uint8_t newSeg),
  WS2812FX_resetSegments(void),
  WS2812FX_resetSegmentRuntimes(void),
  WS2812FX_resetSegmentRuntime(uint8_t),
  WS2812FX_setPixelColor(uint16_t n, uint32_t c),
  WS2812FX_setPixelColor_rgb(uint16_t n, uint8_t r, uint8_t g, uint8_t b),
  WS2812FX_setPixelColor_rgbw(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w),
  WS2812FX_copyPixels(uint16_t d, uint16_t s, uint16_t c),
  WS2812FX_setPixels(uint16_t, uint8_t*),
  WS2812FX_show(void);

uint8_t
  WS2812FX_isRunning(void),
  WS2812FX_isTriggered(void),
  WS2812FX_isFrame(void),
  WS2812FX_isFrame_seg(uint8_t),
  WS2812FX_isCycle(void),
  WS2812FX_isCycle_seg(uint8_t),
  WS2812FX_isActiveSegment(uint8_t seg);

uint8_t
  WS2812FX_random8(void),
  WS2812FX_random8_lim(uint8_t),


  WS2812FX_getModeCount(void),
//      WS2812FX_setCustomMode(const __FlashStringHelper* name, uint16_t (*p)()),
//      WS2812FX_setCustomMode(uint8_t i, const __FlashStringHelper* name, uint16_t (*p)()),
  WS2812FX_getNumSegments(void),
  WS2812FX_get_random_wheel_index(uint8_t),
  WS2812FX_getOptions(uint8_t),
  WS2812FX_getNumBytesPerPixel(void);

uint16_t
  WS2812FX_random16(void),
  WS2812FX_random16_lim(uint16_t),
  WS2812FX_getSpeed(void),
  WS2812FX_getSpeed_seg(uint8_t),
  WS2812FX_getLength(void),
  WS2812FX_getNumBytes(void);

uint32_t
  WS2812FX_color_wheel(uint8_t),
  WS2812FX_getColor(void),
  WS2812FX_getColor_seg(uint8_t),
  WS2812FX_intensitySum(void);

uint32_t* getColors(uint8_t);
uint32_t* intensitySums(void);
uint8_t*  getActiveSegments(void);

const __FlashStringHelper* getModeName(uint8_t m);

Segment* getSegment(void);

Segment* getSegment_seg(uint8_t);

Segment* getSegments(void);

Segment_runtime* getSegmentRuntime(void);

Segment_runtime* getSegmentRuntime_seg(uint8_t);

Segment_runtime* getSegmentRuntimes(void);


void WS2812FX_set_running(void);

// 以最大亮度值给指定位置填充颜色
void WS2812FX_setPixelColor_with_max_brightness(uint16_t n, uint32_t c);
void WS2812FX_setPixelColor_rgbw_with_max_brightness(uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w);


void  WS2812FX_trigger_by_colorful_lights(void);
void WS2812FX_trigger_by_meteor_lights(void);

#endif /* __BSP_WS2812FX_H */

