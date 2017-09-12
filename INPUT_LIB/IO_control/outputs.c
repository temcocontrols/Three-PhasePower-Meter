#include "controls.h"


#ifdef OUTPUT

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
#ifdef ASIX_CON
			if(point < get_max_internal_output())
			{		
				outs->decom = 0;
			}
			else
				outs->switch_status = 2/*SW_AUTO*/;
#endif	

#ifdef ARM_CON
			outs->decom = 0;
#endif			
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
					//output_raw[point] = 0;
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
						case P0_100_PWM:	
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
#ifdef ASIX_CON						
						if(point < get_max_internal_output())  // internal output
#endif
						{
							value = swap_double(outs->value);
							val = value;
							switch( outs->range )
							{
								case V0_10:	
									//outs->count = 10;												
									set_output_raw(point, value * 100 / 1000);//output_raw[point] = value * 100 / 1000;
									break;
								case P0_100_Open:
								case P0_100_Close:
								case P0_100:
								case P0_100_PWM:
									//outs->count = 100;
									set_output_raw(point, value * 10 / 1000);//output_raw[point] = value * 10 / 1000;
								/*	if( outs->m_del_low < outs->s_del_high )
									{
										delta = outs->s_del_high - outs->m_del_low;
										value *= delta;
										value += (long)( outs->m_del_low * 100000L );
									}
									else
									{
										delta =  outs->m_del_low - outs->s_del_high;
										value *= delta;
										value += (long)( outs->s_del_high * 100000L );
									}
									val = (Byte)( value * 213 / 10000000L );*/
									break;
								case P0_20psi:
								case I_0_20ma:
									//outs->count = 20;
									set_output_raw(point, value * 50 / 1000);//output_raw[point] = value * 50 / 1000;
									break;
								
								default:
									val = 0;
									break; 
							 }

//#if MINI						
//						 if(flag_output == ADJUST_AUTO)
//						 {
//							  if(output_raw_back[point] != output_raw[point])
//								{
//									output_raw_back[point] = output_raw[point];
//									AO_auto[point] = output_raw[point];
//								}
//						}
//#endif

							 outs->value = swap_double(val);	
						}
#ifdef ASIX_CON
						else// external ouput 
						{  // range is 0-10v
							if(outs->read_remote == 1)
							{
								val = 10000000 / 4095;
								val = val * get_output_raw(point) / 1000;
								outs->read_remote = 0;
							}
							else
							{		
								val = swap_double(outs->value);	
								val = val * 4095 / 10000;
								// ?????????
								if(val < get_output_raw(point))
								{
									if(get_output_raw(point) - val == 1)
										set_output_raw(point,val + 1);//output_raw[point] = val + 1;
									if(get_output_raw(point) - val == 2)
										set_output_raw(point,val + 2);//output_raw[point] = val + 2;
								}
								else
									set_output_raw(point,val);//output_raw[point] = val;
							}
						}
#endif
					}
				}
			}
#ifdef ASIX_CON
			map_extern_output(point);
#endif

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

