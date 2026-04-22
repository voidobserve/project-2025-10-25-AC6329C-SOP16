#ifndef __ONE_WIRE_H__
#define __ONE_WIRE_H__

#include "system/includes.h"

// 定义与驱动电机ic通信的引脚
#define MOTOR_DATA_IO_PORT IO_PORTB_06

// 定义电机模式
enum
{
    MOTOR_MODE_STOP = 0,
    MOTOR_MODE_FORWARD, // 正转
    MOTOR_MODE_REVERSE, // 反转
    // 正反转（先正转、然后反转，再正转，以此作为循环）：
    MOTOR_MODE_FORWARD_REVERSE,
    // 音乐律动
    MOTOR_MODE_MUSIC_RULATION = 0x05,
};
typedef u8 motor_mode_t;

// // 定义电机的速度值索引
// enum
// {
//     MOTOR_SPEED_8 = 0,
// };
// typedef u8 motor_speed_t;

typedef struct
{
    // 000:回正
    // 001:区域1摇摆
    // 010:区域2摇摆
    // 011:区域1和区域2摇摆
    // 100:360°正转
    // 101:音乐律动

    /*
        实际测得的模式
        0 -- 停止
        1 -- 反转，之后是摇摆，正转3s，反转3s
        2 -- 正转(约2min)，之后是摇摆，正转约2~3s，反转约2~3s
        3 -- 正转，之后是摇摆，正转5s，反转5s
        4 -- 正转
        5 -- 由主控控制的声控模式
    */
    u8 mode;   // 电机模式
    u8 period; // 000:  8S; 001:  13S; 010:  18S ;011:  21S ;100:  26S  //转速
    // u8 dir;         // 1:反转 0:正转  仅音乐律动模式有效
    // u8 music_mode;  // 音乐律动下的转动模式
    u8 sensitivity; // 声控模式下，电机的灵敏度

    /*
        记录关机前的电机模式，关机后再开机，需要恢复成该模式
        last_mode 不存放 MOTOR_MODE_STOP 停止模式
    */
    u8 last_mode;
    u8 dir_in_mode_forward_reverse; // 正反转模式下，电机当前的旋转方向。0：正转，1：反转
} base_ins_t;

extern u8 motor_period[6];

u8 is_one_wire_send_end(void);
void one_wire_send_enable(void);
// void enable_one_wire(void);

void one_wire_set_mode(motor_mode_t mode);

// 设置电机周期（设置电机转速）
void one_wire_set_period(u8 p);

void motor_package_data(motor_mode_t mode, u8 speed_val);
void motor_send_data(void);

void motor_forward_reverse_mode_handle(void);
void motor_music_rulation_mode_handle(void);

#endif
