#include "led_strip_voice.h"
#include "asm/adc_api.h"
#include "led_strip_drive.h"

#include "../../../apps/user_app/ws2812-fx-lib/WS2812FX_C/WS2812FX.H"

static volatile u8 flag_sound_triggered_in_colorful_lights = 0; // 标志位，七彩灯触发声控。0--未触发，1--触发
static volatile u8 flag_sound_triggered_in_meteor_lights = 0;   // 标志位，流星灯触发声控。0--未触发，1--触发

static volatile u8 flag_sound_triggered_in_motor = 0; // 标志位，电机声控模式下，触发声控，0--未触发，1--触发

// 获取七彩灯的声控结果
u8 get_sound_triggered_by_colorful_lights(void)
{
    u8 ret = flag_sound_triggered_in_colorful_lights;
    flag_sound_triggered_in_colorful_lights = 0;
    return ret;
}

// 获取流星灯的声控结果
u8 get_sound_triggered_by_meteor_lights(void)
{
    u8 ret = flag_sound_triggered_in_meteor_lights;
    flag_sound_triggered_in_meteor_lights = 0;
    return ret;
}

// 电机声控模式下，获取声控结果
u8 get_sound_triggered_by_motor(void)
{
    u8 ret = flag_sound_triggered_in_motor;
    flag_sound_triggered_in_motor = 0;
    return ret;
}

/**
 * @brief 七彩灯声控模式下的灵敏度 增加
 *      由遥控器调节时调用
 *
 */
void colorful_lights_sound_sensitivity_add(void)
{
    if (IS_light_music != fc_effect.Now_state)
    {
        return;
    }

    const u8 step = 10;
    if (fc_effect.colorful_lights_sensitivity < 100 - step)
    {
        fc_effect.colorful_lights_sensitivity += step;
    }
    else
    {
        fc_effect.colorful_lights_sensitivity = 100;
    }

    printf("fc_effect.colorful_lights_sensitivity %u\n", (u16)fc_effect.colorful_lights_sensitivity);
}

/**
 * @brief 七彩灯声控模式下的灵敏度 减少
 *      由遥控器调节时调用
 *
 */
void colorful_lights_sound_sensitivity_sub(void)
{
    if (IS_light_music != fc_effect.Now_state)
    {
        return;
    }

    const u8 step = 10;
    if (fc_effect.colorful_lights_sensitivity > step)
    {
        fc_effect.colorful_lights_sensitivity -= step;
    }
    else
    {
        fc_effect.colorful_lights_sensitivity = 0;
    }

    printf("fc_effect.colorful_lights_sensitivity %u\n", (u16)fc_effect.colorful_lights_sensitivity);
}

/**
 * @brief 流星灯声控模式下的灵敏度 增加
 *       由遥控器调节时调用
 *
 */
void meteor_lights_sound_sensitivity_add(void)
{
    const u8 step = 10;
    if (fc_effect.meteor_lights_sensitivity < 100 - step)
    {
        fc_effect.meteor_lights_sensitivity += step;
    }
    else
    {
        fc_effect.meteor_lights_sensitivity = 100;
    }

    printf("fc_effect.meteor_lights_sensitivity %u\n", (u16)fc_effect.meteor_lights_sensitivity);
}

/**
 * @brief 流星灯声控模式下的灵敏度 减少
 *       由遥控器调节时调用
 *
 */
void meteor_lights_sound_sensitivity_sub(void)
{
    const u8 step = 10;
    if (fc_effect.meteor_lights_sensitivity > step)
    {
        fc_effect.meteor_lights_sensitivity -= step;
    }
    else
    {
        fc_effect.meteor_lights_sensitivity = 0;
    }

    printf("fc_effect.meteor_lights_sensitivity %u\n", (u16)fc_effect.meteor_lights_sensitivity);
}

void motor_sound_sensitivity_add(void)
{
    const u8 step = 10;
    if (fc_effect.base_ins.sensitivity < 100 - step)
    {
        fc_effect.base_ins.sensitivity += step;
    }
    else
    {
        fc_effect.base_ins.sensitivity = 100;
    }

    printf("motor sensitivity %u\n", (u16)fc_effect.base_ins.sensitivity);
}

void motor_sound_sensitivity_sub(void)
{
    const u8 step = 10;
    if (fc_effect.base_ins.sensitivity > 0 + step)
    {
        fc_effect.base_ins.sensitivity -= step;
    }
    else
    {
        fc_effect.base_ins.sensitivity = 0;
    }

    printf("motor sensitivity %u\n", (u16)fc_effect.base_ins.sensitivity);
}

void sound_handle(void)
{

#if 1 // 移植其他项目的声控程序

    if (fc_effect.on_off_flag != DEVICE_ON)
    {
        return;
    }

#define SAMPLE_N 20
    static volatile u32 adc_sum = 0;
    static volatile u32 adc_sum_n = 0;
    static volatile u8 adc_v_n = 0;
    static volatile u8 adc_avrg_n = 0;
    static volatile u16 adc_v[SAMPLE_N] = {0};
    static volatile u32 adc_avrg[10] = {0}; // 记录5个平均值
    static volatile u32 adc_total[15] = {0};
    // u8 trg = 0;
    u8 trg_v = 0;
    volatile u16 adc = 0;
    u32 adc_all = 0;
    u32 adc_ttl = 0;

    // 设备未开启，或是状态不是音乐模式，直接退出
    // if (fc_effect.on_off_flag != DEVICE_ON ||                        // 设备未开启
    //     (fc_effect.Now_state != IS_light_music &&                    // 七彩灯不处于声控模式
    //      fc_effect.star_index != 17 && fc_effect.star_index != 18 && // 流星灯不处于声控模式
    //      fc_effect.base_ins.mode != 0x05)                            // 电机不处于声控模式
    // )
    // {
    //     return;
    // }

    // 记录adc值
    adc = check_mic_adc(); // 每次进入，采集一次ad值（即使不在声控模式，也会占用一些时间）

    // printf("adc = %d", adc);

    if (adc >= 1000)
    {
        return;
    }

    // if (adc < 1000) // 当ADC值大于1000，说明硬件电路有问题
    if (adc_sum_n < 2000)
    {
        // 从0开始，一直加到2000，每10ms加一，总共要20s
        adc_sum_n++;
    }

    if (adc_sum_n == 2000)
    {
        if (adc / (adc_sum / adc_sum_n) > 3)
            return; // adc突变，大于平均值的3倍，丢弃改值

        adc_sum = adc_sum - adc_sum / adc_sum_n;
    }

    adc_sum += adc; // 累加adc值

    adc_v_n %= SAMPLE_N;
    adc_v[adc_v_n] = adc;
    adc_v_n++;
    adc_all = 0;

    // 计算ad值总和
    for (u8 i = 0; i < SAMPLE_N; i++)
    {
        adc_all += adc_v[i];
    }

    // 获取ad值平均值
    adc_avrg_n %= 10;
    adc_avrg[adc_avrg_n] = adc_all / SAMPLE_N;
    adc_avrg_n++;
    adc_ttl = 0;

    // 在平均值的基础上，再求总和
    for (u8 i = 0; i < 10; i++)
    {
        adc_ttl += adc_avrg[i];
    }

    memmove((u8 *)adc_total, (u8 *)adc_total + 4, 14 * 4); // 将 src 指向的内存区域中的前 n 个字节复制到 dest 指向的内存区域（能够安全地处理内存重叠的情况）

    adc_total[14] = adc_ttl / 10; // 总数平均值
    // trg = 0;

    if (adc_sum_n != 0)
    {
        if (adc * fc_effect.music.s / 100 > adc_sum / adc_sum_n)
        {

            u32 adc_sum_avrg = adc_sum / adc_sum_n;
            if (adc * fc_effect.colorful_lights_sensitivity / 100 > adc_sum_avrg)
            {

                if (fc_effect.Now_state == IS_light_music)
                {
                    // 如果七彩灯处于声控模式，会进入这里
                    flag_sound_triggered_in_colorful_lights = 1;
                    WS2812FX_triggered_by_colorful_lights();

                    // printf("trigger_by_colorful_lights\n");
                }
            }

            if (adc * fc_effect.base_ins.sensitivity / 100 > adc_sum_avrg)
            { 
                if (5 == fc_effect.base_ins.mode)
                {
                    flag_sound_triggered_in_motor = 1;
                }
            }

            if (adc * fc_effect.meteor_lights_sensitivity / 100 > adc_sum_avrg)
            {
                // 如果流星灯在声控模式，并且触发了声控
                // if (DEVICE_ON == fc_effect.star_on_off &&
                //     (STAR_INDEX_METEOR_MUSIC_CONTROL == fc_effect.star_index ||
                //      STAR_INDEX_METEOR_MUSIC_CONTROL_2 == fc_effect.star_index ||
                //      STAR_INDEX_METEOR_MUSIC_CONTROL_3 == fc_effect.star_index ||
                //      17 == fc_effect.star_index ||
                //      18 == fc_effect.star_index) &&
                //     DEVICE_ON == fc_effect.star_on_off) // 流星灯要确保是开启
                if (DEVICE_ON == fc_effect.star_on_off &&
                    (17 == fc_effect.star_index ||
                     18 == fc_effect.star_index))
                {
                    // 如果流星灯处于声控模式，会进入这里
                    // music_voic.meteor_trg = 1; // 流星声控
                    flag_sound_triggered_in_meteor_lights = 1;

                    WS2812FX_triggered_by_meteor_lights();
                }
            }
        }
    }
    // } // if (adc < 1000) // 当ADC值大于1000，说明硬件电路有问题

#endif // 移植其他项目的声控程序
}
