#ifndef __INPUT_H
#define __INPUT_H

#include "bitmap.h"
#include "define.h"
#include "stm32f10x_adc.h"


extern uint16_t AD_Value[MAX_AI_CHANNEL]; 
extern uint16_t data_change[MAX_AI_CHANNEL];

void inputs_init(void);
void inputs_scan(void);


#endif
