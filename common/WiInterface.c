/********************************************************************

********************************************************************/


/************************ HEADERS **********************************/
#include "debugWireless.h"
#include "MRF24J40.h"
#include "Wi.h"
#include "wiInterface.h"
#include "string.h"
#include "string.h"
#include "stdio.h"
#include "port.h"
//#include "wicnint.h"

/************************ VARIABLES ********************************/
MAC_ADDR8 g_myMACAddress = 
{{
	DEF_EUI_0,
	DEF_EUI_1,
	DEF_EUI_2,
	DEF_EUI_3,
#ifdef SU_APP
	MINERADIO_SU_CONTROLLER,
#elif defined(SU_TX)
	MINERADIO_SU_TX,
#else
#error INVALID DEVICE
#endif	
	MINERADIO_OUI_5,
	MINERADIO_OUI_6,
	MINERADIO_OUI_7
	}};

MAC_ADDR8 g_destMACAddress = 
{{
	BROADCAST_EUI_0,
	BROADCAST_EUI_1,
	BROADCAST_EUI_2,
	BROADCAST_EUI_3,
#ifdef SU_APP
	MINERADIO_SU_TX,
#elif defined(SU_TX)
	MINERADIO_SU_CONTROLLER,
#else
#error INVALID DEVICE
#endif	
	MINERADIO_OUI_5,
	MINERADIO_OUI_6,
	MINERADIO_OUI_7
}};
	
MAC_ADDR8 g_defaultTextMessageMACAddress = 
{{
	0xff,
	0xff,
	0xff,
	0xff,
	MINERADIO_SU_CONTROLLER,
	MINERADIO_OUI_5,
	MINERADIO_OUI_6,
	MINERADIO_OUI_7
}};	
	

#ifdef SUPPORT_SECURITY
const unsigned char mySecurityKey[16] = {SECURITY_KEY_00, SECURITY_KEY_01, SECURITY_KEY_02, SECURITY_KEY_03, SECURITY_KEY_04,
	SECURITY_KEY_05, SECURITY_KEY_06, SECURITY_KEY_07, SECURITY_KEY_08, SECURITY_KEY_09, SECURITY_KEY_10, SECURITY_KEY_11, 
	SECURITY_KEY_12, SECURITY_KEY_13, SECURITY_KEY_14, SECURITY_KEY_15};
const unsigned char mySecurityLevel = SECURITY_LEVEL;
const unsigned char myKeySequenceNumber = KEY_SEQUENCE_NUMBER;
#endif

ROM unsigned char myManufacturerString[]="Mineradio Systems Inc.";
ROM BYTE RSSIlookupTable [256] ={90,89,88,88,88,87,87,87,87,86,86,86,86,85,85,85,85,85,84,84,84,84,84,83,83,83,83,82,82,82,82,82,81,81,81,81,81,80,80,80,80,80,80,79,79,79,79,79,78,78,78,78,78,77,77,77,77,77,76,76,76,76,76,75,75,75,75,75,74,74,74,74,74,73,73,73,73,73,72,72,72,72,72,71,71,71,71,71,71,70,70,70,70,70,70,69,69,69,69,69,68,68,68,68,68,68,68,67,67,67,67,66,66,66,66,66,66,65,65,65,65,64,64,64,64,63,63,63,63,62,62,62,62,61,61,61,61,61,60,60,60,60,60,59,59,59,59,59,58,58,58,58,58,57,57,57,57,57,57,56,56,56,56,56,56,55,55,55,55,55,54,54,54,54,54,54,53,53,53,53,53,53,53,52,52,52,52,52,51,51,51,51,51,50,50,50,50,50,49,49,49,49,49,48,48,48,48,47,47,47,47,47,46,46,46,46,45,45,45,45,45,44,44,44,44,43,43,43,42,42,42,42,42,41,41,41,41,41,41,40,40,40,40,40,40,39,39,39,39,39,38,38,38,37,36,35};

/********************** FUNCTION PROTOTYPES *************************/
void BoardInit(void);
void TransmitWIReport(unsigned char report_type, char* pbuf, unsigned char dlen, MAC_ADDR8* destAddress);
int ConvertRSSItoDBM(unsigned char rssi);

/********************** CONSTANTS/DEFINITIONS ***********************/

/* MAX SPI CLOCK FREQ SUPPORTED FOR MIWI TRANSCIEVER */
#define MAX_SPI_CLK_FREQ_FOR_MIWI            (20000000ul)

long g_MiWi_BytesProcessed = 0;

unsigned char miwitx_fifo[MIWI_TX_FIFOSIZE];
struct CQUEUE miwitxq;

unsigned char MiWi_rx_fifo[MiWi_RX_FIFOSIZE];
struct CQUEUE MiWirxq;

////////////////////////////////////////// VARIABLES ////////////////////////////////////////////
BYTE RxBuffer[RX_BUFFER_SIZE];



int init_Wireless()
{   
    if (!g_UnitFlags.WirelessEnabled) {
    	DebugWireless_PutString((ROM char*)"Wireless MiWi disabled!\r\n");
    	return 0;
    }	
    BoardInit();    
    WiInit();		//	ConsolePutROMString((ROM char*)"Initialzing MiWi 1.0 \r\n");
	CQueue_init(&miwitxq,miwitx_fifo,MIWI_TX_FIFOSIZE);
	CQueue_init(&MiWirxq,MiWi_rx_fifo,MiWi_RX_FIFOSIZE);

	ClearNetworkTable();
	
	SetChannel(DEFAULT_CHANNEL);
	DiscardPacket();			// resets the packet receive to the correct promiscuous mode
	
    return 0;
}



void setWiAddress(MAC_ADDR8* dest, unsigned char type, unsigned long id)	
{
	dest->v[0] = id & 0xff;
	dest->v[1] = (id >> 8) & 0xff;
	dest->v[2] = (id >> 16) & 0xff;
	dest->v[3] = (id >> 24) & 0xff;
	dest->v[4] = type;
	dest->v[5] = MINERADIO_OUI_5;
	dest->v[6] = MINERADIO_OUI_6;
	dest->v[7] = MINERADIO_OUI_7;
}

/*
void setDestMACAddress(MAC_ADDR8* id)	
{
	g_destMACAddress.v[0] = id->v[0];
	g_destMACAddress.v[1] = id->v[1];
	g_destMACAddress.v[2] = id->v[2];
	g_destMACAddress.v[3] = id->v[3];
	g_destMACAddress.v[4] = id->v[4];		
	g_destMACAddress.v[5] = MINERADIO_OUI_5;
	g_destMACAddress.v[6] = MINERADIO_OUI_6;
	g_destMACAddress.v[7] = MINERADIO_OUI_7;
}	
*/

//unsigned char* getLastSourceLongAddress() { return g_LastSourceLongAddress; };
MAC_ADDR8* getMyMACAddress() { return &g_myMACAddress; };

char* GetMyFormattedMACAddress()
{
	static char sz_mac[20];
	sprintf(sz_mac,"%02x%02x%02x%02x%02x%02x%02x%02x",
		g_myMACAddress.v[7] & 0xff,
		g_myMACAddress.v[6] & 0xff,
		g_myMACAddress.v[5] & 0xff,
		g_myMACAddress.v[4] & 0xff,
		g_myMACAddress.v[3] & 0xff,
		g_myMACAddress.v[2] & 0xff,
		g_myMACAddress.v[1] & 0xff,
		g_myMACAddress.v[0] & 0xff
	);
	return sz_mac;	
}


char* GetFormattedMACAddress(MAC_ADDR8* address)
{
	static char sz_mac[20];
	sprintf(sz_mac,"%02x%02x%02x%02x%02x%02x%02x%02x",
		address->v[7] & 0xff,
		address->v[6] & 0xff,
		address->v[5] & 0xff,
		address->v[4] & 0xff,
		address->v[3] & 0xff,
		address->v[2] & 0xff,
		address->v[1] & 0xff,
		address->v[0] & 0xff
	);
	return sz_mac;	
}

char* FormatDeviceType(unsigned char device)
{
	if (device == MINERADIO_SU_TX)
		return "SU Transmitter";
	else if (device == MINERADIO_SU_RX)
		return "SU Receiver";
	else if (device == MINERADIO_SU_CONTROLLER)
		return "SU Controller";
	return "Unknown";
}



/*
int MiWi_tx_enque(unsigned char *pbuf,int len)
{
	int i;
	for (i=0;i< len;i++) {
		if (CQueue_isFull(&miwitxq)) {
			g_SysError.Wireless_Overun = 1;
			 return 1;
		}
		else {
			CQueue_enqueue(&miwitxq, *pbuf++);
		}
	}
	return 0;	
}	

int MiWi_tx_deque(unsigned char *pbuf,int len)
{
	int i;
	for (i=0;i<len;i++) {
		if (CQueue_isEmpty(&miwitxq))
			break;
		CQueue_dequeue(&miwitxq, (unsigned char*)pbuf++);
	}
	return i;
}
*/

/*
// gets up to len characters or up to and including the first line feed - whichever comes first
int MiWi_dequeString(char *pbuf,int len)
{
	int i;
	for (i=0;i<len;i++) {
		if (CQueue_isEmpty(&miwitxq))
			break;
		CQueue_dequeue(&miwitxq, (unsigned char*)&pbuf[i]);
		if (pbuf[i] == '\n') {
			i++;
			break;
		}
	}
	return i;	
}	

int MiWi_isTxQueueEmpty()
{
	return CQueue_isEmpty(&miwitxq);
}	

int GetMiWi_TXpCent()
{
	if (miwitxq.len==0)
		return 0;
	else
		return (int)(((long)miwitxq.peakhold * 100)/miwitxq.len);	
}
int MiWi_GetTxBufFreeSize() { return	CQueue_freesize(&miwitxq);};
unsigned int MiWi_GetTxBufSize() { return	CQueue_size(&miwitxq);};

*/


void ResetMiWipeaks() {	miwitxq.peakhold=0; MiWirxq.peakhold=0; };

/*
int MiWi_PutChar(char ch)
{
	return MiWi_tx_enque((unsigned char*)&ch,1);
}
*/

int MiWi_GetRxBufFreeSize() { return	CQueue_freesize(&MiWirxq);};
unsigned int MiWi_GetRxBufSize() { return	CQueue_size(&MiWirxq);};

char MiWi_isCharReady() {
	if (CQueue_isEmpty(&MiWirxq))
		return 0;
	else
		return 1;
}


int MiWi_rx_enque(unsigned char *pbuf,int len)
{
	int i;
	for (i=0;i< len;i++) {
		if (CQueue_isFull(&MiWirxq)) {
			g_SysError.Wireless_Overun = 1;
			 return 1;
		}
		else {
			CQueue_enqueue(&MiWirxq, *pbuf++);
		}
	}
	return 0;	
}	

int MiWi_rx_deque(unsigned char *pbuf,int len)
{
	int i;
	for (i=0;i<len;i++) {
		if (CQueue_isEmpty(&MiWirxq))
			break;
		CQueue_dequeue(&MiWirxq, pbuf++);
	}
	return i;
}

char MiWi_GetChar()
{
	unsigned char retbyte;
	if (MiWi_rx_deque(&retbyte,1))
		return retbyte;
	else
		return 0;	
}

int MiWi_isRxQueueEmpty()
{
	return CQueue_isEmpty(&MiWirxq);
}	


int Get_MiWi_RXpCent()
{
	if (MiWirxq.len==0)
		return 0;
	else {
		return (int)CALC_PERCENTAGE(MiWirxq.peakhold,MiWirxq.len);
	}
}

int GetMiWi_RXpeak() { return MiWirxq.peakhold; };


int ConvertRSSItoDBM(unsigned char rssi)
{
	return -(RSSIlookupTable[rssi]);
}

void WaitForWiTXNotBusy(int ms);

// waits for the wireless TX bit to be clear indicating it is ok to send
void WaitForWiTXNotBusy(int ms)
{
 	unsigned long long tick;
 	tick = g_tick;
  	// make this into a non-blocking wait for TRANSMIT TO BE COMPLETE
    while((MiWiStatus.bits.TX_BUSY) && ((g_tick - tick) < ms)) {	// WAIT FOR up to 40 ms for ANY TRANSMIT IN PROCESS TO FINISH
          if(RF_INT_PIN == 0) {
              RFIF = 1;
          }
          Idle();
    }    	
    if (MiWiStatus.bits.TX_BUSY)
    {
	    g_SysError.Wireless_Overun = 1;
   		MiWiStatus.bits.TX_BUSY = 0;
	}
}


unsigned long g_wireless_counter = 0l;
unsigned long tx_elapsed_ms = 0;
unsigned long long start_tx_tick;
unsigned char  WI_SEND_buffer[MAXIMUM_WIRELESS_DATA_PACKET+2];        	

//////////////////////////////////////////////////////////////////////////////////////////////////////        
////////////////////////////////////////////// SEND DATA IF NECESSARY ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////   
#ifdef OLD_CODE
void process_WirelessTasks(char command, char *pbuf, int len, MAC_ADDR8* pDestAddress)
{
	if (!g_UnitFlags.WirelessEnabled) {
    	return;
    }   

	switch (command) {
		case USER_REPORT_BINARY_PACKET:		// send the prepared binary packet
		    // make sure that the TX is not busy  
	       	TransmitWIReport(USER_REPORT_BINARY_PACKET, pbuf, len, pDestAddress);
			break;
	    case SEND_DATA_STREAM:				// prepare and send a binary packet consiting of a stream of characters
/*			{
			    // extracts data from the data stream and sends it as a packet over the wireless
				int count = MiWi_tx_deque(WI_SEND_buffer.buffer,MAXIMUM_WIRELESS_DATA_PACKET);	// get up to 80 bytes from buffer or string
			 	if (count) {
				  	// package up the stream of bytes into a packet
					static struct PACKET_SEQUENCE sequence = {0};
					WI_SEND_buffer.header.Bytelen = count;
					WI_SEND_buffer.header.StuffWithEsc = 0;	// Don't need to byte stuff if sending over wireless
					WI_SEND_buffer.header.Command = CMD_TEXT_MSG;
					WI_SEND_buffer.header.Direction = BINARY_CMD_SET;
					WI_SEND_buffer.header.DataId=sequence.id++;
					memcpy(WI_SEND_buffer.header.Source,g_myMACAddress.v,5);
					TransmitWIReport(USER_REPORT_BINARY_PACKET, (char*)&WI_SEND_buffer, count, pDestAddress);
			  	}
			}	    	
*/
	   	default:
	   		break;         
	} 
}
#endif

void checkFor_WirelessMessages()
{	
    BYTE count;
    char RSSI_VAL,LQI_VAL;	
	BYTE destDeviceType;
	BYTE sourceDeviceType;
	WI_Packet* pPacket;
	BYTE bForMe = FALSE;
	BYTE bBroadCastEUI = FALSE;
//	int n;
	
		

    if(RF_INT_PIN == 0) {
        RFIF = 1;    //set the interrupt flag just in case the interrupt was missed
    }
    if (RFIE == 0)
    	RFIE = 1;	// enable interrupts if they somehow have become disabled.
   
	if (MiWiStatus.bits.TX_BUSY)
		WaitForWiTXNotBusy(40);		// wait up to 40 ms for the TX to become not busy if it is busy

    //if there is a message pending 
    char PacketReady = 0;
    if (MiWiStatus.bits.RX_PENDING == 1) {         
        RFIE = 0;          //disable the RF interrupt TO PREVENT A NEW INTERRUPT FROM CHANGING THE STATE
        PHYSetShortRAMAddr(WRITE_BBREG1, 0x04);        		// RX decode symbol sign inverted    
        RxBuffer[0] = PHYGetLongRAMAddr(0x300) + 2;        //get the size of the packet 

        if(RxBuffer[0]<RX_BUFFER_SIZE)					// IF THE SIZE IS REASONABLE
        {
            PHYSetShortRAMAddr(WRITE_RXMCR,0x20);  //disable the ACKs until the buffer is cleared
            MiWiStatus.bits.RX_ENABLED = 0;        //Disable RX of packets until this one is cleared
            MiWiStatus.bits.RX_PENDING = 0;        //Clear the pending flag indicating that we removed the packet from the buffer
            PacketReady = 1;
            //copy all of the data from the FIFO into the RxBuffer
            for(count=1;count<=RxBuffer[0]+8;count++)			//r.s. - added 4 bytes to get link quality and rssi
            {
                RxBuffer[count] = PHYGetLongRAMAddr(0x300+count);
            }
			RSSI_VAL = PHYGetLongRAMAddr(0x300+RxBuffer[0]-0);
			LQI_VAL = PHYGetLongRAMAddr(0x300+RxBuffer[0]-1);
        }
        else {  //else it was a larger packet than we can receive SO flush it
	    	PHYSetShortRAMAddr(WRITE_RXFLUSH,0b00000101); 	// Sets to Data only Frames and Reset the RXFIFO Address Pointer		        
            										// RXFIFO data is not modified. Bit is automatically cleared to ‘0’ by hardware
            MiWiStatus.bits.RX_PENDING = 0;
            MiWiStatus.bits.RX_ENABLED = 1;
        }        
        RFIE = 1;        							// RE-ENABLE INTERRUPTS
        PHYSetShortRAMAddr(WRITE_BBREG1, 0x00);     // get out of loopback mode
	    
	    DiscardPacket();	// allow the next one to be flagged since this one is already buffered if it was good 
    }
    
    if(PacketReady)  {      //if we have a packet in the buffer we can process the packet   	    
        BYTE addrMode;
        MAC_FRAME_CONTROL macFrameControl;        
		pPacket = (WI_Packet*)&RxBuffer[1];
        macFrameControl.Val = RxBuffer[1];
        addrMode = RxBuffer[2] & 0xCC;

        // we only care about long source and dest addressing for MINERADIO devices        
 		if ((pPacket->hdr.addr_control == 0xCC) && 
  			(pPacket->hdr.fc.bits.frameType == FRAME_TYPE_DATA) &&    	//We only respond to data packets	
			(RxBuffer[21]==MINERADIO_OUI_5) && 							// SOURCE must match our OID
 			(RxBuffer[22]==MINERADIO_OUI_6) && 
 			(RxBuffer[23]==MINERADIO_OUI_7) &&
 			(RxBuffer[11]==MINERADIO_OUI_5) && 							// DEST must match our OID
 			(RxBuffer[12]==MINERADIO_OUI_6) && 
 			(RxBuffer[13]==MINERADIO_OUI_7) 
 			)
	 	{	
       	
			destDeviceType = pPacket->hdr.dest.v[4];
			sourceDeviceType = pPacket->hdr.source.v[4];
		    
			if ((pPacket->hdr.dest.v[0] == g_myMACAddress.v[0]) &&
				(pPacket->hdr.dest.v[1] == g_myMACAddress.v[1]) &&
				(pPacket->hdr.dest.v[2] == g_myMACAddress.v[2]) &&
				(pPacket->hdr.dest.v[3] == g_myMACAddress.v[3]))
			{ 
				bForMe = TRUE; 
			}	
			
			if ((pPacket->hdr.dest.v[0] == (BROADCAST_EUI & 0xff)) && 
				(pPacket->hdr.dest.v[1] == ((BROADCAST_EUI >> 8) & 0xff)) &&
				(pPacket->hdr.dest.v[2] == ((BROADCAST_EUI >> 16) & 0xff)) && 
				(pPacket->hdr.dest.v[3] == ((BROADCAST_EUI >> 24) & 0xff))) 
			{
				bBroadCastEUI = TRUE;
	 		}

#if defined(SU_APP)
		    g_wireless_counter++;		// this is incremented in order to tell whether there is activity
 			if (destDeviceType==MINERADIO_SU_CONTROLLER) // if this message is intended for Controllers like  myself
		 	{	
			 	// since we don't expect messages from far away distances, 
			 	// we can listen to all messages sent to the default SEND_PC_EUI address or to our own address
			 	if (bBroadCastEUI || bForMe)
				{					    
	                AddNodeToNetworkTable(&pPacket->hdr.source, LQI_VAL, RSSI_VAL);
					
					if (pPacket->hdr.report_type == USER_REPORT_TEXT_PACKET) // if it is a text packet, insert it into the RX buffer so the command request is queued
					{
						MiWi_rx_enque(pPacket->payload, pPacket->hdr.payload_len);	// stuff the characters into a queue
			    	}					
				}			 	
	 		}
	 		
#elif defined(SU_TX)
		    g_wireless_counter++;		// this is incremented in order to tell whether there is activity
 			if (destDeviceType==MINERADIO_SU_TX) // if this message is intended for SU_TX modules like  myself
		 	{	
			 	// since we don't expect messages from far away distances, 
			 	// we can listen to all messages sent to the default SEND_PC_EUI address or to our own address
			 	if (bBroadCastEUI || bForMe)
				{					    
	                AddNodeToNetworkTable(&pPacket->hdr.source, LQI_VAL, RSSI_VAL);
					
					if (pPacket->hdr.report_type == USER_REPORT_TEXT_PACKET) // if it is a text packet, insert it into the RX buffer so the command request is queued
					{
						MiWi_rx_enque(pPacket->payload, pPacket->hdr.payload_len);	// stuff the characters into a queue
			    	}					
				}			 	
	 		}

#else
#error INVALID DEVICE
#endif     
		}  
    }          

}

// appends a CR and limits message
void SendWirelessTextCRMsg(char* msg)
{
	char buffer[MAXIMUM_WIRELESS_DATA_PACKET];
    unsigned char count = min(strlen(msg),MAXIMUM_WIRELESS_DATA_PACKET-3);	// get up to 80 bytes from string
    memcpy(buffer,msg,count);
    buffer[count++]= '\r';
    buffer[count++]= '\n';
    buffer[count] = 0;
	TransmitWIReport(USER_REPORT_TEXT_PACKET, buffer, strlen(buffer), &g_destMACAddress);
}

void SendWirelessTextMsg(char* msg)
{
    unsigned char count = min(strlen(msg),MAXIMUM_WIRELESS_DATA_PACKET);	// get up to 80 bytes from string
 	if (count) 
	{
		TransmitWIReport(USER_REPORT_TEXT_PACKET, msg, count, &g_destMACAddress);
  	}
}


static void TransmitStart(BYTE destDeviceType, BOOL bBroadCastEUI)
{
	
   //mark the device as busy sending
	MiWiStatus.bits.TX_BUSY = 1;
	
	if 	(bBroadCastEUI)								// don't use an ack for broadcast messages
	{
	   MiWiStatus.bits.TX_PENDING_ACK = 0;
	   PHYSetShortRAMAddr(WRITE_TXNMTRIG,0b00000001);  //transmit packet without ACK requested 
	}
	else 
	{
	   MiWiStatus.bits.TX_PENDING_ACK = 1;       
	   PHYSetShortRAMAddr(WRITE_TXNMTRIG,0b00000101); 	// transmit with ACK requested	
	}
}

U8 IEEESeqNum = 0;
void TransmitWIReport(unsigned char report_type, char* pbuf, unsigned char dlen, MAC_ADDR8* destAddress)
{
    BYTE i;
    WI_Packet fm;
	char msgbuf[60];

	if (MiWiStatus.bits.TX_BUSY)
		WaitForWiTXNotBusy(40);		// wait up to 40 ms for the TX to become not busy if it is busy

    start_tx_tick = g_tick;	// flag the moment at which we start transmitting a packet.
    
    fm.hdr.fc.Val = 0;
    fm.hdr.fc.bits.frameType = FRAME_TYPE_DATA;		// data frame
    fm.hdr.fc.bits.securityEnabled = 0;
    fm.hdr.fc.bits.framePending = 0;
    fm.hdr.fc.bits.ACKRequest = 0;
    fm.hdr.fc.bits.intraPAN = 0;
    fm.hdr.addr_control = 0xCC;		// both long addresses
	
    fm.hdr.sequence = IEEESeqNum++;
    memcpy(&fm.hdr.source,&g_myMACAddress,sizeof(MAC_ADDR8));
    memcpy(&fm.hdr.dest,destAddress,sizeof(MAC_ADDR8));
	fm.hdr.destPanID.val = 0xffff;
	fm.hdr.srcPanID.val = 0xffff; 
	fm.hdr.payload_len = dlen;   
	fm.hdr.report_type = report_type;
    memcpy(fm.payload,pbuf,dlen);
    
    char *phdr = (char*)&fm;  
    PHYSetLongRAMAddr(0x000,sizeof(fm.hdr));
    int total_len = sizeof(fm.hdr) + fm.hdr.payload_len;
   	PHYSetLongRAMAddr(0x001,total_len);
    
#ifdef SUPPORT_SECURITY
// TODO - encrypt data before sending and adjust length of encrypted data
#endif

   	for(i=0;i<total_len;i++) {
       	PHYSetLongRAMAddr(i+2, phdr[i]); 
   	}

#if defined(SU_APP) || defined(SU_TX)   	
if (g_UnitFlags.debugWireless) 
{  	
	sprintf(msgbuf,"SendReportByLongAddress - %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
		destAddress->v[7],  //MSB dest long address 
		destAddress->v[6],  //dest long address 
		destAddress->v[5],  //dest long address 
		destAddress->v[4],  //dest long address 
		destAddress->v[3],  //dest long address 
		destAddress->v[2],  //dest long address 
		destAddress->v[1],  //dest long address 
		destAddress->v[0]
	);  
	DebugWireless_PutString(msgbuf);
}	
#endif

	BYTE destDeviceType = destAddress->v[4];	

	BOOL bBroadCastEUI = 
		((destAddress->v[0] == (BROADCAST_EUI & 0xff)) && 
		(destAddress->v[1] == ((BROADCAST_EUI >> 8) & 0xff)) &&
		(destAddress->v[2] == ((BROADCAST_EUI >> 16) & 0xff)) && 
		(destAddress->v[3] == ((BROADCAST_EUI >> 24) & 0xff)))?TRUE:FALSE; 
	
	TransmitStart(destDeviceType,bBroadCastEUI);	
	
}


	
//#endif


/*********************************************************************
 * Function:        void BoardInit( void )
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    Board is initialized for MiWi usage
 *
 * Overview:        This function configures the board for the PICDEM-z
 *                  MRF24J40 usage 
 *
 * Note:            This routine needs to be called before the function 
 *                  to initialize MiWi stack or any other function that
 *                  operates on the stack
 ********************************************************************/
 // make sure the following timeout reflects the time it takes to shift the 8 bits at clock speed
// SINCE PRESCALER IS 4:1, assume 8 bits * 4 clock cycles * processor MIPs * 2 for some safety
#define SPI_SHIFT_TIMEOUT (8*4*PROCESSOR_MIPS*2)
void BoardInit(void)
{
    RF_SPISTAT 	= 0x0000;			// Dispable SPI until we set it up
    RF_SPICLOCK = OUTPUT;			// CLOCK IS OUTPUT
    RF_SPIOUT 	= OUTPUT;              // define SDO1 as output (master or slave)
    RF_SPIIN 	= INPUT;               // define SDI1 as input (master or slave)

/*
bit 4-2 SPRE2:SPRE0: Secondary Prescale bits (Master mode)
111 = Secondary prescale 1:1
110 = Secondary prescale 2:1
101 = 3:1
100 = 4:1
011 = 5:1
010 = 6:1
001 = 7:1
000 = Secondary prescale 8:1
bit 1-0 PPRE1:PPRE0: Primary Prescale bits (Master mode)
11 = Primary prescale 1:1
10 = Primary prescale 4:1
01 = Primary prescale 16:1
00 = Primary prescale 64:1
*/
    
    RF_SPICON1 = 0b0000000100111110; // 8 bits, CKE=1, CPE=0, MST=1, 2PRESCALER=1:1, 1PRESCALER=4:1 = 4MHZ
//  RF_SPICON1 = 0b0000000100111101; // 8 bits, CKE=1, CPE=0, MST=1, 2PRESCALER=1:1, 1PRESCALER=16:1 = 1MHZ
    
/*In Enhanced Master mode, the SRMPT bit (SPIxSTAT<7>) may erroneously become set for
several clock cycles in the middle of a FIFO transfer,indicating that the shift register is empty when it is
not. This happens when both SPI clock prescalers are set to values other than their maximum
(SPIxCON<4:2> ? 000 and SPIxCON<1:0> ? 00).Work around Set SISEL2:SISEL0 (SPIxSTAT<4:2>) to ‘101’.
This configures the module to generate an SPI event interrupt whenever the last bit is shifted out
of the shift register. When the SPIxIF flag becomes set, the shift register is empty. */
	RF_SPISTATbits.SISEL = 0b101;   
    RF_SPISTATbits.SPIEN = 1;			// ENABLE THE SPI MODULE
    
    PHY_RESETn 		= 0;
    PHY_RESETn_TRIS = OUTPUT;
    PHY_CS 			= 1;
    PHY_CS_TRIS 	= OUTPUT;
    PHY_WAKE        = 1;
    PHY_WAKE_TRIS 	= OUTPUT;  

	RF_INT_PIN_TRIS = INPUT;	// 1. INTERUPT PIN is an input1. 
    RF_INT_EDGE 	= 1;		// 1 = Interrupt on negative edge on INT
	RFIF 			= 0;		// Clear the interrupt flag.
    RF_INT_PRIORITY = 5;		// SET INTERRUPT PRIORITY TO 5
	RFIE 			= 1;		// Enable interrupts 

    if(RF_INT_PIN == 0)
    {
        RFIF = 1;
    }
}

/////////////////////////////////// SPI ROUTINES //////////////////////////////////
void RF_SPIPut(BYTE v)
{
   	BYTE dummy;
   	unsigned int timeout = SPI_SHIFT_TIMEOUT;
   
    RF_SSPIF_BIT = 0;
   	dummy = RF_SSPBUF_REG;
    RF_SSPBUF_REG = v;
    while(RF_SSPIF_BIT == 0) {
	    timeout--;
	    if (timeout == 0) {			// make sure this is non-blocking
	    	g_SysError.wireless_timeout = 1;
	    	break;
	    }	
	};
}

BYTE RF_SPIGet(void)
{
   	RF_SPIPut(0x00);
    return RF_SSPBUF_REG; 
}

////////////////////////////////////// INTERRUPT ROUTINE ////////////////////////////////

#if (RF_INTERRUPT_SOURCE==4)
void _ISRFAST __attribute__((interrupt, auto_psv)) _INT4Interrupt(void)
#elif (RF_INTERRUPT_SOURCE == 0)
void _ISRFAST __attribute__((interrupt, auto_psv)) _INT0Interrupt(void)
#elif (RF_INTERRUPT_SOURCE == 2)
void _ISRFAST __attribute__((interrupt, auto_psv)) _INT2Interrupt(void)
#else
#error Unsupported INT in Winterface.c
#endif
{    
    MRF24J40_IFREG flags;        
    unsigned int value = RF_INT_PORT; // read the PORT register associated with the CN pin(s) to clear the mismatch condition

	if (RFIE && RFIF) //&& (value & 0x1000))		// check if pin for INT was source
	{
	    RFIF = 0;        
	    flags.Val=PHYGetShortRAMAddr(READ_INTSTAT);   //read the interrupt status register to see what caused the interrupt               
	    if(flags.bits.RF_TXIF) {  //if the TX interrupt was triggered clear the busy flag indicating the transmission was complete
			//DebugWireless_PutString((ROM char *)" TX interrupt triggered\r\n");
	        MiWiStatus.bits.TX_BUSY = 0;   
            if ((g_tick-start_tx_tick) > tx_elapsed_ms) {
		    	tx_elapsed_ms = g_tick-start_tx_tick; 	// keep a peak-hold on the max time it takes for a tx
    		}
            //failureCounter = 0;
	        
	        //if we were waiting for an ACK
	        if(MiWiStatus.bits.TX_SECURITY || MiWiStatus.bits.TX_PENDING_ACK)
	        {
	            BYTE_VAL results;                 
	            results.Val = PHYGetShortRAMAddr(READ_TXSTAT);   //read out the results of the transmission                  
	            if(results.bits.b0 == 1) {
	                //the transmission wasn't successful and the number
	                //of retries is located in bits 7-6 of TXSTAT
	            }
	            else
	            {
	                //transmission successful - clear that I am pending an ACK, already got it
	                MiWiStatus.bits.TX_PENDING_ACK = 0;                        
	            }
	        }
	        MiWiStatus.bits.TX_SECURITY = 0;               
	    	PHYSetShortRAMAddr(WRITE_RXFLUSH,0b00000101); 	// Sets to Data only Frames and Reset the RXFIFO Address Pointer		        
	    }           
	    else if(flags.bits.RF_RXIF)  {  //if the RX interrupt was triggered    // DebugWireless_PutString((ROM char *)" RX interrupt triggered\r\n");
	        if(MiWiStatus.bits.RX_ENABLED) { //If the part is enabled for receiving packets right now (not pending an ACK)
	            							//indicate that we have a packet in the buffer pending to be read into the buffer from the FIFO
	            MiWiStatus.bits.RX_PENDING = 1;
	        }
	        else {  //else if the RX is not enabled then we need to flush this packet 
	    		PHYSetShortRAMAddr(WRITE_RXFLUSH,0b00000101); 	// Sets to Data only Frames and Reset the RXFIFO Address Pointer		        
	        }//end of RX_ENABLED check
	    } //end of RXIF check
	    
	    else 
	    {
	    	PHYSetShortRAMAddr(WRITE_RXFLUSH,0b00000101); 	// Sets to Data only Frames and Reset the RXFIFO Address Pointer		        
	//	            DebugWireless_PutString((ROM char *)" RF interrupt ignored\r\n");
	    } //end of RXIF check	        	
	} //end of if(RFIE && RFIF)    
} //end of interrupt handler



