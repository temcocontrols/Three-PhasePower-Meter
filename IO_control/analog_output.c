#include "analog_output.h" 
#include "bitmap.h"
#include "bacnet.h" 
#include "stm32f10x.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "delay.h"
#include "controls.h"

static void output_config()
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	GPIO_ResetBits(GPIOD, GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15); 
}

static void output_initial()
{
	uint8 i;
	output_config();
	for(i=0;i<MAX_OUTS;i++)
	{
		outputs[i].switch_status = 3; 
	}
}

void vOutputTask(void *pvParameters)
{
	uint8 i;
	output_initial();
	for(i = 0;i<MAX_OUTS;i++) output_raw[i] = 0;
	delay_ms(100);
	for(;;)
	{ 
		control_output();
		delay_ms(500);
	}
}










