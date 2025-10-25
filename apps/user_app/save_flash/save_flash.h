
#ifndef save_flash_h
#define save_flash_h
#include "led_strand_effect.h"
#include "led_strip_drive.h"
typedef enum
{
    FRIST_BYTE,         //第一次上电标志
    LED_LEDGTH_MS,      //灯带长度高8位
    LED_LEDGTH_LS,      //灯带长度低8位
}FLASH_BYTE_FLAG;

#pragma pack (1)
typedef struct 
{
    unsigned char header;           //头部
    fc_effect_t fc_save;
}save_flash_t;


 


#pragma pack ()

void save_user_data_area3(void);

#endif