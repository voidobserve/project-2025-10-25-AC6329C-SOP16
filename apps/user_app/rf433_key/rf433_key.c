#include "rf433_key.h"
#include "rf433_learn.h"

#include "../../../apps/user_app/led_strip/led_strip_sys.h"
#include "../../../apps/user_app/led_strip/led_strand_effect.h"

#include "../../../apps/user_app/save_flash/save_flash.h"
#include "../../../apps/user_app/ws2812-fx-lib/WS2812FX_C/ws2812fx_effect.h"

#if RF_433_KEY_ENABLE

static volatile u32 rf_data = 0;              // 定时器中断使用的接收缓冲区，避免直接覆盖全局的数据接收缓冲区
volatile u32 recv_rf_433_data = 0;            // 存放在中断接收完成的rf433数据
volatile u8 flag_is_received_rf_433_data = 0; // 标志位，是否接收到了433数据

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
#if RF_433_LEARN_ENABLE
        recv_rf_433_addr = recv_rf_433_data >> 8; // 存放接收到的遥控器的地址；客户给到的遥控器，后面8位是键值，前面16位是地址
#endif
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

#if 1
#if RF_433_LEARN_ENABLE
    u32 rf_433_addr = rf_433_addr_get();
    u32 cur_rf_433_addr = recv_rf_433_addr;
    if (rf_433_addr != cur_rf_433_addr)
    {
        // 学习/对码 之后的地址与当前接收到的地址不一致，直接返回，不处理事件
        // rf_433_key_structure.rf_433_key_latest_key_val = NO_KEY;
        // rf_433_key_structure.rf_433_key_driver_event = RF_433_KEY_EVENT_NONE;

        // printf("recv_rf_433_addr != cur_rf_433_addr\n");
        return;
    }
#endif // #if RF_433_LEARN_ENABLE
#endif

    rf_433_key_event = rf_433_key_get_keyevent(rf_433_key_structure.rf_433_key_latest_key_val, rf_433_key_structure.rf_433_key_driver_event);
    rf_433_key_structure.rf_433_key_latest_key_val = NO_KEY;
    rf_433_key_structure.rf_433_key_driver_event = RF_433_KEY_EVENT_NONE;

    if (DEVICE_OFF == get_on_off_state())
    { // 如果设备没有启动
        // 只对开关按键做处理
        if (RF_433_KEY_EVENT_R1C4_CLICK == rf_433_key_event ||
            RF_433_KEY_EVENT_R1C4_LONG == rf_433_key_event)
        {
            soft_turn_on_the_light(); // 打开设备
            // save_user_data_area3();
            os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
        }

        return;
    }

    // 执行到这里，说明设备已经启动

    switch (rf_433_key_event)
    {
    case RF_433_KEY_EVENT_R1C1_CLICK:
    case RF_433_KEY_EVENT_R1C1_LONG: // 亮度加
        // case RF_433_KEY_EVENT_R1C1_HOLD:
        {
            // // 在动态模式下，是增加速度；在声控模式下，是加灵敏度
            // if (IS_STATIC == fc_effect.Now_state)
            // {
            //     ls_add_bright();
            // }
            // else if (IS_light_scene == fc_effect.Now_state)
            // {
            //     ls_add_speed();
            // }
            // else if (IS_light_music == fc_effect.Now_state)
            // {
            //     ls_add_sensitive();
            //     fb_sensitive(); // 向app反馈灵敏度
            // }

            if (IS_STATIC == fc_effect.Now_state)
            {
                /*
                    单色模式下，调节亮度
                    只修改七彩灯的亮度，不能通过 WS2812FX_setBrightness() 函数调节亮度，
                    这样会连流星灯的亮度也修改
                */
                // if (fc_effect.ls_b < (MAX_BRIGHT_RANK - 1))
                // {
                //     fc_effect.ls_b++;
                // }

                // fc_effect.app_b = (fc_effect.ls_b + 1) * 10;
                // fc_effect.b = led_b_array[fc_effect.ls_b];

                // printf("fc_effect.ls_b %u\n", (u16)fc_effect.ls_b);
                // printf("fc_effect.app_b %u\n", (u16)fc_effect.app_b);
                printf("fc_effect.b %u\n", (u16)fc_effect.b);
                fb_bright();
            }
#if 0
            else if (IS_light_scene == fc_effect.Now_state && // 七彩灯的动态模式
                     (MODO_COLORFUL_LIGHTS_FLASH == fc_effect.dream_scene.change_type ||
                      MODE_COLORFUL_LIGHTS_BREATH == fc_effect.dream_scene.change_type ||
                      MODE_COLORFUL_LIGHTS_GRADUAL == fc_effect.dream_scene.change_type ||
                      MODE_COLORFUL_LIGHTS_JUMP == fc_effect.dream_scene.change_type ||
                      MODE_COLORFUL_LIGHTS_AUTO == fc_effect.dream_scene.change_type))
            {
                // 七彩灯动态模式下，调节速度
                // 注意：速度值是越小越快
                // 得到速度等级：
                if (fc_effect.ls_speed > 0)
                {
                    fc_effect.ls_speed--;
                }

                fc_effect.dream_scene.speed = colorful_lights_speed_array[fc_effect.ls_speed]; // 根据速度等级进行查表，得到速度值
                fc_effect.app_speed = 100 - (fc_effect.ls_speed) * 10;                         // 根据速度等级得到要反馈给app的速度值
                fb_speed();                                                                    // 给app反馈速度值
                WS2812FX_setSpeed_seg(0, fc_effect.dream_scene.speed);                         // 通过库提供的接口修改速度，而不是修改速度后再调用一次动画函数
                printf("fc_effect.dream_scene.speed %u\n", (u16)fc_effect.dream_scene.speed);
            }
#endif
            else if (IS_light_music == fc_effect.Now_state)
            {
                // 七彩灯声控模式下，调节灵敏度
                // colorful_lights_sound_sensitivity_add();

                // fb_sensitive(); // 向app反馈灵敏度
            }
            else
            {
                // 其他模式，直接退出，不执行后续的读写flash操作
                return;
            }
        }
        break;

    case RF_433_KEY_EVENT_R1C2_CLICK:
    case RF_433_KEY_EVENT_R1C2_LONG: // 亮度减
    case RF_433_KEY_EVENT_R1C2_HOLD:
    {
        // 在动态模式下，是减慢速度；在声控模式下，是减灵敏度
        if (IS_STATIC == fc_effect.Now_state)
        {
            ls_sub_bright();
        }
        else if (IS_light_scene == fc_effect.Now_state)
        {
            ls_sub_speed();
        }
        else if (IS_light_music == fc_effect.Now_state)
        {
            ls_sub_sensitive();
            fb_sensitive(); // 向app反馈灵敏度
        }
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
        if (DEVICE_ON == fc_effect.on_off_flag)
        {
            // 如果已经开机，不响应该按键事件
            return;
        }

        soft_turn_on_the_light(); // 打开设备
    }
    break;

    case RF_433_KEY_EVENT_R2C1_CLICK:
    case RF_433_KEY_EVENT_R2C1_LONG:
    {
        // RED
        color_t color_structure = {0};
        color_structure.r = 0xFF;
        color_structure.g = 0x00;
        color_structure.b = 0x00;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R2C2_CLICK:
    case RF_433_KEY_EVENT_R2C2_LONG:
    {
        // GREEN
        color_t color_structure = {0};
        color_structure.r = 0x00;
        color_structure.g = 0xFF;
        color_structure.b = 0x00;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R2C3_CLICK:
    case RF_433_KEY_EVENT_R2C3_LONG:
    {

        // BLUE
        color_t color_structure = {0};
        color_structure.r = 0x00;
        color_structure.g = 0x00;
        color_structure.b = 0xFF;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R2C4_CLICK:
    case RF_433_KEY_EVENT_R2C4_LONG:
    {
        // 纯白色
        color_t color_structure = {0};
        color_structure.r = 0x00;
        color_structure.g = 0x00;
        color_structure.b = 0x00;
        color_structure.w = 0xFF;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R3C1_CLICK:
    case RF_433_KEY_EVENT_R3C1_LONG: //
    {
        color_t color_structure = {0};
        color_structure.r = 0xFF;
        color_structure.g = 0xFF / 2;
        color_structure.b = 0x00;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R3C2_CLICK:
    case RF_433_KEY_EVENT_R3C2_LONG: //
    {
        // 绿100% 红10%
        color_t color_structure = {0};
        color_structure.r = (u8)((u16)0xFF * 10 / 100); // 10%分量
        color_structure.g = 0xFF / 2;
        color_structure.b = 0x00;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R3C3_CLICK:
    case RF_433_KEY_EVENT_R3C3_LONG: //
    {
        // 绿100% 蓝100%
        color_t color_structure = {0};
        color_structure.r = 0x00;
        color_structure.g = 0xFF;
        color_structure.b = 0xFF;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R3C4_CLICK:
    case RF_433_KEY_EVENT_R3C4_LONG:
    {
        // 混白色
        color_t color_structure = {0};
        color_structure.r = 0xFF;
        color_structure.g = 0xFF;
        color_structure.b = 0xFF;
        color_structure.w = 0xFF;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R4C1_CLICK:
    case RF_433_KEY_EVENT_R4C1_LONG: //
    {
        // 红100% 绿100%
        color_t color_structure = {0};
        color_structure.r = 0xFF;
        color_structure.g = 0xFF;
        color_structure.b = 0x00;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R4C2_CLICK:
    case RF_433_KEY_EVENT_R4C2_LONG: //
    {
        // 绿 100% 红 20% 蓝 20%
        color_t color_structure = {0};
        color_structure.r = (u8)((u16)0xFF * 20 / 100); //  %分量
        color_structure.g = 0xFF;
        color_structure.b = (u8)((u16)0xFF * 20 / 100); //  %分量
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R4C3_CLICK:
    case RF_433_KEY_EVENT_R4C3_LONG: //
    {
        // 绿100% 蓝10%
        color_t color_structure = {0};
        color_structure.r = 0x00;
        color_structure.g = 0xFF;
        color_structure.b = (u8)((u16)0xFF * 10 / 100); // 10%分量
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R4C4_CLICK:
    case RF_433_KEY_EVENT_R4C4_LONG:
    {
        // 电机开关

        u8 motor_mode = 0x00; // 默认是关机
        if (fc_effect.motor_on_off == DEVICE_ON)
        {
            motor_mode = 0x06;                                      // 关机命令
            fc_effect.motor_speed_index = ARRAY_SIZE(motor_period); // 让索引值超出数组的索引范围，表示关闭电机，下一次重新上电让电机默认关闭
            fc_effect.motor_on_off = DEVICE_OFF;
        }
        else
        {
            motor_mode = 0x04; // 控制命令 -- 360度旋转
            fc_effect.motor_on_off = DEVICE_ON;
        }

        one_wire_set_mode(motor_mode); // 配置电机模式
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);

        fb_motor_mode(); // 向app反馈电机的状态
    }
    break;

    case RF_433_KEY_EVENT_R5C1_CLICK:
    case RF_433_KEY_EVENT_R5C1_LONG:
    {
        // 红100% 蓝50%
        color_t color_structure = {0};
        color_structure.r = 0xFF;
        color_structure.g = 0x00;
        color_structure.b = 0xFF / 2; // 50% 分量
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R5C2_CLICK:
    case RF_433_KEY_EVENT_R5C2_LONG:
    {
        // 红100% 蓝100%
        color_t color_structure = {0};
        color_structure.r = 0xFF;
        color_structure.g = 0x00;
        color_structure.b = 0xFF;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R5C3_CLICK:
    case RF_433_KEY_EVENT_R5C3_LONG:
    {
        // 红20% 蓝100%
        color_t color_structure = {0};
        color_structure.r = (u8)((u16)0xFF * 20 / 100); // 20% 分量
        color_structure.g = 0x00;
        color_structure.b = 0xFF;
        color_structure.w = 0x00;
        colorful_lights_set_static_mode(color_structure);
    }
    break;

    case RF_433_KEY_EVENT_R5C4_CLICK:
    case RF_433_KEY_EVENT_R5C4_LONG:
    {
        // 电机模式切换
        // 1.匀速转，2，正转+反转，3，带暂停的转动，4，速度可变模式，5，音频节奏转动
        // USER_TO_DO 可能要先套用现有的模式
        // 测试发现电机一直是同一个方向，没有切换模式

        if (DEVICE_OFF == fc_effect.motor_on_off)
        {
            // 电机没有启动，不调节电机模式
            return;
        }

        fc_effect.base_ins.mode = (fc_effect.base_ins.mode + 1) % 6;

        // 没有观察到电机反转
        // if (fc_effect.base_ins.mode == 2)
        // {
        //     fc_effect.base_ins.dir = 1;
        // }
        // else
        // {
        //     fc_effect.base_ins.dir = 0;
        // }

        printf("motor mode %u \n", (u16)fc_effect.base_ins.mode);
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
    }
    break;

    case RF_433_KEY_EVENT_R6C1_CLICK:
    case RF_433_KEY_EVENT_R6C1_LONG:
    {
        // 七彩频闪 FLASH
        if (IS_light_scene == fc_effect.Now_state &&
            MODO_COLORFUL_LIGHTS_FLASH == fc_effect.dream_scene.change_type &&
            7 == fc_effect.dream_scene.c_n)
        {
            // 如果现在就是七彩灯的频闪模式，不做处理
            return;
        }

        ls_set_color(0, BLUE);
        ls_set_color(1, GREEN);
        ls_set_color(2, RED);
        ls_set_color(3, WHITE);
        ls_set_color(4, YELLOW);
        ls_set_color(5, CYAN);
        ls_set_color(6, PURPLE);

        if ((fc_effect.dream_scene.change_type != MODO_COLORFUL_LIGHTS_FLASH) ||
            (fc_effect.Now_state != IS_light_scene))
        {
            /*
                如果之前不是七彩灯的跳变模式，
                清空灯光动画运行时使用的数据，让动画重新开始跑
            */
            WS2812FX_resetSegmentRuntime(0); //
        }

        fc_effect.dream_scene.change_type = MODO_COLORFUL_LIGHTS_FLASH; //
        fc_effect.dream_scene.c_n = 7;                                  // 有效颜色数量
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
    }
    break;

    case RF_433_KEY_EVENT_R6C2_CLICK:
    case RF_433_KEY_EVENT_R6C2_LONG:
    {
        // 七色跳变 JUMP
        if (IS_light_scene == fc_effect.Now_state &&
            MODE_COLORFUL_LIGHTS_JUMP == fc_effect.dream_scene.change_type)
        {
            // 如果本来就是七彩灯的跳变模式，不做处理 （目前只会是遥控器控制会进入这里）
            return;
        }

        ls_set_color(0, RED);
        ls_set_color(1, GREEN);
        ls_set_color(2, BLUE);
        ls_set_color(3, YELLOW);
        ls_set_color(4, CYAN);
        ls_set_color(5, PURPLE);
        ls_set_color(6, WHITE);

        if ((fc_effect.dream_scene.change_type != MODE_COLORFUL_LIGHTS_JUMP) ||
            (fc_effect.Now_state != IS_light_scene))
        {
            /*
                如果之前不是七彩灯的跳变模式，
                清空灯光动画运行时使用的数据，让动画重新开始跑
            */
            WS2812FX_resetSegmentRuntime(0); //
        }

        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_JUMP; //
        fc_effect.dream_scene.c_n = 7;                                 // 有效颜色数量
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
    }
    break;

    case RF_433_KEY_EVENT_R6C3_CLICK:
    case RF_433_KEY_EVENT_R6C3_LONG:
    {
        // 七彩渐变 FADE
        if (IS_light_scene == fc_effect.Now_state &&
            MODE_COLORFUL_LIGHTS_GRADUAL == fc_effect.dream_scene.change_type)
        {
            // 如果当前是七彩灯渐变模式，不做处理
            return;
        }

        if ((fc_effect.dream_scene.change_type != MODE_COLORFUL_LIGHTS_GRADUAL) ||
            (fc_effect.Now_state != IS_light_scene))
        {
            /*
                如果之前不是七彩灯的渐变模式，
                清空灯光动画运行时使用的数据，让动画重新开始跑
            */
            WS2812FX_resetSegmentRuntime(0); //
        }

        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_GRADUAL;
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
    }
    break;

    case RF_433_KEY_EVENT_R6C4_CLICK:
    case RF_433_KEY_EVENT_R6C4_LONG:
    {
        /*
            电机参数 +

            如果在一般的电机模式，控制电机转速
            如果在声控的电机的模式，控制电机灵敏度
        */
    }
    break;

    case RF_433_KEY_EVENT_R7C1_CLICK:
    case RF_433_KEY_EVENT_R7C1_LONG:
    {
        // AUTO

        if (IS_light_scene == fc_effect.Now_state &&
            MODE_COLORFUL_LIGHTS_AUTO == fc_effect.dream_scene.change_type)
        {
            // 如果之前是七彩灯的AUTO模式，则不做处理
            return;
        }

        if ((fc_effect.dream_scene.change_type != MODE_COLORFUL_LIGHTS_AUTO) ||
            (fc_effect.Now_state != IS_light_scene))
        {
            /*
                如果之前不是七彩灯的自动模式，
                清空灯光动画运行时使用的数据，让动画重新开始跑
            */
            WS2812FX_resetSegmentRuntime(0); //
        }

        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_AUTO;
        fc_effect.Now_state = IS_light_scene;
        set_fc_effect();
    }
    break;

    case RF_433_KEY_EVENT_R7C2_CLICK:
    case RF_433_KEY_EVENT_R7C2_LONG:
    {
        // 声控模式

        // USER_TO_DO 这里面可能没有用最大的亮度 
        ls_set_music_mode();
    }
    break;

    case RF_433_KEY_EVENT_R7C3_CLICK:
    case RF_433_KEY_EVENT_R7C3_LONG:
    {
        // 呼吸模式，1，先按单色键，再按呼吸键，则为单色呼吸，2，先按变色键，再按呼吸，则为变色呼吸

        u8 color_nums = 0; // 存放颜色数量

        if (IS_light_scene == fc_effect.Now_state &&
            MODE_COLORFUL_LIGHTS_BREATH == fc_effect.dream_scene.change_type &&
            7 == fc_effect.dream_scene.c_n)
        { // 如果本来就是呼吸模式，不响应该事件
          // printf("__LINE__ %d\n", __LINE__);
            return;
        }
        else if (IS_STATIC == fc_effect.Now_state)
        {
            /*
                如果是从静态模式进入呼吸模式
                变成单色呼吸
            */
            color_nums = 1; // 颜色数量 -- 只有1个颜色
            fc_effect.dream_scene.rgb[0].r = fc_effect.rgb.r;
            fc_effect.dream_scene.rgb[0].g = fc_effect.rgb.g;
            fc_effect.dream_scene.rgb[0].b = fc_effect.rgb.b;
            fc_effect.dream_scene.rgb[0].w = fc_effect.rgb.w;
            // printf("__LINE__ %d\n", __LINE__);
        }
        else
        {
            /*
                如果不是从静态模式进入呼吸模式，
                变成变色呼吸（每轮呼吸完换一种颜色）
            */
            ls_set_color(color_nums++, BLUE);
            ls_set_color(color_nums++, GREEN);
            ls_set_color(color_nums++, RED);
            ls_set_color(color_nums++, WHITE);
            ls_set_color(color_nums++, YELLOW);
            ls_set_color(color_nums++, CYAN);
            ls_set_color(color_nums++, PURPLE);
        }

        if ((fc_effect.dream_scene.change_type != MODE_COLORFUL_LIGHTS_BREATH) ||
            (fc_effect.Now_state != IS_light_scene))
        {
            /*
                如果之前不是七彩灯的呼吸模式，
                清空灯光动画运行时使用的数据，让动画重新开始跑
            */
            WS2812FX_resetSegmentRuntime(0); //
            // printf("__LINE__ %d\n", __LINE__);
        }

        fc_effect.dream_scene.change_type = MODE_COLORFUL_LIGHTS_BREATH;
        fc_effect.dream_scene.c_n = color_nums; // 颜色数量
        fc_effect.Now_state = IS_light_scene;
        // printf("color_nums = %u\n", (u16)color_nums);
        set_fc_effect();
    }
    break;

    case RF_433_KEY_EVENT_R7C4_CLICK:
    case RF_433_KEY_EVENT_R7C4_LONG:
    {
        /*
            电机参数 -

            如果在普通电机模式，控制电机转速
            如果在声控电机模式，控制电机灵敏度
        */
    }
    break;

    default:
    {           // 如果不是rf433遥控器的按键事件
        return; // 函数返回，不执行接下来的步骤
    }
    break;

    } // switch (rf_433_key_event)

    // save_user_data_area3(); // 执行完对应的操作后，保存数据到flash
    os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
}

#endif // #if RF_433_KEY_ENABLE
