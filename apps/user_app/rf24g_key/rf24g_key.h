#ifndef __RF24G_KEY_H
#define __RF24G_KEY_H

#include "includes.h"

#if (TCFG_RF24GKEY_ENABLE)


typedef struct
{
    u8 header1;      //
    u8 header2;      //
    u8 red_val;      // 红色值
    u8 green_val;    // 绿色值
    u8 blue_val;     // 蓝色值
    u8 cw;           //
    u8 dynamic_code; // 动态码 包号
    u8 type_1;       // 固定不变
    u8 type_2;       // 固定不变
    u8 type_3;       // 固定不变
    u8 key_1;        // 按键值
    u8 key_2;        // 按键值
    u8 crc;
} rf24g_recv_info_t; // 指令数据

// 定义按键键值：（不是真是的按键键值）
enum
{
    RF24G_KEY_ON_OFF = 0x17, // 开关机

    RF24G_KEY_CHROMATIC_CIRCLE = 0x51, // 色环

    RF24G_KEY_BRIGHTNESS_ADD = 0x03, // 亮度加
    RF24G_KEY_BRIGHTNESS_SUB = 0x06, // 亮度减

    RF24G_KEY_DYNAMIC_SPEED_ADD = 0x04, // 动态速度加
    RF24G_KEY_DYNAMIC_SPEED_SUB = 0x07, // 动态速度减

    RF24G_KEY_MODE_ADD = 0x05, // 模式加
    RF24G_KEY_MODE_SUB = 0x08, // 模式减

    RF24G_KEY_COLD_WHITE_BRIGHTNESS_ADD = 0x0A, // 冷白亮度加，同时暖白亮度减
    RF24G_KEY_COLD_WHITE_BRIGHTNESS_SUB = 0x0D, // 冷白亮度减，同时暖白亮度加

    RF24G_KEY_MOTOR_SPEED_ADD = 0x0B, // 电机速度加
    RF24G_KEY_MOTOR_SPEED_SUB = 0x0E, // 电机速度减

    RF24G_KEY_METEOR_SPEED_ADD = 0x0C, // 流星速度加
    RF24G_KEY_METEOR_SPEED_SUB = 0x0F, // 流星速度减

    RF24G_KEY_NONE, // 无按键
};

// 定义按键事件
enum
{
    RF24G_KEY_EVENT_NONE = 0x00,

    RF24G_KEY_EVENT_ON_OFF_CLICK,
    RF24G_KEY_EVENT_ON_OFF_HOLD,
    RF24G_KEY_EVENT_ON_OFF_LOOSE,

    RF24G_KEY_EVENT_CHROMATIC_CIRCLE_CLICK, // 色环按键短按
    RF24G_KEY_EVENT_CHROMATIC_CIRCLE_HOLD,
    RF24G_KEY_EVENT_CHROMATIC_CIRCLE_LOOSE,

    RF24G_KEY_EVENT_BRIGHTNESS_ADD_CLICK, // 亮度加 按键
    RF24G_KEY_EVENT_BRIGHTNESS_ADD_HOLD,
    RF24G_KEY_EVENT_BRIGHTNESS_ADD_LOOSE,

    RF24G_KEY_EVENT_BRIGHTNESS_SUB_CLICK, // 亮度减 按键
    RF24G_KEY_EVENT_BRIGHTNESS_SUB_HOLD,
    RF24G_KEY_EVENT_BRIGHTNESS_SUB_LOOSE,

    RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_CLICK, // 动态速度加 按键  
    RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_HOLD,
    RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_LOOSE,

    RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_CLICK, // 动态速度减 按键
    RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_HOLD,
    RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_LOOSE,

    RF24G_KEY_EVENT_MODE_ADD_CLICK, // 模式加 按键
    RF24G_KEY_EVENT_MODE_ADD_HOLD,
    RF24G_KEY_EVENT_MODE_ADD_LOOSE,

    RF24G_KEY_EVENT_MODE_SUB_CLICK, // 模式减 按键
    RF24G_KEY_EVENT_MODE_SUB_HOLD,
    RF24G_KEY_EVENT_MODE_SUB_LOOSE,

    RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_ADD_CLICK, // 冷白亮度加 同时暖白亮度减 按键
    RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_ADD_HOLD,
    RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_ADD_LOOSE,

    RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_SUB_CLICK, // 冷白亮度减 同时暖白亮度加 按键
    RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_SUB_HOLD,
    RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_SUB_LOOSE,
 
    RF24G_KEY_EVENT_MOTOR_SPEED_ADD_CLICK, // 电机速度加 按键
    RF24G_KEY_EVENT_MOTOR_SPEED_ADD_HOLD,
    RF24G_KEY_EVENT_MOTOR_SPEED_ADD_LOOSE,

    RF24G_KEY_EVENT_MOTOR_SPEED_SUB_CLICK, // 电机速度减 按键
    RF24G_KEY_EVENT_MOTOR_SPEED_SUB_HOLD,
    RF24G_KEY_EVENT_MOTOR_SPEED_SUB_LOOSE,

    RF24G_KEY_EVENT_METEOR_SPEED_ADD_CLICK, // 流星速度加 按键
    RF24G_KEY_EVENT_METEOR_SPEED_ADD_HOLD,
    RF24G_KEY_EVENT_METEOR_SPEED_ADD_LOOSE,

    RF24G_KEY_EVENT_METEOR_SPEED_SUB_CLICK, // 流星速度减 按键
    RF24G_KEY_EVENT_METEOR_SPEED_SUB_HOLD,
    RF24G_KEY_EVENT_METEOR_SPEED_SUB_LOOSE, 
};

#define RF34G_KEY_EVENT_MAX (3) // 按键事件种类个数， 短按、持续hold、松开

#define RF24G_KEY_SCAN_TIME_MS (10) // 2.4G遥控器按键扫描频率，单位：ms
// #define RF24G_KEY_SCAN_TIME_MS (1) // 2.4G遥控器按键扫描频率，单位：ms
#define RF24G_KEY_LONG_TIME_MS (1500) // 长按事件触发时间，要长一些，防止频繁短按而进入了长按判断中
#define RF24G_KEY_HOLD_TIME_MS (500)
#define RF24G_KEY_SCAN_CLICK_DELAY_TIME_MS (0) // 2.4G遥控器按键被抬起后等待连击延时数量
#define RF24G_KEY_SCAN_FILTER_TIME_MS (0)      // 2.4G遥控器按键消抖延时

#define RF24G_HEADER_1 0x55
#define RF24G_HEADER_2 0xAA

extern volatile struct key_driver_para rf24g_scan_para;

extern void rf24g_scan(u8 *recv_buff);
extern void rf24_key_handle(void);

#endif // #if (TCFG_RF24GKEY_ENABLE)

#endif // end of file