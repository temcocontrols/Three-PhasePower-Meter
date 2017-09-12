#include "stdint.h"
#include "types.h"
#include "define.h"
#include "usart.h"
#include "rs485.h"
#include "bacnet.h" 
#include "24cxx.h"
#include "modbus.h"
//#include "inputs.h"
#include "define.h" 
#include "registerlist.h"
#include "store.h" 
#include "controls.h"  
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "controls.h"

//#ifdef CO2_SENSOR
//	uint8_t panelname[21] = "CO2_NET";
//#elif defined PRESSURE_SENSOR	
//	uint8_t panelname[21] = "Pressure";
//#elif defined HUM_SENSOR	
//	uint8_t panelname[21] = "Humdity";
//#else
//	uint8_t panelname[21] = "CO2_NET";
//#endif 

	uint8_t panelname[21];
	u32 Instance = 0x0c;
	u8  Station_NUM= DEFAULT_STATION_NUMBER;
//void bacnet_inital(void)
//{
//	uint8 i;
//	for(i = 0;i < MAX_VARS; i++) 
//	{	
//		memcpy(var[i].description,"test1",6); 
//		memcpy(var[i].label,"test2",6); 
//	}
//	for(i = 0;i < MAX_AOS; i++) 
//	{
//		memcpy(outputs[i].description,"test3",6); 
//		memcpy(outputs[i].label,"test4",6); 
//	}
//            
//}
 
void switch_to_modbus(void)
{
//	printf()
 
//	modbus.protocal = MODBUS;
// 	write_eeprom(EEP_MODBUS_COM_CONFIG, MODBUS);
//	
	if(modbus.baudrate  == BAUDRATE_19200)
		uart1_init(19200);
	else if(modbus.baudrate  == BAUDRATE_9600)
		uart1_init(9600);
	else if(modbus.baudrate  == BAUDRATE_38400)
		uart1_init(38400);
	else if(modbus.baudrate  == BAUDRATE_57600)
		uart1_init(57600);
	else if(modbus.baudrate  == BAUDRATE_115200)
		uart1_init(115200); 
}

uint16_t send_count;
//u16 far Test[50];

uint8_t RS485_Get_Baudrate(void)
{
 if(modbus.baudrate == BAUDRATE_9600)
  return 5;
 else if(modbus.baudrate == BAUDRATE_19200)
  return 6;
 else if(modbus.baudrate == BAUDRATE_38400)
  return 7;
 else if(modbus.baudrate == BAUDRATE_57600)
  return 8;
 else if(modbus.baudrate == BAUDRATE_115200)
  return 9;
 else 
  return 6;// default is 19200
}

//----------------------------------------------------------
void Get_AVS(void)
{
//	//bacnet_AV.reg.avs_num = 50;
//	bacnet_AV.reg.address = Modbus.address;
//	//	bacnet_AV.reg.product_model = Modbus.product_model;
//	bacnet_AV.reg.hardRev = Modbus.hardRev;
//	bacnet_AV.reg.firwareRev = SW_REV;
//	bacnet_AV.reg.tcp_type = Modbus.tcp_type;
//	memcpy(bacnet_AV.reg.ip_addr,Modbus.ip_addr,4);
//	memcpy(bacnet_AV.reg.mac_addr,Modbus.mac_addr,6);
//	memcpy(bacnet_AV.reg.subnet,Modbus.subnet,4);
//	memcpy(bacnet_AV.reg.getway,Modbus.getway,4);
//	bacnet_AV.reg.tcp_port = Modbus.tcp_port;
//	//	bacnet_AV.reg.mini_type = Modbus.mini_type;
//	memcpy(bacnet_AV.reg.com_config,Modbus.com_config,3);
//	bacnet_AV.reg.com_baudrate[0] = uart0_baudrate;
//	bacnet_AV.reg.com_baudrate[1] = uart1_baudrate;
//	bacnet_AV.reg.com_baudrate[2] = uart2_baudrate;
//	//	memcpy(bacnet_AV.reg.start_adc,Modbus.start_adc,11);
//	bacnet_AV.reg.network_number = Modbus.network_number;
//	bacnet_AV.reg.panel_number = Station_NUM;
//	
}
//modbus.input[0]
//----------------------------------------------
float Get_bacnet_value_from_buf(uint8_t type,uint8_t priority,uint8_t i)
{	
	float ftemp;
	if(i == 0) return 1;    //start from var1.
	else i -= 1;
	switch(type)
	{  
		case AV: 
			
			if(i < MAX_AVS)
			{ 
				switch (i)
				{
					case 0://serial number low byte
						var[i].value =((uint16)modbus.serial_Num[1]<<8)|modbus.serial_Num[0];
						break;
					case 1://serial number high byte
						var[i].value=((uint16)modbus.serial_Num[3]<<8)|modbus.serial_Num[2];
						break;
					case 2://software version
						var[i].value=SOFTREV;
						break;
					case 3: //id address 
						var[i].value = modbus.address;
						break; 
					case 4: //product_model  
						var[i].value = PRODUCT_ID;
						break;
					case 5: //Instance
						var[i].value = Instance;
						break;
					case 6: //Station
						var[i].value = Station_NUM;
						break;
					case 7: //baud rate
						var[i].value = modbus.baud;
						break;
					case 8: //update
						var[i].value =  modbus.update;
						break; 
					case 9: //protocol
						var[i].value =  modbus.protocal;
						break; 
 
					default: var[i].value =0;break;
				}
				
				return var[i].value;
			}
			break;
		case AI:
			if(i < MAX_INS)
			{ 
				ftemp = (float)inputs[i].value/1000;
				return ftemp;
			}
						
			 
		break;
		case AO: 
			if(i < MAX_OUTS)
			{
				ftemp = outputs[i].control;
				return ftemp ; 
			}
		break;
			
		case BO:
			  
		break;
			
		default:
			break;
				
	}	
	return 1;
}
//------------------------------------------------------------
void wirte_bacnet_value_to_buf(uint8_t type,uint8_t priority,uint8_t i,float value)
{

//		uint16 StartAdd;
		if(i == 0) return;    //start from var1.
		else i -= 1;
		switch(type)
		{
			case AV:  
				{
					
					switch (i)
					{
						case 3: //id address 
							var[i].value = value;
							modbus.address	= value; 
						    AT24CXX_WriteOneByte((u16)EEP_ADDRESS, value); 
							break;  
						case 5:  //instance 
							var[i].value = value;
							Instance = value;
							AT24CXX_WriteOneByte( EEP_INSTANCE_LOWORD     ,(uint8)Instance); 
							AT24CXX_WriteOneByte((EEP_INSTANCE_LOWORD + 1),(uint8)(Instance >> 8)); 
							AT24CXX_WriteOneByte((EEP_INSTANCE_LOWORD + 2),(uint8)(Instance >> 16)); 
							AT24CXX_WriteOneByte((EEP_INSTANCE_LOWORD + 3),(uint8)(Instance >> 24)); 
							break;
						case 6: //StaNum 
							var[i].value = value;
							Station_NUM=  value; 
							AT24CXX_WriteOneByte(EEP_STATION_NUMBER,value);  
							break;
						case 7: //baud rate
//							StartAdd = MODBUS_BAUDRATE;
							var[i].value = value;
							modbus.baud = value;
							switch(modbus.baud)
							{
								case 0:
									modbus.baudrate = BAUDRATE_9600;
									uart1_init(BAUDRATE_9600);
									AT24CXX_WriteOneByte(EEP_BAUDRATE, value);					
									SERIAL_RECEIVE_TIMEOUT = 6;
								break ;
								case 1:
									modbus.baudrate = BAUDRATE_19200;
									uart1_init(BAUDRATE_19200);
									AT24CXX_WriteOneByte(EEP_BAUDRATE, value);	
									SERIAL_RECEIVE_TIMEOUT = 3;
								break;
								case 2:
									modbus.baudrate = BAUDRATE_38400;
									uart1_init(BAUDRATE_38400);
									AT24CXX_WriteOneByte(EEP_BAUDRATE, value);	
									SERIAL_RECEIVE_TIMEOUT = 2;
								break;
								case 3:
									modbus.baudrate = BAUDRATE_57600;
									uart1_init(BAUDRATE_57600);
									AT24CXX_WriteOneByte(EEP_BAUDRATE, value);	
									SERIAL_RECEIVE_TIMEOUT = 1;
								break;
								case 4:
									modbus.baudrate = BAUDRATE_115200;
									uart1_init(BAUDRATE_115200);
									AT24CXX_WriteOneByte(EEP_BAUDRATE, value);	
									SERIAL_RECEIVE_TIMEOUT = 1;		
								default:
								break ;				
							}
							modbus_init();
							break;
						case 8: //update
//							StartAdd = MODBUS_UPDATE_STATUS;
							var[i].value = value;
							if(value == 0x8f){ reset_to_factory();SoftReset();}
							break; 
						case 9: //protocol
//							StartAdd = MODBUS_PROTOCOL_TYPE;
							var[i].value = value;
							if((value == MODBUS)||(value== BAC_MSTP))
							{
								AT24CXX_WriteOneByte(EEP_MODBUS_COM_CONFIG,value);
								modbus.protocal = value;
							}
							break; 
						default:break;
					}   
//					 
				}
 
			 
			break;
			case AI:
				if(i < MAX_INS)
				{ 	
					 ;
				}

			break;
			case BO:
			 
			break;
			case AO:
				if(i < MAX_OUTS)
				{ 	
					outputs[i].control = value;
					write_page_en[OUT_TYPE] = 1;
				}
 			break;
	
			default:
			break;
		}			

}
//-------------------------------------------------
void write_bacnet_name_to_buf(uint8_t type,uint8_t priority,uint8_t i,char* str)
{
 
		if(i == 0) return;    //start from var1.
		else i -= 1;
		switch(type)
		{
			case AI: 
				if(i < MAX_INS)
				{
					memcpy(inputs[i].label,str,9);
					inputs[i].label[8] = 0;
					write_page_en[IN_TYPE] = 1;
				}
				break;
//			case BO:
//				memcpy(outputs[i].label,str,8);
//				break;
			case AO:
				if(i < MAX_OUTS)
				{
					memcpy(outputs[i].label,str,9); 
					outputs[i].label[8] = 0;
					write_page_en[OUT_TYPE] = 1;
				} 
				break;
			case AV:
				if(i < MAX_AVS) 
				{
					memcpy(var[i].label,str,9); 
					var[i].label[8] = 0;
					 write_page_en[VAR_TYPE] = 1;
				}
				break;
	
			default:
			break;
		} 
}
//---------------------------------------------------
void write_bacnet_unit_to_buf(uint8_t type,uint8_t priority,uint8_t i,uint8_t unit)
{
//	U8_T temp;
		if(i == 0) return;    //start from var1.
		else i -= 1;
		switch(type)
		{
			case AV:
			break ;
			case AI:
				if(i < MAX_INS)
				{
					if(unit == UNITS_DEGREES_FAHRENHEIT)
					{
						if(inputs[i].range == Y3K_40_150DegC)
							inputs[i].range = Y3K_40_300DegF;
						
						else if(inputs[i].range == R3K_40_150DegC)
							inputs[i].range = R3K_40_300DegF;
						
						else if(inputs[i].range == R10K_40_120DegC)
							inputs[i].range = R10K_40_250DegF;
						
						else if(inputs[i].range == KM10K_40_120DegC)	
							inputs[i].range = KM10K_40_250DegF;
						
						else if(inputs[i].range == A10K_50_110DegC) 
							inputs[i].range = A10K_60_200DegF;
						write_page_en[IN_TYPE] = 1;
					}
					else if(unit == UNITS_DEGREES_CELSIUS)
					{
						if(inputs[i].range == Y3K_40_300DegF)
							inputs[i].range = Y3K_40_150DegC;
						
						else if(inputs[i].range == R3K_40_300DegF)
							inputs[i].range = R3K_40_150DegC;
						
						else if(inputs[i].range == R10K_40_250DegF)
							inputs[i].range = R10K_40_120DegC;
						
						else if(inputs[i].range == KM10K_40_250DegF)	
							inputs[i].range = KM10K_40_120DegC;
						
						else if(inputs[i].range == A10K_60_200DegF) 
							inputs[i].range = A10K_50_110DegC;
						write_page_en[IN_TYPE] = 1;
					} 
				}
				  
			break ;
			case AO:
			break ;
			default:
			break;
		}	
}
//------------------------------------------------------------
char get_AM_Status(uint8_t type,uint8_t num)
{	
	if(num == 0) return 0;    //start from var1.
	else num -= 1;
	
	switch(type)
	{
		case AV: 
			if(num < MAX_AVS)
			{
				return 0; 
			}  
		break;
		case AI:
			 if(num < MAX_INS)
				return inputs[num].auto_manual;
		break;
		case AO: 
			 if(num < MAX_OUTS)
				return outputs[num].auto_manual; 

		break;
		case BO:
		   
		break;
		
		default:
		break;
	}
	return 0;
	
}
//------------------------------------------------------------
void write_bacent_AM_to_buf(uint8_t type,uint8_t i,uint8_t am)
{
	if(i == 0) return;    //start from var1.
	else i -= 1;
	switch(type)
	{
		case AV:
		break ;
		case AI:
			if(i < MAX_INS)
			{
				inputs[i].auto_manual = am;
				write_page_en[IN_TYPE] = 1;
			}
			  
		break ;
		case AO:
			if(i < MAX_INS)
			{
				outputs[i].auto_manual = am;
				write_page_en[OUT_TYPE] = 1;
			}
		break ;
		default:
		break;
	}	
}
//------------------------------------------------------------
void add_remote_panel_db(uint32_t device_id,uint8_t panel)
{				
}
//------------------------------------------------------------

char* get_label(uint8_t type,uint8_t num)
{
	if(num == 0) return "null";    //start from var1.
	else num -= 1;
	
	switch(type)
      {
         case AV: 
			if(num < MAX_AVS)
			{
				return (char *)var[num].label; 
			}  
            break;
         case AI:
			 if(num < MAX_INS)
				return (char *)inputs[num].label;
            break;
         case AO: 
			 if(num < MAX_OUTS)
				return (char *)outputs[num].label; 
	 
            break;
          case BO:
			  
//			 if(num < BOS)
//				return (char *)DO_name[num];
			 
            break;
         default:
         break;
      }
	  return "null";
}
char* get_description(uint8_t type,uint8_t num)
{
	if(num == 0) return "null";    //start from var1.
	else num -= 1;
	
	switch(type)
      {
         case AV: 
			if(num < MAX_AVS) 
				return (char *)var[num].description;   
            break;
         case AI: 
			if(num < MAX_INS)
				return (char *)inputs[num].description;
            break;
         case AO: 
			 if(num < MAX_OUTS)
				return (char *)outputs[num].description;  
		 
            break;
          case BO: 
            break;
         default:
         break;
      }
	  return "null";
}

char get_range(uint8_t type,uint8_t num)
{ 
	if(num == 0) return UNITS_NO_UNITS;    //start from var1.
	else num -= 1;
	switch(type)
	{
		case AV: 
		 
		case AI: 
			if(num < MAX_INS)
			{
				if( (inputs[num].range == Y3K_40_150DegC)	||  \
					(inputs[num].range == R3K_40_150DegC)	||  \
					(inputs[num].range == R10K_40_120DegC)	||  \
					(inputs[num].range == KM10K_40_120DegC)	|| 	\
					(inputs[num].range == A10K_50_110DegC))
					return UNITS_DEGREES_CELSIUS;
				else if( (inputs[num].range == Y3K_40_300DegF)	||  \
					(inputs[num].range == R3K_40_300DegF)		||  \
					(inputs[num].range == R10K_40_250DegF)		||  \
					(inputs[num].range == KM10K_40_250DegF)		|| 	\
					(inputs[num].range == A10K_60_200DegF))
					return UNITS_DEGREES_FAHRENHEIT; 
			}
		break;
		
		case AO:  
		break;
		
		case BO: 
		break;
		
		default:
		break;
	}
					//UNITS_DEGREES_CELSIUS
					//UNITS_DEGREES_FAHRENHEIT
	return 		UNITS_NO_UNITS ; 
}


void Set_Object_Name(char * name)	
{
	u8 temp = strlen(name);
	if(temp > 20) temp = 20;
	memcpy(panelname,name,temp + 1);   
 	AT24CXX_Write(EEP_TSTAT_NAME1, panelname,temp+ 1); 
}
void write_bacnet_description_to_buf(uint8_t type, uint8_t priority, uint8_t i, char* str)
{
	
		if(i == 0) return ;    //start from var1.
		else i -= 1;
	
		switch(type)
		{ 
			case AO:
				if(i < MAX_OUTS)
				{
					memcpy(outputs[i].description,str,20); 
					write_page_en[OUT_TYPE] = 1;
				} 
				break;
			case AI:
				if(i < MAX_INS)
				{
					memcpy(inputs[i].description,str,20); 
					write_page_en[IN_TYPE] = 1;
				} 
				break;
			case AV:
				if(i < MAX_AVS) 
				{
					memcpy(var[i].description,str,20); 
					var[i].description[20] = 0;
					write_page_en[VAR_TYPE] = 1; 
				}
				break;
	
			default:
			break;
		} 
}	

char* Get_Object_Name(void)
{
	return (char*)panelname;
} 
#if BAC_SCHEDULE 
BACNET_TIME_VALUE* Get_Time_Value(uint8_t object_index,uint8_t day,uint8_t i) {return 0;}
uint8_t Get_TV_count(uint8_t object_index,uint8_t day) {return 0;}
BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * Get_Object_Property_References(uint8_t i) {return 0;} 
void write_Time_Value(uint8_t index,uint8_t day,uint8_t i,BACNET_TIME_VALUE time_value) {;}
#endif

#if BAC_CALENDAR
 
uint8_t Get_CALENDAR_count(uint8_t object_index) {return 0;}
BACNET_DATE* Get_Calendar_Date(uint8_t object_index,uint8_t i) {return 0;} 
void write_annual_date(uint8_t index,BACNET_DATE date){;}
	
#endif
 

	
void uart_send_string(U8_T *p, U16_T length,U8_T port) 
{
	 memcpy(uart_send, p, length);
	 USART_SendDataString(length);
}

u8 	UART_Get_SendCount(void)
{
	return 1;
}

void Set_TXEN(u8 dir)
{
	if(dir)
		TXEN = 1;
	else
		TXEN = 0;
}
	
void Inital_Bacnet_Server(void)
{
	uint32 ltemp; 
	Set_Object_Name((char *)panelname); 
	Device_Init();
	
	ltemp = 0;
	ltemp |= AT24CXX_ReadOneByte(EEP_INSTANCE_LOWORD);
	ltemp |= (uint32)AT24CXX_ReadOneByte(EEP_INSTANCE_LOWORD+1)<<8;
	ltemp |= (uint32)AT24CXX_ReadOneByte(EEP_INSTANCE_LOWORD+2)<<16;
	ltemp |= (uint32)AT24CXX_ReadOneByte(EEP_INSTANCE_LOWORD+3)<<24;
	if(ltemp == 0xffffffff)
		Instance = ((uint16)modbus.serial_Num[1]<<8)|modbus.serial_Num[0];
	else
		Instance = ltemp;
	Station_NUM = AT24CXX_ReadOneByte(EEP_STATION_NUMBER);
	if(Station_NUM == 0xff)  Station_NUM =  DEFAULT_STATION_NUMBER;
	Device_Set_Object_Instance_Number(Instance);  
	address_init();
	bip_set_broadcast_addr(0xffffffff); 
 
	AIS = MAX_INS + 1;
	AOS = MAX_OUTS + 1;
	BOS = 0;
	AVS = MAX_AVS + 1;
  
}
uint8_t  PDUBuffer[MAX_APDU];
void vMSTP_TASK(void *pvParameters )
{
	uint16_t pdu_len = 0; 
	BACNET_ADDRESS  src;
//	static u16 protocal_timer = SWITCH_TIMER;
//	modbus.protocal_timer_enable = 0;
//	bacnet_inital();	
	Inital_Bacnet_Server();
	dlmstp_init(NULL);
	Recievebuf_Initialize(0); 
	delay_ms(100);
	
	for (;;)
    { 
		
		if(modbus.protocal == BAC_MSTP)
		{
			pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0,	modbus.protocal);
			if(pdu_len) 
			{
				npdu_handler(&src, &PDUBuffer[0], pdu_len, BAC_MSTP);	
			} 
						
		} 
		vTaskDelay(5 / portTICK_RATE_MS);
	} 	
}

	
