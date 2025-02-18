#include "system.h"
#include "pc_cmds.h"
#include <string.h>
#include <math.h> 
#include <stdlib.h>
#include "stdio.h"
#include "putfunctions.h"
#include "trace.h"

#ifdef SU_APP

#include "..\SU2_APP\sdfunctions.h"
#if (BOARD_REV == 2)
#include "i2c_ISL12024_rtc.h"
#endif

#endif


#define MAX_PC_PACKET_SIZE 550
unsigned char process_PC_TextCommands(unsigned char rxbyte, int source_port);
unsigned char pc_Uart_cmdbuf[MAX_PC_PACKET_SIZE+2];		// this should be the maximum of several command structures
struct CSV_PACKET_HANDLER g_pc_Uart_Packet;


void InitPCStream()
{
	// init the comma seperated $SU......\r command handler for special commands via the pc uart
	init_CSV_packethandler(&g_pc_Uart_Packet,pc_Uart_cmdbuf,MAX_PC_PACKET_SIZE);

}


// processes a csv stream from the PC - InitPCStream() must be called first


void ProcessStream( struct CSV_PACKET_HANDLER* pCSV, int port)
{
	unsigned char rxbyte;
				
	while (PortChReady(port)) {
		rxbyte = PortGetCh(port); 
		if ((rxbyte !=0)) {
			rxbyte = process_CSV_packetbyte(pCSV, rxbyte);
			if (pCSV->packet_timeouts > 0) {
				g_SysError.U2_PacketTimeout = 1;
				pCSV->packet_timeouts=0;
			}
			if (pCSV->m_PacketStage == CSV_PACKET_READY) {
				Process_PC_CSV_Packet(pCSV, port); 
				reset_CSV_packethandler(pCSV);
			}
			if ((rxbyte !=0) && (rxbyte != CSV_FRAMECHAR)) {
				rxbyte = process_PC_TextCommands(rxbyte, port);		// process any leftover text commands
			}
		}
	}
}

void ProcessPCStream()
{
	ProcessStream( &g_pc_Uart_Packet, PC_PORT);
}


// converts an ascii hex string to int
unsigned int axtoi(char* hexStg)
{
	int n  = 0;		// position in string
    int m = 0;		// position in digit[] to shift
    int	count;		// loop index
    int	intValue = 0;		// integer value of hex string
    int	digit[32];		// hold values to convert

    while (n < 32)
    {
		if (hexStg[n] == '\0')
			break;
		if (hexStg[n] >= '0' && hexStg[n] <= '9')	//if 0 to 9
			digit[n] = hexStg[n] & 0x0f;	//convert to int
		else if (hexStg[n] >= 'a' && hexStg[n] <= 'f')	//if a to f
			digit[n] = (hexStg[n] & 0x0f) + 9;	//convert to int
		else if (hexStg[n] >= 'A' && hexStg[n] <= 'F')	//if A to F
			digit[n] = (hexStg[n] & 0x0f) + 9;	//convert to int
		else
			break;
		n++;
    }
    count = n;
    m = n - 1;
    n = 0;
    while (n < count)
    {
	// digit[n] is value of hex digit at position n
	// (m << 2) is the number of positions to shift
	// OR the bits into return value
		intValue = intValue | (digit[n] << (m << 2));
		m--;			// adjust the position to set
		n++;			// next digit to process
    }
    return (intValue);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// SU_APP /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SU_APP
void init_unitflags()
{
	g_UnitFlags.value = 0;
	g_UnitFlags.LogStatus = 1;
	g_UnitFlags.AutoLCDPowerOff = 1;
} 

void PutHelp(int port)
{
	PutUnitFlags(port);
	PortPutCRStr("...\r\nSU_APP help menu:",port,1);
	sprintf(g_strbuffer,"(%c) GET_FLASH_CONFIG",GET_FLASH_CONFIG);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GET_RAM_CONFIG",GET_RAM_CONFIG);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GET_HELP",GET_HELP);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) MEM_PROPERTIES",MEM_PROPERTIES);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SWRESET",SWRESET);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GETVERSION",GETVERSION);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SD_WRITETEST",SD_WRITETEST);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SD_READTEST",SD_READTEST);
	PortPutCRStr(g_strbuffer,port,1);

}

void PutUnitFlags(int port)
{
	PortPutCRStr("...\r\nSU_APP FLAGS:",port,1);
	sprintf(g_strbuffer,"$%sFLG,%04X",SOURCE_PREFIX,g_UnitFlags.value);
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) LogStatus=%s",TOGGLE_LOG_STATUS,g_UnitFlags.LogStatus?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SendBattMV=%s",TOGGLE_SENDBATT,g_UnitFlags.SendBattMV?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) DebugMisc=%s",TOGGLE_DEBUG_MISC,g_UnitFlags.DebugMisc?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SendLog=%s",TOGGLE_SEND_LOG,g_UnitFlags.SendLog?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) SendSUMsgs=%s",TOGGLE_SEND_SU_MSGS,g_UnitFlags.SendSUMsgs?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SendMUMsgs=%s",TOGGLE_SEND_MU_MSGS,g_UnitFlags.SendMUMsgs?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) AutoLCDPowerOff=%s",TOGGLE_AUTOLCDSHUTDOWN,g_UnitFlags.AutoLCDPowerOff?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) DebugSD=%s",TOGGLE_DEBUG_SD,g_UnitFlags.DebugSD?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SendTime=%s",TOGGLE_SENDTIME,g_UnitFlags.SendTime?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) DebugSlcd=%s",TOGGLE_DEBUG_SLCD,g_UnitFlags.DebugSLCD?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) DebugButtons=%s",TOGGLE_DEBUG_BUTTONS,g_UnitFlags.DebugButtons?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) DebugWireless=%s",TOGGLE_DEBUGWIRELESSS,g_UnitFlags.debugWireless?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	
}

void Process_PC_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[20];
	if (strcmp((char*)phandler->header,"$SUTIM") == 0) {
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer) == 6) {
				char hour,min,sec,day,month,year;
				hour = (buffer[0]-'0')*10 + buffer[1]-'0';
				min = (buffer[2]-'0')*10 + buffer[3]-'0';
				sec = (buffer[4]-'0')*10 + buffer[5]-'0';
				if (GetNthCSV((char*)phandler->pbuf, 2, buffer, 20) == 0) {
					if (strlen(buffer) == 6) {
						day = (buffer[0]-'0')*10 + buffer[1]-'0';
						month = (buffer[2]-'0')*10 + buffer[3]-'0';
						year = (buffer[4]-'0')*10 + buffer[5]-'0';
						
#if (RTCC_SOURCE == RTCC_INTERNAL)	
						RTCCSetBinSec(sec);
						RTCCSetBinMin(min);
						RTCCSetBinHour(hour);
						RTCCSetBinDay(day);
						RTCCSetBinMonth(month);
						RTCCSetBinYear(year);
						mRTCCSet();	
						g_seconds = RTCCGetTime_t();	// update the time		// read the clock
#else
						struct tm tm_buf;
						tm_buf.tm_hour = hour;
						tm_buf.tm_min = min;
						tm_buf.tm_sec = sec;
						tm_buf.tm_year = year+100;
						tm_buf.tm_mday = day;
						tm_buf.tm_mon = month-1;
						tm_buf.tm_wday = 0;
						tm_buf.tm_isdst = 0;

						ISL_RTC_WriteTMTime(&tm_buf);	// write the new time
						ISL_ReadTime(&g_seconds);		// read it back	
#endif
						
						PutTimeStr(source_port);
					}
				}
			}		
		}
	}
	else if (strcmp((char *)phandler->header,"$SUUID") == 0) {	// GET Unit ID
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
				g_Config.id = atol(buffer);
				save_config(&g_Config);
				PutLastEepromErrorMsg("Save to Flash",source_port);
				PutConfigData(CONFIG_SOURCE_FLASH,source_port);
			}
		}		
	}
	else if (strcmp((char *)phandler->header,"$SUFLG") == 0) {	// Unit ID
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
				g_UnitFlags.value = axtoi(buffer);
				PutUnitFlags(source_port);
			}
		}		
	}
	else if (strcmp((char *)phandler->header,"$SUCPY") == 0) {		// COPY FILES
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
			}
			else {	// for now we only support copy all
				{
					char oldstate = g_UnitFlags.LogStatus;
					g_UnitFlags.LogStatus = 0;
				    SD_power(1);
					DelayMsecs(100);				// not sure how long to delay before powerup
					unsigned long fcopied = SD_CopyFiles(buffer, source_port);
				    SD_power(0);
					PutFileNumCopied(fcopied,source_port);
					g_UnitFlags.LogStatus = oldstate;
				}
			}	
		}	
	}
	else if (strcmp((char *)phandler->header,"$SUREM") == 0) {		// Erase files
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			char oldstate = g_UnitFlags.LogStatus;
			g_UnitFlags.LogStatus = 0;
		    SD_power(1);
			DelayMsecs(100);				// not sure how long to delay before powerup
			SD_EraseFile(buffer, source_port);		// for now - erase all files
			SD_RefreshProperties(source_port);
			SD_power(0);
			PutMemoryProperties(source_port);
			g_UnitFlags.LogStatus = oldstate;
		}	
	}
	else if (strcmp((char *)phandler->header,SU_APP_RESET_CSV_HEADER) == 0) {		// RESET
		SLCD_putBootloadMessage();
		Reset();
	}
	else {
		traceS2("PC_CSV unknown command: ",(char *)phandler->header);	
	}
}	

unsigned char process_PC_TextCommands(unsigned char rxbyte, int source_port)
{
	if (rxbyte != 0) {
		switch (rxbyte) {
			case GETVERSION:
			case ECHOVERSION:
				PutVersion("Refresh",source_port);
				g_SysError.value = 0;
				break;
			case TOGGLE_SENDBATT:
				g_UnitFlags.SendBattMV = !g_UnitFlags.SendBattMV;  // ENABLE/DISABLE BIT STATUS SENDING
				PutUnitFlags(source_port);
				break;	
			case TOGGLE_DEBUG_SD:
				g_UnitFlags.DebugSD = !g_UnitFlags.DebugSD;														
				PutUnitFlags(source_port);
				break;
			case TOGGLE_LOG_STATUS:
				g_UnitFlags.LogStatus = !g_UnitFlags.LogStatus;		
				PutUnitFlags(source_port);
				break;		    
			case SD_WRITETEST:
			    SD_power(1);
				DelayMsecs(100);				// not sure how long to delay before powerup
		    	SD_writetest(source_port);
			    SD_power(0);
				break;
		    case SD_READTEST:
			    SD_power(1);
				DelayMsecs(100);				// not sure how long to delay before powerup
				SD_readtest(source_port);
			    SD_power(0);
				break;
			case GET_FLASH_CONFIG:
				PutConfigData(CONFIG_SOURCE_FLASH,source_port);
				PutUnitFlags(source_port);
				break;
			case GET_RAM_CONFIG:
				PutConfigData(CONFIG_SOURCE_RAM,source_port);
				PutUnitFlags(source_port);
				break;
			case SWRESET:
				Reset();
				break;	
			case MEM_PROPERTIES:
				if (g_SDProperties.MemType == MEMTYPE_UNKNOWN) 
				{
					char oldstate = g_UnitFlags.LogStatus;
					g_UnitFlags.LogStatus = 0;
				    SD_power(1);
					DelayMsecs(100);			// not sure how long to delay before powerup
					SD_RefreshProperties(source_port);
				    SD_power(0);
					PutMemoryProperties(source_port);
					g_UnitFlags.LogStatus = oldstate;
				}
				else {	// already initialize so just send up current
					PutMemoryProperties(source_port);
				}	
				break;
			case TOGGLE_DEBUG_MISC:
				g_UnitFlags.DebugMisc = !g_UnitFlags.DebugMisc;	
				PutUnitFlags(source_port);
				break;	
			case TOGGLE_SENDTIME:
				g_UnitFlags.SendTime = !g_UnitFlags.SendTime;	
				PutUnitFlags(source_port);
				break;
			case TOGGLE_SEND_SU_MSGS:
				g_UnitFlags.SendSUMsgs = !g_UnitFlags.SendSUMsgs;	
				PutUnitFlags(source_port);
				break;
			case TOGGLE_SEND_MU_MSGS:
				g_UnitFlags.SendMUMsgs = !g_UnitFlags.SendMUMsgs;	
				PutUnitFlags(source_port);
				break;
			case TOGGLE_SEND_LOG:
				g_UnitFlags.SendLog = !g_UnitFlags.SendLog;	
				PutUnitFlags(source_port);
				break;
			case TOGGLE_DEBUG_SLCD:
				g_UnitFlags.DebugSLCD = !g_UnitFlags.DebugSLCD;	
				PutUnitFlags(source_port);
				break;
			case TOGGLE_AUTOLCDSHUTDOWN:
				g_UnitFlags.AutoLCDPowerOff = !g_UnitFlags.AutoLCDPowerOff;	
				PutUnitFlags(source_port);
				break;
			case GET_HELP:
				PutHelp(source_port);
				break;
				
			case TOGGLE_DEBUG_BUTTONS:
				g_UnitFlags.DebugButtons = !g_UnitFlags.DebugButtons;	
				PutUnitFlags(source_port);
				break;
			case TOGGLE_DEBUGWIRELESSS:
				g_UnitFlags.debugWireless = !g_UnitFlags.debugWireless;	
				PutUnitFlags(source_port);
				break;	

			default: 
				PutEcho(rxbyte,source_port);
				break;
		} 
	} 
	return rxbyte;
}



#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// SU_TX ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef SU_TX
#include "..\SU2_TX\vlf_txcmds.h"
#include "su_cmds.h"

void init_unitflags()
{
	g_UnitFlags.value = 0;
	g_UnitFlags.DebugMisc = 0;
} 


void PutHelp(int port)
{
	PutUnitFlags(port);
	PortPutCRStr("...\r\nSU_TX help menu:",port,1);

	sprintf(g_strbuffer,"(%c) SendBattMV=%s",TOGGLE_SENDBATT,g_UnitFlags.SendBattMV?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) DebugMisc=%s",TOGGLE_DEBUG_MISC,g_UnitFlags.DebugMisc?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SendTime=%s",TOGGLE_SENDTIME,g_UnitFlags.SendTime?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_TX_ACK",SEND_TX_ACK);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_TX_NACK",SEND_TX_NACK);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GET_HELP",GET_HELP);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) TOGGLE_LEDS",TOGGLE_LEDS);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SWRESET",SWRESET);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GETVERSION",GETVERSION);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_DETECT_MSG",SEND_DETECT_MSG);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_LOCATE_MSG",SEND_LOCATE_MSG);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_QUICKSEARCH_MSG",SEND_QUICKSEARCH_MSG);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_MASK_MSG",SEND_MASK_MSG);
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) SELECT_X_ANTENNA",SELECT_X_ANTENNA);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SELECT_Y_ANTENNA",SELECT_Y_ANTENNA);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SELECT_Z_ANTENNA",SELECT_Z_ANTENNA);
	PortPutCRStr(g_strbuffer,port,1);

	sprintf(g_strbuffer,"(%c) SET_PROTOCOL1",SET_PROTOCOL1);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SET_PROTOCOL2",SET_PROTOCOL2);
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) DebugWireless=%s",TOGGLE_DEBUGWIRELESSS,g_UnitFlags.debugWireless?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);

}

void PutUnitFlags(int port)
{
	PortPutCRStr("...\r\nSU_TX FLAGS:",port,1);
	sprintf(g_strbuffer,"$%sFLG,%04X",SOURCE_PREFIX,g_UnitFlags.value);
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) SendBattMV=%s",TOGGLE_SENDBATT,g_UnitFlags.SendBattMV?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) DebugMisc=%s",TOGGLE_DEBUG_MISC,g_UnitFlags.DebugMisc?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) SendTime=%s",TOGGLE_SENDTIME,g_UnitFlags.SendTime?"1":"0");
	PortPutCRStr(g_strbuffer,port,1);

	switch (g_protocol) {
		case VLF_PROTOCOL_1:
			strcpy(g_strbuffer,"Current Protocol= 1");
			break;
		case VLF_PROTOCOL_2:
			strcpy(g_strbuffer,"Current Protocol= 2");
			break;
		default:
			sprintf(g_strbuffer,"Unknown Protocol %d",g_protocol);
			break;
	}
	PortPutCRStr(g_strbuffer,port,1);
}


void Process_PC_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[20];

	if (strcmp((char *)phandler->header,"$TXFLG") == 0) {	// Unit ID
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
				g_UnitFlags.value = axtoi(buffer);
				PutUnitFlags(source_port);
			}
		}		
	}
	else if (strcmp((char *)phandler->header,SU_TX_RESET_CSV_HEADER) == 0) {		// RESET
		Reset();
	}
	else {
		traceS2("SU_TX: PC_CSV unknown command ",(char *)phandler->header);	
	}
}

extern int LoopCycles;
extern void PutLoopCycles();

unsigned char process_PC_TextCommands(unsigned char rxbyte, int source_port)
{
	static char antenna = TX_ANTENNA_X;
	U32 mu_id = 1;
	static U8 sequence = 1;
	
	if (rxbyte != 0) {
		switch (rxbyte) {
			case GETVERSION:
			case ECHOVERSION:
				PutVersion("Refresh",source_port);
				g_SysError.value = 0;
				break;
			case TOGGLE_SENDBATT:
				g_UnitFlags.SendBattMV = !g_UnitFlags.SendBattMV;  // ENABLE/DISABLE BIT STATUS SENDING
				PutUnitFlags(source_port);
				break;	
			case TOGGLE_SENDTIME:
				g_UnitFlags.SendTime = !g_UnitFlags.SendTime;	
				PutUnitFlags(source_port);
				break;
				
			case GET_FLASH_CONFIG:
				PutConfigData(CONFIG_SOURCE_FLASH,source_port);
				PutUnitFlags(source_port);
				break;
				
			case GET_RAM_CONFIG:
				PutConfigData(CONFIG_SOURCE_RAM,source_port);
				PutUnitFlags(source_port);
				break;
				
			case SWRESET:
				Reset();
				break;	
				
			case TOGGLE_DEBUG_MISC:
				g_UnitFlags.DebugMisc = !g_UnitFlags.DebugMisc;	
				PutUnitFlags(source_port);
				break;	
				
			case SEND_TX_ACK:
				PortPutCRStr(SU_TX_ACK_CSV_HEADER,SU_PORT,1);
				break;
				
			case SEND_TX_NACK:
				PortPutCRStr(SU_TX_NACK_CSV_HEADER,SU_PORT,1);	
				break;
				
			case SEND_DETECT_MSG:
				switch (g_protocol) {
					case VLF_PROTOCOL_1:
						vlf_sendDetect_Protocol_1(123, 0x7f, sequence, antenna);
						break;
					case VLF_PROTOCOL_2:
						vlf_sendDetect(123, sequence, antenna);
						break;
					default:
						traceS("Invalid Protocol setting");
						break;
				}
				break;
				
			case SEND_LOCATE_MSG:
				switch (g_protocol) {
					case VLF_PROTOCOL_1:
						vlf_sendLocate_Protocol_1(mu_id, 10, antenna);
						break;
					case VLF_PROTOCOL_2:
						vlf_sendLocate(1, 10, antenna);
						break;
					default:
						traceS("Invalid Protocol setting");
						break;
				}
				break;
				
			case SEND_QUICKSEARCH_MSG:
				switch (g_protocol) {
					case VLF_PROTOCOL_1:
						traceS("QuickSearch not supported in Protocol 1");
						break;
					case VLF_PROTOCOL_2:
						vlf_sendQuickSearch(123, sequence, antenna);
						break;
					default:
						traceS("Invalid Protocol setting");
						break;
				}
				break;
				
			case SEND_MASK_MSG:
				switch (g_protocol) {
					case VLF_PROTOCOL_1:
						vlf_sendMask_Protocol_1(1, mu_id, sequence, antenna);
						break;
					case VLF_PROTOCOL_2:
						vlf_sendMask(123, 1, sequence, antenna);
						break;
					default:
						traceS("Invalid Protocol setting");
						break;
				}
				break;
				
			case SELECT_X_ANTENNA:
				traceS("Antenna set to X");
				antenna = 'X';
				break;
				
			case SELECT_Y_ANTENNA:
				traceS("Antenna set to Y");
				antenna = 'Y';
				break;
				
			case SELECT_Z_ANTENNA:
				traceS("Antenna set to Z");
				antenna = 'Z';
				break;
				
			case TOGGLE_LEDS:
				TOGGLE_XON;
				TOGGLE_YON;
				TOGGLE_ZON;
				break;	
			case GET_HELP:
				PutHelp(source_port);
				break;

			case SET_PROTOCOL1:
				g_protocol = VLF_PROTOCOL_1;
				PutUnitFlags(source_port);
				break;

			case SET_PROTOCOL2:
				g_protocol = VLF_PROTOCOL_2;
				PutUnitFlags(source_port);
				break;

			case '+':
				sequence++;
				sprintf(g_strbuffer,"sequence = %d",sequence);
				PortPutCRStr(g_strbuffer,PC_PORT,1);
				break;
			case '-':
				sequence--;
				sprintf(g_strbuffer,"sequence = %d",sequence);
				PortPutCRStr(g_strbuffer,PC_PORT,1);
				break;
			case TOGGLE_DEBUGWIRELESSS:
				g_UnitFlags.debugWireless = !g_UnitFlags.debugWireless;	
				PutUnitFlags(source_port);
				break;	
			default: 
				PutEcho(rxbyte,source_port);
				break;
		} 
	} 
	return rxbyte;
}
	
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// SU_RX ////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////


#ifdef SU_RX
#include "..\SU2_RX\vlf_rxcmds.h"
#include "..\SU2_RX\rssi_xyz.h"

void init_unitflags()
{
	g_UnitFlags.value = 0;
	g_UnitFlags.DebugMisc = 0;
} 

void PutHelp(int port)
{
	PutUnitFlags(port);
	PortPutCRStr("...\r\nSU_RX help menu:",port,1);
	sprintf(g_strbuffer,"(%c) GET_FLASH_CONFIG",GET_FLASH_CONFIG);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GET_RAM_CONFIG",GET_RAM_CONFIG);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GET_HELP",GET_HELP);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) TOGGLE_LEDS",TOGGLE_LEDS);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_RESPONSE_TOSU",SEND_RESPONSE_TOSU);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SWRESET",SWRESET);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GETVERSION",GETVERSION);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SET_PROTOCOL1",SET_PROTOCOL1);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SET_PROTOCOL2",SET_PROTOCOL2);
	PortPutCRStr(g_strbuffer,port,1);

}

void PutUnitFlags(int port)
{
	PortPutCRStr("...\r\nSU_RX FLAGS:",port,1);
	sprintf(g_strbuffer,"$%sFLG,%04X",SOURCE_PREFIX,g_UnitFlags.value);
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) SendBattMV=%s",TOGGLE_SENDBATT,g_UnitFlags.SendBattMV?"Yes":"No");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) DebugMisc=%s",TOGGLE_DEBUG_MISC,g_UnitFlags.DebugMisc?"Yes":"No");
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) SendTime=%s",TOGGLE_SENDTIME,g_UnitFlags.SendTime?"Yes":"No");
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) SendRSSI=%s",TOGGLE_SEND_RSSI,g_UnitFlags.SendRSSI?"Yes":"No");
	PortPutCRStr(g_strbuffer,port,1);

	sprintf(g_strbuffer,"(%c) DecodeMode=%s",TOGGLE_MODE_DECODE,(g_Mode == MODE_DECODE)?"ON":"OFF");
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) LocateMode=%s",TOGGLE_MODE_LOCATE,(g_Mode == MODE_LOCATE)?"ON":"OFF");
	PortPutCRStr(g_strbuffer,port,1);

	switch (g_protocol) {
		case VLF_PROTOCOL_1:
			strcpy(g_strbuffer,"Current Protocol= 1");
			break;
		case VLF_PROTOCOL_2:
			strcpy(g_strbuffer,"Current Protocol= 2");
			break;
		default:
			sprintf(g_strbuffer,"Unknown Protocol %d",g_protocol);
			break;
	}
	PortPutCRStr(g_strbuffer,port,1);
	
}

void Process_PC_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[20];

	if (strcmp((char *)phandler->header,"$RXFLG") == 0) {	// Unit ID
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
				g_UnitFlags.value = axtoi(buffer);
				PutUnitFlags(source_port);
			}
		}		
	}
	else if (strcmp((char *)phandler->header,SU_RX_RESET_CSV_HEADER) == 0) {		// RESET
		Reset();
	}
	else {
		traceS2("SU_RX: PC_CSV unknown command ",(char *)phandler->header);	
	}
}

#ifdef ENABLE_ZC_THRESHOLD_TESTING
void Put_VLF_Threshold(int source_port)
{
	sprintf(g_strbuffer,"VLFHigh = %d VLFLow = %d",vlf_high_threshold,vlf_low_threshold);
	PortPutCRStr(g_strbuffer,source_port,1);
}
#endif

unsigned char process_PC_TextCommands(unsigned char rxbyte, int source_port)
{
	if (rxbyte != 0) {
		switch (rxbyte) {
			case GETVERSION:
			case ECHOVERSION:
				PutVersion("Refresh",source_port);
				g_SysError.value = 0;
				break;
			case TOGGLE_SENDBATT:
				g_UnitFlags.SendBattMV = !g_UnitFlags.SendBattMV;  // ENABLE/DISABLE BIT STATUS SENDING
				PutUnitFlags(source_port);
				break;	
			case TOGGLE_SENDTIME:
				g_UnitFlags.SendTime = !g_UnitFlags.SendTime;	
				PutUnitFlags(source_port);
				break;
			case GET_FLASH_CONFIG:
				PutConfigData(CONFIG_SOURCE_FLASH,source_port);
				PutUnitFlags(source_port);
				break;
			case GET_RAM_CONFIG:
				PutConfigData(CONFIG_SOURCE_RAM,source_port);
				PutUnitFlags(source_port);
				break;
			case SWRESET:
				Reset();
				break;	
			case TOGGLE_DEBUG_MISC:
				g_UnitFlags.DebugMisc = !g_UnitFlags.DebugMisc;	
				PutUnitFlags(source_port);
				break;	
			case SEND_RESPONSE_TOSU:
//				Read_AntennaRSSI_MinMaxLevels();
				{
					if (g_protocol == VLF_PROTOCOL_1) 
					{
						vlfP1_SendGenericResponse(10,3);
					}
					else {
						GenericResponse_P2 resp;
#ifdef MU_ID_3BYTE
						resp.id_u24.val[2]= 0x00;
						resp.id_u24.val[1]= 0x04;
						resp.id_u24.val[0]= 0xD2;
#else
						resp.id_u16 = 1234;
#endif	
						resp.m.status = 0;
						resp.m.mobility = 10;
						vlfP2_SendGenericReponseToSu(&resp);
					}
				}
				break;

			case GET_HELP:
				PutHelp(source_port);
				break;
			case TOGGLE_LEDS:
				// todo
				break;
			case TOGGLE_SEND_RSSI:
				g_UnitFlags.SendRSSI = !g_UnitFlags.SendRSSI;	
				PutUnitFlags(source_port);
				break;	
			case TOGGLE_MODE_DECODE:
				if (vlf_getRxMode() != MODE_DECODE) {
					vlf_setRxMode(MODE_STANDBY,g_protocol);
					vlf_setRxMode(MODE_DECODE,g_protocol);
				}
				else
					vlf_setRxMode(MODE_STANDBY,g_protocol);
				break;
			case TOGGLE_MODE_LOCATE:
				if (vlf_getRxMode() != MODE_LOCATE) {
					vlf_setRxMode(MODE_STANDBY,g_protocol);
					vlf_setRxMode(MODE_LOCATE,g_protocol);
				}
				else
					vlf_setRxMode(MODE_STANDBY,g_protocol);
				break;
			case SET_PROTOCOL1:
				g_protocol = VLF_PROTOCOL_1;
				PutUnitFlags(source_port);
				break;

			case SET_PROTOCOL2:
				g_protocol = VLF_PROTOCOL_2;
				PutUnitFlags(source_port);
				break;
#ifdef ENABLE_ZC_THRESHOLD_TESTING
			case '+':
				if (vlf_high_threshold < 47)
					vlf_high_threshold++;
				Put_VLF_Threshold(source_port);
				break;
			case '-':
				if (vlf_high_threshold > vlf_low_threshold) vlf_high_threshold--;
				Put_VLF_Threshold(source_port);
				break;
			case '[':
				if (vlf_low_threshold < vlf_high_threshold) vlf_high_threshold++;
				Put_VLF_Threshold(source_port);
				break;
			case ']':
				if (vlf_low_threshold > 5) vlf_high_threshold--;
				Put_VLF_Threshold(source_port);
				break;
#endif
			default: 
				PutEcho(rxbyte,source_port);
				break;
		} 
	} 
	return rxbyte;
}


#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////// MU_APP ////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef MU_APP
#include "..\mu2_app\mu_def.h"
#include "..\mu2_app\vlf_send.h"
#include "..\mu2_app\vlf_receive.h"
#include "..\mu2_app\alert.h"
#include "adxl345.h"
#include "led.h"

void init_unitflags()
{
	g_UnitFlags.value = 0;
	g_UnitFlags.DebugMisc = 0;
//	g_UnitFlags.DebugAccel = 1;
	g_UnitFlags.DisableAccel = 0;
	g_UnitFlags.EnableVerterbi = 0;
} 

void PutUnitFlags(int port)
{
	sprintf(g_strbuffer,"$%sFLG,%04X",SOURCE_PREFIX,g_UnitFlags.value);
	PortPutCRStr(g_strbuffer,port,1);
	
	sprintf(g_strbuffer,"(%c) DebugMisc=%s",TOGGLE_DEBUG_MISC,g_UnitFlags.DebugMisc?"ON":"OFF");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SendTime=%s",TOGGLE_SENDTIME,g_UnitFlags.SendTime?"ON":"OFF");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SendAccel=%s",TOGGLE_ACCEL,g_UnitFlags.SendAccel?"ON":"OFF");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) Accelerometer=%s",TOGGLE_DISABLE_ACCELEROMETER,g_UnitFlags.DisableAccel?"Disabled":"Enabled");
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) Verterbi Encoding=%s",TOGGLE_ENABLE_VERTERBI,g_UnitFlags.EnableVerterbi?"Enabled":"Disabled");
	PortPutCRStr(g_strbuffer,port,1);

}

void PutHelp(int port)
{
	PortPutCRStr("...\r\nMU help menu:",port,1);
	PutUnitFlags(port);
	
	sprintf(g_strbuffer,"(%c) HELP",GET_HELP);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) ACTIVATE_BUZZER",ACTIVATE_BUZZER);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) TOGGLE_LED",TOGGLE_LED);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SW RESET",SWRESET);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) GET VERSION",GETVERSION);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_TEST_TRANSMIT SIGNAL",SEND_TEST_TRANSMIT);
	PortPutCRStr(g_strbuffer,port,1);
	sprintf(g_strbuffer,"(%c) SEND_DEFAULT_RESPONSE",SEND_DEFAULT_RESPONSE);
	PortPutCRStr(g_strbuffer,port,1);
	PortPutCRStr("(1...9) SEND_LOCATE_SIGNAL for 10 to 90 Sec",port,1);
//	sprintf(g_strbuffer,"(%c) TEST_MASK_COUNTDOWN",TEST_MASK_COUNTDOWN);
//	PortPutCRStr(g_strbuffer,port,1);

}


void Process_PC_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[20];

	if (strcmp((char *)phandler->header,"$SUFLG") == 0) {	// Unit ID
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
				g_UnitFlags.value = axtoi(buffer);
				PutUnitFlags(source_port);
			}
		}		
	}
	else if (strcmp((char *)phandler->header,"$SUUID") == 0) {	// GET Unit ID
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
				g_Config.id = atol(buffer);
				save_config(&g_Config);
				PutLastEepromErrorMsg("Save to Flash",source_port);
				PutConfigData(CONFIG_SOURCE_FLASH,source_port);
			}
		}		
	}
	else if (strcmp((char *)phandler->header,SU_RX_RESET_CSV_HEADER) == 0) {		// RESET
		Reset();
	}
	else {
		traceS2("MU_RX: PC_CSV unknown command ",(char *)phandler->header);	
	}
}



unsigned char process_PC_TextCommands(unsigned char rxbyte, int source_port)
{
	if (rxbyte != 0) {
		if ((rxbyte >= '1') && (rxbyte <= '9')) {
			vlf_StopReceiver();
			vlf_sendLocateSignal(10*(rxbyte-'0'));
			vlf_StartReceiver();			// go back to receive mode after packet sent
		}
		else {
			switch (rxbyte) {
				case GETVERSION:
				case ECHOVERSION:
					PutVersion("Refresh",source_port);
					g_SysError.value = 0;
					break;
				case GET_FLASH_CONFIG:
					PutConfigData(CONFIG_SOURCE_FLASH,source_port);
					PutUnitFlags(source_port);
					break;
				case GET_RAM_CONFIG:
					PutConfigData(CONFIG_SOURCE_RAM,source_port);
					PutUnitFlags(source_port);
					break;
				case SWRESET:
					Reset();
					break;	
				case TOGGLE_DEBUG_MISC:
					g_UnitFlags.DebugMisc = !g_UnitFlags.DebugMisc;	
					PutUnitFlags(source_port);
					break;	
				case TOGGLE_SENDTIME:
					g_UnitFlags.SendTime = !g_UnitFlags.SendTime;	
					PutUnitFlags(source_port);
					break;	
				case ACTIVATE_BUZZER:
					alert_on(3);
					break;
				case SEND_TEST_TRANSMIT:
					vlf_StopReceiver();
					vlf_testSignal();
					vlf_StartReceiver();			// go back to receive mode after packet sent
					break;
				case GET_HELP:
					PutHelp(source_port);
					break;
				case TOGGLE_LED:
					//LED_TOGGLE;
					led_on(5);
					break;	
				case TOGGLE_ACCEL:
					g_UnitFlags.SendAccel = !g_UnitFlags.SendAccel;	
					if (g_UnitFlags.SendAccel)
						SetAccelerometerEnableCountdown(300);
					else
						SetAccelerometerEnableCountdown(0);
					PutUnitFlags(source_port);
					break;
				case SEND_DEFAULT_RESPONSE:
					vlf_StopReceiver();
					vlf_Send_V2_PacketResponse();
					vlf_StartReceiver();			// go back to receive mode after packet sent
					break;
				case 'z':
					ClearLastFilteredMaxAcceleration();
					break;
	//			case TEST_MASK_COUNTDOWN:
	//				SetMaskCountdown(10);
	//				break;
				case TOGGLE_DISABLE_ACCELEROMETER:
					g_UnitFlags.DisableAccel = !g_UnitFlags.DisableAccel;	
					break;
				case TOGGLE_ENABLE_VERTERBI:
					g_UnitFlags.EnableVerterbi = !g_UnitFlags.EnableVerterbi;	
					break;
				default: 
					PutEcho(rxbyte,source_port);
					break;
			} 
		}
	} 
	return rxbyte;
}
#endif




