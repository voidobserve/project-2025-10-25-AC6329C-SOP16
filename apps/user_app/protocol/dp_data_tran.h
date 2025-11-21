#ifndef dp_data_tran_h
#define dp_data_tran_h


#define         DP_TYPE_RAW                     0x00				//RAW
#define         DP_TYPE_BOOL                    0x01	            //Bool
#define         DP_TYPE_VALUE                   0x02	            //Value
#define         DP_TYPE_STRING                  0x03				//String
#define         DP_TYPE_ENUM                    0x04				//Enum
#define         DP_TYPE_BITMAP                  0x05				//Bitmap

/******************************************************************************
						1:dp data point serial number redefinition
		  * * this is automatically generated code. If there are any changes on the development platform, please download MCU_SDK again.**
******************************************************************************/
//开关(可下发可上报)
//备注:
#define DPID_SWITCH_LED 20

//工作模式(可下发可上报)
//备注:
#define DPID_WORK_MODE 21

//白光亮度(可下发可上报)    
//备注:
#define DPID_BRIGHT_VALUE 22

//彩光(可下发可上报)
//备注:用于调节彩光颜色、亮度、饱和度
#define DPID_COLOUR_DATA 24

//倒计时(可下发可上报)
//备注:
#define DPID_COUNTDOWN 26

//音乐灯(只下发)
//备注:通过APP内置律动算法实现灯光律动
#define DPID_MUSIC_DATA 27

//炫彩情景(可下发可上报)
//备注:用于切换、编辑炫彩情景
#define DPID_RGBIC_LINERLIGHT_SCENE 56

//led点数设置(可下发可上报)
//备注:代表灯串led数量，用于切分灯串长度、段数
#define DPID_LED_NUMBER_SET 58

//涂抹功能(可下发可上报)
//备注:用于在面板上直接操作需要控制的led
#define DPID_DRAW_TOOL 59

/* -------------------------------------涂鸦DP数据头-------------------------------------- */
typedef struct
{
    unsigned char id;
    uint8_t type;
    uint16_t len;
    /* uint8_t *data; */    //不包含数据
} dp_data_header_t;  //涂鸦DP数据头

/* -------------------------------------DPID_SWITCH_LED 开关-------------------------------------- */
typedef struct
{
    unsigned char on_off;
} dp_switch_led_t;   //DPID_SWITCH_LED开关

/* -------------------------------------DPID_WORK_MODE 工作模式-------------------------------------- */
typedef struct
{
    unsigned char mode;
} dp_work_mode_t;  //DPID_WORK_MODE工作模式

/* -------------------------------------DPID_COUNTDOWN 倒计时-------------------------------------- */
typedef struct
{
    unsigned long time;
} dp_countdown_t;  //DPID_COUNTDOWN倒计时

/* -------------------------------------DPID_MUSIC_DATA 音乐灯-------------------------------------- */
typedef struct
{
    unsigned char change_type;  //变化方式
    hsv_t colour;    //HSV颜色
    unsigned short white_b;     //白光亮度
    unsigned short ct;          //色温
} dp_music_data_t;   //DPID_MUSIC_DATA音乐灯

/* -------------------------------------DPID_RGBIC_LINERLIGHT_SCENE 炫彩情景----------------------------- */
// typedef struct
// {
//     unsigned char change_type;  //变化方式
//     // unsigned char speed;        //速度
//     unsigned char direction;    //方向
//     unsigned char seg_size;     //段大小
//     struct
//     {
//         hsv_t colour;              //HSV颜色
//         unsigned short white_b;    //白光亮度
//     } HSV_whiteb[8];  //最大8个颜色点
// } dp_secene_data_t;  //DPID_RGBIC_LINERLIGHT_SCENE炫彩情景

/* -------------------------------------DPID_LED_NUMBER_SET led点数设置----------------------------- */
typedef struct
{
    unsigned long lednum;     //led点数
} dp_lednum_set_t;  //DPID_LED_NUMBER_SET led点数设置

/* -------------------------------------DPID_DRAW_TOOL 涂抹功能----------------------------- */
typedef struct
{
    unsigned char tool;         //油桶、画笔、橡皮擦工具
    hsv_t colour;                //HSV颜色
    unsigned short white_b;     //白光亮度
    unsigned short led_place;   //灯串位置
} dp_draw_tool_t;  //DPID_DRAW_TOOL 涂抹功能


extern dp_data_header_t  dp_data_header;  //涂鸦DP数据头
extern dp_switch_led_t   dp_switch_led;   //DPID_SWITCH_LED开关
extern dp_work_mode_t    dp_work_mode;    //DPID_WORK_MODE工作模式
extern dp_countdown_t    dp_countdown;    //DPID_COUNTDOWN倒计时
extern dp_music_data_t   dp_music_data;   //DPID_MUSIC_DATA音乐灯
// extern dp_secene_data_t  dp_secene_data;  //DPID_RGBIC_LINERLIGHT_SCENE炫彩情景
extern dp_lednum_set_t   dp_lednum_set;   //DPID_LED_NUMBER_SET led点数设置
extern dp_draw_tool_t    dp_draw_tool;    //DPID_DRAW_TOOL 涂抹功能

/* 解析中道数据，主要是静态模式，和动态效果的“基本”效果 */
void parse_zd_data(unsigned char *LedCommand);
// int string2hex(char* str,char* hex,uint8_t str_len);
unsigned long string_hex_Byte(char* str, unsigned char Byte_num);
unsigned short dp_extract_data_handle(unsigned char *buff);
extern void printf_buf(u8 *buf, u32 len);


void fb_motor_speed(void); // 向app反馈电机速度值
void fd_meteor_speed(void); // 向app反馈流星灯速度值

#endif


