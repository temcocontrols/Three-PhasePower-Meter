#ifndef _FILTER_H
#define _FILTER_H

#include "stm32f10x.h"
#define T3_TYPE2_10K_THERM	  0
#define T3_TYPE3_10K_THERM	  1 
#define T3_TYPE4_10K_THERM	  2 
/*range define*/
#define T3_RAW_DATA	0
#define T3_C_TYPE2		1
#define T3_F_TYPE2		2
#define T3_PERCENT	  	3
#define T3_ON_OFF		4
#define T3_OFF_ON		5
#define T3_PULSE		6
#define T3_LIGHTING	7
#define T3_C_TYPE3		8
#define T3_F_TYPE3		9


#define T3_NO_USE	0
#define T3_I020ma	1
#define T3_V05		2
#define T3_V010		3





//u16 filter_inputs(u8 channel,u16 input) ;
signed int RangeConverter(unsigned char function, signed int para,signed int cal,unsigned char i);
#endif 
