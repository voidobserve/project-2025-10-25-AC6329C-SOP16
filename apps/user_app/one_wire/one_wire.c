// 中道单线通讯
// 控制步进电机
#include "system/includes.h"
#include "one_wire.h"
#include "led_strand_effect.h"

volatile u16 send_base_ins = 0;
u8 motor_period[6] = {8, 13, 18, 21, 26, 35}; // 转速  app指令，需要将8 13 18 21 26 转换成相应的16进制

// 定义与驱动电机ic通信的引脚
#define MOTOR_DATA_IO_PORT IO_PORTB_06

/**
 * @brief  mcu通讯接口
 *
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
void pack_base(void)
{
    u8 p;
    send_base_ins = 0;
    send_base_ins |= fc_effect.base_ins.mode; // bit0 ~ bit2 电机模式


    // 验证速度值范围是否正确
    {
        for (p = 0; p < 6; p++)
        {
            if (motor_period[p] == fc_effect.base_ins.period)
            {
                break;
            }
        }

        if (p > 5)
        {
            p = 0;
        }

        send_base_ins |= ((u16)motor_period[p] << 8); // bit8 ~ bit15，电机速度
    }

    if (fc_effect.base_ins.dir)
    {
        send_base_ins |= BIT(6); // bit6 0:正转，1:反转
    }

    // bit7 0：开灯，1：关灯
    // send_base_ins &= ~(0x01 << 7); // 实际测试是 关灯
    // send_base_ins |= (0x01 << 7); // 实际测试是 开灯

    // printf("send_base_ins == 0x %x\n", (u16)send_base_ins);
}

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

u8 is_one_wire_send_end(void)
{
    return send_en;
}

void one_wire_send_enable(void)
{
    send_en = 1;
}

/**
 * @brief 构造单线通讯协议，16bit  构造波形
 *          从dat的最低位开始发送
 *
 * @param dat
 */
// void __attribute__((weak)) make_one_wire(volatile u16 dat )
static volatile u8 flag_is_just_begin = 1; // 标志位，是否刚开始进入发送；0--否，1--是
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
void enable_one_wire(void)
{
    send_en = 0;
    flag_is_just_begin = 1;
    pack_base(); // 打包数据
    send_en = 1;

    // if (0 != step)
    // {
    //     printf("sending\n");
    // }
    // else
    // {
    //     printf("begin send\n");
    // }
}

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
 * @param m       // 000:回正
                 // 001:区域1摇摆
                // 010:区域2摇摆
                // 011:区域1和区域2摇摆
                // 100:360°正转
               // 101:音乐律动
               //110：停止
 */

void one_wire_set_mode(u8 m)
{
    fc_effect.base_ins.mode = m;
    printf("base_ins.mode = %d", fc_effect.base_ins.mode);
}
/**
 * @brief 设置电机转速
 *
 * @param p ：8s 13s 18s 21s 26s
 */
void one_wire_set_period(u8 p)
{
    if (fc_effect.base_ins.mode == 0x05)
    { // 如果是声控模式，直接返回
        return;
    }
    fc_effect.base_ins.period = p;

    printf("base_ins.period = %d", fc_effect.base_ins.period);
}
/**
 * @brief 设置电机正反转
 *
 */
void one_wire_set_dir(void)
{
    printf("base_ins.dir = %d", fc_effect.base_ins.dir);
    fc_effect.base_ins.dir = !fc_effect.base_ins.dir;
}
/**
 * @brief Get the stepmotor mode object
 * 获取电机当前模式
 *
 * @return u8
 */
u8 get_stepmotor_mode(void)
{
    printf(" base_ins.mode = %d", fc_effect.base_ins.mode);
    return fc_effect.base_ins.mode;
}

/*************************************音乐律动模式**************************************/

/**
 * @brief 效果：反转
 *
 */
void stepmotor_direction(void)
{
    // one_wire_set_dir();
    fc_effect.base_ins.dir = 1;
}

/**
 * @brief 效果：调到最慢速度
 *
 */
void stepmotor_music_minSpeed(void)
{

    fc_effect.base_ins.dir = 0;
    fc_effect.base_ins.period = 26;
}

/**
 * @brief 效果：调到最快速度
 *
 */
void stepmotor_music_maxSpeed(void)
{
    fc_effect.base_ins.dir = 0;
    fc_effect.base_ins.period = 8;
}

void set_stepmotor_music_mode(void)
{

    switch (fc_effect.base_ins.music_mode)
    {
    case 0:

        stepmotor_direction();
        // enable_one_wire(); // 使用发送数据
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
        break;
    case 1:

        stepmotor_music_minSpeed();
        // enable_one_wire(); // 使用发送数据
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
        break;
    case 2:

        stepmotor_music_maxSpeed();
        // enable_one_wire(); // 使用发送数据
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
        break;
    }
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
        // enable_one_wire(); // 使用发送数据
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
        // enable_one_wire(); // 使用发送数据
        os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
    }
}

/**
 * @brief 声控步进电机
 *
 */

u8 stepmotor_sound_cnt = 0;

void effect_stepmotor(void)
{

    if (fc_effect.base_ins.mode == 0x05)
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

u8 counting_flag = 0; // 1：计时中， 0：计时完成
u16 stop_cnt = 0;
u8 set_time = 0; // 1：不允许修改时间  0：允许修改时间
u8 temp = 0;
void clean_stepmorot_flag(void)
{
    counting_flag = 0;
    stop_cnt = 0;
    set_time = 1;
}
// 10计时
void stepmotor(void)
{

    if (set_time == 1)
    {

        set_time = 0;
        temp = fc_effect.base_ins.period;
        if (fc_effect.on_off_flag == DEVICE_OFF)
            temp = 0;
    }
    if (counting_flag == 1)
    {

        if (stop_cnt == temp * 100)
        {

            one_wire_set_mode(6);
            // enable_one_wire();
            os_taskq_post("msg_task", 1, MSG_SEQUENCER_ONE_WIRE_SEND_INFO);
            clean_stepmorot_flag();
        }
        else
        {
            stop_cnt++;
        }
    }
}

void motor_Init(void)
{
    counting_flag = 0; // 无霍尔时，电机
    set_time = 1;
}
