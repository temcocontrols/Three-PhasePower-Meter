#ifndef __MODBUS_H
#define	__MODBUS_H
#include <string.h>
#include "stm32f10x.h"
#include "bitmap.h"
#include "modbus_crc.h"
#include "define.h"
#include "filter.h"
#include "delay.h"

#ifndef FALSE
#define FALSE 0 
#endif 

#ifndef TRUE
#define TRUE 1 
#endif 

#ifndef TXEN
#define TXEN		PAout(8)
#endif

#define SEND			1			//1
#define	RECEIVE		0

#define	READ_VARIABLES				0x03
#define	WRITE_VARIABLES				0x06
#define	MULTIPLE_WRITE				0x10
#define	CHECKONLINE					0x19

#define DATABUFLEN					200
#define DATABUFLEN_SCAN				12
#define SENDPOOLLEN         		8

#define SERIAL_COM_IDLE				0
#define INVALID_PACKET				1
#define VALID_PACKET				2

#define USART_REC_LEN  			200  	//定义最大接收字节数 200
#define USART_SEND_LEN			490

#define RESPONSERANDVALUE	1


extern u8  USART_RX_BUF[USART_REC_LEN]; //接收缓冲,最大USART_REC_LEN个字节.末字节为换行符 
extern vu8 revce_count ;
extern u8 dealwithTag ;

//extern u8 app2boot_type;

void serial_restart(void);
void modbus_init(void) ;
void SoftReset(void) ;

typedef struct 
{
	u8 serial_Num[4];
//	u16 software ;
	u8 address ;
	u32 baudrate ;
	u8	baud ;
	u8 update ;
	u8 product ;
	u8 hardware_Rev;
	u8 SNWriteflag ;
	u8 com_config;
	u8 protocal ;
	u8 reset ;
	u8 mac_addr[6] ;
	u8 ip_addr[4]  ;
	u8 ip_mode     ;
	u8 mask_addr[4] ;
	u8 gate_addr[4] ;
	u8 tcp_server ;
	u16 listen_port ;
	u8 mac_enable ;
	u8 ghost_ip_addr[4]  ;
	u8 ghost_ip_mode     ;
	u8 ghost_mask_addr[4] ;
	u8 ghost_gate_addr[4] ;
	u8 ghost_tcp_server ;
	u16 ghost_listen_port ;
	
	u8 write_ghost_system  ;
}STR_MODBUS ;

extern STR_MODBUS modbus ;

typedef struct
{
	u8 rx1_flag;
//	u8 rx2_flag;
	u8 tx1_flag;
//	u8 tx2_flag;
}STR_UART;
extern STR_UART uart;


extern vu8 serial_receive_timeout_count ;
void dealwithData(void) ;
void send_byte(u8 ch, u8 crc) ;
void responseCmd(u8 type, u8* pData); 
void internalDeal(u8 type,  u8 *pData) ;
void USART_SendDataString(u16 num) ;
extern u8 uart_send[USART_SEND_LEN] ;
extern u8 SERIAL_RECEIVE_TIMEOUT ;

void EEP_Dat_Init(void) ;


#endif
