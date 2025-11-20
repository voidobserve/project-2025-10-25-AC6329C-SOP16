
#include "system/includes.h"
#include "syscfg_id.h"
#include "save_flash.h"
#include "../../../apps/user_app/rf433_key/rf433_key.h"

#define FLASH_CRC_DATA 0xC5

static volatile u16 time_count_down = 0; // 存放当前的倒计时
static volatile u8 flag_is_enable_count_down = 0;
static volatile u8 flag_is_enable_to_save = 0; // 标志位，是否使能了保存

volatile save_flash_t save_data;

/*******************************************************************************************************
**函数名：上电读取FLASH里保存的指令数据
**输  出：
**输  入：读取 CFG_USER_COMMAND_BUF_DATA 里保存的最后一条接收到的指令，
**描  述：读取 CFG_USER_LED_LEDGTH_DATA 里保存的第一次上电标志，灯带长度，顺序是：：第1字节：第一次上电标志位，第2、3字节：灯带长度
**说  明：
**版  本：
**修改日期：
*******************************************************************************************************/
void read_flash_device_status_init(void)
{
    int ret = 0;
    // local_irq_disable(); // 禁用中断
    ret = syscfg_read(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_data), sizeof(save_flash_t));
    // local_irq_enable(); // 使能中断
    if (ret != sizeof(save_flash_t))
    {
        // 如果读取到的数据个数不一致
        // printf("read save info error \n");
        memset((u8 *)&save_data, 0, sizeof(save_flash_t));
    }

    if (save_data.header != FLASH_CRC_DATA) // 第一次上电
    {
        save_data.header = FLASH_CRC_DATA;
        fc_data_init();
        os_taskq_post("msg_task", 1, MSG_USER_SAVE_INFO);
        // printf("is first power on\n");

        // save_user_data_enable();
    }
    else
    {
        memcpy((u8 *)(&fc_effect), (u8 *)(&save_data.fc_save), sizeof(fc_effect_t));
#if RF_433_LEARN_ENABLE
        rf_433_addr_update(save_data.rf_433_addr); // 更新rf433遥控器的地址
#endif
        // printf("is not first power on\n");
    }
}

// 写入flash时间倒计时
// void save_data_time_count_down(void *p)
/**
 * @brief 写入flash倒计时
 *      10ms调用一次，不需要特别准确
 *
 *      如果 flag_is_enable_count_down == 1，表示使能倒计时
 *      如果 flag_is_enable_count_down == 0，表示未使能倒计时
 *
 *      计时结束，将 flag_is_enable_to_save 置一
 */
void save_user_data_time_count_down(void)
{
    if (0 == flag_is_enable_count_down)
    {
        return;
    }

    if (time_count_down > 0)
    {
        time_count_down--;
    }

    if (0 == time_count_down)
    {
        flag_is_enable_count_down = 0;
        flag_is_enable_to_save = 1;
    }
}

// 把用户数据写到区域3
void save_user_data_area3(void)
{
    int ret = 0;

    save_data.header = FLASH_CRC_DATA; // 表示数据有效

    memcpy((u8 *)(&save_data.fc_save), (u8 *)(&fc_effect), sizeof(fc_effect_t));
#if RF_433_LEARN_ENABLE
    save_data.rf_433_addr = rf_433_addr_get();
#endif

    os_time_dly(1); // 先让出cpu，处理其他任务，防止看门狗复位
    // local_irq_disable(); // 禁用中断
    ret = syscfg_write(CFG_USER_LED_LEDGTH_DATA, (u8 *)(&save_data), sizeof(save_flash_t));
    // local_irq_enable(); // 使能中断

    flag_is_enable_to_save = 0;

    printf("save info done \n");
}

void save_user_data_enable(void)
{
    flag_is_enable_count_down = 0;
    time_count_down = DELAY_SAVE_FLASH_TIMES / 10; // DELAY_SAVE_FLASH_TIMES / 10 ms计时，实现 DELAY_SAVE_FLASH_TIMES ms延时
    flag_is_enable_count_down = 1;
}

/**
 * @brief 保存用户数据
 *          需要放到主循环执行
 *
 * @return * void
 */
void save_user_data_handle(void)
{
    if (flag_is_enable_to_save)
    {
        flag_is_enable_to_save = 0;
        save_user_data_area3();
    }
}
