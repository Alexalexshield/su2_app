#include "system.h"
#include "stdio.h"
#include "putfunctions.h"
#include "string.h"
#include "port.h"

#ifdef MU_APP
#include "adxl345.h"
#include "..\mu2_app\vlf_send.h"
#endif

#ifdef SU_APP
#if (RTCC_SOURCE == RTCC_INTERNAL)	
#include "rtcc.h"
#elif (RTCC_SOURCE == RTCC_EXTERNAL)	
#include "i2c_ISL12024_rtc.h"
#endif
//#include "sdfunctions.h"
#endif


char g_strbuffer[256];



void PutVersion(char *msg, int port)
{
	sprintf(g_strbuffer,"$%sVER,%s,%s,ID=%lu,S=%08lX,%s",SOURCE_PREFIX,MICRO_ID_STR,SW_VERSION_STR,g_Config.id,g_SysError.value,msg);
	PortPutCRStr(g_strbuffer,port,1);
}

void PutStartupMsg(BootStatus* p, int port) 
{
	sprintf(g_strbuffer,"$%sRST,%s,%s,ID=%lu,RC=%04x,INT=%04x",
		SOURCE_PREFIX,
		MICRO_ID_STR,
		SW_VERSION_STR,
		g_Config.id,
		p->RCON,
		p->INTCON1
	);
	PortPutCRStr(g_strbuffer,port,1);
}
 		

/*
void PutTestMsg(char *msg, int port, long bps, long pps)
{
	sprintf(g_strbuffer,"$%sMSG,%s,%s,%s",SOURCE_PREFIX,MICRO_ID_STR,SW_VERSION_STR,msg);
	PortPutStr(g_strbuffer,port,2);
	sprintf(g_strbuffer,",UTC,%s",asctime(gmtime((const time_t * )&g_seconds)));
	g_strbuffer[29] = ' '; 	// remove the linefeed produced by asctime
	PortPutStr(g_strbuffer,port,2);
	sprintf(g_strbuffer,",Status=%08lX,",g_SysError.value);
	PortPutStr(g_strbuffer,port,2);
	sprintf(g_strbuffer,",%s: BPS=%ld",MICRO_ID_STR,bps);
	PortPutStr(g_strbuffer,port,2);
	sprintf(g_strbuffer,",PPS=%ld",pps);
	PortPutStr(g_strbuffer,port,2);
	sprintf(g_strbuffer,",%s: RX=%d%% TX=%d%%","PC",GetPortRXpCent(port),GetPortTXpCent(port));
	PortPutCRStr(g_strbuffer,port,2);
}
*/

char* BuildTimeString() 
{
	static char strbuffer[40];

#ifdef SU_APP
 	
#if (RTCC_SOURCE == RTCC_INTERNAL)	
 	g_seconds = RTCCGetTime_t();
#else
	ISL_ReadTime(&g_seconds);
#endif	
	
#endif 	
	strftime(strbuffer, sizeof(strbuffer), "%Y-%m-%d,%H:%M:%S", gmtime((const time_t * )&g_seconds));

	return strbuffer;
}

void PutTimeStr(int port)
{
//#ifdef SU_APP
//#if (RTCC_SOURCE == RTCC_INTERNAL)
//	RTCCProcessEvents();		// read the clock
//#else
//	ISL_ReadTime(&g_seconds);
//#endif
//#endif 	
	sprintf(g_strbuffer,"$%sTIM,%s\r\n", SOURCE_PREFIX,BuildTimeString());
	PortPutStr(g_strbuffer,port,1);
}	


#ifndef MU_APP
void PutBatteryVoltage(int mvolts, int port)
{
	sprintf(g_strbuffer,"$%sBMV,%d\r\n",SOURCE_PREFIX,mvolts);
	PortPutStr(g_strbuffer,port,1);
}	
#endif

void PutEcho(char rxbyte, int port)
{
	sprintf(g_strbuffer,"$%sECO,%c\r\n",SOURCE_PREFIX,rxbyte);
	PortPutStr(g_strbuffer,port,1);
}	

void PutConfigData(int source,int port)
{
	Config tempConfig;
	Config* pConfig;
	char error = 0;
	
	if (source == CONFIG_SOURCE_FLASH) {
		pConfig = &tempConfig;
		error = load_config(pConfig);
	}
	else {
		pConfig =&g_Config;
	}
		
	if (error) {
		PutLastEepromErrorMsg(" Read from Flash failed ", port);
		pConfig =&g_Config;
	}

	sprintf(g_strbuffer,"$%sCFG,%ld",SOURCE_PREFIX,pConfig->id);
	PortPutStr(g_strbuffer, port,1);
	PortPutStr("\r\n",port,1);

}



void PutEepromErrorMsg(char *msg, DATA_EE_FLAGS flags, int port)
{
	sprintf(g_strbuffer,"$%sMSG,",SOURCE_PREFIX);
	PortPutStr(g_strbuffer, port,1);
	if ((msg != NULL) && (msg != ""))
		PortPutStr(msg, port,1);
	if (flags.val == 0) {
		PortPutStr(",No EEFlags set\r\n",port,1);
	}
	else {		
		if (flags.addrNotFound)
			PortPutStr(",addrNotFound", port,1);
		if (flags.expiredPage)
			PortPutStr(",expiredPage", port,1);
		if (flags.packBeforePageFull)
			PortPutStr(",packBeforePageFull", port,1);
		if (flags.packBeforeInit)
			PortPutStr(",packBeforeInit", port,1);
		if (flags.packSkipped)
			PortPutStr(",packSkipped", port,1);
		if (flags.IllegalAddress)
			PortPutStr(",IllegalAddress", port,1);
		if (flags.pageCorrupt)
			PortPutStr(",pageCorrupt", port,1);
		if (flags.writeError)
			PortPutStr(",writeError", port,1);
		PortPutStr("\r\n",port,1);
	}	
}

void PutLastEepromErrorMsg(char *msg, int port)
{
	PutEepromErrorMsg(msg,dataEEFlags,port);
}


#ifdef SU_APP
void PutButtonState(unsigned short state, int port)
{
	sprintf(g_strbuffer,"$SUBTN,%04X",state);
	PortPutCRStr(g_strbuffer,port,1);
}
#endif

#ifdef MU_APP

void PutAccelStr(int port)
{
	struct ACCEL_TABLE * pAccel = &g_accel_readings;
	
	sprintf(g_strbuffer,"$%sACC",SOURCE_PREFIX);
	PortPutStr(g_strbuffer, port,1);
		
	sprintf(g_strbuffer,",%+5d,%+5d,%+5d",
		pAccel->ref.x,
		pAccel->ref.y,
		pAccel->ref.z
		);
	PortPutStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,",%+5d,%+5d,%+5d",
		pAccel->min.x - pAccel->ref.x,
		pAccel->min.y - pAccel->ref.y,
		pAccel->min.z - pAccel->ref.z
		);
	PortPutStr(g_strbuffer,port,1);

	sprintf(g_strbuffer,",%+5d,%+5d,%+5d",
		pAccel->max.x - pAccel->ref.x,
		pAccel->max.y - pAccel->ref.y,
		pAccel->max.z - pAccel->ref.z
		);
	PortPutStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,",%8.2f M=%d", (float)GetLastFilteredMaxAcceleration(), GetMobility());
	PortPutStr(g_strbuffer,port,1);
	PortPutStr("\r\n",port,1);
}

#endif



