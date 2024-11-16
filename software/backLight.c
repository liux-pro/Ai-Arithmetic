#include "common.h"
#include "STC32G_NVIC.h"
#include "STC32G_Clock.h"
#include "STC32G_PWM.h"
#include "STC32G_Timer.h"
#include "STC32G_GPIO.h"


void configBackLightPWM(u8 brightness)
{
	const u16 period = (u32)MAIN_Fosc / (u32)(20 * 1000);
	PWMx_InitDefine PWMx_InitStructure;

	// PWM3P_2 P0.4
	P0_MODE_IO_PU(GPIO_Pin_4);

	PWMx_InitStructure.PWM_Mode = CCMRn_PWM_MODE1;							// 模式,		CCMRn_FREEZE,CCMRn_MATCH_VALID,CCMRn_MATCH_INVALID,CCMRn_ROLLOVER,CCMRn_FORCE_INVALID,CCMRn_FORCE_VALID,CCMRn_PWM_MODE1,CCMRn_PWM_MODE2
	PWMx_InitStructure.PWM_Duty = (u32)brightness * (u32)period / (u32)255; // PWM占空比时间, 0~Period
	PWMx_InitStructure.PWM_EnoSelect = ENO3P;								// 输出通道选择,	ENO1P,ENO1N,ENO2P,ENO2N,ENO3P,ENO3N,ENO4P,ENO4N / ENO5P,ENO6P,ENO7P,ENO8P
	PWM_Configuration(PWM3, &PWMx_InitStructure);							// 初始化PWM3

	PWMx_InitStructure.PWM_Period = period;		   // 周期时间,   0~65535
	PWMx_InitStructure.PWM_DeadTime = 0;		   // 死区发生器设置, 0~255
	PWMx_InitStructure.PWM_MainOutEnable = ENABLE; // 主输出使能, ENABLE,DISABLE
	PWMx_InitStructure.PWM_CEN_Enable = ENABLE;	   // 使能计数器, ENABLE,DISABLE
	PWM_Configuration(PWMA, &PWMx_InitStructure);  // 初始化PWM通用寄存器,  PWMA,PWMB

	PWMA_PS = (PWMA_PS & ~0x30) | 0x10; // 切换到 PWM3_2 ,也就是 C1PS=0b01, P04P05    这里用的是stc32的库函数，在这个地方不兼容，所以用寄存器
	NVIC_PWM_Init(PWMA, DISABLE, Priority_0);
}