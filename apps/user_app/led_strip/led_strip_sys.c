

#include "led_strip_sys.h"
#include "Adafruit_NeoPixel.H"
#include "led_strand_effect.h"

#include "../../../apps/user_app/protocol/dp_data_tran.h"
#include "../../../apps/user_app/one_wire/one_wire.h"

#define MAX_BRIGHT_RANK 10
#define MAX_SPEED_RANK 10
#define MIN_BRIGHT_VALUE 10
#define MIN_SLOW_SPEED 500
#define MAX_FAST_SPEED 10

#if (LED_STRIP_TYPE == TYPE_Fiber_optic_lights)
#define MAX_MUSIC_EFFECT_NUMBER (4)
#elif (LED_STRIP_TYPE == TYPE_Magic_lights)
#define MAX_MUSIC_EFFECT_NUMBER (12)
#endif

// 效果数据初始化
void fc_data_init(void)
{
    // 灯具
    fc_effect.on_off_flag = DEVICE_ON; // 灯为开启状态
    fc_effect.led_num = 12 + 1;        // 灯带的总灯珠数量（12个流星灯+1七彩灯）
    fc_effect.Now_state = IS_STATIC;   // 当前运行状态 静态
    // fc_effect.rgb.r = 0;
    // fc_effect.rgb.g = 0;
    // fc_effect.rgb.b = 0;
    fc_effect.rgb.r = 255;
    // fc_effect.rgb.g = 255;
    // fc_effect.rgb.b = 255;
#ifdef LED_STRIP_RGBW
    // fc_effect.rgb.w = 255;
    fc_effect.rgb.w = 0;
#endif
    fc_effect.dream_scene.c_n = 1; // 颜色数量为1
    fc_effect.app_b = 100 / 2;
    // fc_effect.app_b = 100 / 10;
    // fc_effect.app_b = 100 / 2;
    fc_effect.b = 255 / 2; // 测试时不能用最大亮度
    // fc_effect.b = 10;

    // fc_effect.b = (u16)fc_effect.app_b * 255 / 100;

    fc_effect.ls_b = (MAX_BRIGHT_RANK - 1);
    // fc_effect.ls_b = (MIN_BRIGHT_VALUE);

    fc_effect.app_speed = 80;
    fc_effect.dream_scene.speed = 100;
    fc_effect.ls_speed = 3;
    fc_effect.sequence = NEO_RBGW;
    fc_effect.auto_f = IS_PAUSE;
    fc_effect.music.s = 80;
    fc_effect.music.m = 0;
    fc_effect.music.m_type = 0;

    fc_effect.app_rgb_mode = 0;
    // 闹钟
    // zd_countdown[0].set_on_off = DEVICE_OFF;
    // zd_countdown[1].set_on_off = DEVICE_OFF;
    // zd_countdown[2].set_on_off = DEVICE_OFF;

    // 流星
    fc_effect.star_on_off = DEVICE_ON;
    fc_effect.star_index = 1;
    fc_effect.star_speed = 30; // 变化速度
    // fc_effect.star_speed = 3000; // 变化速度（数值越小，速度越快）
    fc_effect.app_star_speed = 100;
    fc_effect.meteor_period = 8;                           // 默认8秒  周期值
    fc_effect.period_cnt = fc_effect.meteor_period * 1000; // ms,运行时的计数器
    fc_effect.mode_cycle = 0;                              // 模式完成一个循环的标志
    fc_effect.star_speed_index = 0;                        // 电机模式或电机速度索引

    // 电机
    fc_effect.base_ins.mode = 4;   // 360转
    fc_effect.base_ins.period = 8; // 速度8s
    fc_effect.base_ins.dir = 0;    // 0: 正转  1：
    fc_effect.base_ins.music_mode = 0;
    fc_effect.motor_on_off = DEVICE_ON;
}

void OpenMortor(void);
// static u8 tk = 0;
/*********************************************************
 *
 *      软件开机  关机 API
 *
 *********************************************************/
void soft_turn_on_the_light(void) // 软开灯处理
{
    fc_effect.on_off_flag = DEVICE_ON; // 七彩灯为开启状态
    fc_effect.star_on_off = DEVICE_ON; // 流星灯打开

    // OpenMortor(); // 给对应标志位置位，表示电机打开
    motor_Init();     // 初始化电机相关的变量
    WS2812FX_start(); // 清空动画使用到的缓存，给运行标志位置位

    // 开机前，可能关机前电机就开着，或者关机前电机就已经关了，开机后保持状态不变（开机后，恢复电机在关机前的状态）
    if (fc_effect.star_speed_index >= ARRAY_SIZE(motor_period))
    {
        // 如果开机前，电机的周期索引超过了电机的周期数组大小，说明电机在开机前就是关着的
        one_wire_set_mode(6); // 关闭电机
        fc_effect.motor_on_off = DEVICE_OFF;
    }
    else
    {
        // 如果开机前，电机的周期索引还在电机的周期数组大小内，说明电机在开机前是开着的
        one_wire_set_period(motor_period[fc_effect.star_speed_index]);
        one_wire_set_mode(4); // 360正转
        fc_effect.motor_on_off = DEVICE_ON;
    }

    os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);

    set_fc_effect();         // 七彩灯动画效果设置
    ls_meteor_stat_effect(); // 流星灯动画效果设置 这个函数里面会写入一次flash
    fb_led_on_off_state();   // 与app同步开关状态
    save_user_data_area3();  // 保存参数配置到flash

    printf("soft_turn_on_the_light\n");
}

void CloseMotor(void);
void soft_turn_off_lights(void) // 软关灯处理
{
    if (fc_effect.on_off_flag == DEVICE_ON)
    {
        fc_effect.on_off_flag = DEVICE_OFF;

        CloseMotor(); // 关闭电机

        WS2812FX_stop();
        WS2812FX_strip_off(); // 清空缓存

        fb_led_on_off_state();  // 与app同步设备的开关状态
        save_user_data_area3(); // 保存参数配置到flash

        printf("soft_turn_off_lights\n");
    }
}

/*********************************************************
 *
 *      亮度 速度 灵敏度 流星 API
 *
 *********************************************************/
const u8 led_b_array[MAX_BRIGHT_RANK] = {
    MIN_BRIGHT_VALUE, 50, 75, 100, 125,
    150, 175, 200, 225, 255}; // 0-255
const u16 led_speed_array[MAX_SPEED_RANK] = {
    MAX_FAST_SPEED, 100, 150, 200, 250,
    300, 350, 400, 450, MIN_SLOW_SPEED}; // 0-500

/**
 * @brief  APP设置亮度
 *
 * @param tp_b  0-100
 */
void app_set_bright(u8 tp_b)
{
    if (fc_effect.Now_state == IS_STATIC)
    {
        if (tp_b < MIN_BRIGHT_VALUE)
            tp_b = MIN_BRIGHT_VALUE;
        fc_effect.b = (u16)tp_b * 255 / 100;
        fc_effect.app_b = tp_b; // 反馈给app的亮度
        WS2812FX_setBrightness(fc_effect.b);
    }
}

/**
 * @brief  根据灯带长度，获取最快的速度
 *
 * @return u16
 */
u16 get_max_sp(void)
{
    u16 s;
    s = fc_effect.led_num * 30 / 1000; // 每个LED30us
    if (s < MAX_FAST_SPEED)
        s = MAX_FAST_SPEED;
    return s; //
}

/**
 * @brief APP设置速度
 *
 *
 * @param tp_speed  0-100  0是最慢  100是最快
 */
void app_set_speed(u8 tp_speed)
{
    if (fc_effect.Now_state == IS_light_scene)
    {
        fc_effect.dream_scene.speed = MIN_SLOW_SPEED - (MIN_SLOW_SPEED * tp_speed / 100);
        fc_effect.app_speed = tp_speed;
        if (fc_effect.dream_scene.speed <= get_max_sp())
        {
            fc_effect.dream_scene.speed = get_max_sp();
        }
        set_fc_effect();
    }
}

/**
 * @brief 遥控加亮度
 *
 * @param b
 */
void ls_add_bright(void)
{

    if (fc_effect.Now_state == IS_STATIC)
    {
        if (fc_effect.ls_b < (MAX_BRIGHT_RANK - 1))
            fc_effect.ls_b++;
        fc_effect.app_b = (fc_effect.ls_b + 1) * 10;
        fc_effect.b = led_b_array[fc_effect.ls_b];
        fb_bright();
        WS2812FX_setBrightness(fc_effect.b);
    }

    // printf(" fc_effect.b = %d", fc_effect.b);
}

/**
 * @brief 遥控减亮度
 *
 * @param b
 */
void ls_sub_bright(void)
{
    if (fc_effect.Now_state == IS_STATIC)
    {
        if (fc_effect.ls_b > 0)
            fc_effect.ls_b--;

        fc_effect.app_b = (fc_effect.ls_b + 1) * 10;
        fc_effect.b = led_b_array[fc_effect.ls_b];
        fb_bright();
        WS2812FX_setBrightness(fc_effect.b);
    }

    // printf(" fc_effect.b = %d", fc_effect.b);
}

/**
 * @brief 遥控加速度
 *
 */
void ls_add_speed(void)
{
    if (fc_effect.Now_state == IS_light_scene)
    {
        // 数值越小，速度越快：
        if (fc_effect.ls_speed > 0)
            fc_effect.ls_speed--;
        fc_effect.app_speed = 100 - (fc_effect.ls_speed) * 10;
        fc_effect.dream_scene.speed = led_speed_array[fc_effect.ls_speed];
        fb_speed();
        set_fc_effect();
    }

    // printf("  fc_effect.dream_scene.speed = %d", fc_effect.dream_scene.speed);
}

/**
 * @brief 遥控减速度
 *
 */
void ls_sub_speed(void)
{

    if (fc_effect.Now_state == IS_light_scene)
    {
        // 数值越大，速度越慢：
        if (fc_effect.ls_speed < (MAX_SPEED_RANK - 1))
            fc_effect.ls_speed++;
        fc_effect.app_speed = 100 - (fc_effect.ls_speed) * 10;
        fc_effect.dream_scene.speed = led_speed_array[fc_effect.ls_speed];
        fb_speed();
        set_fc_effect();
    }

    // printf("  fc_effect.dream_scene.speed = %d", fc_effect.dream_scene.speed);
}

/**
 * @brief  app设置灵敏度
 *
 * @param tp_s  0-100;
 */
void app_set_sensitive(u8 tp_s)
{

    fc_effect.music.s = tp_s;
}

/**
 * @brief 遥控加灵敏度
 *
 *
 */
void ls_add_sensitive(void)
{
    u8 sen_gap = 10;
    if (fc_effect.Now_state == IS_light_music)
    {
        if (fc_effect.music.s < (100 - sen_gap))
            fc_effect.music.s += sen_gap;
    }

    printf(" fc_effect.music.s= %d", fc_effect.music.s);
}

/**
 * @brief 遥控减灵敏度
 *
 *
 */
void ls_sub_sensitive(void)
{

    u8 sen_gap = 10;
    if (fc_effect.Now_state == IS_light_music)
    {
        if (fc_effect.music.s > sen_gap)
            fc_effect.music.s -= sen_gap;
    }
    printf(" fc_effect.music.s= %d", fc_effect.music.s);
}

/**
 * @brief 遥控设置音乐效果
 *
 */
void ls_add_music_effect(void)
{
    if (fc_effect.music.m < (MAX_MUSIC_EFFECT_NUMBER - 1))
    {
        fc_effect.music.m++;
        printf("fc_effect.music.m = %d", fc_effect.music.m);

        fc_effect.Now_state = IS_light_music;
        set_fc_effect();
        fb_led_music_mode();
    }
    else if (fc_effect.music.m == 3)
    {
        fc_effect.Now_state = IS_light_music;
        set_fc_effect();
        fb_led_music_mode();
    }
}

/**
 * @brief  遥控设置音乐效果
 *
 */
void ls_sub_music_effect(void)
{
    if (fc_effect.music.m > 0)
    {
        fc_effect.music.m--;
        fc_effect.Now_state = IS_light_music;
        printf("fc_effect.music.m = %d", fc_effect.music.m);
        set_fc_effect();
        fb_led_music_mode();
    }
    else if (fc_effect.music.m == 0)
    {

        fc_effect.Now_state = IS_light_music;

        set_fc_effect();
        fb_led_music_mode();
    }
}

/**
 * @brief 遥控开关
 *
 */
void ls_ledStrip_switch(void)
{
    // fc_effect.on_off_flag == DEVICE_ON ? soft_turn_off_lights(): soft_turn_on_the_light();
}

// 和通信协议对应
u8 RGBsequence_map[6] =
    {
        NEO_RGB,
        NEO_RBG,
        NEO_GRB,
        NEO_GBR,
        NEO_BRG,
        NEO_BGR,
};

// 和通信协议对应
u8 RGBWsequence_map[6] =
    {
        NEO_RGBW,
        NEO_RBGW,
        NEO_GRBW,
        NEO_GBRW,
        NEO_BRGW,
        NEO_BGRW,
};
/**
 * @brief APP设置RGB顺序
 *
 * @param tp_s
 */
void app_set_RGBsequence(u8 tp_s)
{

    if (tp_s < 6)
    {
#if LED_STRIP_RGBW
        fc_effect.sequence = RGBWsequence_map[tp_s];
#elif LED_STRIP_RGB
        fc_effect.sequence = RGBsequence_map[tp_s];
#endif
        WS2812FX_init(fc_effect.led_num, fc_effect.sequence);
        fc_effect.Now_state = IS_STATIC; // 当前运行状态 静态
        set_fc_effect();
    }
}

/**
 * @brief 设置声控使用手机麦还是外部麦
 *
 * @param tp_ty
 */
void set_music_type(u8 tp_ty)
{
    tp_ty == 1 ? (fc_effect.music.m_type = EXTERIOR_MIC) : (fc_effect.music.m_type = PHONE_MIC);
}

/**
 * @brief 选择音乐效果
 *
 * @param tp_m
 */
void app_set_music_mode(u8 tp_m)
{
    if (fc_effect.music.m < MAX_MUSIC_EFFECT_NUMBER)
        fc_effect.music.m = tp_m;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

/**
 * @brief 遥控选择音乐效果
 *
 */
void ls_set_music_mode(void)
{
    fc_effect.music.m++;
    fc_effect.music.m %= MAX_MUSIC_EFFECT_NUMBER;
    fc_effect.Now_state = IS_light_music;
    set_fc_effect();
}

void ls_pause_and_play(void)
{

    if (fc_effect.Now_state == IS_light_scene)
    {
        void WS2812FX_play(void);
        void WS2812FX_pause();
        extern uint8_t _running;
        if (_running == 0)
            WS2812FX_play();
        else
            WS2812FX_pause();
    }
}

/*********************************************************
 *
 *      流星效果设置 API
 *
 *********************************************************/

// 时间递减
// sub:减数，ms
// 放在了while循环，10ms减一次
// fc_effect.meteor_period = 8;//默认8秒  周期值
// fc_effect.period_cnt = fc_effect.meteor_period*1000;  //ms,运行时的计数器 8000ms
void meteor_period_sub(void)
{

    if (fc_effect.period_cnt > 10)
    {
        fc_effect.period_cnt -= 10;
    }
    else
    {
        fc_effect.period_cnt = 0; // 计数器清零
        if (fc_effect.mode_cycle) // 模式循环完成，更新
        {
            fc_effect.period_cnt = fc_effect.meteor_period * 1000;
            fc_effect.mode_cycle = 0;
        }
    }
}

// 0:计时完成
// 1：计时中
u8 get_effect_p(void)
{
    if (fc_effect.period_cnt > 0)
        return 1;
    else
        return 0;
}

/**
 * @brief 设置流星周期时间
 *
 * @param tp_p
 */
void app_set_meteor_pro(u8 tp_p)
{

    if (tp_p >= 2 && tp_p <= 20)
    {
        fc_effect.meteor_period = tp_p;
        fc_effect.period_cnt = 0;
    }
}

/**
 * @brief 设置流星开关
 *
 * @param tp_sw
 */
void app_set_on_off_meteor(u8 tp_sw)
{

    fc_effect.star_on_off = tp_sw;
    if (fc_effect.star_on_off == DEVICE_ON)
    {

        ls_meteor_stat_effect();
    }

    else
    {
        extern void close_metemor(void);
        WS2812FX_stop();
        WS2812FX_setSegment_colorOptions(
            1,                    // 第0段
            1, fc_effect.led_num, // 起始位置，结束位置
            &close_metemor,       // 效果
            0,                    // 颜色
            fc_effect.star_speed, // 速度
            0);                   // 选项，这里像素点大小：3 REVERSE决定方向
        WS2812FX_start();
    }
}

/**
 * @brief app设置流星模式
 *
 *
 * @param tp_m
 */
void app_set_mereor_mode(u8 tp_m)
{

    if (tp_m <= 22) // void custom_meteor_effect(void)有关
    {
        fc_effect.star_index = tp_m;
        ls_meteor_stat_effect();
        printf(" fc_effect.star_index  = %d", fc_effect.star_index);
    }
}

#define MAX_STAR_SEPPD 300
/**
 * @brief APP设置流星速度
 *
 * @param tp_s
 */
void app_set_mereor_speed(u8 tp_s)
{
    if (fc_effect.star_on_off == DEVICE_OFF)
        return;
    fc_effect.app_star_speed = tp_s;
    fc_effect.star_speed = MAX_STAR_SEPPD * (100 - fc_effect.app_star_speed + 10) / 100;
    printf(" fc_effect.star_speed=%d", fc_effect.star_speed);
    ls_meteor_stat_effect();
}

u8 get_custom_index(void)
{
    return fc_effect.star_index;
}

void ls_set_star_one_two(void)
{
    if (fc_effect.star_on_off != DEVICE_ON)
        return;
    if (get_custom_index() != 19)
        app_set_mereor_mode(19); // 单流星 有掉电保存
    else
        app_set_mereor_mode(22); // 双流星
}

void ls_set_star_music(void)
{
    if (fc_effect.star_on_off != DEVICE_ON)
        return;
    if (get_custom_index() != 17)
        app_set_mereor_mode(17); // 单流星
    else
        app_set_mereor_mode(18); // 双流星
}

void ls_set_star_dir(void)
{
    if (fc_effect.star_on_off != DEVICE_ON)
        return;
    if (fc_effect.dream_scene.direction == IS_forward)
    {
        fc_effect.dream_scene.direction = IS_back;
    }
    else
    {
        fc_effect.dream_scene.direction = IS_forward;
    }
    ls_meteor_stat_effect();
}

// 30 -300

void ls_set_star_speed(void)
{
    if (fc_effect.star_on_off != DEVICE_ON)
        return;

    if (fc_effect.app_star_speed <= (100 - 10))
    {
        fc_effect.app_star_speed += 10;
    }
    else
    {
        fc_effect.app_star_speed = 10;
    }

    fc_effect.star_speed = MAX_STAR_SEPPD * (100 - fc_effect.app_star_speed + 10) / 100;

    ls_meteor_stat_effect();
    // printf("fc_effect.star_speed  = %d", fc_effect.app_star_speed);
    printf(" fc_effect.star_speed = %d", fc_effect.star_speed);
    fd_meteor_speed();
}

void ls_add_star_speed(void)
{
    if (fc_effect.star_on_off != DEVICE_ON)
        return;

    if (fc_effect.app_star_speed <= (100 - 10))
    {
        fc_effect.app_star_speed += 10;
    }
    else
    {
        fc_effect.app_star_speed = 100;
    }

    fc_effect.star_speed = MAX_STAR_SEPPD * (100 - fc_effect.app_star_speed + 10) / 100;

    ls_meteor_stat_effect();
    // printf("fc_effect.star_speed  = %d", fc_effect.app_star_speed);
    printf(" fc_effect.star_speed = %d", fc_effect.star_speed);
    fd_meteor_speed();
}

void ls_sub_star_speed(void)
{

    if (fc_effect.star_on_off != DEVICE_ON)
        return;

    if (fc_effect.app_star_speed > 10)
    {
        fc_effect.app_star_speed -= 10;
    }
    else
    {
        fc_effect.app_star_speed = 10;
    }

    fc_effect.star_speed = MAX_STAR_SEPPD * (100 - fc_effect.app_star_speed + 10) / 100;

    ls_meteor_stat_effect();
    printf("fc_effect.star_speed  = %d", fc_effect.app_star_speed);
    printf(" fc_effect.star_speed = %d", fc_effect.star_speed);
    fd_meteor_speed();
}

const u8 meteor_cycle[5] = {2, 8, 12, 16, 20}; // 2s 8s 12s 16s 20s
u8 cycle_cntt = 0;
void ls_set_star_pro(void)
{
    if (fc_effect.star_on_off != DEVICE_ON)
        return;
    app_set_meteor_pro(meteor_cycle[cycle_cntt]);
    ls_meteor_stat_effect();
    cycle_cntt++;
    if (cycle_cntt > 4)
        cycle_cntt = 0;

    printf("fc_effect.meteor_period = %d", fc_effect.meteor_period);
    fd_meteor_cycle();
}

void ls_set_star_tail(void)
{
    u8 p;
    p = get_custom_index();

    if (p == 19 || p == 20 || p == 21)
    {
        fc_effect.star_index++;
        if (fc_effect.star_index > 21)
            fc_effect.star_index = 19;
        p = fc_effect.star_index;

        app_set_mereor_mode(p); // 单流星
    }
    else
    {
        app_set_mereor_mode(19);
    }
}

/*********************************************************
 *
 *      电机效果设置 API
 *
 *********************************************************/
extern u8 motor_period[6];

#if 0 // 未使用
// void ls_set_motor_speed(void)
// {
//     static u8 start = 1;
//     if (fc_effect.star_speed_index < 6)
//     {
//         if (start)
//         {
//             start = 0;
//             one_wire_set_mode(4); // 360正转
//             enable_one_wire();
//             fc_effect.motor_on_off = DEVICE_ON;
//         }
//         else
//         {

//             one_wire_set_period(motor_period[fc_effect.star_speed_index]);
//             enable_one_wire();
//         }
//     }
//     else
//     {
//         start = 1;
//         fc_effect.motor_on_off = DEVICE_OFF;
//         one_wire_set_period(motor_period[fc_effect.star_speed_index]);
//         one_wire_set_mode(6); // 关闭电机
//         enable_one_wire();    // 启动发送电机数据
//     }

//     fc_effect.star_speed_index++;
//     if (fc_effect.star_speed_index >= 7)
//         fc_effect.star_speed_index = 0;
// }
#endif

void ls_add_motor_speed(void)
{
    // 目前是索引值越小，电机转速越快
    if (fc_effect.star_speed_index > 0)
    {
        fc_effect.star_speed_index--;
        one_wire_set_period(motor_period[fc_effect.star_speed_index]);
        // enable_one_wire();
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
        printf("fc_effect.star_speed_index = %d", fc_effect.star_speed_index);
    }
}

void ls_sub_motor_speed(void)
{
    // 目前是索引值越小，电机转速越快
    if (fc_effect.star_speed_index < 5)
    {
        fc_effect.star_speed_index++;
        one_wire_set_period(motor_period[fc_effect.star_speed_index]);
        // enable_one_wire();
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
        printf("fc_effect.star_speed_index = %d", fc_effect.star_speed_index);
    }
}

void CloseMotor(void)
{
    fc_effect.motor_on_off = DEVICE_OFF;
    one_wire_set_period(motor_period[fc_effect.star_speed_index]);
    one_wire_set_mode(0x06); // 关闭电机

    os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
    // enable_one_wire();       // 启动发送电机数据
}

// u8 mo_cnt = 3;
void OpenMortor(void)
{
    fc_effect.motor_on_off = DEVICE_ON;
    // mo_cnt = 3;
}

// void power_motor_Init(void)
// {

//     if (mo_cnt > 1)
//     {
//         mo_cnt--;

//         if (fc_effect.motor_on_off == DEVICE_ON)
//         {
//             one_wire_set_mode(4);
//             enable_one_wire();
//         }
//         else
//         {

//             one_wire_set_mode(6); // 停止电机
//             enable_one_wire();
//         }
//     }
// }

/*********************************************************
 *
 *      灯光效果设置 API
 *
 *********************************************************/

ON_OFF_FLAG get_on_off_state(void)
{
    return fc_effect.on_off_flag;
}

void set_on_off_led(u8 on_off)
{
    if (on_off == DEVICE_ON)
    {
        soft_turn_on_the_light(); // 开灯
    }
    else
    {
        soft_turn_off_lights(); // 关灯
    }
}

/**
 * @brief APP设置纯白光的颜色
 *
 * @param tp_w  0-100
 */
void app_set_w(u8 tp_w)
{
#if LED_STRIP_RGBW
    fc_effect.Now_state = IS_STATIC;
    fc_effect.rgb.r = 0;
    fc_effect.rgb.g = 0;
    fc_effect.rgb.b = 0;
    fc_effect.rgb.w = tp_w * 255 / 100;
    set_fc_effect(); // 效果调度
#endif
}

// 静态效果设置
void set_static_mode(u8 r, u8 g, u8 b)
{
    fc_effect.Now_state = IS_STATIC;
    fc_effect.rgb.r = r;
    fc_effect.rgb.g = g;
    fc_effect.rgb.b = b;
    // fc_effect.rgb.w = 0;

    // printf("r = %d, g = %d, b = %d", r, g, b);

#if LED_STRIP_RGBW

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
#endif

    set_fc_effect(); // 效果调度
}

/**
 * @brief APP设置暖白光的颜色
 *
 */
void app_set_cw(void)
{
}

void ls_add_mode_InAPP(void)
{
    u8 user_buff[3] = {0};
#if 0
    if (fc_effect.app_rgb_mode < 0x1e) // 七彩灯模式
    {
        fc_effect.app_rgb_mode++;

        user_buff[0] = 0x04;
        user_buff[1] = 0x02;
        user_buff[2] = fc_effect.app_rgb_mode;
        parse_zd_data(user_buff);
        // printf("fc_effect.app_rgb_mode = %d", fc_effect.app_rgb_mode);
    }
#endif

    if (fc_effect.app_rgb_mode < 0x1e) // 七彩灯模式
    {
        fc_effect.app_rgb_mode++;

        user_buff[0] = 0x04;
        user_buff[1] = 0x02;
        user_buff[2] = fc_effect.app_rgb_mode;
        parse_zd_data(user_buff);
        // printf("fc_effect.app_rgb_mode = %d", fc_effect.app_rgb_mode);
    }
    else if (fc_effect.app_rgb_mode < 0x1E + 22) // 流星灯模式
    {
        fc_effect.app_rgb_mode++;
        fc_effect.star_index = fc_effect.app_rgb_mode - 0x1E;
        app_set_mereor_mode(fc_effect.star_index);
    }

    printf("fc_effect.app_rgb_mode = %d", fc_effect.app_rgb_mode);
}

void ls_sub_mode_InAPP(void)
{
    u8 user_buff[3] = {0};
#if 0
    if (fc_effect.app_rgb_mode > 0)
    {
        fc_effect.app_rgb_mode--;

        user_buff[0] = 0x04;
        user_buff[1] = 0x02;
        user_buff[2] = fc_effect.app_rgb_mode;
        parse_zd_data(user_buff);
        // printf("fc_effect.app_rgb_mode = %d", fc_effect.app_rgb_mode);
    }
#endif

    if (fc_effect.app_rgb_mode > 0x1E + 1) // 流星灯模式 (流星灯的索引值不能为0，这里要加一)
    {
        fc_effect.app_rgb_mode--;
        fc_effect.star_index = fc_effect.app_rgb_mode - 0x1E;
        app_set_mereor_mode(fc_effect.star_index);
    }
    else if (fc_effect.app_rgb_mode > 0)
    {
        fc_effect.app_rgb_mode--;

        user_buff[0] = 0x04;
        user_buff[1] = 0x02;
        user_buff[2] = fc_effect.app_rgb_mode;
        parse_zd_data(user_buff);
    }

    printf("fc_effect.app_rgb_mode = %d", fc_effect.app_rgb_mode);
}

/**
 * @brief 选择app上某些效果，该些效果需要是有顺序的
 *
 * @param tp_type
 * @param tp_h
 * @param tp_t
 */
void ls_chose_mode_InAPP(u8 tp_type, u8 tp_m, u8 tp_h, u8 tp_t)
{
    u8 uc_buff[3] = {0};
    static u8 index_cmd = 0;

    if (tp_m == 0) // 循环加减
    {
        if (index_cmd < tp_t)
        {
            index_cmd++;
        }
        else
        {
            index_cmd = tp_h;
        }
    }
    else if (tp_m == 1)
    { // 只加

        if (index_cmd < tp_t)
        {

            index_cmd++;
        }
        else
        {
            index_cmd = tp_t;
        }
    }
    else if (tp_m == 2)
    { // 只减

        if (index_cmd > tp_h)
        {

            index_cmd--;
        }
        else
        {
            index_cmd = tp_h;
        }
    }
    else if (tp_m == 3)
    { // 指定某个模式

        index_cmd = tp_h;
    }

    if (tp_type == 1)
    {
        uc_buff[0] = 0x04;
        uc_buff[1] = 0x02;
        uc_buff[2] = index_cmd;
    }
    printf_buf(uc_buff, 3);
    parse_zd_data(uc_buff);
}

/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/

// 遥控一共10个自动mode
#define IR_MODE_QTY 10
/* 定义声控模式数量 */
// 输入ir_scan_mode，用于遥控自动模式，模式+，模式-
const u8 ir_mode[IR_MODE_QTY] = {0, 1, 4, 5, 6, 15, 12, 7, 8, 9};

//  计数器，遥控自动模式，模式+,-计数
u8 ir_mode_cnt;
_AUTO_T ir_auto_f = IS_PAUSE;
// 自动模式下，改变模式计数器
u16 ir_auto_change_tcnt = 0;
/* 定时器状态 */
AUTO_TIME_T ir_timer_state;
/* 定时计数器,累减,单位S */
u16 ir_time_cnt;
/* 本地声控模式index */
u8 ir_local_mic_index = 0;
/* 灵敏度：1-100 */
u8 ir_sensitive = 0;
/* 定时器周期1秒，单位1ms */
#define IR_TIMER_UNIT 1000
#define IR_CHANGE_MODE_T (10 * 1000) // 10秒换一个模式

u8 auto_mode[3] = {0x04, 0x02, 0x07};
//--------------------------------------------------自动
/* 设置自动开与关 */
void set_ir_auto(_AUTO_T auto_f)
{
    /* 检测合法性，和有变化 */
    if (auto_f <= IS_PAUSE && fc_effect.auto_f != auto_f)
    {

        fc_effect.auto_f = auto_f;
        printf("\r set_ir_auto OK");
    }
    else
    {
    }
}

void auto_Function_Switch(void)
{
    fc_effect.auto_f = !fc_effect.auto_f;
}
/* 获取自动状态 */
_AUTO_T get_ir_auto(void)
{
    return fc_effect.auto_f;
}

// 返回遥控器当前操作的模式
u8 get_ir_mode(void)
{
    return ir_mode[ir_mode_cnt];
}
// 每次调用，循环递增模式，到IR_MODE_QTY从0开始
void ir_mode_plus(void)
{
    ir_mode_cnt++;
    ir_mode_cnt %= IR_MODE_QTY;
}

// 每次调用，循环递减模式，到0从IR_MODE_QTY开始
void ir_mode_sub(void)
{
    if (ir_mode_cnt > 0)
    {
        ir_mode_cnt--;
    }
    else
    {
        ir_mode_cnt = IR_MODE_QTY;
    }
}

/* 设置定时关机，按OFF取消 */
void set_ir_timer(AUTO_TIME_T timer)
{
    /* 检测合法性，和有变化 */
    if (timer <= IR_TIMER_120MIN && ir_timer_state != timer)
    {
        ir_timer_state = timer;
        ir_time_cnt = timer / IR_TIMER_UNIT;
    }
}

AUTO_TIME_T get_ir_timer(void)
{
    return ir_timer_state;
}

static void ir_auto_change_mode(void)
{

    extern u8 ws2811fx_set_cycle;

    // 循环模式
    if (fc_effect.auto_f == IS_AUTO)
    {

        if (ir_auto_change_tcnt != 0)
        {

            ir_auto_change_tcnt -= 10;
        }

        if (ws2811fx_set_cycle == 1)
        {
            ws2811fx_set_cycle = 0;
            ir_auto_change_tcnt = IR_CHANGE_MODE_T;
            if (auto_mode[2] < 0x15)
            {
                auto_mode[2] += 1;
            }
            else
            {
                auto_mode[2] = 7;
            }
            printf("aotu change");
            parse_zd_data(auto_mode);
        }
    }
}

// 10ms调用一次
void ir_timer_handler(void)
{
    ir_auto_change_mode();
}

// 全彩效果初始化
void full_color_init(void)
{
    WS2812FX_init(fc_effect.led_num, fc_effect.sequence); // 初始化ws2811
    WS2812FX_setBrightness(fc_effect.b);
    set_on_off_led(fc_effect.on_off_flag);

    extern void count_down_run(void);
    extern void time_clock_handler(void);
    // sys_s_hi_timer_add(NULL, count_down_run, 10); // 注册按键扫描定时器
    // sys_s_hi_timer_add(NULL, ir_timer_handler, 10);   // 注册按键扫描定时器
    // sys_s_hi_timer_add(NULL, time_clock_handler, 10); // 注册按键扫描定时器
    sys_s_hi_timer_add(NULL, meteor_period_sub, 10); // 注册按键扫描定时器
}
