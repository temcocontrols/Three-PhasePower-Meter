/*
 ******************************************************************************
 *     Copyright (c) 2006	ASIX Electronic Corporation      All rights reserved.
 *
 *     This is unpublished proprietary source code of ASIX Electronic Corporation
 *
 *     The copyright notice above does not evidence any actual or intended
 *     publication of such source code.
 ******************************************************************************
 */
/*=============================================================================
 * Module Name:tcpip.h
 * Purpose:
 * Author:
 * Date:
 * Notes:
 * $Log: tcpip.h,v $
 *=============================================================================
 */

#ifndef __TCPIP_H__
#define __TCPIP_H__

/* INCLUDE FILE DECLARATIONS */
#include "stm32f10x.h"

/* NAMING CONSTANT DECLARATIONS */
#define TCPIP_MAX_APP_CONN			12
#define TCPIP_MAX_TCP_LISTEN		4
#define TCPIP_MAX_UDP_LISTEN		4
#define TCPIP_MAX_TCP_CONN			8
#define TCPIP_MAX_UDP_CONN			8

#define TCPIP_NO_NEW_CONN			0xff

#define	TCPIP_CONNECT_CANCEL		0
#define TCPIP_CONNECT_WAIT			1
#define TCPIP_CONNECT_ACTIVE		2
#define TCPIP_CONNECT_XMIT_COMPLETE	3
#define TCPIP_CONNECT_BUSY			0xf1

#define TCPIP_SEND_NOT_FINAL		0
#define TCPIP_SEND_FINAL			1
#define TCPIP_SEND_NOT_PUSH			2

#define TCPIP_KEEPALIVE_ON			1
#define TCPIP_KEEPALIVE_OFF			0

#define TCPIP_DONT_FRAGMENT			1
#define TCPIP_FRAGMENT				0

#define TCPIP_PROTOCOL_ICMP			1
#define TCPIP_PROTOCOL_TCP			6
#define TCPIP_PROTOCOL_UDP			17

#define TCPIP_NON_TRANSPARENT_MODE	0
#define TCPIP_TRANSPARENT_MODE		1

#define TCPIP_TIME_TO_LIVE			255	/* TTL item of ip layer */

/* GLOBAL VARIABLES */

/* for uip */
extern uint8_t  uip_buf[];
extern  uint16_t  uip_len;
//extern  uint8_t *uip_appdata;

#define TCPIP_GetRcvBuffer()		(uip_buf)
#define TCPIP_GetXmitBuffer()		(uip_buf)
#define TCPIP_SetRcvLength(len)		(uip_len = len)
#define TCPIP_SetXmitLength(len)	(uip_len = len)
#define TCPIP_GetXmitLength()		(uip_len)
#define TCPIP_GetPayloadBuffer()	(uip_appdata)

/* EXPORTED SUBPROGRAM SPECIFICATIONS */
void TCPIP_Init(uint8_t);
uint8_t TCPIP_Bind(uint8_t (* )(uint32_t XDATA*, uint16_t, uint16_t), void (* )(uint8_t, uint8_t), void (* )(uint8_t XDATA*, uint16_t, uint8_t));

/* for tcp */
uint8_t TCPIP_TcpListen(uint16_t, uint8_t);
uint8_t TCPIP_TcpNew(uint8_t, uint8_t, uint32_t, uint16_t, uint16_t);
void TCPIP_TcpConnect(uint8_t);
void TCPIP_TcpKeepAlive(uint8_t, uint8_t);
void TCPIP_TcpClose(uint8_t);
uint16_t TCPIP_QueryTcpLocalPort(uint8_t);
void TCPIP_TcpSend(uint8_t, uint8_t*, uint16_t, uint8_t);

/* for udp */
uint8_t TCPIP_UdpListen(uint16_t, uint8_t);
uint8_t TCPIP_UdpNew(uint8_t, uint8_t, uint32_t, uint16_t, uint16_t);
void TCPIP_UdpClose(uint8_t);
uint8_t TCPIP_QueryUdpLocalPort(uint8_t);
void TCPIP_UdpSend(uint8_t, uint8_t*,uint8_t, uint8_t*, uint16_t);

/* for other purpose */
void TCPIP_AssignLowlayerXmitFunc(void (* )(uint16_t));
void TCPIP_AssignPingRespFunc(void (* )(void));
void TCPIP_PeriodicCheck(void);
void TCPIP_SetPppoeMode(uint8_t);
uint32_t TCPIP_GetIPAddr(void);
uint32_t TCPIP_GetSubnetMask(void);
uint32_t TCPIP_GetGateway(void);
void TCPIP_SetIPAddr(uint32_t);
void TCPIP_SetSubnetMask(uint32_t);
void TCPIP_SetGateway(uint32_t);
void TCPIP_SetMacAddr(uint8_t*);
void TCPIP_Receive(void);
void TCPIP_DontFragment(uint8_t);

/* for uip */
uint16_t htons(uint16_t);

/* for uip_arp */
void uip_arp_ipin(void);
void uip_arp_arpin(void);
void uip_arp_out(void);



#define UDP_BACNET_LPORT 47808
void udp_scan_init(void);
void bip_Init(void);
void UDP_bacnet_APP(void);
/* for debug */
//void TCPIP_Debug(void);

#endif /* End of __TCPIP_H__ */


/* End of tcpip.h */
