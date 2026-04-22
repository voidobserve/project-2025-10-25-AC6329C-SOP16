#include "rf24g_key.h"
#include "led_strand_effect.h"
#include "save_flash.h" // 包含读写flash的接口
#include "ws2812fx_effect.h"

#include "user_include.h"

#if (RF24GKEY_ENABLE)

/*
    用 AK803-SOP16 写的遥控器
*/
const u8 rf24g_key_type_28keys_table[][RF34G_KEY_EVENT_MAX + 1] = {
    {RF24G_KEY_VAL_R1C1, RF24G_28_KEY_EVENT_R1C1_CLICK, RF24G_28_KEY_EVENT_R1C1_LONG, RF24G_28_KEY_EVENT_R1C1_HOLD, RF24G_28_KEY_EVENT_R1C1_LOOSE},
    {RF24G_KEY_VAL_R1C2, RF24G_28_KEY_EVENT_R1C2_CLICK, RF24G_28_KEY_EVENT_R1C2_LONG, RF24G_28_KEY_EVENT_R1C2_HOLD, RF24G_28_KEY_EVENT_R1C2_LOOSE},
    {RF24G_KEY_VAL_R1C3, RF24G_28_KEY_EVENT_R1C3_CLICK, RF24G_28_KEY_EVENT_R1C3_LONG, RF24G_28_KEY_EVENT_R1C3_HOLD, RF24G_28_KEY_EVENT_R1C3_LOOSE},
    {RF24G_KEY_VAL_R1C4, RF24G_28_KEY_EVENT_R1C4_CLICK, RF24G_28_KEY_EVENT_R1C4_LONG, RF24G_28_KEY_EVENT_R1C4_HOLD, RF24G_28_KEY_EVENT_R1C4_LOOSE},

    {RF24G_KEY_VAL_R2C1, RF24G_28_KEY_EVENT_R2C1_CLICK, RF24G_28_KEY_EVENT_R2C1_LONG, RF24G_28_KEY_EVENT_R2C1_HOLD, RF24G_28_KEY_EVENT_R2C1_LOOSE},
    {RF24G_KEY_VAL_R2C2, RF24G_28_KEY_EVENT_R2C2_CLICK, RF24G_28_KEY_EVENT_R2C2_LONG, RF24G_28_KEY_EVENT_R2C2_HOLD, RF24G_28_KEY_EVENT_R2C2_LOOSE},
    {RF24G_KEY_VAL_R2C3, RF24G_28_KEY_EVENT_R2C3_CLICK, RF24G_28_KEY_EVENT_R2C3_LONG, RF24G_28_KEY_EVENT_R2C3_HOLD, RF24G_28_KEY_EVENT_R2C3_LOOSE},
    {RF24G_KEY_VAL_R2C4, RF24G_28_KEY_EVENT_R2C4_CLICK, RF24G_28_KEY_EVENT_R2C4_LONG, RF24G_28_KEY_EVENT_R2C4_HOLD, RF24G_28_KEY_EVENT_R2C4_LOOSE},

    {RF24G_KEY_VAL_R3C1, RF24G_28_KEY_EVENT_R3C1_CLICK, RF24G_28_KEY_EVENT_R3C1_LONG, RF24G_28_KEY_EVENT_R3C1_HOLD, RF24G_28_KEY_EVENT_R3C1_LOOSE},
    {RF24G_KEY_VAL_R3C2, RF24G_28_KEY_EVENT_R3C2_CLICK, RF24G_28_KEY_EVENT_R3C2_LONG, RF24G_28_KEY_EVENT_R3C2_HOLD, RF24G_28_KEY_EVENT_R3C2_LOOSE},
    {RF24G_KEY_VAL_R3C3, RF24G_28_KEY_EVENT_R3C3_CLICK, RF24G_28_KEY_EVENT_R3C3_LONG, RF24G_28_KEY_EVENT_R3C3_HOLD, RF24G_28_KEY_EVENT_R3C3_LOOSE},
    {RF24G_KEY_VAL_R3C4, RF24G_28_KEY_EVENT_R3C4_CLICK, RF24G_28_KEY_EVENT_R3C4_LONG, RF24G_28_KEY_EVENT_R3C4_HOLD, RF24G_28_KEY_EVENT_R3C4_LOOSE},

    {RF24G_KEY_VAL_R4C1, RF24G_28_KEY_EVENT_R4C1_CLICK, RF24G_28_KEY_EVENT_R4C1_LONG, RF24G_28_KEY_EVENT_R4C1_HOLD, RF24G_28_KEY_EVENT_R4C1_LOOSE},
    {RF24G_KEY_VAL_R4C2, RF24G_28_KEY_EVENT_R4C2_CLICK, RF24G_28_KEY_EVENT_R4C2_LONG, RF24G_28_KEY_EVENT_R4C2_HOLD, RF24G_28_KEY_EVENT_R4C2_LOOSE},
    {RF24G_KEY_VAL_R4C3, RF24G_28_KEY_EVENT_R4C3_CLICK, RF24G_28_KEY_EVENT_R4C3_LONG, RF24G_28_KEY_EVENT_R4C3_HOLD, RF24G_28_KEY_EVENT_R4C3_LOOSE},
    {RF24G_KEY_VAL_R4C4, RF24G_28_KEY_EVENT_R4C4_CLICK, RF24G_28_KEY_EVENT_R4C4_LONG, RF24G_28_KEY_EVENT_R4C4_HOLD, RF24G_28_KEY_EVENT_R4C4_LOOSE},

    {RF24G_KEY_VAL_R5C1, RF24G_28_KEY_EVENT_R5C1_CLICK, RF24G_28_KEY_EVENT_R5C1_LONG, RF24G_28_KEY_EVENT_R5C1_HOLD, RF24G_28_KEY_EVENT_R5C1_LOOSE},
    {RF24G_KEY_VAL_R5C2, RF24G_28_KEY_EVENT_R5C2_CLICK, RF24G_28_KEY_EVENT_R5C2_LONG, RF24G_28_KEY_EVENT_R5C2_HOLD, RF24G_28_KEY_EVENT_R5C2_LOOSE},
    {RF24G_KEY_VAL_R5C3, RF24G_28_KEY_EVENT_R5C3_CLICK, RF24G_28_KEY_EVENT_R5C3_LONG, RF24G_28_KEY_EVENT_R5C3_HOLD, RF24G_28_KEY_EVENT_R5C3_LOOSE},
    {RF24G_KEY_VAL_R5C4, RF24G_28_KEY_EVENT_R5C4_CLICK, RF24G_28_KEY_EVENT_R5C4_LONG, RF24G_28_KEY_EVENT_R5C4_HOLD, RF24G_28_KEY_EVENT_R5C4_LOOSE},

    {RF24G_KEY_VAL_R6C1, RF24G_28_KEY_EVENT_R6C1_CLICK, RF24G_28_KEY_EVENT_R6C1_LONG, RF24G_28_KEY_EVENT_R6C1_HOLD, RF24G_28_KEY_EVENT_R6C1_LOOSE},
    {RF24G_KEY_VAL_R6C2, RF24G_28_KEY_EVENT_R6C2_CLICK, RF24G_28_KEY_EVENT_R6C2_LONG, RF24G_28_KEY_EVENT_R6C2_HOLD, RF24G_28_KEY_EVENT_R6C2_LOOSE},
    {RF24G_KEY_VAL_R6C3, RF24G_28_KEY_EVENT_R6C3_CLICK, RF24G_28_KEY_EVENT_R6C3_LONG, RF24G_28_KEY_EVENT_R6C3_HOLD, RF24G_28_KEY_EVENT_R6C3_LOOSE},
    {RF24G_KEY_VAL_R6C4, RF24G_28_KEY_EVENT_R6C4_CLICK, RF24G_28_KEY_EVENT_R6C4_LONG, RF24G_28_KEY_EVENT_R6C4_HOLD, RF24G_28_KEY_EVENT_R6C4_LOOSE},

    {RF24G_KEY_VAL_R7C1, RF24G_28_KEY_EVENT_R7C1_CLICK, RF24G_28_KEY_EVENT_R7C1_LONG, RF24G_28_KEY_EVENT_R7C1_HOLD, RF24G_28_KEY_EVENT_R7C1_LOOSE},
    {RF24G_KEY_VAL_R7C2, RF24G_28_KEY_EVENT_R7C2_CLICK, RF24G_28_KEY_EVENT_R7C2_LONG, RF24G_28_KEY_EVENT_R7C2_HOLD, RF24G_28_KEY_EVENT_R7C2_LOOSE},
    {RF24G_KEY_VAL_R7C3, RF24G_28_KEY_EVENT_R7C3_CLICK, RF24G_28_KEY_EVENT_R7C3_LONG, RF24G_28_KEY_EVENT_R7C3_HOLD, RF24G_28_KEY_EVENT_R7C3_LOOSE},
    {RF24G_KEY_VAL_R7C4, RF24G_28_KEY_EVENT_R7C4_CLICK, RF24G_28_KEY_EVENT_R7C4_LONG, RF24G_28_KEY_EVENT_R7C4_HOLD, RF24G_28_KEY_EVENT_R7C4_LOOSE},
};

volatile u8 rf24g_key_driver_event = 0; // 由key_driver_scan() 更新
volatile u8 rf24g_key_driver_value = 0; // 由key_driver_scan() 更新

static volatile u8 rf24g_rx_flag = 0;       // 是否收到了新的数据
volatile rf24g_recv_info_t rf24g_recv_info; // 存放接收到的数据包
volatile u8 chromatic_circle_val = 0;       // 存放色环按键对应的数值，范围：0x00~0xFF
volatile u8 rf24g_key_val = NO_KEY;         // 存放按键键值

static u8 rf24g_get_key_value(void); // 获取按键键值的函数声明
volatile struct key_driver_para rf24g_scan_para = {
    .scan_time = RF24G_KEY_SCAN_TIME_MS,                                                     // 按键扫描频率, 单位: ms
    .last_key = NO_KEY,                                                                      // 上一次get_value按键值, 初始化为NO_KEY;
    .filter_time = RF24G_KEY_SCAN_FILTER_TIME_MS,                                            // 按键消抖延时;
    .long_time = RF24G_KEY_LONG_TIME_MS / RF24G_KEY_SCAN_TIME_MS,                            // 按键判定长按数量
    .hold_time = (RF24G_KEY_LONG_TIME_MS + RF24G_KEY_HOLD_TIME_MS) / RF24G_KEY_SCAN_TIME_MS, // 按键判定HOLD数量
    .click_delay_time = RF24G_KEY_SCAN_CLICK_DELAY_TIME_MS,                                  // 按键被抬起后等待连击延时数量
    .key_type = KEY_DRIVER_TYPE_RF24GKEY,
    .get_value = rf24g_get_key_value,
};

// 底层按键扫描，由 __resolve_adv_report() 调用
void rf24g_scan(u8 *recv_buff)
{
    rf24g_recv_info_t *p = (rf24g_recv_info_t *)recv_buff;
    if (p->header1 == REMOTE_TYPE_28KEY_HEADER_1 &&
        p->header2 == REMOTE_TYPE_28KEY_HEADER_2)
    {
#if USER_DEBUG_ENABLE
        // printf_buf(recv_buff, sizeof(rf24g_recv_info_t)); // 打印接收到的数据包
#endif

        rf24g_key_val = p->key_val;
        rf24g_rx_flag = 1;
    }
}

static u8 rf24g_get_key_value(void)
{
    u8 key_value = 0;
    static u16 time_out_cnt = 0;  // 加入超时，防止丢包（超时时间与按键扫描时间有关）
    static u8 last_key_value = 0; // 上一次按键键值，在超时时间内返回上一次按键键值

    if (rf24g_rx_flag == 1) // 收到2.4G广播
    {
        rf24g_rx_flag = 0;

        key_value = rf24g_key_val;

        /*
            2.4G接收可能会丢失100~200ms的数据包（响应会慢一些）
            值 == 20，10ms调用一次该函数，这里填充200ms的超时值
        */
        // time_out_cnt = 20;
        time_out_cnt = 5;

        last_key_value = key_value;
        return key_value;
    }

    if (time_out_cnt != 0)
    {
        time_out_cnt--;
        return last_key_value;
    }

    return NO_KEY;
}

// 根据按键键值和key_driver_scan得到的事件值，转换为对应的按键事件
u8 rf24g_convert_key_event(u8 key_value, u8 key_driver_event)
{
    // 将key_driver_scan得到的key_event转换成自定义的key_event对应的索引
    // 索引对应 rf24g_key_event_table[][] 中的索引
    u8 key_event_index = 0; // 默认为0，0对应无效索引
    if (KEY_EVENT_CLICK == key_driver_event)
    {
        key_event_index = 1;
    }
    else if (KEY_EVENT_LONG == key_driver_event)
    {
        // long和hold都当作hold处理
        key_event_index = 2;
    }
    else if (KEY_EVENT_HOLD == key_driver_event)
    {
        key_event_index = 3;
    }
    else if (KEY_EVENT_UP == key_driver_event)
    {
        // 长按后松手
        key_event_index = 4;
    }

    if (0 == key_event_index || NO_KEY == key_value)
    {
        // 按键事件与上面的事件都不匹配
        // 得到的键值是无效键值
        return RF24G_28_KEY_EVENT_NONE;
    }

    // 遍历表格中的每一个按键：
    for (u8 i = 0; i < sizeof(rf24g_key_type_28keys_table) / sizeof(rf24g_key_type_28keys_table[0]); i++)
    {
        if (key_value == rf24g_key_type_28keys_table[i][0])
        {
            return rf24g_key_type_28keys_table[i][key_event_index];
        }
    }

    // 如果运行到这里，都没有找到对应的按键，返回无效按键事件
    return RF24G_28_KEY_EVENT_NONE;
}

void rf24_key_handle(void)
{
    u8 rf24g_key_event = 0;
    rf24_key_handle_func_t rf24g_key_handle_func_ptr = NULL;

    rf24g_key_event = rf24g_convert_key_event(rf24g_key_driver_value, rf24g_key_driver_event);
    rf24g_key_driver_value = NO_KEY; // 置为无效键值（由于扫描函数只更新，不会清除，在这里要清除）

    if (rf24g_key_event == RF24G_28_KEY_EVENT_NONE)
    {
        // 如果是无效的按键事件，直接返回
        return;
    }

    rf24g_key_handle_func_ptr = rf24_28keys_handle_func_buff[rf24g_key_event];

    if (NULL == rf24g_key_handle_func_ptr)
    {
        // 如果按键事件没有对应的处理函数，直接退出
        return;
    }

    // 直接调用对应的处理函数，这样需要每个处理函数内都要判断一下设备是否开机
    rf24g_key_handle_func_ptr();

    os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
}

// =============================================================================

void rf24g_28keys_event_r1c1_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r1c1\n");
#endif

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
#if USER_DEBUG_ENABLE
        // printf("fc_effect.app_b %u\n", (u16)fc_effect.app_b);
        // printf("fc_effect.b %u\n", (u16)fc_effect.b);
#endif
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
#if USER_DEBUG_ENABLE
        // printf("fc_effect.app_speed %u\n", (u16)fc_effect.app_speed);
        // printf("fc_effect.dream_scene.speed %u\n", (u16)fc_effect.dream_scene.speed);
#endif

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

void rf24g_28keys_event_r1c2_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r1c2\n");
#endif

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
#if USER_DEBUG_ENABLE
        // printf("fc_effect.app_b %u\n", (u16)fc_effect.app_b);
        // printf("fc_effect.b %u\n", (u16)fc_effect.b);
#endif

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
#if USER_DEBUG_ENABLE
        // printf("fc_effect.app_speed %u\n", (u16)fc_effect.app_speed);
        // printf("fc_effect.dream_scene.speed %u\n", (u16)fc_effect.dream_scene.speed);
#endif

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

void rf24g_28keys_event_r1c3_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r1c3\n");
#endif

    // 只开 七彩灯 和 电机
    colorful_light_open();
    fb_led_on_off_state(); // 与app反馈七彩灯的开关状态

    motor_open();
    fb_motor_mode();  // 向app反馈电机的模式
    fb_motor_speed(); // 向app反馈电机转速
}

void rf24g_28keys_event_r1c4_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r1c4\n");
#endif

    // 只关 七彩灯 和 电机
    colorful_light_close();
    fb_led_on_off_state(); // 与app同步开关状态

    motor_close();
    fb_motor_mode();  // 向app反馈电机的模式
    fb_motor_speed(); // 向app反馈电机转速
}

void rf24g_28keys_event_r2c1_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r2c1\n");
#endif

    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        // 七彩灯没有打开，直接返回
        return;
    }

    colorful_lights_set_static_color(RED);
}

void rf24g_28keys_event_r2c2_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r2c2\n");
#endif

    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        // 七彩灯没有打开，直接返回
        return;
    }

    colorful_lights_set_static_color(GREEN);
}

void rf24g_28keys_event_r2c3_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r2c3\n");
#endif

    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        // 七彩灯没有打开，直接返回
        return;
    }

    colorful_lights_set_static_color(BLUE);
}

void rf24g_28keys_event_r2c4_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r2c4\n");
#endif

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

void rf24g_28keys_event_r3c1_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r3c1\n");
#endif

    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        // 七彩灯没有打开，直接返回
        return;
    }

    // 橙色
    colorful_lights_set_static_color(ORANGE);
}

void rf24g_28keys_event_r3c2_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r3c2\n");
#endif

    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        // 七彩灯没有打开，直接返回
        return;
    }

    // 黄色
    colorful_lights_set_static_color(YELLOW);
}

void rf24g_28keys_event_r3c3_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r3c3\n");
#endif

    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        // 七彩灯没有打开，直接返回
        return;
    }

    // CYAN 青色
    colorful_lights_set_static_color(CYAN);
}

void rf24g_28keys_event_r3c4_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r3c4\n");
#endif

    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        // 七彩灯没有打开，直接返回
        return;
    }

    // 紫色
    colorful_lights_set_static_color(PURPLE);
}

void rf24g_28keys_event_r4c1_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r4c1\n");
#endif

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

void rf24g_28keys_event_r4c2_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r4c2\n");
#endif

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

void rf24g_28keys_event_r4c3_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r4c3\n");
#endif

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

void rf24g_28keys_event_r4c4_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r4c4\n");
#endif

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

void rf24g_28keys_event_r5c1_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r5c1\n");
#endif

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

void rf24g_28keys_event_r5c2_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r5c2\n");
#endif

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

void rf24g_28keys_event_r5c3_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r5c3\n");
#endif

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

void rf24g_28keys_event_r5c4_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r5c4\n");
#endif

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

void rf24g_28keys_event_r6c1_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r6c1\n");
#endif

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

void rf24g_28keys_event_r6c2_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r6c2\n");
#endif

    if (fc_effect.star_on_off == DEVICE_OFF)
    {
        // 如果流星灯没有启动，不做处理
        return;
    }

    // 流星灯模式切换
    // 流星灯索引值范围： 1 ~ 16
    fc_effect.star_index++;
    if (fc_effect.star_index > 16) //
    {
        fc_effect.star_index = 1;
    }

    ls_meteor_stat_effect(); // 根据索引值，设置流星灯模式
}

void rf24g_28keys_event_r6c3_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r6c3\n");
#endif

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

void rf24g_28keys_event_r6c4_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r6c4\n");
#endif

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

void rf24g_28keys_event_r7c1_click_handle(void)
{
#if USER_DEBUG_ENABLE
    printf("28keys event r7c1\n");
#endif

    if (fc_effect.on_off_flag == DEVICE_OFF)
    {
        // 如果七彩灯没有打开，不打开电机，直接返回
        return;
    }

    // 电机开
    motor_open();
    fb_motor_mode(); // 向app反馈电机的状态
}

void rf24g_28keys_event_r7c2_click_handle(void)
{
#if USER_DEBUG_ENABLE
    printf("28keys event r7c2\n");
#endif

    // 电机关
    motor_close();
    fb_motor_mode(); // 向app反馈电机的状态
}

void rf24g_28keys_event_r7c3_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r7c3\n");
#endif

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
        printf("motor speed index %u\n", (u16)fc_effect.motor_speed_index);
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

void rf24g_28keys_event_r7c4_click_handle(void)
{
#if USER_DEBUG_ENABLE
    // printf("28keys event r7c4\n");
#endif

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
        printf("motor speed index %u\n", (u16)fc_effect.motor_speed_index);
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
        motor_sound_sensitivity_sub();
        fc_effect.music.s = fc_effect.base_ins.sensitivity; // 存放要反馈给app的灵敏度
        fb_sensitive();                                     // 向app反馈灵敏度
    }
}

const rf24_key_handle_func_t rf24_28keys_handle_func_buff[RF24G_28_KEY_EVENT_MAX] = {
    [RF24G_28_KEY_EVENT_R1C1_CLICK] = rf24g_28keys_event_r1c1_click_handle, // 按键事件处理函数
    [RF24G_28_KEY_EVENT_R1C1_LONG] = rf24g_28keys_event_r1c1_click_handle,

    [RF24G_28_KEY_EVENT_R1C2_CLICK] = rf24g_28keys_event_r1c2_click_handle,
    [RF24G_28_KEY_EVENT_R1C2_LONG] = rf24g_28keys_event_r1c2_click_handle,

    [RF24G_28_KEY_EVENT_R1C3_CLICK] = rf24g_28keys_event_r1c3_click_handle,
    [RF24G_28_KEY_EVENT_R1C3_LONG] = rf24g_28keys_event_r1c3_click_handle,

    [RF24G_28_KEY_EVENT_R1C4_CLICK] = rf24g_28keys_event_r1c4_click_handle,
    [RF24G_28_KEY_EVENT_R1C4_LONG] = rf24g_28keys_event_r1c4_click_handle,
    // =======================================================================

    [RF24G_28_KEY_EVENT_R2C1_CLICK] = rf24g_28keys_event_r2c1_click_handle,
    [RF24G_28_KEY_EVENT_R2C1_LONG] = rf24g_28keys_event_r2c1_click_handle,

    [RF24G_28_KEY_EVENT_R2C2_CLICK] = rf24g_28keys_event_r2c2_click_handle,
    [RF24G_28_KEY_EVENT_R2C2_LONG] = rf24g_28keys_event_r2c2_click_handle,

    [RF24G_28_KEY_EVENT_R2C3_CLICK] = rf24g_28keys_event_r2c3_click_handle,
    [RF24G_28_KEY_EVENT_R2C3_LONG] = rf24g_28keys_event_r2c3_click_handle,

    [RF24G_28_KEY_EVENT_R2C4_CLICK] = rf24g_28keys_event_r2c4_click_handle,
    [RF24G_28_KEY_EVENT_R2C4_LONG] = rf24g_28keys_event_r2c4_click_handle,

    // =======================================================================
    [RF24G_28_KEY_EVENT_R3C1_CLICK] = rf24g_28keys_event_r3c1_click_handle,
    [RF24G_28_KEY_EVENT_R3C1_LONG] = rf24g_28keys_event_r3c1_click_handle,

    [RF24G_28_KEY_EVENT_R3C2_CLICK] = rf24g_28keys_event_r3c2_click_handle,
    [RF24G_28_KEY_EVENT_R3C2_LONG] = rf24g_28keys_event_r3c2_click_handle,

    [RF24G_28_KEY_EVENT_R3C3_CLICK] = rf24g_28keys_event_r3c3_click_handle,
    [RF24G_28_KEY_EVENT_R3C3_LONG] = rf24g_28keys_event_r3c3_click_handle,

    [RF24G_28_KEY_EVENT_R3C4_CLICK] = rf24g_28keys_event_r3c4_click_handle,
    [RF24G_28_KEY_EVENT_R3C4_LONG] = rf24g_28keys_event_r3c4_click_handle,
    // =======================================================================
    [RF24G_28_KEY_EVENT_R4C1_CLICK] = rf24g_28keys_event_r4c1_click_handle,
    [RF24G_28_KEY_EVENT_R4C1_LONG] = rf24g_28keys_event_r4c1_click_handle,

    [RF24G_28_KEY_EVENT_R4C2_CLICK] = rf24g_28keys_event_r4c2_click_handle,
    [RF24G_28_KEY_EVENT_R4C2_LONG] = rf24g_28keys_event_r4c2_click_handle,

    [RF24G_28_KEY_EVENT_R4C3_CLICK] = rf24g_28keys_event_r4c3_click_handle,
    [RF24G_28_KEY_EVENT_R4C3_LONG] = rf24g_28keys_event_r4c3_click_handle,

    [RF24G_28_KEY_EVENT_R4C4_CLICK] = rf24g_28keys_event_r4c4_click_handle,
    [RF24G_28_KEY_EVENT_R4C4_LONG] = rf24g_28keys_event_r4c4_click_handle,
    // =======================================================================
    [RF24G_28_KEY_EVENT_R5C1_CLICK] = rf24g_28keys_event_r5c1_click_handle,
    [RF24G_28_KEY_EVENT_R5C1_LONG] = rf24g_28keys_event_r5c1_click_handle,

    [RF24G_28_KEY_EVENT_R5C2_CLICK] = rf24g_28keys_event_r5c2_click_handle,
    [RF24G_28_KEY_EVENT_R5C2_LONG] = rf24g_28keys_event_r5c2_click_handle,

    [RF24G_28_KEY_EVENT_R5C3_CLICK] = rf24g_28keys_event_r5c3_click_handle,
    [RF24G_28_KEY_EVENT_R5C3_LONG] = rf24g_28keys_event_r5c3_click_handle,

    [RF24G_28_KEY_EVENT_R5C4_CLICK] = rf24g_28keys_event_r5c4_click_handle,
    [RF24G_28_KEY_EVENT_R5C4_LONG] = rf24g_28keys_event_r5c4_click_handle,
    // =======================================================================
    [RF24G_28_KEY_EVENT_R6C1_CLICK] = rf24g_28keys_event_r6c1_click_handle,
    [RF24G_28_KEY_EVENT_R6C1_LONG] = rf24g_28keys_event_r6c1_click_handle,

    [RF24G_28_KEY_EVENT_R6C2_CLICK] = rf24g_28keys_event_r6c2_click_handle,
    [RF24G_28_KEY_EVENT_R6C2_LONG] = rf24g_28keys_event_r6c2_click_handle,

    [RF24G_28_KEY_EVENT_R6C3_CLICK] = rf24g_28keys_event_r6c3_click_handle,
    [RF24G_28_KEY_EVENT_R6C3_LONG] = rf24g_28keys_event_r6c3_click_handle,

    [RF24G_28_KEY_EVENT_R6C4_CLICK] = rf24g_28keys_event_r6c4_click_handle,
    [RF24G_28_KEY_EVENT_R6C4_LONG] = rf24g_28keys_event_r6c4_click_handle,
    // =======================================================================
    [RF24G_28_KEY_EVENT_R7C1_CLICK] = rf24g_28keys_event_r7c1_click_handle,
    [RF24G_28_KEY_EVENT_R7C1_LONG] = rf24g_28keys_event_r7c1_click_handle,

    [RF24G_28_KEY_EVENT_R7C2_CLICK] = rf24g_28keys_event_r7c2_click_handle,
    [RF24G_28_KEY_EVENT_R7C2_LONG] = rf24g_28keys_event_r7c2_click_handle,

    [RF24G_28_KEY_EVENT_R7C3_CLICK] = rf24g_28keys_event_r7c3_click_handle,
    [RF24G_28_KEY_EVENT_R7C3_LONG] = rf24g_28keys_event_r7c3_click_handle,

    [RF24G_28_KEY_EVENT_R7C4_CLICK] = rf24g_28keys_event_r7c4_click_handle,
    [RF24G_28_KEY_EVENT_R7C4_LONG] = rf24g_28keys_event_r7c4_click_handle,

};

#endif // RF24GKEY_ENABLE
