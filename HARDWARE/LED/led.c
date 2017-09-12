#include "led.h"  
#include "bitmap.h"
#include "bacnet.h"  
#include "stm32f10x_adc.h"
#include "stm32f10x_dma.h"
#include "delay.h" 
#include "FreeRTOS.h"
#include "modbus.h"
#include "task.h"
#include "controls.h"
#include "analog_output.h"
#include "store.h"
void led_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	GPIO_ResetBits(GPIOD, GPIO_Pin_0 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable , ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure); 
	GPIO_ResetBits(GPIOA, GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);	

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure); 
	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}

/*****************************LED TABLE**************************************\
*MAIN_NET ---- PD0
*SUB_NET ----- PD1
*HEARTBEAT --- PD2
\****************************************************************************/
void refresh_led(void)
{
	PDout(0) = ~PDout(0);
}


void wifi_init(void);
void wifi_test(void);

void vLED0Task( void *pvParameters )
{  
	uint8 i = 0;
	led_init();
	mass_flash_init();
	wifi_init();
	uart.rx1_flag = 2;
	uart.tx1_flag = 2;
	delay_ms(100);  
	
	for( ;; )
	{   
		
		if(i < 12) i++;
		else
//heart beart
		{
			i = 0;
			LED_HEARTBEAT = ~LED_HEARTBEAT; 
//			Input1_LED = ~Input1_LED;
//			Input2_LED = ~Input2_LED;
//			Input3_LED = ~Input3_LED;
//			Input4_LED = ~Input4_LED;
//			
//			Relay1_LED = ~Relay1_LED;
//			Relay2_LED = ~Relay2_LED;
//			Relay3_LED = ~Relay3_LED;
//			Relay4_LED = ~Relay4_LED;
		}
		
//uart1 rx led		
		if(uart.rx1_flag == 2)
		{
			uart.rx1_flag = 1;
			RX1_LED = LED_ON;
		}
		else if(uart.rx1_flag == 1)
		{
			uart.rx1_flag = 0;
			RX1_LED = LED_OFF;
		}
//uart1 tx led		
		if(uart.tx1_flag == 2)
		{
			uart.tx1_flag = 1;
			TX1_LED = LED_ON;
		}
		else if(uart.tx1_flag == 1)
		{
			uart.tx1_flag = 0;
			TX1_LED = LED_OFF;
		}
	 
//relay output led
		if(output_raw[0] !=0)
		{
			Relay1_LED = LED_ON;
			Relay1_Out = ON;
		}
		else
		{
			Relay1_LED = LED_OFF;
			Relay1_Out = OFF;
		}
		
		if(output_raw[1] !=0)
		{
			Relay2_LED = LED_ON;
			Relay2_Out = ON;
		}
		else
		{
			Relay2_LED = LED_OFF;
			Relay2_Out = OFF;
		}
		
		if(output_raw[2] !=0)
		{
			Relay3_LED = LED_ON;
			Relay3_Out = ON;
		}
		else
		{
			Relay3_LED = LED_OFF;
			Relay3_Out = OFF;
		}
		
		if(output_raw[3] !=0)
		{
			Relay4_LED = LED_ON;
			Relay4_Out = ON;
		}
		else
		{
			Relay4_LED = LED_OFF;
			Relay4_Out = OFF;
		}
		
//input led

		if(inputs[0].range !=UNUSED) Input1_LED = LED_ON;
		else Input1_LED = LED_OFF;
		
		if(inputs[1].range !=UNUSED) Input2_LED = LED_ON;
		else Input2_LED = LED_OFF;
		
		if(inputs[2].range !=UNUSED) Input3_LED = LED_ON;
		else Input3_LED = LED_OFF;
		
		if(inputs[3].range !=UNUSED) Input4_LED = LED_ON;
		else Input4_LED = LED_OFF;
		 
		
 		Flash_Write_Mass();
		wifi_test();
		
		vTaskDelay(40 / portTICK_RATE_MS);
	}
}


void wifi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure); 
	GPIO_ResetBits(GPIOD, GPIO_Pin_1 | GPIO_Pin_2 );

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE); 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure); 
	GPIO_ResetBits(GPIOC, GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);	 
}

void wifi_test(void)
{
	PDout(1) = ~PDout(1);
	PDout(2) = ~PDout(2);
	PCout(8) = ~PCout(8);
	PCout(9) = ~PCout(9);
	PCout(10) = ~PCout(10);
	PCout(11) = ~PCout(11);
	PCout(12) = ~PCout(12);
}
