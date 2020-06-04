#ifndef  _DEBUGWIRELESS_H_
#define _DEBUGWIRELESS_H_

#include "system.h"
#include "delays.h"
//#define ConsoleIsGetReady() (PortChReady(PC_PORT))
//#define ConsoleGet()        PortGetCh(PC_PORT)
//#define ConsolePut(ch)		if (g_UnitFlags.debugWireless) PortPutChar(ch,PC_PORT)
#define DebugWireless_PutString(msg)	if (g_UnitFlags.debugWireless) {PortPutStr((char*)msg,PC_PORT,1); DelayMsecs(10); };


//#define PrintChar(hex)		if (g_UnitFlags.debugWireless) PortPutHEXByte(hex,PC_PORT)
//#define PrintDec(ch)		if (g_UnitFlags.debugWireless) PortPutDecInt((int)ch,PC_PORT);

#endif // _DEBUGWIRELESS_H_

