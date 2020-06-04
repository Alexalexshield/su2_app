#ifndef __PORT_H
#define __PORT_H

/*****************************************************************************
 *
 * Port Driver mapping function prototypes.
 *
 *****************************************************************************
 *****************************************************************************/
#define PORT_NONE 0
#ifdef SU_APP
#define PORT_UART1 		0x0001
#define PORT_UART2 		0x0002
#define PORT_UART3 		0x0004
#define PORT_UART4 		0x0008
#define PORT_SDLOG 		0x0010
#define PORT_WIRELESS   0x0020

#define PC_PORT PORT_UART1
#define LCD_PORT PORT_UART2
#define SUTX_UART_PORT PORT_UART3
#define SURX_UART_PORT PORT_UART4

#endif

#ifdef SU_TX
#define PORT_UART1 		0x0001
#define PORT_UART2 		0x0002
#define PORT_WIRELESS   0x0020
#define PC_PORT PORT_UART1
#define SU_PORT PORT_UART2
#endif


#ifdef SU_RX
#define PORT_UART1 		0x0001
#define PORT_UART2 		0x0002
#define PC_PORT PORT_UART1
#define SU_PORT PORT_UART2
#endif

#ifdef MU_APP
#define PORT_NONE 0
#define PORT_UART1 		0x0001
#define PC_PORT PORT_UART1
#endif

extern void PortInit(int port, long baudrate);			// Initializes the Uart
extern char PortPutChar(unsigned char Ch, int port); //  Wait for free UART transmission buffer and send a byte.
extern char PortChReady(int port);
extern unsigned char PortGetCh(int port);	// Get next available byte
extern void PortPutDec100(unsigned int Dec,int port);// This function converts decimal data into a string and outputs it into UART.
extern void PortPutDecInt(unsigned int Dec,int port);// This function converts decimal data into a string and outputs it into UART.
extern void PortPutDec(unsigned int Dec,int port);// This function converts decimal data into a string and outputs it into UART.
extern void PortPutStr(char* pstr,int port,char max_retry);
extern void PortPutCRStr(char* pstr,int port,char max_retry);
extern void PortPutInt16(unsigned int value16,int port);
extern void PortPutBytes(unsigned char* pbytes, unsigned char len,int port);
extern unsigned int PortGetTxBufSize(int port);
extern char PortRx_push(unsigned char ch,int port);
extern char PortRx_pushBytes(char* p,int len, int port);
extern void PortPutErrMsg(char *pstr,int error,int port);
extern void PortPutHex(unsigned int Hex,int port);
extern void PortPutLHex(unsigned long value,int port);
void PortPutHEXByte(unsigned int value,int port);

extern void PortDelayForBytes(int port, unsigned int bytelen);
extern void FlushPort(int port);
extern int GetPortRXpCent(int port);
extern int GetPortTXpCent(int port);
extern void ResetPortPeaks(int port);


/*****************************************************************************
 * EOF
 *****************************************************************************/

#endif // __PORT_H

