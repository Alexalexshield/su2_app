/*****************************************************************************
 *
 * UART Driver mapping 
 *
 *****************************************************************************
 *****************************************************************************/
#include "port.h"
#include "uart1.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "delays.h"

#ifdef PORT_UART1		
#include "uart2.h"
unsigned int uart1_bytes_per_ms;
#endif

#ifdef PORT_UART2		
#include "uart2.h"
unsigned int uart2_bytes_per_ms;
#endif

#ifdef PORT_UART3		
#include "uart3.h"
unsigned int uart3_bytes_per_ms;
#endif

#ifdef PORT_UART4		
#include "uart4.h"
unsigned int uart4_bytes_per_ms;
#endif

#ifdef PORT_SDLOG
#include "..\SU2_APP\sdfunctions.h"

#endif

#ifdef PORT_WIRELESS
#include "wiInterface.h"
#endif

void PortDelayForBytes(int port, unsigned int bytelen)
{
	unsigned int uiDelay; 
	unsigned int bytes_per_ms = 0;
	
	switch (port) {
#ifdef PORT_UART1		
		case PORT_UART1:
			bytes_per_ms = uart1_bytes_per_ms;
			break;
#endif			
#ifdef PORT_UART2		
		case PORT_UART2:
			bytes_per_ms = uart2_bytes_per_ms;
			break;
#endif			
#ifdef PORT_UART3
		case PORT_UART3:
			bytes_per_ms = uart3_bytes_per_ms;
			break;
#endif			
#ifdef PORT_UART4
		case PORT_UART4:
			bytes_per_ms = uart4_bytes_per_ms;
			break;
#endif			
		default:
			break;
	}	

	if (bytes_per_ms != 0) {
		uiDelay = bytelen/bytes_per_ms;
		if (uiDelay == 0) uiDelay = 1;
		DelayMsecs(uiDelay);	
	}
#ifdef PORT_WIRELESS
	else if (port == PORT_WIRELESS) {
		checkFor_WirelessMessages();		// process any pending packets
	}
#endif
}

				
void PortInit(int port, long baudrate)
{
	switch (port) {
#ifdef PORT_UART1		
		case PORT_UART1:
#endif			
#ifdef PORT_UART2		
		case PORT_UART2:
#endif		
#ifdef PORT_UART3
		case PORT_UART3:
#endif		
#ifdef PORT_UART4		
		case PORT_UART4:
#endif			
		{
			unsigned char brgval;
			unsigned char bregh;
			unsigned int bytes_per_ms;
			float fbrg = (FCY/4.0)/(float)baudrate;
			long brgval1 = ((long)(fbrg + 0.5))-1;
			long error1 = ((((FCY/4.0)/(brgval1+1))- baudrate)*1000)/baudrate;
			long brgval0 = ((long)(fbrg/4.0 + 0.5))-1;
			long error0 = ((((FCY/16.0)/(brgval0+1))- baudrate)*1000)/baudrate;
		
			if (brgval1 > 255) {
				brgval = brgval0 & 0xff;
				bregh = 0;
			}
			else {
				if (labs(error1) < labs(error0)) {
					brgval = brgval1 & 0xff;
					bregh = 1;
				}
				else {
					brgval = brgval0 & 0xff;
					bregh = 0;
				}
			}	
			bytes_per_ms = (baudrate + 5000l)/10000l;		// roughly 10 bits per character x 1000 is bytes per ms
		
			if (bytes_per_ms == 0)
				bytes_per_ms = 1;
				
#ifdef PORT_UART1		
			if (port == PORT_UART1) {
				uart1_bytes_per_ms = bytes_per_ms;
				UART1Init(brgval,bregh);
			}
#endif			

#ifdef PORT_UART2		
			else if (port == PORT_UART2){
				uart2_bytes_per_ms = bytes_per_ms;
				UART2Init(brgval,bregh);
			}
#endif			

#ifdef PORT_UART3
			else if (port == PORT_UART3){
				uart3_bytes_per_ms = bytes_per_ms;
				UART3Init(brgval,bregh);
			}
#endif		

#ifdef PORT_UART4		
			else if (port == PORT_UART4){
				uart4_bytes_per_ms = bytes_per_ms;
				UART4Init(brgval,bregh);
			}
#endif		

			// It is recommended that there is at least 1 character delay before Tx on the port
			DelayMsecs(1);	
		}
		break;
	}
}		

char  PortPutChar(unsigned char Ch, int port)
{ 
	char error = 0;
	
#ifdef PORT_SDLOG
	if (port & PORT_SDLOG) {
		error += (char)(SD_Write((char*)&Ch,1,PORT_SDLOG));
	}
#endif
	
#ifdef PORT_SDEVENTLOG
	if (port & PORT_SDEVENTLOG) {
		error += (char)(SD_Write((char*)&Ch,1,PORT_SDEVENTLOG));
	}
#endif	

#ifdef PORT_UART1		
	if (port & PORT_UART1)
		error += UART1PutCh(Ch);
#endif
		
#ifdef PORT_UART2		
	if (port & PORT_UART2)
		error += UART2PutCh(Ch);
#endif

#ifdef PORT_UART3
	if (port & PORT_UART3)
		error += UART3PutCh(Ch);
#endif

#ifdef PORT_UART4
	if (port & PORT_UART4)
		error += UART4PutCh(Ch);
#endif

#ifdef PORT_WIRELESS
//	if (port & PORT_WIRELESS) {			// LIMIT ALL MESSAGES OVER WIRELESS TO STRINGS - IT IS SIMPLER THAT WAY
//		error += MiWi_PutChar(Ch);
//	}
#endif
	return error;
}	


// stuffs a character into the rx queue - mostly for testing
char PortRx_push(unsigned char Ch,int port)
{
	char error = 0;
#ifdef PORT_UART2		
	if (port & PORT_UART1)
		error+= UART1Rx_push(Ch);
#endif
		
#ifdef PORT_UART2		
	if (port & PORT_UART2)
		error+= UART2Rx_push(Ch);
#endif		

#ifdef PORT_UART3
	if (port & PORT_UART3)
		error+= UART3Rx_push(Ch);
#endif		
		
#ifdef PORT_UART4
	if (port & PORT_UART4)
		error+= UART4Rx_push(Ch);
#endif	
	
#ifdef PORT_WIRELESS
	if (port & PORT_WIRELESS) {
		error+= MiWi_rx_enque((unsigned char*)&Ch,1);
	}
#endif
	return error;		
}	

// stuffs a buffer of characters into the rx queue - mostly for testing
char PortRx_pushBytes(char* p,int len, int port)
{
	int i;
	int error = 0;
	for (i=0;i<len;i++) {
		error += PortRx_push(*p++,port);
	}
	if (error)
		return 1;
	else
		return 0;
}

unsigned int PortGetTxBufSize(int port)
{ 
#ifdef PORT_UART2		
	if (port == PORT_UART1)
		return UART1GetTxBufSize();
#endif		
		
#ifdef PORT_UART2		
	if (port == PORT_UART2)
		return UART2GetTxBufSize();
#endif		

#ifdef PORT_UART3
	if (port == PORT_UART3)
		return UART3GetTxBufSize();
#endif		
		
		
#ifdef PORT_UART4
	if (port == PORT_UART4)
		return UART4GetTxBufSize();
#endif	
	
#ifdef PORT_WIRELESS
//	if (port == PORT_WIRELESS) {				// WE'RE NOT QUEUING IN WIRELESS TX MODE
//		return MiWi_GetTxBufFreeSize();
//	}
#endif
	return 10000;	
}

char PortChReady(int port)
{
#ifdef PORT_UART1		
	if (port == PORT_UART1)
		return UART1ChReady();
#endif
		
#ifdef PORT_UART2		
	if (port == PORT_UART2)
		return UART2ChReady();
#endif		

#ifdef PORT_UART3
	if (port == PORT_UART3)
		return UART3ChReady();
#endif

#ifdef PORT_UART4
	if (port == PORT_UART4)
		return UART4ChReady();
#endif		

#ifdef PORT_WIRELESS
	if (port == PORT_WIRELESS) {
		return MiWi_isCharReady();
	}
#endif
	return 0;
}		

// gets next available UART character
// make sure you check that a char is ready before calling otherwise the return is 0
unsigned char PortGetCh(int port)
{ 
#ifdef PORT_UART1		
	if (port == PORT_UART1)
		return UART1GetCh();
#endif
		
#ifdef PORT_UART2		
	if (port == PORT_UART2)
		return UART2GetCh();
#endif		

#ifdef PORT_UART3
	if (port == PORT_UART3)
		return UART3GetCh();
#endif

#ifdef PORT_UART4
	if (port == PORT_UART4)
		return UART4GetCh();
#endif		
#ifdef PORT_WIRELESS
	if (port == PORT_WIRELESS) {
		return MiWi_GetChar();
	}
#endif

	return 0xff;
}		

/*****************************************************************************
 *
 * Overview: This function converts decimal data into a string and outputs it into UART.
 *
 *****************************************************************************/
void PortPutNoRetryStr(char *pstr,int port)
{
#ifdef PORT_SDLOG
	if (port & PORT_SDLOG) {
		SD_Write((char*)pstr,strlen(pstr),PORT_SDLOG);
		port &= ~PORT_SDLOG;
	}
#endif

#ifdef 	PORT_SDEVENTLOG
	if (port & PORT_SDEVENTLOG) {
		SD_Write((char*)pstr,strlen(pstr),PORT_SDEVENTLOG);
		port &= ~PORT_SDEVENTLOG;
	}
#endif

#ifdef PORT_WIRELESS
	if (port & PORT_WIRELESS) {
		SendWirelessTextMsg(pstr);
		port &= ~PORT_WIRELESS;
	}
#endif

	while (*pstr) {
		PortPutChar(*pstr++,port);
	}
}

void  UARTPutDec100(unsigned int Dec,int port)
{
	char msgbuf[10];
	char *p = msgbuf;
	*p++ = ',';
	
	if (Dec/100 != 0) {
		*p++ = Dec/100 + '0';
		Dec = Dec%100;
		*p++ = Dec/10 + '0';
		Dec = Dec%10;
	}	
	else if (Dec/10 != 0) {
		*p++ = Dec/10 + '0';
		Dec = Dec%10;
	}	
	*p++ = Dec%10 + '0';
	*p = 0;
	PortPutNoRetryStr(msgbuf,port);
}

// this is faster than sprintf
void  PortPutDecInt(unsigned int Dec,int port)
{
	char msgbuf[10];
	char *p = msgbuf;
	*p++ = ',';
	char digit;
	char nonzero = 0;
	unsigned int divider = 10000;

	while (divider) {
		digit = Dec/divider + '0';
		if ((digit != '0') || (divider == 1)) nonzero = 1;
		if (nonzero)
			*p++ = digit;
		Dec = Dec%divider;
		divider/= 10;
	}
	*p = 0;
	
	PortPutNoRetryStr(msgbuf,port);
}

void PortPutDec(unsigned int Dec,int port)
{
	char msgbuf[10];
	sprintf(msgbuf,"%04d ",Dec);
	PortPutNoRetryStr(msgbuf,port);
}

void PortPutHEXByte(unsigned int value,int port)
{
	char nibble = (value >> 4) & 0x000f;
	PortPutChar((nibble < 10)?(nibble+'0'):(nibble-10+'a'),port);
	nibble = (value & 0x000f);
	PortPutChar((nibble < 10)?(nibble+'0'):(nibble-10+'a'),port);
}
void PortPutHex(unsigned int value,int port)
{
	char msgbuf[10];
	sprintf(msgbuf,"%x ",value);
	PortPutNoRetryStr(msgbuf,port);
}

void PortPutLHex(unsigned long value,int port)
{
	char msgbuf[20];
	sprintf(msgbuf,"%08lx ",value);
	PortPutNoRetryStr(msgbuf,port);
}	

void PortPutStr(char *pstr,int port,char max_retry)
{
#ifdef PORT_SDLOG
	if (port & PORT_SDLOG) {
		SD_Write((char*)pstr,strlen(pstr),PORT_SDLOG);
		port &= ~PORT_SDLOG;
	}
#endif
	
#ifdef PORT_WIRELESS
	if (port & PORT_WIRELESS) {
		SendWirelessTextMsg(pstr);
		port &= ~PORT_WIRELESS;
	}
#endif

#ifdef PORT_SDEVENTLOG
	if (port & PORT_SDEVENTLOG) {
		SD_Write((char*)pstr,strlen(pstr),PORT_SDEVENTLOG);
		port &= ~PORT_SDEVENTLOG;
	}
#endif
	
	int bytelen = strlen(pstr) + 2;		// 2 extra bytes for the usual cr lf
	char retry = 0; 
	unsigned int freebytes;
	
	do {
		freebytes = PortGetTxBufSize(port);
		if (freebytes >= bytelen) {
			PortPutNoRetryStr(pstr,port);
			retry = 0;
		}
		else {
			retry++;
			if (retry == max_retry)
				break;
			PortDelayForBytes(port,bytelen);
		}
	}
	while (retry);
	if (retry) {
#ifdef PORT_UART1		
		if (port & PORT_UART1)
			g_SysError.TX1_Overun = 1;
#endif
			
#ifdef PORT_UART2		
		if (port & PORT_UART2)
			g_SysError.TX2_Overun = 1;
#endif			

#ifdef PORT_UART3
		if (port & PORT_UART3)
			g_SysError.U3_TXError = 1;
#endif
			
#ifdef PORT_UART4
		if (port & PORT_UART4)
			g_SysError.U4_TXError = 1;
#endif	
		
	}
}

void PortPutCRStr(char *pstr,int port,char max_retry)
{
#ifdef PORT_WIRELESS
	if (port & PORT_WIRELESS) 
	{
		SendWirelessTextCRMsg(pstr);
		port &= ~PORT_WIRELESS;
	}
#endif
	
	PortPutStr(pstr,port,max_retry);
	PortPutStr("\r\n",port,max_retry);
}	

void PortPutInt16(unsigned int value16,int port) 
{
	PortPutChar((unsigned char)(value16 >> 8),port);
	PortPutChar((unsigned char)(value16 &  0x00ff),port);
}

void PortPutBytes(unsigned char* pbytes, unsigned char len,int port) 
{
	while (len-- > 0) {
		PortPutChar(*pbytes++,port);
	}
}

void PortPutErrMsg(char *msg,int err,int port)
{
	char msgbuf[20];
	PortPutStr(msg,port,1);
	sprintf(msgbuf," Err=%04x",err);
    PortPutCRStr(msgbuf,port,1);
}

void FlushPort(int port)
{
	while (PortChReady(port)) {
		PortGetCh(port);
	}
}

int GetPortRXpCent(int port)
{
	int peak=0;
#ifdef PORT_UART1
	if (port == PORT_UART1) {
		return GetUART1RXpCent();
	}
#endif
	
#ifdef PORT_UART2		
	if (port == PORT_UART2) {
		return GetUART2RXpCent();
	}
#endif	

#ifdef PORT_UART3
	if (port == PORT_UART3) {
		return GetUART3RXpCent();
	}
#endif

#ifdef PORT_UART4
	if (port == PORT_UART4) {
		return GetUART4RXpCent();
	}
#endif
	
#ifdef PORT_WIRELESS
	if (port == PORT_WIRELESS) {
		return Get_MiWi_RXpCent();
	}
#endif	
	return peak;
}

int GetPortTXpCent(int port)
{
	int peak=0;
#ifdef PORT_UART1		
	if (port == PORT_UART1) {
		return GetUART1TXpCent();
	}
#endif
	
#ifdef PORT_UART2		
	if (port == PORT_UART2) {
		return GetUART2TXpCent();
	}
#endif	

#ifdef PORT_UART3
	if (port == PORT_UART3) {
		return GetUART3TXpCent();
	}
#endif
	
#ifdef PORT_UART4		
	if (port == PORT_UART4) {
		return GetUART4TXpCent();
	}
#endif	
	return peak;
}

void ResetPortPeaks(int port)
{
#ifdef PORT_UART1		
	if (port & PORT_UART1) {
		ResetUART1peaks();
	}
#endif
	
#ifdef PORT_UART2		
	if (port & PORT_UART2) {
		ResetUART2peaks();
	}
#endif	

#ifdef PORT_UART3
	if (port & PORT_UART3) {
		ResetUART3peaks();
	}
#endif

#ifdef PORT_UART4		
	if (port & PORT_UART4) {
		ResetUART4peaks();
	}
#endif
	
#ifdef PORT_WIRELESS
	else if (port & PORT_WIRELESS) {
		ResetMiWipeaks();
	}
#endif	

}


/*****************************************************************************
 * EOF
 *****************************************************************************/
