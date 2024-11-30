#include "common.h"
#include "touch.h"

#define ADC_SPEED 15	 /* 0~15, ADC转换时间(CPU时钟数) = (n+1)*32  ADCCFG */
#define RES_FMT (1 << 5) /* ADC结果格式 0: 左对齐, ADC_RES: D11 D10 D9 D8 D7 D6 D5 D4, ADC_RESL: D3 D2 D1 D0 0 0 0 0 */
						 /* ADCCFG      1: 右对齐, ADC_RES: 0 0 0 0 D11 D10 D9 D8, ADC_RESL: D7 D6 D5 D4 D3 D2 D1 D0 */

// 初始化触摸屏
void touch_init()
{
	ADCTIM = 0x3f; // 设置通道选择时间、保持时间、采样时间
	ADCCFG = RES_FMT + ADC_SPEED;
	// ADC模块电源打开后，需等待1ms，MCU内部ADC电源稳定后再进行AD转换
	ADC_CONTR = 0x80 + 0; // ADC on + channel
}

u16 Get_ADC12bitResult(u8 channel) // channel = 0~15
{
	ADC_RES = 0;
	ADC_RESL = 0;

	ADC_CONTR = (ADC_CONTR & 0xf0) | channel; // 设置ADC转换通道
	ADC_CONTR |= ADC_START;					  // 启动ADC转换
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	while (!(ADC_CONTR & ADC_FLAG))
		;
	// wait for ADC finish
	ADC_CONTR &= ~ADC_FLAG; // 清除ADC结束标志

	ADC_CONTR = (ADC_CONTR & 0xf0) | (u8)15;
	return (((u16)ADC_RES << 8) | ADC_RESL);
}

void xy_reset()
{
	// 0123推挽
	P0M0 |= 0x0f;
	P0M1 &= ~0x0f;
	// 0123拉低
	P0 &= ~0x0f;
	NOP4();
}

u16 x_read()
{
	xy_reset();

	// 0和2推挽，1，3输入
	P0M0 = (P0M0 & ~0x0a) | 0x05;
	P0M1 = (P0M1 & ~0x05) | 0x0a;
	P00 = 0;
	P02 = 1;

	return Get_ADC12bitResult(11);
}

u16 y_read()
{
	xy_reset();

	// 1和3推挽，0，2输入
	P0M0 = (P0M0 & ~0x05) | 0x0a;
	P0M1 = (P0M1 & ~0x0a) | 0x05;
	P01 = 1;
	P03 = 0;

	return Get_ADC12bitResult(8);
}

// 映射adc到实际坐标
u16 remap(u16 adc_value, u16 adc_min, u16 adc_max, u16 output_min, u16 output_max)
{
	u32 normalized;
	// 确保 ADC 输入范围有效
	if (adc_min >= adc_max)
	{
		return output_min; // 返回最小值
	}

	// 检查 adc_value 是否在有效范围内
	if (adc_value < adc_min)
	{
		adc_value = adc_min; // 如果低于最小值，设为最小值
	}
	else if (adc_value > adc_max)
	{
		adc_value = adc_max; // 如果超过最大值，设为最大值
	}

	// 将 ADC 值映射到 [0, 1] 区间（使用整数运算）
	normalized = (u32)(adc_value - adc_min) * (output_max - output_min) / (adc_max - adc_min);

	// 映射到实际的输出范围
	return output_min + normalized;
}

#define SAMP_CNT 8
#define SAMP_CNT_DIV2 4
u16 X, Y;
// 抄的adc滤波代码  https://www.cnblogs.com/yuphone/archive/2010/11/28/1890239.html
// 用adc测量电阻屏按压位置
u8 touch_scan(void)
{
	u8 i, j, k, min;
	u16 temp;
	u16 tempXY[2][SAMP_CNT], XY[2];
	// 采样
	for (i = 0; i < SAMP_CNT; i++)
	{
		tempXY[0][i] = x_read();
		tempXY[1][i] = y_read();
	}
	// 滤波
	for (k = 0; k < 2; k++)
	{ // 降序排列
		for (i = 0; i < SAMP_CNT - 1; i++)
		{
			min = i;
			for (j = i + 1; j < SAMP_CNT; j++)
			{
				if (tempXY[k][min] > tempXY[k][j])
					min = j;
			}
			temp = tempXY[k][i];
			tempXY[k][i] = tempXY[k][min];
			tempXY[k][min] = temp;
		}
		// 设定阈值
		if ((tempXY[k][SAMP_CNT_DIV2] - tempXY[k][SAMP_CNT_DIV2 - 1]) > 5)
			return 0;
		// 求中间值的均值
		XY[k] = (tempXY[k][SAMP_CNT_DIV2] + tempXY[k][SAMP_CNT_DIV2 - 1]) / 2;
	}
	X = XY[0];
	Y = XY[1];
	return 1;
}