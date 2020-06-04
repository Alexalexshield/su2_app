#ifndef __TRACE_H
#define __TRACE_H
#include "system.h"
#include "port.h"

#ifdef SU_APP
#define DEBUG_ON ((g_UnitFlags.DebugSD) || (g_UnitFlags.DebugMisc))
#else
#define DEBUG_ON (g_UnitFlags.DebugMisc)
#endif

#define traceS(msg) if (DEBUG_ON) PortPutCRStr(msg,PC_PORT,1)
#define traceErr(msg,err) if (DEBUG_ON) PortPutErrMsg(msg,PC_PORT,err)
#define traceS2(msg,serr) if (DEBUG_ON) PortPutStr(msg,PC_PORT,1); if (DEBUG_ON) PortPutCRStr(serr,PC_PORT,1)
#define traceHex(value) if (DEBUG_ON) PortPutHex(value,PC_PORT)
#define traceC(ch) if (DEBUG_ON) PortPutChar(ch,PC_PORT)

#endif // __TRACE_H

