#ifndef _ONE_WIRE_H
#define _ONE_WIRE_H

#include "system/includes.h"


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
    u8 mode;           //电机模式 
    u8 period;          //000:  8S; 001:  13S; 010:  18S ;011:  21S ;100:  26S  //转速  
    u8 dir;            //1:反转 0:正转  仅音乐律动模式有效
    u8 music_mode;     //音乐律动下的转动模式
    u8 sensitivity; // 声控模式下，电机的灵敏度
}base_ins_t;




extern volatile u16 send_base_ins;
extern u8 motor_period[6];

u8 is_one_wire_send_end(void);
void one_wire_send_enable(void);
void enable_one_wire(void);

void one_wire_set_mode(u8 m);

// 设置电机周期（设置电机转速）
void one_wire_set_period(u8 p);

#endif



