//#include "store.h"
//#include "modbus.h"
////#include "ud_str.h"
//#include "controls.h"
//#include "bacnet.h"
//#include "stmflash.h"
//#include "delay.h"
//#include "registerlist.h"
//#include "24cxx.h"

#include "bacnet.h" 
#include "dlmstp.h"
#include "store.h" 
#include "controls.h"
#include "stmflash.h"
#include "FreeRTOS.h"
#include "task.h"
#include "registerlist.h"
#include "24cxx.h"
#define PAGE_LENTH		MAX_AVS * sizeof(Str_variable_point)

uint8_t write_page_en[MAX_TYPE];
Str_variable_point var[MAX_AVS];
 
 
void Flash_Write_Mass(void)
{ 
	uint16_t	len = 0 ;
//	uint16_t    loop1;	 
	uint8_t     *tempbuf;//[PAGE_LENTH]; 
	tempbuf = (uint8_t *)pvPortMalloc(PAGE_LENTH);
	if(tempbuf == NULL) return;
	if(write_page_en[OUT_TYPE] == 1)
	{
		write_page_en[OUT_TYPE] = 0 ;	
		STMFLASH_Unlock();  //½âËø	
		STMFLASH_ErasePage(OUT_PAGE_FLAG);				
//		for(loop1 = 0;loop1 < MAX_OUTS;loop1++)
//		{
//			memcpy(&tempbuf[sizeof(Str_out_point) * loop1],&outputs[loop1],sizeof(Str_out_point));					
//		}
//		len = sizeof(Str_out_point) *MAX_OUTS ;
//		iap_write_appbin(OUT_PAGE,(uint8_t*)tempbuf, len); 
		
		len = MAX_OUTS * sizeof(Str_out_point) ; 
		memcpy(tempbuf,(void*)&outputs[0].description[0],len); 
		iap_write_appbin(OUT_PAGE,(uint8_t*)tempbuf, len); 
		
		STMFLASH_WriteHalfWord(OUT_PAGE_FLAG, 10000) ;
		STMFLASH_Lock();	
 	} 
	
	if(write_page_en[IN_TYPE] == 1)
	{
		write_page_en[IN_TYPE] = 0 ;
		STMFLASH_Unlock();	
		STMFLASH_ErasePage(IN_PAGE_FLAG);
//		for(loop1 = 0;loop1 < MAX_INS;loop1++)
//		{
//			memcpy(&tempbuf[sizeof(Str_in_point) * loop1],&inputs[loop1],sizeof(Str_in_point));					
//		}
//		len = sizeof(Str_in_point)*MAX_INS ;
		len = MAX_INS * sizeof(Str_in_point) ; 
		memcpy(tempbuf,(void*)&inputs[0].description[0],len);  
		iap_write_appbin(IN_PAGE,(uint8_t*)tempbuf, len); 
		
		STMFLASH_WriteHalfWord(IN_PAGE_FLAG, 10000) ;
		STMFLASH_Lock();	
	} 
	
	if(write_page_en[VAR_TYPE] == 1)
	{  
			
		STMFLASH_Unlock();
		STMFLASH_ErasePage(AV_PAGE_FLAG);
		STMFLASH_ErasePage(AV_PAGE_FLAG + 2048);
		 
		len = MAX_AVS * sizeof(Str_variable_point) ; 
		memcpy(tempbuf,(void*)&var[0].description[0],len); 
		iap_write_appbin(AV_PAGE,(uint8_t*)tempbuf, len); 
		
		STMFLASH_WriteHalfWord(AV_PAGE_FLAG, 10000) ;	
		STMFLASH_Lock();
		write_page_en[VAR_TYPE] = 0 ; 
	}				
	vPortFree( tempbuf );	 
}
 
const uint8 Var_label[MAX_AVS][9] = {
	
	"SN_L",   //0
	"SN_H",   //1
	"SW_Ver", //2
	"Address",//3
	"Model",  //4 
	"Instance",//5					 
	"Station",//6
	"BaudRate",//7
	"Update",  //8
	"Protocol",//9
 
 
};
const uint8 Var_Description[MAX_AVS][21] = {
	
	"SerialNumberLowByte",   	//0
	"SerialNumberHighByte",   	//1
	"SoftWare Version", 		//2
	"ID Address",				//3
	"Product Model",			//4
	"Instance",					//5					 
	"Station number",			//6
	"Uart BaudRate",			//7
	"Update", 					//8
	"Protocol",					//9
	 
};

const uint8 Outputs_label[MAX_DO][9] = {
 	"Output1",
	"Output2",
	"Output3",
	"Output4" 
};
const uint8 Outputs_Description[MAX_DO][21] = {
 	
 	"Relay output1",
	"Relay output2",
	"Relay output3",
	"Relay output4"
};

const uint8 Inputs_label[MAX_INS][9] = {
 	"Input1",
	"Input2",
	"Input3",
	"Input4", 
};
const uint8 Inputs_Description[MAX_INS][21] = {
 	
 	"Analog Input1",
	"Analog Input2",
	"Analog Input3",
	"Analog Input4", 
};

 
void mass_flash_init(void)
{
	u16 temp = 0 ;
	u16 loop , j ;
	u16 len = 0;
	
	uint8_t  *tempbuf; 
	tempbuf = (uint8_t *)pvPortMalloc(PAGE_LENTH);
	if(tempbuf == NULL) return;
// 	u16 temp2 = 0 ;
//	u8 label_buf[21] ;
	for(j = 0; j < MAX_TYPE; j++)
		write_page_en[j] = 0;
 
 	temp = STMFLASH_ReadHalfWord(OUT_PAGE_FLAG);
//	printf("temp=%x, %x\n\r", temp2, temp);
	if(temp == 0xffff)
	{
		STMFLASH_Unlock();
		STMFLASH_ErasePage(OUT_PAGE_FLAG);
		for(loop=0; loop<MAX_OUTS; loop++ )
		{
			memcpy(outputs[loop].description,Outputs_Description[loop],21);  
			memcpy(outputs[loop].label,Outputs_label[loop],9);  		
			outputs[loop].value = 0; 
			outputs[loop].auto_manual = 1 ;			//manual mode
			outputs[loop].digital_analog = 0 ;		//digital
			outputs[loop].switch_status = 3 ;
			outputs[loop].control = 0 ;				//off
			outputs[loop].read_remote = 0 ;
			outputs[loop].decom = 0 ;
			outputs[loop].range = OFF_ON ;				
			outputs[loop].sub_id = 0 ;
			outputs[loop].sub_product = 0 ;
			outputs[loop].pwm_period = 0 ;
		}
		len = MAX_OUTS * sizeof(Str_out_point) ;
//		if(len > PAGE_LENTH)
//		{
//			memcpy(tempbuf,(void *)&outputs[0].description[0],PAGE_LENTH); 
//			memcpy(tempbuf,(void *)(&outputs[0].description[0] + PAGE_LENTH),len - PAGE_LENTH);
//		}
//		else
		memcpy(tempbuf,(void *)&outputs[0].description[0],len);  
		iap_write_appbin(OUT_PAGE,(uint8_t*)tempbuf, len);	 
		STMFLASH_WriteHalfWord(OUT_PAGE_FLAG, 10000) ;
		STMFLASH_Lock();	
	}
	else
	{
		len = MAX_OUTS * sizeof(Str_out_point) ;
		STMFLASH_MUL_Read(OUT_PAGE,(void *)&outputs[0].description[0], len );	
	}
	
	temp = STMFLASH_ReadHalfWord(AV_PAGE_FLAG);  
	if(temp == 0xffff)
	{
		STMFLASH_Unlock();
		STMFLASH_ErasePage(AV_PAGE_FLAG);
		STMFLASH_ErasePage(AV_PAGE_FLAG + 2048);
		for(loop=0; loop<MAX_AVS; loop++ )
		{
			memcpy(var[loop].description,Var_Description[loop],21);  
			memcpy(var[loop].label,Var_label[loop],9); 
			var[loop].value = 0; 
			var[loop].auto_manual = 0 ;
			var[loop].digital_analog = 0 ;
			var[loop].control = 0 ;
			var[loop].unused = 0 ;
			var[loop].range = 0 ;
			var[loop].range = 0 ;
		}
		len = MAX_AVS * sizeof(Str_variable_point) ;
		memcpy(tempbuf,(void*)&var[0].description[0],len); 
		iap_write_appbin(AV_PAGE,(uint8_t*)tempbuf, len); 
		STMFLASH_WriteHalfWord(AV_PAGE_FLAG, 10000) ;	
		STMFLASH_Lock();
	}
	else
	{
		len = MAX_AVS * sizeof(Str_variable_point) ;
		STMFLASH_MUL_Read(AV_PAGE,(void *)&var[0].description[0], len );
	}
	
	temp = STMFLASH_ReadHalfWord(IN_PAGE_FLAG);
	if(temp == 0xffff)
	{
		STMFLASH_ErasePage(IN_PAGE_FLAG);
		for(loop=0; loop<MAX_INS; loop++ )
		{
			 
			memcpy(inputs[loop].description, Inputs_Description[loop], 21);
			memcpy(inputs[loop].label, Inputs_label[loop], 9); 
			inputs[loop].value = 0; 
			inputs[loop].filter = 5 ;
			inputs[loop].decom = 0 ;
			inputs[loop].sub_id = 0 ;
			inputs[loop].sub_product = 0 ;
			inputs[loop].control = 0 ;
			inputs[loop].auto_manual = 0 ;			//auto mode
			inputs[loop].digital_analog = 1 ;		//analog
			inputs[loop].calibration_sign = 0 ;
			inputs[loop].sub_number = 0 ;
			inputs[loop].calibration_hi = 0;
			inputs[loop].calibration_lo = 0;
			inputs[loop].range = R10K_40_120DegC; 
		}
		len = MAX_INS * sizeof(Str_in_point) ;
		memcpy(tempbuf,(void*)&inputs[0], len);		
		iap_write_appbin(IN_PAGE,(uint8_t*)tempbuf, len);	
		STMFLASH_WriteHalfWord(IN_PAGE_FLAG, 10000) ;
	}
	else
	{
		len = MAX_INS * sizeof(Str_in_point) ;
		STMFLASH_MUL_Read(IN_PAGE,(void *)&inputs[0].description[0], len ); 
	} 
	
	vPortFree( tempbuf );
}
 
void reset_to_factory(void)
{
	u8 i;
//intial variable
	STMFLASH_Unlock();	
	STMFLASH_ErasePage(AV_PAGE_FLAG);
	STMFLASH_WriteHalfWord(AV_PAGE_FLAG, 0xffff) ;
	
	STMFLASH_ErasePage(IN_PAGE_FLAG);
	STMFLASH_WriteHalfWord(IN_PAGE_FLAG, 0xffff) ;
	
	STMFLASH_ErasePage(OUT_PAGE_FLAG);
	STMFLASH_WriteHalfWord(OUT_PAGE_FLAG, 0xffff) ;
	STMFLASH_Lock();
	
	AT24CXX_WriteOneByte(EEP_STATION_NUMBER,DEFAULT_STATION_NUMBER);
	sprintf((char *)panelname,"%s", (char *)"PowerMeter");
				 
	for(i=0;i<20;i++)			 
	{
		AT24CXX_WriteOneByte((EEP_TSTAT_NAME1 + i),panelname[i]); 
	}
	
}
