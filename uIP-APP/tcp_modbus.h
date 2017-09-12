#ifndef _TCP_MODBUS_H
#define _TCP_MODBUS_H
#include "string.h"
#include "stm32f10x.h"
//IO操作函数	 
#include "bitmap.h"
#include "uip.h"
#include "modbus.h"



void tcp_server_appcall(void) ;
void tcp_server_aborted(void) ;
void tcp_server_timedout(void) ;
void tcp_server_closed(void) ;
void tcp_server_connected(void) ;
void tcp_server_acked(void) ;
void tcp_server_senddata(void); 
	
extern u8 tcp_server_databuf[];   	//发送数据缓存	  
extern u8 tcp_server_sta;				//服务端状态
//[7]:0,无连接;1,已经连接;
//[6]:0,无数据;1,收到客户端数据
//[5]:0,无数据;1,有数据需要发送

extern u8 tcp_server_sendbuf[];
extern u16 tcp_server_sendlen;



void tcp_server_appcall(void) ;




#endif
