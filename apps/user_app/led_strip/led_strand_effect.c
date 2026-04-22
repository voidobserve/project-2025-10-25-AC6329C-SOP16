#include "system/includes.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "ws2812fx_effect.h"
#include "Adafruit_NeoPixel.H"
#include "led_strip_drive.h"
#include "app_main.h"
#include "asm/mcpwm.h"

#include "../../../apps/user_app/ws2812-fx-lib/WS2812FX_C/WS2812FX.H"
#include "../../../apps/user_app/rf433_key/rf433_learn.h"

volatile fc_effect_t fc_effect; // 幻彩灯串效果数据
void set_fc_effect(void);

// FADE_SLOW：12颗
// FADE_MEDIUM：6颗
// FADE_FAST：5颗灯
// FADE_XFAST:3颗灯
const u8 fade_type[3] = {
    FADE_XFAST, FADE_FAST, FADE_MEDIUM // FADE_SLOW
};

#define segment_num 1
#define _0_seg_start 0
#define _0_seg_stop 0

/**
 * @brief 设置段的颜色
 *
 * @param n
 * @param c
 */
void ls_set_colors(uint8_t n, color_t *c)
{
    uint32_t colors[MAX_NUM_COLORS];
    uint8_t i;

#if LED_STRIP_RGBW

    for (i = 0; i < n; i++)
    {
        colors[i] = (u32)c[i].w << 24 | (u32)c[i].r << 16 | (u32)c[i].g << 8 | (u32)c[i].b;
    }

#elif LED_STRIP_RGB

    for (i = 0; i < n; i++)
    {
        colors[i] = c[i].r << 16 | c[i].g << 8 | c[i].b;
    }

#endif

    WS2812FX_setColors(0, colors);
}

// 设置fc_effect.dream_scene.rgb的颜色池
// n:0-MAX_NUM_COLORS
// c:WS2812FX颜色系，R<<16,G<<8,B在低8位
void ls_set_color(uint8_t n, uint32_t c)
{
    if (n < MAX_NUM_COLORS)
    {

#if LED_STRIP_RGBW
        fc_effect.dream_scene.rgb[n].w = (c >> 24) & 0xff;
        fc_effect.dream_scene.rgb[n].r = (c >> 16) & 0xff;
        fc_effect.dream_scene.rgb[n].g = (c >> 8) & 0xff;
        fc_effect.dream_scene.rgb[n].b = c & 0xff;
#elif LED_STRIP_RGB
        fc_effect.dream_scene.rgb[n].r = (c >> 16) & 0xff;
        fc_effect.dream_scene.rgb[n].g = (c >> 8) & 0xff;
        fc_effect.dream_scene.rgb[n].b = c & 0xff;

#endif
    }
}

//====================================================================================================
//====================================================================================================
//====================================================================================================

/*----------------------------------静态色效果----------------------------------*/
static void static_mode(void)
{
    extern uint16_t WS2812FX_mode_static(void);

    WS2812FX_setSegment_colorOptions(                   // 设置一段颜色的效果
        0,                                              // 第0段
        0, 0,                                           // 起始位置，结束位置
        &WS2812FX_mode_static,                          // 效果
        0,                                              // 颜色，WS2812FX_setColors设置
        100,                                            // 速度
        0);                                             // 选项，这里像素点大小：1
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
    ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

    // WS2812FX_start(); // 不能在这里清空显示的缓存，会导致流星灯也闪烁（流星灯会重新开始跑）
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();

    // printf("fc_effect.rgb.r %u\n", fc_effect.rgb.r);
    // printf("fc_effect.rgb.g %u\n", fc_effect.rgb.g);
    // printf("fc_effect.rgb.b %u\n", fc_effect.rgb.b);
    // printf("fc_effect.rgb.w %u\n", fc_effect.rgb.w);
    // printf("%s %u\n", __func__, __LINE__);
}

/*----------------------------------彩虹效果----------------------------------*/
static void strand_rainbow(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mode_mutil_fade,   // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        SIZE_SMALL);                 // 选项，这里像素点大小：1

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}

/*----------------------------------跳变效果----------------------------------*/
void strand_jump_change(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                // 第0段
        0, 0,                             // 起始位置，结束位置
        &WS2812FX_mode_single_block_scan, // 效果
        0,                                // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,      // 速度
        SIZE_MEDIUM);                     // 选项，这里像素点大小：3

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}
/*----------------------------------呼吸系列效果----------------------------------*/
void strand_breath(void)
{
    // WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mode_mutil_breath, // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        SIZE_MEDIUM                  // 选项，这里像素点大小：3
        // NO_OPTIONS                  // 选项
    );

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
    // printf("__FUNC__ %s \n__LINE__ %d\n", __func__, __LINE__);
}

void single_c_breath(void)
{

    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mode_breath,       // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速 度
        SIZE_MEDIUM                  // 选项，这里像素点大小：3
    );

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}

/*----------------------------------闪烁效果----------------------------------*/
void strand_twihkle(void)
{

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0, 0,                            // 起始位置，结束位置
        &WS2812FX_mode_mutil_twihkle,    // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed * 4, // 速度
        SIZE_SMALL);                     // 选项，这里像素点大小：1
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}
// 多颜色频闪
void ls_strobe(void)
{

    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0, 0,                            // 起始位置，结束位置
        &WS2812FX_mutil_strobe,          // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed * 5, // 速度
        0);                              // 选项，这里像素点大小：3

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}
/*----------------------------------流水效果----------------------------------*/
void strand_flow_water(void)
{
    uint8_t option;
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = SIZE_MEDIUM | 0;
    }
    else
    {
        option = SIZE_MEDIUM | REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0, 0,                            // 起始位置，结束位置
        &WS2812FX_mode_multi_block_scan, // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,     // 速度
        option);                         // 选项，这里像素点大小：3,反向/反向
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}

/*----------------------------------追光效果----------------------------------*/
void strand_chas_light(void)
{

    WS2812FX_stop();
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        WS2812FX_setSegment_colorOptions(
            0,                                 // 第0段
            0, 0,                              // 起始位置，结束位置
            &WS2812FX_mode_multi_forward_same, // 效果
            0,                                 // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,       // 速度
            0);                                // 选项
    }
    else
    {
        WS2812FX_setSegment_colorOptions(
            0,                              // 第0段
            0, 0,                           // 起始位置，结束位置
            &WS2812FX_mode_multi_back_same, // 效果
            0,                              // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed,    // 速度
            0);
    }
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}

/*----------------------------------炫彩效果----------------------------------*/
void strand_colorful(void)
{
    uint8_t option;
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                               // 第0段
        0, 0,                            // 起始位置，结束位置
        &WS2812FX_mode_multi_block_scan, // 效果
        0,                               // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed,     // 速度
        SIZE_SMALL);                     // 选项，这里像素点大小：1
    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}

/*----------------------------------渐变系列效果----------------------------------*/
void strand_grandual(void)
{
    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mode_mutil_fade,   // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        SIZE_MEDIUM);                // 选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}
// 整条灯带渐变，支持多种颜色之间切换
// 颜色池：fc_effect.dream_scene.rgb[]
// 颜色数量fc_effect.dream_scene.c_n
void mutil_c_grandual(void)
{
    extern uint16_t WS2812FX_mutil_c_gradual(void);
    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &WS2812FX_mutil_c_gradual,   // 效果
        0,                           // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        SIZE_MEDIUM);                // 选项，这里像素点大小：3,反向/反向

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}

// 纯白色渐变
void w_grandual(void)
{

    extern uint16_t breath_w(void);

    WS2812FX_setSegment_colorOptions(
        0,                           // 第0段
        0, 0,                        // 起始位置，结束位置
        &breath_w,                   // 效果
        WHITE,                       // 颜色，WS2812FX_setColors设置
        fc_effect.dream_scene.speed, // 速度
        0);                          // 选项，这里像素点大小：3,反向/反向

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}

/*----------------------------------跳变效果----------------------------------*/
void standard_jump(void)
{
    extern uint16_t WS2812FX_mutil_c_jump(void);
    // WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        0,                                  // 第0段
        0, 0,                               // 起始位置，结束位置
        &WS2812FX_mutil_c_jump,             // 效果
        0,                                  // 颜色，WS2812FX_setColors设置
        (fc_effect.dream_scene.speed * 40), // 速度
        0);                                 // 选项，这里像素点大小：3

    WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
    ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();
}

//====================================================================================================
//====================================================================================================
//====================================================================================================

void strand_meteor(u8 index)
{
    uint8_t option;
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else
    {
        option = REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        1,                    // 第0段
        1, fc_effect.led_num, // 起始位置，结束位置
        // &WS2812FX_mode_comet_1,          // 效果
        &WS2812FX_mode_comet_1_with_max_brightness, // 效果
        WHITE,                                      // 颜色，WS2812FX_setColors设置
        fc_effect.star_speed,                       // 速度
        fade_type[index - 19] | option);            // 选项，这里像素点大小：3,反向/反向
    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(1); // 重置流星灯所在的段运行时参数
    WS2812FX_running_flag_set();
}

void double_meteor(void)
{
    extern uint16_t fc_double_meteor(void);
    uint8_t option;
    // 正向
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        option = 0;
    }
    else
    {
        option = REVERSE;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        1,                    // 第0段
        1, fc_effect.led_num, // 起始位置，结束位置
        // &fc_double_meteor,    // 效果
        &fc_double_meteor_with_max_brightness, // 效果
        WHITE,                                 // 颜色，WS2812FX_setColors设置
        fc_effect.star_speed,                  // 速度
        option);                               // 选项，这里像素点大小：3,反向/反向

    // WS2812FX_start();
    WS2812FX_resetSegmentRuntime(1); // 重置流星灯所在的段运行时参数
    WS2812FX_running_flag_set();
}

//====================================================================================================
//====================================================================================================
//====================================================================================================

/**
 * @brief APP模式中，基本的七彩动态效果集合
 *
 * @param tp_num
 */
void base_Dynamic_Effect(u8 tp_num)
{
    switch (tp_num)
    {
    case 0x07: // 3色跳变
    {
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_JUMP;
        fc_effect.dream_scene.c_n = 3;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x08: // 7色跳变
    {
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_JUMP;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x09: // 3色渐变
    {
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_GRADUAL;
        fc_effect.dream_scene.c_n = 3;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x0A:
    { //  七色渐变
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_GRADUAL;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x0B:

    { // 红色呼吸
        ls_set_color(0, RED);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x0c:

    { // 绿色呼吸
        ls_set_color(0, GREEN);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }

    break;
        // ==============================================================================
    case 0x0D:

    { // 蓝色呼吸
        ls_set_color(0, BLUE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }

    break;
        // ==============================================================================
    case 0x0E:

    { // 青色呼吸
        ls_set_color(0, CYAN);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x0F:

    { // 黄色呼吸
        ls_set_color(0, YELLOW);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x10:

    { // 紫色呼吸
        ls_set_color(0, PURPLE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x11:

    { // 纯白色呼吸
        ls_set_color(0, PURE_WHITE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x12:
    { // 红绿渐变
        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_GRADUAL;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x13:

    { // 红蓝渐变
        ls_set_color(0, BLUE);
        ls_set_color(1, RED);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_GRADUAL;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x14:
    { // 绿蓝渐变
        ls_set_color(0, GREEN);
        ls_set_color(1, BLUE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_GRADUAL;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x15:
    { // 七色频闪
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);

        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x16:
    { // 红色频闪
        ls_set_color(0, RED);
        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x17:
    { // 绿色频闪
        ls_set_color(0, GREEN);
        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x18:
    { // 蓝色频闪
        ls_set_color(0, BLUE);
        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x19:
    { // 青色频闪
        ls_set_color(0, CYAN);
        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x1a:
    { // 黄色频闪
        ls_set_color(0, YELLOW);
        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x1b:
    { // 紫色频闪
        ls_set_color(0, PURPLE);
        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x1c:
    { // 纯白色频闪
        ls_set_color(0, PURE_WHITE);
        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x1d:
    {
        // 七彩呼吸
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
    // ==============================================================================
    case 0x1E:
    {
        // 蓝白呼吸（蓝色白色同时呼吸）
        ls_set_color(0, BLUE | PURE_WHITE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 1;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x1F:
    {
        // 蓝白渐变（纯白色渐变到蓝色，再渐变到纯白色，循环）
        ls_set_color(0, BLUE);
        ls_set_color(1, PURE_WHITE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_GRADUAL;
        fc_effect.dream_scene.c_n = 2;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    case 0x20:
    {
        // 蓝色呼吸、纯白色呼吸、蓝白呼吸（蓝色和纯白色同时呼吸）
        ls_set_color(0, BLUE);
        ls_set_color(1, PURE_WHITE);
        ls_set_color(2, BLUE | PURE_WHITE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 3;
        fc_effect.Now_state = IS_light_scene;
    }
    break;
        // ==============================================================================
    }

    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存（调用相应的动画前，需要重新开始跑）
    set_fc_effect();
}

/**
 * @brief 情景效果集合
 *
 */
static void ls_scene_effect(void)
{
    // app_set_bright(100);
    switch (fc_effect.dream_scene.change_type)
    {

    case MODE_MUTIL_RAINBOW: // 彩虹
        strand_rainbow();
        break;

    case MODE_MUTIL_JUMP: // 跳变模式
        strand_jump_change();
        break;

    case MODE_MUTIL_BRAETH: // 呼吸模式
        // printf("__FUNC__ %s \n__LINE__ %d\n", __func__, __LINE__);
        strand_breath();
        break;

    case MODE_MUTIL_TWIHKLE: // 闪烁模式
        strand_twihkle();
        break;

    case MODE_MUTIL_FLOW_WATER: // 流水模式
        strand_flow_water();
        break;

    case MODE_CHAS_LIGHT: // 追光模式
        strand_chas_light();
        break;

    case MODE_MUTIL_COLORFUL: // 炫彩模式
        strand_colorful();
        break;

    case MODE_MUTIL_SEG_GRADUAL: // 渐变模式
        strand_grandual();
        break;

    case MODE_JUMP: // 标准跳变
        standard_jump();
        break;

    case MODE_MUTIL_C_GRADUAL: // 多段同时渐变
        mutil_c_grandual();
        break;

    case MODE_BREATH_W: // 白色渐变
        w_grandual();
        break;

    case MODE_STROBE: // 标准频闪
        ls_strobe();
        break;

    case MODE_SINGLE_C_BREATH:
        single_c_breath();
        break;

    case MODO_COLORFUL_LIGHTS_FLASH: // 七彩灯频闪

        WS2812FX_setSegment_colorOptions(
            0,                           // 第0段
            0,                           // 起始位置
            0,                           // 结束位置
            &colorful_lights_flash,      // 效果  // 七彩灯渐变
            0,                           // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed, // 速 度
            NO_OPTIONS                   // 选项
        );

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量
        ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
        WS2812FX_running_flag_set();

        break;

    case MODE_COLORFUL_LIGHTS_JUMP: // 七彩灯跳变

        WS2812FX_setSegment_colorOptions(
            0,                           // 第0段
            0,                           // 起始位置
            0,                           // 结束位置
            &colorful_lights_jump,       // 效果  // 七彩灯渐变
            0,                           // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed, // 速 度
            NO_OPTIONS                   // 选项
        );

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量
        ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
        WS2812FX_running_flag_set();

        break;

    case MODE_COLORFUL_LIGHTS_GRADUAL: // 七彩灯渐变

        // 注意，提供的颜色数量至少要有两个，否则函数内部会越界访问

        WS2812FX_setSegment_colorOptions(
            0,                           // 第0段
            0,                           // 起始位置
            0,                           // 结束位置
            &colorful_lights_gradual,    // 效果  // 七彩灯渐变
            0,                           // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed, // 速 度
            NO_OPTIONS                   // 选项
        );

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
        ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
        WS2812FX_running_flag_set();
        break;

    case MODE_COLORFUL_LIGHTS_BREATH: // 七彩灯呼吸
    {
        /*
            注意，进入这里不会重新开始跑动画（样机对应的七彩灯呼吸在调节速度不会重新开始跑动画）

            如果需要重新开始跑动画，在进入这里之前，需要先调用 WS2812FX_resetSegmentRuntime(0);
        */
        WS2812FX_setSegment_colorOptions(
            0,                           // 第0段
            0,                           // 起始位置
            0,                           // 结束位置
            &colorful_lights_breathing,  // 效果  // 七彩灯呼吸
            0,                           // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed, // 速 度
            NO_OPTIONS                   // 选项
        );

        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量
        ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
        WS2812FX_running_flag_set();
    }
    break;

    case MODE_COLORFUL_LIGHTS_AUTO: // 七彩灯的自动模式

        WS2812FX_setSegment_colorOptions(
            0,                           // 第0段
            0,                           // 起始位置
            0,                           // 结束位置
            &colorful_lights_auto,       // 效果  // 七彩灯自动（颜色和颜色数量在函数内部设置，这里不用再设置）
            0,                           // 颜色，WS2812FX_setColors设置
            fc_effect.dream_scene.speed, // 速 度
            NO_OPTIONS                   // 选项
        );

        // WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量
        // ls_set_colors(fc_effect.dream_scene.c_n, &fc_effect.dream_scene.rgb);
        WS2812FX_running_flag_set();
        break;

    default:
        break;
    }
}

/**
 * @brief 涂鸦的配对效果
 *
 */
static void ls_ty_pair_effect(void)
{
}

/**
 * @brief 自定义效果集合
 *
 */
static void ls_custom_effect(void)
{
}

/**
 * @brief 光纤灯的流星集合
 *
 */
void ls_meteor_stat_effect(void)
{
    mode_ptr meteor_light_mode_ptr = NULL;
    u8 opt = NO_OPTIONS;

    if (fc_effect.star_on_off == DEVICE_OFF)
    {
        // 流星灯当前是关闭的，不调节
        return;
    }

    printf("fc_effect.star_index == %u\n", (u16)fc_effect.star_index);

    // 流星效果
    if (fc_effect.star_index == 1)
    {
        // 单流星
        meteor_light_mode_ptr = WS2812FX_mode_comet_1_with_max_brightness;
        opt = FADE_FAST; // 选项
    }
    else if (fc_effect.star_index == 2)
    {
        // 单流星
        meteor_light_mode_ptr = WS2812FX_mode_comet_1_with_max_brightness;
        opt = FADE_SLOW; // 选项
    }
    else if (fc_effect.star_index == 3)
    { // 单流星（反向）
        meteor_light_mode_ptr = WS2812FX_mode_comet_1_with_max_brightness;
        opt = FADE_FAST | REVERSE; // 选项
    }
    else if (fc_effect.star_index == 4)
    {
        meteor_light_mode_ptr = WS2812FX_mode_comet_1_with_max_brightness;
        opt = FADE_SLOW | REVERSE; // 选项
    }
    else if (fc_effect.star_index == 5)
    {
        meteor_light_mode_ptr = WS2812FX_mode_comet_4_with_max_brightness;
        opt = NO_OPTIONS; // 选项
    }
    else if (fc_effect.star_index == 6)
    {
        // 两段流星灯追逐
        meteor_light_mode_ptr = meteor_lights_chase_with_max_brightness;
        opt = NO_OPTIONS; // 选项
    }
    else if (fc_effect.star_index == 7)
    {
        // 两段流星灯追逐
        meteor_light_mode_ptr = meteor_lights_chase_with_max_brightness;
        opt = REVERSE; // 选项
    }
    else if (fc_effect.star_index == 8)
    {
        // 改成先流星上半，时间间隔结束后，流星下半
        meteor_light_mode_ptr = meteor_lights_half_flow_with_max_brightness;
        opt = FADE_MEDIUM; // 选项
    }
    else if (fc_effect.star_index == 9)
    {
        // 改成先流星上半，时间间隔结束后，流星下半
        meteor_light_mode_ptr = meteor_lights_half_flow_with_max_brightness;
        opt = REVERSE | FADE_MEDIUM; // 选项
    }
    else if (fc_effect.star_index == 10)
    {
        // 单点流水，最后四个灯堆积
        meteor_light_mode_ptr = meteor_lights_single_flow_and_stack_with_max_brightness;
        opt = NO_OPTIONS; // 选项
    }
    else if (fc_effect.star_index == 11)
    {
        // 单点流水，最后四个灯堆积（反向）
        meteor_light_mode_ptr = meteor_lights_single_flow_and_stack_with_max_brightness;
        opt = REVERSE; // 选项
    }
    else if (fc_effect.star_index == 12)
    {
        // 堆积流水
        meteor_light_mode_ptr = meteor_lights_stack_flow_with_max_brightness;
        opt = NO_OPTIONS; // 选项
    }
    else if (fc_effect.star_index == 13)
    {
        // 堆积流水（反向）
        meteor_light_mode_ptr = meteor_lights_stack_flow_with_max_brightness;
        opt = REVERSE; // 选项
    }
    else if (fc_effect.star_index == 14)
    {
        // 堆积流水(正向) + 堆积流水(反向)
        meteor_light_mode_ptr = meteor_lights_stack_flow_plus_reverse_with_max_brightness;
        opt = NO_OPTIONS; // 选项
    }
    else if (fc_effect.star_index == 15)
    {
        // 音乐律动1
        meteor_light_mode_ptr = meteor_with_max_brightness;
        opt = NO_OPTIONS; // 选项
    }
    else if (fc_effect.star_index == 16)
    {
        // 音乐律动2
        meteor_light_mode_ptr = music_meteor3_with_max_brightness;
        opt = NO_OPTIONS; // 选项
    }
    else
    {
        // index 不在 索引范围内，直接返回
        return;
    }

    WS2812FX_stop();
    WS2812FX_setSegment_colorOptions(
        1,                           // 第 x 段
        1,                           // 起始位置
        fc_effect.led_num - 1,       // 结束位置
        meteor_light_mode_ptr,      // 效果
        WHITE,                       // 颜色
        fc_effect.star_speed,        // 速度
        opt);                        // 选项
    WS2812FX_resetSegmentRuntime(1); // 重置流星灯所在的段运行时参数
    WS2812FX_running_flag_set();
    os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
}

/**
 * @brief 音乐效果集合
 *
 */
static void ls_music_effect(void)
{
#if 1
    void *music_effect_addr = NULL;
    switch (fc_effect.music.m)
    {
    case 0:
        music_effect_addr = &colorful_lights_sound_gradual_max_brightness;
        break;
    case 1:
        music_effect_addr = &colorful_lights_sound_breath_max_brightness;
        break;
    case 2:
        music_effect_addr = &colorful_lights_sound_static_max_brightness;
        break;
    case 3:
        music_effect_addr = &colorful_lights_sound_twinkle_max_brightness;
        break;
    default:
        break;
    }

    WS2812FX_setSegment_colorOptions(
        0,                       // 第0段
        0,                       // 起始位置
        0,                       // 结束位置
        music_effect_addr,       // 效果
        WHITE,                   // 颜色，WS2812FX_setColors设置
        100,                     // 速度
        SIZE_MEDIUM | FADE_XSLOW // 选项，这里像素点大小：3,反向/反向
    );
    WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
    WS2812FX_running_flag_set();     //
#endif
}

/**
 * @brief 七彩灯的声控模式效果
 *
 */
// void colorful_light_music_mode_effect(void)
// {
//     if (DEVICE_OFF == fc_effect.on_off_flag)
//     {
//         return;
//     }

//     switch (fc_effect.dream_scene.change_type)
//     {
//     case MODE_COLORFUL_MUSIC_SINGLE_COLOR:
//     {
//         WS2812FX_stop();
//         WS2812FX_setSegment_colorOptions(
//             0,                                  // 第0段
//             0,                                  // 起始位置
//             0,                                  // 结束位置
//             &colorful_lights_music_mode_single, // 效果
//             0,                                  // 颜色
//             0,                                  // 速度（这里是由声控决定，跟速度无关）
//             NO_OPTIONS);                        // 选项
//         WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n);
//         ls_set_colors(1, &fc_effect.dream_scene.rgb);
//         WS2812FX_resetSegmentRuntime(0); // 清除指定段的显示缓存
//         WS2812FX_running_flag_set();
//     }
//     break;

//     case MODE_COLORFUL_MUSIC_DYNAMIC:
//     {
//     }
//     break;

//     default:
//     {
//     }
//     break;
//     }
// }

/**
 * @brief 涂抹效果集合
 *
 */
static void ls_smear_adjust_effect(void)
{
}

/**
 * @brief 静态效果集合
 *
 */
static void ls_static_effect(void)
{
    static_mode();
}

//====================================================================================================
//====================================================================================================
//====================================================================================================

/**
 * @brief 七彩灯 灯光模式总调度
 *
 */
void set_fc_effect(void)
{
    if (DEVICE_OFF == fc_effect.on_off_flag)
    {
        return;
    }

#if RF_433_LEARN_ENABLE
    if (RF_433_LEARN_STATUS_PROCESSING == rf_433_learn_status_get())
    {
        // 正在执行七彩灯对码成功的动画，直接退出
        return;
    }
#endif // #if RF_433_LEARN_ENABLE

    switch (fc_effect.Now_state)
    {
    // 幻彩场景
    case IS_light_scene:
        ls_scene_effect();
        break;

    // 配对模式
    case ACT_TY_PAIR:
        // ls_ty_pair_effect();
        break;

    // 自定义效果模式
    case ACT_CUSTOM:
        // ls_custom_effect();
        break;

    // 音乐模式
    case IS_light_music:
        ls_music_effect();

        break;

    // 涂抹模式
    case IS_smear_adjust:
        // ls_smear_adjust_effect();
        break;

    // 静态模式
    case IS_STATIC:
        ls_static_effect();
        break;

    case IS_IN_MODE_PHONE_MIC: // 手机麦克风模式
    {
        WS2812FX_setSegment_colorOptions( // 设置一段颜色的效果
            0,                            // 第0段
            0,                            // 起始位置
            0,                            // 结束位置
            &colorful_lights_static_max_brightness,
            0,                                              // 颜色，WS2812FX_setColors设置
            100,                                            // 速度
            0);                                             // 选项，这里像素点大小：1
        WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色
        WS2812FX_resetSegmentRuntime(0);                    // 清除指定段的显示缓存
        WS2812FX_running_flag_set();                        // 置位运行标志
    }
    break;
    default:
        break;
    }
}
