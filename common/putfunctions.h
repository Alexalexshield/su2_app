#ifndef __PUTFUNCTIONS_H
#define __PUTFUNCTIONS_H
/**********************************************************************
*
**********************************************************************/
#include "su_struct.h"
#include "config.h"
#include "DEE Emulation 16-bit.h"

extern char g_strbuffer[256];	// global string buffer used by most put functions
	
void PutStartupMsg(BootStatus* p, int port); 
extern void PutTestMsg(char *msg, int port, long bps, long pps);
extern void PutTimeStr(int port);
void PutVersion(char *msg, int port);
extern void PutEcho(char rxbyte, int port);

extern void PutConfigData(int source,int port);
void PutLastEepromErrorMsg(char *msg,int port);
void PutEepromErrorMsg(char *msg, DATA_EE_FLAGS flags, int port);
void PutUnitFlags(int port);
void PutMemoryProperties(int port);
void PutFileNumCopied(unsigned long fcopied,int port);
void PutFileToCopy(long count, char *dir, char* name,int port);
void PutFileNumber(long count, unsigned long lindex, int port);
void PutBatteryVoltage(int mvolts, int port);
void PutButtonState(unsigned short state, int port);
char* BuildTimeString();

#ifdef MU_APP
void PutAccelStr(int port);
#endif

#endif // __PUTFUNCTIONS_H
