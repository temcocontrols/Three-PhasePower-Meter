
#include "define.h"
#include "modbus.h"
#include "ud_str.h"
#include "controls.h"
#include "../input/input.h"

static uint16_t analog_buffer[MAX_AI_CHANNEL] ;

// 根据input_type[point]设置相应的硬件
void Set_Input_Type(uint8_t point)
{
	
}


// 返回input的原始ADC值，10bit， 如果不是10位，需要做相应转化
uint16_t get_input_raw(uint8_t point)
{
//	return AD_Value[point] / 4;
	return AD_Value[point];
}

// 设置input的原值
// if digital output, 0 - off, 1000 - on
// if analog ouput, 0 - 10v 对应 0-1023
void set_output_raw(uint8_t point,uint16_t value)
{
//	output_raw[point] = value;
	inputs[value].value = value ;
}

// 把adc值转化成5v的range显示
// 如果是input最新模块不需要修改
unsigned int conver_by_unit_5v(uint8_t sample)
{	
//	sample =  ( sample * 5000L ) >> 10;	
	return ( sample * 5000L ) >> 10;		
}


// 把adc值转化成10v的range显示
// 如果是input最新模块不需要修改
unsigned int conver_by_unit_10v(uint8_t sample)
{
	return  ( sample * 10000L ) >> 10;

}

// 把adc值转化成customer table的range显示
// 如果是input最新模块不需要修改
unsigned int conver_by_unit_custable(uint8_t point,uint8_t sample)
{	
	if(input_type[point] == INPUT_V0_5)
	{
			return  ( sample * 50L ) >> 10;		
	}
	else if(input_type[point] == INPUT_I0_20ma)
	{
		return ( 20000L * sample ) >> 10; 
	}
	else if(input_type[point] == INPUT_0_10V)
	{
		return (sample * 10000) >> 10;
	}	
	else if(input_type[point] == INPUT_THERM)
	{
		return get_input_value_by_range( inputs[point].range, inputs[point].value );
	}
	
	return 0;
}


// 返回最大input数目
uint8_t get_max_input(void)
{	
	return MAX_INS;
}
// 返回最大output数目
uint8_t get_max_output(void)
{	
	return MAX_OUTS;
}
// if有high speed 功能，返回high_spd_counter
uint32_t get_high_spd_counter(uint8_t point)
{
	//return (high_spd_counter[point] + high_spd_counter_tempbuf[point]) * 1000;
	return modbus.pulse[point].word ;

}

 unsigned int Filter(uint8_t channel,uint16_t input)
{
	// -------------FILTERING------------------
	s16  siDelta = 0;
	u16  uiResult = 0;
	u8  i = 0;
  	u16  uiTemp = 0;
	i = channel;
	uiTemp = input;  
	siDelta = uiTemp - analog_buffer[i];    //compare new reading and old reading

	// If the difference in new reading and old reading is greater than 5 degrees, implement rough filtering.
  if (( siDelta >= 100 ) || ( siDelta <= -100)) // deg f
	  	analog_buffer[i] = analog_buffer[i] + (siDelta >> 1);//1 
 			
	// Otherwise, implement fine filtering.
	else
	{			
	  analog_buffer[i] = (u32)analog_buffer[i]*inputs[i].filter;
	  analog_buffer[i] += (u32)uiTemp;
	  analog_buffer[i] = (u16)(analog_buffer[i]/(inputs[i].filter));			 	 
	}
	uiResult = analog_buffer[i]; 
 

	return uiResult;	
}
