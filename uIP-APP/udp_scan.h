#ifndef 	_UDP_SCAN_H
#define  	_UDP_SCAN_H


#include "dhcpc.h"
#include "modbus.h"

void udp_scan_init(void) ;

void udp_appcall(void) ;


void dhcpc_configured(const struct dhcpc_state *s);


#define UIP_UDP_APPCALL		udp_appcall



extern volatile u8 IP_Change ;



void UdpData(u8 type) ;


extern u8 update ;





#endif
