// 实现绘画的各种工具
// 涂抹工具：支持油桶，铅笔，橡皮擦和效果平滑
#include "paint_tool.h"
#include "led_strip_sys.h"
#include "debug.h" 

#if 0

/******************************************************************函数声明******************************************************************/

// 绘画一段灯带，整段颜色一样
// 输入：
// color_t 颜色
// ch:灯珠通道数
// led_qty:绘画多少个灯珠
// 输出：
// pOutput:RGB数据
// 返回值：
// 输出的数量，pOutput填充多少个数据
u16 paint_duan_one_color(color_t rgb, u8 ch, u16 led_qty, u8*pOutput);

static color_t rgb_sequence_adj( color_t *rgb);

// 输入
// in：多段颜色，smear_each_duan_color
// duan_qty：需要查找多少段
// 输出
// addr：哪一段有差异，如“1”表示第1段和第2段颜色有差异
void find_dif_color_duan(color_t *in, u8* addr,u8 duan_qty);

void smooth_handler(smooth_t *smooth_pcb);
/******************************************************************涂抹工具******************************************************************/

/*输出每段的颜色*/
color_t smear_each_duan_color[MAX_DUAN_OF_PAINT];   //根据参数输出每段颜色,掉电保存

u8 smear_each_duan_pixel[MAX_DUAN_OF_PAINT];    //每段像素，偏移地址0=第1段

u16 paint_duan_one_color(color_t rgb, u8 ch, u16 led_qty, u8*pOutput);

// 实现灯带不同颜色段，不带平滑效果
// 灯带每段输出不同颜色，同一段里的每颗灯珠颜色一样
void led_strip_dif_clr_duan(smear_tool_t* smear_pcb, u8* pOutput)
{
    u8 duan_cnt = 0;
    u16 offset = 0, output_len,i,j; //计算存储数据偏移量

    switch (smear_pcb->adj_act)
    {
        case PAINT_BUCKET:

        for(duan_cnt=0; duan_cnt < MAX_DUAN_OF_PAINT; duan_cnt++)
        {
            // 油桶模式，全部段颜色一致
            // smear_each_duan_color[duan_cnt] = smear_pcb->pen_clr;
            smear_pcb->duan_addr[duan_cnt] = duan_cnt+1;
        }

        for(duan_cnt = 0; duan_cnt < smear_pcb->duan_qty; duan_cnt++)
        {
            if(smear_pcb->duan_addr[duan_cnt] != 0)
            {
                smear_each_duan_color[ smear_pcb->duan_addr[duan_cnt] -  1] = smear_pcb->pen_clr;
            }
        }


        break;
        case PENCIL:
        case ERASER:
        for(duan_cnt = 0; duan_cnt < smear_pcb->duan_qty; duan_cnt++)
        {
            if(smear_pcb->duan_addr[duan_cnt] != 0)
            {
                smear_each_duan_color[ smear_pcb->duan_addr[duan_cnt] -  1] = smear_pcb->pen_clr;
            }
        }
        break;
        case COMBINATION:
        // 全部段都操作
        for(duan_cnt=0; duan_cnt < smear_pcb->duan_qty; duan_cnt++)
        {
            smear_pcb->duan_addr[duan_cnt] = duan_cnt+1;
        }


            break;
        default:
        break;
    }


    // 每段纯色
    for(duan_cnt = 0; duan_cnt < smear_pcb->duan_qty; duan_cnt++)
    {
        output_len = paint_duan_one_color(  rgb_sequence_adj(&smear_each_duan_color[duan_cnt]), //颜色
                                            smear_pcb->led_ch,               //灯珠通道数
                                            smear_each_duan_pixel[duan_cnt], //一段多少像素
                                            pOutput + offset);               //数据数据存储地址
        offset += output_len;
    }

    u16 led_addr;
    // 开启效果平滑
    // 通过find_dif_color_duan查找到颜色差异的当前段，存放在addr[]
    // 如第0段和第1段颜色不同
    // 第3段和第4段颜色不同
    // 这时addr[]={1,4}={第0段，第3段}},0代表没有颜色差异，所以+1
    // 平滑效果从第0段的中间位置到第1段中间位置
    if(smear_pcb->smooth == 1)
    {
        // 哪一段有差异，如“1”表示第1段和第2段颜色有差异
        u8 addr[MAX_DUAN_OF_PAINT];
        smooth_t smooth_pcb;
        memset(addr,0,MAX_DUAN_OF_PAINT);
        // 查找不同颜色的段序号
        find_dif_color_duan(smear_each_duan_color,addr,smear_pcb->duan_qty);

        for(i=0; i<smear_pcb->duan_qty; i++)
        {
            // 当前段地址addr[i]-1
            led_addr =0;
            if(addr[i] != 0 && addr[i] < smear_pcb->duan_qty)
            {
                smooth_pcb.s_clr = smear_each_duan_color[addr[i] - 1];
                smooth_pcb.e_clr = smear_each_duan_color[addr[i]];
                for(j=0; j<addr[i]-1; j++)      //addr[]:从1开始，1代表第一段
                {
                    led_addr += smear_each_duan_pixel[j]; //计算当前段前面有多少像素
                }
                smooth_pcb.s_offset = led_addr + smear_each_duan_pixel[addr[i] - 1] /2; //加上当前段一半

                smooth_pcb.e_offset = led_addr + smear_each_duan_pixel[addr[i] - 1] + smear_each_duan_pixel[addr[i]] /2; //led_addr + 后一段的一半
                smooth_pcb.pOutput = pOutput;
                smooth_handler(&smooth_pcb);
            }

        }
        #ifdef MY_DEBUG
        printf("\n addr = ");
        printf_buf(addr, MAX_DUAN_OF_PAINT);

        printf("\n s_clr =");
        printf_buf((u8*)(&smooth_pcb.s_clr), 3);

        printf("\n e_clr =");
        printf_buf((u8*)(&smooth_pcb.e_clr), 3);

        printf("\n s_offset = %d",  smooth_pcb.s_offset);

        printf("\n e_offset = %d",  smooth_pcb.e_offset);

        #endif

    }


    #ifdef MY_DEBUG
    printf("\n duan_qty = %d",  smear_pcb->duan_qty);

    printf("\n duan_addr = ");
    printf_buf(smear_pcb->duan_addr, MAX_DUAN_OF_PAINT);

    printf("\n smooth = %d",  smear_pcb->smooth);

    printf("\n adj_act = %d",  smear_pcb->adj_act);

    printf("\n led_ch = %d",  smear_pcb->led_ch);
    printf("\n r = %d, g = %d, b = %d",  smear_pcb->pen_clr.r, smear_pcb->pen_clr.g, smear_pcb->pen_clr.b);

    printf("\n smear_each_duan_color =");
    printf_buf(smear_each_duan_color,20*3);

    printf("\n smear_each_duan_pixel =");
    printf_buf(smear_each_duan_pixel,MAX_DUAN_OF_PAINT);

    printf("\n out_rgb =");
    printf_buf(pOutput, g_led_strip.lenght*g_led_strip.channel);
    #endif
}



/******************************************************************common******************************************************************/

// 绘画一段灯带，整段颜色一样
// 输入：
// color_t 颜色
// ch:灯珠通道数
// led_qty:绘画多少个灯珠
// 输出：
// pOutput:RGB数据
// 返回值：
// 输出的数量，pOutput填充多少个数据
u16 paint_duan_one_color(color_t rgb, u8 ch, u16 led_qty, u8*pOutput)
{
    u16 led_num, ch_cnt,output_cnt = 0;
    for(led_num=0; led_num < led_qty; led_num++) //灯珠计数
    {
        for(ch_cnt=0; ch_cnt < ch; ch_cnt++) //通道计数
        {
            *(pOutput + output_cnt) = *((u8*)&rgb + ch_cnt);

            output_cnt++;
        }
    }
    return output_cnt;
}

// 把色温转换为RGB数据
// 输入
// b: 亮度
// k:色温
color_t color_temp_tran_rgb(u16 b, u16 k)
{
    color_t out_rgb;
    out_rgb.r = 255*b/1000;
    out_rgb.g = 255*b/1000;
    out_rgb.b = k*b/1000*255/1000;
    return out_rgb;
}

/*******************************************************************************************************
**函数名：hsv转rgb
**输  出：R(0~255),G(0~255),B(0~255)
**输  入：输入范围h(0~360),s(0~1000),v(0~1000)
**描  述：
**说  明：
**版  本：
**修改日期：
*******************************************************************************************************/
//void hsv_to_rgb(int *r, int *g, int *b, int h, int s, int v)
void hsv_to_rgb(int *g, int *r, int *b, int h, int s, int v)
{
	// R,G,B from 0-255, H from 0-360, S,V from 0-1000
	int i;
	float RGB_min, RGB_max;

	if(h>360)
        h=360;
    if(s>1000)
        s=1000;
    if(v>1000)
        v=1000;

	s = s / 10;
	v = v / 10;

	RGB_max = v*2.55f;
	RGB_min = RGB_max*(100 - s) / 100.0f;

	i = h / 60;
	int difs = h % 60; // factorial part of h

					   // RGB adjustment amount by hue
	float RGB_Adj = (RGB_max - RGB_min)*difs / 60.0f;

	switch (i) {
	case 0:
		*r = RGB_max;
		*g = RGB_min + RGB_Adj;
		*b = RGB_min;
		break;
	case 1:
		*r = RGB_max - RGB_Adj;
		*g = RGB_max;
		*b = RGB_min;
		break;
	case 2:
		*r = RGB_min;
		*g = RGB_max;
		*b = RGB_min + RGB_Adj;
		break;
	case 3:
		*r = RGB_min;
		*g = RGB_max - RGB_Adj;
		*b = RGB_max;
		break;
	case 4:
		*r = RGB_min + RGB_Adj;
		*g = RGB_min;
		*b = RGB_max;
		break;
	default:		// case 5:
		*r = RGB_max;
		*g = RGB_min;
		*b = RGB_max - RGB_Adj;
		break;
	}
}





/*******************************************************************************************************
**函数名：hsv转rgb
**输  出：R(0~255),G(0~255),B(0~255)
**输  入：输入范围h(0~360),s(0~1000),v(0~1000)
**描  述：
**说  明：
**版  本：
**修改日期：
*******************************************************************************************************/
//void hsv_to_rgb(int *r, int *g, int *b, int h, int s, int v)
void m_hsv_to_rgb(u8 *R, u8 *G, u8 *B, int h, int s, int v)
{
	// R,G,B from 0-255, H from 0-360, S,V from 0-1000
	int i;
	float RGB_min, RGB_max;
    int g,  r,  b;

	if(h>360)
        h=360;
    if(s>1000)
        s=1000;
    if(v>1000)
        v=1000;

	s = s / 10;
	v = v / 10;

	RGB_max = v*2.55f;
	RGB_min = RGB_max*(100 - s) / 100.0f;

	i = h / 60;
	int difs = h % 60; // factorial part of h

					   // RGB adjustment amount by hue
	float RGB_Adj = (RGB_max - RGB_min)*difs / 60.0f;

	switch (i) {
	case 0:
		r = RGB_max;
		g = RGB_min + RGB_Adj;
		b = RGB_min;
		break;
	case 1:
		r = RGB_max - RGB_Adj;
		g = RGB_max;
		b = RGB_min;
		break;
	case 2:
		r = RGB_min;
		g = RGB_max;
		b = RGB_min + RGB_Adj;
		break;
	case 3:
		r = RGB_min;
		g = RGB_max - RGB_Adj;
		b = RGB_max;
		break;
	case 4:
		r = RGB_min + RGB_Adj;
		g = RGB_min;
		b = RGB_max;
		break;
	default:		// case 5:
		r = RGB_max;
		g = RGB_min;
		b = RGB_max - RGB_Adj;
		break;
	}
    *G = g;
    *B = b;
    *R = r;

}

// 对RGB序列进行调整
static color_t rgb_sequence_adj( color_t *rgb)
{
    color_t rgb_tmp;
    rgb_tmp.r = *((u8*) rgb + rgb_sequence_buf[RGB_SEQUENCE].r);
    rgb_tmp.g = *((u8*) rgb + rgb_sequence_buf[RGB_SEQUENCE].g);
    rgb_tmp.b = *((u8*) rgb + rgb_sequence_buf[RGB_SEQUENCE].b);
    return rgb_tmp;
}

// 输入
// in：多段颜色，smear_each_duan_color
// duan_qty：需要查找多少段
// 输出
// addr：哪一段有差异，如“1”表示第1段和第2段颜色有差异,"0"说明没有差异
void find_dif_color_duan(color_t *in, u8*addr,u8 duan_qty)
{
    u8 i;
    if(duan_qty == 0)
    {
        return;
    }

    for(i=0; i<duan_qty - 1; i++)
    {
        if( (in + i)->r != (in + i + 1)->r ||
            (in + i)->g != (in + i + 1)->g ||
            (in + i)->b != (in + i + 1)->b )
        {
            *addr = i+1;
            addr++;

        }
    }
}

// 起始地址和结束地址保留原色
void smooth_handler(smooth_t *smooth_pcb)
{
    int r_step, g_step, b_step;
    u16 led_qty, i;
    // led_qty:起始LED和终止LED之间有多少个LED
    if(smooth_pcb->s_offset == smooth_pcb->e_offset) return;

    led_qty = (smooth_pcb->s_offset > smooth_pcb->e_offset) ? \
                    smooth_pcb->s_offset - smooth_pcb->e_offset : \
                    smooth_pcb->e_offset - smooth_pcb->s_offset;

    r_step = (smooth_pcb->s_clr.r - smooth_pcb->e_clr.r) / led_qty;
    g_step = (smooth_pcb->s_clr.g - smooth_pcb->e_clr.g) /  led_qty;
    b_step = (smooth_pcb->s_clr.b - smooth_pcb->e_clr.b) / led_qty;

    led_qty--;
    if(led_qty < 2) return; //点数太小不支持平滑

    #ifdef MY_DEBUG
    printf("\n led_qty = %d",led_qty);
    printf("\n r_step = %d",r_step);
    printf("\n g_step = %d",g_step);
    printf("\n b_step = %d",b_step);
    #endif


    for(i=0; i<led_qty+1; i++)
    {
        // +LED_STRIP_CH:下一个灯开始变色，起始地址保留原色
        if(r_step > 0 )
        {
            *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + rgb_sequence_buf[RGB_SEQUENCE].r) = smooth_pcb->s_clr.r - r_step * (i); //
            #ifdef MY_DEBUG
            printf("\n  r =%d", smooth_pcb->s_clr.r - r_step * (i));
            #endif

        }
        if(r_step < 0 )
        {
            *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + rgb_sequence_buf[RGB_SEQUENCE].r) = smooth_pcb->e_clr.r + r_step * (led_qty - i);
            #ifdef MY_DEBUG
            printf("\n  r =%d", smooth_pcb->e_clr.r + r_step * (led_qty - i));
            #endif

        }


        // if( ( (smooth_pcb->s_clr.r + r_step * (i+1)) >= 0) && ((smooth_pcb->s_clr.r + r_step * (i+1)) <= 255) )
        // {
        //     *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + LED_STRIP_CH ) = smooth_pcb->s_clr.r + r_step * (i+1);

        //     #ifdef MY_DEBUG
        //     printf("\n  r =%d", smooth_pcb->s_clr.r + r_step * (i+1));
        //     #endif

        // }
#if 1
        if(g_step > 0)
        {
            *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + rgb_sequence_buf[RGB_SEQUENCE].g) = smooth_pcb->s_clr.g - g_step * (i+1);
            #ifdef MY_DEBUG
            printf("\n  g =%d", smooth_pcb->s_clr.g - g_step * (i+1));
            #endif
        }
        else if(g_step < 0)
        {
            *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + rgb_sequence_buf[RGB_SEQUENCE].g) = smooth_pcb->e_clr.g + g_step * (led_qty - i);
            #ifdef MY_DEBUG
            printf("\n  g =%d", smooth_pcb->e_clr.g + g_step * (led_qty - i));
            #endif
        }

        // if( ((smooth_pcb->s_clr.g + g_step * (i+1)) >= 0) && ((smooth_pcb->s_clr.g + g_step * (i+1)) <= 255))
        // {
        //     *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + LED_STRIP_CH ) = smooth_pcb->s_clr.g + g_step * (i+1);
        //     #ifdef MY_DEBUG
        //     printf("\n  g =%d", smooth_pcb->s_clr.g + g_step * (i+1));
        //     #endif
        // }
        if(b_step > 0)
        {
            *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + rgb_sequence_buf[RGB_SEQUENCE].b) = smooth_pcb->s_clr.b - b_step * (i+1);
            #ifdef MY_DEBUG
            printf("\n  b =%d", smooth_pcb->s_clr.b + b_step * (i+1));
            #endif
        }
        else if(b_step < 0)
        {
            *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + rgb_sequence_buf[RGB_SEQUENCE].b) = smooth_pcb->e_clr.b + b_step * (led_qty - i);
            #ifdef MY_DEBUG
            printf("\n  b =%d", smooth_pcb->e_clr.b + b_step * (led_qty - i));
            #endif
        }
        // if( ((smooth_pcb->s_clr.b + b_step * (i+1)) >= 0) && ((smooth_pcb->s_clr.b + b_step * (i+1)) <= 255))
        // {
        //     *((smooth_pcb->pOutput + smooth_pcb->s_offset * LED_STRIP_CH) + LED_STRIP_CH ) = smooth_pcb->s_clr.b + b_step * (i+1);
        //     #ifdef MY_DEBUG
        //     printf("\n  b =%d", smooth_pcb->s_clr.b + b_step * (i+1));
        //     #endif
        // }
        // printf("\n  r =%d, g =%d, b =%d ", smooth_pcb->s_clr.r + r_step * (i+1), smooth_pcb->s_clr.g + g_step * (i+1), smooth_pcb->s_clr.b + b_step * (i+1));
        smooth_pcb->pOutput += LED_STRIP_CH;
#endif
    }
}

#endif





















