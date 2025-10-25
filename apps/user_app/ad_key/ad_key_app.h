#ifndef ad_key_app_h
#define ad_key_app_h

#include "board_ac632n_demo_cfg.h"

#define ADKEY_S         0x01
#define ADKEY_POWER     0x00
#define ADKEY_MODEL     0x02


void ad_key_handle(struct sys_event *event);


#endif



