#include "Adafruit_NeoPixel.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "includes.h"
#include "adafruit_typedef.h"
#include "../../../../apps/user_app/led_strip/led_strip_sys.h"
#include "../../../../apps/user_app/led_strip/led_strand_effect.h"

/*
Adafruit_NeoPixel库为实现WS2812类似系列的灯珠实现非常酷炫的效果提供了各种接口函数，
应用层可以很方便的利用这些接口函数实现各种丰富的显示效果。Adafruit_NeoPixel库提供的example中包含了很多炫酷效果的例程，
实现更加丰富效果的WS2812FX库便是利用Adafruit_NeoPixel库的基础上实现的。这里将其移植为C语言版本可以更广泛的应用到嵌入式平台上，
也为后续移植WS2812FX库打下铺垫。

————————————————
版权声明：本文为CSDN博主「顶点元」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/xiaoyuanwuhui/article/details/108708071
 */

/*
最重要的部分是要实现Adafruit_NeoPixel_init()函数和修改Adafruit_NeoPixel_show()函数。
其次还包含PWM+DMA底层驱动接口的实现和函数改名和添加库中没有的变量访问接口。

 */

/*
Adafruit_NeoPixel_updateLength()和Adafruit_NeoPixel_updateType()函数要谨慎使用，不推荐在初始化完成后使用，可能会造成内存泄漏。
这里面不包含丰富的显示效果，具体的显示效果例程还需要从example的例子中移植。

 */

#define SYS_MAX_LED_NUMBER 300

/*-------------------------------variable-------------------------------*/
static uint16_t numLEDs;  ///< Number of RGB LEDs in strip
static uint16_t numBytes; ///< Size of 'pixels' buffer below
// static int16_t           pin;        ///< Output pin number (-1 if not yet set)
static uint8_t brightness;                 ///< Strip brightness 0-255 (stored as +1)
static uint8_t pixels[SYS_MAX_LED_NUMBER]; ///< Holds LED color values (3 or 4 bytes each)   配置RAM大小
static uint8_t rOffset;                    ///< Red index within each 3- or 4-byte pixel
static uint8_t gOffset;                    ///< Index of green byte
static uint8_t bOffset;                    ///< Index of blue byte
static uint8_t wOffset;                    ///< Index of white (==rOffset if no white)
// static uint32_t          endTime;    ///< Latch timing reference

// -------------------------   应用代码 -------------------------------------
// -------------------------   应用代码 -------------------------------------
// -------------------------   应用代码 -------------------------------------
static volatile unsigned long tick_ms;
void ws281x_init()
{
}

static volatile u8 buf[SYS_MAX_LED_NUMBER * 3] __attribute((aligned(4))); // 使用中断实现效果时，必须需要全局变量SYS_MAX_LED_NUMBER*3

const u8 meteor_lights_index[12] = {
    1, 3, 2, 4, 5, 7, 6, 8, 9, 11, 10, 12};
/**
 * @brief 灯具的驱动集成，包括七彩的驱动，幻彩的驱动
 *
 * @param pixels_pattern   颜色
 * @param pattern_size     灯具的长度    fc_effect.led_num * 3 或者 * 4
 */
void ws281x_show(unsigned char *pixels_pattern, unsigned short pattern_size)
{

#if (LED_STRIP_TYPE == TYPE_Fiber_optic_lights)

#if LED_STRIP_RGBW

    // printf_buf(pixels_pattern,pattern_size);

    // 七彩灯驱动：
    fc_rgbw_driver(*pixels_pattern,
                   *(pixels_pattern + 1),
                   *(pixels_pattern + 2),
                   *(pixels_pattern + 3));

    // 该数据处理是仅有白光的流星使用
    u16 i = 0;
    u16 j = 0;
    u16 k = 0;
    u8 r, g, b, w;
    // for( i = 0; i < pattern_size / 4; i++)
    // {
    //     // *(buf + i) = *(pixels_pattern + 4 + i * 4);
    //     buf[i] = *(pixels_pattern + 4 + (i * 4));
    // }

    /*
        如果按照原来的顺序存储数据，点亮流星灯的顺序会是：
        1 - 3 - 2 - 4 - 5 - 7 - 6 - 8 - 9 - 11 - 10 - 12
        要求的顺序是：
        1 - 2 - 3 - 4 - 5 - 6 - 7 - 8 - 9 - 10 - 11 - 12

        这里的修改只支持12个灯珠的流星灯
    */
    // u16 buf_index = 0;
    for (i = 0; i < pattern_size / 4; i++)
    {
        // if (i <= 12 - 1)
        // {
        //     buf_index = meteor_lights_index[i] - 1;
        // }
        // else
        // {
        //     buf_index = i;
        // }

        // buf[buf_index] = *(pixels_pattern + 4 + (i * 4));
        buf[i] = *(pixels_pattern + 4 + (i * 4));
    }

    // buf[pattern_size / 4 - 1] = 0xFF;
    // buf[pattern_size / 4 + 0] = 0xFF;

    // buf[pattern_size / 4 + 1] = 0xFF;
    // buf[pattern_size / 4 + 2] = 0xFF;

#if 0  // RGBW颜色的流星灯使用：
    // for(i=0; i< pattern_size/4; i++)
    // {
    //   if(j==0){

    //     r = *(buf+i);

    //   }else if(j==1){

    //     g = *(buf+i);

    //   }
    //   else if(j==2){

    //     b = *(buf+i);

    //   }else if(j==3){

    //     w = *(buf+i);

    //   }

    //   j++;

    //   if(j==4){

    //     j=0;

    //     *(buf + rOffset + (i-3)) = r;
    //     *(buf + gOffset + (i-3)) = g;
    //     *(buf + bOffset + (i-3)) = b;
    //     *(buf + wOffset + (i-3)) = w;

    //   }
    // }
#endif // RGBW颜色的流星灯使用：

    // 幻彩灯驱动函数
    //  流星灯驱动函数：
    // printf("pattern_size %u\n", (u16)pattern_size);
    extern void ledc_send_rgbbuf_isr(u8 index, u8 * rgbbuf, u32 buf_len, u16 again_cnt);
    // ledc_send_rgbbuf_isr(0, buf, pattern_size / 4 - 1, 0); //  有一个灯是七彩灯，不发送它的数据
    ledc_send_rgbbuf_isr(0, buf, fc_effect.led_num - 1, 0); //  有一个灯是七彩灯，不发送它的数据
    // ledc_send_rgbbuf_isr(0, buf, pattern_size / 4 + 2, 0);

#else

#endif

#else if (LED_STRIP_TYPE == TYPE_Fiber_optic_lights)

#endif
}

// 周期10ms
unsigned long HAL_GetTick(void)
{
    return tick_ms;
}

// 每10ms调用一次
void run_tick_per_10ms(void)
{
    tick_ms += 10;
}

//=============================== 以下是库代码 尽量不要改动  =======================
//=============================== 以下是库代码 尽量不要改动  =======================
//=============================== 以下是库代码 尽量不要改动  =======================

void Adafruit_NeoPixel_init(uint16_t pixel_num, neoPixelType type)
{

    /* WS2812系列灯珠，底层驱动初始化 */
    ws281x_init();

    /*   free(pixels); */
    brightness = 0;
    //  endTime = 0;
    wOffset = (type) >> 6 & 0x3;
    rOffset = (type) >> 4 & 0x3;
    gOffset = (type) >> 2 & 0x3;
    bOffset = (type) & 0x3;
    numBytes = pixel_num * ((wOffset == rOffset) ? 3 : 4);
    memset(pixels, 0, numBytes);
    numLEDs = pixel_num;
    /*   pixels = (uint8_t *)malloc(numBytes);
      if(NULL != pixels){ //内存申请成功
        memset(pixels, 0, numBytes);

      } else {
        numLEDs = numBytes = 0;
      } */
}

/*!
  @brief   Change the length of a previously-declared Adafruit_NeoPixel
           strip object. Old data is deallocated and new data is cleared.
           Pin number and pixel format are unchanged.
  @param   n  New length of strip, in pixels.
  @note    This function is deprecated, here only for old projects that
           may still be calling it. New projects should instead use the
           'new' keyword with the first constructor syntax (length, pin,
           type).
*/
void Adafruit_NeoPixel_updateLength(uint16_t n)
{
    /* free(pixels); */ // Free existing data (if any)

    // Allocate new data -- note: ALL PIXELS ARE CLEARED
    numBytes = n * ((wOffset == rOffset) ? 3 : 4);
    memset(pixels, 0, numBytes);
    /*   pixels = (uint8_t *)malloc(numBytes);
      if(pixels) {
        memset(pixels, 0, numBytes);
        numLEDs = n;
      } else {
        numLEDs = numBytes = 0;
      } */
}

/*!
  @brief   Change the pixel format of a previously-declared
           Adafruit_NeoPixel strip object. If format changes from one of
           the RGB variants to an RGBW variant (or RGBW to RGB), the old
           data will be deallocated and new data is cleared. Otherwise,
           the old data will remain in RAM and is not reordered to the
           new format, so it's advisable to follow up with clear().
  @param   t  Pixel type -- add together NEO_* constants defined in
              Adafruit_NeoPixel.h, for example NEO_GRB+NEO_KHZ800 for
              NeoPixels expecting an 800 KHz (vs 400 KHz) data stream
              with color bytes expressed in green, red, blue order per
              pixel.
  @note    This function is deprecated, here only for old projects that
           may still be calling it. New projects should instead use the
           'new' keyword with the first constructor syntax
           (length, pin, type).
*/
void Adafruit_NeoPixel_updateType(neoPixelType t)
{
    unsigned char oldThreeBytesPerPixel = (wOffset == rOffset); // false if RGBW

    wOffset = (t >> 6) & 0x03; // See notes in header file
    rOffset = (t >> 4) & 0x03; // regarding R/G/B/W offsets
    gOffset = (t >> 2) & 0x03;
    bOffset = t & 0x03;

    // If bytes-per-pixel has changed (and pixel data was previously
    // allocated), re-allocate to new size. Will clear any data.
    if (pixels)
    {
        unsigned char newThreeBytesPerPixel = (wOffset == rOffset);
        if (newThreeBytesPerPixel != oldThreeBytesPerPixel)
            Adafruit_NeoPixel_updateLength(numLEDs);
    }
}

/*!
  @brief   Check whether a call to show() will start sending data
            immediately or will 'block' for a required interval. NeoPixels
            require a short quiet time (about 300 microseconds) after the
            last bit is received before the data 'latches' and new data can
            start being received. Usually one's sketch is implicitly using
            this time to generate a new frame of animation...but if it
            finishes very quickly, this function could be used to see if
            there's some idle time available for some low-priority
            concurrent task.
  @return  1 or true if show() will start sending immediately, 0 or false
            if show() would block (meaning some idle time is available).
*/
// static bool canShow(void) {
//   if (endTime > micros()) {
//     endTime = micros();
//   }
//   return (micros() - endTime) >= 300L;
// }

void Adafruit_NeoPixel_show(void)
{

    if (!pixels)
        return;

    // Data latch = 300+ microsecond pause in the output stream. Rather than
    // put a delay at the end of the function, the ending time is noted and
    // the function will simply hold off (if needed) on issuing the
    // subsequent round of data until the latch time has elapsed. This
    // allows the mainline code to start generating the next frame of data
    // rather than stalling for the latch.
    // while(!canShow());
    // endTime is a private member (rather than global var) so that multiple
    // instances on different pins can be quickly issued in succession (each
    // instance doesn't delay the next).

    // In order to make this code runtime-configurable to work with any pin,
    // SBI/CBI instructions are eschewed in favor of full PORT writes via the
    // OUT or ST instructions. It relies on two facts: that peripheral
    // functions (such as PWM) take precedence on output pins, so our PORT-
    // wide writes won't interfere, and that interrupts are globally disabled
    // while data is being issued to the LEDs, so no other code will be
    // accessing the PORT. The code takes an initial 'snapshot' of the PORT
    // state, computes 'pin high' and 'pin low' values, and writes these back
    // to the PORT register as needed.

    // To support both the SoftDevice + Neopixels we use the EasyDMA
    // feature from the NRF25. However this technique implies to
    // generate a pattern and store it on the memory. The actual
    // memory used in bytes corresponds to the following formula:
    //              totalMem = numBytes*8*2+(2*2)
    // The two additional bytes at the end are needed to reset the
    // sequence.
    //
    // If there is not enough memory, we will fall back to cycle counter
    // using DWT
    uint32_t pattern_size = numBytes * 8 * sizeof(uint16_t) + 2 * sizeof(uint16_t);
    uint16_t *pixels_pattern = NULL;
    ws281x_show(pixels, numBytes);
#if 0
#if defined(ARDUINO_NRF52_ADAFRUIT) // use thread-safe malloc
      pixels_pattern = (uint16_t *) rtos_malloc(pattern_size);
#else
      pixels_pattern = (uint16_t *) malloc(pattern_size);
#endif

  // Use the identified device to choose the implementation
  // If a PWM device is available use DMA
  if(pixels_pattern != NULL) {
    uint16_t pos = 0; // bit position

    for(uint16_t n=0; n<numBytes; n++) {
        uint8_t pix = pixels[n];

        for(uint8_t mask=0x80; mask>0; mask >>= 1) {
#if defined(NEO_KHZ400)
        if( !is800KHz ) {
            pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H_400KHz : MAGIC_T0H_400KHz;
        }else
#endif
        {
            pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H : MAGIC_T0H;
        }

        pos++;
        }
    }

    // Zero padding to indicate the end of que sequence
    pixels_pattern[pos++] = 0 | (0x8000); // Seq end
    pixels_pattern[pos++] = 0 | (0x8000); // Seq end

    /* 通过DMA的方式将数据传送到外设 */
    ws281x_show(pixels, numBytes);

#if defined(ARDUINO_NRF52_ADAFRUIT) // use thread-safe free
        rtos_free(pixels_pattern);
#else
        free(pixels_pattern);
#endif

    // endTime = micros(); // Save EOD time for latch on next call
  }
#endif
}

/*!
  @brief   Set a pixel's color using separate red, green and blue
           components. If using RGBW pixels, white will be set to 0.
  @param   n  Pixel index, starting from 0.
  @param   r  Red brightness, 0 = minimum (off), 255 = maximum.
  @param   g  Green brightness, 0 = minimum (off), 255 = maximum.
  @param   b  Blue brightness, 0 = minimum (off), 255 = maximum.
*/
void Adafruit_NeoPixel_setPixelColor_rgb(
    uint16_t n, uint8_t r, uint8_t g, uint8_t b)
{

    if (n < numLEDs)
    {
        if (brightness)
        { // See notes in setBrightness()
            r = (r * brightness) >> 8;
            g = (g * brightness) >> 8;
            b = (b * brightness) >> 8;
        }
        uint8_t *p;
        if (wOffset == rOffset)
        {                       // Is an RGB-type strip
            p = &pixels[n * 3]; // 3 bytes per pixel
        }
        else
        {                       // Is a WRGB-type strip
            p = &pixels[n * 4]; // 4 bytes per pixel
            p[wOffset] = 0;     // But only R,G,B passed -- set W to 0
        }
        p[rOffset] = r; // R,G,B always stored
        p[gOffset] = g;
        p[bOffset] = b;
    }
}

/*!
  @brief   Set a pixel's color using separate red, green, blue and white
           components (for RGBW NeoPixels only).
  @param   n  Pixel index, starting from 0.
  @param   r  Red brightness, 0 = minimum (off), 255 = maximum.
  @param   g  Green brightness, 0 = minimum (off), 255 = maximum.
  @param   b  Blue brightness, 0 = minimum (off), 255 = maximum.
  @param   w  White brightness, 0 = minimum (off), 255 = maximum, ignored
              if using RGB pixels.
*/
void Adafruit_NeoPixel_setPixelColor_rgbw(
    uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{

    if (n < numLEDs)
    {
        if (brightness)
        { // See notes in setBrightness()
            r = (r * brightness) >> 8;
            g = (g * brightness) >> 8;
            b = (b * brightness) >> 8;
            w = (w * brightness) >> 8;
        }
        uint8_t *p;
        if (wOffset == rOffset)
        {                       // Is an RGB-type strip
            p = &pixels[n * 3]; // 3 bytes per pixel (ignore W)
        }
        else
        {                       // Is a WRGB-type strip
            p = &pixels[n * 4]; // 4 bytes per pixel
            p[wOffset] = w;     // Store W
        }
        p[rOffset] = r; // Store R,G,B
        p[gOffset] = g;
        p[bOffset] = b;
        /*     printf("rOffset = %d\n",p[rOffset]);
            printf("gOffset = %d\n",p[gOffset]);
            printf("bOffset = %d\n",p[bOffset]); */
    }
}

void Adafruit_NeoPixel_setPixelColor_rgbw_with_max_brightness(
    uint16_t n, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    u8 brightness = 255; // 最大亮度值

    if (n < numLEDs)
    {

        r = (r * brightness) >> 8;
        g = (g * brightness) >> 8;
        b = (b * brightness) >> 8;
        w = (w * brightness) >> 8;

        uint8_t *p;
        if (wOffset == rOffset)
        {                       // Is an RGB-type strip
            p = &pixels[n * 3]; // 3 bytes per pixel (ignore W)
        }
        else
        {                       // Is a WRGB-type strip
            p = &pixels[n * 4]; // 4 bytes per pixel
            p[wOffset] = w;     // Store W
        }
        p[rOffset] = r; // Store R,G,B
        p[gOffset] = g;
        p[bOffset] = b;
        /*     printf("rOffset = %d\n",p[rOffset]);
            printf("gOffset = %d\n",p[gOffset]);
            printf("bOffset = %d\n",p[bOffset]); */
    }
}

/*!
  @brief   Set a pixel's color using a 32-bit 'packed' RGB or RGBW value.
  @param   n  Pixel index, starting from 0.
  @param   c  32-bit color value. Most significant byte is white (for RGBW
              pixels) or ignored (for RGB pixels), next is red, then green,
              and least significant byte is blue.
*/
void Adafruit_NeoPixel_setPixelColor(uint16_t n, uint32_t c)
{
    if (n < numLEDs)
    {
        uint8_t *p,
            r = (uint8_t)(c >> 16),
            g = (uint8_t)(c >> 8),
            b = (uint8_t)c;
        if (brightness)
        { // See notes in setBrightness()
            r = (r * brightness) >> 8;
            g = (g * brightness) >> 8;
            b = (b * brightness) >> 8;
        }
        if (wOffset == rOffset)
        {
            p = &pixels[n * 3];
        }
        else
        {
            p = &pixels[n * 4];
            uint8_t w = (uint8_t)(c >> 24);
            p[wOffset] = brightness ? ((w * brightness) >> 8) : w;
        }
        p[rOffset] = r;
        p[gOffset] = g;
        p[bOffset] = b;
    }
}

void Adafruit_NeoPixel_setPixelColor_with_max_brightness(uint16_t n, uint32_t c)
{
    u8 brightness = 255; // 最大亮度值

    if (n < numLEDs)
    {
        uint8_t *p,
            r = (uint8_t)(c >> 16),
            g = (uint8_t)(c >> 8),
            b = (uint8_t)c;
        if (brightness)
        { // See notes in setBrightness()
            r = (r * brightness) >> 8;
            g = (g * brightness) >> 8;
            b = (b * brightness) >> 8;
        }
        if (wOffset == rOffset)
        {
            p = &pixels[n * 3];
        }
        else
        {
            p = &pixels[n * 4];
            uint8_t w = (uint8_t)(c >> 24);
            p[wOffset] = brightness ? ((w * brightness) >> 8) : w;
        }
        p[rOffset] = r;
        p[gOffset] = g;
        p[bOffset] = b;
    }
}

// 透传写入颜色，没有调整亮度
void Adafruit_NeoPixel_setPixelColor_raw(uint16_t n, uint32_t c)
{
    if (n < numLEDs)
    {
        uint8_t *p,
            r = (uint8_t)(c >> 16),
            g = (uint8_t)(c >> 8),
            b = (uint8_t)c;
        if (wOffset == rOffset)
        {
            p = &pixels[n * 3];
        }
        else
        {
            p = &pixels[n * 4];
            uint8_t w = (uint8_t)(c >> 24);
            p[wOffset] = brightness ? ((w * brightness) >> 8) : w;
        }
        p[rOffset] = r;
        p[gOffset] = g;
        p[bOffset] = b;
    }
}
/*!
  @brief   Fill all or part of the NeoPixel strip with a color.
  @param   c      32-bit color value. Most significant byte is white (for
                  RGBW pixels) or ignored (for RGB pixels), next is red,
                  then green, and least significant byte is blue. If all
                  arguments are unspecified, this will be 0 (off).
  @param   first  Index of first pixel to fill, starting from 0. Must be
                  in-bounds, no clipping is performed. 0 if unspecified.
  @param   count  Number of pixels to fill, as a positive value. Passing
                  0 or leaving unspecified will fill to end of strip.
                  从first开始，填充多少个
*/
void Adafruit_NeoPixel_fill(uint32_t c, uint16_t first, uint16_t count)
{
    uint16_t i, end;

    if (first >= numLEDs)
    {
        return; // If first LED is past end of strip, nothing to do
    }

    // Calculate the index ONE AFTER the last pixel to fill
    if (count == 0)
    {
        // Fill to end of strip
        end = numLEDs;
    }
    else
    {
        // Ensure that the loop won't go past the last pixel
        end = first + count;
        if (end > numLEDs)
            end = numLEDs;
    }

    for (i = first; i < end; i++)
    {
        Adafruit_NeoPixel_setPixelColor(i, c);
    }
}

void Adafruit_NeoPixel_fill_with_max_brightness(uint32_t c, uint16_t first, uint16_t count)
{
    uint16_t i, end;

    if (first >= numLEDs)
    {
        return; // If first LED is past end of strip, nothing to do
    }

    // Calculate the index ONE AFTER the last pixel to fill
    if (count == 0)
    {
        // Fill to end of strip
        end = numLEDs;
    }
    else
    {
        // Ensure that the loop won't go past the last pixel
        end = first + count;
        if (end > numLEDs)
            end = numLEDs;
    }

    for (i = first; i < end; i++)
    {
        Adafruit_NeoPixel_setPixelColor_with_max_brightness(i, c);
    }
}

/*!
  @brief   Convert hue, saturation and value into a packed 32-bit RGB color
           that can be passed to setPixelColor() or other RGB-compatible
           functions.
  @param   hue  An unsigned 16-bit value, 0 to 65535, representing one full
                loop of the color wheel, which allows 16-bit hues to "roll
                over" while still doing the expected thing (and allowing
                more precision than the wheel() function that was common to
                prior NeoPixel examples).
  @param   sat  Saturation, 8-bit value, 0 (min or pure grayscale) to 255
                (max or pure hue). Default of 255 if unspecified.
  @param   val  Value (brightness), 8-bit value, 0 (min / black / off) to
                255 (max or full brightness). Default of 255 if unspecified.
  @return  Packed 32-bit RGB with the most significant byte set to 0 -- the
           white element of WRGB pixels is NOT utilized. Result is linearly
           but not perceptually correct, so you may want to pass the result
           through the gamma32() function (or your own gamma-correction
           operation) else colors may appear washed out. This is not done
           automatically by this function because coders may desire a more
           refined gamma-correction function than the simplified
           one-size-fits-all operation of gamma32(). Diffusing the LEDs also
           really seems to help when using low-saturation colors.
*/
uint32_t Adafruit_NeoPixel_ColorHSV(uint16_t hue, uint8_t sat, uint8_t val)
{

    uint8_t r, g, b;

    // Remap 0-65535 to 0-1529. Pure red is CENTERED on the 64K rollover;
    // 0 is not the start of pure red, but the midpoint...a few values above
    // zero and a few below 65536 all yield pure red (similarly, 32768 is the
    // midpoint, not start, of pure cyan). The 8-bit RGB hexcone (256 values
    // each for red, green, blue) really only allows for 1530 distinct hues
    // (not 1536, more on that below), but the full unsigned 16-bit type was
    // chosen for hue so that one's code can easily handle a contiguous color
    // wheel by allowing hue to roll over in either direction.
    hue = (hue * 1530L + 32768) / 65536;
    // Because red is centered on the rollover point (the +32768 above,
    // essentially a fixed-point +0.5), the above actually yields 0 to 1530,
    // where 0 and 1530 would yield the same thing. Rather than apply a
    // costly modulo operator, 1530 is handled as a special case below.

    // So you'd think that the color "hexcone" (the thing that ramps from
    // pure red, to pure yellow, to pure green and so forth back to red,
    // yielding six slices), and with each color component having 256
    // possible values (0-255), might have 1536 possible items (6*256),
    // but in reality there's 1530. This is because the last element in
    // each 256-element slice is equal to the first element of the next
    // slice, and keeping those in there this would create small
    // discontinuities in the color wheel. So the last element of each
    // slice is dropped...we regard only elements 0-254, with item 255
    // being picked up as element 0 of the next slice. Like this:
    // Red to not-quite-pure-yellow is:        255,   0, 0 to 255, 254,   0
    // Pure yellow to not-quite-pure-green is: 255, 255, 0 to   1, 255,   0
    // Pure green to not-quite-pure-cyan is:     0, 255, 0 to   0, 255, 254
    // and so forth. Hence, 1530 distinct hues (0 to 1529), and hence why
    // the constants below are not the multiples of 256 you might expect.

    // Convert hue to R,G,B (nested ifs faster than divide+mod+switch):
    if (hue < 510)
    { // Red to Green-1
        b = 0;
        if (hue < 255)
        { //   Red to Yellow-1
            r = 255;
            g = hue; //     g = 0 to 254
        }
        else
        {                  //   Yellow to Green-1
            r = 510 - hue; //     r = 255 to 1
            g = 255;
        }
    }
    else if (hue < 1020)
    { // Green to Blue-1
        r = 0;
        if (hue < 765)
        { //   Green to Cyan-1
            g = 255;
            b = hue - 510; //     b = 0 to 254
        }
        else
        {                   //   Cyan to Blue-1
            g = 1020 - hue; //     g = 255 to 1
            b = 255;
        }
    }
    else if (hue < 1530)
    { // Blue to Red-1
        g = 0;
        if (hue < 1275)
        {                   //   Blue to Magenta-1
            r = hue - 1020; //     r = 0 to 254
            b = 255;
        }
        else
        { //   Magenta to Red-1
            r = 255;
            b = 1530 - hue; //     b = 255 to 1
        }
    }
    else
    { // Last 0.5 Red (quicker than % operator)
        r = 255;
        g = b = 0;
    }

    // Apply saturation and value to R,G,B, pack into 32-bit result:
    uint32_t v1 = 1 + val;  // 1 to 256; allows >>8 instead of /255
    uint16_t s1 = 1 + sat;  // 1 to 256; same reason
    uint8_t s2 = 255 - sat; // 255 to 0
    return ((((((r * s1) >> 8) + s2) * v1) & 0xff00) << 8) |
           (((((g * s1) >> 8) + s2) * v1) & 0xff00) |
           (((((b * s1) >> 8) + s2) * v1) >> 8);
}

/*!
  @brief   Query the color of a previously-set pixel.
  @param   n  Index of pixel to read (0 = first).
  @return  'Packed' 32-bit RGB or WRGB value. Most significant byte is white
           (for RGBW pixels) or 0 (for RGB pixels), next is red, then green,
           and least significant byte is blue.
  @note    If the strip brightness has been changed from the default value
           of 255, the color read from a pixel may not exactly match what
           was previously written with one of the setPixelColor() functions.
           This gets more pronounced at lower brightness levels.
*/
uint32_t Adafruit_NeoPixel_getPixelColor(uint16_t n)
{
    if (n >= numLEDs)
        return 0; // Out of bounds, return no color.

    uint8_t *p;

    if (wOffset == rOffset)
    { // Is RGB-type device
        p = &pixels[n * 3];
        if (brightness)
        {
            // Stored color was decimated by setBrightness(). Returned value
            // attempts to scale back to an approximation of the original 24-bit
            // value used when setting the pixel color, but there will always be
            // some error -- those bits are simply gone. Issue is most
            // pronounced at low brightness levels.
            return (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
                   (((uint32_t)(p[gOffset] << 8) / brightness) << 8) |
                   ((uint32_t)(p[bOffset] << 8) / brightness);
        }
        else
        {
            // No brightness adjustment has been made -- return 'raw' color
            return ((uint32_t)p[rOffset] << 16) |
                   ((uint32_t)p[gOffset] << 8) |
                   (uint32_t)p[bOffset];
        }
    }
    else
    { // Is RGBW-type device
        p = &pixels[n * 4];
        if (brightness)
        { // Return scaled color
            return (((uint32_t)(p[wOffset] << 8) / brightness) << 24) |
                   (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
                   (((uint32_t)(p[gOffset] << 8) / brightness) << 8) |
                   ((uint32_t)(p[bOffset] << 8) / brightness);
        }
        else
        { // Return raw color
            return ((uint32_t)p[wOffset] << 24) |
                   ((uint32_t)p[rOffset] << 16) |
                   ((uint32_t)p[gOffset] << 8) |
                   (uint32_t)p[bOffset];
        }
    }
}

uint32_t Adafruit_NeoPixel_getPixelColor_with_max_brightness(uint16_t n)
{
    if (n >= numLEDs)
        return 0; // Out of bounds, return no color.

    uint8_t *p;
    u8 brightness = 255; // 最大亮度值

    if (wOffset == rOffset)
    { // Is RGB-type device
        p = &pixels[n * 3];
        if (brightness)
        {
            // Stored color was decimated by setBrightness(). Returned value
            // attempts to scale back to an approximation of the original 24-bit
            // value used when setting the pixel color, but there will always be
            // some error -- those bits are simply gone. Issue is most
            // pronounced at low brightness levels.
            return (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
                   (((uint32_t)(p[gOffset] << 8) / brightness) << 8) |
                   ((uint32_t)(p[bOffset] << 8) / brightness);
        }
        else
        {
            // No brightness adjustment has been made -- return 'raw' color
            return ((uint32_t)p[rOffset] << 16) |
                   ((uint32_t)p[gOffset] << 8) |
                   (uint32_t)p[bOffset];
        }
    }
    else
    { // Is RGBW-type device
        p = &pixels[n * 4];
        if (brightness)
        { // Return scaled color
            return (((uint32_t)(p[wOffset] << 8) / brightness) << 24) |
                   (((uint32_t)(p[rOffset] << 8) / brightness) << 16) |
                   (((uint32_t)(p[gOffset] << 8) / brightness) << 8) |
                   ((uint32_t)(p[bOffset] << 8) / brightness);
        }
        else
        { // Return raw color
            return ((uint32_t)p[wOffset] << 24) |
                   ((uint32_t)p[rOffset] << 16) |
                   ((uint32_t)p[gOffset] << 8) |
                   (uint32_t)p[bOffset];
        }
    }
}

/* 获取原始颜色，没有进行亮度调整的颜色 */
uint32_t Adafruit_NeoPixel_getOriginPixelColor(uint16_t n)
{
    if (n >= numLEDs)
        return 0; // Out of bounds, return no color.

    uint8_t *p;

    if (wOffset == rOffset)
    { // Is RGB-type device
        p = &pixels[n * 3];
        // No brightness adjustment has been made -- return 'raw' color
        return ((uint32_t)p[rOffset] << 16) |
               ((uint32_t)p[gOffset] << 8) |
               (uint32_t)p[bOffset];
    }
    else
    { // Is RGBW-type device
        p = &pixels[n * 4];
        return ((uint32_t)p[wOffset] << 24) |
               ((uint32_t)p[rOffset] << 16) |
               ((uint32_t)p[gOffset] << 8) |
               (uint32_t)p[bOffset];
    }
}

/*!
  @brief   Adjust output brightness. Does not immediately affect what's
           currently displayed on the LEDs. The next call to show() will
           refresh the LEDs at this level.
  @param   b  Brightness setting, 0=minimum (off), 255=brightest.
  @note    This was intended for one-time use in one's setup() function,
           not as an animation effect in itself. Because of the way this
           library "pre-multiplies" LED colors in RAM, changing the
           brightness is often a "lossy" operation -- what you write to
           pixels isn't necessary the same as what you'll read back.
           Repeated brightness changes using this function exacerbate the
           problem. Smart programs therefore treat the strip as a
           write-only resource, maintaining their own state to render each
           frame of an animation, not relying on read-modify-write.
*/
void Adafruit_NeoPixel_setBrightness(uint8_t b)
{
    // Stored brightness value is different than what's passed.
    // This simplifies the actual scaling math later, allowing a fast
    // 8x8-bit multiply and taking the MSB. 'brightness' is a uint8_t,
    // adding 1 here may (intentionally) roll over...so 0 = max brightness
    // (color values are interpreted literally; no scaling), 1 = min
    // brightness (off), 255 = just below max brightness.
    uint8_t newBrightness = b + 1;
    if (newBrightness != brightness)
    { // Compare against prior value
        // Brightness has changed -- re-scale existing data in RAM,
        // This process is potentially "lossy," especially when increasing
        // brightness. The tight timing in the WS2811/WS2812 code means there
        // aren't enough free cycles to perform this scaling on the fly as data
        // is issued. So we make a pass through the existing color data in RAM
        // and scale it (subsequent graphics commands also work at this
        // brightness level). If there's a significant step up in brightness,
        // the limited number of steps (quantization) in the old data will be
        // quite visible in the re-scaled version. For a non-destructive
        // change, you'll need to re-render the full strip data. C'est la vie.
        uint8_t c,
            *ptr = pixels,
            oldBrightness = brightness - 1; // De-wrap old brightness value
        uint16_t scale;
        if (oldBrightness == 0)
            scale = 0; // Avoid /0
        else if (b == 255)
            scale = 65535 / oldBrightness;
        else
            scale = (((uint16_t)newBrightness << 8) - 1) / oldBrightness;
        for (uint16_t i = 0; i < numBytes; i++)
        {
            c = *ptr;
            *ptr++ = (c * scale) >> 8;
        }
        brightness = newBrightness;
    }
}

/*!
  @brief   Retrieve the last-set brightness value for the strip.
  @return  Brightness value: 0 = minimum (off), 255 = maximum.
*/
uint8_t Adafruit_NeoPixel_getBrightness(void)
{
    return brightness - 1;
}

/*!
  @brief   Fill the whole NeoPixel strip with 0 / black / off.
*/
void Adafruit_NeoPixel_clear(void)
{
    memset(pixels, 0, numBytes);
}

/*!
  @brief   An 8-bit integer sine wave function, not directly compatible
            with standard trigonometric units like radians or degrees.
  @param   x  Input angle, 0-255; 256 would loop back to zero, completing
              the circle (equivalent to 360 degrees or 2 pi radians).
              One can therefore use an unsigned 8-bit variable and simply
              add or subtract, allowing it to overflow/underflow and it
              still does the expected contiguous thing.
  @return  Sine result, 0 to 255, or -128 to +127 if type-converted to
            a signed int8_t, but you'll most likely want unsigned as this
            output is often used for pixel brightness in animation effects.
*/
uint8_t Adafruit_NeoPixel_sine8(uint8_t x)
{
    return _NeoPixelSineTable[x]; // 0-255 in, 0-255 out
}

/*!
  @brief   An 8-bit gamma-correction function for basic pixel brightness
            adjustment. Makes color transitions appear more perceptially
            correct.
  @param   x  Input brightness, 0 (minimum or off/black) to 255 (maximum).
  @return  Gamma-adjusted brightness, can then be passed to one of the
            setPixelColor() functions. This uses a fixed gamma correction
            exponent of 2.6, which seems reasonably okay for average
            NeoPixels in average tasks. If you need finer control you'll
            need to provide your own gamma-correction function instead.
*/
uint8_t Adafruit_NeoPixel_gamma8(uint8_t x)
{
    return _NeoPixelGammaTable[x]; // 0-255 in, 0-255 out
}

/*!
  @brief   Convert separate red, green and blue values into a single
            "packed" 32-bit RGB color.
  @param   r  Red brightness, 0 to 255.
  @param   g  Green brightness, 0 to 255.
  @param   b  Blue brightness, 0 to 255.
  @return  32-bit packed RGB value, which can then be assigned to a
            variable for later use or passed to the setPixelColor()
            function. Packed RGB format is predictable, regardless of
            LED strand color order.
*/
uint32_t Adafruit_NeoPixel_Color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/*!
  @brief   Convert separate red, green, blue and white values into a
            single "packed" 32-bit WRGB color.
  @param   r  Red brightness, 0 to 255.
  @param   g  Green brightness, 0 to 255.
  @param   b  Blue brightness, 0 to 255.
  @param   w  White brightness, 0 to 255.
  @return  32-bit packed WRGB value, which can then be assigned to a
            variable for later use or passed to the setPixelColor()
            function. Packed WRGB format is predictable, regardless of
            LED strand color order.
*/
uint32_t Adafruit_NeoPixel_Color_rgbw(uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    return ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/*!
  @brief   A gamma-correction function for 32-bit packed RGB or WRGB
            colors. Makes color transitions appear more perceptially
            correct.
  @param   x  32-bit packed RGB or WRGB color.
  @return  Gamma-adjusted packed color, can then be passed in one of the
            setPixelColor() functions. Like gamma8(), this uses a fixed
            gamma correction exponent of 2.6, which seems reasonably okay
            for average NeoPixels in average tasks. If you need finer
            control you'll need to provide your own gamma-correction
            function instead.
*/
// A 32-bit variant of gamma8() that applies the same function
// to all components of a packed RGB or WRGB value.
uint32_t Adafruit_NeoPixel_gamma32(uint32_t x)
{
    uint8_t *y = (uint8_t *)&x;
    // All four bytes of a 32-bit value are filtered even if RGB (not WRGB),
    // to avoid a bunch of shifting and masking that would be necessary for
    // properly handling different endianisms (and each byte is a fairly
    // trivial operation, so it might not even be wasting cycles vs a check
    // and branch for the RGB case). In theory this might cause trouble *if*
    // someone's storing information in the unused most significant byte
    // of an RGB value, but this seems exceedingly rare and if it's
    // encountered in reality they can mask values going in or coming out.
    for (uint8_t i = 0; i < 4; i++)
        y[i] = Adafruit_NeoPixel_gamma8(y[i]);
    return x; // Packed 32-bit return
}

/*!
  @brief   Get a pointer directly to the NeoPixel data buffer in RAM.
            Pixel data is stored in a device-native format (a la the NEO_*
            constants) and is not translated here. Applications that access
            this buffer will need to be aware of the specific data format
            and handle colors appropriately.
  @return  Pointer to NeoPixel buffer (uint8_t* array).
  @note    This is for high-performance applications where calling
            setPixelColor() on every single pixel would be too slow (e.g.
            POV or light-painting projects). There is no bounds checking
            on the array, creating tremendous potential for mayhem if one
            writes past the ends of the buffer. Great power, great
            responsibility and all that.
*/
uint8_t *Adafruit_NeoPixel_getPixels(void)
{
    return pixels;
}

/*!
@brief   Return the number of pixels in an Adafruit_NeoPixel strip object.
@return  Pixel count (0 if not set).
*/
uint16_t Adafruit_NeoPixel_numPixels(void)
{
    return numLEDs;
}

uint8_t Adafruit_NeoPixel_getNumBytesPerPixel(void)
{
    return (wOffset == rOffset) ? 3 : 4; // 3=RGB  4=RGBW
}

uint16_t Adafruit_NeoPixel_getNumBytes(void)
{
    return numBytes;
}
