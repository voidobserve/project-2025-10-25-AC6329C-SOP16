#ifndef led_strand_effect_h
#define led_strand_effect_h
#include "cpu.h"
#include "led_strip_sys.h"
#include "WS2812FX.H"
#include "led_strip_drive.h"
#include "one_wire.h"

#define MAX_SMEAR_LED_NUM 48 // 最多48个灯/48段
#define ALARM_NUMBER 3       // 闹钟个数
#define MAX_SOUND 10         // 声音ADC采集个数

// 当前模式枚举
typedef enum
{
    ACT_TY_PAIR,     // 配对效果
    ACT_CUSTOM,      // 自定义效果
    IS_STATIC,       // 静态模式
    IS_light_music,  // 音乐律动
    IS_light_scene,  // 炫彩情景
    IS_smear_adjust, // 涂抹功能

    COLORFUL_LIGHTS_STATIC,

} Now_state_e;

// 涂抹工具
typedef enum
{
    IS_drum = 1,  // 油桶
    IS_pen = 2,   // 画笔
    IS_eraser = 3 // 橡皮檫
} smear_tool_e;

// 方向
typedef enum
{
    IS_forward,
    IS_back = 16,
} direction_e;

// 变化方式
typedef enum
{
    MODE_MUTIL_RAINBOW = 2,                                      // 彩虹(多段颜色)
    MODE_MUTIL_JUMP = 10,                                        // 跳变模式(多段颜色)
    MODE_MUTIL_BRAETH = 11,                                      // 呼吸模式(多段颜色)
    MODE_MUTIL_TWIHKLE = 12,                                     // 闪烁模式(多段颜色)
    MODE_MUTIL_FLOW_WATER = 13,                                  // 流水模式(多段颜色)
    MODE_CHAS_LIGHT = 14,                                        // 追光模式
    MODE_MUTIL_COLORFUL = 15,                                    // 炫彩模式(多段颜色)
    MODE_MUTIL_SEG_GRADUAL = 16,                                 // 渐变模式(多段颜色)
    MODE_JUMP,                                                   // 标准跳变
    MODE_STROBE,                                                 // 频闪，颜色之间插入黑mode
    MODE_MUTIL_C_GRADUAL,                                        // 多种颜色切换整条渐变
    MODE_2_C_FIX_FLOW,                                           // 两种颜色混合流水，渐变色流水
    MODE_SINGLE_FLASH_RANDOM = 21,                               /// 星空效果，单灯随机闪烁
    MODE_SEG_FLASH_RANDOM = 22,                                  // 星云效果，一段随机闪烁
    MODE_SINGLE_METEOR = 23,                                     // 流星效果
    MODE_SINGLE_C_BREATH = 24,                                   // 单色呼吸
    MODE_B_G_METEOR = 25,                                        // 带背景色流星
    MODE_OPEN = 26,                                              // 开幕式
    MODE_CLOSE = 27,                                             // 闭幕式
    MODE_DOT_RUNNING = 28,                                       // 多个点跑马 ，点和点直接固定间隔5，支持每个点不同颜色，支持设置背景色
    MODE_DOT_RUNNING_COLLECTIONS = 29,                           // 跑马集合模式
    MODE_SINGLE_SUPERPOSITION = 30,                              // 单色堆积
    MODE_B_G_SUPERPOSITION = 31,                                 //带底色堆积
                                 MODE_MUTILE_SUPERPOSITION = 32, // 多色堆积，不灭
    MODE_BREATH_W = 33,                                          // W通道呼吸
    MODE_GRADUAL = 34,                                           // 标准渐变，彩虹颜色
    MODE_MUTIL_C_BREATH = 35,

} change_type_e;

#pragma pack(1)
/*----------------------------涂抹功能结构体----------------------------------*/
typedef struct
{
    smear_tool_e smear_tool;
    color_t rgb[MAX_SMEAR_LED_NUM];
} smear_adjust_t;

/*----------------------------静态模式结构体----------------------------------*/

/*----------------------------幻彩情景结构体----------------------------------*/
typedef struct
{

    change_type_e change_type; // 变化类型、模式
    direction_e direction;     // 效果的方向
    unsigned char seg_size;    // 段大小
    unsigned char c_n;         // 颜色数量
    color_t rgb[MAX_NUM_COLORS]; // 颜色池
    unsigned short speed; // 由档位决定

} dream_scene_t;

/*----------------------------倒计时结构体----------------------------------*/
typedef struct
{
    unsigned char set_on_off;
    unsigned long time;
} countdown_t;

typedef struct
{

    u8 hour;
    u8 minute;
    u8 second;
    u8 week;

} TIME_CLOCK;

typedef struct
{
    u8 week;
    u8 hour;
    u8 minute;
    u8 on_off;
    u8 mode;

} ALARM_CLOCK;

typedef struct
{
    unsigned char m;      // 效果模式
    unsigned char s;      // 灵敏度
    unsigned char m_type; // 区分音乐的模式，手机麦或者外麦
} music_t;

/*----------------------------幻彩灯串效果大结构体----------------------------------*/
typedef struct
{
    unsigned char on_off_flag; // 开关状态
    unsigned char led_num;     // 灯点数
    unsigned char sequence;    // RGB通道顺序
    unsigned char b;           // 本地亮度
    unsigned char app_b;       // 反馈给APP亮度
    unsigned char app_speed;   // 反馈给APP速度
    unsigned char ls_b;        // 遥控调亮度
    unsigned char ls_speed;    // 遥控调速度

    color_t rgb; // 静态模式颜色

    unsigned char meteor_period; // 周期值，单位秒
    unsigned char mode_cycle;    // 1:模式完成一个循环。0：正在跑，和meteor_period搭配用
    u16 period_cnt;              // ms,运行时的计数器
    Now_state_e Now_state;       // 当前运行模式
    // smear_adjust_t smear_adjust; // 涂抹功能
    dream_scene_t dream_scene;   // 幻彩情景

    music_t music; // 音乐效果

    unsigned char auto_f;
    base_ins_t base_ins; // 电机
    unsigned char motor_on_off;
    unsigned char star_on_off; // 流星开关
    unsigned char star_index;
    unsigned short star_speed;      // 流星灯动画的速度值
    unsigned char app_star_speed;   // 反馈给app的，流星灯动画的速度值
    
    unsigned char motor_speed_index; // 电机模式或电机速度索引

    unsigned char app_rgb_mode; // 七彩灯的模式索引，一般由app设置，目前加入了遥控器切换

} fc_effect_t;

// countdown_t zd_countdown[ALARM_NUMBER];

#pragma pack()

extern volatile fc_effect_t fc_effect; // 幻彩灯串效果数据

void base_Dynamic_Effect(u8 tp_num);

void ls_set_color(uint8_t n, uint32_t c); // 设置fc_effect.dream_scene.rgb的颜色池
void ls_set_colors(uint8_t n, color_t *c); // 设置段的颜色


#endif
