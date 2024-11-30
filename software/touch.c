#include "common.h"
#include "touch.h"

#define ADC_SPEED 15	 /* 0~15, ADCת��ʱ��(CPUʱ����) = (n+1)*32  ADCCFG */
#define RES_FMT (1 << 5) /* ADC�����ʽ 0: �����, ADC_RES: D11 D10 D9 D8 D7 D6 D5 D4, ADC_RESL: D3 D2 D1 D0 0 0 0 0 */
						 /* ADCCFG      1: �Ҷ���, ADC_RES: 0 0 0 0 D11 D10 D9 D8, ADC_RESL: D7 D6 D5 D4 D3 D2 D1 D0 */

// ��ʼ��������
void touch_init()
{
	ADCTIM = 0x3f; // ����ͨ��ѡ��ʱ�䡢����ʱ�䡢����ʱ��
	ADCCFG = RES_FMT + ADC_SPEED;
	// ADCģ���Դ�򿪺���ȴ�1ms��MCU�ڲ�ADC��Դ�ȶ����ٽ���ADת��
	ADC_CONTR = 0x80 + 0; // ADC on + channel
}

u16 Get_ADC12bitResult(u8 channel) // channel = 0~15
{
	ADC_RES = 0;
	ADC_RESL = 0;

	ADC_CONTR = (ADC_CONTR & 0xf0) | channel; // ����ADCת��ͨ��
	ADC_CONTR |= ADC_START;					  // ����ADCת��
	_nop_();
	_nop_();
	_nop_();
	_nop_();
	while (!(ADC_CONTR & ADC_FLAG))
		;
	// wait for ADC finish
	ADC_CONTR &= ~ADC_FLAG; // ���ADC������־

	ADC_CONTR = (ADC_CONTR & 0xf0) | (u8)15;
	return (((u16)ADC_RES << 8) | ADC_RESL);
}

void xy_reset()
{
	// 0123����
	P0M0 |= 0x0f;
	P0M1 &= ~0x0f;
	// 0123����
	P0 &= ~0x0f;
	NOP4();
}

u16 x_read()
{
	xy_reset();

	// 0��2���죬1��3����
	P0M0 = (P0M0 & ~0x0a) | 0x05;
	P0M1 = (P0M1 & ~0x05) | 0x0a;
	P00 = 0;
	P02 = 1;

	return Get_ADC12bitResult(11);
}

u16 y_read()
{
	xy_reset();

	// 1��3���죬0��2����
	P0M0 = (P0M0 & ~0x05) | 0x0a;
	P0M1 = (P0M1 & ~0x0a) | 0x05;
	P01 = 1;
	P03 = 0;

	return Get_ADC12bitResult(8);
}

// ӳ��adc��ʵ������
u16 remap(u16 adc_value, u16 adc_min, u16 adc_max, u16 output_min, u16 output_max)
{
	u32 normalized;
	// ȷ�� ADC ���뷶Χ��Ч
	if (adc_min >= adc_max)
	{
		return output_min; // ������Сֵ
	}

	// ��� adc_value �Ƿ�����Ч��Χ��
	if (adc_value < adc_min)
	{
		adc_value = adc_min; // ���������Сֵ����Ϊ��Сֵ
	}
	else if (adc_value > adc_max)
	{
		adc_value = adc_max; // ����������ֵ����Ϊ���ֵ
	}

	// �� ADC ֵӳ�䵽 [0, 1] ���䣨ʹ���������㣩
	normalized = (u32)(adc_value - adc_min) * (output_max - output_min) / (adc_max - adc_min);

	// ӳ�䵽ʵ�ʵ������Χ
	return output_min + normalized;
}

#define SAMP_CNT 8
#define SAMP_CNT_DIV2 4
u16 X, Y;
// ����adc�˲�����  https://www.cnblogs.com/yuphone/archive/2010/11/28/1890239.html
// ��adc������������ѹλ��
u8 touch_scan(void)
{
	u8 i, j, k, min;
	u16 temp;
	u16 tempXY[2][SAMP_CNT], XY[2];
	// ����
	for (i = 0; i < SAMP_CNT; i++)
	{
		tempXY[0][i] = x_read();
		tempXY[1][i] = y_read();
	}
	// �˲�
	for (k = 0; k < 2; k++)
	{ // ��������
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
		// �趨��ֵ
		if ((tempXY[k][SAMP_CNT_DIV2] - tempXY[k][SAMP_CNT_DIV2 - 1]) > 5)
			return 0;
		// ���м�ֵ�ľ�ֵ
		XY[k] = (tempXY[k][SAMP_CNT_DIV2] + tempXY[k][SAMP_CNT_DIV2 - 1]) / 2;
	}
	X = XY[0];
	Y = XY[1];
	return 1;
}