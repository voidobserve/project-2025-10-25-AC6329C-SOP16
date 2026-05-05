#include "cpu.h"
#include "led_strip_sys.h"
#include "paint_tool.h"
#include "dp_data_tran.h"
#include "led_strand_effect.h"
#include "ble_multi_profile.h"
#include "led_strand_effect.h"
#include "WS2812FX.H"
#include "btstack/le/att.h"
#include "ble_user.h"
#include "btstack/le/ble_api.h"
#include "led_strip_drive.h"
#include "one_wire.h"

#include "user_include.h"
#include "led_strand_effect.h"
#include "user_ble_notify.h"

dp_data_header_t dp_data_header; // 涂鸦DP数据头
dp_switch_led_t dp_switch_led;   // DPID_SWITCH_LED开关
dp_work_mode_t dp_work_mode;     // DPID_WORK_MODE工作模式
dp_countdown_t dp_countdown;     // DPID_COUNTDOWN倒计时
dp_music_data_t dp_music_data;   // DPID_MUSIC_DATA音乐灯
// dp_secene_data_t  dp_secene_data;  //DPID_RGBIC_LINERLIGHT_SCENE炫彩情景
dp_lednum_set_t dp_lednum_set; // DPID_LED_NUMBER_SET led点数设置
dp_draw_tool_t dp_draw_tool;   // DPID_DRAW_TOOL 涂抹功能

/************************************************************************************
 *@  函数：string_hex_Byte
 *@  描述：把字符串转为HEX，支持小写abcdef,输出uint8、uint16、uint32类型的hex
 *@  形参1: str         <字符输入>
 *@  形参2: out_Byte    <输出的1、2、4字节>
 *@  返回：unsigned long
 ************************************************************************************/
unsigned long string_hex_Byte(char *str, unsigned char out_Byte)
{
    int i = 0;
    unsigned char temp = 0;
    unsigned long hex = 0;
    if ((out_Byte == 1) || (out_Byte == 2) || (out_Byte == 4))
    {
        while (i < (out_Byte * 2))
        {
            hex <<= 8;
            if (str[i] >= '0' && str[i] <= '9')
            {
                temp = (str[i] & 0x0f);
            }
            else if (str[i] >= 'a' && str[i] <= 'f')
            {
                temp = ((str[i] + 0x09) & 0x0f);
            }
            ++i;
            if (out_Byte != 1)
            {
                temp <<= 4;
                if (str[i] >= '0' && str[i] <= '9')
                {
                    temp |= (str[i] & 0x0f);
                }
                else if (str[i] >= 'a' && str[i] <= 'f')
                {
                    temp |= ((str[i] + 0x09) & 0x0f);
                }
            }
            ++i;
            hex |= temp;
        }
    }
    else
        return 0xffffffff; // 错误类型
    return hex;
}

/*****************************************
 *@  函数：dp_extract_data_handle
 *@  描述：dp数据提取
 *@  形参: buff
 *@  返回：DP数据长度
 ****************************************/
unsigned short dp_extract_data_handle(unsigned char *buff)
{
    unsigned char num = 0;
    unsigned char max;
    unsigned char temp;
    /*提取涂鸦DP数据头*/
    dp_data_header.id = buff[0];
    dp_data_header.type = buff[1];
    dp_data_header.len = buff[2];
    dp_data_header.len <<= 8;
    dp_data_header.len |= buff[3];

    // m_hsv_to_rgb(&colour_R,&colour_G,&colour_B,colour_H,colour_S,colour_V);

    /*提取DP数据*/
    switch (dp_data_header.id)
    {
    case DPID_SWITCH_LED: // 开关(可下发可上报)
        printf("\r\n DPID_SWITCH_LED");

        fc_effect.on_off_flag = buff[4];
        if (fc_effect.on_off_flag == DEVICE_ON)
        {
            soft_turn_on_the_light();
        }
        else
        {
            soft_turn_off_lights();
        }
        break;

    case DPID_WORK_MODE: // 工作模式(可下发可上报)
        dp_work_mode.mode = buff[4];

        printf("dp_work_mode.mode = %d\r\n", dp_work_mode.mode);
        printf("\r\n");

        break;

    case DPID_BRIGHT_VALUE: // 白光亮度(可下发可上报)
        break;

    case DPID_COLOUR_DATA: // 彩光(可下发可上报)
        break;
#if 0
    case DPID_COUNTDOWN://倒计时(可下发可上报)
        //DP协议参数
        dp_countdown.time = buff[4];
        dp_countdown.time <<= 8;
        dp_countdown.time |= buff[5];
        dp_countdown.time <<= 8;
        dp_countdown.time |= buff[6];
        dp_countdown.time <<= 8;
        dp_countdown.time |= buff[7];

        printf("dp_countdown.time = %d\r\n",dp_countdown.time);
        printf("\r\n");

      //效果参数
        fc_effect.countdown.time = dp_countdown.time;

        printf("fc_effect.countdown.time = %d\r\n",fc_effect.countdown.time);
        printf("\r\n");

        break;
#endif
    case DPID_MUSIC_DATA: // 音乐律动(只下发)
                          // DP协议参数
        dp_music_data.change_type = string_hex_Byte(&buff[4], 1);
        dp_music_data.colour.h_val = string_hex_Byte(&buff[5], 2);
        dp_music_data.colour.s_val = string_hex_Byte(&buff[9], 2);
        dp_music_data.colour.v_val = string_hex_Byte(&buff[13], 2);
        dp_music_data.white_b = string_hex_Byte(&buff[17], 2);
        dp_music_data.ct = string_hex_Byte(&buff[21], 2);

        break;

    case DPID_RGBIC_LINERLIGHT_SCENE: // 炫彩情景(可下发可上报)
        fc_effect.Now_state = IS_light_scene;

        // 变化类型、模式
        fc_effect.dream_scene.change_type = buff[4];
        // 方向
        fc_effect.dream_scene.direction = buff[5];
        // 段大小
        fc_effect.dream_scene.seg_size = buff[6];
        if (fc_effect.dream_scene.c_n > MAX_NUM_COLORS)
        {
            fc_effect.dream_scene.c_n = MAX_NUM_COLORS;
        }
        // 颜色数量
        fc_effect.dream_scene.c_n = buff[7];
        // 清除rgb[0~n]数据
        memset(fc_effect.dream_scene.rgb, 0, sizeof(fc_effect.dream_scene.rgb));

        // 数据循环传输
        for (num = 0; num < fc_effect.dream_scene.c_n; num++)
        {
            fc_effect.dream_scene.rgb[num].r = buff[8 + num * 3];
            fc_effect.dream_scene.rgb[num].g = buff[9 + num * 3];
            fc_effect.dream_scene.rgb[num].b = buff[10 + num * 3];
        }
        printf("\n fc_effect.dream_scene.c_n=%d", fc_effect.dream_scene.c_n);
        printf_buf(fc_effect.dream_scene.rgb, fc_effect.dream_scene.c_n * sizeof(color_t));
        printf("\n fc_effect.dream_scene.direction=%d", fc_effect.dream_scene.direction);
        set_fc_effect();
        break;

    case DPID_LED_NUMBER_SET: // led点数设置(可下发可上报)
                              // DP协议参数
        dp_lednum_set.lednum = buff[4];
        dp_lednum_set.lednum <<= 8;
        dp_lednum_set.lednum |= buff[5];
        dp_lednum_set.lednum <<= 8;
        dp_lednum_set.lednum |= buff[6];
        dp_lednum_set.lednum <<= 8;
        dp_lednum_set.lednum |= buff[7];

        printf("dp_lednum_set.lednum = %d\r\n", dp_lednum_set.lednum);
        printf("\r\n");

        // 效果参数
        fc_effect.led_num = dp_lednum_set.lednum;

        if (fc_effect.led_num >= 48)
            fc_effect.led_num = 48;

        break;

    case DPID_DRAW_TOOL: // 涂抹功能(可下发可上报)
        // DP协议参数
        dp_draw_tool.tool = buff[5];

        dp_draw_tool.colour.h_val = buff[7];
        dp_draw_tool.colour.h_val <<= 8;
        dp_draw_tool.colour.h_val |= buff[8];
        dp_draw_tool.colour.s_val = buff[9] * 10;
        dp_draw_tool.colour.v_val = buff[10] * 10;

        dp_draw_tool.white_b = buff[11] * 10;

        if (buff[13] == 0x81)
        {
            dp_draw_tool.led_place = buff[14];
            dp_draw_tool.led_place <<= 8;
            dp_draw_tool.led_place |= buff[15];
        }

        // 效果参数
        if (dp_draw_tool.white_b != 0)
        {
            dp_draw_tool.colour.h_val = 0;
            dp_draw_tool.colour.s_val = 0;
            dp_draw_tool.colour.v_val = dp_draw_tool.white_b;
        }

        // effect_smear_adjust_updata((smear_tool_e )dp_draw_tool.tool, &(dp_draw_tool.colour), &dp_draw_tool.led_place);

        break;
    }
    return (dp_data_header.len + sizeof(dp_data_header_t));
}

// u8 Ble_Addr[6]; // 蓝牙地址
extern hci_con_handle_t ZD_HCI_handle;

extern ALARM_CLOCK alarm_clock[3];
extern TIME_CLOCK time_clock;

/**
 * @brief 向app反馈信息
 *
 * @param p   数据内容
 * @param len  数据长度
 */
void zd_fb_2_app(u8 *p, u8 len)
{
    uint8_t fc_buffer[30]; // 发送缓存
    // memcpy(fc_buffer, Ble_Addr, 6);
    memcpy(fc_buffer + 6, p, len); // 跳过前6个字节的地址
    // ble_comm_att_send_data(
    //     ZD_HCI_handle,
    //     ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE,
    //     fc_buffer,
    //     len + 6,
    //     ATT_OP_AUTO_READ_CCC);
    user_ble_notify_obj.param_put(fc_buffer, len + 6);
}

/*********************************************************
 *
 *      APP反馈 API
 *
 *********************************************************/
/**
 * @brief 向app反馈灯开关状态
 *
 */
void fb_led_on_off_state(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = fc_effect.on_off_flag; //

    zd_fb_2_app(tp_buffer, len);
}
/**
 * @brief 反馈音乐模式
 *
 */
// void fb_led_music_mode(void)
// {
//     uint8_t tp_buffer[6];
//     tp_buffer[0] = 0x06;
//     tp_buffer[1] = 0x06;
//     tp_buffer[2] = fc_effect.music.m; //
//     zd_fb_2_app(tp_buffer, 3);
// }

/**
 * @brief 反馈流星速度
 *
 */
void fd_meteor_speed(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x2F;
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = fc_effect.app_star_speed;
    zd_fb_2_app(tp_buffer, len);
}

/**
 * @brief 反馈流星周期
 *
 */
void fd_meteor_cycle(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x2F;
    tp_buffer[len++] = 0x03;
    tp_buffer[len++] = fc_effect.meteor_period;

    zd_fb_2_app(tp_buffer, len);
}

/**
 * @brief 反馈流星开关
 *
 */
void fd_meteor_on_off(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    u8 data = DEVICE_OFF;
    // 指令前缀 0x 01 E9 00
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x2F;
    tp_buffer[len++] = 0x02;

    // 目前在 app 中，1表示开启，2表示关闭
    if (DEVICE_ON == fc_effect.star_on_off)
    {
        data = 1;
    }
    else
    {
        data = 2;
    }

    tp_buffer[len++] = data;
    zd_fb_2_app(tp_buffer, len);
}

void fb_bright(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x04;
    tp_buffer[len++] = 0x03;
    tp_buffer[len++] = fc_effect.app_b;

    zd_fb_2_app(tp_buffer, len);
}

void fb_speed(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x04;
    tp_buffer[len++] = 0x04;
    tp_buffer[len++] = fc_effect.app_speed;

    zd_fb_2_app(tp_buffer, len);
}

void fb_sensitive(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x2F;
    tp_buffer[len++] = 0x05;
    tp_buffer[len++] = fc_effect.music.s;

    zd_fb_2_app(tp_buffer, len);
}

void fb_rgb_value(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x04;
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0x1e;
    tp_buffer[len++] = fc_effect.rgb.r;
    tp_buffer[len++] = fc_effect.rgb.g;
    tp_buffer[len++] = fc_effect.rgb.b;

    zd_fb_2_app(tp_buffer, len);
}

// void fb_RGBsequence(void)
// {
//     uint8_t tp_buffer[6];
//     tp_buffer[0] = 0x04;
//     tp_buffer[1] = 0x05;
//     tp_buffer[2] = fc_effect.sequence;
//     zd_fb_2_app(tp_buffer, 6);
// }

/**
 * @brief 反馈电机速度
 *
 */
void fb_motor_speed(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x2F;
    tp_buffer[len++] = 0x07;
    tp_buffer[len++] = fc_effect.base_ins.period;
    zd_fb_2_app(tp_buffer, len);
}

void fb_motor_mode(void)
{
    uint8_t tp_buffer[10];
    u8 len = 0;
    // 0x 01 E9 00，指令前缀
    tp_buffer[len++] = 0x01;
    tp_buffer[len++] = 0xE9;
    tp_buffer[len++] = 0x00;

    tp_buffer[len++] = 0x2F;
    tp_buffer[len++] = 0x06;
    tp_buffer[len++] = fc_effect.base_ins.mode;
    zd_fb_2_app(tp_buffer, len);
}

void parse_zd_data(unsigned char *LedCommand, u8 len)
{
    volatile u8 send_buf_len = 0; // 存放待发送的指令长度
    // const u8 send_addr_len = 6;        // 存放待发送的地址的长度
    const u8 send_data_prefix_len = 3; // 存放待发送的指令的前缀长度(0x01、0xE9、0x00共3bytes)
    volatile uint8_t Send_buffer[20];  // 发送缓存
    // memcpy(Send_buffer, Ble_Addr, 6);
    // send_buf_len += 6;

    if (len >= 5 &&
        LedCommand[0] == 0x01 &&
        LedCommand[1] == 0xE9 &&
        LedCommand[2] == 0x00 &&

        LedCommand[3] == 0x01 &&
        LedCommand[4] == 0x03) // 与APP同步数据
    {
        // 灯光
        //  -----------------设备信息------------------------------

        Send_buffer[send_buf_len++] = 0x01;
        Send_buffer[send_buf_len++] = 0xE9;
        Send_buffer[send_buf_len++] = 0x00;

        Send_buffer[send_buf_len++] = 0x07;
        Send_buffer[send_buf_len++] = 0x01;
        Send_buffer[send_buf_len++] = 0x01;
        Send_buffer[send_buf_len++] = 0x02; // 灯具类型：RGBW
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);
        // os_time_dly(1);
        //-------------------发送开关机状态---------------------------

        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x01;
        Send_buffer[send_buf_len++] = 0x01;
        Send_buffer[send_buf_len++] = get_on_off_state(); // 目前状态
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);
        // os_time_dly(1);

        //-------------------发送亮度---------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x04;
        Send_buffer[send_buf_len++] = 0x03;
        Send_buffer[send_buf_len++] = fc_effect.app_b;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);
        // os_time_dly(1);

        //-------------------发送速度---------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x04;
        Send_buffer[send_buf_len++] = 0x04;
        Send_buffer[send_buf_len++] = fc_effect.app_speed;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);
        // os_time_dly(1);

        //-------------------发送灯带长度---------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x04;
        Send_buffer[send_buf_len++] = 0x08;
        Send_buffer[send_buf_len++] = fc_effect.led_num >> 8;
        Send_buffer[send_buf_len++] = fc_effect.led_num & 0xff;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);
        // os_time_dly(1);

        //-------------------发送灵敏度---------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x2F;
        Send_buffer[send_buf_len++] = 0x05;
        Send_buffer[send_buf_len++] = fc_effect.music.s;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        //-------------------发送静态RGB模式--------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x04;
        Send_buffer[send_buf_len++] = 0x01;
        Send_buffer[send_buf_len++] = 0x1e;
        Send_buffer[send_buf_len++] = fc_effect.rgb.r;
        Send_buffer[send_buf_len++] = fc_effect.rgb.g;
        Send_buffer[send_buf_len++] = fc_effect.rgb.b;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        //-------------------发送RGB接口模式--------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x04;
        Send_buffer[send_buf_len++] = 0x05;
        Send_buffer[send_buf_len++] = fc_effect.sequence;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        //-------------------声控模式（手机麦或外麦--------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x06;
        Send_buffer[send_buf_len++] = 0x07;
        Send_buffer[send_buf_len++] = fc_effect.music.m_type;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        //-------------------本地麦克风模式--------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x06;
        Send_buffer[send_buf_len++] = 0x06;
        Send_buffer[send_buf_len++] = fc_effect.music.m;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        // 流星

        //-------------------流星开关--------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x2F;
        Send_buffer[send_buf_len++] = 0x02;
        Send_buffer[send_buf_len++] = fc_effect.star_on_off;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        //-------------------流星速度--------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x2F;
        Send_buffer[send_buf_len++] = 0x01;
        Send_buffer[send_buf_len++] = fc_effect.app_star_speed;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        //-------------------流星周期--------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x2F;
        Send_buffer[send_buf_len++] = 0x03;
        Send_buffer[send_buf_len++] = fc_effect.meteor_period;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        // 电机
        //-------------------电机模式--------------------------
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x2F;
        Send_buffer[send_buf_len++] = 0x06;
        Send_buffer[send_buf_len++] = fc_effect.base_ins.mode;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);

        //-------------------电机速度--------------------------
        // send_buf_len = send_addr_len + send_data_prefix_len;
        send_buf_len = send_data_prefix_len;
        Send_buffer[send_buf_len++] = 0x2F;
        Send_buffer[send_buf_len++] = 0x07;
        Send_buffer[send_buf_len++] = fc_effect.base_ins.period;
        // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
        // os_time_dly(1);
        user_ble_notify_obj.param_put(Send_buffer, send_buf_len);
    }
    else
    {
        //---------------------------------接收到开关灯命令-----------------------------------
        if (len >= 6 &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x01 &&
            LedCommand[4] == 0x01)
        {
            /*
                这里只控制七彩灯开关
                开启七彩灯，顺便打开电机
                关闭七彩灯，顺便关电机
            */
            if (LedCommand[5] == 0x01)
            {
                // 开灯
                colorful_light_open();
                motor_open();
            }
            else
            {
                // 关闭七彩灯，顺便关电机
                colorful_light_close();
                motor_close();
            }

            fb_led_on_off_state(); // 与app反馈七彩灯的开关状态
            fb_motor_mode();       // 向app反馈电机的模式
            fb_motor_speed();      // 向app反馈电机转速
        }

        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x2F &&
            LedCommand[4] == 0x02)
        {
            // ----------------------------------------------------------------
            // 流星灯开关
            app_set_on_off_meteor(LedCommand[5]);
            fd_meteor_on_off();
        }

        //---------------------------------更新RGB-----------------------------------
        if (len >= (3 + 6) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x04 &&
            LedCommand[4] == 0x01 &&
            LedCommand[5] == 0x1e)
        {
            // 根据app发送过来的rgb数据，设置七彩灯为静态模式，显示对应的颜色
            if (DEVICE_OFF == get_on_off_state())
            {
                return;
            }

            // phone_music_soure = 1;
            set_static_mode(LedCommand[6], LedCommand[7], LedCommand[8]);
            fb_rgb_value();
        }

        //---------------------------------静态（模式）任务处理-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x04 &&
            LedCommand[4] == 0x02 &&
            LedCommand[5] >= 0 && // 颜色索引
            LedCommand[5] < 0x07)
        {
            if (DEVICE_OFF == get_on_off_state())
            {
                return;
            }

            fc_effect.app_rgb_mode = LedCommand[5];
            switch (LedCommand[5])
            {
            case 0:
            {
                colorful_lights_set_static_color(RED);
            }
            break;
                // ================================
            case 1:
            {
                colorful_lights_set_static_color(GREEN);
            }
            break;
                // ================================
            case 2:
            {
                colorful_lights_set_static_color(BLUE);
            }
            break;
                // ================================
            case 3:
            {
                colorful_lights_set_static_color(CYAN);
            }
            break;
                // ================================
            case 4:
            {
                colorful_lights_set_static_color(YELLOW);
            }
            break;
                // ================================
            case 5:
            {
                colorful_lights_set_static_color(PURPLE);
            }
            break;
                // ================================
            case 6:
            {
                colorful_lights_set_static_color(PURE_WHITE);
            }
            break;
            }
        }

        //---------------------------------动态处理-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x04 &&
            LedCommand[4] == 0x02 &&
            LedCommand[5] >= 0x07 && // 动态模式索引
            LedCommand[5] <= 0x20)
        {
            // 设置七彩灯为动态模式，模式由app发送过来
            if (DEVICE_OFF == get_on_off_state())
            {
                return;
            }
            fc_effect.app_rgb_mode = LedCommand[5];
            base_Dynamic_Effect(LedCommand[5]);
        }

        //---------------------------------调节亮度0-100-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x04 &&
            LedCommand[4] == 0x03)
        {
            if (DEVICE_OFF == get_on_off_state())
            {
                return;
            }

            extern void app_set_bright(u8 tp_b);
            app_set_bright(LedCommand[5]);
            WS2812FX_resetSegmentRuntime(0); // 清空灯光动画运行时使用的数据，让动画重新开始跑
            set_fc_effect();                 // 设置完后，让七彩灯重新开始跑
            fb_bright();
        }

        //---------------------------------调节速度0-100-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x04 &&
            LedCommand[4] == 0x04)
        {
            // USER_TO_DO 调节速度时，如果设备已经关机，只修改速度值，不调用对应的动画
            if (DEVICE_OFF == get_on_off_state())
            {
#if USER_DEBUG_ENABLE
                // printf("device off\n");
                // printf("app set speed exit\n");
#endif
                return;
            }

            // 范围 0-100
            extern void app_set_speed(u8 tp_speed);
            app_set_speed(LedCommand[5]);    //
            WS2812FX_resetSegmentRuntime(0); // 清空灯光动画运行时使用的数据，让动画重新开始跑
            set_fc_effect();                 //
            fb_speed();
            // phone_music_soure = 1;
        }

        //---------------------------------更改RGB接口-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x04 &&
            LedCommand[4] == 0x05)
        {
            // extern void app_set_RGBsequence(u8 s);
            // app_set_RGBsequence(LedCommand[5]);
            // fb_RGBsequence();
            // phone_music_soure = 1;
        }

        //---------------------------------W（灰度调节）控制----------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x04 &&
            LedCommand[4] == 0x06)
        {
            if (DEVICE_OFF == get_on_off_state())
            {
                return;
            }

            extern void app_set_w(u8 tp_w);
            app_set_w(LedCommand[5]);
        }

        //---------------------------------手机音乐律动 手机麦克风-----------------------------------
        if (len >= (3 + 5) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x06 &&
            LedCommand[4] == 0x04)
        {
            if (DEVICE_OFF == get_on_off_state())
            {
                return;
            }

            if (fc_effect.music.m_type == PHONE_MIC) // 手机麦模式
            {
                // 改成使用最大亮度：
                fc_effect.Now_state = IS_IN_MODE_PHONE_MIC;
                fc_effect.rgb.r = LedCommand[5];
                fc_effect.rgb.g = LedCommand[6];
                fc_effect.rgb.b = LedCommand[7];
#if USER_DEBUG_ENABLE
// printf("r = %d, g = %d, b = %d", r, g, b);
#endif
                if (fc_effect.rgb.r == 0xFF &&
                    fc_effect.rgb.g == 0xFF &&
                    fc_effect.rgb.b == 0xFF)
                {

                    fc_effect.rgb.r = 0;
                    fc_effect.rgb.g = 0;
                    fc_effect.rgb.b = 0;
                    fc_effect.rgb.w = 255;
                }
                else
                {
                    fc_effect.rgb.w = 0;
                }
                set_fc_effect(); // 效果调度
            }
        }

        //---------------------------------外麦声控模式-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x06 &&
            LedCommand[4] == 0x06)
        {
            if (DEVICE_OFF == get_on_off_state())
            {
                return;
            }

            extern void app_set_music_mode(u8 tp_m);
            if (fc_effect.music.m_type != EXTERIOR_MIC)
            {
                // 如果不在外部麦克风模式，设置为外部麦克风模式（可能app界面跳转太快，单片机没有收到切换模式的信息）
                fc_effect.music.m_type = EXTERIOR_MIC;
            }

            app_set_music_mode(LedCommand[5]);

            // send_buf_len = send_addr_len;
            Send_buffer[send_buf_len++] = 0x01;
            Send_buffer[send_buf_len++] = 0xE9;
            Send_buffer[send_buf_len++] = 0x00;

            Send_buffer[send_buf_len++] = 0x06;
            Send_buffer[send_buf_len++] = 0x06;
            Send_buffer[send_buf_len++] = LedCommand[5];
            // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
            user_ble_notify_obj.param_put(Send_buffer, send_buf_len);
        }

        //---------------------------------设备手机麦或者外麦-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x06 &&
            LedCommand[4] == 0x07)
        {
            if (DEVICE_OFF == get_on_off_state())
            {
                return;
            }

            extern void set_music_type(u8 ty);
            set_music_type(LedCommand[5]);

            fc_effect.Now_state = IS_light_music;

            // printf("fc_effect.music.m = %u\n", fc_effect.music.m);
            if (fc_effect.music.m >= 4)
            {
                // 如果音乐模式的索引越界，则将索引置为
                fc_effect.music.m = 3;
            }
            set_fc_effect();

            // send_buf_len = send_addr_len;
            Send_buffer[send_buf_len++] = 0x01;
            Send_buffer[send_buf_len++] = 0xE9;
            Send_buffer[send_buf_len++] = 0x00;

            Send_buffer[send_buf_len++] = 0x06;
            Send_buffer[send_buf_len++] = 0x07;
            Send_buffer[send_buf_len++] = LedCommand[5];
            // ble_comm_att_send_data(ZD_HCI_handle, ATT_CHARACTERISTIC_fff1_01_VALUE_HANDLE, Send_buffer, send_buf_len, ATT_OP_AUTO_READ_CCC);
            user_ble_notify_obj.param_put(Send_buffer, send_buf_len);
        }

        //---------------------------------设置麦克风灵，电机，流星 灵敏度-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x2F &&
            LedCommand[4] == 0x05)
        {
            // if (DEVICE_OFF == get_on_off_state())
            // {
            //     return;
            // }

            app_set_sensitive(LedCommand[5]);
            fb_sensitive();
        }

        //--------------------------  流星相关 ----------------------------------------------
        // --------------------------------流星模式-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x2F &&
            LedCommand[4] == 0x00 &&
            fc_effect.star_on_off == DEVICE_ON)
        {
            if (DEVICE_OFF == fc_effect.star_on_off)
            {
                return;
            }

            extern void app_set_mereor_mode(u8 tp_m); // app设置流星灯模式
            app_set_mereor_mode(LedCommand[5]);       // LedCommand[5] 模式索引
        }
        //-------------------------------- 流星速度-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x2F &&
            LedCommand[4] == 0x01 &&
            fc_effect.star_on_off == DEVICE_ON)
        {
            if (DEVICE_OFF == fc_effect.star_on_off)
            {
                fd_meteor_speed();
                return;
            }

            app_set_mereor_speed(LedCommand[5]);
            ls_meteor_stat_effect(); // 设置完成后，重新跑流星灯动画
            fd_meteor_speed();
        }

        // --------------------------------流星灯时间间隔-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x2F &&
            LedCommand[4] == 0x03 &&
            fc_effect.star_on_off == DEVICE_ON)
        {
            if (DEVICE_OFF == fc_effect.star_on_off)
            {
                fd_meteor_cycle();
                return;
            }

            app_set_meteor_pro(LedCommand[5]);
            ls_meteor_stat_effect(); // 设置完成后，重新跑流星灯动画
            fd_meteor_cycle();
        }

        // ---------------------------------设置电机模式-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x2F &&
            LedCommand[4] == 0x06)
        {
            // if (DEVICE_OFF == get_on_off_state())
            // {
            //     // 如果七彩灯没有开，不设置电机模式（七彩灯跟电机绑定）

            //     // USER_TO_DO 可能在关机之后，关闭电机之后，也要能设置电机模式，再次开启电机时能恢复到对应模式
            //     one_wire_set_mode(LedCommand[5]);
            //     fb_motor_mode();
            //     return;
            // }

            // 旧版的程序是延时关闭，如果频繁切换电机模式，就会有问题，现在是如果设置了关闭电机，就直接关闭，不用延时关闭
            if (LedCommand[5] == MOTOR_MODE_STOP)
            {
                if (fc_effect.base_ins.mode != MOTOR_MODE_STOP)
                {
                    fc_effect.base_ins.last_mode = fc_effect.base_ins.mode;
                    fc_effect.base_ins.mode = MOTOR_MODE_STOP;
                    fc_effect.motor_on_off = DEVICE_OFF;
                }
                else
                {
                    // 如果当前电机已经停止，需要恢复成原来的模式
                    fc_effect.base_ins.mode = fc_effect.base_ins.last_mode;
                    fc_effect.motor_on_off = DEVICE_ON;
                }
            }
            else
            {
                fc_effect.motor_on_off = DEVICE_ON;
                fc_effect.base_ins.mode = LedCommand[5];
            }

            if (fc_effect.base_ins.mode == MOTOR_MODE_MUSIC_RULATION)
            {
                // 声控模式，默认设置电机为正转，最慢速度
                motor_package_data(MOTOR_MODE_FORWARD, 35);
            }
            else if (fc_effect.base_ins.mode == MOTOR_MODE_FORWARD_REVERSE)
            {
                // 如果是正反转
                if (fc_effect.base_ins.dir_in_mode_forward_reverse == 0)
                {
                    motor_package_data(MOTOR_MODE_FORWARD, fc_effect.base_ins.period);
                }
                else
                {
                    motor_package_data(MOTOR_MODE_REVERSE, fc_effect.base_ins.period);
                }
            }
            else
            {
                motor_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
            }

            os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
            fb_motor_mode();
        }

        // --------------------------------设置电机转速-----------------------------------
        if (len >= (3 + 3) &&
            LedCommand[0] == 0x01 &&
            LedCommand[1] == 0xE9 &&
            LedCommand[2] == 0x00 &&

            LedCommand[3] == 0x2F &&
            LedCommand[4] == 0x07)
        {
            fc_effect.base_ins.period = LedCommand[5];

            // 如果app传过来的数值不在motor_period数组中，则给速度值和速度值索引一个默认值
            // 更新 fc_effect.star_speed_index 索引值，后续重新上电要根据这个索引值来找到对应的电机转速
            for (u8 i = 0; i < ARRAY_SIZE(motor_period); i++)
            {
                if (motor_period[i] == fc_effect.base_ins.period)
                {
                    fc_effect.motor_speed_index = i;
                    break;
                }

                if (i == ARRAY_SIZE(motor_period) - 1)
                {
                    // 如果到最后一个元素，都没有找到对应的索引值
                    // 给一个默认值
                    fc_effect.motor_speed_index = 0;
                }
            }

            fc_effect.base_ins.period = motor_period[fc_effect.motor_speed_index];

            if (DEVICE_OFF == get_on_off_state())
            {
                // 七彩灯没有打开，只给电机转速相关的变量赋值，不驱动电机
                fb_motor_speed();
                return;
            }

            // motor_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
            if (fc_effect.base_ins.mode == MOTOR_MODE_FORWARD_REVERSE)
            {
                // 如果是在正反转模式下，改变了电机转速，需要根据当前的旋转方向，再设置一次速度
                if (fc_effect.base_ins.dir_in_mode_forward_reverse == 0)
                {
                    // 当前是在正转
                    motor_package_data(MOTOR_MODE_FORWARD, fc_effect.base_ins.period);
                }
                else
                {
                    // 当前是在反转
                    motor_package_data(MOTOR_MODE_REVERSE, fc_effect.base_ins.period);
                }

                os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
            }
            else if (fc_effect.base_ins.mode == MOTOR_MODE_MUSIC_RULATION)
            {
                /*
                    如果是在声控模式下，改变了电机转速，不给电机ic发送数据，
                    只修改 fc_effect.base_ins.period 和 fc_effect.motor_speed_index
                */
            }
            else
            {
                // 其他模式，直接给电机ic发送数据
                motor_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
                os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
            }

            fb_motor_speed(); // 向app反馈设置之后的电机速度
        }
    }
}

/* APP数据解析入口函数 */
void parse_led_strip_data(u8 *pBuf, u8 len)
{

    /* 涂鸦DP协议解析 */
    // dp_extract_data_handle(pBuf);
    /* 中道孔明灯协议解析 */
    parse_zd_data(pBuf, len);
    /* 为兼容全彩的协议 */

    // save_user_data_area3();
    os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
}

void tuya_fb_sw_state(void)
{
    dp_data_header_t *p_dp;
    u8 dp_data[4 + 1];
    p_dp = (dp_data_header_t *)dp_data;
    p_dp->id = DPID_SWITCH_LED;
    p_dp->type = DP_TYPE_BOOL;
    p_dp->len = __SWP16(1);
    dp_data[4] = 1; // 默认开机
}

/* -------------------------------------DPID_CONTROL_DATA 调节模式----------------------------- */

// 调节(只下发)
// 备注:类型：字符串;
// Value: 011112222333344445555  ;
// 0：   变化方式，0表示直接输出，1表示渐变;
// 1111：H（色度：0-360，0X0000-0X0168）;
// 2222：S (饱和：0-1000, 0X0000-0X03E8);
// 3333：V (明度：0-1000，0X0000-0X03E8);
// 4444：白光亮度（0-1000）;
// 5555：色温值（0-1000）
//  typedef struct
//  {
//      uint8_t change_type;        //变化方式，0表示直接输出，1表示渐变;
//      hsv_t c;
//      uint16_t white_b;        //白光亮度
//      uint16_t ct;             //色温
//  }dp_control_t; //DPID_CONTROL_DATA

/* -------------------------------------DPID_SCENE_DATA 场景----------------------------- */

// 场景(可下发可上报)
// 备注:Value: 0011223344445555666677778888
// 00：情景号
// 11：单元切换间隔时间（0-100）
// 22：单元变化时间（0-100）
// 33：单元变化模式（0 静态 1 跳变 2 渐变）
// 4444：H（色度：0-360，0X0000-0X0168）
// 5555：S (饱和：0-1000, 0X0000-0X03E8)
// 6666：V (明度：0-1000，0X0000-0X03E8)
// 7777：白光亮度（0-1000）
// 8888：色温值（0-1000）
// 注：数字 1-8 的标号对应有多少单元就有多少组
//  #pragma pack (1)
//  typedef struct
//  {
//      uint8_t sw_time;            //单元切换间隔时间
//      uint8_t chg_time;            //单元变化时间
//      uint8_t change_type;        //单元变化模式（0 静态 1 跳变 2 渐变）
//      hsv_t c;
//      uint16_t white_b;           //白光亮度
//      uint16_t ct;                //色温
//  }dp_secene_data_t;                 //场景数据

// typedef struct
// {
//     uint8_t secene_n;           //情景号
//     dp_secene_data_t  *data;        //多组
// }dp_secene_t;
// #pragma pack ()
/*
typedef struct
{
    color_t c[MAX_CORLOR_N];               //颜色池
    uint8_t s;                              //速度
    uint8_t n;                             //当前颜色数量
    uint8_t change_type;                   //变化模式（0 静态 1 跳变 2 渐变）
}fc_effect_t;       //全彩效果

 */
// void dp_secene_data_handle(u8 *pIn)
// {
//     dp_data_header_t *header = (dp_secene_t*) pIn;
//     uint8_t len, i = 0;
//     if(header->type != DP_TYPE_STRING)
//     {
//         #ifdef MY_DEBUG
//         printf("\n dp_secene_data_handle type err");
//         #endif
//         return;
//     }

//     /* 提取涂鸦效果数据 */
//     uint8_t secene_data[ sizeof(dp_data_header_t) * MAX_CORLOR_N + 1];

//     len = string2hex(pIn + sizeof(dp_data_header_t), &secene_data, __SWP16(header->len));

//     #ifdef MY_DEBUG
//     printf("\n secene_data =");
//     printf_buf(secene_data, len);
//     #endif

//     /* 提取颜色数量 */
//     fc_effect.n = (len - 1) / sizeof(dp_secene_data_t);
//     #ifdef MY_DEBUG
//     printf("\n dp_secene_data_t =%d",sizeof(dp_secene_data_t));
//     #endif
//     /* 提取颜色 */
//     dp_secene_data_t *pdata;
//     pdata = (dp_secene_data_t *) (secene_data + 1); //第1个byte是情景号
//     m_hsv_to_rgb(   &fc_effect.c[i].r, &fc_effect.c[i].g, &fc_effect.c[i].b, \
//                     __SWP16(pdata->c.h_val), \
//                     __SWP16(pdata->c.s_val), \
//                     __SWP16(pdata->c.v_val));
//     /* 提取速度 */
//     fc_effect.s = pdata->chg_time;
//     fc_effect.change_type = pdata->change_type;

//     #ifdef MY_DEBUG
//     printf("\n rgb =");
//     printf_buf(fc_effect.c[i], 3);
//     printf("\n fc_effect.n = %d",fc_effect.n);

//     #endif
//     i++;
//     while(i < fc_effect.n)
//     {

//         pdata = (dp_secene_data_t *) (secene_data + 1 + sizeof(dp_secene_data_t) * i); //第1个byte是情景号
//         m_hsv_to_rgb(   &fc_effect.c[i].r, &fc_effect.c[i].g, &fc_effect.c[i].b, \
//                     __SWP16(pdata->c.h_val), \
//                     __SWP16(pdata->c.s_val), \
//                     __SWP16(pdata->c.v_val));

//         #ifdef MY_DEBUG
//         printf("\n rgb =");
//         printf_buf(fc_effect.c[i], 3);
//         #endif
//         i++;
//     }
// }
