#include "rf433_key.h"
#include "rf433_learn.h"

#include "../../../apps/user_app/one_wire/one_wire.h"

#include "../../../apps/user_app/led_strip/led_strip_sys.h"
#include "led_strand_effect.h"

#include "../../../apps/user_app/save_flash/save_flash.h"
#include "../../../apps/user_app/ws2812-fx-lib/WS2812FX_C/ws2812fx_effect.h"

#include "user_include.h"

#if RF_433_KEY_ENABLE

static volatile u32 rf_data = 0;              // 定时器中断使用的接收缓冲区，避免直接覆盖全局的数据接收缓冲区
volatile u32 recv_rf_433_data = 0;            // 存放在中断接收完成的rf433数据
volatile u8 flag_is_received_rf_433_data = 0; // 标志位，是否接收到了433数据

void rf_433_key_config(void)
{
    // gpio_set_pull_up(RF_433_KEY_SCAN_PIN, 1);   // 上拉
    gpio_set_pull_up(RF_433_KEY_SCAN_PIN, 0);   // 不上拉（因为433接收模块ic的拉低能力比较低）
    gpio_set_pull_down(RF_433_KEY_SCAN_PIN, 0); // 不下拉

    gpio_set_die(RF_433_KEY_SCAN_PIN, 1);       // 普通输入模式
    gpio_set_direction(RF_433_KEY_SCAN_PIN, 1); // 输入模式
}

static u8 rf_433_key_get_value(void);
volatile rf_433_key_struct_t rf_433_key_structure = {
    .rf_433_key_para.scan_time = RF_433_KEY_SCAN_CIRCLE_TIMES, // 扫描间隔时间
    .rf_433_key_para.last_key = NO_KEY,
    .rf_433_key_para.filter_value = 0,
    .rf_433_key_para.filter_cnt = 0,
    .rf_433_key_para.filter_time = 0, // 按键消抖次数
    .rf_433_key_para.long_time = RF_433_KEY_SCAN_LONG_PRESS_TIME_THRESHOLD / RF_433_KEY_SCAN_CIRCLE_TIMES,
    .rf_433_key_para.hold_time = (RF_433_KEY_SCAN_LONG_PRESS_TIME_THRESHOLD + RF_433_KEY_SCAN_HOLD_PRESS_TIME_THRESHOLD) / RF_433_KEY_SCAN_CIRCLE_TIMES,
    .rf_433_key_para.press_cnt = 0,
    .rf_433_key_para.click_cnt = 0,
    .rf_433_key_para.click_delay_cnt = 0,
    .rf_433_key_para.click_delay_time = RF_433_KEY_SCAN_MUILTY_CLICK_TIME_THRESHOLD / RF_433_KEY_SCAN_CIRCLE_TIMES,
    .rf_433_key_para.notify_value = 0,
    .rf_433_key_para.key_type = KEY_DRIVER_TYPE_RF_433_KEY,
    .rf_433_key_para.get_value = rf_433_key_get_value,
    .rf_433_key_driver_event = 0,
    .rf_433_key_latest_key_val = NO_KEY,
};

// 将遥控器按键的键值和按键事件绑定
const u8 rf_433_key_event_table[][RF_433_KEY_VALID_EVENT_NUMS + 1] = {
    {RF_433_KEY_R1C1, RF_433_KEY_EVENT_R1C1_CLICK, RF_433_KEY_EVENT_R1C1_LONG, RF_433_KEY_EVENT_R1C1_HOLD, RF_433_KEY_EVENT_R1C1_LOOSE},
    {RF_433_KEY_R1C2, RF_433_KEY_EVENT_R1C2_CLICK, RF_433_KEY_EVENT_R1C2_LONG, RF_433_KEY_EVENT_R1C2_HOLD, RF_433_KEY_EVENT_R1C2_LOOSE},
    {RF_433_KEY_R1C3, RF_433_KEY_EVENT_R1C3_CLICK, RF_433_KEY_EVENT_R1C3_LONG, RF_433_KEY_EVENT_R1C3_HOLD, RF_433_KEY_EVENT_R1C3_LOOSE},
    {RF_433_KEY_R1C4, RF_433_KEY_EVENT_R1C4_CLICK, RF_433_KEY_EVENT_R1C4_LONG, RF_433_KEY_EVENT_R1C4_HOLD, RF_433_KEY_EVENT_R1C4_LOOSE},

    {RF_433_KEY_R2C1, RF_433_KEY_EVENT_R2C1_CLICK, RF_433_KEY_EVENT_R2C1_LONG, RF_433_KEY_EVENT_R2C1_HOLD, RF_433_KEY_EVENT_R2C1_LOOSE},
    {RF_433_KEY_R2C2, RF_433_KEY_EVENT_R2C2_CLICK, RF_433_KEY_EVENT_R2C2_LONG, RF_433_KEY_EVENT_R2C2_HOLD, RF_433_KEY_EVENT_R2C2_LOOSE},
    {RF_433_KEY_R2C3, RF_433_KEY_EVENT_R2C3_CLICK, RF_433_KEY_EVENT_R2C3_LONG, RF_433_KEY_EVENT_R2C3_HOLD, RF_433_KEY_EVENT_R2C3_LOOSE},
    {RF_433_KEY_R2C4, RF_433_KEY_EVENT_R2C4_CLICK, RF_433_KEY_EVENT_R2C4_LONG, RF_433_KEY_EVENT_R2C4_HOLD, RF_433_KEY_EVENT_R2C4_LOOSE},

    {RF_433_KEY_R3C1, RF_433_KEY_EVENT_R3C1_CLICK, RF_433_KEY_EVENT_R3C1_LONG, RF_433_KEY_EVENT_R3C1_HOLD, RF_433_KEY_EVENT_R3C1_LOOSE},
    {RF_433_KEY_R3C2, RF_433_KEY_EVENT_R3C2_CLICK, RF_433_KEY_EVENT_R3C2_LONG, RF_433_KEY_EVENT_R3C2_HOLD, RF_433_KEY_EVENT_R3C2_LOOSE},
    {RF_433_KEY_R3C3, RF_433_KEY_EVENT_R3C3_CLICK, RF_433_KEY_EVENT_R3C3_LONG, RF_433_KEY_EVENT_R3C3_HOLD, RF_433_KEY_EVENT_R3C3_LOOSE},
    {RF_433_KEY_R3C4, RF_433_KEY_EVENT_R3C4_CLICK, RF_433_KEY_EVENT_R3C4_LONG, RF_433_KEY_EVENT_R3C4_HOLD, RF_433_KEY_EVENT_R3C4_LOOSE},

    {RF_433_KEY_R4C1, RF_433_KEY_EVENT_R4C1_CLICK, RF_433_KEY_EVENT_R4C1_LONG, RF_433_KEY_EVENT_R4C1_HOLD, RF_433_KEY_EVENT_R4C1_LOOSE},
    {RF_433_KEY_R4C2, RF_433_KEY_EVENT_R4C2_CLICK, RF_433_KEY_EVENT_R4C2_LONG, RF_433_KEY_EVENT_R4C2_HOLD, RF_433_KEY_EVENT_R4C2_LOOSE},
    {RF_433_KEY_R4C3, RF_433_KEY_EVENT_R4C3_CLICK, RF_433_KEY_EVENT_R4C3_LONG, RF_433_KEY_EVENT_R4C3_HOLD, RF_433_KEY_EVENT_R4C3_LOOSE},
    {RF_433_KEY_R4C4, RF_433_KEY_EVENT_R4C4_CLICK, RF_433_KEY_EVENT_R4C4_LONG, RF_433_KEY_EVENT_R4C4_HOLD, RF_433_KEY_EVENT_R4C4_LOOSE},

    {RF_433_KEY_R5C1, RF_433_KEY_EVENT_R5C1_CLICK, RF_433_KEY_EVENT_R5C1_LONG, RF_433_KEY_EVENT_R5C1_HOLD, RF_433_KEY_EVENT_R5C1_LOOSE},
    {RF_433_KEY_R5C2, RF_433_KEY_EVENT_R5C2_CLICK, RF_433_KEY_EVENT_R5C2_LONG, RF_433_KEY_EVENT_R5C2_HOLD, RF_433_KEY_EVENT_R5C2_LOOSE},
    {RF_433_KEY_R5C3, RF_433_KEY_EVENT_R5C3_CLICK, RF_433_KEY_EVENT_R5C3_LONG, RF_433_KEY_EVENT_R5C3_HOLD, RF_433_KEY_EVENT_R5C3_LOOSE},
    {RF_433_KEY_R5C4, RF_433_KEY_EVENT_R5C4_CLICK, RF_433_KEY_EVENT_R5C4_LONG, RF_433_KEY_EVENT_R5C4_HOLD, RF_433_KEY_EVENT_R5C4_LOOSE},

    {RF_433_KEY_R6C1, RF_433_KEY_EVENT_R6C1_CLICK, RF_433_KEY_EVENT_R6C1_LONG, RF_433_KEY_EVENT_R6C1_HOLD, RF_433_KEY_EVENT_R6C1_LOOSE},
    {RF_433_KEY_R6C2, RF_433_KEY_EVENT_R6C2_CLICK, RF_433_KEY_EVENT_R6C2_LONG, RF_433_KEY_EVENT_R6C2_HOLD, RF_433_KEY_EVENT_R6C2_LOOSE},
    {RF_433_KEY_R6C3, RF_433_KEY_EVENT_R6C3_CLICK, RF_433_KEY_EVENT_R6C3_LONG, RF_433_KEY_EVENT_R6C3_HOLD, RF_433_KEY_EVENT_R6C3_LOOSE},
    {RF_433_KEY_R6C4, RF_433_KEY_EVENT_R6C4_CLICK, RF_433_KEY_EVENT_R6C4_LONG, RF_433_KEY_EVENT_R6C4_HOLD, RF_433_KEY_EVENT_R6C4_LOOSE},

    {RF_433_KEY_R7C1, RF_433_KEY_EVENT_R7C1_CLICK, RF_433_KEY_EVENT_R7C1_LONG, RF_433_KEY_EVENT_R7C1_HOLD, RF_433_KEY_EVENT_R7C1_LOOSE},
    {RF_433_KEY_R7C2, RF_433_KEY_EVENT_R7C2_CLICK, RF_433_KEY_EVENT_R7C2_LONG, RF_433_KEY_EVENT_R7C2_HOLD, RF_433_KEY_EVENT_R7C2_LOOSE},
    {RF_433_KEY_R7C3, RF_433_KEY_EVENT_R7C3_CLICK, RF_433_KEY_EVENT_R7C3_LONG, RF_433_KEY_EVENT_R7C3_HOLD, RF_433_KEY_EVENT_R7C3_LOOSE},
    {RF_433_KEY_R7C4, RF_433_KEY_EVENT_R7C4_CLICK, RF_433_KEY_EVENT_R7C4_LONG, RF_433_KEY_EVENT_R7C4_HOLD, RF_433_KEY_EVENT_R7C4_LOOSE},
};

static u8 rf_433_key_get_value(void)
{
    // 超时时间，在超时时间内，仍认为有按键按下
    static u8 time_out_cnt = 0;
    static u32 last_rf_433_data = NO_KEY;

    u8 ret = NO_KEY;

    if (flag_is_received_rf_433_data)
    {
        flag_is_received_rf_433_data = 0;

        last_rf_433_data = recv_rf_433_data;
#if RF_433_LEARN_ENABLE
        recv_rf_433_addr = recv_rf_433_data >> 8; // 存放接收到的遥控器的地址；客户给到的遥控器，后面8位是键值，前面16位是地址
#endif                                            // #if RF_433_LEARN_ENABLE
        time_out_cnt = RF_433_KEY_SCAN_EFFECTIVE_TIME_OUT / RF_433_KEY_SCAN_CIRCLE_TIMES;
        ret = (u8)(recv_rf_433_data & 0xFF);

        // printf("get key val 0x %lx \n", recv_rf_433_data);
        // printf("rf 433 addr 0x %lx \n", recv_rf_433_data >> 8);
        // printf("recved 433 data\n");
    }
    else if (time_out_cnt > 0)
    {
        ret = (u8)(last_rf_433_data & 0xFF);
        time_out_cnt--;
        // printf("exterd 433 data\n");
    }

    return ret;
}

/**
 * @brief 根据 key_driver_scan()得到的按键键值和按键事件，转换成自定义的按键事件
 *
 * @param key_value key_driver_scan()得到的按键键值
 * @param key_event key_driver_scan()得到的按键事件
 *
 * @return u8 自定义的按键事件
 */
static u8 rf_433_key_get_keyevent(const u8 key_val, const u8 key_event)
{
    u8 i;
    u8 rf_433_key_event = RF_433_KEY_EVENT_NONE;
    u8 rf_433_key_event_table_index = 0;

    // printf("key event %u\n", (u16)key_event);

    /* 将 key_driver_scan() 得到的按键事件映射到自定义的按键事件列表中，用于下面的查表 */
    switch (key_event)
    {
    case KEY_EVENT_CLICK:
        /* 短按，在数组 rf_433_key_event_table 的[x][1]位置*/
        rf_433_key_event_table_index = 1;
        // printf("key event click\n");
        break;
    case KEY_EVENT_LONG:
        /* 长按，在数组 rf_433_key_event_table 的[x][2]位置 */
        rf_433_key_event_table_index = 2;
        // printf("key event long\n");
        break;
    case KEY_EVENT_HOLD:
        /* 长按持续（不松手），在数组 rf_433_key_event_table 的[x][3]位置 */
        rf_433_key_event_table_index = 3;
        // printf("key event hold\n");
        break;
    case KEY_EVENT_UP:
        /* 长按后松手，在数组 rf_433_key_event_table 的[x][4]位置 */
        rf_433_key_event_table_index = 4;
        break;

    default:
        // 其他按键事件，认为是没有事件
        rf_433_key_event = RF_433_KEY_EVENT_NONE;
        return rf_433_key_event;
        break;
    }

    for (i = 0; i < ARRAY_SIZE(rf_433_key_event_table); i++)
    {
        if (key_val == rf_433_key_event_table[i][0])
        {
            rf_433_key_event = rf_433_key_event_table[i][rf_433_key_event_table_index];
            break;
        }
    }

    // printf("rf key event %u\n", (u16)rf_433_key_event);

    return rf_433_key_event;
}

void rf_433_key_decode_isr(void)
{
#if 1 // rf信号接收 （125us调用一次，由100us调用一次的版本修改而来）
    {
        static volatile u8 rf_bit_cnt; // RF信号接收的数据位计数值

        static volatile u8 flag_is_enable_recv;   // 是否使能接收的标志位，要接收到 5ms+ 的低电平才开始接收
        static volatile u8 __flag_is_recved_data; // 表示中断服务函数接收到了rf数据

        static volatile u8 low_level_cnt;  // RF信号低电平计数值
        static volatile u8 high_level_cnt; // RF信号高电平计数值

        // 测试中断调用该函数的周期：
        // static volatile u8 cnt = 0;
        // cnt++;
        // if (cnt >= 100)
        // {
        //     cnt = 0;
        //     printf("%s\n", __func__);
        // }

        // 在定时器 中扫描端口电平
        // if (0 == RFIN_PIN)
        if (0 == gpio_read(RF_433_KEY_SCAN_PIN))
        {
            // 测试用，看看能不能检测到低电平
            // gpio_set_output_value(IO_PORTB_00, 0); // 1高0低

            // 如果RF接收引脚为低电平，记录低电平的持续时间
            low_level_cnt++;

            /*
                下面的判断条件是避免部分遥控器或接收模块只发送24位数据，最后不拉高电平的情况
            */
            // if (low_level_cnt >= (u8)((u32)30 * 100 / 125) && rf_bit_cnt == 23) // 如果低电平大于3000us，并且已经接收了23位数据
            if (low_level_cnt >= 24 && rf_bit_cnt == 23) // 如果低电平大于3000us，并且已经接收了23位数据
            {
                // if (high_level_cnt >= (u8)((u32)6 * 100 / 125) && high_level_cnt < (u8)((u32)20 * 100 / 125))
                if (high_level_cnt >= 5 && high_level_cnt < 12)
                {
                    /* 高电平时间在 【625us ~  1500us】，认为是逻辑1*/
                    rf_data |= 0x01;
                }
                // else if (high_level_cnt >= 1 /* 这里不能为0，因此不能加 【* 100 / 125】  */
                //          && high_level_cnt < ((u32)6 * 100 / 125))
                else if (high_level_cnt >= 0 && high_level_cnt < 5)
                {
                }

                __flag_is_recved_data = 1; // 接收完成标志位置一
                flag_is_enable_recv = 0;
            }
        }
        else
        {
            // 测试用，看看能不能检测到高电平
            // gpio_set_output_value(IO_PORTB_00, 1); // 1高0低

            if (low_level_cnt > 0)
            {
                // 如果之前接收到了低电平信号，现在遇到了高电平，判断是否接收完成了一位数据
                // if (low_level_cnt > (u8)((u32)50 * 100 / 125))
                if (low_level_cnt > 40)
                {
                    // 如果低电平持续时间大于50 * 100us（5ms），准备下一次再读取有效信号
                    rf_data = 0;    // 清除接收的数据帧
                    rf_bit_cnt = 0; // 清除用来记录接收的数据位数

                    flag_is_enable_recv = 1;
                }
                else if (flag_is_enable_recv &&
                         low_level_cnt > 0 && low_level_cnt < 5 &&
                         high_level_cnt >= 5 && high_level_cnt < 12)
                {
                    /* 逻辑1 高电平时间 625 ~ 1500us，低电平时间 0 ~ 625us */
                    rf_data |= 0x01;
                    rf_bit_cnt++;
                    if (rf_bit_cnt != 24)
                    {
                        rf_data <<= 1; // 用于存放接收24位数据的变量左移一位
                    }
                }
                else if (flag_is_enable_recv &&
                         low_level_cnt >= 5 && low_level_cnt < 12 &&
                         high_level_cnt > 0 /* 这里不能为0，因此不能加 【* 100 / 125】  */
                         && high_level_cnt < 5)
                {
                    /* 逻辑0 高电平时间 0~625us，低电平时间 625~1500us */
                    rf_data &= ~1;
                    rf_bit_cnt++;
                    if (rf_bit_cnt != 24)
                    {
                        rf_data <<= 1; // 用于存放接收24位数据的变量左移一位
                    }
                }
                else
                {
                    // 如果低电平持续时间不符合0和1的判断条件，说明此时没有接收到信号
                    rf_data = 0;
                    rf_bit_cnt = 0;
                    flag_is_enable_recv = 0;
                }

                low_level_cnt = 0; // 无论是否接收到一位数据，遇到高电平时，先清除之前的计数值
                high_level_cnt = 0;

                if (24 == rf_bit_cnt)
                {
                    // 如果接收成了24位的数据
                    __flag_is_recved_data = 1; // 接收完成标志位置一
                    flag_is_enable_recv = 0;
                }
            }
            else
            {
                // 如果接收到高电平后，低电平的计数为0

                if (0 == flag_is_enable_recv)
                {
                    rf_data = 0;
                    rf_bit_cnt = 0;
                    flag_is_enable_recv = 0;
                }
            }

            // 如果RF接收引脚为高电平，记录高电平的持续时间
            high_level_cnt++;
        }

        if (__flag_is_recved_data) //
        {
            rf_bit_cnt = 0;
            __flag_is_recved_data = 0;
            low_level_cnt = 0;
            high_level_cnt = 0;

            // if (rf_data != 0)
            // if (0 == flag_is_recved_rf_data) /* 如果之前未接收到数据 或是 已经处理完上一次接收到的数据 */
            {
                // 现在改为只要收到新的数据，就覆盖 recv_rf_433_data
                recv_rf_433_data = rf_data;

                flag_is_received_rf_433_data = 1;

                // printf("recv_rf_433_data = %x\n", (u32)recv_rf_433_data);
            }
            // else
            // {
            //     __rf_data = 0;
            // }
        }
    }
#endif // rf信号接收 （125us调用一次，由100us调用一次的版本修改而来）
}

void rf_433_key_event_handle(void)
{
    u8 rf_433_key_event = RF_433_KEY_EVENT_NONE;

    // if (NO_KEY == rf_433_key_structure.rf_433_key_latest_key_val)
    // {
    //     return;
    // }

    // printf("get key event\n");

#if 1
#if RF_433_LEARN_ENABLE
    u32 rf_433_addr = rf_433_addr_get();
    u32 cur_rf_433_addr = recv_rf_433_addr;
    if (rf_433_addr != cur_rf_433_addr)
    {
        // 学习/对码 之后的地址与当前接收到的地址不一致，直接返回，不处理事件

        // 不能在这里清除事件，会影响对码操作
        // rf_433_key_structure.rf_433_key_latest_key_val = NO_KEY;
        // rf_433_key_structure.rf_433_key_driver_event = RF_433_KEY_EVENT_NONE;

        // printf("recv_rf_433_addr != cur_rf_433_addr\n");
        return;
    }

    // 如果还在执行学习成功的动画，需要等动画结束再继续响应

    if (RF_433_LEARN_STATUS_PROCESSING == rf_433_learn_status_get())
    {
        rf_433_key_structure.rf_433_key_latest_key_val = NO_KEY;
        rf_433_key_structure.rf_433_key_driver_event = RF_433_KEY_EVENT_NONE;
        return;
    }

#endif // #if RF_433_LEARN_ENABLE
#endif

    rf_433_key_event = rf_433_key_get_keyevent(rf_433_key_structure.rf_433_key_latest_key_val, rf_433_key_structure.rf_433_key_driver_event);
    rf_433_key_structure.rf_433_key_latest_key_val = NO_KEY;
    rf_433_key_structure.rf_433_key_driver_event = RF_433_KEY_EVENT_NONE;

    if (RF_433_KEY_EVENT_NONE == rf_433_key_event)
    {
        return;
    }

    switch (rf_433_key_event)
    {
    case RF_433_KEY_EVENT_R1C1_CLICK:
    case RF_433_KEY_EVENT_R1C1_LONG:
    case RF_433_KEY_EVENT_R1C1_HOLD:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        if (IS_STATIC == fc_effect.Now_state)
        {
            const u8 step = 10;
            if (fc_effect.app_b < 100 - step)
            {
                fc_effect.app_b += step;
            }
            else
            {
                fc_effect.app_b = 100;
            }

            /*
                七彩灯的亮度值范围： 0 ~ 255 ，
                但是只用到 25（255的10%） ~ 255，
                这里通过计算，将 fc_effect.app_b 的 0 ~ 100 映射到 25 ~ 255
            */
            // fc_effect.b = (u16)fc_effect.app_b * (255 - 25) / 100 + 25;
            colorful_lights_set_brightness(fc_effect.app_b);
            WS2812FX_setBrightness(fc_effect.b);
            printf("fc_effect.app_b %u\n", (u16)fc_effect.app_b);
            printf("fc_effect.b %u\n", (u16)fc_effect.b);
            fb_bright();
        }
        else if (IS_light_scene == fc_effect.Now_state && // 七彩灯的动态模式
                 (MODO_COLORFUL_LIGHTS_FLASH == fc_effect.dream_scene.change_type ||
                  MODE_COLORFUL_LIGHTS_BREATH == fc_effect.dream_scene.change_type ||
                  MODE_COLORFUL_LIGHTS_GRADUAL == fc_effect.dream_scene.change_type ||
                  MODE_COLORFUL_LIGHTS_JUMP == fc_effect.dream_scene.change_type ||
                  MODE_COLORFUL_LIGHTS_AUTO == fc_effect.dream_scene.change_type))
        {
            // 七彩灯动态模式下，调节速度
            // 注意：速度值是越小越快
            const u8 step = 10;
            if (fc_effect.app_speed < 100 - step)
            {
                fc_effect.app_speed += step;
            }
            else
            {
                fc_effect.app_speed = 100;
            }

            /*
                MODO_COLORFUL_LIGHTS_FLASH ~ MODE_COLORFUL_LIGHTS_AUTO 模式中，速度值范围：0 ~ 2000
                一般只用 200 ~ 2000 这个范围，
                这里通过计算将 fc_effect.dream_scene.speed 的值限制在 200 ~ 2000

                MODO_COLORFUL_LIGHTS_FLASH ~ MODE_COLORFUL_LIGHTS_AUTO 模式中，速度值范围：0 ~ 5000
                一般只用 200 ~ 5000 这个范围，
                这里通过计算将 fc_effect.dream_scene.speed 的值限制在 200 ~ 5000
            */
            // fc_effect.dream_scene.speed = 2000 - ((u32)fc_effect.app_speed * (2000 - 200) / 100);
            // fc_effect.dream_scene.speed = 5000 - ((u32)fc_effect.app_speed * (5000 - 200) / 100);
            colorful_lights_set_speed(fc_effect.app_speed);
            printf("fc_effect.app_speed %u\n", (u16)fc_effect.app_speed);
            printf("fc_effect.dream_scene.speed %u\n", (u16)fc_effect.dream_scene.speed);
            fb_speed();
        }
        else if (IS_light_music == fc_effect.Now_state)
        {
            // 七彩灯声控模式下，调节灵敏度
            colorful_lights_sound_sensitivity_add();
            fc_effect.music.s = fc_effect.colorful_lights_sensitivity;
            fb_sensitive(); // 向app反馈灵敏度
        }
        else
        {
            // 其他模式，直接退出，不执行后续的读写flash操作
            return;
        }
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R1C2_CLICK:
    case RF_433_KEY_EVENT_R1C2_LONG: // 亮度减
    case RF_433_KEY_EVENT_R1C2_HOLD:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        if (IS_STATIC == fc_effect.Now_state)
        {
            // 注意：不能让灯光亮度减到0%，灯光会熄灭
            const u8 step = 10;
            if (fc_effect.app_b > 0 + step)
            {
                fc_effect.app_b -= step;
            }
            else
            {
                fc_effect.app_b = 0;
            }

            /*
                七彩灯的亮度值范围： 0 ~ 255 ，
                但是只用到 25（255的10%） ~ 255，
                这里通过计算，将 fc_effect.app_b 的 0 ~ 100 映射到 25 ~ 255
            */
            // fc_effect.b = (u16)fc_effect.app_b * (255 - 25) / 100 + 25;
            colorful_lights_set_brightness(fc_effect.app_b);
            WS2812FX_setBrightness(fc_effect.b);
            printf("fc_effect.app_b %u\n", (u16)fc_effect.app_b);
            printf("fc_effect.b %u\n", (u16)fc_effect.b);
            fb_bright();
        }
        else if (IS_light_scene == fc_effect.Now_state && // 七彩灯的动态模式
                 (MODO_COLORFUL_LIGHTS_FLASH == fc_effect.dream_scene.change_type ||
                  MODE_COLORFUL_LIGHTS_BREATH == fc_effect.dream_scene.change_type ||
                  MODE_COLORFUL_LIGHTS_GRADUAL == fc_effect.dream_scene.change_type ||
                  MODE_COLORFUL_LIGHTS_JUMP == fc_effect.dream_scene.change_type ||
                  MODE_COLORFUL_LIGHTS_AUTO == fc_effect.dream_scene.change_type))
        {
            // 七彩灯动态模式下，调节速度
            // 注意：速度值是越小越快
            const u8 step = 10;
            if (fc_effect.app_speed > 0 + step)
            {
                fc_effect.app_speed -= step;
            }
            else
            {
                fc_effect.app_speed = 0;
            }

            /*
                MODO_COLORFUL_LIGHTS_FLASH ~ MODE_COLORFUL_LIGHTS_AUTO 模式中，速度值范围：0 ~ 2000
                一般只用 200 ~ 2000 这个范围，
                这里通过计算将 fc_effect.dream_scene.speed 的值限制在 200 ~ 2000

                MODO_COLORFUL_LIGHTS_FLASH ~ MODE_COLORFUL_LIGHTS_AUTO 模式中，速度值范围：0 ~ 5000
                一般只用 200 ~ 5000 这个范围，
                这里通过计算将 fc_effect.dream_scene.speed 的值限制在 200 ~ 5000
            */
            // fc_effect.dream_scene.speed = 2000 - ((u32)fc_effect.app_speed * (2000 - 200) / 100);
            // fc_effect.dream_scene.speed = 5000 - ((u32)fc_effect.app_speed * (5000 - 200) / 100);
            colorful_lights_set_speed(fc_effect.app_speed);
            printf("fc_effect.app_speed %u\n", (u16)fc_effect.app_speed);
            printf("fc_effect.dream_scene.speed %u\n", (u16)fc_effect.dream_scene.speed);
            fb_speed();
        }
        else if (IS_light_music == fc_effect.Now_state)
        {
            // 七彩灯声控模式下，调节灵敏度
            colorful_lights_sound_sensitivity_sub();
            fc_effect.music.s = fc_effect.colorful_lights_sensitivity;
            fb_sensitive(); // 向app反馈灵敏度
        }
        else
        {
            // 其他模式，直接退出，不执行后续的读写flash操作
            return;
        }
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R1C3_CLICK:
    case RF_433_KEY_EVENT_R1C3_LONG:
    {
        // soft_turn_on_the_light(); // 打开设备

        // 只开 七彩灯 和 电机
        colorful_light_open();
        fb_led_on_off_state(); // 与app反馈七彩灯的开关状态

        motor_open();
        fb_motor_mode();  // 向app反馈电机的模式
        fb_motor_speed(); // 向app反馈电机转速
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R1C4_CLICK:
    case RF_433_KEY_EVENT_R1C4_LONG:
    {
        // 关灯
        // soft_turn_off_lights();

        // 只关 七彩灯 和 电机
        colorful_light_close();
        fb_led_on_off_state(); // 与app同步开关状态

        motor_close();
        fb_motor_mode();  // 向app反馈电机的模式
        fb_motor_speed(); // 向app反馈电机转速
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R2C1_CLICK:
    case RF_433_KEY_EVENT_R2C1_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // RED
        colorful_lights_set_static_color(RED);
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R2C2_CLICK:
    case RF_433_KEY_EVENT_R2C2_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // GREEN
        colorful_lights_set_static_color(GREEN);
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R2C3_CLICK:
    case RF_433_KEY_EVENT_R2C3_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // BLUE
        colorful_lights_set_static_color(BLUE);
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R2C4_CLICK:
    case RF_433_KEY_EVENT_R2C4_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 纯白色
        color_t color_structure = {0};
        color_structure.r = 0x00;
        color_structure.g = 0x00;
        color_structure.b = 0x00;
        color_structure.w = 0xFF;
        colorful_lights_set_static_mode(color_structure);
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R3C1_CLICK:
    case RF_433_KEY_EVENT_R3C1_LONG: //
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 橙色
        colorful_lights_set_static_color(ORANGE);
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R3C2_CLICK:
    case RF_433_KEY_EVENT_R3C2_LONG: //
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 黄色
        colorful_lights_set_static_color(YELLOW);
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R3C3_CLICK:
    case RF_433_KEY_EVENT_R3C3_LONG: //
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // CYAN 青色
        colorful_lights_set_static_color(CYAN);
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R3C4_CLICK:
    case RF_433_KEY_EVENT_R3C4_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 紫色
        colorful_lights_set_static_color(PURPLE);
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R4C1_CLICK:
    case RF_433_KEY_EVENT_R4C1_LONG: //
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 七彩跳变
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_JUMP;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        WS2812FX_resetSegmentRuntime(0); // 清空灯光动画运行时使用的数据，让动画重新开始跑
        set_fc_effect();
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R4C2_CLICK:
    case RF_433_KEY_EVENT_R4C2_LONG: //
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 七彩渐变
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_GRADUAL;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        WS2812FX_resetSegmentRuntime(0); // 清空灯光动画运行时使用的数据，让动画重新开始跑
        set_fc_effect();
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R4C3_CLICK:
    case RF_433_KEY_EVENT_R4C3_LONG: //
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 七彩呼吸
        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = 7;
        fc_effect.Now_state = IS_light_scene;
        WS2812FX_resetSegmentRuntime(0); // 清空灯光动画运行时使用的数据，让动画重新开始跑
        set_fc_effect();
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R4C4_CLICK:
    case RF_433_KEY_EVENT_R4C4_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 自动模式 七彩跳变->七彩渐变->七彩呼吸->七彩跳变-> ......
        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_AUTO;
        fc_effect.Now_state = IS_light_scene;
        WS2812FX_resetSegmentRuntime(0); // 清空灯光动画运行时使用的数据，让动画重新开始跑
        set_fc_effect();
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R5C1_CLICK:
    case RF_433_KEY_EVENT_R5C1_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 声控 渐变
        fc_effect.Now_state = IS_light_music;
        fc_effect.music.m = 0; // 设置 声控模式索引
        set_fc_effect();
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R5C2_CLICK:
    case RF_433_KEY_EVENT_R5C2_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 声控 呼吸
        fc_effect.Now_state = IS_light_music;
        fc_effect.music.m = 1; // 设置 声控模式索引
        set_fc_effect();
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R5C3_CLICK:
    case RF_433_KEY_EVENT_R5C3_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 声控 静态定色
        fc_effect.Now_state = IS_light_music;
        fc_effect.music.m = 2;
        set_fc_effect();
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R5C4_CLICK:
    case RF_433_KEY_EVENT_R5C4_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 七彩灯没有打开，直接返回
            return;
        }

        // 声控 跳变
        fc_effect.Now_state = IS_light_music;
        fc_effect.music.m = 3;
        set_fc_effect();
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R6C1_CLICK:
    case RF_433_KEY_EVENT_R6C1_LONG:
    {
        // 流星灯开关
        if (fc_effect.star_on_off == DEVICE_OFF)
        {
            fc_effect.star_on_off = DEVICE_ON;
            printf("meteor lights on\n");
        }
        else
        {
            fc_effect.star_on_off = DEVICE_OFF;
            printf("meteor lights off\n");
        }

        if (DEVICE_ON == fc_effect.star_on_off)
        {
            ls_meteor_stat_effect();
        }
        else
        {
            WS2812FX_stop();
            WS2812FX_setSegment_colorOptions(
                1,                     // 第0段
                1,                     // 起始位置
                fc_effect.led_num - 1, // 结束位置
                &close_metemor,        // 效果
                0,                     // 颜色
                fc_effect.star_speed,  // 速度
                0);                    // 选项，这里像素点大小：3 REVERSE决定方向
            // WS2812FX_start();
            WS2812FX_resetSegmentRuntime(1); // 重置流星灯所在的段运行时参数
            WS2812FX_running_flag_set();
        }

        fd_meteor_on_off(); // 向app反馈流星灯的开关机状态
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R6C2_CLICK:
    case RF_433_KEY_EVENT_R6C2_LONG:
    {
        if (fc_effect.star_on_off == DEVICE_OFF)
        {
            // 如果流星灯没有启动，不做处理
            return;
        }

        // 流星灯模式切换
        // 流星灯索引值范围： 1 ~ 16
        fc_effect.star_index++;
        if (fc_effect.star_index > 16)
        {
            fc_effect.star_index = 1;
        }

        ls_meteor_stat_effect(); // 根据索引值，设置流星灯模式
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R6C3_CLICK:
    case RF_433_KEY_EVENT_R6C3_LONG:
    {
        if (fc_effect.star_on_off == DEVICE_OFF)
        {
            // 如果流星灯没有启动，不做处理
            return;
        }

        if (fc_effect.star_index >= 1 && fc_effect.star_index <= 14)
        {
            // 如果不在声控模式，调节流星灯速度
            // 流星灯 速度 加
            const u8 step = 10;
            if (fc_effect.app_star_speed < 100 - step)
            {
                fc_effect.app_star_speed += step;
            }
            else
            {
                fc_effect.app_star_speed = 100;
            }

            meteor_lights_set_speed(fc_effect.app_star_speed);
            printf("fc_effect.app_star_speed = %u\n", (u16)fc_effect.app_star_speed);
            printf("fc_effect.star_speed = %u\n", (u16)fc_effect.star_speed);
            fd_meteor_speed(); // 向app反馈流星灯速度
        }
        else if (fc_effect.star_index >= 15 && fc_effect.star_index <= 16)
        {
            // 如果在声控模式，调节流星灯声控模式的灵敏度
            meteor_lights_sound_sensitivity_add();
            printf("fc_effect.meteor_lights_sensitivity = %u\n", (u16)fc_effect.meteor_lights_sensitivity);
            fc_effect.music.s = fc_effect.meteor_lights_sensitivity; // 存放流星灯的灵敏度，准备发送给app
            fb_sensitive();
        }

        ls_meteor_stat_effect(); // 根据索引值，设置流星灯模式
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R6C4_CLICK:
    case RF_433_KEY_EVENT_R6C4_LONG:
    {
        if (fc_effect.star_on_off == DEVICE_OFF)
        {
            // 如果流星灯没有启动，不做处理
            return;
        }

        if (fc_effect.star_index >= 1 && fc_effect.star_index <= 14)
        {
            // 如果不在声控模式，调节流星灯速度
            // 流星灯 速度减
            const u8 step = 10;
            if (fc_effect.app_star_speed > 0 + step)
            {
                fc_effect.app_star_speed -= step;
            }
            else
            {
                fc_effect.app_star_speed = 0;
            }

            meteor_lights_set_speed(fc_effect.app_star_speed);
            printf("fc_effect.app_star_speed = %u\n", (u16)fc_effect.app_star_speed);
            printf("fc_effect.star_speed = %u\n", (u16)fc_effect.star_speed);
            fd_meteor_speed(); // 向app反馈流星灯速度
        }
        else if (fc_effect.star_index >= 15 && fc_effect.star_index <= 16)
        {
            // 如果在声控模式，调节流星灯声控模式的灵敏度
            meteor_lights_sound_sensitivity_sub();
            printf("fc_effect.meteor_lights_sensitivity = %u\n", (u16)fc_effect.meteor_lights_sensitivity);
            fc_effect.music.s = fc_effect.meteor_lights_sensitivity; // 存放流星灯的灵敏度，准备发送给app
            fb_sensitive();
        }

        ls_meteor_stat_effect(); // 根据索引值，设置流星灯模式
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R7C1_CLICK:
    case RF_433_KEY_EVENT_R7C1_LONG:
    {
        if (fc_effect.on_off_flag == DEVICE_OFF)
        {
            // 如果七彩灯没有打开，不打开电机，直接返回
            return;
        }

        // 电机开
        motor_open();
        fb_motor_mode(); // 向app反馈电机的状态
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R7C2_CLICK:
    case RF_433_KEY_EVENT_R7C2_LONG:
    {
        // 声控模式
        // ls_set_music_mode();

        // 电机关
        motor_close();
        fb_motor_mode(); // 向app反馈电机的状态
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R7C3_CLICK:
    case RF_433_KEY_EVENT_R7C3_LONG:
    {
        // 电机速度加
        if (DEVICE_OFF == fc_effect.motor_on_off)
        {
            // 电机没有启动，不调节电机转速
            return;
        }

        // 判断电机是否处于普通模式（非声控模式）
        if (MOTOR_MODE_MUSIC_RULATION != fc_effect.base_ins.mode)
        {
            if (fc_effect.motor_speed_index > 0)
            {
                fc_effect.motor_speed_index--;
            }

            fc_effect.base_ins.period = motor_period[fc_effect.motor_speed_index];
#if USER_DEBUG_ENABLE
            // printf("motor speed index %u\n", (u16)fc_effect.motor_speed_index);
#endif

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
            }
            else
            {
                motor_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
            }

            os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
            fb_motor_speed(); // 向app反馈电机的转速
        }
        else
        {
            // 如果电机处于声控模式，调节灵敏度
            // motor_sound_sensitivity_sub();
            motor_sound_sensitivity_add();
            fc_effect.music.s = fc_effect.base_ins.sensitivity; // 存放要反馈给app的灵敏度
            fb_sensitive();                                     // 向app反馈灵敏度
        }
    }
    break;
        // ==============================================================================
    case RF_433_KEY_EVENT_R7C4_CLICK:
    case RF_433_KEY_EVENT_R7C4_LONG:
    {
        // 电机速度减
        if (DEVICE_OFF == fc_effect.motor_on_off)
        {
            // 电机没有启动，不调节电机转速
            return;
        }

        // 判断电机是否处于普通模式（非声控模式）
        if (MOTOR_MODE_MUSIC_RULATION != fc_effect.base_ins.mode)
        {
            if (fc_effect.motor_speed_index < ARRAY_SIZE(motor_period) - 1)
            {
                fc_effect.motor_speed_index++; // 索引值越大，对应的电机转速就越慢
            }

            fc_effect.base_ins.period = motor_period[fc_effect.motor_speed_index];
#if USER_DEBUG_ENABLE
            // printf("motor speed index %u\n", (u16)fc_effect.motor_speed_index);
#endif

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
            }
            else
            {
                motor_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
            }

            motor_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
            os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
            fb_motor_speed(); // 向app反馈电机的转速
        }
        else
        {
            // 如果电机处于声控模式，调节灵敏度
            motor_sound_sensitivity_sub();
            fc_effect.music.s = fc_effect.base_ins.sensitivity; // 存放要反馈给app的灵敏度
            fb_sensitive();                                     // 向app反馈灵敏度
        }
    }
    break;
        // ==============================================================================
    default:
    {           // 如果不是rf433遥控器的按键事件
        return; // 函数返回，不执行接下来的步骤
    }
    break;

    } // switch (rf_433_key_event)

    os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
}

#endif // #if RF_433_KEY_ENABLE
