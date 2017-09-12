
#include"../tcpip/tcpip.h"
/*******************************************************************************
* Function Name  : TCPIP_Init
* Description    : Initialize the TCPIP module
* @param  mode: where mode can be
*   - 0 : non-transparent mode
*   - 1 : transparent mode 
* Return         : None.
*******************************************************************************/
void TCPIP_Init(u8 mode)
{
	

}


/*******************************************************************************
* Function Name  : TCPIP_Bind
* Description    : Initialize the TCPIP module

* @param  
U8_T (*newConnHandle)(U32_T XDATA* pIP, U16_T remotePort, U8_T socket)
*   - pIP 			: A pointer to the IP address of remote host.
*   - remotePort 	: The port number of remote host. 
*   - socket 		: The socket identification of pending connection.

void (*eventHandle)(U8_T id, U8_T event)
*   - id 			: The socket identification of connection.
*   - event 		: The event identification. Description of each event identification is listed as follows: 
#define INTERFACE_CONNECT_CANCEL 			0
#define INTERFACE_CONNECT_ACTIVE 			2
#define INTERFACE_CONNECT_XMIT_COMPLETE  	3
#define TCPIP_CONNECT_BUSY 					0xF1

void (*rcvHandle)(U8_T XDATA* pbuf, U16_T length, U8_T id)
*	-  pbuf  			A pointer to the buffer of received data.
*	-  length  			The length of data in bytes.
*	-  id  				The connection identification of upper layer module 

* Return  : 
The interface identification, or TCPIP_NO_NEW_CONN if a new connection is not available.
*******************************************************************************/
U8_T TCPIP_Bind(U8_T (*newConnHandle)(U32_T *, U16_T, U8_T), void (*eventHandle)(U8_T, U8_T), void (*rcvHandle)(U8_T XDATA*, U16_T, U8_T))
{
	
}
/*******************************************************************************
* Function Name  : TCPIP_TcpListen
* Description    : This function is usually called by the serverfunction of upper layer module to listen 
on a specified port for incoming connections.

* @param  
U8_T (*newConnHandle)(U32_T XDATA* pIP, U16_T remotePort, U8_T socket)
*   - localPort 			: The port number of the local host.
*   - interfaceId 			: The interface identification of the upper layer module. 

Return value: 
1 if the function is successful; otherwise 0. 
*/
U8_T TCPIP_TcpListen(U16_T port, U8_T interface_id)
{

}
/*******************************************************************************
* Function Name  : TCPIP_TcpNew
* Description    : The upper layer module calls this function to create a new TCP socket from TCPIP 
module. TCPIP module can create up to 8 TCP sockets.

* @param  
*   - interfaceId 			: The interface identification of the upper layer module. 
*   - applicationId 		: The connection identification in the upper layer module. 
*   - remoteIp 				: The IP address of the remote host, or 0 if unknown
*   - localPort 			: The port number of the local host, or 0 if unknown. 
*   - remotePort 			: The port number of the remote host, or 0 if unknown. 

Return value: 
The socket identification if the function is successful; otherwise the value of 
TCPIP_NO_NEW_CONN is returned.
*/
U8_T TCPIP_TcpNew(U8_T interfaceId, U8_T applicationId, U32_T remoteIp, U16_T localPort, U16_T remotePort)
{
	
}


/*******************************************************************************
* Function Name  : TCPIP_TcpConnect
* Description    : This function is called to establish a connection to a specified socket. If the 
connection is established successfully, TCPIP module will send the event of 
INTERFACE_CONNECT_ACTIVE tothe upper layer module

* @param  
*   - tcpSocket 			: The socket identification of connection. 
Return value: NULL
*/
void TCPIP_TcpConnect(U8_T tcpSocket)
{
	
}
/*******************************************************************************
* Function Name  : TCPIP_TcpKeepAlive
* Description    : This function is called to enable or disable the TCP Keep Alive timer. If the timer is 
disabled, TCPIP module will close any TCP connection that idles over 8 minutes 
timer. If the timer is enabled, TCPIP module sends periodically a special ACK 
packet to keep an idle TCP session alive

* @param  
*   - tcpSocket 			: The socket identification of connection. 
*   - flag 					
	  TCPIP_KEEPALIVE_ON		1 	
	  TCPIP_KEEPALIVE_OFF		0	
Return value: NULL
*/
void TCPIP_TcpKeepAlive(U8_T tcpSocket, U8_T flag)
{
	
}
/*******************************************************************************
* Function Name  : TCPIP_TcpClose
* Description    : This function is called toclose a TCP connection.
* @param  
*   - tcpSocket 			: The socket identification of connection. 
Return value: NULL
*/
void TCPIP_TcpClose(U8_T  tcpSocket)
{
	
}
/*******************************************************************************
* Function Name  : TCPIP_QueryTcpLocalPort
* Description    : This function is called to get the local port number of a TCP connection. 
* @param  
*   - tcpSocket 			: The socket identification of connection. 
Return value: NULL
*/
U16_T TCPIP_QueryTcpLocalPort(U8_T tcpSocket)
{

}
/*******************************************************************************
* Function Name  : TCPIP_TcpSend
* Description    : This function is called to get the local port number of a TCP connection. 
* @param  
*   - tcpSocket 			: The socket identification of connection.
*   - pbuf		 			: The socket identification of connection. 
*   - length 				: The socket identification of connection. 
*   - flag 					: The socket identification of connection. 
	TCPIP_SEND_NOT_FINAL	0		 
	TCPIP_SEND_FINAL 		1
	TCPIP_SEND_NOT_PUSH 	2 
Return value: NULL
*/
void TCPIP_TcpSend(U8_T tcpSocket, U8_T*pbuf, U16_T length, U8_T flag)
{
	
}