/*****************************************************************************/
// * UART Driver for PIC24.
//*****************************************************************************
#include "system.h"

extern void UART4Init(unsigned char brgval,unsigned char bregh);// Initializes the Uart1
extern char UART4PutCh(unsigned char Ch);//  Wait for free UART transmission buffer and send a byte.
extern char UART4ChReady(); // returns 1 if a new character is ready
extern unsigned char UART4GetCh();// Wait for a byte.
extern unsigned int UART4GetTxBufSize();
char UART4Rx_push(unsigned char ch);
int GetUART4RXpCent();
int GetUART4TXpCent();
void ResetUART4peaks();

/*****************************************************************************
 * EOF
 *****************************************************************************/
