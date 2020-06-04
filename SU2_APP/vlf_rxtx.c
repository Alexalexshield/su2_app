#include "vlf_rxtx.h"
#include "su_slcd.h"
#include "config.h"
#include "csv_packet.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "trace.h"
#include "putfunctions.h"
#include "vlf_pkt.h"


////////////////////////////////////// Some globals used by the parsing functions ///////////////////
#define MAX_SU_VLF_RX_PACKET_SIZE 128
struct CSV_PACKET_HANDLER g_SU_VLF_RX_Uart_Packet;
unsigned char SU_VLF_RX_Uart_cmdbuf[MAX_SU_VLF_RX_PACKET_SIZE+2];		// this should be the maximum MESSAGES SIZE FROM THE MURX module

#define MAX_SU_VLF_TX_PACKET_SIZE 128
struct CSV_PACKET_HANDLER g_SU_VLF_TX_Uart_Packet;
unsigned char SU_VLF_TX_Uart_cmdbuf[MAX_SU_VLF_TX_PACKET_SIZE+2];		// this should be the maximum MESSAGES SIZE FROM THE MURX module

unsigned int g_muRx_MU_ResponseCount = 0;	// counter for number of generic response packets -helps determine if an asychronous response arrived
unsigned int g_muRx_RSSIPacketCount = 0;	// counter for number of RSSI PACKETS
unsigned int g_muRx_ZCPacketCount = 0;		// counter for number of zero crossing data PACKETS
unsigned int g_muRx_PingCount = 0;			// counter for number of RX ping responses
unsigned int g_SU_Tx_PingCount = 0;			// counter for number of TX ping responses

/////////////////////////////////////// private prototypes /////////////////////////////////////
void Process_SURX_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port);
void Process_SUTX_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port);
void ExtractIDMessage(struct CSV_PACKET_HANDLER* phandler, MuData* pdata);
void ExtractZeroCrossingData(struct CSV_PACKET_HANDLER* phandler);
void ExtractRSSIData(struct CSV_PACKET_HANDLER* phandler);


void clear_MU_TX_PingCount() { g_SU_Tx_PingCount=0; };
unsigned int get_MU_TX_PingCount() { return g_SU_Tx_PingCount; };

void clear_MU_RX_PingCount() { g_muRx_PingCount=0; };
unsigned int get_MU_RX_PingCount() { return g_muRx_PingCount; };

void clear_MU_ResponseCount() { g_muRx_MU_ResponseCount=0; };
unsigned int get_MU_ResponseCount() { return g_muRx_MU_ResponseCount; };

void clear_RSSI_RX_ResponseCount() { g_muRx_RSSIPacketCount=0; };
unsigned int get_RSSI_RX_ResponseCount() { return g_muRx_RSSIPacketCount; };

void clear_SU_ZC_RX_ResponseCount() { g_muRx_ZCPacketCount=0; };
unsigned int get_ZC_RX_ResponseCount() { return g_muRx_ZCPacketCount; };



static U8 g_sequence = 1;

void vlf_SetNewSearchSequence()
{
	g_sequence++;		// incrementing the search sequence creates a new search so that all MU units will respond again
}

static U32 m_lastMUID = 0;
static char m_lastTXAntenna = TX_ANTENNA_NONE;

// sets the ID for any messages coming from the MURX module which aren't identifed by an ID in the message from the MU
void vlf_SetMUID(U32 id) { m_lastMUID = id;};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////                                ///////////////////////////////////////
/////////////////////////////////////  SEND TO SU_TX FUNCTIONS  ///////////////////////////////////////
/////////////////////////////////////                                ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

//#define TEST_ON_PC_PORT
// sends a broadcast command to search for MU units
void vlf_SendDetectMsg(char antenna)
{
	char buffer[80];
	g_SysError.MU_DecodeError = 0;		// reset the fact that any decode errors occurred

	m_lastTXAntenna = antenna;
	unsigned int scope = 0x80;
	
	sprintf(buffer,"%s,%d,%u,%u,%u,%c\r\n",
		SU_TX_DETECT_CSV_HEADER,
#ifdef PROTOCOL_1
		VLF_PROTOCOL_1,
#elif PROTOCOL_2		
		VLF_PROTOCOL_2,
#else
#error PROTOCOL NOT DEFINED					
#endif	
		(U16)g_Config.id,				// Contains a unique SU ID
		scope,							// Sets the scope of the detect operation - high bit set is new search
		(unsigned int)g_sequence,		// Gives a unique sequence for the detect operation 
		m_lastTXAntenna					// contains the antenna index
		);
	PortPutStr(buffer, (g_SuSettings.enable_wireless?PORT_WIRELESS:SUTX_UART_PORT),1);	

}

void vlf_SendQuickSearchMsg(char antenna)
{
	char buffer[80];
	g_SysError.MU_DecodeError = 0;		// reset the fact that any decode errors occurred

	m_lastTXAntenna = antenna;
	unsigned int scope = 0x80;
	
	sprintf(buffer,"%s,%d,%u,%u,%u,%c\r\n",
		SU_TX_QUICK_SEARCH_CSV_HEADER,
#ifdef PROTOCOL_1
		VLF_PROTOCOL_1,
#elif PROTOCOL_2		
		VLF_PROTOCOL_2,
#else
#error PROTOCOL NOT DEFINED					
#endif	
		(U16)g_Config.id,				// Contains a unique SU ID
		scope,							// Sets the scope of the detect operation - high bit set is new search
		(unsigned int)g_sequence,		// Gives a unique sequence for the detect operation 
		m_lastTXAntenna					// contains the antenna index
		);
	PortPutStr(buffer, (g_SuSettings.enable_wireless?PORT_WIRELESS:SUTX_UART_PORT),1);	
}


// send a mask command to tell the MU unit with the ID to no longer respond to this sequence number
void vlf_SendMaskMsg(U32 id, char antenna)
{
	char buffer[80];
	m_lastTXAntenna = antenna;
	
	sprintf(buffer,"%s,%d,%u,%lu,%u,%c\r\n",
		SU_TX_MASK_CSV_HEADER,
#ifdef PROTOCOL_1
		VLF_PROTOCOL_1,
#elif PROTOCOL_2		
		VLF_PROTOCOL_2,
#else
#error PROTOCOL NOT DEFINED					
#endif	
		(U16)g_Config.id,				// Contains a unique SU ID
		id,								// The id of the MU unit to mask 
		(unsigned int)g_sequence,		// Gives a unique sequence for the detect operation 
		m_lastTXAntenna					// contains the antenna index
		);
	PortPutStr(buffer, (g_SuSettings.enable_wireless?PORT_WIRELESS:SUTX_UART_PORT),1);	
}

//FULL_MASK
// send a full mask command to tell the MU unit with the ID to no longer respond to this SU and sequence number
// on any of the broadcast commands. It will only listen to Mask and unmask commands
// note that if minutes is 0, the device is unmasked
void vlf_SendFullMaskMsg(U32 id, U8 minutes, char antenna)
{
	char buffer[80];
	m_lastTXAntenna = antenna;
	
	sprintf(buffer,"%s,%d,%lu,%u,%c\r\n",
		SU_TX_FULLMASK_CSV_HEADER,
#ifdef PROTOCOL_1
		VLF_PROTOCOL_1,
#elif PROTOCOL_2		
		VLF_PROTOCOL_2,
#else
#error PROTOCOL NOT DEFINED					
#endif	
		id,								// The id of the MU unit to mask 
		(unsigned int)minutes,			// Number of minutes the tag should not reply to broadcast commands
		m_lastTXAntenna					// contains the antenna index
		);
	PortPutStr(buffer, (g_SuSettings.enable_wireless?PORT_WIRELESS:SUTX_UART_PORT),1);	
}
//FULL_MASK END


// send a locate command to tell the MU unit with the ID to go into locate mode
// Seconds is the time to transmit LOC signal for. 
// A value of zero indicates a transmission
// that is prolonged by the SU periodically transmitting a signature as long as it
// wants the MU to carry on transmitting
void vlf_SendLocateMsg(U32 id, U8 seconds, char antenna)
{
	char buffer[80];
	m_lastTXAntenna = antenna;
	
	sprintf(buffer,"%s,%d,%lu,%u,%c\r\n",
		SU_TX_LOCATE_CSV_HEADER,
#ifdef PROTOCOL_1
		VLF_PROTOCOL_1,
#elif PROTOCOL_2		
		VLF_PROTOCOL_2,
#else
#error PROTOCOL NOT DEFINED					
#endif	
		id,									// The id of the MU unit to put into locate mode 
		(unsigned int)seconds,				// Contains the number of seconds - 0 means that it is on demand 
		m_lastTXAntenna						// contains the antenna index
		);
	PortPutStr(buffer, (g_SuSettings.enable_wireless?PORT_WIRELESS:SUTX_UART_PORT),1);	
	
}

void send_SU_TX_PING()
{
	char buffer[32];
	sprintf(buffer,"%s",SU_TX_PING_CSV_HEADER);
	PortPutCRStr(buffer, (g_SuSettings.enable_wireless?PORT_WIRELESS:SUTX_UART_PORT),1);	
	traceS(buffer);	
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////                                ///////////////////////////////////////
/////////////////////////////////////  TO SU_RX FUNCTIONS            ///////////////////////////////////////
/////////////////////////////////////                                ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void set_SU_VLF_RX_Mode(char* pmode)
{
	char buffer[32];
#ifdef TEST_ON_PC_PORT
	int port = PC_PORT;
#else
	int port = SURX_UART_PORT;
#endif

#ifdef PROTOCOL_1
	int protocol = VLF_PROTOCOL_1;
#elif PROTOCOL_2		
	int protocol = VLF_PROTOCOL_2;
#else
#error PROTOCOL NOT DEFINED					
#endif	

	sprintf(buffer,"%s,%s,%d",SU_RX_MODE_CSV_HEADER,pmode,protocol);
	PortPutCRStr(buffer,port,1);	
	traceS(buffer);	
		
}

// requests current clutter from SU_RX
void Send_Clutter_request()	
{
	PortPutCRStr(SU_RX_CLUTTER_RSSI_CSV_HEADER, SURX_UART_PORT, 1);
	traceS2("Sending: ",SU_RX_CLUTTER_RSSI_CSV_HEADER);
}

void Send_ZC_request()
{
	PortPutCRStr(SU_RX_ZC_CSV_HEADER, SURX_UART_PORT, 1);	
	traceS2("Sending: ",SU_RX_ZC_CSV_HEADER);
}

void send_SU_RX_PING()
{
	char buffer[32];
#ifdef TEST_ON_PC_PORT
	int port = PC_PORT;
#else
	int port = SURX_UART_PORT;
#endif
	
	sprintf(buffer,"%s",SU_RX_PING_CSV_HEADER);
	PortPutCRStr(buffer,port,1);	
	traceS(buffer);	
}

void send_SU_RX_ZC_Trigger_Level(int levelx, int levely, int levelz)
{
	char buffer[32];
	sprintf(buffer,"%s,%d,%d,%d",SU_RX_ZC_TRIGGER_LEVEL_CSV_HEADER, levelx, levely, levelz);
	PortPutCRStr(buffer,SURX_UART_PORT,1);	
	traceS(buffer);	
}

void send_SU_RX_ZC_Trigger()
{
	send_SU_RX_ZC_Trigger_Level(g_SuSettings.ZC_trigger_level+g_SuSettings.ZCx_trigger_adjust,
								g_SuSettings.ZC_trigger_level+g_SuSettings.ZCy_trigger_adjust,
								g_SuSettings.ZC_trigger_level+g_SuSettings.ZCz_trigger_adjust);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////                                ///////////////////////////////////////
/////////////////////////////////////  RECEIVE FROM SU_RX FUNCTIONS  ///////////////////////////////////////
/////////////////////////////////////                                ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UpdateClutterRssi_Peaks()
{
	g_peak_Clutter_rssi.x = max(g_peak_Clutter_rssi.x,g_last_Clutter_rssi.x);
	g_peak_Clutter_rssi.y = max(g_peak_Clutter_rssi.y,g_last_Clutter_rssi.y);
	g_peak_Clutter_rssi.z = max(g_peak_Clutter_rssi.z,g_last_Clutter_rssi.z);		
}

void UpdatePacketRssi_Peaks()
{
	g_peak_Packet_rssi.x = max(g_peak_Packet_rssi.x,g_last_Packet_rssi.x);
	g_peak_Packet_rssi.y = max(g_peak_Packet_rssi.y,g_last_Packet_rssi.y);
	g_peak_Packet_rssi.z = max(g_peak_Packet_rssi.z,g_last_Packet_rssi.z);		
}

// call this first before calling ProcessMURXStream for the first time.
void InitSURXStream()
{
	init_CSV_packethandler(&g_SU_VLF_RX_Uart_Packet,SU_VLF_RX_Uart_cmdbuf,MAX_SU_VLF_RX_PACKET_SIZE);
}


// parses the incoming messages from the murx - InitMURXStream() must be called at least once before this function is called	
void ProcessSURXStream()
{
	struct CSV_PACKET_HANDLER* pCSV = &g_SU_VLF_RX_Uart_Packet;
	unsigned char rxbyte;
	
	while (PortChReady(SURX_UART_PORT)) {
		rxbyte = PortGetCh(SURX_UART_PORT); 
//		PortPutChar('r',PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar('X',PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar(rxbyte,PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar('\r',PC_PORT);		// todo comment out - this is for debugging
		if ((rxbyte !=0)) {
//			PortPutChar(rxbyte,PC_PORT);
			rxbyte = process_CSV_packetbyte(pCSV, rxbyte);
			if (pCSV->packet_timeouts > 0) {
				g_SysError.U4_RXError = 1;
				pCSV->packet_timeouts=0;
			}
			if (pCSV->m_PacketStage == CSV_PACKET_READY) {
				Process_SURX_CSV_Packet(pCSV, SURX_UART_PORT); 
				reset_CSV_packethandler(pCSV);
			}
		}
	}
}


// handles the CSV command
void Process_SURX_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	MuData data;
	SU_DataResetUnit(&data);	// resets all values to not initialized	

	if (m_lastMUID > 0) {
		data.id_u32 = m_lastMUID;		// by default, all messages without an ID are sent to the last one
		data.v.ID_valid = 1;
	}
	
	data.antenna = m_lastTXAntenna;		// last antenna
	int bArchive = FALSE;
	
	if (strcmp((char *)phandler->header,SU_RX_MU_RESPONSE) == 0) {
		g_muRx_MU_ResponseCount++;
		ExtractIDMessage(phandler, &data);		
		SU_UpdateUnit(&data);				// update the unit's information
		bArchive = 1;
		g_muRx_PingCount++;
	}
	else if (strcmp((char *)phandler->header,SU_RX_CLUTTER_RSSI_CSV_HEADER) == 0) {				// Background clutter from SU-RX
		g_muRx_RSSIPacketCount++;				// increment counter to say that a new data is available
		ExtractRSSIData(phandler);
		traceS2((char *)phandler->header, (char *)phandler->pbuf);	
		g_muRx_PingCount++;
	}
	else if (strcmp((char *)phandler->header,SU_RX_ZC_CSV_HEADER) == 0) {				// RESPONSE FROM MU VIA MURX MODULE
		g_muRx_ZCPacketCount++;				// increment counter to say that a new data is available
		ExtractZeroCrossingData(phandler);		
		traceS2((char *)phandler->header, (char *)phandler->pbuf);	
		g_muRx_PingCount++;
	}
	else if (strcmp((char *)phandler->header,SU_RX_ERROR_CSV_HEADER) == 0) {			// DECODE ERROR MESSAGE
		g_SysError.MU_DecodeError = 1;
		traceS("Decode error from SURX");
		bArchive = 1;
	}
	else if (strcmp((char *)phandler->header, SU_RX_ACK_CSV_HEADER) == 0) {			// ACK MESSAGE
		traceS2("ACK from SURX module: ",(char *)phandler->header);	
		g_muRx_PingCount++;
	}
	else if (strcmp((char *)phandler->header, SU_RX_NACK_CSV_HEADER) == 0) {			// NACK MESSAGE
		traceS2("NACK from SURX module: ",(char *)phandler->header);	
		g_muRx_PingCount++;
	}
	else {
		traceS2("SURX_CSV got command: ",(char *)phandler->header);	
	}
	if (bArchive) {
		// Save the message to the SD card
		PutTimeStr(PORT_SDLOG);
		PortPutStr((char *)phandler->header,PORT_SDLOG,1);
		PortPutStr((char*)phandler->pbuf,PORT_SDLOG,1);
		PortPutChar('\n',PORT_SDLOG);

		if (g_UnitFlags.DebugMisc) 
		{
			PutTimeStr(PC_PORT);
			PortPutStr((char *)phandler->header,PC_PORT,1);
			PortPutStr((char*)phandler->pbuf,PC_PORT,1);
			PortPutChar('\n',PC_PORT);
		}
	}
}	



void ExtractIDMessage(struct CSV_PACKET_HANDLER* phandler, MuData* pdata)
{
	char buffer[30];
	int i;
	
	for (i=1;i<9;i++) {
		if (GetNthCSV((char*)phandler->pbuf, i, buffer, 20) == 0) {
			if (strlen(buffer)) {	
				switch (i) {
					case 1:
						pdata->id_u32 = atol(buffer);
						pdata->v.ID_valid = 1;
						break;
					case 2:		
						pdata->m.mobility = (U8)atoi(buffer);	// read status sent by unit
						pdata->v.Mobility_valid = 1;
						break;
					case 3:		
						pdata->x1_power = atoi(buffer);			// read x1 delta RSSI as seen by su unit
						pdata->v.X1_power_valid = 1;
						g_last_Packet_rssi.x = pdata->x1_power;			// take a copy of RSSI for background RSSI display
						break;
					case 4:		
						pdata->y1_power = atoi(buffer);			// read y1 delta RSSI as seen by su unit
						pdata->v.Y1_power_valid = 1;
						g_last_Packet_rssi.y = pdata->y1_power;			// take a copy of RSSI for background RSSI display
						break;
					case 5:		
						pdata->z1_power = atoi(buffer);			// read z1 delta RSSI as seen by su unit
						pdata->v.Z1_power_valid = 1;
						g_last_Packet_rssi.z = pdata->z1_power;			// take a copy of RSSI for background RSSI display
						break;
					default:
						break;
				}	
			}
		}	
	}
	UpdatePacketRssi_Peaks();	// update rssi peaks 
	
}


// parses 	sprintf(g_strbuffer,"%s,X=%3u,Y=%3u,Z=%3u",	SU_RX_ZC_CSV_HEADER,g_x_lastrssi,g_y_lastrssi,g_z_lastrssi);
/////////////////////////////////////////////////// test FUNCTIONS /////////////////////////////////////



void ExtractRSSIData(struct CSV_PACKET_HANDLER* phandler)
{
	char buffer[20];
	if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
		if (strlen(buffer)>0) {
			g_last_Clutter_rssi.x = atoi(&buffer[0]);					//read x RSSI count
		}
	}
	if (GetNthCSV((char*)phandler->pbuf, 2, buffer, 20) == 0) {
		if (strlen(buffer)>0) {
			g_last_Clutter_rssi.y = atoi(&buffer[0]);					//read y RSSI count
		}
	}
	if (GetNthCSV((char*)phandler->pbuf, 3, buffer, 20) == 0) {
		if (strlen(buffer)>0) {
			g_last_Clutter_rssi.z = atoi(&buffer[0]);					//read z RSSI count
		}
	}
	UpdateClutterRssi_Peaks();
}


// parses 	sprintf(g_strbuffer,"%s,X=%3u,Y=%3u,Z=%3u",	SU_RX_ZC_CSV_HEADER,g_XZeroCrossings,g_YZeroCrossings,g_ZZeroCrossings);
void ExtractZeroCrossingData(struct CSV_PACKET_HANDLER* phandler)
{
	char buffer[20];
	if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
		if (strlen(buffer)>2) {
			g_zero_crossings.x = atoi(&buffer[2]);					//read x zero crossing count
		}
	}
	if (GetNthCSV((char*)phandler->pbuf, 2, buffer, 20) == 0) {
		if (strlen(buffer)>2) {
			g_zero_crossings.y = atoi(&buffer[2]);					//read y zero crossing count
		}
	}
	if (GetNthCSV((char*)phandler->pbuf, 3, buffer, 20) == 0) {
		if (strlen(buffer)>2) {
			g_zero_crossings.z = atoi(&buffer[2]);					//read z zero crossing count
		}
	}
}
	

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////                                ///////////////////////////////////////
/////////////////////////////////////  RECEIVE FROM SU_TX FUNCTIONS  ///////////////////////////////////////
/////////////////////////////////////                                ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

int g_SU_TX_battmvolts = 0;
// handles the CSV commands received FROM THE TX MODULE
// EVERYTHING SIMPLY GETS PUT TO THE LOG FILE
void Process_SUTX_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[20];
	PutTimeStr(PORT_SDLOG);
	PortPutStr((char *)phandler->header,PORT_SDLOG,1);
	PortPutStr((char*)phandler->pbuf,PORT_SDLOG,1);
	PortPutChar('\n',PORT_SDLOG);
	

	if (strcmp((char *)phandler->header,SU_TX_ACK_CSV_HEADER) == 0) {		// RESPONSE TO A MESSAGE FROM MU VIA MURX MODULE
		traceS2("ACK from SUTX module:",(char *)phandler->pbuf);	
		g_SU_Tx_PingCount++;
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) 
		{
			if (strlen(buffer)) 
			{
				g_SU_TX_battmvolts = atoi(buffer);					//read battery voltage
			}
		}	
	}
	else if (strcmp((char *)phandler->header,SU_TX_NACK_CSV_HEADER) == 0) {		// RESPONSE TO A MESSAGE FROM MU VIA MURX MODULE
		traceS2("NACK from SUTX module:",(char *)phandler->pbuf);	
	}
	else if (strcmp((char *)phandler->header,SU_TX_PING_CSV_HEADER) == 0) {		// RESPONSE TO A PING MESSAGE FROM MU VIA MURX MODULE
		traceS2("Ping from SUTX module:",(char *)phandler->pbuf);	
		g_SU_Tx_PingCount++;
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) 
		{
			if (strlen(buffer)) 
			{
				g_SU_TX_battmvolts = atoi(buffer);					//read battery voltage
			}
		}	
	}
	else {
		traceS("SUTX_CSV unexpected response: ");
		traceS2((char *)phandler->header,(char *)phandler->pbuf);	
	}
	
}	



// call this first before calling ProcessSUTXStream for the first time.
void InitSUTXStream()
{
	init_CSV_packethandler(&g_SU_VLF_TX_Uart_Packet,SU_VLF_TX_Uart_cmdbuf,MAX_SU_VLF_TX_PACKET_SIZE);
}



// parses the incoming messages from the SUTX on the UART - InitSUTXStream() must be called at least once before this function is called	
void ProcessSUTXStream()
{
	struct CSV_PACKET_HANDLER* pCSV = &g_SU_VLF_TX_Uart_Packet;
	unsigned char rxbyte;
	
	while (PortChReady(SUTX_UART_PORT)) {
		rxbyte = PortGetCh(SUTX_UART_PORT); 
//		PortPutChar('T',PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar('x',PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar(rxbyte,PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar(' ',PC_PORT);		// todo comment out - this is for debugging
		if ((rxbyte !=0)) {
			rxbyte = process_CSV_packetbyte(pCSV, rxbyte);
			if (pCSV->packet_timeouts > 0) {
				g_SysError.U4_RXError = 1;
				pCSV->packet_timeouts=0;
			}
			if (pCSV->m_PacketStage == CSV_PACKET_READY) {
				Process_SUTX_CSV_Packet(pCSV, SUTX_UART_PORT); 
				reset_CSV_packethandler(pCSV);
			}
		}
	}
}


#define MAX_WI_BUF_SIZE 512
unsigned char g_wi_buf[MAX_WI_BUF_SIZE];		// this should be the maximum of several command structures
struct CSV_PACKET_HANDLER g_Wi_CSV_Packet;

// returns TRUE if a character was received else FALSE	
void Init_SU_Wireless_Stream()
{
	init_CSV_packethandler(&g_Wi_CSV_Packet, g_wi_buf, MAX_WI_BUF_SIZE);
}

// parses the incoming messages from the SUTx ON THE WIRELESS - Init_SU_Wireless_Stream() must be called at least once before this function is called	
void Process_SU_Wireless_Stream()
{
	struct CSV_PACKET_HANDLER* pCSV = &g_Wi_CSV_Packet;
	unsigned char rxbyte;
	
	while (PortChReady(PORT_WIRELESS)) {
		rxbyte = PortGetCh(PORT_WIRELESS); 
//		PortPutChar('T',PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar('x',PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar(rxbyte,PC_PORT);		// todo comment out - this is for debugging
//		PortPutChar(' ',PC_PORT);		// todo comment out - this is for debugging
		if ((rxbyte !=0)) {
			rxbyte = process_CSV_packetbyte(pCSV, rxbyte);
			if (pCSV->packet_timeouts > 0) {
				g_SysError.wireless_timeout = 1;
				pCSV->packet_timeouts=0;
			}
			if (pCSV->m_PacketStage == CSV_PACKET_READY) {
				Process_SUTX_CSV_Packet(pCSV, PORT_WIRELESS); 
				reset_CSV_packethandler(pCSV);
			}
		}
	}
}
