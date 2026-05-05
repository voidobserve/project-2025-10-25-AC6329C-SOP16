#ifndef __USER_BLE_NOTIFY_H__
#define __USER_BLE_NOTIFY_H__

#include "typedef.h"

// 发送缓冲区中，一条指令最大的长度：
#define USER_BLE_NOTIFY_SEND_BUFF_MAX_LEN 30
// 发送缓冲区中，最大的指令数量：
#define USER_BLE_NOTIFY_SEND_BUFF_MAX_NUM 30
typedef struct
{
    // 发送缓冲区
    u8 send_buff[USER_BLE_NOTIFY_SEND_BUFF_MAX_NUM][USER_BLE_NOTIFY_SEND_BUFF_MAX_LEN];
    u16 send_buff_len[USER_BLE_NOTIFY_SEND_BUFF_MAX_NUM];
    u8 send_buff_head;
    u8 send_buff_tail;
    u8 send_buff_num;

    // 存放指令
    void (*param_put)(u8 *buff, u16 len);
    // 指令处理函数
    void (*param_handle)(void);
} user_ble_notify_t;

extern volatile user_ble_notify_t user_ble_notify_obj;

#endif