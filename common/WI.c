

/************************ HEADERS **********************************/

#include "debugWireless.h"
#include "Wi.h"
#include "MRF24J40.h"
#include "WiDefs.h"
#include "stdlib.h"
#include "stdio.h"
#include "port.h"
#include "timer.h"
#include "string.h"

/************************ VARIABLES ********************************/
extern void SendBinaryPacketOverWireless(unsigned char command, char* buf, unsigned char len, unsigned char id, MAC_ADDR8* dest);
#ifdef SUPPORT_SECURITY
	#define INPUT
	#define OUTPUT
	#define IOPUT
	
	extern ROM unsigned char mySecurityLevel;
	extern ROM unsigned char mySecurityKey[16];
	extern ROM unsigned char myKeySequenceNumber;
	DWORD_VAL OutgoingFrameCounter;
	
	BOOL DataEncrypt(IOPUT BYTE *Input, IOPUT BYTE *DataLen, INPUT BYTE *Header, INPUT BYTE HeaderLen);
	BOOL DataDecrypt(IOPUT BYTE *Input, IOPUT BYTE *DataLen, INPUT BYTE *Header, INPUT BYTE HeaderLen, char tempLongAddress[8]);
	
    DWORD_VAL	OutgoingFrameCount;
    DWORD_VAL IncomingFrameCount[NETWORK_TABLE_SIZE];
#endif	

//BYTE g_LastSourceLongAddress[8];
#ifdef SUPPORT_SECURITY
    BYTE tmpBuf[TX_BUFFER_SIZE];
#endif

//BYTE TxHeader;
//BYTE TxData;
//BYTE *pRxData;
//BYTE RxSize;
volatile MIWI_STATUS MiWiStatus;
WI_NetworkEntry gNetworkTable[NETWORK_TABLE_SIZE];
WORD_VAL myShortAddress;
WORD_VAL myPANID;
volatile BYTE failureCounter = 0;
BYTE currentChannel;

extern char* GetFormattedMACAddress(BYTE address[8]);

/************************ FUNCTIONS ********************************/

/*********************************************************************
 * Function:        void DiscardPacket(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    The RxBuffer gets flushed and the MRF24J40 is allowed
 *                  to receive again.
 *
 * Overview:        This function needs to be called to discard, clear
 *                  and reset the Rx module of the MRF24J40.  This 
 *                  function needs to be called after the user is done
 *                  processing a received packet
 ********************************************************************/
void DiscardPacket(void)
{
    RFIE = 0;		
    PHYSetShortRAMAddr(WRITE_RXMCR,0x01);		// SET TO Promiscuous mode TO RECEIVE ALL PACKETS 
//    PHYSetShortRAMAddr(WRITE_RXMCR,0x00);		// ENABLE NORMAL PACKETS (RESPONSE TO BEACON OR PART OF NETOWRK
    MiWiStatus.bits.RX_BUFFERED = 0;
    MiWiStatus.bits.RX_ENABLED = 1;
    RFIE = 1;
}




/*********************************************************************
 * Function:        BOOL SendTxBuffer(BOOL AckRequested)
 *
 * PreCondition:    TxBuffer is loaded with the data to send
 *
 * Input:           BOOL AckRequested - if called with TRUE then the ACK
 *                  requested bit will be set.  If FALSE then the ACK
 *                  requested bit will be clear.
 *
 * Output:          BOOL this will indicate if the transmission was 
 *                  successful (returns TRUE) or not (returns FALSE)
 *
 * Side Effects:    The TxBuffer pointers are reset
 *
 * Overview:        This function is used to transmit a packet in the
 *                  Tx buffer.  This function is a helper function for 
 *                  the stack and is not intended for user usage.
 ********************************************************************/
/*
#ifdef SUPPORT_SECURITY
	BOOL SendTxBuffer(BOOL AckRequested, BOOL SecurityOn)
#else	 
	BOOL SendTxBuffer(BOOL AckRequested)
#endif	
{
    while( MiWiStatus.bits.TX_BUSY )
    {
        if( RF_INT_PIN == 0 )
        {
            RFIF = 1;
        }    
    }
    
    //if(MiWiStatus.bits.TX_BUSY == 0)
    {
        BYTE i,j;
        
        #ifdef SUPPORT_SECURITY
		    if( SecurityOn )
    		{
	    		BYTE extraLen = 0;
		    	switch(mySecurityLevel)
			    {
				    case 1:
					    extraLen = 13;
    					break;
	    			case 2:
		    		case 5:
			    		extraLen = 29;
				    	break;
    				case 3:
	    			case 6:
		    			extraLen = 21;
			    		break;
				    case 4:
    				case 7:
	    				extraLen = 17;
		    			break;
				
			    }
    			if( TxData + extraLen > TxHeader )
	    		{
		    		DebugWireless_PutString((ROM char *)"Packet too big to send");
			    	return FALSE;
    			}

	    		TxBuffer[1] |= 0x01;
		    	TxData -= 11;
			    if( !DataEncrypt(&(TxBuffer[11]), &TxData, &(TxBuffer[1]), 10) )
    			{
	    			//DebugWireless_PutString((ROM char *)"Encryption error\r\n");
	            	return FALSE;
	            }
    			TxData += 11;
	    	}
        #endif        
        
        //write the packet length
        PHYSetLongRAMAddr(0,TX_BUFFER_SIZE-1-TxHeader);
        //write the header length
        PHYSetLongRAMAddr(1,TxData+TX_BUFFER_SIZE-1-TxHeader);

        
        i=2;
        
        //write the packet header
        while(TxHeader < (TX_BUFFER_SIZE - 1))
        {
            PHYSetLongRAMAddr(i++,TxBuffer[++TxHeader]);
        }
        TxHeader = TX_BUFFER_SIZE-1;
                
        //write the packet data
        for(j=0;j<TxData;j++)
        {
            PHYSetLongRAMAddr(i++,TxBuffer[j]);
        }
        TxData = 0;
    
        //mark the device as busy sending
        MiWiStatus.bits.TX_BUSY = 1;
        
        //if an ACK is required then indicated that
        if(AckRequested == TRUE)
        {    
            MiWiStatus.bits.TX_PENDING_ACK = 1;       
            PHYSetShortRAMAddr(WRITE_TXNMTRIG,0x05); 
        }
        else
        {
            MiWiStatus.bits.TX_PENDING_ACK = 0;
            PHYSetShortRAMAddr(WRITE_TXNMTRIG,0x01); 
        }
        return TRUE;
    }
    return FALSE;
}
*/

/*********************************************************************
 * Function:        void PHYSetLongRAMAddr(WORD address, BYTE value)
 *
 * PreCondition:    Communication port to the MRF24J40 initialized
 *
 * Input:           WORD address is the address of the LONG RAM address
 *                      that you want to write to
 *                  BYTE value is the value that you want to write to
 *                      that register
 *
 * Output:          None
 *
 * Side Effects:    The register value is changed
 *
 * Overview:        This function writes a value to a LONG RAM address
 ********************************************************************/
void PHYSetLongRAMAddr(WORD address, BYTE value)
{
    volatile BYTE tmpRFIE = RFIE;
    RFIE = 0;
    PHY_CS = 0;
    Nop();
    RF_SPIPut((((BYTE)(address>>3))&0x7F)|0x80);
    RF_SPIPut((((BYTE)(address<<5))&0xE0)|0x10);
    RF_SPIPut(value);
    Nop();
    PHY_CS = 1;
    RFIE = tmpRFIE;
}

/*********************************************************************
 * Function:        void PHYSetShortRAMAddr(BYTE address, BYTE value)
 *
 * PreCondition:    Communication port to the MRF24J40 initialized
 *
 * Input:           BYTE address is the address of the short RAM address
 *                      that you want to write to.  Should use the
 *                      WRITE_ThisAddress definition in the MRF24J40 
 *                      include file.
 *                  BYTE value is the value that you want to write to
 *                      that register
 *
 * Output:          None
 *
 * Side Effects:    The register value is changed
 *
 * Overview:        This function writes a value to a short RAM address
 ********************************************************************/
void PHYSetShortRAMAddr(BYTE address, BYTE value)
{
    volatile BYTE tmpRFIE = RFIE;
    RFIE = 0;	     //disabling the interrupt to the microcontroller
    PHY_CS = 0;      //Pull the Chip select signal to low  
    Nop();
    RF_SPIPut(address); //8 bit address (short address 0, 6 bit address, read/write (0/1))
    RF_SPIPut(value);	 //8 bit value
    PHY_CS = 1; 	 //disable chip select
    RFIE = tmpRFIE;  //Interrupt status is set back to previous state

}


/*********************************************************************
 * Function:        void PHYGetShortRAMAddr(BYTE address, BYTE value)
 *
 * PreCondition:    Communication port to the MRF24J40 initialized
 *
 * Input:           BYTE address is the address of the short RAM address
 *                      that you want to read from.  Should use the
 *                      READ_ThisAddress definition in the MRF24J40 
 *                      include file.
 *
 * Output:          BYTE the value read from the specified short register
 *
 * Side Effects:    None
 *
 * Overview:        This function reads a value from a short RAM address
 ********************************************************************/
BYTE PHYGetShortRAMAddr(BYTE address)
{
    BYTE toReturn;
    volatile BYTE tmpRFIE = RFIE;
    RFIE = 0;
    PHY_CS = 0;
    Nop();
    RF_SPIPut(address);
    toReturn = RF_SPIGet();
    Nop();
    PHY_CS = 1;
    RFIE = tmpRFIE;
    return toReturn;
}

/*********************************************************************
 * Function:        BYTE PHYGetLongRAMAddr(WORD address)
 *
 * PreCondition:    Communication port to the MRF24J40 initialized
 *
 * Input:           WORD address is the address of the long RAM address
 *                      that you want to read from.  
 *
 * Output:          BYTE the value read from the specified Long register
 *
 * Side Effects:    None
 *
 * Overview:        This function reads a value from a long RAM address
 ********************************************************************/
BYTE PHYGetLongRAMAddr(WORD address)
{
    BYTE toReturn;
    volatile BYTE tmpRFIE = RFIE;
    RFIE = 0;
    PHY_CS = 0;
    Nop();
    RF_SPIPut(((address>>3)&0x7F)|0x80);
    RF_SPIPut(((address<<5)&0xE0));
    toReturn = RF_SPIGet();
    Nop();
    PHY_CS = 1;
    RFIE = tmpRFIE;
    return toReturn;
}



/*********************************************************************
 * Function:        BYTE findNextAvailableNetworkEntry(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          BYTE 0xFF if there is no room in the network table
 *                  otherwise this function returns the index of the 
 *                  first blank entry in the table
 *
 * Side Effects:    None
 *
 * Overview:        This function is used to determine the next available
 *                  slot in the network table.  This function can be used
 *                  to manually create entries in the networkTable
 ********************************************************************/
BYTE findNextAvailableNetworkEntry(void)
{
    BYTE i;
    
    for(i=0;i < NETWORK_TABLE_SIZE;i++)
    {
        if(gNetworkTable[i].isValid == 0)
        {
            return i;
        }
    }
    return 0xFF;
}

BYTE findNetworkEntry(MAC_ADDR8* Address)
{
    BYTE i;
    
    for(i=0;i < NETWORK_TABLE_SIZE;i++)
    {
        if(gNetworkTable[i].isValid) {
	        if ((memcmp(&gNetworkTable[i].macAddress, Address, sizeof(MAC_ADDR8)) == 0))
	            return i;
        }
        else
        	break;
    }
    return 0xFF;
}



BYTE getIndexOfNextNetworkTypeAddress(BYTE type,BYTE startindex)
{
    BYTE i;
    
    for(i=startindex;i < NETWORK_TABLE_SIZE;i++)
    {
        if(gNetworkTable[i].isValid) {
	        if (gNetworkTable[i].macAddress.v[4] == type)
	            return i;
        }
        else
        	break;
    }
    return 0xFF;
}

BYTE getCountOfNetworkType(BYTE type)
{
    BYTE i;
    BYTE count=0;
    
    for(i=0;i < NETWORK_TABLE_SIZE;i++)
    {
        if(gNetworkTable[i].isValid) {
	        if (gNetworkTable[i].macAddress.v[4] == type)
	            count++;
        }
        else
        	break;
    }
    return count;
}


MAC_ADDR8* getNetworkAddressByIndex(BYTE index)
{
    if (index < NETWORK_TABLE_SIZE)
    	return &gNetworkTable[index].macAddress;
    else
    	return NULL;
}	
    


void SendTextNetworkTable()
{
	char strbuf[40];
	int i, j;
    for(i=0;i<NETWORK_TABLE_SIZE;i++)
    {
        if(gNetworkTable[i].isValid)
        {
#if defined(SU_APP) 
          	sprintf(strbuf,"$SLIST,%d,%03d,%03d,",i,gNetworkTable[i].lqi, gNetworkTable[i].rssi);
#elif defined(SU_TX)		
          	sprintf(strbuf,"$TLIST,%d,%03d,%03d,",i,gNetworkTable[i].lqi, gNetworkTable[i].rssi);
#else
#error 	INVALID DEVICE
#endif             
			PortPutStr(strbuf,PC_PORT,1);		
			PortPutStr(",",PC_PORT,1);				
			PortPutStr(",",PC_PORT,1);		// upper 32 bits of long address
			for (j=7;j>=4;j--) {
	            sprintf(strbuf,"%02x",gNetworkTable[i].macAddress.v[j]);
				PortPutStr(strbuf,PC_PORT,1);
			}
			PortPutStr(",",PC_PORT,1);		// lower 32 bits of long address
			for (j=3;j>=0;j--) {
	            sprintf(strbuf,"%02x",gNetworkTable[i].macAddress.v[j]);
				PortPutStr(strbuf,PC_PORT,1);
			}				
			PortPutStr("\r\n",PC_PORT,1);		
       }
       else
       		break;
    }
}


void ClearNetworkTable()
{
	int i;
    for(i=0;i<NETWORK_TABLE_SIZE;i++) {
    	gNetworkTable[i].isValid = 0;
    	gNetworkTable[i].rssi = 0;
    	gNetworkTable[i].lqi = 0;
    	memset(&gNetworkTable[i].macAddress, 0, sizeof(MAC_ADDR8));
    }	
}

	
void DumpNetworkTable(void)
{
#if defined(SU_APP) 
		DebugWireless_PutString("I am an SU_CONTROLLER\r\n");
#elif defined(SU_TX)		
		DebugWireless_PutString("I am an SU TRANSMITTER\r\n");
#elif defined(SU_RX)
		DebugWireless_PutString("I am an SU RECEIVER\r\n");
#else
#error 	INVALID DEVICE
#endif             
	
	if (g_UnitFlags.debugWireless) 
		SendTextNetworkTable();
		
}

/*********************************************************************
 * Function:        BYTE AddNodeToNetworkTable(void)
 *
 * PreCondition:    tempLongAddress, tempShortAddress, tempPANID, and 
 *                  tempNodeStatus are set to the correct values.  If 
 *                  tempNodeStatus.bits.longAddressValid is set then
 *                  tempLongAddress needs to be set.  If 
 *                  tempNodeStatus.bits.shortAddressValid is set then
 *                  tempShortAddress and tempPANID need to be set.
 *
 * Input:           None
 *
 * Output:          BYTE - the index of the network table entry 
 *                  where the device was inserted.  0xFF indicates that
 *                  the requested device couldn't be inserted into the table
 *
 * Side Effects:    Network table is updated with the devices info
 *
 * Overview:        This function is used to insert new device into the 
 *                  network table (or update already existing entries)
 ********************************************************************/
BYTE AddNodeToNetworkTable(MAC_ADDR8* address, char lqi, char rssi)
{
    BYTE handle;
   
    handle = findNetworkEntry(address);
                                  
    if(handle==0xFF) {
        handle = findNextAvailableNetworkEntry();
        if(handle !=0xFF) {
			gNetworkTable[handle].isValid = 1;
			gNetworkTable[handle].bRoute = 1;		// default is to route it unless explicitly disabled
			gNetworkTable[handle].spare = 0;
        	memcpy(gNetworkTable[handle].macAddress.v, address->v, 8);
 	    	gNetworkTable[handle].rssi = rssi;		// update rssi value for this ID      
	    	gNetworkTable[handle].lqi = lqi;		// update LQI value for this ID   
//     		DebugWireless_PutString((ROM char *)"Adding/updating network table\r\n");                            
  			DumpNetworkTable();
 	    }
    }
    else {
	    gNetworkTable[handle].rssi = rssi;		// update rssi value for this ID
	    gNetworkTable[handle].lqi = lqi;		// update LQI value for this ID   
	}
       
    return handle;
}



/*********************************************************************
 * Function:        void WiInit(void)
 *
 * PreCondition:    BoardInit (or other initialzation code is required)
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    the MiWi stack is initialized
 *
 * Overview:        This function initializes the MiWi stack and is required
 *                  before stack operation is available
 ********************************************************************/
 void WiInit(void)
{
    myShortAddress.Val = 0xFFFF;		// todo - we may set this if we want to use normal mode
    myPANID.Val = 0xFFFF;				// todo - we may set this if we want to use normal mode

 	DebugWireless_PutString((ROM char*)"Initializing - WiInit 1.0 \r\n");
   
    initMRF24J40(DEFAULT_CHANNEL);
    if (g_SysError.wireless_timeout) {
    	DebugWireless_PutString((ROM char*)"Timeout in initMRF24J40 \r\n");
    	return;
    }	
    //clear all status bits
    MiWiStatus.Val = 0;
    //enable reception
    MiWiStatus.bits.RX_ENABLED = 1;
    MiWiStatus.bits.RX_BUFFERED = 0;    
    RFIF = 0;
    RFIE = 1;

    if(RF_INT_PIN == 0)
    {
        RFIF = 1;
    }
    
#ifdef SUPPORT_SECURITY
	OutgoingFrameCounter.Val = 0;
#endif    
}


#ifdef CW_TEST
extern char g_EscapeKey;
void cw_test()
{
/*
//if using external PA and LNA
    if(UseExternal)
    {
        PHYSetLongRAMAddr(0x22F, 0x08);
        PHYSetShortRAMAddr(WRITE_TRISGPIO, 0b00001111);
        PHYSetShortRAMAddr(WRITE_GPIO, 0b00000011);
    } 
*/
            		
    while(1)						//set TX bit in RFCTL
    {
    	PHYSetShortRAMAddr(WRITE_RFCTL,0b00000010);  //force transmit of CW
	    DelayMsecs(12);
	    DelayUsecs(500);
    	PHYSetShortRAMAddr(WRITE_RFCTL,0b00000000);  //stop CW transmit
	    DelayMsecs(12);
	    DelayUsecs(500);
	    
//		Process_PCUartStream(); 
//		if (g_EscapeKey == TRUE)
//			break;
	}

	WiInit();				
/*
    if(UseExternal)
    {
        PHYSetShortRAMAddr(WRITE_RFCTL,0b00000000);
        PHYSetShortRAMAddr(WRITE_GPIO, 0b00000000);
        PHYSetShortRAMAddr(WRITE_TRISGPIO, 0b00000000);
        PHYSetLongRAMAddr(0x22F, 0x0F);
    }
*/		                      
}
#endif
