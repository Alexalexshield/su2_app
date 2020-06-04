#ifndef __SU_STRUCT_H
#define __SU_STRUCT_H
/**********************************************************************
*
**********************************************************************/
#include "time.h"

#ifdef _WIN32
// make sure structures are packed the same way in Windows as they are in the firmware
#pragma pack(push, 2 )
#define int8	__int8
#define int16   __int16
#define int32   __int32
#define int64   __int64
#else
#define int8    char
#define int16   short
#define int32   long
#define int64   long long
#endif


#define TOGGLE_ENABLE_VERTERBI '#'
/////////////////////////  SU2_APP TEXT FUNCTIONS //////////////////////////////////////////////////
#ifdef SU_APP
#define TOGGLE_LOG_STATUS 'a'
#define TOGGLE_SENDBATT 'b'
#define TOGGLE_DEBUG_MISC 'c'
#define TOGGLE_DEBUG_SD 'd'
#define GET_FLASH_CONFIG 'f'
#define GET_RAM_CONFIG 'g'
#define GET_HELP 'h'
#define TOGGLE_SEND_SU_MSGS 'i'
#define TOGGLE_SEND_MU_MSGS 'j'
#define TOGGLE_SEND_LOG 'k'
#define TOGGLE_DEBUG_SLCD 's'

#define MEM_PROPERTIES 'm'
#define TOGGLE_DEBUG_BUTTONS 'n'
#define TOGGLE_AUTOLCDSHUTDOWN 'p'
#define SWRESET 'r'
#define TOGGLE_SENDTIME 't'
#define GETVERSION 'v'

#define SD_WRITETEST 'W'
#define SD_READTEST 'R'
#define TOGGLE_DEBUGWIRELESSS 'w'
#endif

/////////////////////////  SU2_TX TEXT FUNCTIONS //////////////////////////////////////////////////
#ifdef SU_TX
#define SEND_TX_ACK 'a'
#define SEND_TX_NACK 'n'
#define TOGGLE_SENDBATT 'b'
#define TOGGLE_DEBUG_MISC 'c'
#define GET_FLASH_CONFIG 'f'
#define GET_RAM_CONFIG 'g'
#define GET_HELP 'h'
#define TOGGLE_LEDS 'e'
#define SWRESET 'r'
#define TOGGLE_SENDTIME 't'
#define GETVERSION 'v'
#define SEND_DETECT_MSG 'd'
#define SEND_LOCATE_MSG 'l'
#define SEND_QUICKSEARCH_MSG 'q'
#define SEND_MASK_MSG 'm'
#define SET_PROTOCOL1 '1'
#define SET_PROTOCOL2 '2'
#define TOGGLE_DEBUGWIRELESSS 'w'
#define SELECT_X_ANTENNA 'x'
#define SELECT_Y_ANTENNA 'y'
#define SELECT_Z_ANTENNA 'z'
#endif

/////////////////////////  SU2_RX TEXT FUNCTIONS //////////////////////////////////////////////////
#ifdef SU_RX
#define TOGGLE_SENDBATT 	'b'
#define TOGGLE_DEBUG_MISC 	'c'
#define TOGGLE_MODE_DECODE	'd'
#define TOGGLE_MODE_LOCATE  'e'
#define GET_FLASH_CONFIG 	'f'
#define GET_RAM_CONFIG 		'g'
#define GET_HELP 			'h'
#define SEND_RESPONSE_TOSU	'i'
#define TOGGLE_LEDS 		'l'
#define SWRESET 			'r'
#define TOGGLE_SENDTIME 	't'
#define TOGGLE_SEND_RSSI	'x'
#define GETVERSION 			'v'
#define SET_PROTOCOL1 '1'
#define SET_PROTOCOL2 '2'
#endif

/////////////////////////  MU TEXT FUNCTIONS //////////////////////////////////////////////////
#ifdef MU_APP
#define TOGGLE_ACCEL 'a'
#define ACTIVATE_BUZZER 'b'
#define TOGGLE_DEBUG_MISC 'c'
#define GET_FLASH_CONFIG 'f'
#define GET_RAM_CONFIG 'g'
#define GET_HELP 'h'
#define TOGGLE_LED 'l'
#define SWRESET 'r'
#define TOGGLE_SENDTIME 't'
#define GETVERSION 'v'
#define SEND_TEST_TRANSMIT 'x'
#define SEND_DEFAULT_RESPONSE 'd'
#define TOGGLE_DISABLE_ACCELEROMETER 'm'
#endif

#define ECHOVERSION '\r'
#define ESCAPE 27


////////////////////////////////// PC - CSV COMMANDS //////////////////////////////////////////////

#define SU_APP_RESET_CSV_HEADER "$SURST"
#define SU_TX_RESET_CSV_HEADER "$TXRST"
#define SU_RX_RESET_CSV_HEADER "$RXRST"


////////////////////////////////// CSV MESSAGES TO AND FROM THE SU_VLF_TX MODULE /////////////////////////////////////////

// MESSAGE SENT TO TELL THE SU_VLF_TX module to send a broadcast DETECT MESSAGE to all MUs
#define SU_TX_DETECT_CSV_HEADER "$TXDET"
// MESSAGE SENT TO TELL THE SU_VLF_TX MODULE TO SEND A MASK MESSAGE TO A SPECIFIC MU
#define SU_TX_MASK_CSV_HEADER "$TXMSK"
//FULL_MASK
#define SU_TX_FULLMASK_CSV_HEADER "$TXMSF"
//FULL_MASK END

// MESSAGE SENT TO TELL THE SU_VLF_TX MODULE TO SEND A LOCATE MESSAGE TO A SPECIFIC MU
#define SU_TX_LOCATE_CSV_HEADER "$TXLOC"

// MESSAGE SENT TO TELL THE SU_VLF_TX MODULE TO SEND A QuickSearch MESSAGE TO All MUs
#define SU_TX_QUICK_SEARCH_CSV_HEADER "$TXQKS"

// MESSAGE SENT FROM SU_VLF_TX MODULE TO THE SU to indicate that the last command was successfully sent
#define SU_TX_ACK_CSV_HEADER "$TXACK"

// MESSAGE SENT FROM SU_VLF_TX MODULE TO THE SU to indicate that the last command was ignored
#define SU_TX_NACK_CSV_HEADER "$TXNAK"

#define SU_TX_PING_CSV_HEADER "$TXPNG"

////////////////////////////////// CSV MESSAGES TO AND FROM THE SU_VLF_RX MODULE /////////////////////////////////////////


// MESSAGE SENT TO TELL THE SU_VLF_RX MODULE TO PUT ITSELF INTO STANDBY, LOCATE OR DECODE MODE
#define SU_RX_MODE_CSV_HEADER "$RXMOD"
#define SU_RX_PING_CSV_HEADER "$RXPNG"
#define SU_RX_ZC_TRIGGER_LEVEL_CSV_HEADER "$RXZCT"
#define SU_RX_MODE_STANDBY	 "Standby"
#define SU_RX_MODE_DECODE	 "Decode"
#define SU_RX_MODE_LOCATE	 "Locate"
#define SU_RX_MODE_CALIBRATE "Calibrate"
#define MODE_STANDBY 0
#define MODE_DECODE 1
#define MODE_LOCATE 2
#define MODE_CALIBRATE 3


// MESSAGE SENT FROM SU_VLF_RX MODULE TO THE SU to indicate that the last command was successfully sent
#define SU_RX_ACK_CSV_HEADER "$RXACK"

// MESSAGE SENT FROM SU_VLF_RX MODULE TO THE SU to indicate that the last command was ignored
#define SU_RX_NACK_CSV_HEADER "$RXNAK"

// MESSAGE SENT FROM THE SU_VLF_RX module to the SU unit with the response from an MU with fields depending on protocol
#define SU_RX_MU_RESPONSE "$RXRSP"

// MESSAGE SENT FROM SU_VLF_RX MODULE TO THE SU to indicate that an error occurred
#define SU_RX_ERROR_CSV_HEADER "$RXERR"

// MESSAGE SENT FROM SU_VLF_RX MODULE TO THE SU every 100ms with Clutter RSSI
#define SU_RX_CLUTTER_RSSI_CSV_HEADER "$RXCLT"

// DEBUG SENT FROM SU_VLF_RX MODULE TO THE SU with Packet RSSI
#define SU_RX_PACKET_RSSI_CSV_HEADER "$RXPRS"

// MESSAGE SENT FROM SU_VLF_RX MODULE TO THE SU every 100ms with ZERO CROSSINGS
#define SU_RX_ZC_CSV_HEADER "$RXZCR"



//////////////////////////////////////////////////////////////////////////
// PICmicro Information
//You may have a 64-bit global identifier (EUI-64) provided by an authorized manufacturer of these values (in the form of 
//electronically-readable chips). The most-significant 24 /_to__ 36 bits_/ of this value are the /company_id/ value assigned to the manufacturer by the IEEE Registration Authority. The least-significant /_28-_/40-bit extension identifier is assigned by the manufacturer.
//
//For example, assume that a manufacturer's IEEE-assigned /_OUI-24_ company_id/ value is ACDE48_16 and the manufacturer-selected extension identifier for a given component is 234567ABCD_16 . The EUI-64 value generated from these two numbers is ACDE48234567ABCD16, whose byte and bit representations are illustrated below:
//
//|        company_id       |            extension identifier           | field 
//|addr+0 | addr+1 | addr+2 | addr+3 | addr+4 | addr+5 | addr+6 | addr+7| order
//|  AC   |   DE   |   48   |   23   |   45   |   67   |   AB   |   CD  | hex
//10101100 11011110 01001000 00100011 01000101 01100111 10101011 11001101 bits
//|  |                                                               |  |
//|  most significant byte                      least significant byte  |
//most-significant bit                              least-significant bit

// MINERADIO'S OUI IS TODO - USING MICROCHIP'S 0004A3 FOR NOW 

#define MINERADIO_OUI_7 0x00
#define MINERADIO_OUI_6 0x04
#define MINERADIO_OUI_5 0xA3
#define MINERADIO_SU_CONTROLLER 0x01
#define MINERADIO_SU_TX 0x02
#define MINERADIO_SU_RX 0x03

#define DEF_EUI_3 0x00
#define DEF_EUI_2 0x00
#define DEF_EUI_1 0x00
#define DEF_EUI_0 0x01

#define BROADCAST_EUI 0xffffffffl
#define BROADCAST_EUI_3 0xff
#define BROADCAST_EUI_2 0xff
#define BROADCAST_EUI_1 0xff
#define BROADCAST_EUI_0 0xff

///////////////////////////////////////////////////////////////////////////////////


#define VLF_PROTOCOL_1 0
#define VLF_PROTOCOL_2 1
#define VLF_PROTOCOL_MAX 2

#define LOCATE_DURATION_MIN 1
#define LOCATE_DURATION_DEFAULT 5
#define LOCATE_DURATION_MAX 120

#define TX_ANTENNA_NONE '0'
#define TX_ANTENNA_X 'X'
#define TX_ANTENNA_Y 'Y'
#define TX_ANTENNA_Z 'Z'
#define TX_ANTENNA_ALL 'A'
#define TX_ANTENNA_NUMBER 3


#define MEMTYPE_UNKNOWN 0
#define MEMTYPE_FAT12 1
#define MEMTYPE_FAT16 2
#define MEMTYPE_FAT32 3 


typedef struct SU_SETTINGS 
{
	int screen_brightness;
	int locate_duration_seconds;
	int screen_power_off_seconds;
	int ZC_trigger_level;	// threshold value for detecting zero-crossings
	int ZCx_trigger_adjust;			// adjust value to be added to ZC_trigger_level for X
	int ZCy_trigger_adjust;			// adjust value to be added to ZC_trigger_level for y
	int ZCz_trigger_adjust;			// adjust value to be added to ZC_trigger_level for z
	int K1;
	int BeepVolume;
	char advanced_interface;
	char enable_audio_blanking;
	char enable_beep_on_VLF_packet; 
	char enable_wireless;
	char locate_ref_meters;
	char enable_autocalibration;  //by Alex
} SuSettings;

typedef struct CONFIG {
	unsigned int32 id;
} Config;


typedef struct SYSERRROR {
union {
    struct {
	  unsigned int32 LowBatt:1;				// Low battery
      unsigned int32 HighBatt:1;			// 
 	  unsigned int32 SDCloseError:1;
	  unsigned int32 SDNoCard:1;		
      unsigned int32 SDError_write_error:1;	// 
 	  unsigned int32 SDError_read_error:1;
 	  unsigned int32 SDMountError:1;
	  unsigned int32 SDSpare:1;				// 
	  
	  unsigned int32 RX1_Overun:1;			// Receive buffer overflow
 	  unsigned int32 RX1_Packet_Error:1;	// Error on received packet
      unsigned int32 TX1_Overun:1;			// Transmit buffer overflow
	  unsigned int32 TX1_Packet_Overun:1;	// Can't send all packets - some skipped
 	  unsigned int32 RX2_Overun:1;			// Receive buffer overflow
 	  unsigned int32 RX2_Packet_Error:1;	// Error on received packet
      unsigned int32 TX2_Overun:1;			// Transmit buffer overflow
	  unsigned int32 TX2_Packet_Overun:1;	// Can't send all packets - some skipped
	  
	  unsigned int32 U1_ParityError:1;
	  unsigned int32 U1_FramingError:1;
	  unsigned int32 U1_OverunError:1;
	  unsigned int32 U1_PacketTimeout:1;
	  unsigned int32 U2_ParityError:1;
	  unsigned int32 U2_FramingError:1;
	  unsigned int32 U2_OverunError:1;
	  unsigned int32 U2_PacketTimeout:1;
	  
  	  unsigned int32 MU_DecodeError:1;
  	  unsigned int32 U3_TXError:1;
  	  unsigned int32 U3_RXError:1;
  	  unsigned int32 U4_TXError:1;
  	  unsigned int32 U4_RXError:1;
	  unsigned int32 VLF_Overun:1;
	  unsigned int32 wireless_timeout:1;
	  unsigned int32 Wireless_Overun:1;
  };
    struct {
	  unsigned int32 value;
   };
  };
} SystemError;

typedef struct UNITFLAGS {
	union {
		struct {
			unsigned int16 DebugMisc:1;
	 		unsigned int16 SendTime:1;
			unsigned int16 SendBattMV:1;
#ifdef SU_APP									// SU_APP
			unsigned int16 SendSUMsgs:1;
			unsigned int16 SendMUMsgs:1;
			unsigned int16 SendRSSI:1;
			unsigned int16 AutoLCDPowerOff:1;
			unsigned int16 DebugSLCD:1;
			unsigned int16 SendLog:1;
			unsigned int16 DebugSD:1;
	 		unsigned int16 LogStatus:1;
			unsigned int16 DebugButtons:1;
			unsigned int16 WirelessEnabled:1;
			unsigned int16 AutoCalibrationEnabled:1;		//by Alex
	 		unsigned int16 debugWireless:1;
	 		unsigned int16 bit14:1;
	 		unsigned int16 bit15:1;
#else	 		
#ifdef MU_APP								// MU_APP
			unsigned int16 bit3:1;
			unsigned int16 bit4:1;
			unsigned int16 SendRSSI:1;
	 		unsigned int16 bit6:1;
			unsigned int16 SendAccel:1;
			unsigned int16 DebugAccel:1;
	 		unsigned int16 DisableAccel:1;
	 		unsigned int16 bit10:1;
	 		unsigned int16 bit11:1;
			unsigned int16 bit12:1;
			unsigned int16 bit13:1;
	 		unsigned int16 bit14:1;
	 		unsigned int16 bit15:1;
#else										// SU_RX AND SU_TX
			unsigned int16 bit3:1;
			unsigned int16 bit4:1;
			unsigned int16 SendRSSI:1;
	 		unsigned int16 bit6:1;
			unsigned int16 bit7:1;
			unsigned int16 bit8:1;
			unsigned int16 bit9:1;
			unsigned int16 bit10:1;
	 		unsigned int16 bit11:1;
#ifdef SU_TX
			unsigned int16 WirelessEnabled:1;		// SU_TX has a wirless module
			unsigned int16 debugWireless:1;
#else
			unsigned int16 bit12:1;
			unsigned int16 bit13:1;
#endif
	 		unsigned int16 bit14:1;
	 		unsigned int16 bit15:1;
#endif
#endif	 
	
	 	};
	    struct {
		  	unsigned int16 value;
	   	};
	};
    	
#ifdef WIN32
void SetToDefault() {
	isValid = 1;
	AutoRecord = 1;
	AdaptiveFMScan = 1;
	RecordOnAccelTrigger = 1;
	ScanGPS = 0;
	
	RecordtoMemory = 1;
	SendRecording = 0;
	Zigbeeenabled = 1;
	AutoLCDPowerOff = 1;

	sendBits = 0;
	debugZigbee = 0;
	DebugSD = 0;
	DebugMisc = 0;
	spare = 0;
}
#endif
} UnitFlags;

typedef struct BOOT_STATUS {
	unsigned int16 RCON;
	unsigned int16 INTCON1;
	char id[10];
	char version[10];
} BootStatus;


typedef struct MAC_ADDR8_STRUCT
{
    unsigned int8 v[8];
#ifdef WIN32
	void set(unsigned char type, unsigned int32 id)
	{
		v[0] = (id) & 0xff;
		v[1] = (id>>8) & 0xff;
		v[2] = (id>>16) & 0xff;
		v[3] = (id>>24) & 0xff;
		v[4] = type;
		v[5] = MINERADIO_OUI_5;
		v[6] = MINERADIO_OUI_6;
		v[7] = MINERADIO_OUI_7;
	}
	bool operator==(const MAC_ADDR8_STRUCT& other) { return (memcmp(v,other.v,8)==0)?true:false; };
	bool operator!=(const MAC_ADDR8_STRUCT& other) { return (memcmp(v,other.v,8)==0)?false:true; };

	unsigned long GetID()
	{
		unsigned long UnitID =  v[3] << 24;
		UnitID += v[2] << 16;
		UnitID += v[1] << 8;
		UnitID += v[0];
		return UnitID;
	}
#endif
} MAC_ADDR8;

typedef struct _WI_NETWORK_ENTRY 
{
    unsigned int16 isValid:1;     	// 1 = this entry is valid, 0 = this entry is not valid
    unsigned int16 bRoute:1;		// 1 = route this device, 0 = don't route it
    unsigned int16 spare:14;     	//
    unsigned int8 rssi;
	unsigned int8 lqi;
    MAC_ADDR8 macAddress;
} WI_NetworkEntry;


typedef struct SDMEMORY_PROPERTIES 
{
	int16 MemType:4;
	int16 spare:12;
	unsigned int16 MemBytesPerSector;
	int32 MemSectorCapacity;
	int32 MemSectorUsed;
	int32 MemFolders;
	int32 MemFiles;
} MemProperties;

//typedef struct SDMEMORY_FILEENTRY
//{
//	char path[14];		
//	char name[14];
//  unsigned int32   filesize;         // The size of the file 
//    unsigned int32   timestamp;        // The last modified time of the file that has been found (create time for directories)
//	unsigned int16	 handle;	   // This is used when fetching a file so that name can be correlated to packet
//} MemFileEntry;

typedef struct LOG_FILE_DATA {
	unsigned long id;		// # of log file						
	char filename[15];		// filename
	time_t ftime;
	long size;				// 
} LogFileData;


struct SDMEMORY_FILE_HEADER {
	unsigned int16	 handle;		// id associated with this file
	unsigned int32   filepos;		// position in file that this block belongs to
};

typedef struct SDMEMORY_FILE
{
	struct SDMEMORY_FILE_HEADER header;
	char body[500];					// should be no larger than maximum packet size - sizeof magic number + sizeof filepos
} SDMemoryFileBlock;


#ifdef _WIN32
#pragma pack(pop)   /* restore original alignment from stack */
#endif

#endif // __SU_STRUCT_H
