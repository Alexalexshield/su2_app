#ifndef __UART2_H
#define __UART2_H

//*****************************************************************************
// uart2 prototypes
//
// *****************************************************************************/
#include "system.h"

extern void UART2Init(unsigned char brgval,unsigned char bregh);
extern char  UART2PutCh(unsigned char Ch);
extern char UART2ChReady();
extern unsigned char UART2GetCh();
extern unsigned int UART2GetTxBufSize();
char UART2Rx_push(unsigned char ch);
int GetUART2RXpCent();
int GetUART2TXpCent();
void ResetUART2peaks();
/*****************************************************************************
 * EOF
 *****************************************************************************/

#endif // #ifndef __UART2_H

