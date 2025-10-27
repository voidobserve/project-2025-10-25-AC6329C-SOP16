#include "rf24g_key.h"
#include "../../../apps/user_app/led_strip/led_strand_effect.h"
#include "../../../apps/user_app/save_flash/save_flash.h" // 包含读写flash的接口

#if 1

static volatile u8 rf24g_rx_flag = 0;       // 是否收到了新的数据
volatile rf24g_recv_info_t rf24g_recv_info; // 存放接收到的数据包
volatile u8 chromatic_circle_val = 0;       // 存放色环按键对应的数值，范围：0x00~0xFF
volatile u8 rf24g_key_val = NO_KEY;         // 存放按键键值

const u8 rf24g_key_event_table[][RF34G_KEY_EVENT_MAX + 1] = {
    // 0,
    {RF24G_KEY_ON_OFF, RF24G_KEY_EVENT_ON_OFF_CLICK, RF24G_KEY_EVENT_ON_OFF_HOLD, RF24G_KEY_EVENT_ON_OFF_LOOSE},

    // 色环按键
    {RF24G_KEY_CHROMATIC_CIRCLE, RF24G_KEY_EVENT_CHROMATIC_CIRCLE_CLICK, RF24G_KEY_EVENT_CHROMATIC_CIRCLE_HOLD, RF24G_KEY_EVENT_CHROMATIC_CIRCLE_LOOSE},
    // 亮度加
    {RF24G_KEY_BRIGHTNESS_ADD, RF24G_KEY_EVENT_BRIGHTNESS_ADD_CLICK, RF24G_KEY_EVENT_BRIGHTNESS_ADD_HOLD, RF24G_KEY_EVENT_BRIGHTNESS_ADD_LOOSE},
    // 亮度减
    {RF24G_KEY_BRIGHTNESS_SUB, RF24G_KEY_EVENT_BRIGHTNESS_SUB_CLICK, RF24G_KEY_EVENT_BRIGHTNESS_SUB_HOLD, RF24G_KEY_EVENT_BRIGHTNESS_SUB_LOOSE},
    // 动态速度加
    {RF24G_KEY_DYNAMIC_SPEED_ADD, RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_CLICK, RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_HOLD, RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_LOOSE},
    // 动态速度减
    {RF24G_KEY_DYNAMIC_SPEED_SUB, RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_CLICK, RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_HOLD, RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_LOOSE},
    // 模式加
    {RF24G_KEY_MODE_ADD, RF24G_KEY_EVENT_MODE_ADD_CLICK, RF24G_KEY_EVENT_MODE_ADD_HOLD, RF24G_KEY_EVENT_MODE_ADD_LOOSE},
    // 模式减
    {RF24G_KEY_MODE_SUB, RF24G_KEY_EVENT_MODE_SUB_CLICK, RF24G_KEY_EVENT_MODE_SUB_HOLD, RF24G_KEY_EVENT_MODE_SUB_LOOSE},
    // 冷白亮度加，同时暖白亮度减
    {RF24G_KEY_COLD_WHITE_BRIGHTNESS_ADD, RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_ADD_CLICK, RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_ADD_HOLD, RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_ADD_LOOSE},
    // 冷白亮度减，同时暖白亮度加
    {RF24G_KEY_COLD_WHITE_BRIGHTNESS_SUB, RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_SUB_CLICK, RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_SUB_HOLD, RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_SUB_LOOSE},
    // 电机速度加
    {RF24G_KEY_MOTOR_SPEED_ADD, RF24G_KEY_EVENT_MOTOR_SPEED_ADD_CLICK, RF24G_KEY_EVENT_MOTOR_SPEED_ADD_HOLD, RF24G_KEY_EVENT_MOTOR_SPEED_ADD_LOOSE},
    // 电机速度减
    {RF24G_KEY_MOTOR_SPEED_SUB, RF24G_KEY_EVENT_MOTOR_SPEED_SUB_CLICK, RF24G_KEY_EVENT_MOTOR_SPEED_SUB_HOLD, RF24G_KEY_EVENT_MOTOR_SPEED_SUB_LOOSE},
    // 流星速度加
    {RF24G_KEY_METEOR_SPEED_ADD, RF24G_KEY_EVENT_METEOR_SPEED_ADD_CLICK, RF24G_KEY_EVENT_METEOR_SPEED_ADD_HOLD, RF24G_KEY_EVENT_METEOR_SPEED_ADD_LOOSE},
    // 流星速度减
    {RF24G_KEY_METEOR_SPEED_SUB, RF24G_KEY_EVENT_METEOR_SPEED_SUB_CLICK, RF24G_KEY_EVENT_METEOR_SPEED_SUB_HOLD, RF24G_KEY_EVENT_METEOR_SPEED_SUB_LOOSE},

};

volatile u8 rf24g_key_driver_event = 0; // 由key_driver_scan() 更新
volatile u8 rf24g_key_driver_value = 0; // 由key_driver_scan() 更新

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
    if (p->header1 == RF24G_HEADER_1 && p->header2 == RF24G_HEADER_2)
    // if (p->header1 == 0xDC && p->header2 == 0xDC || (p->header1 == RF24G_HEADER_1 && p->header2 == RF24G_HEADER_2)) // 测试时使用
    {
        // printf_buf(recv_buff, sizeof(rf24g_recv_info_t)); // 打印接收到的数据包

        // rf24g_recv_info = *p; // 结构体变量赋值

        // 测试时使用：
        // if (p->header1 == 0xDC && p->header2 == 0xDC)
        {
            rf24g_key_val = p->key_1;
            rf24g_rx_flag = 1;
            return;
        }

        // if (p->key_1 == RF24G_KEY_CHROMATIC_CIRCLE) // 如果是色环按键
        // {
        //     rf24g_key_val = p->key_1;
        //     chromatic_circle_val = p->key_2;
        // }
        // else
        // {
        //     rf24g_key_val = p->key_2;
        // }

        rf24g_rx_flag = 1; // 测试的时候，屏蔽2.4G遥控器按键
    }
}

static u8 rf24g_get_key_value(void)
{
    u8 key_value = 0;
    static u16 time_out_cnt = 0; // 加入超时，防止丢包（超时时间与按键扫描时间有关）
    static u8 last_key_value = 0;

    if (rf24g_rx_flag == 1) // 收到2.4G广播
    {
        rf24g_rx_flag = 0;

        key_value = rf24g_key_val;

        // printf("__LINE__ %u", __LINE__);

        if (RF24G_KEY_CHROMATIC_CIRCLE != key_value) // 收到一次色环的键值就处理一次，不用等超时
        {
            // time_out_cnt = 30; // 2.4G接收可能会丢失100~200ms的数据包（响应会慢一些）
            time_out_cnt = 20; // 2.4G接收可能会丢失100~200ms的数据包（响应会慢一些）
            // time_out_cnt = 10; // 2.4G接收可能会丢失100~200ms的数据包（响应会慢一些）
        }

        if (0x52 == key_value) // 如果键值是0x52，可能是803接收数据时丢失数据包，认为这个键值还是上一次收到的键值
        {
            key_value = last_key_value;
        }

        last_key_value = key_value;

        // rf24g_recv_info.key_v = NO_KEY;
        return last_key_value;
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
    else if (KEY_EVENT_LONG == key_driver_event || KEY_EVENT_HOLD == key_driver_event)
    {
        // long和hold都当作hold处理
        key_event_index = 2;
    }
    else if (KEY_EVENT_UP == key_driver_event)
    {
        // 长按后松手
        key_event_index = 3;
    }

    if (0 == key_event_index || NO_KEY == key_value)
    {
        // 按键事件与上面的事件都不匹配
        // 得到的键值是无效键值
        return RF24G_KEY_EVENT_NONE;
    }

    for (u8 i = 0; i < sizeof(rf24g_key_event_table) / sizeof(rf24g_key_event_table[0]); i++)
    {
        if (key_value == rf24g_key_event_table[i][0])
        {
            return rf24g_key_event_table[i][key_event_index];
        }
    }

    // 如果运行到这里，都没有找到对应的按键，返回无效按键事件
    return RF24G_KEY_EVENT_NONE;
}

volatile u8 color_index_cur;
volatile u8 color_index_dest;

#define ADJUST_WHITE_BRIGHTNESS_STEP 10 // 冷白亮度、暖白亮度的调节步长
// static u8 cold_white_brightness = 128;  // 冷白亮度，初始值50%
// static u8 warm_white_brightness = 128;  // 暖白亮度，初始值50%
static u8 cold_white_brightness = 0;   // 冷白亮度，初始值
static u8 warm_white_brightness = 255; // 暖白亮度，初始值

void rf24_key_handle(void)
{
    u8 rf24g_key_event = 0;

    if (NO_KEY == rf24g_key_driver_value)
        return;

    rf24g_key_event = rf24g_convert_key_event(rf24g_key_driver_value, rf24g_key_driver_event);
    rf24g_key_driver_value = NO_KEY;

    switch (rf24g_key_event)
    {
        // 收到短按、长按后松手，再执行对应的功能

    case RF24G_KEY_EVENT_ON_OFF_CLICK: // 短按 -- 开机
    {
        printf("key event on/off click\n");
        // read_flash_device_status_init(); // 读取设备状态（目前应该不用加这一句）
        soft_turn_on_the_light(); // 在测试时屏蔽
    }
    break;
    case RF24G_KEY_EVENT_ON_OFF_HOLD: // 长按 -- 关机
    {
        printf("key event on/off hold\n");
        soft_turn_off_lights(); // 在测试时屏蔽
    }
    break;
    case RF24G_KEY_EVENT_ON_OFF_LOOSE:
        printf("key event on/off loose\n");
        break;

    // 色环按键的短按和长按，都是执行相同的功能
    case RF24G_KEY_EVENT_CHROMATIC_CIRCLE_CLICK:
    case RF24G_KEY_EVENT_CHROMATIC_CIRCLE_HOLD:
    {
        // printf("key event chromatic circle\n");
        // printf("key event chromatic circle click\n");
        // printf("key event chromatic circle hold\n");

        // 根据色环按键的数值转换成对应的RGB值：
        u8 r;
        u8 g;
        u8 b;
        extern const u8 chromatic_circle_table[][3];

        if (0x51 == chromatic_circle_val )
        {
            // 如果收到了0x51，可能是803接收触摸ic传过来的数据丢失了，这里选择屏蔽这个数据
            return; 
        }
        

        r = chromatic_circle_table[chromatic_circle_val][0];
        g = chromatic_circle_table[chromatic_circle_val][1];
        b = chromatic_circle_table[chromatic_circle_val][2];
        fc_effect.rgb.r = r;
        fc_effect.rgb.g = g;
        fc_effect.rgb.b = b;
        fc_effect.Now_state = IS_STATIC;

        // 如果调节了冷白、暖白亮度，不能只通过 set_static_mode 来调节颜色：
        // set_static_mode(r, g, b);
        set_fc_effect(); // 效果调度

        // 这里可能要一段时间无操作后再写入flash：（实际测试，频繁调节色环时，同时频繁写入flash，不会造成卡顿）
        save_user_data_area3();
#if 0
        fc_effect.Now_state = COLORFUL_LIGHTS_STATIC;
        fc_effect.dream_scene.c_n = 1;

        color_index_cur = color_index_dest;
        color_index_dest = chromatic_circle_val;


        // 每次传入颜色，如果都是这个模式，让颜色从当前颜色渐变至目标颜色

        extern u16 colorful_lights_static(void);
        WS2812FX_setSegment_colorOptions(                   // 设置一段颜色的效果
            0,                                              // 第0段
            0, 0,                                           // 起始位置，结束位置
            &colorful_lights_static,                          // 效果
            0,                                              // 颜色，WS2812FX_setColors设置
            100,                                            // 速度
            0);                                             // 选项，这里像素点大小：1
        // WS2812FX_set_coloQty(0, fc_effect.dream_scene.c_n); // 设置颜色数量  0：第0段   fc_effect.dream_scene.c_n  颜色数量，一个颜色包含（RGB）
        // ls_set_colors(1, &fc_effect.rgb);                   // 1:1个颜色    &fc_effect.rgb 这个颜色是什么色

        WS2812FX_start();
#endif
    }
    break;

    case RF24G_KEY_EVENT_BRIGHTNESS_ADD_CLICK: // 亮度加
        printf("key event brightness add click\n");
    case RF24G_KEY_EVENT_BRIGHTNESS_ADD_HOLD:
    {

        // 目前只有静态模式才能调亮度
        ls_add_bright();
        save_user_data_area3();
    }
    break;

#if 0 // 测试能不能检测长短按

    case RF24G_KEY_EVENT_BRIGHTNESS_ADD_CLICK: // 亮度加
        printf("key event brightness add click\n");
        break;
    case RF24G_KEY_EVENT_BRIGHTNESS_ADD_HOLD:
    {
        printf("key event brightness add hold\n");
    }
    break;
    case RF24G_KEY_EVENT_BRIGHTNESS_ADD_LOOSE: //
        printf("key event brightness sub click\n");
        break;

#endif // 测试能不能检测长短按

    case RF24G_KEY_EVENT_BRIGHTNESS_SUB_CLICK: // 亮度减
    case RF24G_KEY_EVENT_BRIGHTNESS_SUB_HOLD:
    {
        // 目前只有静态模式才能调亮度
        ls_sub_bright();
        save_user_data_area3();
    }
    break;

    case RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_CLICK: // 动态速度加
                                                  // case RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_HOLD:
    case RF24G_KEY_EVENT_DYNAMIC_SPEED_ADD_LOOSE:
    {
        // 目前只有在动态模式下，才加快速度
        ls_add_speed();
        save_user_data_area3();
    }
    break;

    case RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_CLICK: // 动态速度减
                                                  // case RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_HOLD:
    case RF24G_KEY_EVENT_DYNAMIC_SPEED_SUB_LOOSE:
    {
        // 目前只有在动态模式下，才减慢速度
        ls_sub_speed();
        save_user_data_area3();
    }
    break;

    case RF24G_KEY_EVENT_MODE_ADD_CLICK: // 模式加
    case RF24G_KEY_EVENT_MODE_ADD_LOOSE:
    {
        // USER_TO_DO 需要加上流星灯模式的切换
        ls_add_mode_InAPP();
        save_user_data_area3();
    }
    break;

    case RF24G_KEY_EVENT_MODE_SUB_CLICK: // 模式减
    case RF24G_KEY_EVENT_MODE_SUB_LOOSE:
    {
        // USER_TO_DO 需要加上流星灯模式的切换
        ls_sub_mode_InAPP();
        save_user_data_area3();
    }
    break;

    case RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_ADD_CLICK: // 冷白亮度加 同时暖白亮度减
    case RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_ADD_HOLD:
    {
        // 在静态模式下调节冷白和暖白亮度
        if (fc_effect.Now_state == IS_STATIC)
        {
            // 增加冷白亮度
            // if (cold_white_brightness < 255)
            // {
            //     cold_white_brightness++;
            // }

            // // 减少暖白亮度
            // if (warm_white_brightness > 0)
            // {
            //     warm_white_brightness--;
            // }

            if (cold_white_brightness < 255 - ADJUST_WHITE_BRIGHTNESS_STEP)
            {
                cold_white_brightness += ADJUST_WHITE_BRIGHTNESS_STEP;
            }
            else
            {
                cold_white_brightness = 255;
            }

            // 减少暖白亮度
            if (warm_white_brightness > 0 + ADJUST_WHITE_BRIGHTNESS_STEP)
            {
                warm_white_brightness -= ADJUST_WHITE_BRIGHTNESS_STEP;
            }
            else
            {
                warm_white_brightness = 0;
            }

            // 根据冷白和暖白的组合值设置最终的白光输出
            // 这里简化处理，实际可以根据色温需求调整算法
            // u8 final_white = (cold_white_brightness + (255 - warm_white_brightness)) / 2;
            fc_effect.rgb.w = (cold_white_brightness + (255 - warm_white_brightness)) / 2;

            // printf("cold_white_brightness: %u\n", (u16)cold_white_brightness);
            // printf("warm_white_brightness: %u\n", (u16)warm_white_brightness);
            // printf("fc_effect.rgb.w: %u\n", (u16)fc_effect.rgb.w);

            // 更新显示
            // set_static_mode(fc_effect.rgb.r, fc_effect.rgb.g, fc_effect.rgb.b, fc_effect.rgb.w);
            set_fc_effect(); // 效果调度
            fb_bright();     // 反馈亮度变化给APP
            save_user_data_area3();
        }
    }
    break;

    case RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_SUB_CLICK: // 冷白亮度减 同时暖白亮度加
    case RF24G_KEY_EVENT_COLD_WHITE_BRIGHTNESS_SUB_HOLD:
    {
        // 在静态模式下调节冷白和暖白亮度
        if (fc_effect.Now_state == IS_STATIC)
        {
            // // 减少冷白亮度
            // if (cold_white_brightness > 0)
            // {
            //     cold_white_brightness--;
            // }

            // // 增加暖白亮度
            // if (warm_white_brightness < 255)
            // {
            //     warm_white_brightness++;
            // }

            // 减少冷白亮度
            if (cold_white_brightness > 0 + ADJUST_WHITE_BRIGHTNESS_STEP)
            {
                cold_white_brightness -= ADJUST_WHITE_BRIGHTNESS_STEP;
            }
            else
            {
                cold_white_brightness = 0;
            }

            // 增加暖白亮度
            if (warm_white_brightness < 255 - ADJUST_WHITE_BRIGHTNESS_STEP)
            {
                warm_white_brightness += ADJUST_WHITE_BRIGHTNESS_STEP;
            }
            else
            {
                warm_white_brightness = 255;
            }

            // 根据冷白和暖白的组合值设置最终的白光输出
            // 这里简化处理，实际可以根据色温需求调整算法
            // u8 final_white = (cold_white_brightness + (255 - warm_white_brightness)) / 2;
            fc_effect.rgb.w = (cold_white_brightness + (255 - warm_white_brightness)) / 2;

            // printf("cold_white_brightness: %u\n", (u16)cold_white_brightness);
            // printf("warm_white_brightness: %u\n", (u16)warm_white_brightness);
            // printf("fc_effect.rgb.w: %u\n", (u16)fc_effect.rgb.w);

            // 更新显示
            // set_static_mode(fc_effect.rgb.r, fc_effect.rgb.g, fc_effect.rgb.b, fc_effect.rgb.w);
            set_fc_effect(); // 效果调度
            fb_bright();     // 反馈亮度变化给APP
            save_user_data_area3();
        }
    }
    break;

    case RF24G_KEY_EVENT_MOTOR_SPEED_ADD_CLICK: // 电机速度加
    case RF24G_KEY_EVENT_MOTOR_SPEED_ADD_LOOSE:
    {
        ls_add_motor_speed();
        save_user_data_area3();
    }
    break;

    case RF24G_KEY_EVENT_MOTOR_SPEED_SUB_CLICK: // 电机速度减
    case RF24G_KEY_EVENT_MOTOR_SPEED_SUB_LOOSE:
    {
        ls_sub_motor_speed();
        save_user_data_area3();
    }
    break;

    case RF24G_KEY_EVENT_METEOR_SPEED_ADD_CLICK: // 流星灯速度加
    case RF24G_KEY_EVENT_METEOR_SPEED_ADD_LOOSE:
    {
        // 流星灯开启时，才调节速度：
        ls_add_star_speed();
        if (DEVICE_ON == fc_effect.star_on_off)
        {
            save_user_data_area3();
        }
    }
    break;

    case RF24G_KEY_EVENT_METEOR_SPEED_SUB_CLICK: // 流星灯速度减
    case RF24G_KEY_EVENT_METEOR_SPEED_SUB_LOOSE:
    {
        // 流星灯开启时，才调节速度：
        ls_sub_star_speed();
        if (DEVICE_ON == fc_effect.star_on_off)
        {
            save_user_data_area3();
        }
    }
    break;

    } // switch (rf24g_key_event)
}
#endif

// 色环数值对应的查表数组
const u8 chromatic_circle_table[][3] = {
    {255, 0, 0},
    {255, 5, 0},
    {255, 7, 0},
    {255, 17, 0},
    {255, 20, 0},
    {255, 30, 0},
    {255, 33, 0},
    {255, 38, 0},
    {255, 45, 0},
    {255, 51, 0},
    {255, 58, 0},
    {255, 63, 0},
    {255, 68, 0},
    {255, 76, 0},
    {255, 81, 0},
    {255, 89, 0},
    {255, 94, 0},
    {255, 96, 0},
    {255, 107, 0},
    {255, 109, 0},
    {255, 119, 0},
    {255, 122, 0},
    {255, 127, 0},
    {255, 135, 0},
    {255, 140, 0},
    {255, 147, 0},
    {255, 153, 0},
    {255, 158, 0},
    {255, 165, 0},
    {255, 170, 0},
    {255, 178, 0},
    {255, 183, 0},
    {255, 191, 0},
    {255, 196, 0},
    {255, 198, 0},
    {255, 209, 0},
    {255, 211, 0},
    {255, 221, 0},
    {255, 224, 0},
    {255, 229, 0},
    {255, 237, 0},
    {255, 242, 0},
    {255, 249, 0},
    {255, 255, 0},
    {249, 255, 0},
    {242, 255, 0},
    {237, 255, 0},
    {229, 255, 0},
    {224, 255, 0},
    {221, 255, 0},
    {211, 255, 0},
    {209, 255, 0},
    {198, 255, 0},
    {196, 255, 0},
    {191, 255, 0},
    {183, 255, 0},
    {178, 255, 0},
    {170, 255, 0},
    {165, 255, 0},
    {160, 255, 0},
    {153, 255, 0},
    {147, 255, 0},
    {140, 255, 0},
    {135, 255, 0},
    {127, 255, 0},
    {122, 255, 0},
    {119, 255, 0},
    {109, 255, 0},
    {107, 255, 0},
    {96, 255, 0},
    {94, 255, 0},
    {89, 255, 0},
    {81, 255, 0},
    {76, 255, 0},
    {68, 255, 0},
    {63, 255, 0},
    {58, 255, 0},
    {51, 255, 0},
    {45, 255, 0},
    {38, 255, 0},
    {33, 255, 0},
    {30, 255, 0},
    {20, 255, 0},
    {17, 255, 0},
    {7, 255, 0},
    {5, 255, 0},
    {0, 255, 0},
    {0, 255, 7},
    {0, 255, 12},
    {0, 255, 20},
    {0, 255, 25},
    {0, 255, 30},
    {0, 255, 38},
    {0, 255, 43},
    {0, 255, 51},
    {0, 255, 56},
    {0, 255, 63},
    {0, 255, 68},
    {0, 255, 71},
    {0, 255, 81},
    {0, 255, 84},
    {0, 255, 94},
    {0, 255, 96},
    {0, 255, 102},
    {0, 255, 109},
    {0, 255, 114},
    {0, 255, 122},
    {0, 255, 127},
    {0, 255, 132},
    {0, 255, 140},
    {0, 255, 145},
    {0, 255, 153},
    {0, 255, 158},
    {0, 255, 160},
    {0, 255, 170},
    {0, 255, 173},
    {0, 255, 183},
    {0, 255, 186},
    {0, 255, 191},
    {0, 255, 198},
    {0, 255, 204},
    {0, 255, 211},
    {0, 255, 216},
    {0, 255, 221},
    {0, 255, 229},
    {0, 255, 234},
    {0, 255, 242},
    {0, 255, 247},
    {0, 255, 255},
    {0, 249, 255},
    {0, 247, 255},
    {0, 237, 255},
    {0, 234, 255},
    {0, 224, 255},
    {0, 221, 255},
    {0, 216, 255},
    {0, 209, 255},
    {0, 204, 255},
    {0, 196, 255},
    {0, 191, 255},
    {0, 186, 255},
    {0, 178, 255},
    {0, 173, 255},
    {0, 165, 255},
    {0, 160, 255},
    {0, 158, 255},
    {0, 147, 255},
    {0, 145, 255},
    {0, 135, 255},
    {0, 132, 255},
    {0, 127, 255},
    {0, 119, 255},
    {0, 114, 255},
    {0, 107, 255},
    {0, 102, 255},
    {0, 96, 255},
    {0, 89, 255},
    {0, 84, 255},
    {0, 76, 255},
    {0, 71, 255},
    {0, 63, 255},
    {0, 58, 255},
    {0, 56, 255},
    {0, 45, 255},
    {0, 43, 255},
    {0, 33, 255},
    {0, 30, 255},
    {0, 25, 255},
    {0, 17, 255},
    {0, 12, 255},
    {0, 5, 255},
    {0, 0, 255},
    {5, 0, 255},
    {12, 0, 255},
    {17, 0, 255},
    {25, 0, 255},
    {30, 0, 255},
    {33, 0, 255},
    {43, 0, 255},
    {45, 0, 255},
    {56, 0, 255},
    {58, 0, 255},
    {63, 0, 255},
    {71, 0, 255},
    {76, 0, 255},
    {84, 0, 255},
    {89, 0, 255},
    {94, 0, 255},
    {102, 0, 255},
    {107, 0, 255},
    {114, 0, 255},
    {119, 0, 255},
    {127, 0, 255},
    {132, 0, 255},
    {135, 0, 255},
    {145, 0, 255},
    {147, 0, 255},
    {158, 0, 255},
    {160, 0, 255},
    {165, 0, 255},
    {173, 0, 255},
    {178, 0, 255},
    {186, 0, 255},
    {191, 0, 255},
    {196, 0, 255},
    {204, 0, 255},
    {209, 0, 255},
    {216, 0, 255},
    {221, 0, 255},
    {224, 0, 255},
    {234, 0, 255},
    {237, 0, 255},
    {247, 0, 255},
    {249, 0, 255},
    {255, 0, 255},
    {255, 0, 247},
    {255, 0, 242},
    {255, 0, 234},
    {255, 0, 229},
    {255, 0, 224},
    {255, 0, 216},
    {255, 0, 211},
    {255, 0, 204},
    {255, 0, 198},
    {255, 0, 191},
    {255, 0, 186},
    {255, 0, 183},
    {255, 0, 173},
    {255, 0, 170},
    {255, 0, 160},
    {255, 0, 158},
    {255, 0, 153},
    {255, 0, 145},
    {255, 0, 140},
    {255, 0, 132},
    {255, 0, 127},
    {255, 0, 122},
    {255, 0, 114},
    {255, 0, 109},
    {255, 0, 102},
    {255, 0, 96},
    {255, 0, 94},
    {255, 0, 84},
    {255, 0, 81},
    {255, 0, 71},
    {255, 0, 68},
    {255, 0, 63},
    {255, 0, 56},
    {255, 0, 51},
    {255, 0, 43},
    {255, 0, 38},
    {255, 0, 33},
    {255, 0, 25},
    {255, 0, 20},
    {255, 0, 12},
    {255, 0, 7},

};

void convert_key_val_to_rgb(u8 key_val)
{
    // 如果当前遥控器调节的是RGB值
    // if (CUR_SEL_RGB_MODE == ret_mem_data.current_rgb_mode)
    // {
    //     r = table[key_val][0];
    //     g = table[key_val][1];
    //     b = table[key_val][2];
    // }
    // else if (CUR_SEL_CW_MODE == ret_mem_data.current_rgb_mode)
    // {
    //     // 如果当前调节的是亮度值
    //     cw = (u16)key_val * 100 / 255;
    // }
}
