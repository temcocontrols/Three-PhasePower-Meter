#include "modbus.h"
#include "define.h"
#include "controls.h"
#include "../output/output.h"


void digital_output_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOD, GPIO_InitStructure.GPIO_Pin);	
}
void analog_output_init(void)
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM3, ENABLE); //TIM3_CH1->PC6, TIM3_CH2->PC7, TIM3_CH3->PC8, TIM3_CH4->PC9	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9;  //TIM3_CH1-4
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Period = 10000;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;

	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	TIM_OC2Init(TIM3, &TIM_OCInitStructure);
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4Init(TIM3, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
	
	TIM_Cmd(TIM3, ENABLE);
}

void output_init(void)
{
	digital_output_init();
	analog_output_init();
}

// TIM_Period = 10000, so duty = real_pwm_duty * 10000
void set_pwm_output(u8 channel, u16 duty)
{
	u16 compare_value = duty;
	// calculate the compare value and then set it to change the pwm duty
	switch(channel)
	{
		case 1:
			TIM_SetCompare1(TIM3, compare_value);
		break;
		case 2:
			TIM_SetCompare2(TIM3, compare_value);
		break;
		case 3:
			TIM_SetCompare3(TIM3, compare_value);
		break;
		case 4:
			TIM_SetCompare4(TIM3, compare_value);
		break;
	}
}

void output_refresh(void)
{
	u8 loop ;
	u8 oneswitch_buf;
	
	for(loop = 0; loop < 4/*MAX_AO*/; loop++)
	{
		oneswitch_buf = modbus.switch_gourp[1] >> (loop * 2) & 0x03;
		outputs[loop + MAX_DO].switch_status = oneswitch_buf;
		if(oneswitch_buf == SW_OFF) 
		{
			set_pwm_output(loop, 0);
			outputs[loop+MAX_DO].value =  0;
		}
		else if(oneswitch_buf == SW_HAND) 
		{
			set_pwm_output(loop, 10000);
			outputs[loop + MAX_DO].value = 10000;
		}
		else if(oneswitch_buf == SW_AUTO) 
		{
			set_pwm_output(loop, outputs[loop + MAX_DO].value);			
		}
	}
}

void update_digit_output(void) 
{
//	RELAY1_ON();
//	delay_ms(1000);
//	RELAY1_OFF();
//	delay_ms(1000);
//	
//	RELAY2_ON();
//	delay_ms(1000);
//	RELAY2_OFF();
//	delay_ms(1000);
//	
//	RELAY3_ON();
//	delay_ms(1000);
//	RELAY3_OFF();
//	delay_ms(1000);
//	
//	RELAY4_ON();
//	delay_ms(1000);
//	RELAY4_OFF();
//	delay_ms(1000);
//	
//	RELAY5_ON();
//	delay_ms(1000);
//	RELAY5_OFF();
//	delay_ms(1000);
//	
//	RELAY6_ON();
//	delay_ms(1000);
//	RELAY6_OFF();
//	delay_ms(1000);
	
//	set_pwm_output(1, 0);
//	delay_ms(10000);
//	set_pwm_output(1, 2500);
//	delay_ms(10000);
	set_pwm_output(1, 5000);
	delay_ms(10000);
//	set_pwm_output(1, 7500);
//	delay_ms(10000);
//	set_pwm_output(1, 10000);
//	delay_ms(10000);
	
//	set_pwm_output(2, 0);
//	delay_ms(2000);
//	set_pwm_output(2, 2500);
//	delay_ms(2000);
//	set_pwm_output(2, 5000);
//	delay_ms(2000);
//	set_pwm_output(2, 7500);
//	delay_ms(2000);
//	set_pwm_output(2, 10000);
//	delay_ms(2000);
//	
//	set_pwm_output(3, 0);
//	delay_ms(2000);
//	set_pwm_output(3, 2500);
//	delay_ms(2000);
//	set_pwm_output(3, 5000);
//	delay_ms(2000);
//	set_pwm_output(3, 7500);
//	delay_ms(2000);
//	set_pwm_output(3, 10000);
//	delay_ms(2000);
//	
//	set_pwm_output(4, 0);
//	delay_ms(2000);
//	set_pwm_output(4, 2500);
//	delay_ms(2000);
//	set_pwm_output(4, 5000);
//	delay_ms(2000);
//	set_pwm_output(4, 7500);
//	delay_ms(2000);
//	set_pwm_output(4, 10000);
//	delay_ms(2000);
}

//#define DO_OFF 0 
//#define DO_ON  1000

//void update_digit_output(void) 
//{
//	u8 loop ;
//	u8 oneswitch_buf ;
//	for(loop = 0; loop < 6; loop++)
//	{
//		oneswitch_buf = modbus.switch_gourp[0]>>(loop*2) & 0x03 ;
//		outputs[loop].switch_status = oneswitch_buf;
//		if(oneswitch_buf == SW_OFF) 
//		{
//			set_output(DO_CONFIG[loop].TIMx, DO_CONFIG[loop].chip_channel,DO_OFF);
//			outputs[loop].value = 0 ; 
//		}
//		else if(oneswitch_buf == SW_HAND)
//		{
//			set_output(DO_CONFIG[loop].TIMx, DO_CONFIG[loop].chip_channel,DO_ON);
//			outputs[loop].value = 1 ;
//		}
//		else if(oneswitch_buf == SW_AUTO) 
//		{
//			if(outputs[loop].value == 0)
//			set_output(DO_CONFIG[loop].TIMx, DO_CONFIG[loop].chip_channel,DO_OFF);
//			else
//			set_output(DO_CONFIG[loop].TIMx, DO_CONFIG[loop].chip_channel,DO_ON);		
//		}
//	}	
//}

