
#ifndef __SAVE_FLASH_H__
#define __SAVE_FLASH_H__

#include "led_strand_effect.h"
#include "led_strip_drive.h"

#pragma pack(1)
typedef struct
{
    unsigned char header; // 头部
    fc_effect_t fc_save;

    u32 rf_433_addr; // 存放rf433遥控器的地址
} save_flash_t;

#pragma pack()

// 需要保存数据时，延时保存的时间：（单位：ms）
#define DELAY_SAVE_FLASH_TIMES ((u16)3000)

void read_flash_device_status_init(void);
void save_user_data_area3(void);

void save_user_data_enable(void);

void save_user_data_time_count_down(void);
void save_user_data_handle(void);

#endif