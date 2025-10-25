#ifndef ws2812fx_effect_h
#define ws2812fx_effect_h

uint16_t WS2812FX_multiColor_wipe(uint8_t rev) ;


// builtin modes
uint16_t
  WS2812FX_mode_static(void),
  WS2812FX_mode_blink(void),
  WS2812FX_mode_blink_rainbow(void),
  WS2812FX_mode_strobe(void),
  WS2812FX_mode_strobe_rainbow(void),
  WS2812FX_mode_color_wipe(void),
  WS2812FX_mode_color_wipe_inv(void),
  WS2812FX_mode_color_wipe_rev(void),
  WS2812FX_mode_color_wipe_rev_inv(void),
  WS2812FX_mode_color_wipe_random(void),
  WS2812FX_mode_color_sweep_random(void),
  WS2812FX_mode_random_color(void),
  WS2812FX_mode_single_dynamic(void),
  WS2812FX_mode_multi_dynamic(void),
  WS2812FX_mode_breath(void),
  WS2812FX_mode_fade(void),
  WS2812FX_mode_scan(void),
  WS2812FX_mode_dual_scan(void),
  WS2812FX_mode_theater_chase(void),
  WS2812FX_mode_theater_chase_rainbow(void),
  WS2812FX_mode_rainbow(void),
  WS2812FX_mode_rainbow_cycle(void),
  WS2812FX_mode_running_lights(void),
  WS2812FX_mode_twinkle(void),
  WS2812FX_mode_twinkle_random(void),
  WS2812FX_mode_twinkle_fade(void),
  WS2812FX_mode_twinkle_fade_random(void),
  WS2812FX_mode_sparkle(void),
  WS2812FX_mode_flash_sparkle(void),
  WS2812FX_mode_hyper_sparkle(void),
  WS2812FX_mode_multi_strobe(void),
  WS2812FX_mode_chase_white(void),
  WS2812FX_mode_chase_color(void),
  WS2812FX_mode_chase_random(void),
  WS2812FX_mode_chase_rainbow(void),
  WS2812FX_mode_chase_flash(void),
  WS2812FX_mode_chase_flash_random(void),
  WS2812FX_mode_chase_rainbow_white(void),
  WS2812FX_mode_chase_blackout(void),
  WS2812FX_mode_chase_blackout_rainbow(void),
  WS2812FX_mode_running_color(void),
  WS2812FX_mode_running_red_blue(void),
  WS2812FX_mode_running_random(void),
  WS2812FX_mode_larson_scanner(void),
  WS2812FX_mode_comet(void),
  WS2812FX_mode_fireworks(void),
  WS2812FX_mode_fireworks_random(void),
  WS2812FX_mode_merry_christmas(void),
  WS2812FX_mode_halloween(void),
  WS2812FX_mode_fire_flicker(void),
  WS2812FX_mode_fire_flicker_soft(void),
  WS2812FX_mode_fire_flicker_intense(void),
  WS2812FX_mode_circus_combustus(void),
  WS2812FX_mode_bicolor_chase(void),
  WS2812FX_mode_tricolor_chase(void),
  WS2812FX_mode_custom_0(void),
  WS2812FX_mode_custom_1(void),
  WS2812FX_mode_custom_2(void),
  WS2812FX_mode_custom_3(void),
  WS2812FX_mode_custom_4(void),
  WS2812FX_mode_custom_5(void),
  WS2812FX_mode_custom_6(void),
  WS2812FX_mode_custom_7(void),
  WS2812FX_mode_fade_single(void),
  WS2812FX_smear_adjust_effect(void);


uint16_t WS2812FX_mode_fade_each_led(void) ;
void set_seg_forward_out(uint8_t s, uint16_t ms);
uint16_t WS2812FX_mode_single_block_scan(void);
uint16_t WS2812FX_mode_mutil_fade(void) ;
uint16_t WS2812FX_mode_multi_block_scan(void);
uint16_t WS2812FX_mode_mutil_breath(void);
uint16_t WS2812FX_mode_mutil_twihkle(void);
uint16_t WS2812FX_mode_multi_forward_same(void);
uint16_t WS2812FX_mode_multi_back_same(void);
uint16_t WS2812FX_adj_rgb_sequence(void);
uint16_t WS2812FX_mutil_c_jump(void);
uint16_t WS2812FX_mutil_c_gradual(void);
uint16_t breath_w(void);
uint16_t WS2812FX_mutil_strobe(void);
uint16_t breath_rgb(void);



/***************天奕光纤满天星流星效果*****************/

uint16_t WS2812FX_mode_comet_1(void);
uint16_t WS2812FX_mode_comet_2(void);
uint16_t WS2812FX_mode_comet_3(void);
uint16_t meteor_effect_G(void);
uint16_t meteor_effect_H(void);
uint16_t WS2812FX_mode_comet_4(void);
uint16_t WS2812FX_mode_comet_5(void);
uint16_t WS2812FX_mode_comet_6(void);

uint16_t fc_double_meteor(void);
void close_metemor(void);





#endif