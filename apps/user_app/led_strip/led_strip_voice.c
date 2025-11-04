#include "led_strip_voice.h"
#include "asm/adc_api.h"
#include "led_strip_drive.h"

#define MAX_SOUND 10
struct MUSIC_VOICE_T
{
    u8 sound_trg;
    u8 meteor_trg;
    u32 adc_sum;
    u32 adc_sum_n;
    int sound_buf[MAX_SOUND];
    u8 sound_cnt;
    int c_v;
    int v;
    u8 valid;
};

struct MUSIC_VOICE_T music_voic = {

    .sound_trg = 0,
    .meteor_trg = 0,
    .adc_sum = 0,
    .adc_sum_n = 0,
    .sound_cnt = 0,
    .valid = 0,
    .v = 0,
    .c_v = 0,
};

// 获取声控结果
// 触发条件：（（当前声音大小 - 平均值）* 100 ）/ 平均值 > 灵敏度（0~100）
// 0:没触发
// 1:触发
u8 get_sound_result(void)
{
    u8 p_trg;
    p_trg = music_voic.sound_trg;
    music_voic.sound_trg = 0;
    return p_trg;
}

// 流星灯声控模式下，需要调用的接口，检测是否触发流星灯声控
u8 get_meteor_result(void)
{
    u8 p_metemor_trg;
    p_metemor_trg = music_voic.meteor_trg;
    music_voic.meteor_trg = 0;
    return p_metemor_trg;
}

void sound_handle(void)
{
#if 0 // 旧版的声控检测程序
    u16 adc;
    u8 i;
    // 记录adc值

    if (fc_effect.on_off_flag == DEVICE_ON &&
        (fc_effect.Now_state == IS_light_music ||
         fc_effect.base_ins.mode == 0x05 ||
         fc_effect.star_index == 17 ||
         fc_effect.star_index == 18))
    {

#if 1
        music_voic.sound_buf[music_voic.sound_cnt] = check_mic_adc();
        music_voic.c_v = music_voic.sound_buf[music_voic.sound_cnt]; // 记录当前值
        music_voic.sound_cnt++;

        if (music_voic.sound_cnt > (MAX_SOUND - 1))
        {
            music_voic.sound_cnt = 0;
            music_voic.valid = 1;
            music_voic.v = 0;
            for (i = 0; i < MAX_SOUND; i++)
            {
                music_voic.v += music_voic.sound_buf[i];
            }
            music_voic.v = music_voic.v / MAX_SOUND; // 计算平均值
        }

        if (music_voic.valid)
        {

            if (music_voic.c_v > music_voic.v)
            {
                if ((music_voic.c_v - music_voic.v) * 100 / music_voic.v > fc_effect.music.s) // 很灵敏
                {
                    if (fc_effect.Now_state == IS_light_music || fc_effect.base_ins.mode == 0x05)
                    {
                        // 如果七彩灯处于声控模式，会进入这里（补充，电机处于声控模式下，也会用到这个标志位，目前没有另外使用新的标志位）
                        music_voic.sound_trg = 1; // 七彩声控

                        if (fc_effect.Now_state == IS_light_music) // 防止电机的声控模式触发了七彩灯的声控标志变化
                        {
                            WS2812FX_trigger_by_colorful_lights();
                        }
                        
                        // printf("trigger_by_colorful_lights\n");
                    }

                    if (fc_effect.star_index == 17 || fc_effect.star_index == 18)
                    {
                        // 如果流星灯处于声控模式，会进入这里
                        music_voic.meteor_trg = 1; // 流星声控
                        WS2812FX_trigger_by_meteor_lights();
                        // printf("trigger_by_meteor_lights\n");
                    }

                    // WS2812FX_trigger_2();
                }
            }
        }

#endif

#if 0
        adc = check_mic_adc(); 
        if(adc < 1000)
        {
            if(music_voic.adc_sum_n < 2000)
            {
                music_voic.adc_sum_n++;
            }
            if(music_voic.adc_sum_n == 2000)
            {
                if(adc / (music_voic.adc_sum/music_voic.adc_sum_n) > 3) return ; //adc突变，大于平均值的3倍，丢弃改值
                music_voic.adc_sum = music_voic.adc_sum - music_voic.adc_sum/music_voic.adc_sum_n;
            }
            music_voic.adc_sum+=adc;
            if(music_voic.adc_sum_n!=0)
            {
                /* 结合灵敏度触发 */
                if(adc * fc_effect.music.s / 100> music_voic.adc_sum/music_voic.adc_sum_n)
                {   
                    music_voic.sound_trg = 1;  //七彩声控
                    music_voic.meteor_trg = 1; //流星声控
                  
                }
                /* 大于平均值 */
                if(adc > music_voic.adc_sum/music_voic.adc_sum_n)
                {

                }
            }
        }
#endif
    }
    else
    {
        music_voic.valid = 0;
    }

#endif

#if 1 // 移植其他项目的声控程序

// extern u32 adc_get_value(u32 ch);
// extern void WS2812FX_trigger();
// extern u32 adc_sample(u32 ch);
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
    if (fc_effect.on_off_flag != DEVICE_ON ||                        // 设备未开启
        (fc_effect.Now_state != IS_light_music &&                    // 七彩灯不处于声控模式
         fc_effect.star_index != 17 && fc_effect.star_index != 18 && // 流星灯不处于声控模式
         fc_effect.base_ins.mode != 0x05)                            // 电机不处于声控模式
    )
    {
        return;
    }

    // 记录adc值
    adc = check_mic_adc(); // 每次进入，采集一次ad值（即使不在声控模式，也会占用一些时间）

    // printf("adc = %d", adc);

    if (adc >= 1000)
    {
        return;
    }

    // if (adc < 1000) // 当ADC值大于1000，说明硬件电路有问题
    // {
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

            // trg = 200;
                if (fc_effect.Now_state == IS_light_music || fc_effect.base_ins.mode == 0x05)
                {
                    // 如果七彩灯处于声控模式，会进入这里（补充，电机处于声控模式下，也会用到这个标志位，目前没有另外使用新的标志位）
                    music_voic.sound_trg = 1; // 七彩声控

                    if (fc_effect.Now_state == IS_light_music) // 防止电机的声控模式触发了七彩灯的声控标志变化
                    {
                        WS2812FX_trigger_by_colorful_lights();
                    }

                    // printf("trigger_by_colorful_lights\n");
                }

                if (fc_effect.star_index == 17 || fc_effect.star_index == 18)
                {
                    // 如果流星灯处于声控模式，会进入这里
                    music_voic.meteor_trg = 1; // 流星声控
                    WS2812FX_trigger_by_meteor_lights();
                    // printf("trigger_by_meteor_lights\n");
                } 
        }
    }
    // } // if (adc < 1000) // 当ADC值大于1000，说明硬件电路有问题

#endif // 移植其他项目的声控程序
}
