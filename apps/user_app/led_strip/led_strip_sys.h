#ifndef led_srtip_sys_h
#define led_srtip_sys_h

// 控灯系统属性
#include "cpu.h"
/******************************************************************系统cfg******************************************************************/
/*定义数据总线大端/小端，二选一*/
#define SYS_LITTLE_END 0
#define SYS_BIG_END 1
// 定义协议大小端
#define PROTOCOL_END SYS_BIG_END
#define SYS_BUS_END SYS_LITTLE_END

#if (PROTOCOL_END == SYS_BUS_END)
#define __SWP16(n) n
#define __SWP32(n) n
#endif
#if (PROTOCOL_END != SYS_BUS_END)
#define __SWP16(n) ((((u16)(n) & 0xff00) >> 8) | (((u16)(n) & 0x00FF) << 8))
#define __SWP32(n) (((u32)(n) & 0xff000000 >> 24) | \
                    ((u32)(n) & 0x00ff0000 >> 8) |  \
                    ((u32)(n) & 0x0000ff00 << 8) |  \
                    ((u32)(n) & 0x000000ff << 24))

#endif
/******************************************************************灯具配置 cfg******************************************************************/

#define TYPE_Fiber_optic_lights (1) // 光纤灯
#define TYPE_Magic_lights (2)       // 幻彩灯
#define LED_STRIP_TYPE TYPE_Fiber_optic_lights

/* 定义灯珠颜色，五选1*/
// #define LED_STRIP_R
// #define LED_STRIP_RG
// #define LED_STRIP_RGB
#define LED_STRIP_RGBW 1
// #define LED_STRIP_RGBCW

/* 定义灯珠通道,取值1~5*/
#define LED_STRIP_CH 3

/******************************************************************common*****************************************************************/

typedef struct _HSV_COLOUR_DAT
{
    u32 h_val;
    u32 s_val;
    u32 v_val;
} HSV_COLOUR_DAT;

typedef struct
{
    u16 h_val; // h的数值，色相 0~360
    u16 s_val; // s的数值，饱和度 0~360
    u16 v_val; // v的数值，亮度0~1000
} hsv_t;

typedef struct
{
#ifdef LED_STRIP_R
    u8 r;
#endif
#ifdef LED_STRIP_RG
    u8 r;
    u8 g;
#endif
#ifdef LED_STRIP_RGB
    u8 r;
    u8 g;
    u8 b;
#endif
#ifdef LED_STRIP_RGBW
    u8 r;
    u8 g;
    u8 b;
    u8 w;
#endif
#ifdef LED_STRIP_RGBCW
    u8 r;
    u8 g;
    u8 b;
    u8 c;
    u8 w;
#endif
} color_t;

typedef enum
{
    DEVICE_OFF, // 关机
    DEVICE_ON,  // 开机
} ON_OFF_FLAG;

typedef enum
{
    IS_AUTO,
    IS_PAUSE,
} _AUTO_T;

typedef enum
{
    PHONE_MIC,    // 手机麦克风
    EXTERIOR_MIC, // 外部麦克风
} MIC_TYPE_T;

typedef enum
{
    IR_TIMER_NO = 0, // 无定时
    IR_TIMER_30MIN = 30 * 60 * 1000,
    IR_TIMER_60MIN = 60 * 60 * 1000,
    IR_TIMER_90MIN = 90 * 60 * 1000,
    IR_TIMER_120MIN = 120 * 60 * 1000,
} AUTO_TIME_T;

void set_static_mode(u8 r, u8 g, u8 b);

void ls_add_speed(void); // 遥控器控制 -- 添加速度
void ls_sub_speed(void); // 遥控器控制 -- 减去速度

void ls_add_bright(void); // 遥控器控制 -- 亮度加
void ls_sub_bright(void); // 遥控器控制 -- 亮度减

void ls_add_sensitive(void); // 遥控器控制 -- 灵敏度加
void ls_sub_sensitive(void); // 遥控器控制 -- 灵敏度减

void ls_add_mode_InAPP(void); // 遥控器控制 -- 模式切换 顺序
void ls_sub_mode_InAPP(void); // 遥控器控制 -- 模式切换 逆序

void ls_add_motor_speed(void); // 遥控器控制 -- 电机速度 变快
void ls_sub_motor_speed(void); // 遥控器控制 -- 电机速度 变慢

void ls_add_star_speed(void);             // 遥控器控制 -- 流星灯速度 变快
void ls_sub_star_speed(void);             // 遥控器控制 -- 流星灯速度 变慢
void meteor_set_mode_can_be_cycled(void); // 遥控器设置流星灯模式（可以循环）

void soft_turn_on_the_light(void); // 软开灯处理
void soft_turn_off_lights(void);   // 软关灯处理

#endif
