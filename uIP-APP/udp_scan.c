#include "stdio.h"
#include "string.h"
#include "udp_scan.h"
#include "uip.h"
#include "modbus.h"
#include "resolv.h"
#include "registerlist.h"
#include "tapdev.h"
#include "24cxx.h"
#include "tcpip.h"
#define GUDPBC_MAX_CONNS			2
#define GUDPBC_NO_NEW_CONN			0xFF

#define GUDPBC_STATE_FREE			0
#define	GUDPBC_STATE_WAIT			1
#define	GUDPBC_STATE_CONNECTED		2
#define UDP_SCAN_LPORT 		1234
#define UDP_CLIENT_RPORT	5555

u8   InformationStr[60];
u8    state=1;
u8    scanstart=0;
volatile u8 IP_Change = 0  ;
//u8 test[50];
u8 state;
static u8 scan_temp[300];
void udp_scan_init(void)
{
//	struct uip_udp_conn *conn;
//	uip_ipaddr_t addr;
	
	// udp server
	uip_listen(HTONS(UDP_SCAN_LPORT));
	uip_udp_bind(&uip_udp_conns[0], HTONS(UDP_SCAN_LPORT));
	
}

void UdpData(u8 type)
{
// header 2 bytes
	memset(InformationStr, 0, 60);
 
// command header
	if(type == 0)
		InformationStr[0] = 0x65;
	else if(type == 1)
		InformationStr[0] = 0x67;
	InformationStr[1] = 0x00;
 
// two bytes data length, no use
//     InformationStr[2] = 0x00;
//     InformationStr[3] = 0x00;
 
// 4 bytes serialnumber 
	InformationStr[4] = modbus.serial_Num[0];
	InformationStr[6] = modbus.serial_Num[1];
	InformationStr[8] = modbus.serial_Num[2];
	InformationStr[10] = modbus.serial_Num[3];
 
// product ID
	InformationStr[12] = modbus.product;
 
// Modbus ID
	InformationStr[14] = modbus.address;
 
// IP address
	InformationStr[16] = modbus.ip_addr[0];
	InformationStr[18] = modbus.ip_addr[1];
	InformationStr[20] = modbus.ip_addr[2];
	InformationStr[22] = modbus.ip_addr[3];
 
// TCP port
	InformationStr[24] = modbus.listen_port;
	InformationStr[25] = modbus.listen_port >> 8;
 
// two bytes runtime code software version, no use in bootloader
     InformationStr[26] = SOFTREV;
     InformationStr[27] = SOFTREV >> 8;
 
// two bytes hardware version, no use in bootloader
     InformationStr[28] = modbus.hardware_Rev;
     InformationStr[29] = modbus.hardware_Rev >> 8;
 
// sub devices serial numbers, no use in bootloader
     InformationStr[30] = 0;
     InformationStr[31] = 0;
     InformationStr[32] = 0;
     InformationStr[33] = 0;
 
// two lower bytes of bacnet instance, no use in bootloader
     InformationStr[34] = Instance;
     InformationStr[35] = Instance >> 8;
 
// one byte bacnet station number, no use in bootloader
      InformationStr[36] = 0;
 
// 20 bytes device label, no use in bootloader
      memcpy(&InformationStr[37], panelname, 20);
 
// two higher bytes of bacnet instance, no use in bootloader
     InformationStr[57] = Instance >> 16;
     InformationStr[58] = Instance >> 24;
 
// bootloader mode or runtime mode, 0=runtime, !0=bootloader
	InformationStr[59] = 0;
 
// send out the response data
	uip_send((char *)InformationStr, 60); 
	state = 1;
}
 
void UDP_SCAN_APP(void)
{
//	struct uip_udp_conn *conn;
//	uip_ipaddr_t addr;
	
	u8 len;
	u8  n = 0;
	u8 i ;
	
	/* check the status */
//	if(uip_poll())
//	{  // auto send
//		char *tmp_dat = "udp server: the auto send!\r\n";
//		uip_send((char *)tmp_dat, strlen(tmp_dat));
//	}
	
	if(uip_newdata())
   {  // receive data
//      char tmp_dat[10];
      //char *tmp_dat = "udp client: receive the data\r\n";
      

     
//	   u8 *temp = uip_appdata;
//      uip_ipaddr_copy(addr, uip_udp_conn->ripaddr);
//      conn = uip_udp_new(&addr, uip_udp_conn->rport);
//      /* new data comes in */
            
      len = uip_len; 
	   memcpy(scan_temp, uip_appdata, len);
      if(scan_temp[0] == 0x64)
      {
         state = 1;
        for(n = 0;n < (u8)len / 4;n++)
         {       
            if((scan_temp[4*n+1] == modbus.ip_addr[0]) && (scan_temp[4*n+2] == modbus.ip_addr[1])
               &&(scan_temp[4*n+3] == modbus.ip_addr[2]) && (scan_temp[4*n+4] == modbus.ip_addr[3]))
            { 
               state=0;
            }
         }

         if(state)
         {            
            UdpData(0);
            
         }
      }
   #if 1
   else if((scan_temp[0] == 0x66) && (scan_temp[1] == modbus.ip_addr[0]) && (scan_temp[2] == modbus.ip_addr[1]) && (scan_temp[3] == modbus.ip_addr[2]) && (scan_temp[4] == modbus.ip_addr[3]))
   {
     
//	  print("%u,%u,%u,%u,%u,%u,%u,%u,", temp[1],temp[2],temp[3],temp[4],temp[5],temp[6],temp[7],temp[8]);
	  n = 5;
	 UdpData(1); 
     modbus.ip_mode = 0;
   
     modbus.ip_addr[0] = scan_temp[n++];
     modbus.ip_addr[1] = scan_temp[n++];
     modbus.ip_addr[2] = scan_temp[n++];
     modbus.ip_addr[3] = scan_temp[n++];
	   
	  modbus.ghost_ip_addr[0] = modbus.ip_addr[0];
	  modbus.ghost_ip_addr[1] = modbus.ip_addr[1];
	  modbus.ghost_ip_addr[2] = modbus.ip_addr[2];
	  modbus.ghost_ip_addr[3] = modbus.ip_addr[3];
   
	//print("%u,%u,%u,%u,%u,%u,%u,%u,",  modbus.ip_addr[0], modbus.ip_addr[1], modbus.ip_addr[2], modbus.ip_addr[3],temp[5],temp[6],temp[7],temp[8]);
     modbus.mask_addr[0] = scan_temp[n++];
     modbus.mask_addr[1] = scan_temp[n++];
     modbus.mask_addr[2] = scan_temp[n++];
     modbus.mask_addr[3] = scan_temp[n++];
	 
	 modbus.ghost_mask_addr[0] = modbus.mask_addr[0] ;
	 modbus.ghost_mask_addr[1] = modbus.mask_addr[1] ;
	 modbus.ghost_mask_addr[2] = modbus.mask_addr[2] ;
	 modbus.ghost_mask_addr[3] = modbus.mask_addr[3] ;
   
	  modbus.gate_addr[0] = scan_temp[n++];
      modbus.gate_addr[1] = scan_temp[n++];
      modbus.gate_addr[2] = scan_temp[n++];
      modbus.gate_addr[3] = scan_temp[n++];
	  
	 modbus.ghost_gate_addr[0] = modbus.gate_addr[0] ;
	 modbus.ghost_gate_addr[1] = modbus.gate_addr[1] ;
	 modbus.ghost_gate_addr[2] = modbus.gate_addr[2] ;
	 modbus.ghost_gate_addr[3] = modbus.gate_addr[3] ;
   
	IP_Change =1 ;
//		gudpbc_HandleSearchReq(pData, id, 1);
		
		
		
//		uip_send("command = 0x66", strlen("command = 0x66"));
//     ChangeIP = 1;
        if((modbus.ip_addr[0] != 0)  && (modbus.ip_addr[1] != 0)  && (modbus.ip_addr[3] != 0) )
        {

					
		for(i=0; i<4; i++)
		{
			
			AT24CXX_WriteOneByte(EEP_IP_ADDRESS_1+i, modbus.ip_addr[i]);
			AT24CXX_WriteOneByte(EEP_SUB_MASK_ADDRESS_1+i, modbus.mask_addr[i]);
			AT24CXX_WriteOneByte(EEP_GATEWAY_ADDRESS_1+i, modbus.gate_addr[i]);						
		}
		
//		delay_ms(100);
//		SoftReset();
//		tapdev_init() ;
         
//         //   reg_temp = 0;   // clear 0 to let bootloader knows the ip changes cause the reboot
//         //   IntFlashWrite(0x4001, &reg_temp, 1, ERA_RUN); // so it will back to runtime quickly
//            if(flag_I2C_ERROR == 0)
//            {
//               IntFlashWriteByte(0x4001,0);
//               AX11000_SoftReboot();
//            }
      }
  }
 // uip_send((char *)InformationStr, 60);
   #endif
//      uip_udp_remove(conn);
	}
}

void udp_appcall(void)
{
// udp server
	switch(uip_udp_conn->lport)
	{
		case HTONS(UDP_SCAN_LPORT):
			UDP_SCAN_APP();
		break;
		case HTONS(UDP_BACNET_LPORT):         
			UDP_bacnet_APP();
		break;
	}

// udp client   
    switch(uip_udp_conn->rport)
    {
      case HTONS(67):
         dhcpc_appcall();
         break;
      case HTONS(68):
         dhcpc_appcall();
         break;
      case HTONS(53):
         resolv_appcall();
         break;
//      case HTONS(UDP_CLIENT_RPORT):
//         UDP_CLIENT_APP();
//         break;
    }
}

void dhcpc_configured(const struct dhcpc_state *s)
{
	uip_sethostaddr(s->ipaddr);
	uip_setnetmask(s->netmask);
	uip_setdraddr(s->default_router);
	resolv_conf((u16_t *)s->dnsaddr);
	
	udp_scan_init();
	uip_listen(HTONS(modbus.listen_port));       // 10000, modbustcp
//	tcp_app_init();
	
	
	modbus.ip_addr[0] = uip_ipaddr1(uip_hostaddr);
	modbus.ip_addr[1] = uip_ipaddr2(uip_hostaddr);
	modbus.ip_addr[2] = uip_ipaddr3(uip_hostaddr);
	modbus.ip_addr[3] = uip_ipaddr4(uip_hostaddr);
	
	modbus.mask_addr[0] = uip_ipaddr1(uip_netmask);
	modbus.mask_addr[1] = uip_ipaddr2(uip_netmask);
	modbus.mask_addr[2] = uip_ipaddr3(uip_netmask);
	modbus.mask_addr[3] = uip_ipaddr4(uip_netmask);
	
	modbus.gate_addr[0] = uip_ipaddr1(uip_draddr);
	modbus.gate_addr[1] = uip_ipaddr2(uip_draddr);
	modbus.gate_addr[2] = uip_ipaddr3(uip_draddr);
	modbus.gate_addr[3] = uip_ipaddr4(uip_draddr);
	{
		uint8 i;
		for(i=0; i<4; i++)
		{
			modbus.ghost_ip_addr[i] = modbus.ip_addr[i];
			modbus.ghost_mask_addr[i] = modbus.mask_addr[i];
			modbus.ghost_gate_addr[i] = modbus.gate_addr[i];
			 					
		}
	}

	uip_ipaddr(uip_hostaddr_submask, modbus.ip_addr[0], modbus.ip_addr[1], modbus.ip_addr[2],255);

	bip_Init();	
}
