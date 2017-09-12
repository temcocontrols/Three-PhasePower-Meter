
/* Calendar Objects customize -- writen by chesea*/

#include <stdbool.h>
#include <stdint.h>
#include "bacnet.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"

#include "calendar.h"

#if BAC_CALENDAR
/*
reliable have following property  

-present_value
-description
-Date_list
-local_date

// to be added more

*/

//BACnetCalendarEntry ::= CHOICE {
//date [0] Date,
//dateRange [1] BACnetDateRange,
//weekNDay [2] BACnetWeekNDay
//}

//BACnetDateRange ::= SEQUENCE {
//StartDate Date,
//endDate Date
//}

//BACnetWeekNDay ::= OCTET STRING (SIZE (3))
//-- first octet month (1..14) 1 =January
//-- 13 = odd months
//-- 14 = even months
//-- X'FF' = any month
//-- second octet weekOfMonth where: 1 = days numbered 1-7
//-- 2 = days numbered 8-14
//-- 3 = days numbered 15-21
//-- 4 = days numbered 22-28
//-- 5 = days numbered 29-31
//-- 6 = last 7 days of this month
//-- X'FF' = any week of this month
//-- third octet dayOfWeek (1..7) where 1 = Monday
//-- 7 = Sunday
//-- X'FF' = any day of week

//Date ::= [APPLICATION 10] OCTET STRING (SIZE(4)) -- see 20.2.12
//-- first octet year minus 1900 X'FF' = unspecified
//-- second octet month (1.. 14) 1 = January
//-- 13 = odd months
//-- 14 = even months
//-- X'FF' = unspecified
//-- third octet day of month (1..32), 32 = last day of month
//-- X'FF' = unspecified
//-- fourth octet day of week (1..7) 1 = Monday
//-- 7 = Sunday
//-- X'FF' = unspecified


unsigned char CALENDARS;

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Calendar_Valid_Instance(uint32_t object_instance)
{
    if(object_instance < MAX_CALENDARS)
        return true;
}

/* we simply have 0-n object instances. */
unsigned Calendar_Count(void)
{	
		return CALENDARS;
}

/* we simply have 0-n object instances. */
uint32_t Calendar_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances. */
unsigned Calendar_Instance_To_Index(
    uint32_t object_instance)
{
		return object_instance;
}




/* return apdu length, or -1 on error */
/* assumption - object has already exists */
// read
int Calendar_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;   /* return value */
//    BACNET_BIT_STRING  bit_string;
    BACNET_CHARACTER_STRING  char_string;
    unsigned object_index;
		char i;
		int len = 0;

    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = 
                encode_application_object_id(&apdu[0], OBJECT_CALENDAR,
                object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different.
               Note that Object-Name must be unique in this device */
        case PROP_OBJECT_NAME:
						characterstring_init_ansi(&char_string, get_label(CALENDAR,object_instance));
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
						characterstring_init_ansi(&char_string,get_description(CALENDAR,object_instance));
						apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_CALENDAR);
            break;
        case PROP_PRESENT_VALUE:
            object_index = Calendar_Instance_To_Index(object_instance);
            apdu_len =
                encode_application_unsigned(&apdu[0], Get_bacnet_value_from_buf(CALENDAR,0,object_index)/*AI_Present_Value[object_index]*/);
            break;       
        case PROP_DATE_LIST:		
            object_index = Calendar_Instance_To_Index(object_instance);
						for (i = 0; i < Get_CALENDAR_count(object_index); i++) 
						{
								apdu_len +=
										encode_context_date(&apdu[apdu_len],0,
										Get_Calendar_Date(object_index,i));
						}
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
bool Calendar_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    int far len = 0;
    BACNET_APPLICATION_DATA_VALUE far value;
	
   	BACNET_DATE  far date;
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
		
		object_index = Calendar_Instance_To_Index(wp_data->object_instance);
		
    switch ((int) wp_data->object_property) {
        case PROP_PRESENT_VALUE:
			if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                object_index =
                    Calendar_Instance_To_Index(wp_data->object_instance);

				wirte_bacnet_value_to_buf(CALENDAR,wp_data->priority,object_index,value.type.Boolean);

                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }

            break;
				 case PROP_DATE_LIST:		
				 {
						char i;
						char tv_count;
						len = 0;
						tv_count = wp_data->application_data_len / 5;				 
            object_index = Calendar_Instance_To_Index(wp_data->object_instance);
					
					 for(i = 0;i < tv_count;i++)
						{
							decode_date(&wp_data->application_data[len + 1],&date);						
							write_annual_date(object_index,date);	
							len += 5;						
						}	
					status = true;
					break;
					}	
				case PROP_OBJECT_NAME:	 
				if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                object_index =
                    Calendar_Instance_To_Index(wp_data->object_instance);

					write_bacnet_name_to_buf(CALENDAR,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;
				case PROP_DESCRIPTION:
								if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                object_index =
                    Calendar_Instance_To_Index(wp_data->object_instance);

					write_bacnet_description_to_buf(CALENDAR,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;		
						
        case PROP_OUT_OF_SERVICE:
				if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                object_index =
                    Calendar_Instance_To_Index(wp_data->object_instance);

					write_bacent_AM_to_buf(CALENDAR,object_index,value.type.Boolean);
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