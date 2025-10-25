#include "system/includes.h"
#include "led_strip_sys.h"
#include "ir_key_app.h"
#include "task.h"
#include "event.h"
#include "my_effect.h"
#include "board_ac632n_demo_cfg.h"
#include "ir_key_app.h"
#include "led_strand_effect.h"
#if 1

// @key_app.c
// 实现以下功能APP
// 物理按键
// AD按键
// 红外遥控


/***********************************************************移植须修改****************************************************************/
// 遥控一共10个自动mode
#define IR_MODE_QTY 10
/* 定义声控模式数量 */
#define IR_MIC_QTY  4
// 输入ir_scan_mode，用于遥控自动模式，模式+，模式-
const u8 ir_mode[IR_MODE_QTY] = {0,1,4,5,6,15,12,7,8,9};
/* 作为light_mode_change()输入形参 */
// 数组元素：MIC_ROLL_CMD ，MIC_ERENGY_CMD，MIC_SPECTRUM_CMD，MIC_ROCK_CMD
const u8 ir_mic_mode[IR_MIC_QTY] = { };
/***********************************************************END****************************************************************/



//  计数器，遥控自动模式，模式+,-计数
u8 ir_mode_cnt;
ir_auto_e ir_auto_f = IR_PAUSE;
// 自动模式下，改变模式计数器
u16 ir_auto_change_tcnt = 0;
/* 定时器状态 */
ir_timer_e ir_timer_state ;
/* 定时计数器,累减,单位S */
u16 ir_time_cnt;
/* 本地声控模式index */
u8 ir_local_mic_index = 0;
/* 灵敏度：1-100 */
u8 ir_sensitive = 0;
/***********************************************************API*******************************************************************/
/* 定时器周期1秒，单位1ms */
#define IR_TIMER_UNIT   1000
#define IR_CHANGE_MODE_T    (10*1000)     //10秒换一个模式

extern void save_user_data_area3(void);

//--------------------------------------------------自动
/* 设置自动开与关 */
void set_ir_auto(ir_auto_e auto_f)
{
    /* 检测合法性，和有变化 */
    if(auto_f <= IR_PAUSE && fc_effect.auto_f != auto_f)
    {

        fc_effect.auto_f = auto_f;
        printf("\r set_ir_auto OK");
  
    }
    else
    {

    }
}

void change_ir_auto()
{
    fc_effect.auto_f = !fc_effect.auto_f;
    save_user_data_area3();

}
/* 获取自动状态 */
ir_auto_e get_ir_auto(void)
{
    return fc_effect.auto_f;
}

//-------------------------------------------------效果

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
void  ir_mode_sub(void)
{
    if(ir_mode_cnt>0)
    {
        ir_mode_cnt--;
    }
    else
    {
        ir_mode_cnt = IR_MODE_QTY;
    }
}
// -----------------------------------------------遥控开启定时功能

/* 设置定时关机，按OFF取消 */
void set_ir_timer(ir_timer_e timer)
{
    /* 检测合法性，和有变化 */
    if(timer <= IR_TIMER_120MIN && ir_timer_state != timer)
    {
        ir_timer_state = timer;
        ir_time_cnt = timer / IR_TIMER_UNIT;
    }
}

ir_timer_e get_ir_timer(void)
{
    return ir_timer_state;
}



/***********************************************************APP*******************************************************************/
extern u8 all_mode[];
static void ir_auto_change_mode(void)
{
    u32 err;
    extern u8 ws2811fx_set_cycle;
    extern void build_in_mode_plus(void);

    // 循环模式
    if(fc_effect.auto_f == IR_AUTO)
    {

        if(ir_auto_change_tcnt!=0)
        {

            ir_auto_change_tcnt-=10;
            // printf("\n ir_auto_change_tcnt=%d",ir_auto_change_tcnt);
        }

        if(ws2811fx_set_cycle == 1)
        {
            ws2811fx_set_cycle = 0;
            ir_auto_change_tcnt = IR_CHANGE_MODE_T;
            if(all_mode[2] <  0x15)
            {
                all_mode[2] += 1;
            }
            else{
                all_mode[2] = 7;
            }
            parse_zd_data(all_mode);
        }
    }
}

static void ir_timer_handle(void)
{
    if(ir_timer_state != IR_TIMER_NO)
    {
        if(ir_time_cnt > 0)
        {
            ir_time_cnt--;
            #ifdef MY_DEBUG
            printf("\r ir_time_cnt--");
            #endif
        }
        else
        {
            /* 关灯 */
            set_ir_timer(IR_TIMER_NO);
            soft_rurn_off_lights();
        }
    }
}


extern void set_static_mode(u8 r, u8 g, u8 b);

u8 ir_lock;

void ir_key_handle(struct sys_event *event)
{
    u8 event_type = 0;
    u8 key_value = 0;
    event_type = event->u.key.event;
    key_value = event->u.key.value;
    printf("ir_key_handle");
    printf("\n event->u.key.type = %d",event->u.key.type);

    if(event->u.key.type == KEY_DRIVER_TYPE_IR)
    {

        if(key_value == IRKEY_LOCK )
        {
            ir_lock =!ir_lock;
        }
        if(ir_lock) return;

        if(event_type == KEY_EVENT_HOLD &&get_on_off_state() == DEVICE_ON)
        {
            if(key_value == IRKEY_SPEED_PLUS)
            {
                extern void ir_color_plus(void);
                ir_color_plus();
            }

            if(key_value == IRKEY_SPEED_SUB)
            {
                extern void ir_color_sub(void);
                ir_color_sub();

            }

        }

        if (event_type == KEY_EVENT_CLICK && (key_value == IRKEY_ON || key_value == IRKEY_OFF))   //长按遥控器电源键
        {
            if(key_value == IRKEY_OFF)
            {
                soft_rurn_off_lights();
            }
            if(key_value == IRKEY_ON)
            {
                soft_turn_on_the_light();
            }
        }

        if(event_type == KEY_EVENT_CLICK && get_on_off_state() == DEVICE_ON)   //开机的状态下才能操作
        {
            if(
                key_value != IRKEY_LIGHT_PLUS &&
                key_value != IRKEY_LIGHT_SUB &&
                key_value != IRKEY_SPEED_PLUS &&
                key_value != IRKEY_SPEED_SUB &&
                key_value != IRKEY_SPEED_PLUS &&
                key_value != IRKEY_ON &&
                key_value != IRKEY_OFF
            )
            {
                set_ir_auto(IR_PAUSE);
            }

            switch(key_value)
            {

                case IRKEY_R:
                    set_static_mode(255, 0, 0);
                    break;
                case IRKEY_G:
                    set_static_mode(0, 255, 0);

                    break;
                case IRKEY_B:
                    set_static_mode(0, 0, 255);

                    break;
                case IRKEY_YELLOW:
                    set_static_mode(255, 255, 0);

                    break;
                case IRKEY_CYAN:
                    printf("\n IRKEY_CYAN");
                    set_static_mode(0, 255, 255);

                    break;
                case IRKEY_PURPLE:
                    printf("\n IRKEY_PURPLE");

                    set_static_mode(255, 0, 255);

                    break;
                case IRKEY_W:
                    set_static_mode(255, 255, 255);

                    break;
                case IRKEY_ORANGE:
                    printf("\n IRKEY_ORANGE");

                    set_static_mode(255, 165, 0);

                    break;

                case IRKEY_B1:
                    printf("\n IRKEY_B1");

                    set_static_mode(30, 130, 255);

                    break;

                case IRKEY_B2:
                    printf("\n IRKEY_B2");

                    set_static_mode(72, 61, 139);

                    break;

                case IRKEY_G1:
                    printf("\n IRKEY_G1");

                    set_static_mode(50, 205, 50);

                    break;

                case IRKEY_LIGHT_PLUS:
                    extern void bright_plus(void);
                    bright_plus();
                    break;
                case IRKEY_LIGHT_SUB:
                    extern void bright_sub(void);
                    bright_sub();
                    break;
                case IRKEY_SPEED_PLUS:
                    extern void speed_up(void);
                    speed_up();
                    break;
                case IRKEY_SPEED_SUB:
                    extern void speed_down(void);
                    speed_down();
                    break;
                case IRKEY_MODE_ADD:
                    extern void build_in_mode_plus(void);
                    build_in_mode_plus();
                    break;
                case IRKEY_MODE_DEC:
                    extern void build_in_mode_sub(void);
                    build_in_mode_sub();
                    break;

                case IRKEY_AUTO:
                    set_ir_auto(IR_AUTO);
                break;

                case  IRKEY_MUSIC1:
                    extern void music_mode_plus(void);
                    music_mode_plus();
                break;

                case  IRKEY_MUSIC2:

                    extern void music_mode_sub(void);
                    music_mode_sub();
                break;
                case IRKEY_COLOR:
                    extern void set_custom_effect(u8 m);
                    set_custom_effect(3);
                break;

            }//switch
            extern void save_user_data_area3(void);
            save_user_data_area3();
        }//if(event_type == KEY_EVENT_CLICK && on_off_flag == DEVICE_ON)
    }//if(event->u.key.type == KEY_DRIVER_TYPE_IR)
}


/***********************************************************key_event_task_handle*******************************************************************/

// 10ms调用一次
void ir_timer_handler(void)
{
    ir_auto_change_mode();
    // ir_timer_handle();
}


#endif
