#include <string.h>
#include "stm32f10x.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "24cxx.h"
#include "spi.h"
#include "flash.h"
#include "stmflash.h"
#include "enc28j60.h"
#include "timerx.h"
#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "modbus.h"
#include "define.h"
#include "registerlist.h"
#include "rs485.h"
#include "ade7753.h"
#include "analog_input.h"
#include "analog_output.h"
static void vPowerMeterTask(void *pvParameters);
static void vCOMMTask(void *pvParameters);

static void vNETTask(void *pvParameters);
 

void uip_polling(void);

#define	BUF	((struct uip_eth_hdr *)&uip_buf[0])	
	
u8 update = 0;

static void debug_config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
}

int main(void)
{
  	NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8008000);
	debug_config();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
 	delay_init(72);
	
//	delay_ms(3000);
	EEP_Dat_Init();
//	uart1_init(115200);
	printf("\r\n main test \n\r");
	SPI2_Init();
 	TIM3_Int_Init(100,2);
//	SPI1_Init(); //initialise it in ade7753_init() function
	xTaskCreate( vPowerMeterTask, ( signed portCHAR * ) "PowerMeter", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
 	
   	xTaskCreate( vLED0Task, ( signed portCHAR * ) "LED0", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
 	xTaskCreate( vCOMMTask, ( signed portCHAR * ) "COMM", configMINIMAL_STACK_SIZE + 512, NULL, tskIDLE_PRIORITY + 3, NULL );
   	xTaskCreate( vNETTask, ( signed portCHAR * ) "NET",  configMINIMAL_STACK_SIZE + 256, NULL, tskIDLE_PRIORITY + 3, NULL );
	
	xTaskCreate( vInputTask, ( signed portCHAR * ) "AnalogInput",  256, NULL, tskIDLE_PRIORITY + 2, NULL );
	xTaskCreate( vOutputTask, ( signed portCHAR * ) "AnalogOutput",  256, NULL, tskIDLE_PRIORITY + 2, NULL );
	
	xTaskCreate( vMSTP_TASK, ( signed portCHAR * ) "MSTP", configMINIMAL_STACK_SIZE + 256  , NULL, tskIDLE_PRIORITY + 3, NULL );
 
	vTaskStartScheduler();
}

void vPowerMeterTask( void *pvParameters )
{ 
	int8 PHASE_TEMP = 0;
	int8 i;
	ade7753_init();
	delay_ms(100);
	while(1)
	{ 
//		if(PHASE_TEMP > 2) PHASE_TEMP = 0;
//		else
//			PHASE_TEMP++; 
//		printf("\r\n vPowerMeterTask \n\r");
		PHASE_TEMP = 1;
		for(i=0;i<30;i++)
		{
			read_frequency(PHASE_TEMP);
			read_voltage_peak(PHASE_TEMP);
			read_current_peak(PHASE_TEMP); 
			
			get_vrms(PHASE_TEMP);
			get_irms(PHASE_TEMP);
			 
			
			if(wh_calibration[PHASE_TEMP].cf_calibration_enable == 1)
			{
				if(watt_hour_calibration_cf(PHASE_TEMP))
				{
					wh_calibration[PHASE_TEMP].cf_calibration_enable = 0;
				}
				else
				{
					wh_calibration[PHASE_TEMP].cf_calibration_enable = 2;
				}
			}
			
			delay_ms(100);
		}
		delay_ms(1000);
	}
}
 
void vCOMMTask(void *pvParameters )
{
	modbus_init();
	delay_ms(100);
	for( ;; )
	{
		if(dealwithTag)
		{  
			dealwithTag--;
			if(dealwithTag == 1)//&& !Serial_Master)	
				dealwithData();
		}
		
		if(serial_receive_timeout_count > 0)  
		{
			serial_receive_timeout_count--; 
			if(serial_receive_timeout_count == 0)
			{
				serial_restart();
			}
		}
		delay_ms(5);
	}
}

void vNETTask( void *pvParameters )
{
	u8 count = 0 ;
	while(tapdev_init())	//初始化ENC28J60错误
	{								   
		delay_ms(50);
		printf("tapdev_init() failed ...\r\n");
	}
	delay_ms(100);
    for( ;; )
	{
		uip_polling();	//处理uip事件，必须插入到用户程序的循环体中 
		
		if((IP_Change == 1) || (update == 1))
		{
			count++ ;
			if(count == 10)
			{
//				if(IP_Change)
//				{
//					app2boot_type = 0x55;
//					AT24CXX_WriteOneByte(EEP_APP2BOOT_TYPE, app2boot_type);
//				}
				count = 0;
				IP_Change = 0;
				SoftReset();
			}
			
		}
		
		delay_ms(5);
    }
}




//uip事件处理函数
//必须将该函数插入用户主循环,循环调用.
void uip_polling(void)
{
	u8 i;
	static struct timer periodic_timer, arp_timer;
	static u8 timer_ok = 0;	 
	if(timer_ok == 0)		//仅初始化一次
	{
		timer_ok = 1;
		timer_set(&periodic_timer, CLOCK_SECOND / 2); 	//创建1个0.5秒的定时器 
		timer_set(&arp_timer, CLOCK_SECOND * 10);	   	//创建1个10秒的定时器 
	}
	
	uip_len = tapdev_read();							//从网络设备读取一个IP包,得到数据长度.uip_len在uip.c中定义
	if(uip_len > 0)							 			//有数据
	{   
		//处理IP数据包(只有校验通过的IP包才会被接收) 
		if(BUF->type == htons(UIP_ETHTYPE_IP))			//是否是IP包? 
		{
			uip_arp_ipin();								//去除以太网头结构，更新ARP表
			uip_input();   								//IP包处理			
			//当上面的函数执行后，如果需要发送数据，则全局变量 uip_len > 0
			//需要发送的数据在uip_buf, 长度是uip_len  (这是2个全局变量)		    
			if(uip_len > 0)								//需要回应数据
			{
				uip_arp_out();							//加以太网头结构，在主动连接时可能要构造ARP请求
				tapdev_send();							//发送数据到以太网
			}
		}
		else if (BUF->type == htons(UIP_ETHTYPE_ARP))	//处理arp报文,是否是ARP请求包?
		{
			uip_arp_arpin();
			
 			//当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
			//需要发送的数据在uip_buf, 长度是uip_len(这是2个全局变量)
 			if(uip_len > 0)
				tapdev_send();							//需要发送数据,则通过tapdev_send发送	 
		}
	}
	else if(timer_expired(&periodic_timer))				//0.5秒定时器超时
	{
		timer_reset(&periodic_timer);					//复位0.5秒定时器 
		
		//轮流处理每个TCP连接, UIP_CONNS缺省是40个  
		for(i = 0; i < UIP_CONNS; i++)
		{
			 uip_periodic(i);							//处理TCP通信事件
			
	 		//当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
			//需要发送的数据在uip_buf, 长度是uip_len (这是2个全局变量)
	 		if(uip_len > 0)
			{
				uip_arp_out();							//加以太网头结构，在主动连接时可能要构造ARP请求
				tapdev_send();							//发送数据到以太网
			}
		}
		
#if UIP_UDP	//UIP_UDP 
		//轮流处理每个UDP连接, UIP_UDP_CONNS缺省是10个
		for(i = 0; i < UIP_UDP_CONNS; i++)
		{
			uip_udp_periodic(i);						//处理UDP通信事件
			
	 		//当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
			//需要发送的数据在uip_buf, 长度是uip_len (这是2个全局变量)
			if(uip_len > 0)
			{
				uip_arp_out();							//加以太网头结构，在主动连接时可能要构造ARP请求
				tapdev_send();							//发送数据到以太网
			}
		}
#endif 
		//每隔10秒调用1次ARP定时器函数 用于定期ARP处理,ARP表10秒更新一次，旧的条目会被抛弃
		if(timer_expired(&arp_timer))
		{
			timer_reset(&arp_timer);
			uip_arp_timer();
		}
	}
}

void EEP_Dat_Init(void)
{
	u8 loop ;
	u8 temp[6]; 
	AT24CXX_Init();
	modbus.serial_Num[0] = AT24CXX_ReadOneByte(EEP_SERIALNUMBER_LOWORD);
	modbus.serial_Num[1] = AT24CXX_ReadOneByte(EEP_SERIALNUMBER_LOWORD+1);
	modbus.serial_Num[2] = AT24CXX_ReadOneByte(EEP_SERIALNUMBER_HIWORD);
	modbus.serial_Num[3] = AT24CXX_ReadOneByte(EEP_SERIALNUMBER_HIWORD+1);

	if((modbus.serial_Num[0]==0xff)&&(modbus.serial_Num[1]== 0xff)&&(modbus.serial_Num[2] == 0xff)&&(modbus.serial_Num[3] == 0xff))
	{
		modbus.serial_Num[0] = 1 ;
		modbus.serial_Num[1] = 1 ;
		modbus.serial_Num[2] = 2 ;
		modbus.serial_Num[3] = 2 ;
		AT24CXX_WriteOneByte(EEP_SERIALNUMBER_LOWORD, modbus.serial_Num[0]);
		AT24CXX_WriteOneByte(EEP_SERIALNUMBER_LOWORD+1, modbus.serial_Num[1]);
		AT24CXX_WriteOneByte(EEP_SERIALNUMBER_LOWORD+2, modbus.serial_Num[2]);
		AT24CXX_WriteOneByte(EEP_SERIALNUMBER_LOWORD+3, modbus.serial_Num[3]);
	}

	AT24CXX_WriteOneByte(EEP_VERSION_NUMBER_LO, SOFTREV&0XFF);
	AT24CXX_WriteOneByte(EEP_VERSION_NUMBER_HI, (SOFTREV>>8)&0XFF);
	modbus.address = AT24CXX_ReadOneByte(EEP_ADDRESS);
	if((modbus.address == 255) || (modbus.address == 0))
	{
		modbus.address = 254;
		AT24CXX_WriteOneByte(EEP_ADDRESS, modbus.address);
	}
//	modbus.product = AT24CXX_ReadOneByte(EEP_PRODUCT_MODEL);
//	if((modbus.product == 255)||(modbus.product == 0))
//	{
//		modbus.product = PRODUCT_ID ;
//		AT24CXX_WriteOneByte(EEP_PRODUCT_MODEL, modbus.product);
//	}
	modbus.product = PRODUCT_ID ;
	modbus.hardware_Rev = AT24CXX_ReadOneByte(EEP_HARDWARE_REV);
	if((modbus.hardware_Rev == 255)||(modbus.hardware_Rev == 0))
	{
		modbus.hardware_Rev = HW_VER ;
		AT24CXX_WriteOneByte(EEP_HARDWARE_REV, modbus.hardware_Rev);
	}
	modbus.update = AT24CXX_ReadOneByte(EEP_UPDATE_STATUS);
	modbus.SNWriteflag = AT24CXX_ReadOneByte(EEP_SERIALNUMBER_WRITE_FLAG);
	
	modbus.baud = AT24CXX_ReadOneByte(EEP_BAUDRATE);
	if(modbus.baud > 4) 
	{	
		modbus.baud = 1;
		AT24CXX_WriteOneByte(EEP_BAUDRATE, modbus.baud);
	}
	
	modbus.baud = 1;
	switch(modbus.baud)
	{
		case 0:
			modbus.baudrate = BAUDRATE_9600;
			uart1_init(BAUDRATE_9600);
			SERIAL_RECEIVE_TIMEOUT = 6;
		break ;
		case 1:
			modbus.baudrate = BAUDRATE_19200;
			uart1_init(BAUDRATE_19200);	
			SERIAL_RECEIVE_TIMEOUT = 3;
		break;
		case 2:
			modbus.baudrate = BAUDRATE_38400;
			uart1_init(BAUDRATE_38400);
			SERIAL_RECEIVE_TIMEOUT = 2;
		break;
		case 3:
			modbus.baudrate = BAUDRATE_57600;
			uart1_init(BAUDRATE_57600);	
			SERIAL_RECEIVE_TIMEOUT = 1;
		break;
		case 4:
			modbus.baudrate = BAUDRATE_115200;
			uart1_init(BAUDRATE_115200);
			SERIAL_RECEIVE_TIMEOUT = 1;
		break;
		default:
			modbus.baud = 4;
			modbus.baudrate = BAUDRATE_115200;
			uart1_init(BAUDRATE_115200);
			SERIAL_RECEIVE_TIMEOUT = 1;
		break ;				
	}
	
	for(loop = 0 ; loop<6; loop++)
	{
		temp[loop] = AT24CXX_ReadOneByte(EEP_MAC_ADDRESS_1+loop); 
	}
	
	if((temp[0]== 0xff)&&(temp[1]== 0xff)&&(temp[2]== 0xff)&&(temp[3]== 0xff)&&(temp[4]== 0xff)&&(temp[5]== 0xff) )
	{
		temp[0] = 0x04 ;
		temp[1] = 0x02 ;
		temp[2] = 0x35 ;
		temp[3] = 0xaF ;
		temp[4] = 0x00 ;
		temp[5] = 0x01 ;
		AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_1, temp[0]);
		AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_2, temp[1]);
		AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_3, temp[2]);
		AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_4, temp[3]);
		AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_5, temp[4]);
		AT24CXX_WriteOneByte(EEP_MAC_ADDRESS_6, temp[5]);		
	}
	for(loop =0; loop<6; loop++)
	{
		modbus.mac_addr[loop] =  temp[loop]	;
	}
	
	for(loop = 0 ; loop<4; loop++)
	{
		temp[loop] = AT24CXX_ReadOneByte(EEP_IP_ADDRESS_1+loop); 
	}
	if((temp[0]== 0xff)&&(temp[1]== 0xff)&&(temp[2]== 0xff)&&(temp[3]== 0xff) )
	{
		temp[0] = 192 ;
		temp[1] = 168 ;
		temp[2] = 0 ;
		temp[3] = 183 ;
		AT24CXX_WriteOneByte(EEP_IP_ADDRESS_1, temp[0]);
		AT24CXX_WriteOneByte(EEP_IP_ADDRESS_2, temp[1]);
		AT24CXX_WriteOneByte(EEP_IP_ADDRESS_3, temp[2]);
		AT24CXX_WriteOneByte(EEP_IP_ADDRESS_4, temp[3]);
	}
	for(loop = 0 ; loop<4; loop++)
	{
		modbus.ip_addr[loop] = 	temp[loop] ;
		modbus.ghost_ip_addr[loop] = modbus.ip_addr[loop];
	}
	
	temp[0] = AT24CXX_ReadOneByte(EEP_IP_MODE);
	if(temp[0] > 0)
	{
		temp[0] = 1;
		AT24CXX_WriteOneByte(EEP_IP_MODE, temp[0]);	
	}
	modbus.ip_mode = temp[0];
	modbus.ip_mode = 0;//////////////////////////////////////
	modbus.ghost_ip_mode = modbus.ip_mode;
	
	for(loop = 0 ; loop<4; loop++)
	{
		temp[loop] = AT24CXX_ReadOneByte(EEP_SUB_MASK_ADDRESS_1+loop); 
	}
	if((temp[0]== 0xff)&&(temp[1]== 0xff)&&(temp[2]== 0xff)&&(temp[3]== 0xff) )
	{
		temp[0] = 0xff ;
		temp[1] = 0xff ;
		temp[2] = 0xff ;
		temp[3] = 0 ;
		AT24CXX_WriteOneByte(EEP_SUB_MASK_ADDRESS_1, temp[0]);
		AT24CXX_WriteOneByte(EEP_SUB_MASK_ADDRESS_2, temp[1]);
		AT24CXX_WriteOneByte(EEP_SUB_MASK_ADDRESS_3, temp[2]);
		AT24CXX_WriteOneByte(EEP_SUB_MASK_ADDRESS_4, temp[3]);
	
	}				
	for(loop = 0 ; loop<4; loop++)
	{
		modbus.mask_addr[loop] = 	temp[loop] ;
		modbus.ghost_mask_addr[loop] = modbus.mask_addr[loop] ;
	}
	
	for(loop = 0 ; loop<4; loop++)
	{
		temp[loop] = AT24CXX_ReadOneByte(EEP_GATEWAY_ADDRESS_1+loop); 
	}
	if((temp[0]== 0xff)&&(temp[1]== 0xff)&&(temp[2]== 0xff)&&(temp[3]== 0xff) )
	{
		temp[0] = 192 ;
		temp[1] = 168 ;
		temp[2] = 0 ;
		temp[3] = 4 ;
		AT24CXX_WriteOneByte(EEP_GATEWAY_ADDRESS_1, temp[0]);
		AT24CXX_WriteOneByte(EEP_GATEWAY_ADDRESS_2, temp[1]);
		AT24CXX_WriteOneByte(EEP_GATEWAY_ADDRESS_3, temp[2]);
		AT24CXX_WriteOneByte(EEP_GATEWAY_ADDRESS_4, temp[3]);
	
	}				
	for(loop = 0 ; loop<4; loop++)
	{
		modbus.gate_addr[loop] = 	temp[loop] ;
		modbus.ghost_gate_addr[loop] = modbus.gate_addr[loop] ;
	}
	
	temp[0] = AT24CXX_ReadOneByte(EEP_TCP_SERVER);
	if(temp[0] == 0xff)
	{
		temp[0] = 0 ;
		AT24CXX_WriteOneByte(EEP_TCP_SERVER, temp[0]);
	}
	modbus.tcp_server = temp[0];
	modbus.ghost_tcp_server = modbus.tcp_server  ;
	
	temp[0] =AT24CXX_ReadOneByte(EEP_LISTEN_PORT_HI);
	temp[1] =AT24CXX_ReadOneByte(EEP_LISTEN_PORT_LO);
	if(temp[0] == 0xff && temp[1] == 0xff )
	{
		modbus.listen_port = 502 ;
		temp[0] = (modbus.listen_port>>8)&0xff ;
		temp[1] = modbus.listen_port&0xff ;				
	}
	modbus.listen_port = (temp[0]<<8)|temp[1] ;
	modbus.ghost_listen_port = modbus.listen_port ;
	
	modbus.write_ghost_system = 0 ;
	modbus.reset = 0 ;

	// restore the vrms&irms calibration
	//phrase A
	/////////VRMS
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_VALUE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_VALUE_BYTE_1);
	vrms_cal[PHASE_A].value = (temp[0] << 8) | temp[1];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_ADC_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_ADC_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_ADC_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_ADC_BYTE_3);
	vrms_cal[PHASE_A].adc = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_SLOPE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_SLOPE_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_SLOPE_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_VRMS_CAL0_SLOPE_BYTE_3);
	vrms_cal[PHASE_A].k_slope.l = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	///////////IRMS
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_VALUE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_VALUE_BYTE_1);
	irms_cal[PHASE_A].value = (temp[0] << 8) | temp[1];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_ADC_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_ADC_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_ADC_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_ADC_BYTE_3);
	irms_cal[PHASE_A].adc = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_SLOPE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_SLOPE_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_SLOPE_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_IRMS_CAL0_SLOPE_BYTE_3);
	irms_cal[PHASE_A].k_slope.l = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];



//phrase B
	/////////VRMS
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_VALUE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_VALUE_BYTE_1);
	vrms_cal[PHASE_B].value = (temp[0] << 8) | temp[1];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_ADC_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_ADC_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_ADC_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_ADC_BYTE_3);
	vrms_cal[PHASE_B].adc = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_SLOPE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_SLOPE_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_SLOPE_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_VRMS_CAL1_SLOPE_BYTE_3);
	vrms_cal[PHASE_B].k_slope.l = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	///////////IRMS
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_VALUE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_VALUE_BYTE_1);
	irms_cal[PHASE_B].value = (temp[0] << 8) | temp[1];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_ADC_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_ADC_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_ADC_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_ADC_BYTE_3);
	irms_cal[PHASE_B].adc = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_SLOPE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_SLOPE_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_SLOPE_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_IRMS_CAL1_SLOPE_BYTE_3);
	irms_cal[PHASE_B].k_slope.l = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];

//phrase C
	/////////VRMS
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_VALUE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_VALUE_BYTE_1);
	vrms_cal[PHASE_C].value = (temp[0] << 8) | temp[1];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_ADC_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_ADC_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_ADC_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_ADC_BYTE_3);
	vrms_cal[PHASE_C].adc = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_SLOPE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_SLOPE_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_SLOPE_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_VRMS_CAL2_SLOPE_BYTE_3);
	vrms_cal[PHASE_C].k_slope.l = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	///////////IRMS
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_VALUE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_VALUE_BYTE_1);
	irms_cal[PHASE_C].value = (temp[0] << 8) | temp[1];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_ADC_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_ADC_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_ADC_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_ADC_BYTE_3);
	irms_cal[PHASE_C].adc = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];
	
	temp[0] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_SLOPE_BYTE_0);
	temp[1] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_SLOPE_BYTE_1);
	temp[2] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_SLOPE_BYTE_2);
	temp[3] = AT24CXX_ReadOneByte(EEP_IRMS_CAL2_SLOPE_BYTE_3);
	irms_cal[PHASE_C].k_slope.l = (temp[0] << 24) | (temp[1] << 16) | (temp[2] << 8) | temp[3];





	AT24CXX_Read(EEP_TSTAT_NAME1, panelname, 21); 
	
	modbus.protocal= AT24CXX_ReadOneByte(EEP_MODBUS_COM_CONFIG);
	if((modbus.protocal!=MODBUS)&&(modbus.protocal!=BAC_MSTP ))
	{
		modbus.protocal = MODBUS ;
		AT24CXX_WriteOneByte(EEP_MODBUS_COM_CONFIG, modbus.protocal);
	}

}




