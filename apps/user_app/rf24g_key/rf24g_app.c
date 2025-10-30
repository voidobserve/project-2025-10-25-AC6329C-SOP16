
#include "system/includes.h"
#include "task.h"
#include "event.h"
#include "btstack/btstack_typedef.h"
#include "app_config.h"
#include "rf24g_app.h"
#include "WS2812FX.H"
#include "led_strip_sys.h"
#if (TCFG_RF24GKEY_ENABLE)


#if 0
/* ==============================================================

                2.4g遥控初始化

    备注：该2.4G遥控的代码逻辑，是根据遥控而定的，可能不适宜其他遥控
=================================================================*/

// 2.4G遥控
#define HEADER1 0X55
#define HEADER2 0XAA

rf24g_ins_t rf24g_ins; // 2.4g遥控数据包格式

#define SUPPORT_LONG (1)

struct RF24G_PARA is_rf24g_ = {

    .clink_delay_up = 0,
    .long_press_cnt = 0,
    .last_key_v = NO_KEY,
    .last_dynamic_code = 0,
    .rf24g_rx_flag = 0,
    .hold_pess_cnt = 0,
    .is_long_time = 30,
    .rf24g_key_state = 0,
#if SUPPORT_LONG
    // .is_click = 30, // 因没有长按功能，缩短抬起判断时间
    .is_click = 20, // 原本是30，改成20后，识别短按的速度会快一点，但是如果在关机时一直长按，会先开机，再关机
    // .is_click = 15, // 这样会识别不到长按
#else
    .is_click = 10, // 因没有长按功能，缩短抬起判断时间
#endif
    .is_long = 1,
    ._sacn_t = 10,
};

static u8 Get_24G_KeyValue()
{
    if (is_rf24g_.rf24g_rx_flag)
    {
        is_rf24g_.rf24g_rx_flag = 0;

        if ((is_rf24g_.last_dynamic_code != rf24g_ins.dynamic_code) && rf24g_ins.remoter_id == 0x00)
        {

            is_rf24g_.clink_delay_up = 0;
            is_rf24g_.hold_pess_cnt = 0;
            is_rf24g_.last_key_v = rf24g_ins.key_v;
            is_rf24g_.last_dynamic_code = rf24g_ins.dynamic_code;
            return rf24g_ins.key_v; // 当前2.4G架构无用
        }
#if SUPPORT_LONG
        else
#else
        else if (rf24g_ins.remoter_id == 0x01)
#endif
        {
            is_rf24g_.clink_delay_up = 0;
            if (is_rf24g_.hold_pess_cnt >= is_rf24g_.is_long_time)
            {
                is_rf24g_.long_press_cnt++;
                is_rf24g_.hold_pess_cnt = 0;
                is_rf24g_.rf24g_key_state = KEY_EVENT_LONG;
                is_rf24g_.last_key_v = rf24g_ins.key_v;
                return rf24g_ins.key_v; // 当前2.4G架构无用
            }
        }
    }
    return NO_KEY;
}

struct key_driver_para rf24g_scan_para = {
    .scan_time = 2,        // 按键扫描频率, 单位: ms
    .last_key = NO_KEY,    // 上一次get_value按键值, 初始化为NO_KEY;
    .filter_time = 0,      // 按键消抖延时;
    .long_time = 200,      // 按键判定长按数量
    .hold_time = 200,      // 按键判定HOLD数量
    .click_delay_time = 0, // 按键被抬起后等待连击延时数量
    .key_type = KEY_DRIVER_TYPE_RF24GKEY,
    .get_value = Get_24G_KeyValue,
};

/* 长按计数器 */
void RF24G_Key_Long_Scan(void)
{
    if (rf24g_ins.remoter_id == 0x01) // 如果2.4G遥控发过来的连续码 == 1，说明按键长按
    {
        is_rf24g_.hold_pess_cnt += (is_rf24g_._sacn_t + 2);
    }
    else // 如果2.4G遥控发过来的连续码 != 1，说明不是按键长按
    {
        is_rf24g_.hold_pess_cnt = 0;
    }
}

// 底层按键扫描，由__resolve_adv_report()调用
void rf24g_scan(u8 *pBuf)
{
    rf24g_ins_t *p = (rf24g_ins_t *)pBuf;
    if (p->header1 == HEADER1 && p->header2 == HEADER2)
    {
        // printf_buf(pBuf, sizeof(rf24g_ins_t));
        // memcpy((u8 *)&rf24g_ins, pBuf, sizeof(rf24g_ins_t));

        rf24g_ins = *p; // 结构体变量赋值
        is_rf24g_.rf24g_rx_flag = 1;
    }
}

/* ==============================================================

                2.4g遥控按键功能


=================================================================*/
u8 adjust_cw_rgb_mode = 0;
void RF24G_Key_Handle(void)
{
    // printf("%s\n", __func__); // 目前约10ms调用一次

    u8 key_value = 0;
    if (is_rf24g_.clink_delay_up < 0xFF)
        is_rf24g_.clink_delay_up++;

    // printf("is_rf24g_.clink_delay_up=%d",is_rf24g_.clink_delay_up);

    if ((is_rf24g_.clink_delay_up >= is_rf24g_.is_click) && (is_rf24g_.long_press_cnt < is_rf24g_.is_long))
    {
        key_value = is_rf24g_.last_key_v;

        if (key_value == NO_KEY)
            return;

        is_rf24g_.last_key_v = NO_KEY;
        is_rf24g_.clink_delay_up = 0;
        is_rf24g_.long_press_cnt = 0;
        printf(" ------------------------duanna key_value = %x\n", key_value);

        /*  code  */

        if (key_value == TOUCHU_KEY_1)
        {

            soft_turn_on_the_light(); // 这里开机可能需要加快速度（应该是按键检测的速度不够快）
        }

        if (get_on_off_state())
        {

            if (key_value == TOUCHU_KEY_2)
            {

                adjust_cw_rgb_mode = !adjust_cw_rgb_mode;
            }
            else if (key_value == TOUCHU_KEY_3)
            {

                // 加

                ls_add_bright();
            }
            else if (key_value == TOUCHU_KEY_6)
            {
                // 减

                ls_sub_bright();
            }
            else if (key_value == TOUCHU_KEY_4)
            {
                // 加
                if (adjust_cw_rgb_mode == 0)
                {
                    // rgb模式速度
                    ls_add_speed();
                }
                else
                {
                    //   电机速度

                    ls_add_motor_speed();
                }
            }
            else if (key_value == TOUCHU_KEY_7)
            {
                //    减

                if (adjust_cw_rgb_mode == 0)
                {
                    // rgb模式速度
                    ls_sub_speed();
                }
                else
                {
                    //   电机速度

                    ls_sub_motor_speed();
                }
            }
            else if (key_value == TOUCHU_KEY_5)
            {
                // 加
                if (adjust_cw_rgb_mode == 0)
                {
                    // rgb模式
                    ls_add_mode_InAPP();
                }
                else
                {
                    //   流星速度
                    ls_add_star_speed();
                }
            }
            else if (key_value == TOUCHU_KEY_8)
            {
                // 减
                if (adjust_cw_rgb_mode == 0)
                {
                    // rgb模式
                    ls_sub_mode_InAPP();
                }
                else
                {
                    //   流星速度
                    ls_sub_star_speed();
                }
            }
            else if (key_value == TOUCHU_KEY_9)
            {
                //    备用
            }
            else if (key_value == TOUCHU_KEY_a)
            {
                // 色环
                // printf("chromatic circle\n");
                set_static_mode(rf24g_ins.sum_r, rf24g_ins.sum_g, rf24g_ins.sum_b);
            }
            else if (key_value == TOUCHU_KEY_b)
            {

                // 色环

                app_set_w(rf24g_ins.sum_cw);
            }
            save_user_data_area3();
        }
    }
    else if (is_rf24g_.clink_delay_up > is_rf24g_.is_click)
    {
        is_rf24g_.clink_delay_up = 0;
        is_rf24g_.long_press_cnt = 0;
        is_rf24g_.last_key_v = NO_KEY;
        // printf(" ================================songhshou ");
        printf("loose\n");
    }

    if (is_rf24g_.long_press_cnt > is_rf24g_.is_long)
    {

        key_value = is_rf24g_.last_key_v;
        if (key_value == NO_KEY)
            return;
        is_rf24g_.last_key_v = NO_KEY;
        /*  因为执行这里，清了键值，而长按时，1s才获取一次键值 */
        printf(" ================================longkey_value = %d", key_value);

#if SUPPORT_LONG

        /* code */
        if (key_value == 82)
        {

            if (is_rf24g_.long_press_cnt == 2)
                soft_turn_off_lights();
        }
        else if (key_value == TOUCHU_KEY_a)
        {
            // 色环
            printf("chromatic circle\n");
            set_static_mode(rf24g_ins.sum_r, rf24g_ins.sum_g, rf24g_ins.sum_b);
        }
#endif
    }
}
#endif

#endif // #if (TCFG_RF24GKEY_ENABLE)
