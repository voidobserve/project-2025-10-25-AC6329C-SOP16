
#include "system/includes.h"
#include "syscfg_id.h"
#include "save_flash.h"
#define     CFG_USER_LED_LEDGTH_DATA    3

const u8 frist_mode[]={0x3D,0x00,0x00,0x0B,0x00,0x01,0x00,0x14,0x00,0x00,0x01,0x03,0xE8,0x03,0xE8}; //第一次上电默认模式




/*******************************************************************************************************
**函数名：上电读取FLASH里保存的指令数据
**输  出：
**输  入：读取CFG_USER_COMMAND_BUF_DATA里保存的最后一条接收到的指令，
**描  述：读取CFG_USER_LED_LEDGTH_DATA里保存的第一次上电标志，灯带长度，顺序是：：第1字节：第一次上电标志位，第2、3字节：灯带长度
**说  明：
**版  本：
**修改日期：
*******************************************************************************************************/
void read_flash_device_status_init(void)
{

    #if 1
    u8 res;
    save_flash_t save_flash3;

    memset((u8*)&save_flash3,0,sizeof(save_flash_t));


    res = syscfg_read(CFG_USER_LED_LEDGTH_DATA,(u8 *)(&save_flash3), sizeof(save_flash_t));


    os_time_dly(1);


    if(save_flash3.header != 0x55)  //第一次上电
    {
       
        fc_data_init();
    }
    else
    {
        memcpy( (u8*)(&fc_effect), (u8*)(&save_flash3.fc_save) , sizeof(fc_effect_t));
    }


#endif
}


// 把用户数据写到区域3
void save_user_data_area3(void)
{
    save_flash_t save_data;
    save_data.header = 0x55;

    memcpy((u8*)(&save_data.fc_save) , (u8*)(&fc_effect), sizeof(fc_effect_t));
    syscfg_write(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_data), sizeof(save_flash_t));

    // printf("__FUNC__ %s __LINE__ %u\n", __func__, __LINE__);
}


void read_user_data_area3(void)
{

}
















