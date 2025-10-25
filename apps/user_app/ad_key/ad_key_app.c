#include "board_ac632n_demo_cfg.h"
#include "event.h"
#include "key_driver.h"
#include "led_strip_sys.h"
#include "ir_key_app.h"
#include "led_strand_effect.h"
#include "tuya_ble_type.h"
#include "system/includes.h"

#if TCFG_ADKEY_ENABLE
// ---------------------------------------------定义AD_KEY键值
// 要和board_ac632n_demo.c的定义：adkey_platform_data adkey_data[] 匹配
typedef enum
{
    AD_KEY_PWR = 0
}ad_key_value_e;


void ad_key_handle(struct sys_event *event)
{
    u8 event_type = 0;
    u8 key_value = 0;

    event_type = event->u.key.event;
    key_value = event->u.key.value;
    printf("ad_key_handle");
    if(event->u.key.type == KEY_DRIVER_TYPE_AD)
    {
        printf("\n key_value=%d",key_value);
        if(key_value == AD_KEY_PWR)
        {
            if(get_on_off_state() == DEVICE_ON)
            {
            soft_rurn_off_lights();

            }
            else
            {
                soft_turn_on_the_light();

            }
        }

    }
    


}//end

#endif


