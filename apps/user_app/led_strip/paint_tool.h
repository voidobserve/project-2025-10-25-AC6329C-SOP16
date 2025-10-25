// 制作效果用到的工具
#ifndef paint_tool_h
#define paint_tool_h

#include "led_strip_sys.h"
#if 0
/******************************************************************common******************************************************************/
#define MAX_DUAN_OF_PAINT   20      //涂抹模式最多支持20段,根据涂鸦平台定义
#define Disp_buf_Len        1100    //显存大小，决定了可用灯带的长度，3*350=1050，最多1050个灯珠，350组灯




typedef enum{
    NO_EFFECT,      //无效果
    SMOOTH          //平滑过度
}adjust_effect_e; //调节效果
// -----------------------------------------------------------------涂抹（smear_mode）工具

typedef enum {
    PAINT_BUCKET,   //油漆桶
    PENCIL,         //铅笔
    ERASER,         //橡皮擦
    COMBINATION     //组合，多种颜色同时绘画
}adj_act_e;     //调节动作

typedef struct
{
    /*输入参数*/
    u8 duan_qty;                            //灯带总段数
    u8 duan_addr[MAX_DUAN_OF_PAINT];        //操作的paint的地址,第一段=1，第二段=2,每次用完清0
    u8 smooth;                 //是否开启平滑
    u8 adj_act;                      //调节动作
    u8 led_ch;                              //灯珠通道数，有多少种颜色
    color_t  pen_clr;                       //画笔颜色,适用油桶，铅笔，橡皮擦。不适用组合
}smear_tool_t;

typedef struct
{
    color_t s_clr;      //起点颜色
    color_t e_clr;      //终点颜色
    u16 s_offset;       //起始LED偏移地址
    u16 e_offset;       //终止LED偏移地址,在输出buf地址要乘以LED通道数
    u8* pOutput;        //存储地址
}smooth_t;  //平滑效果

typedef struct _PAINT_COLOUR
{
    u8 adjust_mode;     //调节模式
    u8 adjust_effect;   //调节效果
    u8 led_paragraph;   //灯带段数
    u8 adjust_action;   //调节动作
    u32 light_val;      //亮度    范围：10~1000
    u32 color_temper;   //色温    范围：0~1000
    u32 h_val;          //h的数值
    u32 s_val;          //s的数值
    u32 v_val;          //v的数值
    u32 r_val;
    u32 g_val;
    u32 b_val;
}PAINT_COLOUR;

// 实现灯带不同颜色段，不带平滑效果
// 灯带每段输出不同颜色，同一段里的每颗灯珠颜色一样
void led_strip_dif_clr_duan(smear_tool_t* smear_pcb, u8* pOutput);
void smear_dp_parse(const u8 *pData ,smear_tool_t *smear_pcb);
// 把色温转换为RGB数据
// 输入
// b: 亮度
// k:色温
color_t color_temp_tran_rgb(u16 b, u16 k);
void m_hsv_to_rgb(u8 *R, u8 *G, u8 *B, int h, int s, int v);
void hsv_to_rgb(int *g, int *r, int *b, int h, int s, int v);

#endif
#endif



