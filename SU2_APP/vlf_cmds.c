#include "vlf_cmds.h"
#include "su_slcd.h"
#include "config.h"
#include "csv_packet.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "trace.h"
#include "putfunctions.h"


////////////////////////////////////// Some globals used by the parsing functions ///////////////////
#define MAX_MURX_PACKET_SIZE 128
struct CSV_PACKET_HANDLER g_murx_Uart_Packet;
unsigned char murx_Uart_cmdbuf[MAX_MURX_PACKET_SIZE+2];		// this should be the maximum MESSAGES SIZE FROM THE MURX module

#define MAX_MUTX_PACKET_SIZE 128
struct CSV_PACKET_HANDLER g_muTx_Uart_Packet;
unsigned char mutx_Uart_cmdbuf[MAX_MUTX_PACKET_SIZE+2];		// this should be the maximum MESSAGES SIZE FROM THE MURX module

unsigned int g_muRx_ResponseCount = 0;	// counter for number of generic response packets -helps determine if an asychronous response arrived

/////////////////////////////////////// private prototypes /////////////////////////////////////
void Process_MURX_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port);
void Process_MUTX_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port);

void clearMURXResponseCount() { g_muRx_ResponseCount=0; };
unsigned int getMURXResponseCount() { return g_muRx_ResponseCount; };

static U8 sequence = 0;

void vlf_SetNewSearchSequence()
{
	sequence++;		// incrementing the search sequence creates a new search so that all MU units will respond again
}

static U32 m_lastMUID = 0;
static int m_lastTXAntenna = TX_ANTENNA_NONE;

// sets the ID for any messages coming from the MURX module which aren't identifed by an ID in the message from the MU
void vlf_SetMUID(U32 id) { m_lastMUID = id;};

// sends a broadcast command to search for MU units

#define TEST_ON_PC_PORT
void vlf_SendDetectMsg(int antenna)
{
	char buffer[64];
	g_SysError.MU_DecodeError = 0;		// reset the fact that any decode errors occurred
#ifdef TEST_ON_PC_PORT
	int port = PC_PORT;
#else
	int port = MUTX_PORT;
#endif
	m_lastTXAntenna = antenna % TX_ANTENNA_COUNT;
	
	sprintf(buffer,"$MUDET,%d,%u,%u,%u,%d\r\n",
		g_SuSettings.protocol,
		g_SUConfig.id,				// Contains a unique SU ID
		0x80,						// Sets the scope of the detect operation - high bit set is new search
		sequence,					// Gives a unique sequence for the detect operation 
		m_lastTXAntenna			// contains the antenna index
		);
	PortPutStr(buffer,port,1);	

}

// send a mask command to tell the MU unit with the ID to no longer respond to this sequence number
void vlf_SendMaskMsg(U32 id, int antenna)
{
	char buffer[64];
#ifdef TEST_ON_PC_PORT
	int port = PC_PORT;
#else
	int port = MUTX_PORT;
#endif
	m_lastTXAntenna = antenna % TX_ANTENNA_COUNT;
	
	sprintf(buffer,"$MUMSK,%d,%u,%lu,%u,%d\r\n",
		g_SuSettings.protocol,
		g_SUConfig.id,				// Contains a unique SU ID
		id,							// The id of the MU unit to mask 
		sequence,					// Gives a unique sequence for the detect operation 
		m_lastTXAntenna			// contains the antenna index
		);
	PortPutStr(buffer,port,1);	
}


// send a locate command to tell the MU unit with the ID to go into locate mode
// Seconds is the time to transmit LOC signal for. 
// A value of zero indicates a transmission
// that is prolonged by the SU periodically transmitting a signature as long as it
// wants the MU to carry on transmitting
void vlf_SendLocateMsg(U32 id, U8 seconds, int antenna)
{
	char buffer[64];
#ifdef TEST_ON_PC_PORT
	int port = PC_PORT;
#else
	int port = MUTX_PORT;
#endif
	m_lastTXAntenna = antenna % TX_ANTENNA_COUNT;
	
	sprintf(buffer,"$MULOC,%d,%lu,%u,%d\r\n",
		g_SuSettings.protocol,
		id,						// The id of the MU unit to put into locate mode 
		seconds,				// Contains the number of seconds - 0 means that it is on demand 
		m_lastTXAntenna			// contains the antenna index
		);
	PortPutStr(buffer,port,1);	
	
}


int vld_SetMURXMode(int mode)
{
	char buffer[32];
#ifdef TEST_ON_PC_PORT
	int port = PC_PORT;
#else
	int port = MURX_PORT;
#endif
	int error = 0;
	
	switch (mode) {
		case MURX_MODE_STANDBY:
			PortPutCRStr("MUMOD,Standby",port,1);	
			break;
		case MURX_MODE_DECODE:
			sprintf(buffer,"$MUMOD,Decode,%d",g_SuSettings.protocol);
			PortPutCRStr(buffer,port,1);	
			break;
		case MURX_MODE_LOCATE:
			sprintf(buffer,"$MUMOD,Decode,%d",g_SuSettings.protocol);
			PortPutCRStr(buffer,port,1);	
			break;
		default:
			error = 1;
			break;
	}
	return error;
}


// call this first before calling ProcessMURXStream for the first time.
void InitMURXStream()
{
	init_CSV_packethandler(&g_murx_Uart_Packet,murx_Uart_cmdbuf,MAX_MURX_PACKET_SIZE);
}


// parses the incoming messages from the murx - InitMURXStream() must be called at least once before this function is called	
void ProcessMURXStream()
{
	int port = MURX_PORT;
	struct CSV_PACKET_HANDLER* pCSV = &g_murx_Uart_Packet;
	unsigned char rxbyte;
	
	while (PortChReady(port)) {
		rxbyte = PortGetCh(port); 
		if ((rxbyte !=0)) {
			rxbyte = process_CSV_packetbyte(pCSV, rxbyte);
			if (pCSV->packet_timeouts > 0) {
				g_SysError.U4_RXError = 1;
				pCSV->packet_timeouts=0;
			}
			if (pCSV->m_PacketStage == CSV_PACKET_READY) {
				Process_MURX_CSV_Packet(pCSV, port); 
				reset_CSV_packethandler(pCSV);
			}
		}
	}
}


// handles the CSV command
void Process_MURX_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{
	char buffer[20];
	MuData data;
	SU_DataResetUnit(&data);	// resets all values to not initialized	
	data.id = m_lastMUID;		// by default, all messages without an ID are sent to the last one
	data.antenna = m_lastTXAntenna;

	PutTimeStr(PORT_SDLOG);
	//PortPutStr(BuildTimeString(),PORT_SDLOG,1);
	//PortPutChar(',',PORT_SDLOG);
	PortPutStr((char *)phandler->header,PORT_SDLOG,1);
	PortPutStr((char*)phandler->pbuf,PORT_SDLOG,1);
	PortPutChar('\n',PORT_SDLOG);
	
	if (strcmp((char *)phandler->header,"$MUDET") == 0) {		// RESPONSE TO A DETECT MESSAGE FROM MU VIA MURX MODULE
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
				data.id = atol(buffer);
				SU_UpdateUnit(&data);				// add ID OF MU UNIT
			}
		}	
		if (GetNthCSV((char*)phandler->pbuf, 2, buffer, 20) == 0) {
			if (strlen(buffer)) {
				data.murx_power = atoi(buffer);	// set power as seen by su unit
			}
		}
		SU_UpdateUnit(&data);				// update the unit's information
//		traceS2("processed command FROM MURX:",(char *)phandler->header);	
	}
	else if (strcmp((char *)phandler->header,"$MURSP") == 0) {				// RESPONSE FROM MU VIA MURX MODULE
		g_muRx_ResponseCount++;		// increment the number of responses from an MU
		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
			if (strlen(buffer)) {
				data.mobility = atoi(buffer);	//read mobility as seen by mu unit
			}
		}
		if (GetNthCSV((char*)phandler->pbuf, 2, buffer, 20) == 0) {
			if (strlen(buffer)) {
				data.murx_power = atoi(buffer);	// set power as seen by su unit
			}
		}
		SU_UpdateUnit(&data);				// update the unit's information
	
//		traceS2("processed command FROM MURX:",(char *)phandler->header);	
	}
	else if (strcmp((char *)phandler->header,"$MUERR") == 0) {			// DECODE ERROR MESSAGE
		g_SysError.MU_DecodeError = 1;
//		traceS2("processed command FROM MURX:",(char *)phandler->header);	
	}
	else {
		traceS2("MURX_CSV unknown command: ",(char *)phandler->header);	
	}

	
}	

// call this first before calling ProcessMURXStream for the first time.
void InitMUTXStream()
{
	init_CSV_packethandler(&g_muTx_Uart_Packet,mutx_Uart_cmdbuf,MAX_MUTX_PACKET_SIZE);
}

// parses the incoming messages from the muTx - InitMUTXStream() must be called at least once before this function is called	
void ProcessMUTXStream()
{
	int port = MUTX_PORT;
	struct CSV_PACKET_HANDLER* pCSV = &g_muTx_Uart_Packet;
	unsigned char rxbyte;
	
	while (PortChReady(port)) {
		rxbyte = PortGetCh(port); 
		if ((rxbyte !=0)) {
			rxbyte = process_CSV_packetbyte(pCSV, rxbyte);
			if (pCSV->packet_timeouts > 0) {
				g_SysError.U4_RXError = 1;
				pCSV->packet_timeouts=0;
			}
			if (pCSV->m_PacketStage == CSV_PACKET_READY) {
				Process_MUTX_CSV_Packet(pCSV, port); 
				reset_CSV_packethandler(pCSV);
			}
		}
	}
}

// handles the CSV command FROM THE TX MODULE
// EVERYTHING SIMPLY GETS PUT TO THE LOG FILE
void Process_MUTX_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int source_port)
{

	PutTimeStr(PORT_SDLOG);
	PortPutStr((char *)phandler->header,PORT_SDLOG,1);
	PortPutStr((char*)phandler->pbuf,PORT_SDLOG,1);
	PortPutChar('\n',PORT_SDLOG);
	

	if (strcmp((char *)phandler->header,"$MUTXA") == 0) {		// RESPONSE TO A DETECT MESSAGE FROM MU VIA MURX MODULE
//		if (GetNthCSV((char*)phandler->pbuf, 1, buffer, 20) == 0) {
//			if (strlen(buffer)) {
//				data.id = atol(buffer);
//				SU_UpdateUnit(&data);				// add ID OF MU UNIT
//			}
//		}	
//		if (GetNthCSV((char*)phandler->pbuf, 2, buffer, 20) == 0) {
//			if (strlen(buffer)) {
//				data.murx_power = atoi(buffer);	// set power as seen by su unit
//			}
//		}
//		SU_UpdateUnit(&data);				// update the unit's information
		traceS2("Response from MUTX module:",(char *)phandler->header);	
	}
	else {
		traceS2("MUTX unexpected response: ",(char *)phandler->header);	
	}
	
}	



//////////////////////////////// Below is the reference MU code for protocol V1///////////////////////////
/**
 * Handles a packet RX'd on the VLF interface.
 * This function is called when a packet is received on the vLF interface. The packet is assembled
 * from the Viterbi traceback buffer in EEPROM and checked for validity. Finally, it is processed and 
 * a repsonse generated if need be.
 */
 /*
void HandleVLFPktRxdEvt( void )
{
   U8 check_u8 = (U8)OPEN_FLAG ^ (OPEN_FLAG >> 8) ^ (packet_st.type_u8 << 4);
   U8 ii_u8;
   U8 tx_power_u8;

   AssemblePacket();

	// R.S. calculate the XORed sum of the data packet which should be equal to zero
   for ( ii_u8 = 0; 
         ii_u8 < UNCODED_RX_PAYLOAD_SIZE( packet_st.type_u8 );
         ii_u8++ ) {
      check_u8 ^= packet_st.u.data_au8[ ii_u8 ];
   }
   
   // R.S.  if checksum doesn't match then reject the packet and wait for another one
   if ( check_u8 != 0 ) {
      WaitForPacket();
      return;
   }

	// R.S.  assuming we got a legitimate packet - respond to it if necessary
   switch ( packet_st.type_u8 ) {
      case VLF_MU_MASK:
      case VLF_MU_SET_CLASS:
      case VLF_MU_PING:
      case VLF_MU_LOCATE:
      case VLF_MU_IDENTIFY: {
         if ( packet_st.u.mask_cmd_st.id_u32 != tid_u.all_u32 ) { // R.S.  check and see if it is for us
            WaitForPacket();
            return;
         }
//         tx_power_u8 = ((U16)tx_sig_u8 * (U16)TX_SIG_SCALE_FACTOR) >> 8;
         tx_power_u8 = eeprom_read( EEPROM_SD_OFFSETOF( tx_hw_power_u8 ) );

         EvtQueueWriteFromMain( VLF_REPLY_UNICAST );	// R.S.  if it is for us then queue an event to respond to it
      }
      break;

      case VLF_MU_DETECT:
      case VLF_MU_EVACUATE:
      break;

      default: {
         WaitForPacket();
         return;
      }
   }
      
	// R.S.  if it is for us, then do something if necessary
   switch ( packet_st.type_u8 ) {
      case VLF_MU_DETECT: {
         if ( su_id_u16 != packet_st.u.det_cmd_st.su_id_u16 ) {
            class_u8 = CLASS_UNFOUND;
         }
         if ( (detect_seq_u8 != packet_st.u.det_cmd_st.seq_u8)&&
              (class_u8 <= packet_st.u.det_cmd_st.scope_u8) ) {
            EvtQueueWriteFromMain(  VLF_REPLY_BROADCAST );
            packet_st.u.det_ev_st.id_u32 = tid_u.all_u32;
         }
         else {
            WaitForPacket();
            return;
         }
      }
      break;
         
      case VLF_MU_MASK: {
         if ( mu_state_u8 != SEARCH_IN_PROGRESS ) {
            mu_state_u8 = SEARCH_IN_PROGRESS;
            mu_state_timeout_u32 = (U32)MU_SEARCH_STATE_TIMEOUT;
         }
         detect_seq_u8 = packet_st.u.mask_cmd_st.seq_u8;
         su_id_u16 = packet_st.u.mask_cmd_st.su_id_u16;
         packet_st.u.resp_st.mobility_u8 = GetMobilityValue();
         packet_st.u.resp_st.power_u8 = tx_power_u8;
      }
      break;
         
      case VLF_MU_SET_CLASS: {
         class_u8 = packet_st.u.setcls_cmd_st.class_u8;
         su_id_u16 = packet_st.u.setcls_cmd_st.su_id_u16;
         packet_st.u.resp_st.mobility_u8 = GetMobilityValue();
         packet_st.u.resp_st.power_u8 = tx_power_u8;
      }
      break;

      case VLF_MU_LOCATE: {
         vlf_loc_mode_u8 = (packet_st.u.loc_cmd_st.secs_u8 == 0) ? VLF_LOC_CONTINUOUS : VLF_LOC_ONESHOT;
         vlf_loc_secs_u8 = packet_st.u.loc_cmd_st.secs_u8;
         vlf_loc_state_u8 = VLF_LOC_STATE_RESET;
         packet_st.u.resp_st.mobility_u8 = GetMobilityValue();
         packet_st.u.resp_st.power_u8 = tx_power_u8;
      }
      break;

      case VLF_MU_PING: {
         packet_st.u.resp_st.mobility_u8 = GetMobilityValue();
         packet_st.u.resp_st.power_u8 = tx_power_u8;
      }
      break;

      case VLF_MU_IDENTIFY: {
         mu_state_u8 = MU_IDENTIFY;
         mu_state_timeout_u32 = (U32)MU_IDENTIFY_STATE_TIMEOUT;
         packet_st.u.resp_st.mobility_u8 = GetMobilityValue();
         packet_st.u.resp_st.power_u8 = tx_power_u8;
      }
      break;

      case VLF_MU_EVACUATE:
         {
         if ( packet_st.u.evac_cmd_st.sig_u16 == EVAC_SIGNATURE )
            {
            if ( mu_state_u8 != EVACUATE_IN_PROGRESS ) {
               mu_state_u8 = EVACUATE_IN_PROGRESS;
               mu_state_timeout_u32 = (U32)MU_EVACUATE_STATE_TIMEOUT;
               }
            }
         WaitForPacket();
         return;
         }
      break;
      }

   check_u8 = (U8)OPEN_FLAG ^ (OPEN_FLAG >> 8) ^ (packet_st.type_u8 << 4);
   for ( ii_u8 = 0; 
         ii_u8 < UNCODED_TX_DATA_SIZE( packet_st.type_u8 );
         ii_u8++ ) {
      check_u8 ^= packet_st.u.data_au8[ ii_u8 ];
   }
   
   packet_st.u.data_au8[ ii_u8 ] = check_u8;
}
*/

/**
 * Handles a reply broadcast command.
 * A random time period within the 5 second windows is waited for. The queued packet is then sent 
 * on the VLF interface.
 */
/*
void HandleVLFReplyBroadcastEvt( void )
{
   U8 rand_time_u8;
   srand( (U16)TMR1L );
   rand_time_u8 = (U8)(((U16)(rand() & 0xFF))*VLF_RAND_MULTIPLIER/VLF_RAND_DIVIDER);
   WatchStart( &rand_timer_st, rand_time_u8 );
   while ( !WatchIsExpired( &rand_timer_st ) )
      CLRWDT();
      
   SendPacket();	// R.S. Sends whatever packet was in memory
}
*/

