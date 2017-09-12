#ifndef __LED_H
#define __LED_H
 
#include "bitmap.h"

#define LED_ON	0
#define LED_OFF	1	


#define ON 		1
#define OFF 	0
// DS0
#define LED_HEARTBEAT_OFF()	GPIO_SetBits(GPIOD, GPIO_Pin_2);
#define LED0_HEARTBEAT_ON()	GPIO_ResetBits(GPIOD, GPIO_Pin_2);
// DS1
#define LED1_MAINNET_OFF()	GPIO_SetBits(GPIOD, GPIO_Pin_0);
#define LED1_MAINNET_ON()	GPIO_ResetBits(GPIOD, GPIO_Pin_0);
// DS2
#define LED2_SUBNET_OFF()	GPIO_SetBits(GPIOD, GPIO_Pin_1);
#define LED2_SUBNET_ON()	GPIO_ResetBits(GPIOD, GPIO_Pin_1);


void led_init(void);
void refresh_led(void);
void vLED0Task( void *pvParameters);

#define RX1_LED		 PAout(15) 
#define TX1_LED		 PBout(0) 	

#define Relay1_LED	 PEout(1)
#define Relay2_LED	 PEout(2)
#define Relay3_LED	 PEout(3)
#define Relay4_LED	 PEout(4)


#define Input1_LED	PAout(14) 
#define Input2_LED	PAout(13) 
#define Input3_LED	PAout(12) 
#define Input4_LED	PAout(11) 

#define LED_HEARTBEAT	PEout(0)
		 				    
#endif
