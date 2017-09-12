
#include "define.h"
#include "../input/input.h"
#include "delay.h"
#include "modbus.h"
#include "filter.h"
#include "controls.h"

uint16_t data_change[MAX_AI_CHANNEL] = {0};
uint16_t AD_Value[MAX_AI_CHANNEL] = {0}; 

void inputs_io_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_10;  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; 
	GPIO_Init(GPIOC, &GPIO_InitStructure);	
}

void inputs_adc_init(void)
{
	ADC_InitTypeDef ADC_InitStructure;
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	/* configuration ------------------------------------------------------*/  
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;

	ADC_InitStructure.ADC_NbrOfChannel = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_Cmd(ADC1, ENABLE); 
	/* Enable ADC1 reset calibaration register */   
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1)== SET)
	{
//		printf("ADC_GetResetCalibrationStatus() Error!\r\n");
		delay_ms(500);
	}		
     ADC_StartCalibration(ADC1);
     while(ADC_GetCalibrationStatus(ADC1) == SET);
}

void inputs_init(void) 
{
	inputs_io_init();
	inputs_adc_init();
}

u16 ADC_getChannal(ADC_TypeDef* ADCx, u8 channal)
{
	 uint16_t tem = 0;
	 ADC_ClearFlag(ADCx, ADC_FLAG_EOC);
	 ADC_RegularChannelConfig(ADCx, channal, 1, ADC_SampleTime_55Cycles5);
	 ADC_SoftwareStartConvCmd(ADCx, ENABLE);         
	 while(ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET);
	 tem = ADC_GetConversionValue(ADCx);
	 return tem;
}

void select_channel(u8 chn)
{
	CHA_SEL3 = 0;
	switch(chn)
	{
		case 0:
			CHA_SEL2 = 0;
			CHA_SEL1 = 0;
			CHA_SEL0 = 0;
		break;
		case 1:
			CHA_SEL2 = 0;
			CHA_SEL1 = 0;
			CHA_SEL0 = 1;
		break;
		case 2:
			CHA_SEL2 = 0;
			CHA_SEL1 = 1;
			CHA_SEL0 = 0;
		break;
		case 3:
			CHA_SEL2 = 0;
			CHA_SEL1 = 1;
			CHA_SEL0 = 1;
		break;
		case 4:
			CHA_SEL2 = 1;
			CHA_SEL1 = 0;
			CHA_SEL0 = 0;
		break;
		case 5:
			CHA_SEL2 = 1;
			CHA_SEL1 = 0;
			CHA_SEL0 = 1;
		break;
		case 6:
			CHA_SEL2 = 1;
			CHA_SEL1 = 1;
			CHA_SEL0 = 0;
		break;
		case 7:
			CHA_SEL2 = 1;
			CHA_SEL1 = 1;
			CHA_SEL0 = 1;
		break;
	}
	CHA_SEL3 = 1;
}

void range_set_func(u8 range)
{
	if(range == T3_V05)
	{
		RANGE_SET0 = 1;
		RANGE_SET1 = 0;
	}
	else if (range == T3_V010)
	{
		RANGE_SET0 = 0;
		RANGE_SET1 = 1;
	}
	else if (range == T3_I020ma)
	{
		RANGE_SET0 = 0;
		RANGE_SET1 = 0;
	}
	else
	{
		RANGE_SET0 = 1;
		RANGE_SET1 = 1;
	}
}

void inputs_scan(void)
{
	static u8 channel_count = 0;
	static u16 sum = 0;
	static u8 sum_times = 0;
	
	sum += ADC_getChannal(ADC1, ADC_Channel_14);
	sum_times++;
	if(sum_times == 5)
	{
		sum_times = 0;
		AD_Value[channel_count] = sum / 5;
		sum = 0;
		channel_count++;
		channel_count %= MAX_AI_CHANNEL;
		select_channel(channel_count);
		range_set_func((inputs[channel_count].decom >> 4) & 0x0f);
	}
}
