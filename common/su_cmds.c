#include "su_cmds.h"
#include "csv_packet.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "trace.h"
#include "putfunctions.h"
#include "vlf_pkt.h"
#ifdef SU_TX
#include "..\SU2_TX\vlf_txcmds.h"
#endif

#ifdef SU_RX
#include "..\SU2_RX\vlf_rxcmds.h"
#include "..\SU2_RX\dac.h"
#endif

////////////////////////////////////// Some globals used by the parsing functions ///////////////////
#define MAX_SU_PACKET_SIZE 128
struct CSV_PACKET_HANDLER g_su_Uart_Packet;
unsigned char su_Uart_cmdbuf[MAX_SU_PACKET_SIZE+2];		// this should be the maximum MESSAGES SIZE FROM THE MURX module
extern unsigned int g_lastbattMV;

/////////////////////////////////////// private prototypes /////////////////////////////////////
void Process_SU_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port);
int ProcessDetectMessage(struct CSV_PACKET_HANDLER* phandler, int source_port);
int ProcessMaskMessage(struct CSV_PACKET_HANDLER* phandler, int source_port);
int ProcessLocateMessage(struct CSV_PACKET_HANDLER* phandler, int source_port);
int ProcessQuickSearchMessage(struct CSV_PACKET_HANDLER* phandler, int source_port);
int ProcessThresholdMessage(struct CSV_PACKET_HANDLER* phandler, int source_port);
int ProcessFullMaskMessage(struct CSV_PACKET_HANDLER* phandler, int source_port);

// call this first before calling ProcessMURXStream for the first time.
void Init_SU_Uart_Stream()
{
	init_CSV_packethandler(&g_su_Uart_Packet,su_Uart_cmdbuf,MAX_SU_PACKET_SIZE);
}

// uncomment the following block for testing RX characters
// #define SHOW_RAW_RX_CHARACTERS

// parses the incoming messages from the SU - InitSUStream() must be called at least once before this function is called	
static void ProcessSUStream(struct CSV_PACKET_HANDLER* pCSV, int port)
{
	unsigned char rxbyte;
	
	while (PortChReady(port)) {
		rxbyte = PortGetCh(port); 
#ifdef SHOW_RAW_RX_CHARACTERS
		if (g_UnitFlags.DebugMisc) 
		{		
			PortPutStr("su> ",PC_PORT,1);
			PortPutChar(rxbyte,PC_PORT);
			PortPutStr("\r\n",PC_PORT,1);
		}
#endif
		if ((rxbyte !=0)) {
			rxbyte = process_CSV_packetbyte(pCSV, rxbyte);
			if (pCSV->packet_timeouts > 0) {
				g_SysError.U2_PacketTimeout = 1;
				pCSV->packet_timeouts=0;
			}
			if (pCSV->m_PacketStage == CSV_PACKET_READY) {
				Process_SU_CSV_Packet(pCSV, port); 
				reset_CSV_packethandler(pCSV);
			}
		}
	}
}

void Process_SU_Uart_Stream()
{
	ProcessSUStream(&g_su_Uart_Packet,SU_PORT);
}


#ifdef SU_TX
// handles the CSV command FROM THE TX MODULE
/////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////                                      //////////////////////////////
////////////////////////////////////// PROCESS SU MESSAGES FOR THE SU_TX   ////////////////////////////////
/////////////////////////////////////                                     ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
U8 g_protocol = VLF_PROTOCOL_2;		// this holds the last protocol 

#define MAX_WI_BUF_SIZE 512
unsigned char g_wi_buf[MAX_WI_BUF_SIZE];		// this should be the maximum of several command structures
struct CSV_PACKET_HANDLER g_Wi_CSV_Packet;

// returns TRUE if a character was received else FALSE	
void Init_SU_Wireless_Stream()
{
	init_CSV_packethandler(&g_Wi_CSV_Packet, g_wi_buf, MAX_WI_BUF_SIZE);
}

void Process_SU_Wireless_Stream()
{
	ProcessSUStream(&g_Wi_CSV_Packet, PORT_WIRELESS);
}	

void Process_SU_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[60];
	int error = 0;

// Send the message to the PC for debugging
	if (g_UnitFlags.DebugMisc) {
		PortPutStr("Process_SU_CSV_Packet start\r\n",PC_PORT,1);
		PutTimeStr(PC_PORT);
		PortPutStr((char *)phandler->header,PC_PORT,1);
		PortPutStr((char*)phandler->pbuf,PC_PORT,1);
		PortPutChar('\n',PC_PORT);
	}
	
// Check the message header for legitimate headers	
	if (strcmp((char *)phandler->header,SU_TX_DETECT_CSV_HEADER) == 0) {		// Detect request from SU
		error = ProcessDetectMessage(phandler, source_port);
	}
	else if (strcmp((char *)phandler->header,SU_TX_MASK_CSV_HEADER) == 0) {		// Mask request from SU
		error = ProcessMaskMessage(phandler, source_port);
	}
	else if (strcmp((char *)phandler->header,SU_TX_LOCATE_CSV_HEADER) == 0) {	// Locate request from SU
		error = ProcessLocateMessage(phandler, source_port);
	}
	else if (strcmp((char *)phandler->header,SU_TX_QUICK_SEARCH_CSV_HEADER) == 0) {		// Quick Search Request from SU
		error = ProcessQuickSearchMessage(phandler, source_port);
	}
	else if (strcmp((char *)phandler->header,SU_TX_PING_CSV_HEADER) == 0) {		// Ping Request from SU
		error = 0;	// JUST A PING COMMAND SO RETURN WITH ACK
		traceS2("SU ping command: ",(char *)phandler->header);	
	}
	//FULL_MASK
	else if (strcmp((char *)phandler->header,SU_TX_FULLMASK_CSV_HEADER) == 0) {		// FULLMask request from SU
		error = ProcessFullMaskMessage(phandler, source_port);
	}
	//FULL_MASK END
	else {
		error = VLF_ERROR_COMMAND;
		if (g_UnitFlags.DebugMisc) {
			PortPutStr("unexpected request FROM SU: ",PC_PORT,1);
			PortPutCRStr((char*)phandler->header,PC_PORT,1);
		}
	}
	
	// Reply with battery level and ack or nack and error code 
	if (error) 
	{
		sprintf(buffer,"%s,%d,%d\r\n",SU_TX_NACK_CSV_HEADER,error,g_lastbattMV);
		PortPutStr(buffer,source_port,1); 
		traceS(buffer);
	}
	else 
	{
		sprintf(buffer,"%s,%d\r\n",SU_TX_ACK_CSV_HEADER,g_lastbattMV);
		PortPutStr(buffer,source_port,1); 
		traceS(buffer);
	}

	if (g_UnitFlags.DebugMisc) {
		PortPutStr("Process_SU_CSV_Packet end\r\n",PC_PORT,1);
	}
	
}	



int ProcessQuickSearchMessage(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[30];
	U8 protocol=0;
	U16 su_id=0;
	U8 scope=0;
	U8 sequence=0;
	U8 antenna=0;
	int n = 0;
	int i;
	int error = 0;
	
	for (i=1;i<6;i++) {
		if (GetNthCSV((char*)phandler->pbuf, i, buffer, 20) == 0) {
			if (strlen(buffer)) {	
				switch (i) {
					case 1:
						protocol = atoi(buffer);		// read the protocol
						n++;
						break;
					case 2:
						su_id = atoi(buffer);				// read the id
						n++;
						break;
					case 3:
						scope = atoi(buffer);				// read the scope
						n++;
						break;
					case 4:
						sequence = atoi(buffer);			// read the sequence
						n++;
						break;
					case 5:
						antenna = buffer[0];				// read the antenna
						n++;
						break;
					default:
						break;
				}	
			}
		}	
	}
	if (n == 5) {	// if the message matches the number of expected fields, then send the messages
		traceS2("Processing QuickSearch message from SU: ",(char *)phandler->header);
		switch (protocol) 
		{
			case VLF_PROTOCOL_1:
				g_protocol = protocol;
				error = vlf_sendDetect_Protocol_1(su_id, scope, sequence, antenna);		
				break;
			case VLF_PROTOCOL_2:
				g_protocol = protocol;
				error = vlf_sendQuickSearch(su_id, sequence, antenna);
				break;	
			default:
				error = VLF_ERROR_PROTOCOL;			
		}
	}
	else {
		traceS2("Insufficient parameters for command: ",(char *)phandler->header);	
		error = VLF_ERROR_COMMAND;
	}
	return error;
}



int ProcessDetectMessage(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[30];
	U8 protocol=0;
	U16 su_id=0;
	U8 scope=0;
	U8 sequence=0;
	U8 antenna=0;
	int n = 0;
	int i;
	int error = 0;
	
	for (i=1;i<6;i++) {
		if (GetNthCSV((char*)phandler->pbuf, i, buffer, 20) == 0) {
			if (strlen(buffer)) {	
				switch (i) {
					case 1:
						protocol = atoi(buffer);		// read the protocol
						n++;
						break;
					case 2:
						su_id = atoi(buffer);				// read the id
						n++;
						break;
					case 3:
						scope = atoi(buffer);				// read the scope
						n++;
						break;
					case 4:
						sequence = atoi(buffer);			// read the sequence
						n++;
						break;
					case 5:
						antenna = buffer[0];				// read the antenna
						n++;
						break;
					default:
						break;
				}	
			}
		}	
	}
	if (n == 5) {	// if the message matches the number of expected fields, then send the messages
		traceS2("Processing detect message from SU: ",(char *)phandler->header);	
		switch (protocol) 
		{
			case VLF_PROTOCOL_1:
				g_protocol = protocol;
				error = vlf_sendDetect_Protocol_1(su_id, scope, sequence, antenna);
				break;
			case VLF_PROTOCOL_2:
				g_protocol = protocol;
				error = vlf_sendDetect(su_id, sequence, antenna);
				break;	
			default:
				error = VLF_ERROR_PROTOCOL;			
		}	
	}
	else {
		traceS2("Insufficient parameters for command: ",(char *)phandler->header);	
		error = VLF_ERROR_COMMAND;
	}
	return error;
}


int ProcessMaskMessage(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[30];
	U8 protocol=0;
	U16 su_id=0;
	U32 mu_id=0;
	U8 sequence=0;
	U8 antenna=0;
	int n = 0;
	int i;
	int error = 0;
	
	for (i=1;i<6;i++) {
		if (GetNthCSV((char*)phandler->pbuf, i, buffer, 20) == 0) {
			if (strlen(buffer)) {	
				switch (i) {
					case 1:
						protocol = atoi(buffer);			// read the protocol
						n++;
						break;
					case 2:
						su_id = (U16)atoi(buffer);				// read the su_id
						n++;
						break;
					case 3:
						mu_id = atol(buffer);				// read the mu_id
						n++;
						break;
					case 4:
						sequence = atoi(buffer);			// read the sequence
						n++;
						break;
					case 5:
						antenna = buffer[0];				// read the antenna
						n++;
						break;
					default:
						break;
				}	
			}
		}	
	}
	if (n == 5) {	// if the message matches the number of expected fields, then send the messages
		traceS2("Processing mask message from SU:",(char *)phandler->header);	
		switch (protocol) 
		{
			case VLF_PROTOCOL_1:
				g_protocol = protocol;
				error = vlf_sendMask_Protocol_1(su_id, mu_id, sequence, antenna);
				break;
			case VLF_PROTOCOL_2:
				g_protocol = protocol;
				error = vlf_sendMask(su_id, mu_id, sequence, antenna);
				break;	
			default:
				error = VLF_ERROR_PROTOCOL;			
		}			

	}
	else {
		error = VLF_ERROR_COMMAND;
		traceS2("Insufficient parameters for command:",(char *)phandler->header);	
	}
	return error;
}

//FULL_MASK
int ProcessFullMaskMessage(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[30];
	U8 protocol=0;
	U32 mu_id=0;
	U8 minutes = 0;
	U8 antenna=0;
	int n = 0;
	int i;
	int error = 0;
	
	for (i=1;i<5;i++) {
		if (GetNthCSV((char*)phandler->pbuf, i, buffer, 20) == 0) {
			if (strlen(buffer)) {	
				switch (i) {
					case 1:
						protocol = atoi(buffer);			// read the protocol
						n++;
						break;
					case 2:
						mu_id = atol(buffer);				// read the mu_id
						n++;
						break;
					case 3:
						minutes = atoi(buffer);				// read the minutes
						n++;
						break;
					case 4:
						antenna = buffer[0];				// read the antenna
						n++;
						break;
					default:
						break;
				}	
			}
		}	
	}

	if (n == 4) {	// if the message matches the number of expected fields, then send the messages
		traceS2("Processing full mask message from SU:",(char *)phandler->header);	
		switch (protocol) 
		{
			case VLF_PROTOCOL_2:
				g_protocol = protocol;
				error = vlf_sendFullMask(mu_id, minutes, antenna);
				break;	
			default:
				error = VLF_ERROR_PROTOCOL;			
		}			

	}
	else {
		error = VLF_ERROR_COMMAND;
		traceS2("Insufficient parameters for command:",(char *)phandler->header);	
	}
	return error;
}
//FULL_MASK END



int ProcessLocateMessage(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[30];
	U8 protocol=0;
	U32 mu_id=0;
	U8 seconds=0;
	U8 antenna=0;
	int n = 0;
	int i;
	int error = 0;
	
	for (i=1;i<5;i++) {
		if (GetNthCSV((char*)phandler->pbuf, i, buffer, 20) == 0) {
			if (strlen(buffer)) {	
				switch (i) {
					case 1:
						protocol = atoi(buffer);			// read the protocol
						n++;
						break;
					case 2:
						mu_id = atol(buffer);				// read the mu id
						n++;
						break;
					case 3:
						seconds = atoi(buffer);				// read the seconds
						n++;
						break;
					case 4:
						antenna = buffer[0];				// read the antenna
						n++;
						break;
					default:
						break;
				}	
			}
		}	
	}
	if (n == 4) {	// if the message matches the number of expected fields, then send the messages
		traceS2("Processing Locate message from SU:",(char *)phandler->header);	
		switch (protocol) 
		{
			case VLF_PROTOCOL_1:
				g_protocol = protocol;
				error = vlf_sendLocate_Protocol_1(mu_id, seconds, antenna);
				break;
			case VLF_PROTOCOL_2:
				g_protocol = protocol;
				error = vlf_sendLocate(mu_id, seconds, antenna);
				break;	
			default:
				error = VLF_ERROR_PROTOCOL;			
		}			

	}
	else {
		error = VLF_ERROR_COMMAND;
		traceS2("Insufficient parameters for command:",(char *)phandler->header);	
	}
	return error;
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////                                    //////////////////////////////
////////////////////////////////////// PROCESS SU MESSAGES FOR THE SU_RX  ////////////////////////////////
//////////////////////////////////////                                   //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef SU_RX
#define SU_RX_MODE_STANDBY	 "Standby"
#define SU_RX_MODE_DECODE	 "Decode"
#define SU_RX_MODE_LOCATE	 "Locate"
int ProcessModeMessage(struct CSV_PACKET_HANDLER* phandler, int source_port);
U8 g_protocol = VLF_PROTOCOL_2;		// this holds the last protocol 

// handles the CSV command FROM THE SU MODULE  
void Process_SU_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[30];
	int error = 0;
	BOOL bGetClutter = FALSE;
	BOOL bGetZC = FALSE;

// Send the message to the PC for debugging
	if (g_UnitFlags.DebugMisc) {
		PortPutStr("Process_SU_CSV_Packet start\r\n",PC_PORT,1);
		PutTimeStr(PC_PORT);
		PortPutStr((char *)phandler->header,PC_PORT,1);
		PortPutStr((char*)phandler->pbuf,PC_PORT,1);
		PortPutChar('\n',PC_PORT);
	}
	
// Check the message header for legitimate headers	
	if (strcmp((char *)phandler->header,SU_RX_MODE_CSV_HEADER) == 0) {		// REQUEST TO CHANGE MODE
		error = ProcessModeMessage(phandler, source_port);
	}
	else if (strcmp((char *)phandler->header,SU_RX_PING_CSV_HEADER) == 0) {		// PING
		error = 0;	// JUST A PING COMMAND
		traceS2("SU ping command: ",(char *)phandler->header);	
	}
	else if (strcmp((char *)phandler->header,SU_RX_CLUTTER_RSSI_CSV_HEADER) == 0) {		// CLUTTER REQUEST
		error = 0;	// JUST A clutter COMMAND
		bGetClutter = TRUE;
		traceS2("SU clutter command: ",(char *)phandler->header);	
	}
	else if (strcmp((char *)phandler->header,SU_RX_ZC_CSV_HEADER) == 0) {		// ZC REQUEST
		error = 0;	// JUST A ZC COMMAND
		bGetZC = TRUE;
		traceS2("SU ZC command: ",(char *)phandler->header);	
	}
	else if (strcmp((char *)phandler->header,SU_RX_ZC_TRIGGER_LEVEL_CSV_HEADER) == 0) {	 // REQUEST TO CHANGE ZC THRESHOLD
		error = ProcessThresholdMessage(phandler, source_port);
	}
	else {
		error = VLF_ERROR_COMMAND;
		traceS2("SU unexpected command: ",(char *)phandler->header);	
	}
	
	if (error) {
		sprintf(buffer,"%s,%d",SU_RX_NACK_CSV_HEADER,error);
		PortPutCRStr(buffer,SU_PORT,1); 
	}
	else
	{
		if ((bGetClutter == FALSE) && (bGetZC == FALSE))
			PortPutCRStr(SU_RX_ACK_CSV_HEADER,SU_PORT,1); 
	}
		

	if (g_UnitFlags.DebugMisc) {
		PortPutStr("Process_SU_CSV_Packet end\r\n",PC_PORT,1);
	}
	
	if (bGetClutter)
	{
		Put_Clutter_RSSI(SU_PORT);		// send RSSI in calibrate mode
		if (g_UnitFlags.DebugMisc)
			Put_Clutter_RSSI(PC_PORT);		// send RSSI in calibrate mode
	}
	
	if (bGetZC)
	{
		PutZeroCrossings(SU_PORT);		// send the zero crossings in calibrate mode
		if (g_UnitFlags.DebugMisc) 	PutZeroCrossings(PC_PORT);
		ResetZeroCrossing();
	}
}	

int ProcessModeMessage(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[30];
	U8 mode=0;
	int n = 0;
	int i;
	int error = 0;
	U8 protocol=VLF_PROTOCOL_2;
	
	for (i=1;i<3;i++) {
		if (GetNthCSV((char*)phandler->pbuf, i, buffer, 20) == 0) {
			if (strlen(buffer)) {	
				switch (i) {
					case 1:
						if (strcmp(buffer,SU_RX_MODE_STANDBY) == 0) {
							mode = MODE_STANDBY;		// read the MODE
							n++;
						}
						else if (strcmp(buffer,SU_RX_MODE_DECODE) == 0) {
							mode = MODE_DECODE;		// read the MODE
							n++;
						}
						else if (strcmp(buffer,SU_RX_MODE_LOCATE) == 0) {
							mode = MODE_LOCATE;		// read the MODE
							n++;
						}
						else if (strcmp(buffer,SU_RX_MODE_CALIBRATE) == 0) {
							mode = MODE_CALIBRATE;
							n++;
						}
						else {
							error = VLF_ERROR_PARAMETER;
						}
		traceS2("Changed to Mode: ", buffer);	
						break;
					case 2:		
						protocol = atoi(buffer);		// read the protocol
						n++;
						break;
					default:
						break;
				}	
			}
		}	
	}
	if (n >= 2) {	// if the message matches the number of expected fields, then send the messages
		sprintf(buffer,"Protocol=%d",protocol+1);
		traceS(buffer);	
		
		// Set up background message updates based on mode
		switch (mode)
		{
			case MODE_DECODE:
				g_SendClutterRSSIUpdates = FALSE;
				g_SendZCUpdates = FALSE;
				break;
			case MODE_CALIBRATE:
				mode = MODE_DECODE;
				g_SendClutterRSSIUpdates = FALSE;
				g_SendZCUpdates = FALSE;// why FALSE instead of TRUE <---CALIBRATION MODE asked by Alex 3_12_2019
				break;
			case MODE_LOCATE:
			case MODE_STANDBY:
			default:
				g_SendClutterRSSIUpdates = FALSE;
				g_SendZCUpdates = FALSE;
				break;		
		}
		error = vlf_setRxMode(mode, protocol);
		
	}
	else {
		error = VLF_ERROR_COMMAND;
		traceS2("Insufficient parameters for command:",(char *)phandler->header);	
	}
	return error;
}


int ProcessThresholdMessage(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[30];
	int n = 0;
	int i;
	int error = 0;
	int dacx,dacy,dacz;
	
	for (i=1;i<4;i++) {
		if (GetNthCSV((char*)phandler->pbuf, i, buffer, 20) == 0) {
			if (strlen(buffer)) {	
				switch (i) {
					case 1:
						dacx = atoi(buffer);		// read the DAC value for X
						n++;
						break;
					case 2:		
						dacy = atoi(buffer);		// read the DAC value for Y
						n++;
						break;
					case 3:
						dacz = atoi(buffer);		// read the DAC value for Z
						n++;
						break;
					default:
						break;
				}	
			}
		}	
	}
	if (n >= 3) {	// if the message matches the number of expected fields, then send the messages
		sprintf(buffer,"dacX=%d, dacY=%d, dacZ=%d",dacx,dacy,dacz);
		traceS(buffer);	
		dac_setXYZ(dacx,dacy,dacz);
	}
	else {
		error = VLF_ERROR_COMMAND;
		traceS2("Insufficient parameters for command:",(char *)phandler->header);	
	}
	return error;
}


#endif









