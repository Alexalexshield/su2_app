#ifndef __UART1_H
#define __UART1_H

/*****************************************************************************/
// * UART Driver for PIC24.
//*****************************************************************************
#include "system.h"

extern void UART1Init(unsigned char brgval,unsigned char bregh);// Initializes the Uart1
extern char UART1PutCh(unsigned char Ch);//  Wait for free UART transmission buffer and send a byte.
extern char UART1ChReady(); // returns 1 if a new character is ready
extern unsigned char UART1GetCh();// Wait for a byte.
extern unsigned int UART1GetTxBufSize();
char UART1Rx_push(unsigned char ch);
int GetUART1RXpCent();
int GetUART1TXpCent();
void ResetUART1peaks();

/*****************************************************************************
 * EOF
 *****************************************************************************/

#endif // __UART1_H

