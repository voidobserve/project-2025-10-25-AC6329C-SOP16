#ifndef __ws2812_bsp_h
#define __ws2812_bsp_h


#include "adafruit_typedef.h"

void ws281x_init(void);

void ws281x_show(unsigned char *pixels_pattern, uint16_t pattern_size);

unsigned long HAL_GetTick(void);

// 每10ms调用一次
void run_tick_per_10ms(void);



#endif
