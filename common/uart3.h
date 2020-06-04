/*****************************************************************************/
// * UART Driver for PIC24.
//*****************************************************************************
#include "system.h"

extern void UART3Init(unsigned char brgval,unsigned char bregh);// Initializes the Uart1
extern char UART3PutCh(unsigned char Ch);//  Wait for free UART transmission buffer and send a byte.
extern char UART3ChReady(); // returns 1 if a new character is ready
extern unsigned char UART3GetCh();// Wait for a byte.
extern unsigned int UART3GetTxBufSize();
char UART3Rx_push(unsigned char ch);
int GetUART3RXpCent();
int GetUART3TXpCent();
void ResetUART3peaks();

/*****************************************************************************
 * EOF
 *****************************************************************************/
