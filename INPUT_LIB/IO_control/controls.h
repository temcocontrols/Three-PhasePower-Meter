#ifndef	CONTROLS_H
#define	CONTROLS_H



//#define ASIX_CON
#define ARM_CON 

#ifdef ARM_CON

#define far 
#define xdata 
#define code 

#endif


#include "ud_str.h"


#define INPUT
#define OUTPUT

typedef enum
{
	INPUT_NOUSED = 0,
	INPUT_I0_20ma,
	INPUT_V0_5,
	INPUT_0_10V,
	INPUT_THERM,
}E_IN_TYPE;

extern uint8_t far input_type[32];
extern Str_in_point  far  inputs[MAX_INS];
extern Str_out_point far	outputs[MAX_OUTS];

void control_input(void);
void control_output(void);
uint32_t swap_double( uint32_t dat );
uint16_t swap_word( uint16_t dat );
uint32_t get_input_value_by_range(uint8_t range, uint16_t raw);






// do it in own code
extern void Set_Input_Type(uint8_t point);  
extern uint16_t get_input_raw(uint8_t point);
extern void set_output_raw(uint8_t point,uint16_t value);
extern uint16_t get_output_raw(uint8_t point);
extern uint16_t Filter(uint8_t channel,uint16_t input);
extern uint8_t get_max_output(void);
extern uint8_t get_max_input(void);
extern uint32_t conver_by_unit_5v(uint32_t sample);
extern uint32_t conver_by_unit_10v(uint32_t sample);
extern uint32_t conver_by_unit_custable(uint8_t point,uint32_t sample);
extern uint32_t get_high_spd_counter(uint8_t point);

#ifdef ASIX_CON
void map_extern_output(uint8_t point);
extern uint8_t get_max_internal_input(void);
extern uint8_t get_max_internal_output(void);
#endif

#endif

