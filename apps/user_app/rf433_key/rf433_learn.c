#include "rf433_learn.h"
#include "rf433_key.h"

#include "../../../apps/user_app/led_strip/led_strip_sys.h"

#if RF_433_LEARN_ENABLE

/*
    存放433遥控器的地址，默认全1
    这里故意使用32位的变量存放地址，防止普通的遥控器发送24位数据而让处理函数做出了响应
*/
static volatile u32 rf_433_addr = 0xFFFFFFFF;
/*
    存放接收到的433信号的遥控器地址
    在 rf_433_key_get_value() 中更新
*/
volatile u32 recv_rf_433_addr = 0;

static volatile u8 rf_433_learn_status = RF_433_LEARN_STATUS_NONE;

// 更新rf433遥控器的地址
void rf_433_addr_update(u32 addr)
{
    rf_433_addr = addr;
}

/**
 * @brief 获取rf433遥控器的地址
 *
 * @return u32
 */
u32 rf_433_addr_get(void)
{
    return rf_433_addr;
}

RF_433_LEARN_STATUS_T rf_433_learn_status_get(void)
{
    return rf_433_learn_status;
}

void rf_433_learn_status_update(RF_433_LEARN_STATUS_T status)
{
    rf_433_learn_status = status;
}

/**
 * @brief rf433学习/对码
 *
 *      放到主循环，要在rf433键值、事件处理函数之前调用
 *      需要填写调用的时间 ： RF_433_KEY_LEARN_FUNC_PERIOD = ? ，不需要特别精准
 */
void rf_433_key_learn(void)
{

    static u8 flag_is_learn_exit = 0; // 标志位，学习/对码状态是否退出
    static u16 learn_times_cnt = 0;

    if (flag_is_learn_exit)
    {
        // 如果已经退出了学习/对码的状态，则直接返回
        return;
    }

    learn_times_cnt++;
    if (learn_times_cnt >= RF_433_KEY_LEARN_TIMES_ARTER_PWR_ON / RF_433_KEY_LEARN_FUNC_PERIOD)
    {
        // 超过了学习/对码的时间，自动退出
        learn_times_cnt = 0;
        flag_is_learn_exit = 1;

        // 给处理用户消息的线程发送信息
        // os_taskq_post("msg_task", 1, MSG_RF_433_LEARN_FAIL);

        printf("rf 433 learn timeout\n");
    }

    // 学习/对码期间，检测到有按键长按未松手，进行学习/对码，完成之后退出
    if (KEY_EVENT_HOLD == rf_433_key_structure.rf_433_key_driver_event)
    {
        // 客户给到的遥控器，后面8位是键值，前面16位是地址
        // rf_433_addr_update(recv_rf_433_data >> 8);
        rf_433_addr_update(recv_rf_433_addr);

        // 清空得到的键值和按键事件，不让处理函数响应
        rf_433_key_structure.rf_433_key_latest_key_val = NO_KEY;
        rf_433_key_structure.rf_433_key_driver_event = RF_433_KEY_EVENT_NONE;

        learn_times_cnt = 0;
        flag_is_learn_exit = 1;

        // 给处理用户消息的线程发送信息
        os_taskq_post("msg_task", 1, MSG_RF_433_LEARN_SUCCEED);

        printf("rf 433 learn succeed\n");
    }
}

#endif // #if RF_433_LEARN_ENABLE
