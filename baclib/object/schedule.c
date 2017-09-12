
/* Schedule Objects customize -- writen by chesea*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacnet.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"

#include "schedule.h"

#if BAC_SCHEDULE
/*
reliable have following property  

-present_value
-description
-list_of_object_property_references
-weeky_schedule
-out_if_service

// to be added more

-status_flag
-effectieve_period
-exception_schedule
-schedule_default
-local_date
-local_time
-reliabiltity
-prioritoy_for_writing
*/






uint8_t  SCHEDULES;

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Schedule_Valid_Instance(uint32_t object_instance)
{
    if (object_instance < MAX_SCHEDULES)
        return true;
}

/* we simply have 0-n object instances. */
unsigned Schedule_Count(void)
{	
		return SCHEDULES;
}

/* we simply have 0-n object instances. */
uint32_t Schedule_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances. */
unsigned Schedule_Instance_To_Index(
    uint32_t object_instance)
{

	return object_instance;
}


/* return apdu length, or -1 on error */
/* assumption - object has already exists */
// read
int Schedule_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;   /* return value */
    BACNET_BIT_STRING  bit_string;
    BACNET_CHARACTER_STRING  char_string;
    unsigned object_index;

		int len = 0;
	  int far i;
	  int far day;

    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = 
                encode_application_object_id(&apdu[0], OBJECT_SCHEDULE,
                object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different.
               Note that Object-Name must be unique in this device */
        case PROP_OBJECT_NAME:
						characterstring_init_ansi(&char_string, get_label(SCHEDULE,object_instance));
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
						characterstring_init_ansi(&char_string,get_description(SCHEDULE,object_instance));
						apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_SCHEDULE);
            break;
        case PROP_PRESENT_VALUE:
            object_index = Schedule_Instance_To_Index(object_instance);
            apdu_len =
                encode_application_unsigned(&apdu[0], Get_bacnet_value_from_buf(SCHEDULE,0,object_index)/*AI_Present_Value[object_index]*/);
            break;       
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            object_index =
                Schedule_Instance_To_Index( object_instance);
            apdu_len = encode_application_boolean(&apdu[0], get_AM_Status(SCHEDULE,object_instance));
            break;
				
            apdu_len = encode_application_boolean(&apdu[0], false);
            break;
        case PROP_WEEKLY_SCHEDULE:	
            if (array_index == 0)       /* count, always 7 */
						{
                apdu_len = encode_application_unsigned(&apdu[0], 7);
						}
            else if (array_index == BACNET_ARRAY_ALL) { /* full array */
             object_index =
                    Schedule_Instance_To_Index(object_instance);  
						
                for (day = 0; day < 7; day++) { 
									apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                    for (i = 0; i < Get_TV_count(object_index,day); i++) 
									  {
                        apdu_len +=
                            bacapp_encode_time_value(&apdu[apdu_len],
                            Get_Time_Value(object_index,day,i));
                    }
                    apdu_len += encode_closing_tag(&apdu[apdu_len], 0);

                }
            } 
						else if (array_index <= 7) {      /* some array element */
                int day = array_index - 1;
                apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                for (i = 0; i < Get_TV_count(object_index,day)/*CurrentSC->Weekly_Schedule[day].TV_Count*/; i++) {
                    apdu_len +=
                        bacapp_encode_time_value(&apdu[apdu_len],
                        Get_Time_Value(object_index,day,i)/*&CurrentSC->Weekly_Schedule[day].Time_Values[i]*/);
                }
                apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
            } else {    /* out of bounds */
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }

            break;
        case PROP_EFFECTIVE_PERIOD:
//					apdu_len = encode_bacnet_date(&apdu[0], Get_Object_Property_References(0));
//          apdu_len +=  encode_bacnet_date(&apdu[apdu_len], Get_Object_Property_References(0));
            break;
	      case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            for (i = 0; i < 1; i++) {
                apdu_len +=
                    bacapp_encode_device_obj_property_ref(&apdu[apdu_len],
                    Get_Object_Property_References(i));
            }
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (array_index != BACNET_ARRAY_ALL)) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = -1;
    }
	
    return apdu_len;
}


/* returns true if successful */
// write
bool Schedule_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    int far len = 0;
    BACNET_APPLICATION_DATA_VALUE far value;
		BACNET_TIME_VALUE far time_value;
    /* decode the some of the request */
	
	  if(!IS_CONTEXT_SPECIFIC(*wp_data->application_data))
		{
				len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
		

    /* FIXME: len < application_data_len: more data? */
			if (len < 0) {
					/* error while decoding - a value larger than we can handle */
					wp_data->error_class = ERROR_CLASS_PROPERTY;
					wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
					return false;
			}

			/*  only array properties can have array options */
			if ((wp_data->object_property != PROP_EVENT_TIME_STAMPS) &&
					(wp_data->array_index != BACNET_ARRAY_ALL)) {
					wp_data->error_class = ERROR_CLASS_PROPERTY;
					wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;						
					return false;
			}
		}
		
		object_index = Schedule_Instance_To_Index(wp_data->object_instance);
		
    switch ((int) wp_data->object_property) {
        case PROP_PRESENT_VALUE:
			if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                object_index =
                    Schedule_Instance_To_Index(wp_data->object_instance);

				wirte_bacnet_value_to_buf(SCHEDULE,wp_data->priority,object_index,value.type.Boolean);

                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }

            break;
				// add it by chelsea
				case PROP_WEEKLY_SCHEDULE:   
				{		
					char i;
					char tv_count;
					len = 1;
					tv_count = (wp_data->application_data_len - 2) / 7;
					
					
				  for(i = 0;i < tv_count;i++)
					{
						bacapp_decode_time_value(&wp_data->application_data[len],&time_value);						
						write_Time_Value(object_index,wp_data->array_index - 1,i,time_value);	
						len += 7;						
					}	
			
          status = true;
				}
				break;
				case PROP_OBJECT_NAME:	 
				if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                object_index =
                    Schedule_Instance_To_Index(wp_data->object_instance);

					write_bacnet_name_to_buf(SCHEDULE,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;
				case PROP_DESCRIPTION:
								if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                object_index =
                    Schedule_Instance_To_Index(wp_data->object_instance);

					write_bacnet_description_to_buf(SCHEDULE,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;		
						
        case PROP_OUT_OF_SERVICE:
				if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                object_index =
                    Schedule_Instance_To_Index(wp_data->object_instance);

					write_bacent_AM_to_buf(SCHEDULE,object_index,value.type.Boolean);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }				
				
            break; 
						
      	case PROP_OBJECT_IDENTIFIER:         
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
        
        case PROP_RELIABILITY:
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}

#endif