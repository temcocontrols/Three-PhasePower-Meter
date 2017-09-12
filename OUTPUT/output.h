#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include "bitmap.h"
#include "stm32f10x_adc.h"


//DIGITAL OUTPUT
#define RELAY1_ON()		GPIO_SetBits(GPIOD, GPIO_Pin_8)
#define RELAY1_OFF()	GPIO_ResetBits(GPIOD, GPIO_Pin_8)

#define RELAY2_ON()		GPIO_SetBits(GPIOD, GPIO_Pin_9)
#define RELAY2_OFF()	GPIO_ResetBits(GPIOD, GPIO_Pin_9)

#define RELAY3_ON()		GPIO_SetBits(GPIOD, GPIO_Pin_10)
#define RELAY3_OFF()	GPIO_ResetBits(GPIOD, GPIO_Pin_10)

#define RELAY4_ON()		GPIO_SetBits(GPIOD, GPIO_Pin_11)
#define RELAY4_OFF()	GPIO_ResetBits(GPIOD, GPIO_Pin_11)

#define RELAY5_ON()		GPIO_SetBits(GPIOD, GPIO_Pin_12)
#define RELAY5_OFF()	GPIO_ResetBits(GPIOD, GPIO_Pin_12)

#define RELAY6_ON()		GPIO_SetBits(GPIOD, GPIO_Pin_13)
#define RELAY6_OFF()	GPIO_ResetBits(GPIOD, GPIO_Pin_13)

extern u16 output_setting;

void update_digit_output(void);
void output_init(void);
void output_refresh(void);


#endif
