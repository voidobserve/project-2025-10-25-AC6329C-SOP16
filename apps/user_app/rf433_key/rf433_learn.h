#ifndef __RF433_LEARN_H__
#define __RF433_LEARN_H__

#include "includes.h"

#define RF_433_LEARN_ENABLE 0

#if RF_433_LEARN_ENABLE

// 定义 rf 433 学习/对码 的状态
enum
{
    RF_433_LEARN_STATUS_NONE,       // 未开始学习/对码
    RF_433_LEARN_STATUS_PROCESSING, // 正在处理中
    RF_433_LEARN_STATUS_DONE,       // 学习/对码完成
};
typedef u8 RF_433_LEARN_STATUS_T;

/*
    上电之后，rf433按键学习/对码时间。
    单位：ms
*/
#define RF_433_KEY_LEARN_TIMES_ARTER_PWR_ON ((u16)3000)
#define RF_433_KEY_LEARN_FUNC_PERIOD (10) // 调用 rf_433_key_learn() 该函数的周期，单位：ms

extern volatile u32 recv_rf_433_addr; // 存放接收到的433信号的遥控器地址

void rf_433_addr_update(u32 addr); // 更新rf433遥控器的地址
u32 rf_433_addr_get(void);         // 获取rf433遥控器的地址

// RF_433_LEARN_STATUS_T rf_433_learn_status_get(void);
// void rf_433_learn_status_update(RF_433_LEARN_STATUS_T status);

void rf_433_key_learn(void); // rf433学习/对码

#endif // #if RF_433_LEARN_ENABLE

#endif