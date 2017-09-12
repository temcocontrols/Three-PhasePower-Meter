#include "controls.h"


#ifdef OUTPUT_CONTROL

Str_out_point far	outputs[MAX_OUTS];

void control_output(void)
{
	Str_out_point *outs;
	U8_T point = 0;
	U32_T val;
//	U8_T loop;
	U32_T value;
	point = 0;
	outs = outputs;

	while( point < MAX_OUTS )
	{	
		if(point < get_max_output())
		{
			outs->decom = 0;		
			if( outs->range == not_used_output )
			{
				outs->value = 0L;
				val = 0;
			}
			else
			{	
				if(outs->switch_status == 0/*SW_OFF*/)
				{
					outs->value = 0L;
					outs->control = 0;
					set_output_raw(point,0);
				}
				else if(outs->switch_status == 2/*SW_HAND*/)
				{
					set_output_raw(point,1000);//output_raw[point] = 1000;
					outs->control = 1;
					switch( outs->range )
					{
						case V0_10:	
							outs->value = swap_double(10000);		
							break;
						case P0_100_Open:
						case P0_100_Close:
						case P0_100:
//						case P0_100_PWM:	
							outs->value = swap_double(100000);
							break;
						case P0_20psi:
						case I_0_20ma:
							outs->value = swap_double(20000);
							break;
						default:
							val = 0;
							break; 
					 }
				}
				else
				{
					if( outs->digital_analog == 0 ) // digital_analog 0=digital 1=analog
					{ // digtal input range 
						 if( outs->range >= OFF_ON && outs->range <= LOW_HIGH )
							if( outs->control ) val = 1000;
							else 
								val = 0;
						if( outs->range >= ON_OFF && outs->range <= HIGH_LOW )
							if( outs->control ) val = 0;
							else 
								val = 1000;
						if( outs->range >= custom_digital1 && outs->range <= custom_digital8 ) 
							if( outs->control ) val = 1000;
							else 
								val = 0;
						
					  set_output_raw(point,val);//output_raw[point] = val;					

						outs->value = swap_double(val);	
						
					}
					else if( outs->digital_analog == 1 ) //  analog
					{	
							value = swap_double(outs->value);
							val = value;
							switch( outs->range )
							{
								case V0_10:												
									set_output_raw(point, value * 100 / 1000);//output_raw[point] = value * 100 / 1000;
									break;
								case P0_100_Open:
								case P0_100_Close:
								case P0_100:
									set_output_raw(point, value * 10 / 1000);//output_raw[point] = value * 10 / 1000;

									break;
								case P0_20psi:
								case I_0_20ma:
									set_output_raw(point, value * 50 / 1000);//output_raw[point] = value * 50 / 1000;
									break;
								
								default:
									val = 0;
									break; 
							 }


							 outs->value = swap_double(val);	
					}
				}
			}

		}
		else
		{
			outs->sub_id = 0;
			outs->sub_product = 0;
			outs->sub_number = 0;
			
			outs->decom = 1;  // no used
		}
		point++;
		outs++;
				
	}	

}

#endif

