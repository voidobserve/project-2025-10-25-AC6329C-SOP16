#include "rf433_key.h"

#include "../../../apps/user_app/led_strip/led_strip_sys.h"
#include "../../../apps/user_app/led_strip/led_strand_effect.h"

#if RF_433_KEY_ENABLE

static volatile u32 rf_data = 0;                     // 定时器中断使用的接收缓冲区，避免直接覆盖全局的数据接收缓冲区
static volatile u32 recv_rf_433_data = 0;            // 存放在中断接收完成的rf433数据
static volatile u8 flag_is_received_rf_433_data = 0; // 标志位，是否接收到了433数据

void rf_433_key_config(void)
{
    gpio_set_pull_up(RF_433_KEY_SCAN_PIN, 1);   // 上拉
    gpio_set_pull_down(RF_433_KEY_SCAN_PIN, 0); // 不下拉

    gpio_set_die(RF_433_KEY_SCAN_PIN, 1);       // 普通输入模式
    gpio_set_direction(RF_433_KEY_SCAN_PIN, 1); // 输入模式
}

static u8 rf_433_key_get_value(void);
rf_433_key_struct_t rf_433_key_structure = {
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
        time_out_cnt = RF_433_KEY_SCAN_EFFECTIVE_TIME_OUT / RF_433_KEY_SCAN_CIRCLE_TIMES; //
        ret = (u8)(recv_rf_433_data & 0xFF);

        // printf("get key val 0x %lx \n", recv_rf_433_data);
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
        break;
    case KEY_EVENT_LONG:
        /* 长按，在数组 rf_433_key_event_table 的[x][2]位置 */
        rf_433_key_event_table_index = 2;
        break;
    case KEY_EVENT_HOLD:
        /* 长按持续（不松手），在数组 rf_433_key_event_table 的[x][3]位置 */
        rf_433_key_event_table_index = 3;
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

    rf_433_key_event = rf_433_key_get_keyevent(rf_433_key_structure.rf_433_key_latest_key_val, rf_433_key_structure.rf_433_key_driver_event);
    rf_433_key_structure.rf_433_key_latest_key_val = NO_KEY;
    rf_433_key_structure.rf_433_key_driver_event = RF_433_KEY_EVENT_NONE;

    switch (rf_433_key_event)
    {
    case RF_433_KEY_EVENT_R1C1_CLICK:
    case RF_433_KEY_EVENT_R1C1_LONG: // 亮度加
    {
    }
    break;

    case RF_433_KEY_EVENT_R1C2_CLICK:
    case RF_433_KEY_EVENT_R1C2_LONG: // 亮度减
    {
    }
    break;

    case RF_433_KEY_EVENT_R1C3_CLICK:
    case RF_433_KEY_EVENT_R1C3_LONG: // OFF
    {
        soft_turn_off_lights();
    }
    break;

    case RF_433_KEY_EVENT_R1C4_CLICK:
    case RF_433_KEY_EVENT_R1C4_LONG: // ON
    {
        soft_turn_on_the_light();
    }
    break;

    case RF_433_KEY_EVENT_R2C1_CLICK:
    case RF_433_KEY_EVENT_R2C1_LONG: // RED
    {
    }
    break;

    case RF_433_KEY_EVENT_R2C2_CLICK:
    case RF_433_KEY_EVENT_R2C2_LONG: // GREEN
    {
    }
    break;

    case RF_433_KEY_EVENT_R2C3_CLICK:
    case RF_433_KEY_EVENT_R2C3_LONG: // BLUE
    {
    }
    break;

    case RF_433_KEY_EVENT_R2C4_CLICK:
    case RF_433_KEY_EVENT_R2C4_LONG: // WHITE
    {
        fc_effect.Now_state = IS_STATIC;
        fc_effect.rgb.r = 0xFF;
        fc_effect.rgb.g = 0xFF;
        fc_effect.rgb.b = 0xFF;
        fc_effect.rgb.w = 0xFF;
        set_fc_effect(); // 效果调度
    }

    } // switch (rf_433_key_event)
}

#endif // #if RF_433_KEY_ENABLE
