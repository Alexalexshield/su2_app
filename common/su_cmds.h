/**************************************************************************************************

**************************************************************************************************/

#ifndef SU_CMDS_H
#define SU_CMDS_H

/*************************************************************************************************/
/************************************* Included Files ********************************************/
/*************************************************************************************************/

#include "system.h"

#ifdef SU_TX
extern void Process_SU_Wireless_Stream();
extern void Init_SU_Wireless_Stream();
#endif

extern void Init_SU_Uart_Stream();
extern void Process_SU_Uart_Stream();
extern U8 g_protocol;	// this holds the last protocol 

#endif // SU_CMDS_H
