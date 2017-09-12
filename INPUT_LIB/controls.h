#ifndef	CONTROLS_H
#define	CONTROLS_H

#include "define.h"
#include "ud_str.h"

#define ARM_CON 

#ifdef ARM_CON

#define far 
#define xdata 

#endif




#define INPUT
#define OUTPUT

#ifdef OUTPUT
extern Str_out_point   outputs[];
void control_output(void);
extern uint8_t get_max_output(void);
extern void set_output_raw(uint8_t point,uint16_t value);
#endif
typedef enum
{
	INPUT_NOUSED = 0,
	INPUT_I0_20ma,
	INPUT_V0_5,
	INPUT_0_10V,
	INPUT_THERM,
} INPUT_RANGE;

extern uint8_t input_type[];
extern Str_in_point inputs[];
extern Str_table_point custom_tab[MAX_TBLS];


void control_input(void);

uint32_t swap_double( uint32_t dat );
uint16_t swap_word( uint16_t dat );
uint32_t get_input_value_by_range( uint8_t range, uint16_t raw );



 							   




// do it in own code

extern unsigned int Filter(uint8_t channel,uint16_t input);



extern void Set_Input_Type(uint8_t point);  
extern uint16_t get_input_raw(uint8_t point);


extern uint8_t get_max_input(void);

extern uint32_t conver_by_unit_5v(uint8 sample);
extern uint32_t conver_by_unit_10v(uint8 sample);
extern uint32_t conver_by_unit_custable(uint8_t point,uint8 sample);
extern uint32_t get_high_spd_counter(uint8_t point);



extern uint16_t output_raw[] ;
extern uint16_t input_raw[];

#endif
