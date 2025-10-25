
#ifndef led_strip_drive_h
#define led_strip_drive_h

#include "board_ac632n_demo_cfg.h"
#include "asm/ledc.h"
#include "asm/gpio.h"







void led_state_init(void);
void led_gpio_init(void);
void led_pwm_init(void);



#endif







