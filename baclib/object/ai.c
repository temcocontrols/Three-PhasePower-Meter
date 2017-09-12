/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

/* Analog Input Objects customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacnet.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"

#include "ai.h"

#if BAC_COMMON

/* Analog Input = Photocell */

//uint16_t far AI_Present_Value[MAX_AIS];
uint8_t  AIS;
//extern char text_string[20];

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Analog_Input_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_AIS)
        return true;

    return false;
}

/* we simply have 0-n object instances. */
unsigned Analog_Input_Count(
    void)
{
	
		return AIS;
}

/* we simply have 0-n object instances. */
uint32_t Analog_Input_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances. */
unsigned Analog_Input_Instance_To_Index(
    uint32_t object_instance)
{
//	unsigned index = MAX_AIS;
//
//    if (object_instance < MAX_AIS)
//        index = object_instance;
//
//    return index;
	return object_instance;
}

//extern  
//char *Analog_Input_Name(
//    uint32_t object_instance)
//{
////    static char text_string[5] = "AI-0";        /* okay for single thread */

//    if (object_instance < MAX_AIS) {
//       text_string[3] = '0' + (uint8_t) object_instance;
//		
//	memcpy(text_string,get_label(AI,object_instance),9);
//	//memcpy(text_string,inputs[object_instance].label,9);

//        return text_string;
//    }

//    return NULL;
//}

/* return apdu length, or -1 on error */
/* assumption - object has already exists */
// read
int Analog_Input_Encode_Property_APDU(
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

//    (void) array_index;
    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = 
                encode_application_object_id(&apdu[0], OBJECT_ANALOG_INPUT,
                object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different.
               Note that Object-Name must be unique in this device */
        case PROP_OBJECT_NAME:
						characterstring_init_ansi(&char_string, get_label(AI,object_instance));
            apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
						characterstring_init_ansi(&char_string,get_description(AI,object_instance));
						apdu_len = encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_ANALOG_INPUT);
            break;
        case PROP_PRESENT_VALUE:
            object_index = Analog_Input_Instance_To_Index(object_instance);
            apdu_len =
                encode_application_real(&apdu[0], Get_bacnet_value_from_buf(AI,0,object_index)/*AI_Present_Value[object_index]*/);
            break;
        case PROP_STATUS_FLAGS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            object_index =
                Analog_Input_Instance_To_Index( object_instance);
            apdu_len = encode_application_boolean(&apdu[0], get_AM_Status(AI,object_instance));
            break;
				
            apdu_len = encode_application_boolean(&apdu[0], false);
            break;
        case PROP_UNITS:			
						apdu_len = encode_application_enumerated(&apdu[0], get_range(AI,object_instance));
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
bool Analog_Input_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    unsigned int object_index = 0;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    /* decode the some of the request */
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
    object_index = Analog_Input_Instance_To_Index(wp_data->object_instance);

    switch ((int) wp_data->object_property) {
        case PROP_PRESENT_VALUE:
			if (value.tag == BACNET_APPLICATION_TAG_REAL) {
                object_index =
                    Analog_Input_Instance_To_Index(wp_data->object_instance);
				
          //      AI_Present_Value[object_index] = value.type.Real; 
				//Set_value(AI,object_index,value.type.Real);
				wirte_bacnet_value_to_buf(AI,wp_data->priority,object_index,value.type.Real);

                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }

            break;
				// add it by chelsea
				case PROP_UNITS:
				if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                object_index =
                    Analog_Input_Instance_To_Index(wp_data->object_instance);
				
					write_bacnet_unit_to_buf(AI,wp_data->priority,object_index,value.type.Enumerated);

                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }	
				
				break;
				case PROP_OBJECT_NAME:	 
				if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                object_index =
                    Analog_Input_Instance_To_Index(wp_data->object_instance);

					write_bacnet_name_to_buf(AI,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;
				case PROP_DESCRIPTION:
								if (value.tag == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                object_index =
                    Analog_Input_Instance_To_Index(wp_data->object_instance);

					write_bacnet_description_to_buf(AI,wp_data->priority,object_index,value.type.Character_String.value);
                status = true;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }						
				break;		
						
        case PROP_OUT_OF_SERVICE:
				if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                object_index =
                    Analog_Input_Instance_To_Index(wp_data->object_instance);

					write_bacent_AM_to_buf(AI,object_index,value.type.Boolean);
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