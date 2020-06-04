/********************************************************************

* FileName:		MRF24J40.c
* Dependencies:    
* Processor:	PIC24F
* Complier:     Microchip C30 v2.03 or higher		
* Company:		Microchip Technology, Inc.
*
* Copyright and Disclaimer Notice for MiWi Software:
*
* Copyright © 2007-2008 Microchip Technology Inc.  All rights reserved.
*
* Microchip licenses to you the right to use, modify, copy and distribute 
* Software only when embedded on a Microchip microcontroller or digital 
* signal controller and used with a Microchip radio frequency transceiver, 
* which are integrated into your product or third party product (pursuant 
* to the terms in the accompanying license agreement). 
*
* You should refer to the license agreement accompanying this Software for 
* additional information regarding your rights and obligations.
*
* SOFTWARE AND DOCUMENTATION ARE PROVIDED “AS IS” WITHOUT WARRANTY OF ANY 
* KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY 
* WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A 
* PARTICULAR PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE 
* LIABLE OR OBLIGATED UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, 
* CONTRIBUTION, BREACH OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY 
* DIRECT OR INDIRECT DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO 
* ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, 
* LOST PROFITS OR LOST DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, 
* TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT 
* NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
*
*********************************************************************
* File Description:
*
*
*
* Change History:
*  Rev   Date         Author	Description
*  0.1   11/09/2006   df        Initial revision
*  1.0   01/09/2007   yfy       Initial release
********************************************************************/

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

#ifdef SUPPORT_SECURITY
	extern ROM unsigned char mySecurityLevel;
	extern ROM unsigned char mySecurityKey[16];
	extern ROM unsigned char myKeySequenceNumber;
	DWORD_VAL OutgoingFrameCounter;
	
	BOOL DataEncrypt(IOPUT BYTE *Input, IOPUT BYTE *DataLen, INPUT BYTE *Header, INPUT BYTE HeaderLen);
	BOOL DataDecrypt(IOPUT BYTE *Input, IOPUT BYTE *DataLen, INPUT BYTE *Header, INPUT BYTE HeaderLen);
	
    DWORD_VAL	OutgoingFrameCount;
    DWORD_VAL IncomingFrameCount[NETWORK_TABLE_SIZE];
#endif	


/*********************************************************************
 * Function:        void SetChannel(BYTE channel)
 *
 * PreCondition:    MRF24J40 is initialized
 *
 * Input:           BYTE channel - this is the channel that you wish
 *                  to operate on.  This should be CHANNEL_11, CHANNEL_12,
 *                  ..., CHANNEL_26. 
 *
 * Output:          None
 *
 * Side Effects:    the MRF24J40 now operates on that channel
 *
 * Overview:        This function sets the current operating channel
 *                  of the MRF24J40
 ********************************************************************/
void SetChannel(BYTE channel)
{
    currentChannel = channel;
    PHYSetLongRAMAddr(RFCTRL0,channel|0x02);
    PHYSetShortRAMAddr(WRITE_RFCTL,0x04); // Hold RF state machine in Reset
    PHYSetShortRAMAddr(WRITE_RFCTL,0x00);
}

/*********************************************************************
 * Function:        void initMRF24J40(void)
 *
 * PreCondition:    BoardInit (or other initialzation code is required)
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    MRF24J40 is initialized
 *
 * Overview:        This function initializes the MRF24J40 and is required
 *                  before stack operation is available
 ********************************************************************/
// todo - verify that delays are good

void initMRF24J40(BYTE channel)
{
    BYTE i;
    WORD j;
 	unsigned long long tick = g_tick;
   
	//reset the MRF24J40 by pulling the RESET pin low
    PHY_RESETn = 0;
    for(j=0;j<(WORD)300;j++){}
    PHY_RESETn = 1;
    for(j=0;j<(WORD)300;j++){}
  
    /* do a soft reset */
    PHYSetShortRAMAddr(WRITE_SOFTRST,0x07);
    if (g_SysError.wireless_timeout)
    	return;
    	
    do
    {
        i = PHYGetShortRAMAddr(READ_SOFTRST);
        if ((g_tick - tick) > TICKSPERSECOND*2) {	// make sure this is non-blocking - 1 second is plenty
	        g_SysError.wireless_timeout = 1;
	        break;
	    }
	}
    while((i&0x07) != (BYTE)0x00);   
    if (g_SysError.wireless_timeout)
    	return;
    	
    //Delay for 192 us after RF State Machine Reset
	DelayUsecs(192);
    
    PHYSetShortRAMAddr(WRITE_RXFLUSH,0b00000101); 	// Sets to Data only Frames 
    												// and Resets the RXFIFO Address Pointer to zero. RXFIFO data is not modified. Bit is automatically cleared to ‘0’ by hardware. 
//    PHYSetShortRAMAddr(WRITE_RXFLUSH,0x01);					
    			
    // Although these may be programmed, they are only useful in normal mode whereby incoming packets
    // are filtered according to the short address and  panid.														     
    PHYSetShortRAMAddr(WRITE_SADRL,myShortAddress.v[0]);    // Program the short MAC Address, 0xffff 
    PHYSetShortRAMAddr(WRITE_SADRH,myShortAddress.v[1]);
    PHYSetShortRAMAddr(WRITE_PANIDL,myPANID.v[0]);
    PHYSetShortRAMAddr(WRITE_PANIDH,myPANID.v[1]);
    
    /* Program Long MAC Address*/
    for(i=0;i<8;i++)
    {
        PHYSetShortRAMAddr(WRITE_EADR0+i*2,g_myMACAddress.v[i]);
    }
    
    /* select the correct channel */
    PHYSetLongRAMAddr(RFCTRL0,0x02);    
    
    /* setup */
    PHYSetLongRAMAddr(RFCTRL1,0x01);   	//Initialize VCOOPT=1
    PHYSetLongRAMAddr(RFCTRL2,0x80);	//Enable PLL
    PHYSetLongRAMAddr(RFCTRL3,PA_LEVEL_0_DB);  // program TX power to highest at 0DB
    PHYSetLongRAMAddr(RFCTRL6,0x90);	//Initialize TXFIL=1, 20MRECVR=1
    PHYSetLongRAMAddr(RFCTRL7,0x80); 	//Initialize SLPCLKSEL = 0x2 (100KHz internal oscialltor)
    PHYSetLongRAMAddr(RFCTRL8,0x10);   //Initialize RFVCO =1
    PHYSetLongRAMAddr(SCLKDIV, 0x01);
    PHYSetShortRAMAddr(WRITE_BBREG2,0x80); 		//Set CCA mode to ED (Energy Detection)
    PHYSetShortRAMAddr(WRITE_CCAEDTH,0x60);    // Program CCA, RSSI threshold values 
    PHYSetShortRAMAddr(WRITE_BBREG6,0x40);  	//Calculate RSSI for each received packet. The RSSI value is stored in RXFIFO.
    PHYSetShortRAMAddr(WRITE_FFOEN, 0x98);
    PHYSetShortRAMAddr(WRITE_TXPEMISP, 0x95);
    PHYSetShortRAMAddr(WRITE_RXMCR,0x01);		// SET TO Promiscuous mode TO RECEIVE ALL PACKETS 

	tick = g_tick;
    do
    {
        i = PHYGetLongRAMAddr(RFSTATE);
        if ((g_tick - tick) > TICKSPERSECOND) {	// make sure this is non-blocking - 1 second is plenty
	        g_SysError.wireless_timeout = 1;
	        break;
	    }
    }
    while((i&0xE0) != 0xA0);		//Wait until the RFSTATE machine indicates RX state
    if (g_SysError.wireless_timeout)
	  	return;

   
    PHYSetShortRAMAddr(WRITE_INTCON,0b11100110);	// disable sleep alert interrupt
    												// disable wake-up alert interrupt
     												// disable half-symbol timer interrupt
      												// enable security key request interrupt
     												// enable RX Fifo reception interrupt
     												// disable TX GTS2 FIFO transmission interrupt
     												// disable TX GTS1 FIFO transmission interrupt
     												// Enables the TX Normal FIFO transmission interrupt
  
    SetChannel(channel);

    #ifdef TURBO_MODE
    
        PHYSetShortRAMAddr(WRITE_BBREG0, 0x01);
        PHYSetShortRAMAddr(WRITE_BBREG3, 0x38);
        PHYSetShortRAMAddr(WRITE_BBREG4, 0x5C);
        
        PHYSetShortRAMAddr(WRITE_RFCTL,0x04);	// Hold RF state machine in Reset
        PHYSetShortRAMAddr(WRITE_RFCTL,0x00);	// release from reset

    #endif
}


/*********************************************************************
* Function:        void MRF24J40Sleep(void)
*
* PreCondition:    BoardInit (or other initialzation code is required)
*
* Input:           None
*
* Output:          None
*
* Side Effects:    
*
* Overview:        
********************************************************************/

void MRF24J40Sleep(void)
{
    MiWiStatus.bits.PHY_SLEEPING = 1;
    
    //;clear the WAKE pin in order to allow the device to go to sleep
    PHY_WAKE = 0;
    PHY_WAKE_TRIS = OUTPUT;
    
    PHYSetShortRAMAddr(WRITE_SOFTRST, 0x04);    	// make a power management reset to ensure device goes to sleep
    PHYSetShortRAMAddr(WRITE_RFCTL,0x00); 			// Doing RF state machine reset first 
    PHYSetShortRAMAddr(WRITE_WAKECON,0x80);			// WAKECON = 0x80 Enable immediate wakeup mode
    PHYSetShortRAMAddr(WRITE_RXFLUSH,0b01100000); 	// RXFLUSH = 0x60 Enabling WAKE pin and setting polarity high
    PHYSetShortRAMAddr(WRITE_SLPACK,0x80);			// SLPACK = 0x80 Put MRF24J40 to sleep mode

}


/*********************************************************************
 * Function:        void MRF24J40Wake(void)
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
void MRF24J40Wake(void)
{
    BYTE results;
 	unsigned long long tick = g_tick;
     
    //;wake up the device
    PHY_WAKE = 1;

	// Delay 2 ms to allow 20 MHz main oscillator time to stabilize before transmitting or receiving.
    do
    {
        results = PHYGetShortRAMAddr(READ_INTSTAT);
        if ((g_tick - tick) > TICKSPERSECOND/8) {		// make sure this is non-blocking - 1/8 second is plenty
	        g_SysError.wireless_timeout = 1;
	        break;
	    }
	}
    while(!(results & 0x40));   //WAIT UNTIL THE the interrupt status register WAKEIF bit is set

  
    do
    {
        results = PHYGetLongRAMAddr(RFSTATE);
        if ((g_tick - tick) > TICKSPERSECOND/8) {		// make sure this is non-blocking - 1/8 second is plenty
	        g_SysError.wireless_timeout = 1;
	        break;
	    }
	}
    while((results & 0xE0) != 0xA0);   //Wait until the RFSTATE machine indicates RX state
    
         
    MiWiStatus.bits.PHY_SLEEPING = 0;
 }
 

#ifdef SUPPORT_SECURITY

/*********************************************************************
 * Function:        CIPHER_STATUS PHYCipher(INPUT CIPHER_MODE CipherMode, INPUT SECURITY_INPUT SecurityInput, OUTPUT BYTE *OutData, OUTPUT BYTE *OutDataLen)
 *
 * PreCondition:    Called by DataEncrypt or DataDecrypt
 *
 * Input:           CIPHER_MODE CipherMode       - Either MODE_ENCRYPTION or MODE_DECRYPTION
 *                  SECURITY_INPUT SecurityInput - Cipher operation input. Filled by DataEncryption or DataDecryption
 *
 * Output:          BYTE *OutData                - Encrypted or decrypted data, including MIC
 *                  BYTE *OutDataLen             - Data length after cipher operation, including MIC bytes
 *                  CIPHER_STATUS                - Cipher operation result
 *
 * Side Effects:    Input data get encrypted or decrypted and put into output buffer
 *
 * Overview:        This is the function that invoke the hardware cipher to do encryption and decryption
 ********************************************************************/
CIPHER_STATUS PHYCipher(INPUT CIPHER_MODE CipherMode, INPUT SECURITY_INPUT SecurityInput, OUTPUT BYTE *OutData, OUTPUT BYTE *OutDataLen)
{
	BYTE CipherRetry = CIPHER_RETRY;
	BYTE i;
	WORD loc;

	// make sure that we are not in the process of sending out a packet
	while( MiWiStatus.bits.TX_BUSY )
	{
	   if(RF_INT_PIN == 0)
	   {
	       RFIF = 1;
	   }
		Nop();
	}
	
CipherOperationStart:

	// step 1, set the normal FIFO
	// step 1a. fill the length of the header
	if( mySecurityLevel > 0x04 )
	{
		PHYSetLongRAMAddr(0x000, SecurityInput.HeaderLen+SecurityInput.TextLen+13);
	} else {
		PHYSetLongRAMAddr(0x000, SecurityInput.HeaderLen+13);
	}

	// step 1b, fill the length of the packet
	if( CipherMode == MODE_ENCRYPTION )
	{
		PHYSetLongRAMAddr(0x001, SecurityInput.TextLen+SecurityInput.HeaderLen+13); 
	} else {
		PHYSetLongRAMAddr(0x001, SecurityInput.TextLen+SecurityInput.HeaderLen+15);// two additional bytes FCS
	}	

	//step 1c, fill the MiWi header
	loc = 0x002;
	for(i = 0; i < SecurityInput.HeaderLen; i++)
	{
		PHYSetLongRAMAddr(loc++, SecurityInput.Header[i]);
	}
	// step 1d, fill the frame counter
	for(i = 0; i < 4; i++)
	{
		PHYSetLongRAMAddr(loc++, SecurityInput.FrameCounter.v[i]);
	}
	// step 1e, fill the source extended address
	for(i = 0; i < 8; i++)
	{
		PHYSetLongRAMAddr(loc++, SecurityInput.SourceAddress[i]);
	}
	// step 1f, fill the security key sequence number. It will be useful
	// if application layer decided to be able to request and transport key
	PHYSetLongRAMAddr(loc++, myKeySequenceNumber);
	
	// step 1g, fill the data to be encrypted or decrypted
	for(i = 0; i < SecurityInput.TextLen; i++) 
	{
		PHYSetLongRAMAddr(loc++, SecurityInput.InputText[i]);
	}

	// step 2, set nounce
	loc = 0x24C;
	for(i = 0; i < 8; i++) {
		PHYSetLongRAMAddr(loc--, SecurityInput.SourceAddress[i]);
	}
	for(i = 0; i < 4; i++) 
	{
		PHYSetLongRAMAddr(loc--, SecurityInput.FrameCounter.v[i]);
	}
	PHYSetLongRAMAddr(loc--, mySecurityLevel);
	

	// step 3, set TXNFIFO security key
	loc = 0x280;
	for(i = 0; i < 16; i++) 
	{
		PHYSetLongRAMAddr(loc++, mySecurityKey[i]);
	}

	// step 4, set the mode either to encrypt or decrypt
	if( CipherMode == MODE_ENCRYPTION )
	{
		PHYSetShortRAMAddr(WRITE_SECCR2, 0x40);
	} else {
		PHYSetShortRAMAddr(WRITE_SECCR2, 0x80);
	}

	// step 5, fill the security mode
	PHYSetShortRAMAddr(WRITE_SECCR0, mySecurityLevel);

	// trigger the hardware cipher to operate
	MiWiStatus.bits.TX_SECURITY = 1;
	PHYSetShortRAMAddr(WRITE_TXNMTRIG, 0x03);

	// wait until hardware cipher report job finished
	// in interrupt handler
	i = 0;
	while( MiWiStatus.bits.TX_SECURITY ) 
	{
	   if(RF_INT_PIN == 0)
	   {
	       RFIF = 1;
	   }
	   
		i++;
		#if 1
		// in rare condition, the hardware cipher will stall. Handle such condition
		// here
		if(i > 0xef)
		{
			DebugWireless_PutString((ROM char*)"X ");
			MiWiStatus.bits.TX_SECURITY = 0;
			PHY_RESETn = 0;
			initMRF24J40();
			return CIPHER_ERROR;
		}
		#endif
	}
	//PHYSetShortRAMAddr(WRITE_RXFLUSH, 0x01);


	// if MIC is generated, check MIC here
	if( (CipherMode == MODE_DECRYPTION) && (mySecurityLevel != 0x01))
	{
		BYTE MIC_check = PHYGetShortRAMAddr(READ_RXSR);
		if( MIC_check & 0x40 )
		{
			PHYSetShortRAMAddr(WRITE_RXSR, 0x40);
			// there is a small chance that the hardware cipher will not 
			// decrypt for the first time, retry to solve this problem.
			// details documented in errata
			if( CipherRetry )
			{
				//ConsolePutROMString((ROM char*)"Cipher Retry\r\n");
				CipherRetry--;
				for(loc = 0; loc < 0x255; loc++);
				goto CipherOperationStart;
			}
			PHY_RESETn = 0;
			initMRF24J40();
			return CIPHER_MIC_ERROR;
		}
	} 

	// working with interrupt handler to solve the problem in case
	// there is an error while doing hardware cipher operation. The error
	// can be detected while reading TXSTAT register. 
	{	
		// get output data length
		*OutDataLen = PHYGetLongRAMAddr(0x001)- SecurityInput.HeaderLen - 13; 
		// get the index of data encrypted or decrypted
		loc = 0x002 + SecurityInput.HeaderLen + 13;
		
		// if this is a decryption operation, get rid of the useless MIC and two bytes FCS
		if( CipherMode == MODE_DECRYPTION )
		{
			switch( mySecurityLevel )
			{
				case 0x02:
				case 0x05:
					*OutDataLen -= 18; 
					break;
				case 0x03:
				case 0x06:
					*OutDataLen -= 10; 
					break;
				case 0x04:
				case 0x07:
					*OutDataLen -= 6; 
					break;
				case 0x01:
					*OutDataLen -= 2;
					break;
			}
		}
		
		// copy the output data
		for(i = 0; i < *OutDataLen; i++) 
		{
			OutData[i] = PHYGetLongRAMAddr(loc++);
		}
		return CIPHER_SUCCESS;	
	}

	return CIPHER_ERROR;
}

/*********************************************************************
 * Function:        BOOL DataEncrypt(IOPUT BYTE *Input, IOPUT BYTE *DataLen, INPUT BYTE *Header, INPUT BYTE HeaderLen)
 *
 * PreCondition:    Input and Header has been filled 
 *
 * Input:           BYTE *Header   - Point to MiWi header
 *                  BYTE HeaderLen - MiWi header length
 *
 * Output:          BOOL           - If data encryption successful
 *
 * Input/Output:    BYTE *Input    - Pointer to the data to be encrypted. The encrypted data will be write back to the pointer
 *                  BYTE *DataLen  - Input as the length of the data to be encrypted. The encrypted data length (including MICs) will be written back
 *
 * Side Effects:    Input data get encrypted and written back to the input pointer
 *
 * Overview:        This is the function that call the hardware cipher to encrypt input data
 ********************************************************************/
BOOL DataEncrypt(IOPUT BYTE *Input, IOPUT BYTE *DataLen, INPUT BYTE *Header, INPUT BYTE HeaderLen)
{
	SECURITY_INPUT SecurityInput;
	BYTE EncryptedLen;
	BYTE i;
	BYTE Counter;

	// operate the frame counter 
	SecurityInput.FrameCounter = OutgoingFrameCounter;
	OutgoingFrameCounter.Val++;

	// fill the source extended address
	SecurityInput.SourceAddress[0] = g_myMACAddress[0];
	SecurityInput.SourceAddress[1] = g_myMACAddress[1];
	SecurityInput.SourceAddress[2] = g_myMACAddress[2];
	SecurityInput.SourceAddress[3] = g_myMACAddress[3];
	SecurityInput.SourceAddress[4] = g_myMACAddress[4];
	SecurityInput.SourceAddress[5] = g_myMACAddress[5];
	SecurityInput.SourceAddress[6] = g_myMACAddress[6];
	SecurityInput.SourceAddress[7] = g_myMACAddress[7];
	
	// fill the pointer to header and header length
	SecurityInput.Header = Header;
	SecurityInput.HeaderLen = HeaderLen;
 
 	// in rare cases, the hardware encryption engine may not suceed for the 
 	// first time. Retry a few times will solve the problem
	Counter = CIPHER_RETRY;
	while(Counter)
	{
		// fill the input data and data length
		SecurityInput.InputText = Input;
		SecurityInput.TextLen = *DataLen;
		// call hardware cipher and store the output to temporary buffer
		PHYCipher(MODE_ENCRYPTION, SecurityInput, tmpBuf, &EncryptedLen);
		
		// try to decrypt the buffer to make sure that encryption is correct
		SecurityInput.InputText = tmpBuf;
		SecurityInput.TextLen = EncryptedLen;
		if( PHYCipher(MODE_DECRYPTION, SecurityInput, Input, &i) == CIPHER_SUCCESS )
		{
			break;
		}
		Counter--;
	} 

	// fill the frame counter
	for(i = 0; i < 4; i++)
	{
		Input[i] = SecurityInput.FrameCounter.v[i];
	}
	// fill the source extended address
	Counter = i;
	for(i = 0; i < 8; i++)
	{
		Input[Counter++] = SecurityInput.SourceAddress[i];
	}
	// fill the security key sequence number
	Input[Counter++] = myKeySequenceNumber;	
		
	// fill the encrypted data
	for(i = 0; i < EncryptedLen; i++)
	{
		Input[Counter++] = tmpBuf[i];
	}

	*DataLen = Counter;

	return TRUE;
}

/*********************************************************************
 * Function:        BOOL DataDecrypt(IOPUT BYTE *Input, IOPUT BYTE *DataLen, INPUT BYTE *Header, INPUT BYTE HeaderLen)
 *
 * PreCondition:    Input and Header has been filled 
 *
 * Input:           BYTE *Header   - Point to MiWi header
 *                  BYTE HeaderLen - MiWi header length
 *
 * Output:          BOOL           - If data encryption successful
 *
 * Input/Output:    BYTE *Input    - Pointer to the data to be decrypted. The decrypted data will be write back to the pointer
 *                  BYTE *DataLen  - Input as the length of the data to be decrypted. The encrypted data length (excluding MICs) will be written back
 *
 * Side Effects:    Input data get decrypted and written back to the input pointer
 *
 * Overview:        This is the function that call the hardware cipher to decrypt input data
 ********************************************************************/
BOOL DataDecrypt(IOPUT BYTE *Input, IOPUT BYTE *DataLen, INPUT BYTE *Header, INPUT BYTE HeaderLen, char tempLongAddress[8])
{
	SECURITY_INPUT SecurityInput;
	BYTE i;
	BYTE Counter;
	DWORD_VAL	FrameCounter;
	BYTE KeySeq;

	// fill the frame counter
	Counter = 0;
	for(i = 0; i < 4; i++)
	{
		FrameCounter.v[i] = Input[Counter++];
	}
	// fill the source extended address
	for(i = 0; i < 8; i++)
	{
		tempLongAddress[i] = Input[Counter++];
	}
	// get security key sequence number
	KeySeq = Input[Counter++];
	
	// check if the security key sequence number match
	if( KeySeq != myKeySequenceNumber )
	{
		// if keys can be requested/transferred in application layer
		// request the security key here
		DebugWireless_PutString((ROM char *)"Key Sequence Number fail.");
		return FALSE;
	}
	
	// fill the security input
	SecurityInput.FrameCounter = FrameCounter;
	SecurityInput.InputText = &(Input[Counter]);
	SecurityInput.TextLen = *DataLen - Counter;
	SecurityInput.Header = Header;
	SecurityInput.HeaderLen = HeaderLen;
	SecurityInput.SourceAddress[0] = tempLongAddress[0];
	SecurityInput.SourceAddress[1] = tempLongAddress[1];
	SecurityInput.SourceAddress[2] = tempLongAddress[2];
	SecurityInput.SourceAddress[3] = tempLongAddress[3];
	SecurityInput.SourceAddress[4] = tempLongAddress[4];
	SecurityInput.SourceAddress[5] = tempLongAddress[5];
	SecurityInput.SourceAddress[6] = tempLongAddress[6];
	SecurityInput.SourceAddress[7] = tempLongAddress[7];

	// call hardware cipher
	if( PHYCipher(MODE_DECRYPTION, SecurityInput, Input, DataLen) != CIPHER_SUCCESS )
	{
		return FALSE;
	}
	
	// check the frame counter, make sure that the frame counter increase always
	// we only check family members, because only family members know if a node 
	// join or leave the network to reset the frame counter
	i = SearchForLongAddress();
	if( (i != 0xFF) && networkStatus[i].bits.isFamily )
	{
		if( FrameCounter.Val < networkTable[i].IncomingFrameCounter.Val )
		{
			/*
			DebugWireless_PutString((ROM char *)"Frame counter test fail\r\n");
			PrintChar(FrameCounter.v[3]);
			PrintChar(FrameCounter.v[2]);
			PrintChar(FrameCounter.v[1]);
			PrintChar(FrameCounter.v[0]);
			DebugWireless_PutString((ROM char *)" vs ");
			PrintChar(networkTable[i].IncomingFrameCounter.v[3]);
			PrintChar(networkTable[i].IncomingFrameCounter.v[2]);
			PrintChar(networkTable[i].IncomingFrameCounter.v[1]);
			PrintChar(networkTable[i].IncomingFrameCounter.v[0]);
			*/
			return FALSE;
		}
		networkTable[i].IncomingFrameCounter.Val = FrameCounter.Val + 1;
	}
	
	
	return TRUE;
}

void SetTurboMode(int bEnable)
{
	if (bEnable) {
			//Program the Turbo mode configuration registers
        PHYSetShortRAMAddr(WRITE_BBREG0, 0x01);
        PHYSetShortRAMAddr(WRITE_BBREG3, 0x38);
        PHYSetShortRAMAddr(WRITE_BBREG4, 0x5C);
                            
	}
	else {
		//Program to normal mode 
        PHYSetShortRAMAddr(WRITE_BBREG0, 0x00);
        PHYSetShortRAMAddr(WRITE_BBREG3, 0xD8);
        PHYSetShortRAMAddr(WRITE_BBREG4, 0x9C);
	}
	
	//Reset the Baseband Circuitry
    PHYSetShortRAMAddr(WRITE_SOFTRST,0x02);
}

#endif

