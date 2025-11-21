#ifndef led_strip_voice_h
#define led_strip_voice_h

#include "led_strand_effect.h"

u8 get_sound_triggered_by_colorful_lights(void); // 获取七彩灯的声控结果
u8 get_sound_triggered_by_meteor_lights(void);   // 获取流星灯的声控结果
u8 get_sound_triggered_by_motor(void);

void colorful_lights_sound_sensitivity_add(void); // 七彩灯声控模式下的灵敏度 增加
void colorful_lights_sound_sensitivity_sub(void); // 七彩灯声控模式下的灵敏度 减少

void meteor_lights_sound_sensitivity_add(void); // 流星灯声控模式下的灵敏度 增加
void meteor_lights_sound_sensitivity_sub(void); // 流星灯声控模式下的灵敏度 减少

void motor_sound_sensitivity_add(void);
void motor_sound_sensitivity_sub(void);

#endif
