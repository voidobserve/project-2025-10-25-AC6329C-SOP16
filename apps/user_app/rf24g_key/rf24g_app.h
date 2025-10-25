#ifndef _RF24G_APP_H_
#define _RF24G_APP_H_

#include "board_ac632n_demo_cfg.h"

#if TCFG_RF24GKEY_ENABLE

#pragma pack (1)
//遥控配�??
typedef struct
{
    u8 pair[3];
    u8 flag;    //0:表示该数组没使用�?0xAA：表示改数组已配对使�?
}rf24g_pair_t;

//2.4G遥控 不同的遥控，该结构体不一�?
typedef struct
{
    u8 header1;  //
    u8 header2; //
    u8 sum_r;  //
    u8 sum_g; //
    u8 sum_b;  //
    u8 sum_cw;  //
    u8 dynamic_code;    //  动态码 包号
    u8 type;           //不变的，
    u16 signatures;   //不变的厂�?
    u8 key_v;
    u8 remoter_id;
    u8 k_crc;
}rf24g_ins_t;   //指令数据


struct RF24G_PARA{


    u8 rf24g_rx_flag ;
    u8 last_dynamic_code;   
    u8 last_key_v;
    u8 rf24g_key_state;   
    u8 clink_delay_up;
    u8 long_press_cnt;
    u16 hold_pess_cnt;
    const u16 is_long_time;
    const u8 is_click;
    const u8 is_long;
    const u8 _sacn_t;
};

#pragma pack ()



#define RF24_SPEED_BRIGHT_SUB       0x02	
#define RF24_SPEED_BRIGHT_PLUS      0x03	
#define RF24_ON_OFF                 0x08	
#define RF24_RED                    0x04
#define RF24_GREEN                  0x05	
#define RF24_BLUE                   0x14	
#define RF24_YELLOW                 0x06	
#define RF24_AZURE                  0x01	
#define RF24_ROSE_RED               0x0E
#define RF24_WHITE                   0x15	
#define RF24_WARM_WHITE              0x0F	
#define RF24_ALL_MODE                0x09	
#define RF24_SEVEN_COLOR_GRADUAL     0x16	
#define RF24_SEVEN_COLOR_BREATHE     0x10	
#define RF24_SEVEN_COLOR_JUMP        0x0A	
#define RF24_STEMPMOTOR_SPEED        0x17	
#define RF24_SOUND_ONE               0x11  //七彩？流星？声控
#define RF24_SOUND_TWO               0x0B	 //七彩？流星？声控
#define RF24_ONE_TOW_METEOR          0x18	
#define RF24_METEOR_SOUND_ONE_TWO    0x12
#define RF24_DIRECTION               0x0C
#define RF24_METEOR_SPEED            0x13
#define RF24_METEOR_FREQUENCY        0x0D	
#define RF24_METEOR_TAIL             0x07	



#define RFKEY_AUTO 0x3B
#define RFKEY_PAUSE 0x3A
#define RFKEY_ON_OFF 0x40
#define RFKEY_LIGHT_PLUS 0x3C
#define RFKEY_LIGHT_SUB 0x3E
#define RFKEY_SPEED_PLUS 0x3D
#define RFKEY_SPEED_SUB 0x39
#define RFKEY_R 0x4D
#define RFKEY_G 0x47
#define RFKEY_B 0x41
#define RFKEY_YELLOW 0x4E
#define RFKEY_CYAN 0x48
#define RFKEY_PURPLE 0x42
#define RFKEY_W 0x49
#define RFKEY_MODE_ADD 0x4C
#define RFKEY_MODE_DEC 0x46
#define RFKEY_7COL_JUMP 0x3F
#define RFKEY_W_BREATH 0x4B
#define RFKEY_7COL_GRADUAL 0x45
#define RFKEY_MUSIC1 0x50
#define RFKEY_MUSIC2 0x4A
#define RFKEY_MUSIC3 0x44
#define RFKEY_2H 0x43
#define RFKEY_1H 0x4F


#define TOUCHU_KEY_1 0x01
#define TOUCHU_KEY_2 0x02
#define TOUCHU_KEY_3 0x03
#define TOUCHU_KEY_4 0x04
#define TOUCHU_KEY_5 0x05
#define TOUCHU_KEY_6 0x06
#define TOUCHU_KEY_7 0x07
#define TOUCHU_KEY_8 0x08
#define TOUCHU_KEY_9 0x09
#define TOUCHU_KEY_a 0x0a
#define TOUCHU_KEY_b 0x0b




// extern rf24g_pair_t rf24g_pair[];        //需要写flash
// extern rf24g_ins_t rf24g_ins;


// void RF24G_Key_Handle(void);


#endif
#endif

