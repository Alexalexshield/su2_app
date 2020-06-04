#ifndef __WI_H_
#define __WI_H_

/************************ HEADERS **********************************/
#include "WiDefs.h"
#include "su_struct.h"

/************************ DEFINITIONS ******************************/
#define FRAME_TYPE_COMMAND  0b011
#define FRAME_TYPE_DATA     0b001
#define FRAME_TYPE_BEACON   0b000
#define FRAME_TYPE_ACK      0b010

#define MAC_COMMAND_ASSOCIATION_REQUEST             0x01
#define MAC_COMMAND_ASSOCIATION_RESPONSE            0x02
#define ASSOCIATION_SUCCESSFUL                      0x00
#define ASSOCIATION_PAN_FULL                        0x01
#define ASSOCIATION_ACCESS_DENIED                   0x02
#define MAC_COMMAND_DISASSOCIATION_NOTIFICATION     0x03
#define MAC_COMMAND_DATA_REQUEST                    0x04
#define MAC_COMMAND_PAN_ID_CONFLICT_NOTIFICATION    0x05
#define MAC_COMMAND_ORPHAN_NOTIFICATION             0x06
#define MAC_COMMAND_BEACON_REQUEST                  0x07
#define MAC_COMMAND_COORDINATOR_REALIGNMENT         0x08

#define MIWI_PROTOCOL_ID 0x4D
#define MIWI_VERSION_NUM 0x10   //v1.0
#define MIWI_ACK_REQ 0x04

#define CLEAR_ALL_ENTRIES 0b11111111
#define CLEAR_NON_DIRECT_CONNECTIONS 0b00000001
#define CLEAR_NO_LONG_ADDRESS 0b00000010
#define CLEAR_NO_SHORT_ADDRESS 0b00000100  //this does not effect P2P nodes
#define CLEAR_P2P 0b00001000
#define CLEAR_NETWORKS 0b00010000
#define CLEAR_NEIGHBORS 0b00100000

//#define NETWORK 0
//#define NEIGHBOR 1


#define DATA_REQUEST_ASSOCIATION_RESPONSE 0x00
#define DATA_REQUEST_SHORT_ADDRESSES 0x01

#define NETWORKED_TRANSMISSION 0x00
#define P2P_TRANSMISSION 0x01
#define BY_HANDLE_TRANSMISSION 0x02
#define BY_LONG_ADDRESS_TRANSMISSION 0x04

/* Report type and ID definitions */
/* as a user you are able to use Report types 0x10 - 0xFF */

#define MIWI_STACK_REPORT_TYPE 0x00
#define OPEN_CLUSTER_SOCKET_REQUEST 0x10
#define OPEN_CLUSTER_SOCKET_RESPONSE 0x11
#define OPEN_P2P_SOCKET_REQUEST 0x12
#define OPEN_P2P_SOCKET_RESPONSE 0x13
#define EUI_ADDRESS_SEARCH_REQUEST 0x20
#define EUI_ADDRESS_SEARCH_RESPONSE 0x21
#define ACK_REPORT_TYPE 0x30
/* Report Type 0x06-0x0F are reserved */
#define USER_REPORT_TYPE 0x12
#define LIGHT_REPORT 0x34
#define LIGHT_TOGGLE 0x55
#define USER_REPORT_TEXT_PACKET 0x35
#define LIGHT_OFF 0x00
#define LIGHT_ON 0xFF
#define CIPHER_RETRY 3

/************************ DATA TYPE *******************************/
#define MAXIMUM_WIRELESS_DATA_PACKET 80

typedef union _MAC_FRAME_CONTROL
{
    BYTE Val;
    struct _MAC_FRAME_CONTROL_bits
    {
        BYTE frameType :3;
        BYTE securityEnabled :1;
        BYTE framePending :1;
        BYTE ACKRequest :1;
        BYTE intraPAN :1;
        BYTE :1;
    } bits;
} MAC_FRAME_CONTROL;




typedef union _MIWI_STATUS
{
    WORD Val;
    struct _MIWI_STATUS_bits
    {
        BYTE RX_PENDING         :1;
        BYTE RX_BUFFERED        :1;
        BYTE RX_ENABLED         :1;
        BYTE TX_BUSY            :1;
        BYTE TX_PENDING_ACK     :1;
        BYTE TX_SECURITY		:1;
        BYTE PHY_SLEEPING       :1;
    }bits;
} MIWI_STATUS;


typedef struct 
{
    U16 val;
} PANID;

typedef struct WI_MSG_HDR {
	MAC_FRAME_CONTROL fc;
	U8 addr_control;
	U8 sequence;
	PANID destPanID;
	MAC_ADDR8 dest;
	PANID srcPanID;
	MAC_ADDR8 source;
	U8 report_type;
	U8 payload_len;
} __attribute__((packed)) WiMsgHdr; 

typedef struct _WI_PACKET 
{
	WiMsgHdr hdr;
	BYTE payload[128];
} __attribute__((packed)) WI_Packet;


#if defined(SUPPORT_SECURITY)
typedef enum _CIPHER_MODE
{
	MODE_ENCRYPTION,
	MODE_DECRYPTION
} CIPHER_MODE;

typedef enum _CIPHER_STATUS
{
	CIPHER_SUCCESS = 0,
	CIPHER_ERROR,
	CIPHER_MIC_ERROR
} CIPHER_STATUS;

typedef struct _SECURITY_INPUT
{
	BYTE	*InputText;
	BYTE	TextLen;
	BYTE	*Header;
	BYTE	HeaderLen;
	BYTE	SourceAddress[8];
	DWORD_VAL	FrameCounter;
} SECURITY_INPUT;
#endif //#if defined(SUPPORT_SECURITY)
/************************ EXTERNAL VARIABLES **********************/

extern WI_NetworkEntry gNetworkTable[NETWORK_TABLE_SIZE];
extern WORD_VAL myPANID;
extern WORD_VAL myShortAddress;
   
/************************ FUNCTION PROTOTYPES **********************/
void WiInit(void);
void DiscardPacket(void);
#ifdef SUPPORT_SECURITY
	BOOL SendTxBuffer(BOOL AckRequested, BOOL SecurityOn);
#else
	BOOL SendTxBuffer(BOOL AckRequested);
#endif	
void SetChannel(BYTE channel);
BYTE AddNodeToNetworkTable(MAC_ADDR8* address, char lqi, char rssi);
void initMRF24J40(BYTE channel);
void MRF24J40Sleep(void);
void MRF24J40Wake(void);

void PHYSetLongRAMAddr(WORD address, BYTE value);
void PHYSetShortRAMAddr(BYTE address, BYTE value);
BYTE PHYGetShortRAMAddr(BYTE address);
BYTE PHYGetLongRAMAddr(WORD address);

void PHYDumpRegisters(void);
void PHYDumpTxFIFO(void);

extern MAC_ADDR8 g_myMACAddress;
extern volatile MIWI_STATUS MiWiStatus;
extern volatile BYTE failureCounter;
extern BYTE currentChannel;
extern unsigned long g_wireless_counter;

void ClearNetworkTable();
BYTE findNextAvailableNetworkEntry(void);
BYTE findNetworkEntry(MAC_ADDR8* LongAddress);
void DumpNetworkTable(void);
MAC_ADDR8* getNetworkAddressByIndex(BYTE index);
BYTE getIndexOfNextNetworkTypeAddress(BYTE type,BYTE startindex);
BYTE getCountOfNetworkType(BYTE type);
void SendBinaryNetworkEntry(int port, MAC_ADDR8* dest, int i);

void cw_test();

#endif

    
    
