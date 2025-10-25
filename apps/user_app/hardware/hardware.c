#include "hardware.h"
#include "gpio.h"
#include "led_strand_effect.h"


/**************************  风扇IO口初始化   ***********/
void fan_gpio_init(void)
{

    gpio_set_pull_down(IO_PORT_DM,0);
    gpio_set_pull_up(IO_PORT_DM,0);
    gpio_direction_output(IO_PORT_DM,1);
}

void open_fan(void)
{
    gpio_direction_output(IO_PORT_DM,1);

}

void close_fan(void)
{
    gpio_direction_output(IO_PORT_DM,0);

}



/*************************  边沿检测 *******************/
/*************************  边沿检测 *******************/
/*************************  边沿检测 *******************/

unsigned long syn_ms = 0;
u8 syn_timer_f = 0;

AT_VOLATILE_RAM_CODE
void get_syn_signal(void)
{
    syn_timer_f = 1;

    if(gpio_read(IO_PORT_DP) && syn_timer_f == 1) //  读取到高电平
    {
        syn_timer_f = 0;
        syn_ms += 20;

    }
}


unsigned long get_syn_time(void)
{
    return syn_ms;
}
void my_io_isr_cbfun_syn(u32 index)
{
    // printf(my_io_isr_cbfun_syn);
    extern void get_syn_signal(void);
    get_syn_signal();
}
void io_ext_interrupt_syn(void)
{

    printf("io_ext_interrupt_syn");
    set_io_ext_interrupt_cbfun(my_io_isr_cbfun_syn);
    io_ext_interrupt_init(pwm_ch3, IO_PORT_DP, 0);


}















/**********************************  中道闹钟  *******************************************************/
u8 set_week[ALARM_NUMBER][7];
ALARM_CLOCK alarm_clock[3];
TIME_CLOCK time_clock;
u8 calculate_ms = 0;

void set_zd_countdown_state(u8 s,u8 index)
{

    // zd_countdown[index].set_on_off = s;   //计时开关或者闹钟开关

}

/**
 * @brief 闹钟数据出解析
 *
 * @param index
 */
void parse_alarm_data(int index)
{

    /*解析循环星期*/
    u8 p_mode;
    p_mode = alarm_clock[index].mode;
    for(int i = 0; i < 7; i++)
    {
        if(p_mode & 0x01)
            set_week[index][i] =  i + 1;

        else
            set_week[index][i] = 0;
        p_mode = p_mode >> 1;

    }


    if(alarm_clock[index].on_off == 0x80)  //闹钟开
    {
        printf("open alarm");
        set_zd_countdown_state(DEVICE_ON,index);    //开启闹钟
    }

    if(alarm_clock[index].on_off == 0x00)  //闹钟关
    {
        printf("close alarm");
        set_zd_countdown_state(DEVICE_OFF,index);  //关闭闹钟
    }



}

/**
 * @brief 关闭中道闹钟
 *
 */
void close_alarm(int index)
{
    uint8_t Send_buffer[6];        //发送缓存

    // zd_countdown[index].set_on_off = 0;
    alarm_clock[index].on_off = 0;
    Send_buffer[0] = 0x05;
    Send_buffer[1] = index;
    Send_buffer[2] = alarm_clock[index].hour;
    Send_buffer[3] = alarm_clock[index].minute;
    Send_buffer[4] = 0;
    Send_buffer[5] = alarm_clock[index].mode;
    extern void zd_fb_2_app(u8 *p, u8 len);
    zd_fb_2_app(Send_buffer, 6);
}




/**
 * @brief 闹钟处理
 *
 * @param index 闹钟编号
 */

void countdown_handler(int index)
{
#if 0
    if( zd_countdown[index].set_on_off)  //闹钟开启
    {

        if(time_clock.hour == alarm_clock[index].hour &&  time_clock.minute == alarm_clock[index].minute)
        {

            if((alarm_clock[index].mode & 0x7f) == 0)  //没有星期
            {
                if((alarm_clock[index].mode >> 7))  //闹钟设置里灯的状态
                {
                    soft_turn_on_the_light();
                    close_alarm(index);
                    save_user_data_area3();
                }
                else
                {
                    soft_turn_off_lights();
                    close_alarm(index);
                    save_user_data_area3();
                }


            }
            else
            {

                for(int i = 0; i < 7; i++)
                {

                    if(time_clock.week == set_week[index][i])
                    {


                        if((alarm_clock[index].mode >> 7))
                        {

                            soft_turn_on_the_light();
                            close_alarm(index);
                            save_user_data_area3();
                        }
                        else
                        {
                            soft_turn_off_lights();
                            close_alarm(index);
                            save_user_data_area3();
                        }

                    }

                }


            }


        }

    }
#endif
}




/**
 * @brief 放在主函数的while中，10ms调用一次
 *
 */
void time_clock_handler(void)
{

    calculate_ms++;
    if(calculate_ms == 100)   //秒
    {
        calculate_ms = 0;
        time_clock.second++;

        if(time_clock.second == 60)  //分
        {
            time_clock.second = 0;
            time_clock.minute++;

            if(time_clock.minute == 60) //时
            {
                time_clock.minute = 0;
                time_clock.hour++;

                if(time_clock.hour == 24) //日
                {
                    time_clock.hour = 0;
                    time_clock.week++;
                    if(time_clock.week == 8)  //周
                    {
                        time_clock.week = 1;
                    }

                }
            }

        }
        // printf("time_clock.hour  = %d",time_clock.hour );
        // printf("time_clock.minute  = %d",time_clock.minute );
        // printf("time_clock.second  = %d",time_clock.second );
        // printf("time_clock.week  = %d",time_clock.week );

    }

    countdown_handler(0);  //闹钟0
    countdown_handler(1);
    countdown_handler(2);

}




typedef enum
{
    COUNTDOWN_TIMER_NO = 0,        //无定时
    COUNTDOWN_TIMER_1H = 1*60*60*1000,
    COUNTDOWN_TIMER_2H = 2*60*60*1000,
    COUNTDOWN_TIMER_4H = 4*60*60*1000,
    COUNTDOWN_TIMER_8H = 8*60*60*1000,



}COUNTDWN_TIME_T;

/* 
0：无定时
1：2h
2：4h
3：8h
 */
u8 cd_state = 0; 
u32 setting_time;
#define MAX_COUNTDOWN_NUM 4
void ls_change_countdowm(void)
{
    cd_state++;
    cd_state %= MAX_COUNTDOWN_NUM;
    switch(cd_state)
    {
        case 0: setting_time=0;            break;
        case 1: setting_time=COUNTDOWN_TIMER_2H;  break;
        case 2: setting_time=COUNTDOWN_TIMER_4H;  break;
        case 3: setting_time=COUNTDOWN_TIMER_8H;  break;
    }
}

void ls_set_countdown(u8 t)
{
    cd_state = t;
    switch(cd_state)
    {
        case 0: setting_time=0;            break;
        case 1: setting_time=COUNTDOWN_TIMER_2H;  break;
        case 2: setting_time=COUNTDOWN_TIMER_4H;  break;
        case 3: setting_time=COUNTDOWN_TIMER_8H;  break;
    }
    
}



// 每10ms跑一次
void count_down_run(void)
{
    if(cd_state != 0)
    {
        if(setting_time >= 10)
        {
            setting_time -=  10;
        }
        else
        {
            cd_state = 0;
            soft_turn_off_lights();  //关灯.
            
        }
    }
}