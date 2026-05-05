// 中道单线通讯
// 控制步进电机
#include "system/includes.h"
#include "one_wire.h"
#include "led_strand_effect.h"
#include "led_strip_voice.h"
#include "user_include.h"

static volatile u16 send_base_ins = 0;
u8 motor_period[6] = {8, 13, 18, 21, 26, 35}; // 转速  app指令，需要将 8 13 18 21 26 转换成相应的16进制

static volatile u16 motor_mode_data = 0;

// ======================================================================
// 发送数据期间，使用到的各个变量和配置：
static volatile u8 flag_is_just_begin = 1; // 标志位，是否刚开始进入发送；0--否，1--是
// #define INS_LEN (7) // 指令长度
// #define INS_LEN (16 - 1) // 指令长度
#define INS_LEN (16) // 指令长度
#define W_0_5MS 4    // 脉宽0.5ms
#define W_1MS 8
#define W_2MS 16
static volatile u8 send_cnt = 0;
static volatile u8 step = 0;       // 控制发送阶段的状态机
static volatile u8 _125ms_cnt = 0; // 125us
static volatile u8 h_l = 0;        // 0:输出低电平，1：高电平
static volatile u8 send_en = 0;    // 0:不发送， 1：发送   使能变量
static volatile u16 count_ = 0;
// ======================================================================

/**
 * @brief  mcu通讯接口
 *
 */
void mcu_com_init(void)
{
    gpio_set_die(MOTOR_DATA_IO_PORT, 1);
    gpio_set_pull_up(MOTOR_DATA_IO_PORT, 1);
    gpio_direction_output(MOTOR_DATA_IO_PORT, 1);
}

/**
 * @brief 打包基本数据  数据包
 *
 */
// void pack_base(void)
// {
// #if 0

//     u8 p;
//     send_base_ins = 0;
//     send_base_ins |= fc_effect.base_ins.mode; // bit0 ~ bit2 电机模式

//     // 验证速度值范围是否正确
//     {
//         for (p = 0; p < 6; p++)
//         {
//             if (motor_period[p] == fc_effect.base_ins.period)
//             {
//                 break;
//             }
//         }

//         if (p > 5)
//         {
//             p = 0;
//         }

//         send_base_ins |= ((u16)motor_period[p] << 8); // bit8 ~ bit15，电机速度
//     }

//     if (fc_effect.base_ins.dir)
//     {
//         send_base_ins |= BIT(6); // bit6 0:正转，1:反转
//     }
// #endif

//     send_base_ins = 0;

//     // 速度值
//     send_base_ins |= ((u16)fc_effect.base_ins.period << 8); // bit8 ~ bit15，电机速度

//     switch (fc_effect.base_ins.mode)
//     {
//     case MOTOR_MODE_STOP:
//         // send_base_ins &= ~(0x07 << 0); // 0b-000 停止
//         break;
//     case MOTOR_MODE_FORWARD:
//         send_base_ins |= 0x01 << 2; // 0b-100 360度转动
//         // send_base_ins &= ~(0x01 << 6); // bit6 0:正转
//         break;
//     case MOTOR_MODE_REVERSE:
//         send_base_ins |= (0x01 << 2 | 0x01 << 0); //
//         send_base_ins |= 0x01 << 6;               // bit6 1：反转
//         break;
//     case MOTOR_MODE_FORWARD_REVERSE:
//         // 正反转，由程序控制
//         break;
//     case MOTOR_MODE_MUSIC_RULATION:
//         // 音乐律动，由程序控制
//         break;
//     }

//     // bit7 0：开灯，1：关灯
//     // send_base_ins &= ~(0x01 << 7); // 实际测试是 关灯
//     // send_base_ins |= (0x01 << 7); // 实际测试是 开灯

//     // printf("send_base_ins == 0x %x\n", (u16)send_base_ins);
// }

u8 is_one_wire_send_end(void)
{
    return send_en;
}

void one_wire_send_enable(void)
{
    flag_is_just_begin = 1;
    send_en = 1;
}

void one_wire_send_disable(void)
{
    send_en = 0;
}

/**
 * @brief 构造单线通讯协议，16bit  构造波形
 *          从dat的最低位开始发送
 *
 * @param dat
 */
void __attribute__((weak)) make_one_wire(void)
{
    static volatile u16 dat = 0;

    if (send_en == 0)
    {
        flag_is_just_begin = 1;
        return;
    }

    if (flag_is_just_begin)
    {
        flag_is_just_begin = 0;
        count_ = 0;
        h_l = 0;
        send_cnt = 0;
        step = 0;

        dat = send_base_ins; // 每次发送只获取一次数据，避免重复获取
        // printf("just begin\n");
    }

    // printf("dat == 0x %x\n", (u16)dat); // 这里打印看不出问题

    switch (step)
    {
    case 0: // 起始位
        /***********************************************************/
        // 解决了app发送指令的，波形不正确的问题，但是问题愿意未清晰
        if (count_ <= 40) // 2.5ms
        {
            count_++;
            return;
        }
        /**********************************************************/
        if (h_l == 0)
        {
            gpio_direction_output(MOTOR_DATA_IO_PORT, 0);
            _125ms_cnt++;

            //++_125ms_cnt;
            if (_125ms_cnt > (W_1MS)) // 从1开始
            {
                gpio_direction_output(MOTOR_DATA_IO_PORT, 1);

                h_l = 1;
                _125ms_cnt = 0;
            }
        }
        else
        {
            // gpio_direction_output(MOTOR_DATA_IO_PORT, 1);
            _125ms_cnt++;

            if (_125ms_cnt == W_1MS)
            {
                gpio_direction_output(MOTOR_DATA_IO_PORT, 0);
                h_l = 0;
                step = 1;
                _125ms_cnt = 0;
            }
        }
        break;

    case 1: // 发送中
        if (h_l == 0)
        {
            // gpio_direction_output(MOTOR_DATA_IO_PORT, 0);
            _125ms_cnt++;

            if (_125ms_cnt == W_0_5MS)
            {

                gpio_direction_output(MOTOR_DATA_IO_PORT, 1);
                h_l = 1;
                _125ms_cnt = 0;
            }
        }
        else
        {
            if ((dat >> send_cnt) & 0x01) // 1
            {
                // gpio_direction_output(MOTOR_DATA_IO_PORT, 1);
                _125ms_cnt++;

                if (_125ms_cnt == W_1MS)
                {
                    gpio_direction_output(MOTOR_DATA_IO_PORT, 0);

                    h_l = 0;
                    _125ms_cnt = 0;
                    // 完成1bit发送
                    send_cnt++;
                    if (send_cnt == INS_LEN)
                    {
                        send_cnt = 0;
                        step = 2;
                    }
                }
            }
            else
            {
                // gpio_direction_output(MOTOR_DATA_IO_PORT, 1);
                _125ms_cnt++;
                if (_125ms_cnt == W_0_5MS)
                {
                    gpio_direction_output(MOTOR_DATA_IO_PORT, 0);
                    h_l = 0;
                    _125ms_cnt = 0;
                    // 完成1bit发送
                    send_cnt++;
                    if (send_cnt == INS_LEN)
                    {
                        send_cnt = 0;
                        step = 2;
                    }
                }
            }
        }

        break;

    case 2: // 发送结束
        if (h_l == 0)
        {
            gpio_direction_output(MOTOR_DATA_IO_PORT, 0);
            _125ms_cnt++;
            if (_125ms_cnt == W_2MS)
            {
                gpio_direction_output(MOTOR_DATA_IO_PORT, 1);

                _125ms_cnt = 0;
                step = 0;
                send_cnt = 0;
                send_en = 0;            // 等待下一次触发
                flag_is_just_begin = 1; // 发送完成，清除该标志位，等待下一次发送

                // printf("send end\n");
            }
        }
        break;
    }
    //  gpio_direction_output(IO_PORTA_01, 0);
}

// 数据发送使能
// void enable_one_wire(void)
// {
//     // send_en = 0; // 不让发送中断继续发送
//     // pack_base(); // 打包数据
//     // flag_is_just_begin = 1;
//     // send_en = 1;

//     one_wire_send_disable();
//     motor_mode_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
//     send_base_ins = motor_mode_data; // send_base_ins ， 最终要发送给电机ic的数据
//     one_wire_send_enable();
// }

/**
 * @brief 125ms调用一次  放在定时器使用
 *
 */
// AT_VOLATILE_RAM_CODE
void one_wire_send(void)
{
    /*
        发送未使能，不发送

        测试发现，即使准备发送的数据打印出来没有问题，
        实际观察发现电机转速变了

        可能是中断导致数组访问冲突，还未准备完数据就已经有中断
    */
    if (!is_one_wire_send_end())
    {
        return;
    }

    make_one_wire();
}

// -------------------------------------------------------------------API
/**
 * @brief 设置电机的模式
 *
 * @param mode
 */
void one_wire_set_mode(motor_mode_t mode)
{
    fc_effect.base_ins.mode = mode;
#if USER_DEBUG_ENABLE
    printf("base_ins.mode = %u\n", (u16)fc_effect.base_ins.mode);
#endif
}

/**
 * @brief 设置电机转速
 *
 * @param p ：8s 13s 18s 21s 26s
 */
void one_wire_set_period(u8 p)
{
    // if (fc_effect.base_ins.mode == 0x05)
    // { // 如果是声控模式，直接返回
    //     return;
    // }
    fc_effect.base_ins.period = p;

#if USER_DEBUG_ENABLE
    printf("base_ins.period = %d\n", fc_effect.base_ins.period);
#endif
}

/**
 * @brief 设置电机正反转
 *
 */
// void one_wire_set_dir(void)
// {
//     printf("base_ins.dir = %d\n", fc_effect.base_ins.dir);
//     fc_effect.base_ins.dir = !fc_effect.base_ins.dir;
// }

/*************************************音乐律动模式**************************************/

/**
 * @brief 效果：反转
 *
 */
// void stepmotor_direction(void)
// {
//     // one_wire_set_dir();
//     fc_effect.base_ins.dir = 1;
// }

/**
 * @brief 效果：调到最慢速度
 *
 */
void stepmotor_music_minSpeed(void)
{

    // fc_effect.base_ins.dir = 0;
    fc_effect.base_ins.period = 26;
}

/**
 * @brief 效果：调到最快速度
 *
 */
void stepmotor_music_maxSpeed(void)
{
    // fc_effect.base_ins.dir = 0;
    fc_effect.base_ins.period = 8;
}

/**
 * @brief 设置是最慢
 *
 */
void set_stepmotor_slow(void)
{
    if (fc_effect.base_ins.period != 26)
    {
        stepmotor_music_minSpeed();
        motor_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
    }
}

/**
 * @brief 设置是最快
 *
 */
void set_stepmotor_fast(void)
{
    if (fc_effect.base_ins.period != 8)
    {
        stepmotor_music_maxSpeed();
        motor_package_data(fc_effect.base_ins.mode, fc_effect.base_ins.period);
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
    }
}

/**
 * @brief 声控步进电机
 *
 */
void effect_stepmotor(void)
{
    static u8 stepmotor_sound_cnt = 0; // 超时计数值

    if (fc_effect.base_ins.mode == MOTOR_MODE_MUSIC_RULATION)
    {

        if (get_sound_triggered_by_motor())
        {
            set_stepmotor_fast();
            // printf("1111\n");
            stepmotor_sound_cnt = 0;
        }

        if (stepmotor_sound_cnt < 100)
        {
            stepmotor_sound_cnt++;
        }

        if (stepmotor_sound_cnt >= 100)
        {
            // printf("2222\n");
            set_stepmotor_slow();
        }
    }
}

/**
 * @brief 准备要发送给电机ic的数据
 *
 * @param mode 只支持停止、正转、反转，其他模式需要结合当前函数和对应的处理函数来处理
 *          MOTOR_MODE_STOP
 *          MOTOR_MODE_FORWARD
 *          MOTOR_MODE_REVERSE
 * @param speed_val 速度值，0~255
 */
void motor_package_data(motor_mode_t mode, u8 speed_val)
{
    motor_mode_data = 0;

    // 速度值
    motor_mode_data |= ((u16)speed_val << 8); // bit8 ~ bit15，电机速度

    switch (mode)
    {
    case MOTOR_MODE_STOP:
        // motor_mode_data &= ~(0x07 << 0); // 0b-000 停止
        break;
    case MOTOR_MODE_FORWARD:
        motor_mode_data |= 0x01 << 2; // 0b-100 360度转动
        // motor_mode_data &= ~(0x01 << 6); // bit6 0:正转
        break;
    case MOTOR_MODE_REVERSE:
        // 由于电机ic的反转模式只在声控模式的前提下有效，这里需要设置电机的模式为声控模式
        motor_mode_data |= (0x01 << 2 | 0x01 << 0);
        motor_mode_data |= 0x01 << 6; // bit6 1:反转
        break;
    }
}

/**
 * @brief 完成一次数据的发送
 *
 * @attention 内部调用了 os_time_dly ，不能在时间敏感的任务中调用
 */
void motor_send_data(void)
{
    // 防止电机ic接收时丢失了数据，这里要多发送几次
    for (u8 i = 0; i < 3; i++)
    {
        while (is_one_wire_send_end())
        {
            // 如果之前的数据没有发送完成，等待发送完成
            os_time_dly(1);
        }

        one_wire_send_disable();
        send_base_ins = motor_mode_data;
#if USER_DEBUG_ENABLE
        // printf("motor_mode_data == %x\n", motor_mode_data);
#endif
        one_wire_send_enable();
    }
}

// 控制正反转时使用到的状态机
enum
{
    MOTOR_MODE_FORWARD_REVERSE_BEGIN = 0, // 刚开始进入正反转，先正转
    MOTOR_MODE_FORWARD_REVERSE_HANDLING,  // 正在处理正反转
};

/**
    电机模式处理，10ms调用一次

    @attention 目前只用在控制电机正反转模式中
*/

// 电机正反转的处理函数，每10ms调用一次
void motor_forward_reverse_mode_handle(void)
{
    static volatile u32 time_cnt = 0; // 计数器
    // static volatile u8 dir = 0;       // 电机转动方向，0：正转，1：反转
    static volatile u8 mode_state = MOTOR_MODE_FORWARD_REVERSE_BEGIN;
    if (fc_effect.base_ins.mode != MOTOR_MODE_FORWARD_REVERSE)
    {
        time_cnt = 0;
        mode_state = MOTOR_MODE_FORWARD_REVERSE_BEGIN;
        fc_effect.base_ins.dir_in_mode_forward_reverse = 0;
        return;
    }

    if (mode_state == MOTOR_MODE_FORWARD_REVERSE_BEGIN)
    {
        // 刚进入正反转的模式，先让电机正转

        motor_package_data(MOTOR_MODE_FORWARD, fc_effect.base_ins.period);
        motor_send_data();

        fc_effect.base_ins.dir_in_mode_forward_reverse = 0;
        mode_state = MOTOR_MODE_FORWARD_REVERSE_HANDLING;
    }

    time_cnt++;
    if (time_cnt >= (u16)((u32)10 * 1000 / 10)) // 10 * 1000 ms / 10ms(函数调用周期)
    {
        time_cnt = 0;
        fc_effect.base_ins.dir_in_mode_forward_reverse = !fc_effect.base_ins.dir_in_mode_forward_reverse;

        // motor_package_data(dir ? MOTOR_MODE_FORWARD : MOTOR_MODE_REVERSE,
        //                    fc_effect.base_ins.period);
        motor_package_data(
            fc_effect.base_ins.dir_in_mode_forward_reverse
                ? MOTOR_MODE_REVERSE
                : MOTOR_MODE_FORWARD,
            fc_effect.base_ins.period);
        motor_send_data();
    }
}

// 声控模式下，电机快速转动的超时时间
// 由于处理函数是10ms调用一次，这里要除以10
#define MOTOR_TIMEOUT_CNT_IN_MUSIC_RULATION (2000 / 10)
// 电机声控模式下，对应的处理函数
// 目前与 fc_effect.base_ins.period 无关，只与 fc_effect.base_ins.sensitivity 有关
void motor_music_rulation_mode_handle(void)
{
    static volatile u8 timeout_cnt = 0; // 超时计数器
    /*
        是否发送了最小速度
        （不处于声控模式、检测到了声控信号，都要给这个变量清零）：
    */
    static volatile u8 is_send_slowest_speed = 0;

    if (fc_effect.base_ins.mode != MOTOR_MODE_MUSIC_RULATION)
    {
        // 不在声控模式，直接返回
        is_send_slowest_speed = 0;
        timeout_cnt = 0;
        return;
    }

    if (sound_triggered_by_motor_get())
    {

        sound_triggered_by_motor_clear();

        if (is_send_slowest_speed)
        {
            /*
                有声控信号，并且当前速度为最慢
            */
            is_send_slowest_speed = 0;
            motor_package_data(MOTOR_MODE_FORWARD, 8);
            os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
        }

        // 有声控信号，重置超时时间
        timeout_cnt = 0;
    }

    if (timeout_cnt < MOTOR_TIMEOUT_CNT_IN_MUSIC_RULATION)
    {
        timeout_cnt++;
    }
    else
    {
        // 超时，把电机速度设置为最慢

        // 超时之后，只发送一次
        if (is_send_slowest_speed == 0)
        {
            motor_package_data(MOTOR_MODE_FORWARD, 35);
            os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
            is_send_slowest_speed = 1;
        }
    }

#if 0
    // 超时之后，再检测有没有声控信号，否则一旦有声控信号，就会给电机ic发送数据
    // if (timeout_cnt >= 200 && get_sound_triggered_by_motor())
    if (timeout_cnt >= 200 && sound_triggered_by_motor_get())
    {
#if USER_DEBUG_ENABLE
        // printf("sound triggered by motor\n");
#endif
        sound_triggered_by_motor_clear();

        // 检测到声控信号，把电机速度暂时设置为最快
        motor_package_data(MOTOR_MODE_FORWARD, 8);
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);

        timeout_cnt = 0;
        is_send_slowest_speed = 0;
    }

    // if (timeout_cnt < 100)
    // 200，10ms调用一次该函数，如果有声控信号，这里会让电机快速转2s
    if (timeout_cnt < 200)
    {
        timeout_cnt++;
    }
    else
    {
        // 超时，把电机速度设置为最慢

        // 超时之后，只发送一次
        if (is_send_slowest_speed == 0)
        {
            motor_package_data(MOTOR_MODE_FORWARD, 35);
            os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
            is_send_slowest_speed = 1;
        }
    }
#endif
}
