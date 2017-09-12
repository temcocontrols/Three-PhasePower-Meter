#ifndef __MYIIC_H
#define __MYIIC_H

#include "bitmap.h"
   	   		   
//IO方向设置
//#define SDA_IN()	{GPIOE->CRH &= 0XF0FFFFFF; GPIOE->CRH |= ((u32)8 << 24);}
//#define SDA_OUT()	{GPIOE->CRH &= 0XF0FFFFFF; GPIOE->CRH |= ((u32)3 << 24);}
#define SDA_IN()	{GPIOF->CRH &= 0XFFFFFFF0; GPIOF->CRH |= ((u32)8 << 0);}
#define SDA_OUT()	{GPIOF->CRH &= 0XFFFFFFF0; GPIOF->CRH |= ((u32)3 << 0);}

//IO操作函数	 
#define IIC_SCL		PFout(9)	//SCL
#define IIC_SDA		PFout(8)	//SDA	 
#define READ_SDA	PFin(8)	//输入SDA 
#define IIC_WP		PFout(7)

//IIC所有操作函数
void IIC_Init(void);				//初始化IIC的IO口				 
void IIC_Start(void);				//发送IIC开始信号
void IIC_Stop(void);				//发送IIC停止信号
void IIC_Send_Byte(u8 txd);			//IIC发送一个字节
u8 IIC_Read_Byte(unsigned char ack);//IIC读取一个字节
u8 IIC_Wait_Ack(void);				//IIC等待ACK信号
void IIC_Ack(void);					//IIC发送ACK信号
void IIC_NAck(void);				//IIC不发送ACK信号

void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_Read_One_Byte(u8 daddr,u8 addr);
  
#endif
