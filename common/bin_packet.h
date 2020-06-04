#ifndef __BIN_PACKET_H
#define __BIN_PACKET_H

#ifdef _WIN32
#pragma pack(push, 2 )
#else 
#endif	

#ifdef _WIN32
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

#define ECHOVERSION '\r'
#define ESCAPE 27

#define 	STX   0x02
#define 	ETX   0x03


///////////////////////// BOOTLOADER COMMANDS AND DEFINES
#define BOOT_CMD_NACK     0x00
#define BOOT_CMD_ACK      0x01
#define BOOT_CMD_READ256_PM  0x02
#define BOOT_CMD_LOAD_PM_DATA_256 0x03
#define BOOT_CMD_LOAD_PM_DATA_512 0x04
#define BOOT_CMD_LOAD_PM_DATA_768 0x05
#define BOOT_CMD_LOAD_PM_DATA_1024 0x06
#define BOOT_CMD_LOAD_PM_DATA_1280 0x07
#define BOOT_CMD_LOAD_PM_DATA_1536 0x08
#define BOOT_CMD_WRITE_PM_ADDR 0x09
#define BOOT_CMD_LOAD_CM 0x0a
#define BOOT_CMD_RESET    0x0b
#define BOOT_CMD_READ_ID  0x0c
#define BOOT_CMD_PING 0x0d
#define PM_ROW_SIZE 64 * 8
#define CM_ROW_SIZE 8
#define CONFIG_WORD_SIZE 1
//////////////////////////////////////////////////////////////////////////

#define BINARY_PACKET_CMD 0
#define BINARY_PACKET_DATA 1
#define BINARY_CMD_GET 0
#define BINARY_CMD_SET 1
#define ESCCHAR '/'
#define BINARY_FRAMECHAR STX


typedef enum {
	CMD_RESERVED=0,
	CMD_CONFIG=1, 
	CMD_UNITSTATUS=2, 
	CMD_SCANLO=3,
	CMD_SCANHI=4,
	CMD_FULL_RECORDING=5,
	CMD_CONDENSED_RECORDING=6,
	CMD_ACCEL_MINMAX=7,
	CMD_ACCEL_SCAN=8,
	CMD_SETUNITID = 9,
	CMD_REBOOT = 10,
	CMD_BOOTLOADER_CMD_SU2 = 11,
	CMD_UNITTIME = 12,
	CMD_UNITFLAGS = 13,
	CMD_RECEIVERCMD = 14,
	CMD_RECEIVERFLAGS = 15,
	CMD_CHAR_COMMAND = 16,
	CMD_MIWIFRAMED_CMD = 17,
	CMD_MEM_PROPERTIES = 18,
	CMD_BOOTLOADER_CMD_SU2_RX = 19,
	CMD_BOOTLOADER_CMD_SU2_TX = 20,
	CMD_BOOTLOADER_CMD_MU2 = 21,
	CMD_NONE=63
} 	eCommands;


#define BINARY_PACKET_MAX_DATA_SIZE 1024
#define BINARY_PACKET_MAX_COMMAND_SIZE 512

#define PACKET_SET_BIT 0x04
#define PACKET_DATA_BIT 0x08
#define VERSION_MASK 0xe0;
#define BINARY_PACKET_HEADER_VERSION 0x01

typedef struct BINARY_PACKETHEADER {
  union {
    struct {
		unsigned int32 PacketType:1;		// 0 = PACKET_CMD, 1 = PACKET_DATA
		unsigned int32 Direction:1;			// 0 = Get(Unit to PC), 1 = Set (PC to Unit)
		unsigned int32 Spare:4;
		unsigned int32 Version:2;			// VERSION 1,2,3  
		unsigned int32 DataId:8;			// data byte for empty packets or id for streaming
		unsigned int32 Bytelen:10;			// byte length from 0 to 1024 
		unsigned int32 Command:6;			// 1 to 63 are possible commands
   };
   struct {
		unsigned char bytes[4];
   };
  };
} PHdrBits;	


typedef enum {
	BINARY_PACKET_LOOKING=0, 
	BINARY_PACKET_HEADER, 
	BINARY_PACKET_CMDSTREAM, 
	BINARY_PACKET_CHECKSUM, 
	BINARY_PACKET_READY
} eBinaryPacketStage;

struct BINARY_PACKET_HANDLER {
	unsigned char lastbyte0;					// for a sync - wait for 3 zero bytes in a row
	unsigned char lastbyte1;
	int hdrcount;
	unsigned int datacount;
	unsigned char checksum;
	eBinaryPacketStage m_PacketStage;
	PHdrBits header;
	unsigned long long start_tick;
	int skip_esc;
	int source_cnt;
	unsigned char cmdbuf[BINARY_PACKET_MAX_DATA_SIZE+2];		// this should be the maximum of several command structures
};


#ifdef _WIN32

class CPacketStreamParser {
public:
	CPacketStreamParser() { reset_packethandler(); };

	struct BINARY_PACKET_HANDLER m_RxPacket;
	int iBadChecksum;

	void reset_packethandler() {
		m_RxPacket.lastbyte0 = 0xff; // for a sync - wait for 2 zero bytes in a row
		m_RxPacket.lastbyte1 = 0xff;
		m_RxPacket.hdrcount=0;
		m_RxPacket.datacount=0;
		m_RxPacket.checksum=0;
		m_RxPacket.m_PacketStage = BINARY_PACKET_LOOKING;
		iBadChecksum = 0;
	};

	//#define TRACE_PACKETBUILDING 1
	unsigned char process_packetbyte(unsigned char rdByte)
	{
		unsigned char retbyte=0;
	//	unsigned long long elapsed_ms;
		
	//	TRACE("%c%c",HIHEXNIB(rdByte),LOHEXNIB(rdByte));

	#ifdef TRACE_PACKETBUILDING
	if (m_RxPacket.m_PacketStage != PACKET_LOOKING)
		TRACE("%02x",((unsigned char)rdByte)& 0xff);
	#endif
		switch (m_RxPacket.m_PacketStage) {
			case BINARY_PACKET_LOOKING:
				if ((m_RxPacket.lastbyte0 == BINARY_FRAMECHAR) && (m_RxPacket.lastbyte1 == BINARY_FRAMECHAR) && (rdByte == BINARY_FRAMECHAR)) { // 0x00000000 signifies a packet start
					m_RxPacket.datacount=0;
					m_RxPacket.m_PacketStage = BINARY_PACKET_HEADER;
					m_RxPacket.hdrcount = 0;
					m_RxPacket.checksum=0;
	//				m_RxPacket.start_tick = g_tick;
	#ifdef TRACE_PACKETBUILDING
					TRACE("\n Packet=");
	#endif
				}
				else {
	#ifdef TRACE_PACKETBUILDING
					if ((isprint(rdByte)))
						TRACE("%c",rdByte);
					else 
						TRACE("[%02x] ",((unsigned char)rdByte)& 0xff);
	#endif				
					retbyte = rdByte;
				}

				break;
			case BINARY_PACKET_HEADER:
				m_RxPacket.checksum+=rdByte;
				m_RxPacket.header.bytes[m_RxPacket.hdrcount++]=rdByte;
				if (m_RxPacket.hdrcount == sizeof(PHdrBits)) {
					m_RxPacket.datacount=0;
					if (m_RxPacket.header.Bytelen == 0) {
						m_RxPacket.m_PacketStage = BINARY_PACKET_CHECKSUM;
					}
					else {
							m_RxPacket.m_PacketStage = BINARY_PACKET_CMDSTREAM;
							m_RxPacket.skip_esc = 1;
	#ifdef TRACE_PACKETBUILDING
							TRACE(" ");
	#endif
					}
				}
				break;
			case BINARY_PACKET_CMDSTREAM:
				if ((rdByte == ESCCHAR) && (m_RxPacket.skip_esc))
					m_RxPacket.skip_esc = 0;
				else {
					m_RxPacket.skip_esc = 1;
					m_RxPacket.checksum+=rdByte;
					m_RxPacket.cmdbuf[m_RxPacket.datacount++] = rdByte;
					
					if (m_RxPacket.datacount >= m_RxPacket.header.Bytelen) {
						m_RxPacket.m_PacketStage = BINARY_PACKET_CHECKSUM;
					}	
					else if (m_RxPacket.datacount >= BINARY_PACKET_MAX_COMMAND_SIZE) {
						m_RxPacket.m_PacketStage = BINARY_PACKET_LOOKING;
						rdByte = 0xff;
					}
				}
				break;
			case BINARY_PACKET_CHECKSUM:
				if (m_RxPacket.checksum == rdByte) {
					m_RxPacket.m_PacketStage = BINARY_PACKET_READY;
	#ifdef TRACE_PACKETBUILDING
					TRACE("\n");
	#endif
				}
				else {
					iBadChecksum++;
					m_RxPacket.m_PacketStage = BINARY_PACKET_LOOKING;
				}
				rdByte = 0xff;	// make sure the checksum never gets mistaken for a sync character.
				break;
			default:
				break;
		}
		m_RxPacket.lastbyte0 = m_RxPacket.lastbyte1;
		m_RxPacket.lastbyte1 = rdByte;

	//	if (m_RxPacket.m_PacketStage != PACKET_LOOKING) {
	//		elapsed_ms = g_tick - m_RxPacket.start_tick;
	//		if (elapsed_ms > 3000) {	// packets should not take longer than 3 seconds to build
	//			m_RxPacket.packet_timeouts++;
	//			m_RxPacket.m_PacketStage = PACKET_LOOKING;
	//		}
	//	}

		return retbyte;
	};	
};

class CSRPacket {
public:
	PHdrBits m_header;
	unsigned char m_buffer[BINARY_PACKET_MAX_DATA_SIZE];		// this should be the maximum of several command structures
	unsigned char m_checksum;
//	MAC_ADDR8 source_address;
	BOOL isChecksumOk() {
		unsigned char checksum = 0;
		for (int i=0;i<sizeof(m_header);i++)
			checksum += m_header.bytes[i];
//		if (m_header.SourceAddressIncluded) {
//			for (int i=0;i<sizeof(MAC_ADDR8);i++)
//				checksum += source_address.v[i];
//		}
		ASSERT(m_header.Bytelen < BINARY_PACKET_MAX_DATA_SIZE);
		for (unsigned int i=0;i< m_header.Bytelen;i++) {
			checksum+=m_buffer[i];
		}
		if (checksum == m_checksum)
			return TRUE;
		else
			return FALSE;
	}
	void reset() { 
		m_header.Version = BINARY_PACKET_HEADER_VERSION;
		m_header.PacketType = BINARY_PACKET_DATA;
		m_header.Direction = BINARY_CMD_GET;
//		m_header.SourceAddressIncluded = 0;
		m_header.Bytelen = 0;
		m_header.Spare = 3;
		m_header.DataId = 3;
	};
};
#endif

///////////////////////////////////////////////////////////////////////////



typedef struct SYSERRROR {
union {
    struct {
 	  unsigned int32 isValid:1;
	  unsigned int32 wireless_timeout:1;		
	  unsigned int32 LowBatt:1;			// Low battery
      unsigned int32 Tamper:1;			// Tamper bit set
      unsigned int32 Test:1;			// Test pin set	
 	  unsigned int32 NoGPS:1;
 	  unsigned int32 SDError:1;
	  unsigned int32 Wireless_Overun:1;	// Can't keep ahead of processing
	  
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
	  
 	  unsigned int32 WirelessProblem:1;
 	  unsigned int32 HighBatt:1;
 	  unsigned int32 LowRSSI:1;
 	  unsigned int32 spare:5;
  };
    struct {
	  unsigned int32 value;
   };
  };
} SystemError;



#ifdef _WIN32
#pragma pack(pop)   /* restore original alignment from stack */
#else
// these are prototypes for the binary packet handler used by the firmware

unsigned char SendBinaryPacketHeader(PHdrBits* phdr, int port);
char SendBinaryPacket(PHdrBits* phdr, unsigned char* pbuf, int port, char max_retry);
void reset_bin_packethandler(struct BINARY_PACKET_HANDLER* phandler);
unsigned char SendBinaryPacketData(PHdrBits* phdr,unsigned char* cptr,int port); 
unsigned char process_bin_packetbyte(struct BINARY_PACKET_HANDLER* phandler, unsigned char rdByte);
#endif



#endif //__BIN_PACKET_H
