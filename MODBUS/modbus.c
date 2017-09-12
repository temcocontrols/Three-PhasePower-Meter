#include "stdlib.h"
#include "modbus_crc.h"
#include "usart.h"
#include "24cxx.h" 
#include "delay.h"
#include "define.h"
#include "modbus.h"
#include "registerlist.h"
#include "led.h"
#include "tcp_modbus.h"
#include "tapdev.h"
#include "ade7753.h"
#include "portmacro.h"
#include "controls.h"
#include "store.h"  
#include "stmflash.h"
#include "fifo.h"
STR_UART uart;
static u8 randval = 0;
//u8 i2c_test[10];
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
u8 uart_send[USART_SEND_LEN];
vu8 transmit_finished = 0; 
vu8 revce_count = 0;
vu8 rece_size = 0;
vu8 serial_receive_timeout_count;
u8 SERIAL_RECEIVE_TIMEOUT;
u8 dealwithTag;
STR_MODBUS modbus;
u8 DealwithTag;
u16 sendbyte_num = 0;
u16 uart_num = 0;
extern FIFO_BUFFER Receive_Buffer0;
//u8 app2boot_type = 0xff;

void USART1_IRQHandler(void)                	//串口1中断服务程序
{		
	static u16 send_count = 0;
	unsigned portBASE_TYPE uxSavedInterruptStatus;
	uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)	//接收中断
	{
		if(modbus.protocal == MODBUS)		
		{
			if(revce_count < USART_REC_LEN)
				USART_RX_BUF[revce_count++] = USART_ReceiveData(USART1);		//读取接收到的数据
			else
				serial_restart();
			
			if(revce_count == 1)
			{
				// This starts a timer that will reset communication.  If you do not
				// receive the full packet, it insures that the next receive will be fresh.
				// The timeout is roughly 7.5ms.  (3 ticks of the hearbeat)
				rece_size = 250;
				serial_receive_timeout_count = SERIAL_RECEIVE_TIMEOUT;
			}
			else if(revce_count == 3 )
			{
				if(USART_RX_BUF[1] == CHECKONLINE)
				rece_size = 6;
			}
			else if(revce_count == 4)
			{
				//check if it is a scan command
				if((((vu16)(USART_RX_BUF[2] << 8) + USART_RX_BUF[3]) == 0x0a) && (USART_RX_BUF[1] == WRITE_VARIABLES))
				{
					rece_size = DATABUFLEN_SCAN;
					serial_receive_timeout_count = SERIAL_RECEIVE_TIMEOUT;	
				}
			}
			else if(revce_count == 7)
			{
				if((USART_RX_BUF[1] == READ_VARIABLES) || (USART_RX_BUF[1] == WRITE_VARIABLES))
				{
					rece_size = 8;
					//dealwithTag = 1;
				}
				else if(USART_RX_BUF[1] == MULTIPLE_WRITE)
				{
					rece_size = USART_RX_BUF[6] + 9;
					serial_receive_timeout_count = USART_RX_BUF[6] + 8;
				}
				else
				{
					rece_size = 250;
				}
			}
			else if(USART_RX_BUF[0] == 0x55 && USART_RX_BUF[1] == 0xff && USART_RX_BUF[2] == 0x01 && USART_RX_BUF[5] == 0x00 && USART_RX_BUF[6] == 0x00)
			{
				
			}
			else if(revce_count == rece_size)		
			{
				// full packet received - turn off serial timeout
				serial_receive_timeout_count = 0;
				dealwithTag = 2;		// making this number big to increase delay
				uart.rx1_flag = 2;
			}
		}
		else if(modbus.protocal == BAC_MSTP )
		{
				u8 receive_buf ;
				if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
				{
						receive_buf =  USART_ReceiveData(USART1); 
						FIFO_Put(&Receive_Buffer0, receive_buf);
				}
		}
	}
	else if(USART_GetITStatus(USART1, USART_IT_TXE) == SET)
	{ 
		{
			 USART_SendData(USART1, uart_send[send_count++]);  
			 Timer_Silence_Reset();
			 if( send_count >= sendbyte_num)
			 {
						while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
						USART_ClearFlag(USART1, USART_FLAG_TC); 
						USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
						send_count = 0 ; 
//						if(modbus.protocal == MODBUS)
//						{
//							reply_done = receive_delay_time;
//							if(reply_done == 0) serial_restart();
//						}
//						else
							serial_restart(); 	
			 }  
		}
	}
	 portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedInterruptStatus);
}

void serial_restart(void)
{
	TXEN = RECEIVE;
	revce_count = 0;
	dealwithTag = 0;
} 

//it is ready to send data by serial port . 
static void initSend_COM(void)
{
	TXEN = SEND;
}

void send_byte(u8 ch, u8 crc)
{
	USART_ClearFlag(USART1, USART_FLAG_TC); 
	USART_SendData(USART1, ch);
	if(crc)
	{
		crc16_byte(ch);
	}
}

void USART_SendDataString(u16 num)
{
	sendbyte_num = num;
	uart_num = 0;
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
	uart.tx1_flag = 2;
}

void modbus_init(void)
{
	serial_restart();
	SERIAL_RECEIVE_TIMEOUT = 3;
	serial_receive_timeout_count = SERIAL_RECEIVE_TIMEOUT;
}
void write_user_data_by_block(U16_T StartAdd,U8_T HeadLen,U8_T *pData) 
{
	U8_T far i;

	if(StartAdd  >= MODBUS_OUTPUT_BLOCK_FIRST && StartAdd  <= MODBUS_OUTPUT_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_OUTPUT_BLOCK_FIRST) % ((sizeof(Str_out_point) + 1) / 2) == 0)
		{
			i = (StartAdd - MODBUS_OUTPUT_BLOCK_FIRST) / ((sizeof(Str_out_point) + 1) / 2);
			memcpy(&outputs[i],&pData[HeadLen + 7],sizeof(Str_out_point));
		}
	}
	else if(StartAdd  >= MODBUS_INPUT_BLOCK_FIRST && StartAdd  <= MODBUS_INPUT_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1) / 2);
			memcpy(&inputs[i],&pData[HeadLen + 7],sizeof(Str_in_point)); 	
		}
	}
	else if(StartAdd  >= MODBUS_VAR_BLOCK_FIRST && StartAdd  <= MODBUS_VAR_BLOCK_LAST)
	{
		if((StartAdd - MODBUS_VAR_BLOCK_FIRST) % ((sizeof(Str_variable_point) + 1	) / 2) == 0)
		{
			i = (StartAdd - MODBUS_VAR_BLOCK_FIRST) / ((sizeof(Str_variable_point) + 1) / 2);
			memcpy(&var[i],&pData[HeadLen + 7],sizeof(Str_variable_point)); 	
		}
	}
//	else if(StartAdd  >= MODBUS_PRG_BLOCK_FIRST && StartAdd  <= MODBUS_PRG_BLOCK_LAST)
//	{
//		if((StartAdd - MODBUS_PRG_BLOCK_FIRST) % ((sizeof(Str_program_point) + 1	) / 2) == 0)
//		{
//			i = (StartAdd - MODBUS_PRG_BLOCK_FIRST) / ((sizeof(Str_program_point) + 1) / 2);
//			memcpy(&programs[i],&pData[HeadLen + 7],sizeof(Str_program_point)); 	
//		}
//	}
//	else if(StartAdd  >= MODBUS_WR_BLOCK_FIRST && StartAdd  <= MODBUS_WR_BLOCK_LAST)
//	{
//		if((StartAdd - MODBUS_WR_BLOCK_FIRST) % ((sizeof(Str_weekly_routine_point) + 1	) / 2) == 0)
//		{
//			i = (StartAdd - MODBUS_VAR_BLOCK_FIRST) / ((sizeof(Str_weekly_routine_point) + 1) / 2);
//			memcpy(&weekly_routines[i],&pData[HeadLen + 7],sizeof(Str_weekly_routine_point)); 	
//		}
//	}
//	else if(StartAdd  >= MODBUS_AR_BLOCK_FIRST && StartAdd  <= MODBUS_AR_BLOCK_LAST)
//	{
//		if((StartAdd - MODBUS_AR_BLOCK_FIRST) % ((sizeof(Str_annual_routine_point) + 1	) / 2) == 0)
//		{
//			i = (StartAdd - MODBUS_AR_BLOCK_FIRST) / ((sizeof(Str_annual_routine_point) + 1) / 2);
//			memcpy(&annual_routines[i],&pData[HeadLen + 7],sizeof(Str_annual_routine_point)); 	
//		}
//	}
//	else if(StartAdd  >= MODBUS_WR_TIME_FIRST && StartAdd  <= MODBUS_WR_TIME_LAST)
//	{
//		if((StartAdd - MODBUS_WR_TIME_FIRST) % (sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK / 2) == 0)
//		{		
//			i = (StartAdd - MODBUS_WR_TIME_FIRST) / (sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK / 2);
//			memcpy(&wr_times[i],&pData[HeadLen + 7],(sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK)); 	
//		}
//	}
//	else if(StartAdd  >= MODBUS_AR_TIME_FIRST && StartAdd  <= MODBUS_AR_TIME_LAST)
//	{
//		if((StartAdd - MODBUS_AR_TIME_FIRST) % AR_DATES_SIZE == 0)
//		{
//			i = ((StartAdd - MODBUS_AR_TIME_FIRST) / AR_DATES_SIZE);
//			memcpy(&ar_dates[i],&pData[HeadLen + 7],AR_DATES_SIZE); 	
//		}
//	}	
//	else  if(StartAdd  >= MODBUS_CONTROLLER_BLOCK_FIRST && StartAdd  <= MODBUS_CONTROLLER_BLOCK_LAST)
//	{ 
//		if((StartAdd - MODBUS_CONTROLLER_BLOCK_FIRST) % ((sizeof(Str_controller_point) + 1	) / 2) == 0)
//		{
//			i = (StartAdd - MODBUS_CONTROLLER_BLOCK_FIRST) / ((sizeof(Str_controller_point) + 1) / 2);
//			memcpy(&controllers[i],&pData[HeadLen + 7],sizeof(Str_controller_point));
//			PID[i].Set_Flag = 1;			
//		}
//	}
}



U16_T read_user_data_by_block(U16_T addr) 
{
	U8_T far index,item;	
	U16_T far *block;			
	if( addr >= MODBUS_OUTPUT_BLOCK_FIRST && addr <= MODBUS_OUTPUT_BLOCK_LAST )
	{
		index = (addr - MODBUS_OUTPUT_BLOCK_FIRST) / ( (sizeof(Str_out_point) + 1) / 2);
		block = (U16_T *)&outputs[index];
		item = (addr - MODBUS_OUTPUT_BLOCK_FIRST) % ((sizeof(Str_out_point) + 1) / 2);
	}
	else if( addr >= MODBUS_INPUT_BLOCK_FIRST && addr <= MODBUS_INPUT_BLOCK_LAST )
	{
		index = (addr - MODBUS_INPUT_BLOCK_FIRST) / ((sizeof(Str_in_point) + 1) / 2);
		block = (U16_T *) &inputs[index];
		item = (addr - MODBUS_INPUT_BLOCK_FIRST) % ((sizeof(Str_in_point) + 1) / 2);
	}
	else if( addr >= MODBUS_VAR_BLOCK_FIRST && addr <= MODBUS_VAR_BLOCK_LAST )
	{
		index = (addr - MODBUS_VAR_BLOCK_FIRST) / ((sizeof(Str_variable_point) + 1) / 2);
		block = (U16_T *)&inputs[index];
		item = (addr - MODBUS_VAR_BLOCK_FIRST) % ((sizeof(Str_variable_point) + 1) / 2);
	}
//	else if( addr >= MODBUS_PRG_BLOCK_FIRST && addr <= MODBUS_PRG_BLOCK_LAST )
//	{

//		index = (addr - MODBUS_PRG_BLOCK_FIRST) / ((sizeof(Str_program_point) + 1) / 2);
//		block = &programs[index];
//		item = (addr - MODBUS_PRG_BLOCK_FIRST) % ((sizeof(Str_program_point) + 1) / 2);
//	}
//	else if( addr >= MODBUS_CODE_BLOCK_FIRST && addr <= MODBUS_CODE_BLOCK_LAST )
//	{	
//		index = (addr - MODBUS_CODE_BLOCK_FIRST) / 100;
//		block = &prg_code[index / (CODE_ELEMENT * MAX_CODE / 200)][CODE_ELEMENT * MAX_CODE % 200];
//		item = (addr - MODBUS_CODE_BLOCK_FIRST) % 100;
//	}
//	else if( addr >= MODBUS_WR_BLOCK_FIRST && addr <= MODBUS_WR_BLOCK_LAST )
//	{
//		index = (addr - MODBUS_WR_BLOCK_FIRST) / ((sizeof(Str_weekly_routine_point) + 1) / 2);
//		block = &weekly_routines[index];
//		item = (addr - MODBUS_WR_BLOCK_FIRST) % ((sizeof(Str_weekly_routine_point) + 1) / 2);
//	}
//	else if( addr >= MODBUS_WR_TIME_FIRST && addr <= MODBUS_WR_TIME_LAST )
//	{
//		index = (addr - MODBUS_WR_TIME_FIRST) / ((sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK + 1) / 2);
//		block = &wr_times[index];
//		item = (addr - MODBUS_WR_TIME_FIRST) % ((sizeof(Wr_one_day) * MAX_SCHEDULES_PER_WEEK + 1) / 2);
//	}
//	else if( addr >= MODBUS_AR_BLOCK_FIRST && addr <= MODBUS_AR_BLOCK_LAST )
//	{
//		index = (addr - MODBUS_AR_BLOCK_FIRST) / ((sizeof(Str_annual_routine_point) + 1) / 2);
//		block = &annual_routines[index];
//		item = (addr - MODBUS_AR_BLOCK_FIRST) % ((sizeof(Str_annual_routine_point) + 1) / 2);
//		
//	}
//	else if( addr >= MODBUS_AR_TIME_FIRST && addr <= MODBUS_AR_TIME_LAST )
//	{
//	
//		index = (addr - MODBUS_AR_TIME_FIRST) / (AR_DATES_SIZE / 2);
//		block = &ar_dates[index];
//		item = (addr - MODBUS_AR_TIME_FIRST) % (AR_DATES_SIZE / 2);
//	}
//	else if( addr >= MODBUS_CONTROLLER_BLOCK_FIRST && addr <= MODBUS_CONTROLLER_BLOCK_LAST )
//	{
//		index = (addr - MODBUS_CONTROLLER_BLOCK_FIRST) / ((sizeof(Str_controller_point) + 1) / 2);
//		block = (U16_T *)&controllers[index];
//		item = (addr - MODBUS_CONTROLLER_BLOCK_FIRST) % ((sizeof(Str_controller_point) + 1) / 2);
//	 
//	}
	return block[item];
	
} 

void internalDeal(u8 type,  u8 *pData)
{

	u8 address_temp;
//	u8 div_buf;
	u8 i;
	u8 HeadLen;
	u16 StartAdd;
	
	if(type == 0)
	{
		HeadLen = 0;	
	}
	else
	{
		HeadLen = 6;
	}
	
	StartAdd = (u16)(pData[HeadLen + 2] << 8) + pData[HeadLen + 3];
	 if (pData[HeadLen + 1] == MULTIPLE_WRITE) //multi_write
	{
		if(StartAdd == MODBUS_MAC_ADDRESS_1)
		{
			if((modbus.mac_enable == 1) && (pData[HeadLen + 6] == 12))
			{
				modbus.mac_addr[0] = pData[HeadLen + 8];
				modbus.mac_addr[1] = pData[HeadLen + 10];
				modbus.mac_addr[2] = pData[HeadLen + 12];
				modbus.mac_addr[3] = pData[HeadLen + 14];
				modbus.mac_addr[4] = pData[HeadLen + 16];
				modbus.mac_addr[5] = pData[HeadLen + 18];
				for(i=0; i<6; i++)
				{
					AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_1+i, modbus.mac_addr[i]);
				}
//				while(tapdev_init())	//初始化ENC28J60错误
//				{								   
//				//	printf("ENC28J60 Init Error!\r\n");
//				delay_ms(50);
//				};
				IP_Change = 1 ;
				modbus.mac_enable = 0 ;
			}
		}
		else if(StartAdd  >= MODBUS_USER_BLOCK_FIRST && StartAdd  <= MODBUS_USER_BLOCK_LAST)
		{  
			write_user_data_by_block(StartAdd,HeadLen,pData);
		}
		else if(StartAdd  >= TSTAT_NAME1 && StartAdd <= TSTAT_NAME8)
		{
			if(pData[HeadLen +6] <= 20)
			{
				for(i=0;i<pData[HeadLen + 6];i++)			//	(data_buffer[6]*2)
				{
					AT24CXX_WriteOneByte((EEP_TSTAT_NAME1 + i),pData[HeadLen + 7+i]);
					panelname[i] = pData[HeadLen + 7+i];
				}
			}
		}
	}
	else if(pData[HeadLen + 1] == WRITE_VARIABLES)
	{
		if(StartAdd  <= 99 )
		{								
			// If writing to Serial number Low word, set the Serial number Low flag
			if(StartAdd <= MODBUS_SERIALNUMBER_LOWORD+1)
			{
				AT24CXX_WriteOneByte((u16)EEP_SERIALNUMBER_LOWORD, pData[HeadLen+5]);
				AT24CXX_WriteOneByte((u16)EEP_SERIALNUMBER_LOWORD+1, pData[HeadLen+4]);
				modbus.serial_Num[0] = pData[HeadLen+5] ;
				modbus.serial_Num[1] = pData[HeadLen+4] ;
				modbus.SNWriteflag |= 0x01;
				AT24CXX_WriteOneByte((u16)EEP_SERIALNUMBER_WRITE_FLAG, modbus.SNWriteflag);
				
				if(modbus.SNWriteflag)
				{
					modbus.update = 0;
					AT24CXX_WriteOneByte((u16)EEP_UPDATE_STATUS, 0);
				}
			}
			// If writing to Serial number High word, set the Serial number High flag
			else if(StartAdd <= MODBUS_SERIALNUMBER_HIWORD+1)
			{
				
				AT24CXX_WriteOneByte((u16)EEP_SERIALNUMBER_HIWORD, pData[HeadLen+5]);
				AT24CXX_WriteOneByte((u16)EEP_SERIALNUMBER_HIWORD+1, pData[HeadLen+4]);
				modbus.serial_Num[2] = pData[HeadLen+5] ;
				modbus.serial_Num[3] = pData[HeadLen+4] ;
				modbus.SNWriteflag |= 0x02;
				AT24CXX_WriteOneByte((u16)EEP_SERIALNUMBER_WRITE_FLAG, modbus.SNWriteflag);
				
				if(modbus.SNWriteflag)
				{
					modbus.update = 0;
					AT24CXX_WriteOneByte((u16)EEP_UPDATE_STATUS, 0);
				}
			}
//			else if(USART_RX_BUF[3] <= MODBUS_VERSION_NUMBER_LO+1)
//			{	
//				AT24CXX_WriteOneByte((u16)EEP_VERSION_NUMBER_LO, USART_RX_BUF[5]);
//				AT24CXX_WriteOneByte((u16)EEP_VERSION_NUMBER_LO+1, USART_RX_BUF[4]);
//				modbus.software = (USART_RX_BUF[5]<<8) ;
//				modbus.software |= USART_RX_BUF[4] ;				
//			}
			else if(StartAdd == MODBUS_ADDRESS )
			{
				AT24CXX_WriteOneByte((u16)EEP_ADDRESS, pData[HeadLen+5]);
				modbus.address	= pData[HeadLen+5] ;
			}
			else if(StartAdd == MODBUS_PRODUCT_MODEL )
			{
				AT24CXX_WriteOneByte((u16)EEP_PRODUCT_MODEL, pData[HeadLen+5]);
				modbus.product	= pData[HeadLen+5] ;
				modbus.SNWriteflag |= 0x08;
				AT24CXX_WriteOneByte((u16)EEP_SERIALNUMBER_WRITE_FLAG, modbus.SNWriteflag);
			}
			else if(StartAdd == MODBUS_HARDWARE_REV )
			{
				AT24CXX_WriteOneByte((u16)EEP_HARDWARE_REV, pData[HeadLen+5]);
				modbus.hardware_Rev	= pData[HeadLen+5] ;
				modbus.SNWriteflag |= 0x04;
				AT24CXX_WriteOneByte((u16)EEP_SERIALNUMBER_WRITE_FLAG, modbus.SNWriteflag);
			}
			else if(StartAdd == MODBUS_BAUDRATE )			// july 21 Ron
			{			
				modbus.baud = pData[HeadLen+5];
				switch(modbus.baud)
				{
					case 0:
						modbus.baudrate = BAUDRATE_9600;
						uart1_init(BAUDRATE_9600);
						AT24CXX_WriteOneByte(EEP_BAUDRATE, pData[HeadLen+5]);					
						SERIAL_RECEIVE_TIMEOUT = 6;
					break ;
					case 1:
						modbus.baudrate = BAUDRATE_19200;
						uart1_init(BAUDRATE_19200);
						AT24CXX_WriteOneByte(EEP_BAUDRATE, pData[HeadLen+5]);	
						SERIAL_RECEIVE_TIMEOUT = 3;
					break;
					case 2:
						modbus.baudrate = BAUDRATE_38400;
						uart1_init(BAUDRATE_38400);
						AT24CXX_WriteOneByte(EEP_BAUDRATE, pData[HeadLen+5]);	
						SERIAL_RECEIVE_TIMEOUT = 2;
					break;
					case 3:
						modbus.baudrate = BAUDRATE_57600;
						uart1_init(BAUDRATE_57600);
						AT24CXX_WriteOneByte(EEP_BAUDRATE, pData[HeadLen+5]);	
						SERIAL_RECEIVE_TIMEOUT = 1;
					break;
					case 4:
						modbus.baudrate = BAUDRATE_115200;
						uart1_init(BAUDRATE_115200);
						AT24CXX_WriteOneByte(EEP_BAUDRATE, pData[HeadLen+5]);	
						SERIAL_RECEIVE_TIMEOUT = 1;		
					default:
					break ;				
				}
				modbus_init();
			}
			else if(StartAdd == MODBUS_UPDATE_STATUS )			// july 21 Ron
			{
				//AT24CXX_WriteOneByte(EEP_UPDATE_STATUS, pData[HeadLen+5]);
				modbus.update = pData[HeadLen+5];
			}
			else if(StartAdd == MODBUS_PROTOCOL_TYPE )			// july 21 Ron
			{
				if((pData[HeadLen+5] == MODBUS)||(pData[HeadLen+5]== BAC_MSTP))
				{
					AT24CXX_WriteOneByte(EEP_MODBUS_COM_CONFIG, pData[HeadLen+5]);
					modbus.protocal = pData[HeadLen+5];
				}
			}
			else if(StartAdd == MODBUS_INSTANCE_LOWORD)
			{
				Instance &= 0xffff0000;
				Instance |= ((uint16)pData[HeadLen+4] << 8 | pData[HeadLen+5]); 
				AT24CXX_WriteOneByte(EEP_INSTANCE_LOWORD,pData[HeadLen+5]);
				AT24CXX_WriteOneByte(EEP_INSTANCE_LOWORD + 1,pData[HeadLen+4]); 
			}
			else if(StartAdd == MODBUS_INSTANCE_HIWORD)
			{
				Instance &= 0x0000ffff;
				Instance |= (uint32)((uint16)pData[HeadLen+4] << 8 | pData[HeadLen+5]) << 16; 
				AT24CXX_WriteOneByte(EEP_INSTANCE_HIWORD,pData[HeadLen+5]);
				AT24CXX_WriteOneByte(EEP_INSTANCE_HIWORD + 1,pData[HeadLen+4]); 
			} 
			else if(StartAdd == MODBUS_STATION_NUMBER)
			{ 
				Station_NUM=  pData[HeadLen+5]; 
				AT24CXX_WriteOneByte(EEP_STATION_NUMBER,pData[HeadLen+5]);  
			}
			else if(( StartAdd >= MODBUS_MAC_ADDRESS_1 )&&( StartAdd <= MODBUS_MAC_ADDRESS_6 ))
			{
					address_temp	= StartAdd - MODBUS_MAC_ADDRESS_1 ;
					modbus.mac_addr[address_temp] = pData[HeadLen+5] ;
					AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_1+address_temp, pData[HeadLen+5]);
			}
			else if(StartAdd == MODBUS_GHOST_IP_MODE )
			{
				modbus.ghost_ip_mode = pData[HeadLen+5] ;
			}
			else if(( StartAdd >= MODBUS_GHOST_IP_ADDRESS_1 )&&( StartAdd <= MODBUS_GHOST_IP_ADDRESS_4 ))
			{
					address_temp	= StartAdd - MODBUS_GHOST_IP_ADDRESS_1 ;
					modbus.ghost_ip_addr[address_temp] = pData[HeadLen+5] ;
			}
			else if((StartAdd >= MODBUS_GHOST_SUB_MASK_ADDRESS_1 )&&( StartAdd <= MODBUS_GHOST_SUB_MASK_ADDRESS_4 ))
			{
					address_temp	= StartAdd - MODBUS_GHOST_SUB_MASK_ADDRESS_1 ;
					modbus.ghost_mask_addr[address_temp] = pData[HeadLen+5] ;
			}
			else if(( StartAdd >= MODBUS_GHOST_GATEWAY_ADDRESS_1 )&&( StartAdd <= MODBUS_GHOST_GATEWAY_ADDRESS_4 ))
			{
					address_temp	= StartAdd - MODBUS_GHOST_GATEWAY_ADDRESS_1 ;
					modbus.ghost_gate_addr[address_temp] = pData[HeadLen+5] ;
			}
			else if(StartAdd == MODBUS_GHOST_TCP_SERVER )
			{
				modbus.ghost_tcp_server = pData[HeadLen+5] ;
			}
			else if(StartAdd == MODBUS_GHOST_LISTEN_PORT )
			{
				modbus.ghost_listen_port =  (pData[HeadLen+4]<<8) +pData[HeadLen+5] ;
			}
			else if(StartAdd == MODBUS_WRITE_GHOST_SYSTEM )
			{
				modbus.write_ghost_system = pData[HeadLen+5] ;
				if(modbus.write_ghost_system == 1)
				{
						modbus.ip_mode = modbus.ghost_ip_mode ;
						modbus.tcp_server = modbus.ghost_tcp_server ;
						modbus.listen_port = modbus.ghost_listen_port ;
						AT24CXX_WriteOneByte(EEP_IP_MODE, modbus.ip_mode);
						AT24CXX_WriteOneByte(EEP_TCP_SERVER, modbus.tcp_server);				
						AT24CXX_WriteOneByte(EEP_LISTEN_PORT_HI, modbus.listen_port>>8);
						AT24CXX_WriteOneByte(EEP_LISTEN_PORT_LO, modbus.listen_port &0xff);
						for(i=0; i<4; i++)
						{
							modbus.ip_addr[i] = modbus.ghost_ip_addr[i] ;
							modbus.mask_addr[i] = modbus.ghost_mask_addr[i] ;
							modbus.gate_addr[i] = modbus.ghost_gate_addr[i] ;
							
							AT24CXX_WriteOneByte(EEP_IP_ADDRESS_1+i, modbus.ip_addr[i]);
							AT24CXX_WriteOneByte(EEP_SUB_MASK_ADDRESS_1+i, modbus.mask_addr[i]);
							AT24CXX_WriteOneByte(EEP_GATEWAY_ADDRESS_1+i, modbus.gate_addr[i]);						
						}
//						for(i=0; i<5; i++)
//						{
//							AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_1+i, modbus.mac_addr[i]);
//						}
//						if(!tapdev_init()) 
//						{
//							printf("Init fail\n\r");
//						}
//							while(tapdev_init())	//初始化ENC28J60错误
//							{								   
//							//	printf("ENC28J60 Init Error!\r\n");
//							delay_ms(50);
//							};	
							IP_Change = 1; 
						modbus.write_ghost_system = 0 ;
				}
			}
			else if(StartAdd == MODBUS_MAC_ENABLE )
			{
				modbus.mac_enable = pData[HeadLen+5] ;	
			}
			
			else if(StartAdd == MODBUS_RESET)
			{ 				
				modbus.reset = pData[HeadLen+5] ;
				if(modbus.reset == 1)
				{
					for(i=0; i<255; i++)
						AT24CXX_WriteOneByte(i, 0xff);
				}				
				EEP_Dat_Init();
//				AT24CXX_WriteOneByte(16, 1);
				SoftReset();
			}
		}
		///////////////////////////
		//PHRASE A
		else if(StartAdd == MODBUS_PM0_VRMS_CAL_VALUE)
		{
			vrms_cal[PHASE_A].value = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
			vrms_cal[PHASE_A].adc = ade7753_regs[PHASE_A].ro_voltage_rms;
			vrms_cal[PHASE_A].k_slope.f = 1.0 * vrms_cal[PHASE_A].value / vrms_cal[PHASE_A].adc;
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_VALUE_BYTE_0, vrms_cal[PHASE_A].value >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_VALUE_BYTE_1, vrms_cal[PHASE_A].value & 0xff);
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_ADC_BYTE_0, vrms_cal[PHASE_A].adc >> 24);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_ADC_BYTE_1, vrms_cal[PHASE_A].adc >> 16);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_ADC_BYTE_2, vrms_cal[PHASE_A].adc >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_ADC_BYTE_3, vrms_cal[PHASE_A].adc & 0xff);
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_SLOPE_BYTE_0, vrms_cal[PHASE_A].k_slope.l >> 24);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_SLOPE_BYTE_1, vrms_cal[PHASE_A].k_slope.l >> 16);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_SLOPE_BYTE_2, vrms_cal[PHASE_A].k_slope.l >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL0_SLOPE_BYTE_3, vrms_cal[PHASE_A].k_slope.l & 0xff);
		}
		else if(StartAdd == MODBUS_PM0_IRMS_CAL_VALUE)
		{
			irms_cal[PHASE_A].value = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
			irms_cal[PHASE_A].adc = ade7753_regs[PHASE_A].ro_current_rms;
			irms_cal[PHASE_A].k_slope.f = 1.0 * irms_cal[PHASE_A].value / irms_cal[PHASE_A].adc;
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_VALUE_BYTE_0, irms_cal[PHASE_A].value >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_VALUE_BYTE_1, irms_cal[PHASE_A].value & 0xff);
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_ADC_BYTE_0, irms_cal[PHASE_A].adc >> 24);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_ADC_BYTE_1, irms_cal[PHASE_A].adc >> 16);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_ADC_BYTE_2, irms_cal[PHASE_A].adc >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_ADC_BYTE_3, irms_cal[PHASE_A].adc & 0xff);
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_SLOPE_BYTE_0, irms_cal[PHASE_A].k_slope.l >> 24);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_SLOPE_BYTE_1, irms_cal[PHASE_A].k_slope.l >> 16);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_SLOPE_BYTE_2, irms_cal[PHASE_A].k_slope.l >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL0_SLOPE_BYTE_3, irms_cal[PHASE_A].k_slope.l & 0xff);
		}
		else if(StartAdd == MODBUS_PM0_IRQ_ENABLE)
		{
			ade7753_regs[PHASE_A].rw_interrupt_enable.flags = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
		}
		else if(StartAdd == MODBUS_PM_TEST)
		{
			ade7753_test = pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM0_LINECYC)
		{
			ade7753_regs[PHASE_A].rw_line_cycle = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
// ENERGY CALIBRATION SECTION
		else if(StartAdd == MODBUS_PM0_CF_CAL_EN)
		{
			wh_calibration[PHASE_A].cf_calibration_enable = pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM0_CF_IMP_PER_KWH)
		{
			wh_calibration[PHASE_A].cf_imp_per_kwh = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM0_VT)
		{
			wh_calibration[PHASE_A].Vtest = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM0_IT)
		{
			wh_calibration[PHASE_A].Itest = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM0_LINECYC_IB)
		{
			wh_calibration[PHASE_A].LineCyc_ib = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		
		else if(StartAdd == MODBUS_PM0_OFFSET_CAL_PRO)
		{
			wh_calibration[PHASE_A].offset_calibration_pro = pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM0_OFFSET_CALI_EN)
		{
			wh_calibration[PHASE_A].offset_calibration_enable = pData[HeadLen+5];
		}

		else if(StartAdd == MODBUS_PM0_LINECYC_I_MIN)
		{
			wh_calibration[PHASE_A].LineCyc_imin = pData[HeadLen+5];
		}
		
//PHRASE B
		else if(StartAdd == MODBUS_PM1_VRMS_CAL_VALUE)
		{
			vrms_cal[PHASE_B].value = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
			vrms_cal[PHASE_B].adc = ade7753_regs[PHASE_B].ro_voltage_rms;
			vrms_cal[PHASE_B].k_slope.f = 1.0 * vrms_cal[PHASE_B].value / vrms_cal[PHASE_B].adc;
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_VALUE_BYTE_0, vrms_cal[PHASE_B].value >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_VALUE_BYTE_1, vrms_cal[PHASE_B].value & 0xff);
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_ADC_BYTE_0, vrms_cal[PHASE_B].adc >> 24);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_ADC_BYTE_1, vrms_cal[PHASE_B].adc >> 16);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_ADC_BYTE_2, vrms_cal[PHASE_B].adc >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_ADC_BYTE_3, vrms_cal[PHASE_B].adc & 0xff);
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_SLOPE_BYTE_0, vrms_cal[PHASE_B].k_slope.l >> 24);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_SLOPE_BYTE_1, vrms_cal[PHASE_B].k_slope.l >> 16);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_SLOPE_BYTE_2, vrms_cal[PHASE_B].k_slope.l >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL1_SLOPE_BYTE_3, vrms_cal[PHASE_B].k_slope.l & 0xff);
		}
		else if(StartAdd == MODBUS_PM1_IRMS_CAL_VALUE)
		{
			irms_cal[PHASE_B].value = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
			irms_cal[PHASE_B].adc = ade7753_regs[PHASE_B].ro_current_rms;
			irms_cal[PHASE_B].k_slope.f = 1.0 * irms_cal[PHASE_B].value / irms_cal[PHASE_B].adc;
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_VALUE_BYTE_0, irms_cal[PHASE_B].value >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_VALUE_BYTE_1, irms_cal[PHASE_B].value & 0xff);
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_ADC_BYTE_0, irms_cal[PHASE_B].adc >> 24);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_ADC_BYTE_1, irms_cal[PHASE_B].adc >> 16);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_ADC_BYTE_2, irms_cal[PHASE_B].adc >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_ADC_BYTE_3, irms_cal[PHASE_B].adc & 0xff);
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_SLOPE_BYTE_0, irms_cal[PHASE_B].k_slope.l >> 24);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_SLOPE_BYTE_1, irms_cal[PHASE_B].k_slope.l >> 16);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_SLOPE_BYTE_2, irms_cal[PHASE_B].k_slope.l >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL1_SLOPE_BYTE_3, irms_cal[PHASE_B].k_slope.l & 0xff);
		}
		else if(StartAdd == MODBUS_PM1_IRQ_ENABLE)
		{
			ade7753_regs[PHASE_B].rw_interrupt_enable.flags = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
		}
	 
		else if(StartAdd == MODBUS_PM1_LINECYC)
		{
			ade7753_regs[PHASE_B].rw_line_cycle = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
// ENERGY CALIBRATION SECTION
		else if(StartAdd == MODBUS_PM1_CF_CAL_EN)
		{
			wh_calibration[PHASE_B].cf_calibration_enable = pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM1_CF_IMP_PER_KWH)
		{
			wh_calibration[PHASE_B].cf_imp_per_kwh = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM1_VT)
		{
			wh_calibration[PHASE_B].Vtest = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM1_IT)
		{
			wh_calibration[PHASE_B].Itest = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM1_LINECYC_IB)
		{
			wh_calibration[PHASE_B].LineCyc_ib = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		
		else if(StartAdd == MODBUS_PM1_OFFSET_CAL_PRO)
		{
			wh_calibration[PHASE_B].offset_calibration_pro = pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM1_OFFSET_CALI_EN)
		{
			wh_calibration[PHASE_B].offset_calibration_enable = pData[HeadLen+5];
		}

		else if(StartAdd == MODBUS_PM1_LINECYC_I_MIN)
		{
			wh_calibration[PHASE_B].LineCyc_imin = pData[HeadLen+5];
		}





//PHRASE C		
		else if(StartAdd == MODBUS_PM2_VRMS_CAL_VALUE)
		{
			vrms_cal[PHASE_C].value = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
			vrms_cal[PHASE_C].adc = ade7753_regs[PHASE_C].ro_voltage_rms;
			vrms_cal[PHASE_C].k_slope.f = 1.0 * vrms_cal[PHASE_C].value / vrms_cal[PHASE_C].adc;
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_VALUE_BYTE_0, vrms_cal[PHASE_C].value >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_VALUE_BYTE_1, vrms_cal[PHASE_C].value & 0xff);
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_ADC_BYTE_0, vrms_cal[PHASE_C].adc >> 24);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_ADC_BYTE_1, vrms_cal[PHASE_C].adc >> 16);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_ADC_BYTE_2, vrms_cal[PHASE_C].adc >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_ADC_BYTE_3, vrms_cal[PHASE_C].adc & 0xff);
			
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_SLOPE_BYTE_0, vrms_cal[PHASE_C].k_slope.l >> 24);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_SLOPE_BYTE_1, vrms_cal[PHASE_C].k_slope.l >> 16);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_SLOPE_BYTE_2, vrms_cal[PHASE_C].k_slope.l >> 8);
			AT24CXX_WriteOneByte(EEP_VRMS_CAL2_SLOPE_BYTE_3, vrms_cal[PHASE_C].k_slope.l & 0xff);
		}
		else if(StartAdd == MODBUS_PM2_IRMS_CAL_VALUE)
		{
			irms_cal[PHASE_C].value = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
			irms_cal[PHASE_C].adc = ade7753_regs[PHASE_C].ro_current_rms;
			irms_cal[PHASE_C].k_slope.f = 1.0 * irms_cal[PHASE_C].value / irms_cal[PHASE_C].adc;
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_VALUE_BYTE_0, irms_cal[PHASE_C].value >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_VALUE_BYTE_1, irms_cal[PHASE_C].value & 0xff);
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_ADC_BYTE_0, irms_cal[PHASE_C].adc >> 24);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_ADC_BYTE_1, irms_cal[PHASE_C].adc >> 16);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_ADC_BYTE_2, irms_cal[PHASE_C].adc >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_ADC_BYTE_3, irms_cal[PHASE_C].adc & 0xff);
			
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_SLOPE_BYTE_0, irms_cal[PHASE_C].k_slope.l >> 24);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_SLOPE_BYTE_1, irms_cal[PHASE_C].k_slope.l >> 16);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_SLOPE_BYTE_2, irms_cal[PHASE_C].k_slope.l >> 8);
			AT24CXX_WriteOneByte(EEP_IRMS_CAL2_SLOPE_BYTE_3, irms_cal[PHASE_C].k_slope.l & 0xff);
		}
		else if(StartAdd == MODBUS_PM2_IRQ_ENABLE)
		{
			ade7753_regs[PHASE_C].rw_interrupt_enable.flags = (pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
		} 
		else if(StartAdd == MODBUS_PM2_LINECYC)
		{
			ade7753_regs[PHASE_C].rw_line_cycle = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
// ENERGY CALIBRATION SECTION
		else if(StartAdd == MODBUS_PM2_CF_CAL_EN)
		{
			wh_calibration[PHASE_C].cf_calibration_enable = pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM2_CF_IMP_PER_KWH)
		{
			wh_calibration[PHASE_C].cf_imp_per_kwh = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM2_VT)
		{
			wh_calibration[PHASE_C].Vtest = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM2_IT)
		{
			wh_calibration[PHASE_C].Itest = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM2_LINECYC_IB)
		{
			wh_calibration[PHASE_C].LineCyc_ib = (pData[HeadLen+4] << 8) | pData[HeadLen+5];
		}
		
		else if(StartAdd == MODBUS_PM2_OFFSET_CAL_PRO)
		{
			wh_calibration[PHASE_C].offset_calibration_pro = pData[HeadLen+5];
		}
		else if(StartAdd == MODBUS_PM2_OFFSET_CALI_EN)
		{
			wh_calibration[PHASE_C].offset_calibration_enable = pData[HeadLen+5];
		}
 
		else if(StartAdd == MODBUS_PM2_LINECYC_I_MIN)
		{
			wh_calibration[PHASE_C].LineCyc_imin = pData[HeadLen+5];
		}
		
		
		
		
//END		
		else if((StartAdd >= MODBUS_AI_FILTER0)&&(StartAdd <= MODBUS_AI_FILTER3))
		{
			u8 pos; 
			pos = StartAdd - MODBUS_AI_FILTER0; 
			inputs[pos].filter =pData[HeadLen+5];  
			write_page_en[IN_TYPE] = 1;
		}
		else if((StartAdd >= MODBUS_AI_RANGE0)&&(StartAdd <= MODBUS_AI_RANGE3))
		{
			u8 pos; 
			pos = StartAdd - MODBUS_AI_RANGE0; 
			inputs[pos].range = pData[HeadLen+5];
			inputs[pos].digital_analog = 1 ;
			write_page_en[IN_TYPE] = 1;
		}
		else if((StartAdd >= MODBUS_AI_AM0)&&(StartAdd <= MODBUS_AI_AM3))
		{
			u8 pos; 
			pos = StartAdd - MODBUS_AI_AM0; 
			inputs[pos].auto_manual = pData[HeadLen+5]; 
			write_page_en[IN_TYPE] = 1;
		}
		else if((StartAdd >= MODBUS_AI_CAL_SIGN0)&&(StartAdd <= MODBUS_AI_CAL_SIGN3))
		{
			u8 pos; 
			pos = StartAdd - MODBUS_AI_CAL_SIGN0; 
			inputs[pos].calibration_sign = pData[HeadLen+5]; 
			write_page_en[IN_TYPE] = 1;
		}
		else if((StartAdd >= MODBUS_AI_OFFSET0)&&(StartAdd <= MODBUS_AI_OFFSET3))
		{
			u8 pos; 
			pos = StartAdd - MODBUS_AI_OFFSET0; 
			inputs[pos].calibration_hi = pData[HeadLen+4];
			inputs[pos].calibration_lo = pData[HeadLen+5]; 
			write_page_en[IN_TYPE] = 1;
		}
		else if((StartAdd >= MODBUS_AO_VALUE0)&&(StartAdd <= MODBUS_AO_VALUE3))
		{
			u8 pos; 
			pos = StartAdd - MODBUS_AO_VALUE0;
			outputs[pos].control = ((uint16)pData[HeadLen + 4] << 8) | pData[HeadLen + 5];
			write_page_en[OUT_TYPE] = 1;
		}
		else if((StartAdd >= MODBUS_AO_AM0)&&(StartAdd <= MODBUS_AO_AM3))
		{
			u8 pos;  
			pos = StartAdd - MODBUS_AO_AM0; 
			outputs[pos].auto_manual = pData[HeadLen+5]; 
			write_page_en[OUT_TYPE] = 1;
		}
		else if((StartAdd >= MODBUS_AO_RANGE0)&&(StartAdd <= MODBUS_AO_RANGE3))
		{
			u8 pos;  
			pos = StartAdd - MODBUS_AO_RANGE0;  
			outputs[pos].range = pData[HeadLen+5]; 
			outputs[pos].digital_analog = 0;
			write_page_en[OUT_TYPE] = 1;
		}
	}
	
	if (modbus.update == 0x7F)
	{
		SoftReset();		
	}
	else if((modbus.update == 0x8e)||(modbus.update == 0x8f))
	{
		if(modbus.update == 0x8e)
		{
			modbus.SNWriteflag = 0x00;
			AT24CXX_WriteOneByte(EEP_SERIALNUMBER_WRITE_FLAG, 0);
		}  
		
		reset_to_factory();			 
 		SoftReset();
	}
}

//static void responseData(u16 start_address)
void responseCmd(u8 type, u8* pData)
{
	u8 i, temp1 = 0, temp2 = 0;
	u16 send_cout = 0;
	u8 sendbuf[300];
	u8 HeadLen = 0;
	u16  RegNum;
	u8 cmd;
	u16 StartAdd;
	if(type == 0)
	{
		HeadLen = 0;	
	}
	else
	{
		HeadLen = 6;
		for(i=0; i<6; i++)
		{
			sendbuf[i] = 0;	
		}
	}
	cmd = pData[HeadLen + 1]; 
	StartAdd = (u16)(pData[HeadLen + 2] << 8) + pData[HeadLen + 3];
	RegNum = (u8)pData[HeadLen + 5];
	
	if(cmd == WRITE_VARIABLES)
	{
		send_cout = HeadLen;
		if(type == 0)
		{
			for(i = 0; i < rece_size; i++)
			{
				sendbuf[send_cout++] = pData[i];
			}
			memcpy(uart_send, sendbuf, send_cout);
			USART_SendDataString(send_cout);		
		}
		else // TCP   dont have CRC 
		{
		//	SetTransactionId(6 + UIP_HEAD);
			sendbuf[0] = pData[0];//0;			//	TransID
			sendbuf[1] = pData[1];//TransID++;	
			sendbuf[2] = 0;			//	ProtoID
			sendbuf[3] = 0;
			sendbuf[4] = 0;	//	Len
			sendbuf[5] = 6 ;

			for (i = 0; i < 6; i++)
			{
				sendbuf[HeadLen + i] = pData[HeadLen + i];	
			}
			
			memcpy(tcp_server_sendbuf,sendbuf,6+ HeadLen);
			tcp_server_sendlen = 6 + HeadLen;
//			if(cSemaphoreTake( xSemaphore_tcp_send, ( portTickType ) 10 ) == pdTRUE)
//			{				
//				TCPIP_TcpSend(pHttpConn->TcpSocket, sendbuf, 6 + UIP_HEAD, TCPIP_SEND_NOT_FINAL); 
//				cSemaphoreGive( xSemaphore_tcp_send );
//			}
		}
	}
	else if(cmd == MULTIPLE_WRITE)
	{
		//send_cout = HeadLen ;
		if(type == 0)
		{		
			for(i = 0; i < 6; i++)
			{
			sendbuf[HeadLen+i] = pData[HeadLen+i] ;
			crc16_byte(sendbuf[HeadLen+i]);
			}
			sendbuf[HeadLen+i] = CRChi ;
			sendbuf[HeadLen+i+1] = CRClo ;
			memcpy(uart_send, sendbuf, 8);
			USART_SendDataString(8);
		}
		else
		{
			sendbuf[0] = pData[0] ;
			sendbuf[1] = pData[1] ;
			sendbuf[2] = 0 ;
			sendbuf[3] = 0 ;
			sendbuf[4] = 0; 
			sendbuf[5] =6;					
			for (i = 0;i < 6;i++)
			{
				sendbuf[HeadLen + i] = pData[HeadLen + i];	
			}
			memcpy(tcp_server_sendbuf,sendbuf,	6 + HeadLen);
			tcp_server_sendlen = 6 + HeadLen;
		}
	}
	else if(cmd == READ_VARIABLES)
	{
		u16 address;
		u16 address_temp;
//		u16 div_temp;
//		u16 address_buf;
//		u16 buf = 0;
		sendbuf[HeadLen] = pData[HeadLen];
		sendbuf[HeadLen + 1] = pData[HeadLen + 1];
		sendbuf[HeadLen + 2] = RegNum * 2;
		crc16_byte(sendbuf[HeadLen]);
		crc16_byte(sendbuf[HeadLen + 1]);
		crc16_byte(sendbuf[HeadLen + 2]);
		send_cout = HeadLen + 3 ;
		for(i = 0; i < RegNum; i++)
		{
			address = StartAdd + i;
			if(address <= MODBUS_SERIALNUMBER_HIWORD + 1)
			{
				temp1 = 0 ;
				temp2 = modbus.serial_Num[address] ;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_VERSION_NUMBER_LO)
			{
				temp1 = 0 ;
				temp2 =  (u8)(SOFTREV>>8) ;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);		
			}
			else if(address == MODBUS_VERSION_NUMBER_HI)
			{
				temp1 = 0 ;
				temp2 =  (u8)(SOFTREV) ;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);	
			}
			else if(address == MODBUS_ADDRESS)
			{
				temp1 = 0 ;
				temp2 =  modbus.address;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);		
			}
			else if(address == MODBUS_PRODUCT_MODEL)
			{
				temp1 = 0 ;
				temp2 =  modbus.product;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_HARDWARE_REV)
			{
				temp1 = 0 ;
				temp2 =  modbus.hardware_Rev;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);				
			}
			else if(address == MODBUS_BAUDRATE)
			{
				temp1 = 0 ;
				temp2 =  modbus.baud;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);				
			}
//			else if(address == MODBUS_BASE_ADDRESS)
//			{
//				temp1 = 0 ;
//				temp2 =  0;
//				uart_send[send_cout++] = temp1;
//				uart_send[send_cout++] = temp2;
//				crc16_byte(temp1);
//				crc16_byte(temp2);
//			}
			else if(address == MODBUS_UPDATE_STATUS)
			{
				temp1 = 0 ;
				temp2 =   modbus.update;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_SERINALNUMBER_WRITE_FLAG)
			{
				temp1 = 0 ;
				temp2 =  modbus.SNWriteflag;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PROTOCOL_TYPE)
			{
				temp1 = 0 ;
				temp2 =  modbus.protocal ;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			} 
			else if(address == MODBUS_INSTANCE_LOWORD)
			{   
				temp1 = Instance>>8 ;
				temp2 = Instance;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_INSTANCE_HIWORD)
			{   
				temp1 = Instance>>24;
				temp2 =  Instance>>16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_STATION_NUMBER)
			{   
				temp1 = 0 ;
				temp2 =  Station_NUM;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_MAC_ADDRESS_1)&&(address<= MODBUS_MAC_ADDRESS_6))
			{
				address_temp = address - MODBUS_MAC_ADDRESS_1;
				temp1 = 0;
				temp2 =  modbus.mac_addr[address_temp];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_IP_MODE)
			{
				temp1 = 0;
				temp2 = modbus.ip_mode;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_IP_ADDRESS_1)&&(address<= MODBUS_IP_ADDRESS_4))
			{
				address_temp = address - MODBUS_IP_ADDRESS_1;
				temp1 = 0;
				temp2 = modbus.ip_addr[address_temp];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_SUB_MASK_ADDRESS_1)&&(address<= MODBUS_SUB_MASK_ADDRESS_4))
			{
				address_temp = address - MODBUS_SUB_MASK_ADDRESS_1;
				temp1 = 0;
				temp2 = modbus.mask_addr[address_temp];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_GATEWAY_ADDRESS_1)&&(address<= MODBUS_GATEWAY_ADDRESS_4))
			{
				address_temp = address - MODBUS_GATEWAY_ADDRESS_1;
				temp1 = 0;
				temp2 = modbus.gate_addr[address_temp];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_TCP_SERVER)
			{
				temp1 = 0;
				temp2 =  modbus.tcp_server;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_LISTEN_PORT)
			{
				temp1 = (modbus.listen_port>>8)&0xff;
				temp2 =  modbus.listen_port &0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_GHOST_IP_MODE)
			{
				temp1 = 0;
				temp2 = modbus.ghost_ip_mode;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_GHOST_IP_ADDRESS_1)&&(address<= MODBUS_GHOST_IP_ADDRESS_4))
			{
				address_temp = address - MODBUS_GHOST_IP_ADDRESS_1;
				temp1 = 0;
				temp2 = modbus.ghost_ip_addr[address_temp];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_GHOST_SUB_MASK_ADDRESS_1)&&(address<= MODBUS_GHOST_SUB_MASK_ADDRESS_4))
			{
				address_temp = address - MODBUS_GHOST_SUB_MASK_ADDRESS_1;
				temp1 = 0;
				temp2 = modbus.ghost_mask_addr[address_temp];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_GHOST_GATEWAY_ADDRESS_1)&&(address<= MODBUS_GHOST_GATEWAY_ADDRESS_4))
			{
				address_temp = address - MODBUS_GHOST_GATEWAY_ADDRESS_1;
				temp1 = 0;
				temp2 = modbus.ghost_gate_addr[address_temp];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_GHOST_TCP_SERVER)
			{
				temp1 = 0 ;
				temp2 = modbus.ghost_tcp_server;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_GHOST_LISTEN_PORT)
			{
				temp1 = (modbus.ghost_listen_port>>8)&0xff;
				temp2 = modbus.ghost_listen_port &0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_WRITE_GHOST_SYSTEM)
				{
				temp1 = 0;
				temp2 = modbus.write_ghost_system;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_MAC_ENABLE)
			{				
				temp1 = 0;
				temp2 = modbus.mac_enable;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);	
			}
			else if(address == MODBUS_RESET)
			{ 				
				
				temp1 = 0;
				temp2 = modbus.reset;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
/********************************************************************************/
			 
			else if(address == MODBUS_PM0_MODE)
			{
				temp1 = ade7753_regs[PHASE_A].rw_ade7753_mode.modes >> 8;
				temp2 = ade7753_regs[PHASE_A].rw_ade7753_mode.modes & 0x00ff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_MODE)
			{
				temp1 = ade7753_regs[PHASE_B].rw_ade7753_mode.modes >> 8;
				temp2 = ade7753_regs[PHASE_B].rw_ade7753_mode.modes & 0x00ff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_MODE)
			{
				temp1 = ade7753_regs[PHASE_C].rw_ade7753_mode.modes >> 8;
				temp2 = ade7753_regs[PHASE_C].rw_ade7753_mode.modes & 0x00ff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_PM0_ONCHIP_TEMPERATURE)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_A].ro_temperature;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_ONCHIP_TEMPERATURE)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_B].ro_temperature;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_ONCHIP_TEMPERATURE)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_B].ro_temperature;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_PM0_PGA_GAIN)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_A].rw_pga_gain.pca_gain;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_PGA_GAIN)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_B].rw_pga_gain.pca_gain;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_PGA_GAIN)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_C].rw_pga_gain.pca_gain;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			 
			else if(address == MODBUS_PM0_VOLTAGE_FREQUENCY)
			{
				temp1 = ade7753_regs[PHASE_A].voltage_frequency >> 8;
				temp2 = ade7753_regs[PHASE_A].voltage_frequency & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VOLTAGE_FREQUENCY)
			{
				temp1 = ade7753_regs[PHASE_B].voltage_frequency >> 8;
				temp2 = ade7753_regs[PHASE_B].voltage_frequency & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VOLTAGE_FREQUENCY)
			{
				temp1 = ade7753_regs[PHASE_C].voltage_frequency >> 8;
				temp2 = ade7753_regs[PHASE_C].voltage_frequency & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_PM2_PERIOD)
			{
				temp1 = ade7753_regs[PHASE_C].ro_voltage_period >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_voltage_period & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_PM0_IRQ_ENABLE)
			{
				temp1 = ade7753_regs[PHASE_A].rw_interrupt_enable.flags >> 8;
				temp2 = ade7753_regs[PHASE_A].rw_interrupt_enable.flags & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRQ_ENABLE)
			{
				temp1 = ade7753_regs[PHASE_B].rw_interrupt_enable.flags >> 8;
				temp2 = ade7753_regs[PHASE_B].rw_interrupt_enable.flags & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRQ_ENABLE)
			{
				temp1 = ade7753_regs[PHASE_C].rw_interrupt_enable.flags >> 8;
				temp2 = ade7753_regs[PHASE_C].rw_interrupt_enable.flags & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IRQ_STATUS)
			{
				temp1 = ade7753_regs[PHASE_A].ro_interrupt_status.flags >> 8;
				temp2 = ade7753_regs[PHASE_A].ro_interrupt_status.flags & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRQ_STATUS)
			{
				temp1 = ade7753_regs[PHASE_B].ro_interrupt_status.flags >> 8;
				temp2 = ade7753_regs[PHASE_B].ro_interrupt_status.flags & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRQ_STATUS)
			{
				temp1 = ade7753_regs[PHASE_C].ro_interrupt_status.flags >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_interrupt_status.flags & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_VPEAK_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_A].ro_voltage_peak >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VPEAK_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_B].ro_voltage_peak >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VPEAK_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_C].ro_voltage_peak >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_PM0_VPEAK_LW)
			{
				temp1 = ade7753_regs[PHASE_A].ro_voltage_peak >> 8;
				temp2 = ade7753_regs[PHASE_A].ro_voltage_peak & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VPEAK_LW)
			{
				temp1 = ade7753_regs[PHASE_B].ro_voltage_peak >> 8;
				temp2 = ade7753_regs[PHASE_B].ro_voltage_peak & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VPEAK_LW)
			{
				temp1 = ade7753_regs[PHASE_C].ro_voltage_peak >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_voltage_peak & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_VRMS_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_A].ro_voltage_rms >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VRMS_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_B].ro_voltage_rms >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VRMS_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_C].ro_voltage_rms >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_VRMS_LW)
			{
				temp1 = ade7753_regs[PHASE_A].ro_voltage_rms >> 8;
				temp2 = ade7753_regs[PHASE_A].ro_voltage_rms & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VRMS_LW)
			{
				temp1 = ade7753_regs[PHASE_B].ro_voltage_rms >> 8;
				temp2 = ade7753_regs[PHASE_B].ro_voltage_rms & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VRMS_LW)
			{
				temp1 = ade7753_regs[PHASE_C].ro_voltage_rms >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_voltage_rms & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_PM0_IPEAK_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_A].ro_current_peak >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IPEAK_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_B].ro_current_peak >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IPEAK_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_C].ro_current_peak >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_PM0_IPEAK_LW)
			{
				temp1 = ade7753_regs[PHASE_A].ro_current_peak >> 8;
				temp2 = ade7753_regs[PHASE_A].ro_current_peak & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IPEAK_LW)
			{
				temp1 = ade7753_regs[PHASE_B].ro_current_peak >> 8;
				temp2 = ade7753_regs[PHASE_B].ro_current_peak & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IPEAK_LW)
			{
				temp1 = ade7753_regs[PHASE_C].ro_current_peak >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_current_peak & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IRMS_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_A].ro_current_rms >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_B].ro_current_rms >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_C].ro_current_rms >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IRMS_LW)
			{
				temp1 = ade7753_regs[PHASE_A].ro_current_rms >> 8;
				temp2 = ade7753_regs[PHASE_A].ro_current_rms & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_LW)
			{
				temp1 = ade7753_regs[PHASE_B].ro_current_rms >> 8;
				temp2 = ade7753_regs[PHASE_B].ro_current_rms & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_LW)
			{
				temp1 = ade7753_regs[PHASE_C].ro_current_rms >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_current_rms & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_WAVEFORM_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_A].ro_waveform >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_WAVEFORM_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_B].ro_waveform >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_WAVEFORM_HW)
			{
				temp1 = 0;
				temp2 = ade7753_regs[PHASE_C].ro_waveform >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_WAVEFORM_LW)
			{
				temp1 = ade7753_regs[PHASE_A].ro_waveform >> 8;
				temp2 = ade7753_regs[PHASE_A].ro_waveform & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_WAVEFORM_LW)
			{
				temp1 = ade7753_regs[PHASE_B].ro_waveform >> 8;
				temp2 = ade7753_regs[PHASE_B].ro_waveform & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_WAVEFORM_LW)
			{
				temp1 = ade7753_regs[PHASE_C].ro_waveform >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_waveform & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_PM0_VRMS_VALUE)
			{
				temp1 = ade7753_regs[PHASE_A].ro_vrms_value >> 8;
				temp2 = ade7753_regs[PHASE_A].ro_vrms_value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VRMS_VALUE)
			{
				temp1 = ade7753_regs[PHASE_B].ro_vrms_value >> 8;
				temp2 = ade7753_regs[PHASE_B].ro_vrms_value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VRMS_VALUE)
			{
				temp1 = ade7753_regs[PHASE_C].ro_vrms_value >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_vrms_value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IRMS_VALUE)
			{
				temp1 = ade7753_regs[PHASE_A].ro_irms_value >> 8;
				temp2 = ade7753_regs[PHASE_A].ro_irms_value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_VALUE)
			{
				temp1 = ade7753_regs[PHASE_B].ro_irms_value >> 8;
				temp2 = ade7753_regs[PHASE_B].ro_irms_value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_VALUE)
			{
				temp1 = ade7753_regs[PHASE_C].ro_irms_value >> 8;
				temp2 = ade7753_regs[PHASE_C].ro_irms_value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
				

			else if(address == MODBUS_PM0_VRMS_CAL_VALUE)
			{
				temp1 = vrms_cal[PHASE_A].value >> 8;
				temp2 = vrms_cal[PHASE_A].value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VRMS_CAL_VALUE)
			{
				temp1 = vrms_cal[PHASE_B].value >> 8;
				temp2 = vrms_cal[PHASE_B].value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VRMS_CAL_VALUE)
			{
				temp1 = vrms_cal[PHASE_C].value >> 8;
				temp2 = vrms_cal[PHASE_C].value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_VRMS_CAL_ADC_HW)
			{
				temp1 = vrms_cal[PHASE_A].adc >> 24;
				temp2 = vrms_cal[PHASE_A].adc >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VRMS_CAL_ADC_HW)
			{
				temp1 = vrms_cal[PHASE_B].adc >> 24;
				temp2 = vrms_cal[PHASE_B].adc >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VRMS_CAL_ADC_HW)
			{
				temp1 = vrms_cal[PHASE_C].adc >> 24;
				temp2 = vrms_cal[PHASE_C].adc >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_VRMS_CAL_ADC_LW)
			{
				temp1 = vrms_cal[PHASE_A].adc >> 8;
				temp2 = vrms_cal[PHASE_A].adc & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VRMS_CAL_ADC_LW)
			{
				temp1 = vrms_cal[PHASE_B].adc >> 8;
				temp2 = vrms_cal[PHASE_B].adc & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VRMS_CAL_ADC_LW)
			{
				temp1 = vrms_cal[PHASE_C].adc >> 8;
				temp2 = vrms_cal[PHASE_C].adc & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_VRMS_CAL_K_HW)
			{
				temp1 = vrms_cal[PHASE_A].k_slope.l >> 24;
				temp2 = vrms_cal[PHASE_A].k_slope.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VRMS_CAL_K_HW)
			{
				temp1 = vrms_cal[PHASE_B].k_slope.l >> 24;
				temp2 = vrms_cal[PHASE_B].k_slope.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VRMS_CAL_K_HW)
			{
				temp1 = vrms_cal[PHASE_C].k_slope.l >> 24;
				temp2 = vrms_cal[PHASE_C].k_slope.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			else if(address == MODBUS_PM0_IRMS_CAL_K_LW)
			{
				temp1 = vrms_cal[PHASE_A].k_slope.l >> 8;
				temp2 = vrms_cal[PHASE_A].k_slope.l & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_CAL_K_LW)
			{
				temp1 = vrms_cal[PHASE_B].k_slope.l >> 8;
				temp2 = vrms_cal[PHASE_B].k_slope.l & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_CAL_K_LW)
			{
				temp1 = vrms_cal[PHASE_C].k_slope.l >> 8;
				temp2 = vrms_cal[PHASE_C].k_slope.l & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IRMS_CAL_VALUE)
			{
				temp1 = irms_cal[PHASE_A].value >> 8;
				temp2 = irms_cal[PHASE_A].value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_CAL_VALUE)
			{
				temp1 = irms_cal[PHASE_B].value >> 8;
				temp2 = irms_cal[PHASE_B].value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_CAL_VALUE)
			{
				temp1 = irms_cal[PHASE_C].value >> 8;
				temp2 = irms_cal[PHASE_C].value & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			else if(address == MODBUS_PM0_IRMS_CAL_ADC_HW)
			{
				temp1 = irms_cal[PHASE_A].adc >> 24;
				temp2 = irms_cal[PHASE_A].adc >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_CAL_ADC_HW)
			{
				temp1 = irms_cal[PHASE_B].adc >> 24;
				temp2 = irms_cal[PHASE_B].adc >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_CAL_ADC_HW)
			{
				temp1 = irms_cal[PHASE_C].adc >> 24;
				temp2 = irms_cal[PHASE_C].adc >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IRMS_CAL_ADC_LW)
			{
				temp1 = irms_cal[PHASE_A].adc >> 8;
				temp2 = irms_cal[PHASE_A].adc & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_CAL_ADC_LW)
			{
				temp1 = irms_cal[PHASE_B].adc >> 8;
				temp2 = irms_cal[PHASE_B].adc & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_CAL_ADC_LW)
			{
				temp1 = irms_cal[PHASE_C].adc >> 8;
				temp2 = irms_cal[PHASE_C].adc & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IRMS_CAL_K_HW)
			{
				temp1 = irms_cal[PHASE_A].k_slope.l >> 24;
				temp2 = irms_cal[PHASE_A].k_slope.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_CAL_K_HW)
			{
				temp1 = irms_cal[PHASE_B].k_slope.l >> 24;
				temp2 = irms_cal[PHASE_B].k_slope.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_CAL_K_HW)
			{
				temp1 = irms_cal[PHASE_C].k_slope.l >> 24;
				temp2 = irms_cal[PHASE_C].k_slope.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IRMS_CAL_K_LW)
			{
				temp1 = irms_cal[PHASE_A].k_slope.l >> 8;
				temp2 = irms_cal[PHASE_A].k_slope.l & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IRMS_CAL_K_LW)
			{
				temp1 = irms_cal[PHASE_B].k_slope.l >> 8;
				temp2 = irms_cal[PHASE_B].k_slope.l & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IRMS_CAL_K_LW)
			{
				temp1 = irms_cal[PHASE_C].k_slope.l >> 8;
				temp2 = irms_cal[PHASE_C].k_slope.l & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
/********************************************************************************/
			else if(address == MODBUS_PM0_LINECYC)
			{
				temp1 = ade7753_regs[PHASE_A].rw_line_cycle >> 8;
				temp2 = ade7753_regs[PHASE_A].rw_line_cycle & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LINECYC)
			{
				temp1 = ade7753_regs[PHASE_B].rw_line_cycle >> 8;
				temp2 = ade7753_regs[PHASE_B].rw_line_cycle & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LINECYC)
			{
				temp1 = ade7753_regs[PHASE_C].rw_line_cycle >> 8;
				temp2 = ade7753_regs[PHASE_C].rw_line_cycle & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			else if(address == MODBUS_TEST_COUNTER)
			{
				temp1 = test_counter >> 8;
				temp2 = test_counter & 0X00FF;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM_TEST)
			{
				temp1 = 0;
				temp2 = ade7753_test;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
// ENERGY CALIBRATION SECTION
			else if(address == MODBUS_PM0_CF_CAL_EN)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_A].cf_calibration_enable;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_CF_CAL_EN)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_B].cf_calibration_enable;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_CF_CAL_EN)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_C].cf_calibration_enable;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_CF_IMP_PER_KWH)
			{
				temp1 = wh_calibration[PHASE_A].cf_imp_per_kwh >> 8;
				temp2 = wh_calibration[PHASE_A].cf_imp_per_kwh & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_CF_IMP_PER_KWH)
			{
				temp1 = wh_calibration[PHASE_B].cf_imp_per_kwh >> 8;
				temp2 = wh_calibration[PHASE_B].cf_imp_per_kwh & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_CF_IMP_PER_KWH)
			{
				temp1 = wh_calibration[PHASE_C].cf_imp_per_kwh >> 8;
				temp2 = wh_calibration[PHASE_C].cf_imp_per_kwh & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_VT)
			{
				temp1 = wh_calibration[PHASE_A].Vtest >> 8;
				temp2 = wh_calibration[PHASE_A].Vtest & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_VT)
			{
				temp1 = wh_calibration[PHASE_B].Vtest >> 8;
				temp2 = wh_calibration[PHASE_B].Vtest & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_VT)
			{
				temp1 = wh_calibration[PHASE_C].Vtest >> 8;
				temp2 = wh_calibration[PHASE_C].Vtest & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_IT)
			{
				temp1 = wh_calibration[PHASE_A].Itest >> 8;
				temp2 = wh_calibration[PHASE_A].Itest & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_IT)
			{
				temp1 = wh_calibration[PHASE_B].Itest >> 8;
				temp2 = wh_calibration[PHASE_B].Itest & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_IT)
			{
				temp1 = wh_calibration[PHASE_C].Itest >> 8;
				temp2 = wh_calibration[PHASE_C].Itest & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_CF_EXPECTED_HIGH)
			{
				temp1 = wh_calibration[PHASE_A].cf_expected.l >> 24;
				temp2 = wh_calibration[PHASE_A].cf_expected.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_CF_EXPECTED_HIGH)
			{
				temp1 = wh_calibration[PHASE_B].cf_expected.l >> 24;
				temp2 = wh_calibration[PHASE_B].cf_expected.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_CF_EXPECTED_HIGH)
			{
				temp1 = wh_calibration[PHASE_C].cf_expected.l >> 24;
				temp2 = wh_calibration[PHASE_C].cf_expected.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_CF_EXPECTED_LOW)
			{
				temp1 = wh_calibration[PHASE_A].cf_expected.l >> 8;
				temp2 = wh_calibration[PHASE_A].cf_expected.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_CF_EXPECTED_LOW)
			{
				temp1 = wh_calibration[PHASE_B].cf_expected.l >> 8;
				temp2 = wh_calibration[PHASE_B].cf_expected.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_CF_EXPECTED_LOW)
			{
				temp1 = wh_calibration[PHASE_C].cf_expected.l >> 8;
				temp2 = wh_calibration[PHASE_C].cf_expected.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_CF_CONST_W_LSB_HIGH)
			{
				temp1 = wh_calibration[PHASE_A].const_w_lsb.l >> 24;
				temp2 = wh_calibration[PHASE_A].const_w_lsb.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_CF_CONST_W_LSB_HIGH)
			{
				temp1 = wh_calibration[PHASE_B].const_w_lsb.l >> 24;
				temp2 = wh_calibration[PHASE_B].const_w_lsb.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_CF_CONST_W_LSB_HIGH)
			{
				temp1 = wh_calibration[PHASE_C].const_w_lsb.l >> 24;
				temp2 = wh_calibration[PHASE_C].const_w_lsb.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			else if(address == MODBUS_PM0_CF_CONST_W_LSB_LOW)
			{
				temp1 = wh_calibration[PHASE_A].const_w_lsb.l >> 8;
				temp2 = wh_calibration[PHASE_A].const_w_lsb.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_CF_CONST_W_LSB_LOW)
			{
				temp1 = wh_calibration[PHASE_B].const_w_lsb.l >> 8;
				temp2 = wh_calibration[PHASE_B].const_w_lsb.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_CF_CONST_W_LSB_LOW)
			{
				temp1 = wh_calibration[PHASE_C].const_w_lsb.l >> 8;
				temp2 = wh_calibration[PHASE_C].const_w_lsb.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			else if(address == MODBUS_PM0_LAENERGY_TEST_HIGH)
			{
				temp1 = wh_calibration[PHASE_A].Laenergy_test >> 24;
				temp2 = wh_calibration[PHASE_A].Laenergy_test >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LAENERGY_TEST_HIGH)
			{
				temp1 = wh_calibration[PHASE_B].Laenergy_test >> 24;
				temp2 = wh_calibration[PHASE_B].Laenergy_test >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LAENERGY_TEST_HIGH)
			{
				temp1 = wh_calibration[PHASE_C].Laenergy_test >> 24;
				temp2 = wh_calibration[PHASE_C].Laenergy_test >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_LAENERGY_TEST_LOW)
			{
				temp1 = wh_calibration[PHASE_A].Laenergy_test >> 8;
				temp2 = wh_calibration[PHASE_A].Laenergy_test & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LAENERGY_TEST_LOW)
			{
				temp1 = wh_calibration[PHASE_B].Laenergy_test >> 8;
				temp2 = wh_calibration[PHASE_B].Laenergy_test & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LAENERGY_TEST_LOW)
			{
				temp1 = wh_calibration[PHASE_C].Laenergy_test >> 8;
				temp2 = wh_calibration[PHASE_C].Laenergy_test & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_LINECYC_IB)
			{
				temp1 = wh_calibration[PHASE_A].LineCyc_ib >> 8;
				temp2 = wh_calibration[PHASE_A].LineCyc_ib & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LINECYC_IB)
			{
				temp1 = wh_calibration[PHASE_B].LineCyc_ib >> 8;
				temp2 = wh_calibration[PHASE_B].LineCyc_ib & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LINECYC_IB)
			{
				temp1 = wh_calibration[PHASE_C].LineCyc_ib >> 8;
				temp2 = wh_calibration[PHASE_C].LineCyc_ib & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			else if(address == MODBUS_PM0_OFFSET_CAL_PRO)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_A].offset_calibration_pro;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_OFFSET_CAL_PRO)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_B].offset_calibration_pro;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_OFFSET_CAL_PRO)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_C].offset_calibration_pro;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_OFFSET_CALI_EN)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_A].offset_calibration_enable;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_OFFSET_CALI_EN)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_B].offset_calibration_enable;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_OFFSET_CALI_EN)
			{
				temp1 = 0;
				temp2 = wh_calibration[PHASE_C].offset_calibration_enable;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_I_CORRECTION_HIGH)
			{
				temp1 = wh_calibration[PHASE_A].I_correction.l >> 24;
				temp2 = wh_calibration[PHASE_A].I_correction.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_I_CORRECTION_HIGH)
			{
				temp1 = wh_calibration[PHASE_B].I_correction.l >> 24;
				temp2 = wh_calibration[PHASE_B].I_correction.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_I_CORRECTION_HIGH)
			{
				temp1 = wh_calibration[PHASE_C].I_correction.l >> 24;
				temp2 = wh_calibration[PHASE_C].I_correction.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_I_CORRECTION_LOW)
			{
				temp1 = wh_calibration[PHASE_A].I_correction.l >> 8;
				temp2 = wh_calibration[PHASE_A].I_correction.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_I_CORRECTION_LOW)
			{
				temp1 = wh_calibration[PHASE_B].I_correction.l >> 8;
				temp2 = wh_calibration[PHASE_B].I_correction.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_I_CORRECTION_LOW)
			{
				temp1 = wh_calibration[PHASE_C].I_correction.l >> 8;
				temp2 = wh_calibration[PHASE_C].I_correction.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			else if(address == MODBUS_PM0_LAENERGY_CORRECTION_HIGH)
			{
				temp1 = wh_calibration[PHASE_A].Laenergy_correction >> 24;
				temp2 = wh_calibration[PHASE_A].Laenergy_correction >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LAENERGY_CORRECTION_HIGH)
			{
				temp1 = wh_calibration[PHASE_B].Laenergy_correction >> 24;
				temp2 = wh_calibration[PHASE_B].Laenergy_correction >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LAENERGY_CORRECTION_HIGH)
			{
				temp1 = wh_calibration[PHASE_C].Laenergy_correction >> 24;
				temp2 = wh_calibration[PHASE_C].Laenergy_correction >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			else if(address == MODBUS_PM0_LAENERGY_CORRECTION_LOW)
			{
				temp1 = wh_calibration[PHASE_A].Laenergy_correction >> 8;
				temp2 = wh_calibration[PHASE_A].Laenergy_correction & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LAENERGY_CORRECTION_LOW)
			{
				temp1 = wh_calibration[PHASE_B].Laenergy_correction >> 8;
				temp2 = wh_calibration[PHASE_B].Laenergy_correction & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LAENERGY_CORRECTION_LOW)
			{
				temp1 = wh_calibration[PHASE_C].Laenergy_correction >> 8;
				temp2 = wh_calibration[PHASE_C].Laenergy_correction & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			else if(address == MODBUS_PM0_LSB_VARIATION_HIGH)
			{
				temp1 = wh_calibration[PHASE_A].lsb_variation.l >> 24;
				temp2 = wh_calibration[PHASE_A].lsb_variation.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LSB_VARIATION_HIGH)
			{
				temp1 = wh_calibration[PHASE_B].lsb_variation.l >> 24;
				temp2 = wh_calibration[PHASE_B].lsb_variation.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LSB_VARIATION_HIGH)
			{
				temp1 = wh_calibration[PHASE_C].lsb_variation.l >> 24;
				temp2 = wh_calibration[PHASE_C].lsb_variation.l >> 16;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			else if(address == MODBUS_PM0_LSB_VARIATION_LOW)
			{
				temp1 = wh_calibration[PHASE_A].lsb_variation.l >> 8;
				temp2 = wh_calibration[PHASE_A].lsb_variation.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LSB_VARIATION_LOW)
			{
				temp1 = wh_calibration[PHASE_B].lsb_variation.l >> 8;
				temp2 = wh_calibration[PHASE_B].lsb_variation.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LSB_VARIATION_LOW)
			{
				temp1 = wh_calibration[PHASE_C].lsb_variation.l >> 8;
				temp2 = wh_calibration[PHASE_C].lsb_variation.l & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
			
			
			
			
			else if(address == MODBUS_PM0_LINECYC_I_MIN)
			{
				temp1 = wh_calibration[PHASE_A].LineCyc_imin >> 8;
				temp2 = wh_calibration[PHASE_A].LineCyc_imin & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM1_LINECYC_I_MIN)
			{
				temp1 = wh_calibration[PHASE_B].LineCyc_imin >> 8;
				temp2 = wh_calibration[PHASE_B].LineCyc_imin & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address == MODBUS_PM2_LINECYC_I_MIN)
			{
				temp1 = wh_calibration[PHASE_C].LineCyc_imin >> 8;
				temp2 = wh_calibration[PHASE_C].LineCyc_imin & 0xff;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			
	//VRMS OFFSET CALIBRATION		
			
			
			
			else if((address >= MODBUS_AI_VALUE0)&&(address <= MODBUS_AI_VALUE3))
			{
				u8 pos;
				uint16 itemp;
				pos = address - MODBUS_AI_VALUE0;
				itemp = inputs[pos].value/100;
				temp1 = itemp>>8;
				temp2 = itemp;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AI_FILTER0)&&(address <= MODBUS_AI_FILTER3))
			{
				u8 pos; 
				pos = address - MODBUS_AI_FILTER0; 
				temp1 = 0;
				temp2 = inputs[pos].filter;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AI_RANGE0)&&(address <= MODBUS_AI_RANGE3))
			{
				u8 pos; 
				pos = address - MODBUS_AI_RANGE0; 
				temp1 = 0;
				temp2 = inputs[pos].range;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AI_AM0)&&(address <= MODBUS_AI_AM3))
			{
				u8 pos; 
				pos = address - MODBUS_AI_AM0; 
				temp1 = 0;
				temp2 = inputs[pos].auto_manual;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AI_CAL_SIGN0)&&(address <= MODBUS_AI_CAL_SIGN3))
			{
				u8 pos; 
				pos = address - MODBUS_AI_CAL_SIGN0; 
				temp1 = 0;
				temp2 = inputs[pos].calibration_sign;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AI_OFFSET0)&&(address <= MODBUS_AI_OFFSET3))
			{
				u8 pos; 
				pos = address - MODBUS_AI_OFFSET0; 
				temp1 = inputs[pos].calibration_hi;
				temp2 = inputs[pos].calibration_lo;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AO_VALUE0)&&(address <= MODBUS_AO_VALUE3))
			{
				u8 pos;
				uint16 itemp;
				
				pos = address - MODBUS_AO_VALUE0; 
				itemp = outputs[pos].control;
				temp1 = itemp >> 8;
				temp2 = itemp;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AO_AM0)&&(address <= MODBUS_AO_AM3))
			{
				u8 pos;  
				pos = address - MODBUS_AO_AM0; 
				temp1 = 0;
				temp2 = outputs[pos].auto_manual;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AO_RANGE0)&&(address <= MODBUS_AO_RANGE3))
			{
				u8 pos;  
				pos = address - MODBUS_AO_RANGE0; 
				temp1 = 0;
				temp2 = outputs[pos].range;
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AI_RAW0)&&(address <= MODBUS_AI_RAW3))
			{
				u8 pos;  
				pos = address - MODBUS_AI_RAW0; 
				temp1 = input_raw[pos] >> 8;
				temp2 = input_raw[pos];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if((address >= MODBUS_AO_RAW0)&&(address <= MODBUS_AO_RAW3))
			{
				u8 pos;  
				pos = address - MODBUS_AO_RAW0; 
				temp1 = output_raw[pos] >> 8;
				temp2 = output_raw[pos];
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if( address == TSTAT_NAME_ENABLE)  
			{ 
				temp1= 0;
				temp2= 0x56;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);				
			}	
			else if((address >= TSTAT_NAME1) && (address <= TSTAT_NAME10))  
			{
				u16 temp = address - TSTAT_NAME1;  
				temp1= panelname[temp * 2];
				temp2= panelname[temp * 2 + 1];
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}
			else if(address >= MODBUS_USER_BLOCK_FIRST && address <= MODBUS_USER_BLOCK_LAST)
			{
				U16_T far temp;
				temp = read_user_data_by_block(address);
				
				temp1 = (temp >> 8) & 0xFF;;
				temp2 = temp & 0xFF;
				sendbuf[send_cout++] = temp1 ;
				sendbuf[send_cout++] = temp2 ;
				crc16_byte(temp1);
				crc16_byte(temp2);
			} 
			else
			{
				temp1 = 0 ;
				temp2 =  0; 
				sendbuf[send_cout++] = temp1;
				sendbuf[send_cout++] = temp2;
				crc16_byte(temp1);
				crc16_byte(temp2);
			}

		}//end of number
		temp1 = CRChi;
		temp2 =  CRClo; 
		sendbuf[send_cout++] = temp1;
		sendbuf[send_cout++] = temp2;
		if(type == 0)
		{
			memcpy(uart_send, sendbuf, send_cout);
			USART_SendDataString(send_cout);
		}
		else
		{
			sendbuf[0] = pData[0];
			sendbuf[1] = pData[1];
			sendbuf[2] = 0 ;
			sendbuf[3] = 0 ;
			sendbuf[4] = (3 + RegNum * 2) >> 8; 
			sendbuf[5] =(u8)(3 + RegNum * 2) ;
			memcpy(tcp_server_sendbuf,sendbuf,RegNum * 2 + 3 + HeadLen);
			tcp_server_sendlen = RegNum * 2 + 3 + HeadLen;
		}
	}
	else if (USART_RX_BUF[1] == CHECKONLINE)
	{
		// send first byte of information
		temp2 =  pData[HeadLen+0]; 
		uart_send[send_cout++] = temp2 ;
		crc16_byte(temp2);
	

		temp2 = pData[HeadLen+1]; 
		uart_send[send_cout++] = temp2 ;
		crc16_byte(temp2);
	

		temp2 =  modbus.address; 
		uart_send[send_cout++] = temp2 ;
		crc16_byte(temp2);
		

		temp2 =  modbus.serial_Num[0]; 
		uart_send[send_cout++] = temp2 ;
		crc16_byte(temp2);
		
		temp2 =  modbus.serial_Num[1]; 
		uart_send[send_cout++] = temp2 ;
		crc16_byte(temp2);
		

		temp2 =  modbus.serial_Num[2]; 
		uart_send[send_cout++] = temp2 ;
		crc16_byte(temp2);
		

		temp2 =  modbus.serial_Num[3]; 
		uart_send[send_cout++] = temp2 ;
		crc16_byte(temp2);
		
		temp2 =  CRChi; 
		uart_send[send_cout++] = temp2 ;
		temp2 =  CRClo; 
		uart_send[send_cout++] = temp2 ;
		USART_SendDataString(send_cout);
	}
}

u8 checkData(u16 address)
{
	//static unsigned char xdata rand_read_ten_count = 0 ;
	u16 crc_val;
	u8 minaddr,maxaddr, variable_delay;
	u8 i;
	static u8 srand_count =0 ;
	srand_count ++ ;
	// check if packet completely received
	if(revce_count != rece_size)
		return 0;

	// check if talking to correct device ID
	if(USART_RX_BUF[0] != 255 && USART_RX_BUF[0] != modbus.address && USART_RX_BUF[0] != 0)
		return 0;	

	//  --- code to verify what is on the network ---------------------------------------------------
	if( USART_RX_BUF[1] == CHECKONLINE)
	{
		crc_val = crc16(USART_RX_BUF,4) ;
		if(crc_val != (USART_RX_BUF[4]<<8) + USART_RX_BUF[5] )
		{
			return 0;
		}
		minaddr = (USART_RX_BUF[2] >= USART_RX_BUF[3] ) ? USART_RX_BUF[3] : USART_RX_BUF[2] ;	
		maxaddr = (USART_RX_BUF[2] >= USART_RX_BUF[3] ) ? USART_RX_BUF[2] : USART_RX_BUF[3] ;
		if(modbus.address < minaddr || modbus.address > maxaddr)
			return 0;
		else
		{	// in the TRUE case, we add a random delay such that the Interface can pick up the packets
			srand(srand_count);
			variable_delay = rand() % 20;
			for ( i=0; i<variable_delay; i++)
				delay_us(100);
			return 1;
		}
	}
	
	// check that message is one of the following
	if( (USART_RX_BUF[1]!=READ_VARIABLES) && (USART_RX_BUF[1]!=WRITE_VARIABLES) && (USART_RX_BUF[1]!=MULTIPLE_WRITE) )
		return 0;
		
	if(USART_RX_BUF[2] * 256 + USART_RX_BUF[3] ==  MODBUS_ADDRESS_PLUG_N_PLAY)
	{
		if(USART_RX_BUF[1] == WRITE_VARIABLES)
		{
			if(USART_RX_BUF[6] != modbus.serial_Num[0]) 
			return FALSE;
			if(USART_RX_BUF[7] != modbus.serial_Num[1]) 
			return FALSE;
			if(USART_RX_BUF[8] != modbus.serial_Num[2])  
			return FALSE;
			if(USART_RX_BUF[9] != modbus.serial_Num[3]) 
			return FALSE;
		}
		
		if (USART_RX_BUF[1] == READ_VARIABLES)
		{
			randval = rand() % 5;
		}
		
		if(randval != RESPONSERANDVALUE)
		{
			return FALSE;
		}
		else
		{	// in the TRUE case, we add a random delay such that the Interface can pick up the packets
			variable_delay = rand() % 20;
			for ( i=0; i<variable_delay; i++)
				delay_us(100);
		}
	}

	// if trying to write the Serial number, first check to see if it has been already written
	// note this does not take count of multiple-write, thus if try to write into those reg with multiple-write, command will accept
	if( (USART_RX_BUF[1]==WRITE_VARIABLES)  && (address<= MODBUS_HARDWARE_REV) )
	{
		// Return false if trying to write SN Low word that has already been written
		if(USART_RX_BUF[3] < 2)
		{
			if(modbus.SNWriteflag & 0x01)                // low byte of SN writed
				return FALSE;
		}
		// Return false if trying to write SN High word that has already been written
		else if (USART_RX_BUF[3] < 4)
		{
			if(modbus.SNWriteflag  & 0x02)                 // high byte of SN writed
				return FALSE;
		}
		else if (USART_RX_BUF[3] ==  MODBUS_HARDWARE_REV)
		{
			if(modbus.SNWriteflag  & 0x04)                 // hardware byte writed
				return FALSE;
		}
	}

	crc_val = crc16(USART_RX_BUF, rece_size-2);

	if(crc_val == (USART_RX_BUF[rece_size-2]<<8) + USART_RX_BUF[rece_size-1] )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
 
 void dealwithData(void)
{	
	u16 address;
	// given this is used in multiple places, decided to put it as an argument
	address = (u16)(USART_RX_BUF[2] << 8) + USART_RX_BUF[3];
	if(checkData(address))
	{		
		initSend_COM();	
		init_crc16();		
		responseCmd(0,USART_RX_BUF);
		internalDeal(0, USART_RX_BUF);
	}
	else
	{
		serial_restart();
	}
}

void SoftReset(void)
{
	__set_FAULTMASK(1);      // 关闭所有中断
	NVIC_SystemReset();      // 复位
}
