/*********************************************************************
 *                                                                    
 * Software License Agreement                                         
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
 *********************************************************************/

#ifndef _MIWI_DEFS_H_
#define _MIWI_DEFS_H_
#define ENABLE_TABLE_DUMP

#include "GenericTypeDefs.h"

#define ENABLE_CONSOLE


#ifdef SU_APP

// SPI CONFIGURATION

#define RF_SPICLOCK       _TRISG6   	// The TRIS bit for the SCK pin
#define RF_SPIIN          _TRISG7  		// The TRIS bit for the SDI pin
#define RF_SPIOUT         _TRISG8		// The TRIS bit for the SDO pin
	
// RF is on SPI2
#define RF_SPIRBF	 	SPI2STATbits.SPIRBF
#define RF_SPISTATbits 	SPI2STATbits
#define RF_SSPIF_BIT 	IFS2bits.SPI2IF
#define RF_SSPBUF_REG 	SPI2BUF
#define RF_SPISTAT 		SPI2STAT
#define RF_SPICON1 		SPI2CON1

// Transceiver Configuration 

#define RF_INTERRUPT_SOURCE 4
#define RF_INT_PIN _RD11
#define RF_INT_PIN_TRIS _TRISD11
#define RF_INT_PORT PORTD
#define RFIF _INT4IF
#define RFIE _INT4IE
#define RF_INT_EDGE INTCON2bits.INT4EP
#define RF_INT_PRIORITY _INT4IP


#define PHY_CS 			_LATB2
#define PHY_CS_TRIS 	_TRISB2

#define PHY_RESETn 		_LATB0
#define PHY_RESETn_TRIS _TRISB0

#define PHY_WAKE 		_LATB1
#define PHY_WAKE_TRIS 	_TRISB1

void RF_SPIPut(BYTE v);
BYTE RF_SPIGet();
#define RF_SPIInit()   RF_SSPIF_BIT = 1

#endif

#ifdef SU_TX				////////////////////////////////////////////////////////////////////////

// SPI CONFIGURATION

#define RF_SPICLOCK       _TRISB5   	// The TRIS bit for the SCK pin
#define RF_SPIOUT         _TRISB4		// The TRIS bit for the SDO pin
#define RF_SPIIN          _TRISF5  		// The TRIS bit for the SDI pin
	
// RF is on SPI2
#define RF_SPIRBF	 	SPI2STATbits.SPIRBF
#define RF_SPISTATbits 	SPI2STATbits
#define RF_SSPIF_BIT 	IFS2bits.SPI2IF
#define RF_SSPBUF_REG 	SPI2BUF
#define RF_SPISTAT 		SPI2STAT
#define RF_SPICON1 		SPI2CON1

// Transceiver Configuration 

#define RF_INT_PIN _RF6
#define RF_INT_PIN_TRIS _TRISF6
#define RF_INT_PORT PORTF

// INTERRUPT IS ON INT0
#define RF_INTERRUPT_SOURCE 0
#define RFIF _INT0IF
#define RFIE _INT0IE
#define RF_INT_EDGE INTCON2bits.INT0EP
#define RF_INT_PRIORITY _INT0IP

#define PHY_CS 			_LATB2
#define PHY_CS_TRIS 	_TRISB2

#define PHY_RESETn 		_LATB0
#define PHY_RESETn_TRIS _TRISB0

#define PHY_WAKE 		_LATB1
#define PHY_WAKE_TRIS 	_TRISB1

void RF_SPIPut(BYTE v);
BYTE RF_SPIGet();
#define RF_SPIInit()   RF_SSPIF_BIT = 1

#endif


//#define TMRL		TMR2
//#define TMRH		TMR3

#define SECURITY_CAPABLE 0
#define ALLOCATE_ADDRESS 1


// These are large scale adjustment power levels
#define PA_LEVEL_0_DB   0b00000000  // -0.00 dBm  = max output power 
#define PA_LEVEL_N10_DB 0b01000000  // -0.10 dBm  = max output power 
#define PA_LEVEL_N20_DB 0b10000000  // -0.20 dBm  = max output power 
#define PA_LEVEL_N30_DB 0b11000000  // -0.30 dBm  = max output power 

// Add the values below to the large scale adjustment above adjust the power level by small increments
#define PA_LEVEL_N0P0_DB 0b00000000  // -0.5 dBm  = max output power 
#define PA_LEVEL_N0P5_DB 0b00001000  // -0.5 dBm  = max output power 
#define PA_LEVEL_N1P2_DB 0b00010000  // -1.2 dBm  = max output power 
#define PA_LEVEL_N1P9_DB 0b00011000  // -1.9 dBm  = max output power 
#define PA_LEVEL_N2P8_DB 0b00100000  // -2.8 dBm  = max output power 
#define PA_LEVEL_N3P7_DB 0b00101000  // -3.7 dBm  = max output power 
#define PA_LEVEL_N4P9_DB 0b00110000  // -4.9 dBm  = max output power 
#define PA_LEVEL_N6P3_DB 0b00111000  // -6.3 dBm  = max output power 

#define FREQUENCY_BAND 2400
#define ALLOWED_CHANNELS CHANNEL_25
#define AVAILABLE_CHANNELS_SIZE 1
#define RSSI_SAMPLES_PER_CHANNEL 5  //0-255

// Message Buffers
#define TX_BUFFER_SIZE 127
#define RX_BUFFER_SIZE 127



// Indirect Buffer Management
#define INDIRECT_BUFFER_SIZE 127

// Additional NWK/MAC Constants
#define NETWORK_TABLE_SIZE 16


//#define SUPPORT_SECURITY
#define SECURITY_KEY_00 0x00
#define SECURITY_KEY_01 0x01
#define SECURITY_KEY_02 0x02
#define SECURITY_KEY_03 0x03
#define SECURITY_KEY_04 0x04
#define SECURITY_KEY_05 0x05
#define SECURITY_KEY_06 0x06
#define SECURITY_KEY_07 0x07
#define SECURITY_KEY_08 0x08
#define SECURITY_KEY_09 0x09
#define SECURITY_KEY_10 0x0a
#define SECURITY_KEY_11 0x0b
#define SECURITY_KEY_12 0x0c
#define SECURITY_KEY_13 0x0d
#define SECURITY_KEY_14 0x0e
#define SECURITY_KEY_15 0x0f
#define KEY_SEQUENCE_NUMBER 0x00
#define SECURITY_LEVEL 0x04


#if (RX_BUFFER_SIZE > 127)
    #error RX BUFFER SIZE too large. Must be <= 127.
#endif

#if (TX_BUFFER_SIZE > 127)
    #error TX BUFFER SIZE too large. Must be <= 127.
#endif

#if (RX_BUFFER_SIZE < 25)
    #error RX BUFFER SIZE too small. Must be >= 25.
#endif

#if (TX_BUFFER_SIZE < 25)
    #error TX BUFFER SIZE too small. Must be >= 25.
#endif

#if (NETWORK_TABLE_SIZE == 0)
    #error NETWORK TABLE SIZE too small.
#endif

#if (NETWORK_TABLE_SIZE > 0xFE)
    #error NETWORK TABLE SIZE too large.  Must be < 0xFF.
#endif

#if (INDIRECT_BUFFER_SIZE > 0xFE)
    #error INDIRECT BUFFER SIZE too large.  Must be < 0xFF.
#endif

#endif
