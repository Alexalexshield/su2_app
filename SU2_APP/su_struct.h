#ifndef __SU_STRUCT_H
#define __SU_STRUCT_H
/**********************************************************************
*
**********************************************************************/

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



/////////////////////////  SU2_APP TEXT FUNCTIONS //////////////////////////////////////////////////
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
#define TOGGLE_LOG_SEARCH 'l'
#define TOGGLE_DEBUG_SLCD 's'

#define MEM_PROPERTIES 'm'
#define TOGGLE_AUTOLCDSHUTDOWN 'p'
#define SWRESET 'r'
#define TOGGLE_SENDTIME 't'
#define GETVERSION 'v'

#define SD_WRITETEST 'W'
#define SD_READTEST 'R'

/////////////////////////  SU2_TX TEXT FUNCTIONS //////////////////////////////////////////////////
#define SEND_TX_ACK 'a'
#define SEND_TX_NACK 'n'
#define TOGGLE_SENDBATT 'b'
#define TOGGLE_DEBUG_MISC 'c'
#define GET_FLASH_CONFIG 'f'
#define GET_RAM_CONFIG 'g'
#define GET_HELP 'h'
#define TOGGLE_LEDS 'l'
#define SWRESET 'r'
#define TOGGLE_SENDTIME 't'
#define GETVERSION 'v'
#define SEND_DETECT_XMSG 'x'
#define SEND_DETECT_YMSG 'y'
#define SEND_DETECT_ZMSG 'z'


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

// MESSAGE SENT TO TELL THE SU_VLF_TX MODULE TO SEND A LOCATE MESSAGE TO A SPECIFIC MU
#define SU_TX_LOCATE_CSV_HEADER "$TXLOC"

// MESSAGE SENT FROM SU_VLF_TX MODULE TO THE SU to indicate that the last command was successfully sent
#define SU_TX_ACK_CSV_HEADER "$TXACK"

// MESSAGE SENT FROM SU_VLF_TX MODULE TO THE SU to indicate that the last command was ignored
#define SU_TX_NACK_CSV_HEADER "$TXNAK"


////////////////////////////////// CSV MESSAGES TO AND FROM THE SU_VLF_RX MODULE /////////////////////////////////////////

// MESSAGE SENT TO TELL THE SU_VLF_RX MODULE TO PUT ITSELF INTO STANDBY, LOCATE OR DECODE MODE
#define SU_RX_MODE_CSV_HEADER "$RXMOD"
#define SU_RX_MODE_STANDBY	 "Standby"
#define SU_RX_MODE_DECODE	 "Decode"
#define SU_RX_MODE_LOCATE	 "Locate"
#define MODE_STANDBY 0
#define MODE_DECODE 1
#define MODE_LOCATE 2

// MESSAGE SENT FROM THE SU_VLF_RX module to the SU unit with the response from an MU to a detect message
#define SU_RX_DETECT_CSV_HEADER "$RXDET"

// MESSAGE SENT FROM SU_VLF_RX MODULE TO THE SU with the response from a MU
#define SU_RX_RESPONSE_CSV_HEADER "$RXRSP"

// MESSAGE SENT FROM SU_VLF_RX MODULE TO THE SU to indicate that an error occurred
#define SU_RX_ERROR_CSV_HEADER "$RXERR"


#define PROTOCOL_V1 0
#define PROTOCOL_V2 1
#define PROTOCOL_MAX 2

#define LOCATE_DURATION_MIN 5
#define LOCATE_DURATION_DEFAULT 10
#define LOCATE_DURATION_MAX 60

#define TX_ANTENNA_NONE '0'
#define TX_ANTENNA_X 'X'
#define TX_ANTENNA_Y 'Y'
#define TX_ANTENNA_Z 'Z'
#define TX_ANTENNA_NUMBER 3


#define MEMTYPE_UNKNOWN 0
#define MEMTYPE_FAT12 1
#define MEMTYPE_FAT16 2
#define MEMTYPE_FAT32 3 


typedef struct SU_SETTINGS {
	int screen_brightness;
	int locate_duration_seconds;
	int screen_power_off_seconds;
	int protocol;
} SuSettings;

typedef struct SUCONFIG {
	unsigned int16 id;
} SUConfig;


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
	  unsigned int32 spare:3;
  };
    struct {
	  unsigned int32 value;
   };
  };
} SystemError;

typedef struct UNITFLAGS {
union {
    struct {
 		unsigned int16 LogStatus:1;
 		unsigned int16 SendTime:1;
		unsigned int16 SendBattMV:1;
		unsigned int16 SendSUMsgs:1;
		unsigned int16 SendMUMsgs:1;
		unsigned int16 LogSearch:1;
		unsigned int16 AutoLCDPowerOff:1;
		unsigned int16 DebugSLCD:1;
		unsigned int16 SendLog:1;
		unsigned int16 DebugSD:1;
		unsigned int16 DebugMisc:1;
 		unsigned int16 spare:5;
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
