/**************************************************************************************************

**************************************************************************************************/

#ifndef VLF_PKT_H
#define VLF_PKT_H

/*************************************************************************************************/
/************************************* Included Files ********************************************/
/*************************************************************************************************/

#include "system.h"
#include "vlf.h"

/*************************************************************************************************/


/************************************ Defined Literals *******************************************/
/*************************************************************************************************/

// Hardware constants
#define  VLF_LOC_ONESHOT_TX_PERIOD_MS        ((U32)1000)           /**< The period for which to transmit a location signal in one shot mode */                
#define  VLF_LOC_CONT_TX_PERIOD_MS           ((U32)300)            /**< The period for which to transmit a location signal in continuous mode */                
#define  VLF_LOC_CONT_WAIT_PERIOD_MS         ((U32)101)            /**< The period for which to wait for a location signature in continuous mode (+1 to correct for TX BIT timer granularity)*/                
#define  VLF_LOC_CONT_INIT_TX_HOLDOFF_MS     ((U32)10)
#define  VLF_LOC_CONT_INIT_WAIT_PERIOD_MS    ((U32)1000)
#define  VLF_LOC_CONT_MISSED_SIGS            ((U32)6)              /**< The number of missed signatures before cancelling continuous location mode */                

// Packet sizes
//R.S. - renamed the word symbols to bits - the use of symbols is otherwise very confusing
#define  PCKT_PREAMBLE_SIZE                  12                      /**< The size in bits of the packet preamble */
#define  OPEN_FLAG                           0x5A5                   /**< The opening flag value */
#define  PCKT_LOC_SIG_SIZE                   7                       /**< The size in bits of the location signature */
#define  PCKT_TYPE_SIZE                      4                       /**< The size in bits of the packet type */
//#define  LOC_SIGNATURE                       0x6B                    /**< The location signature value */
#define  LOC_SIGNATURE                       0x7F                    /**< The location signature value */
#define  LOC_SIGNTAURE_SIZE                  7                       /**< The size in bits of the location signature */
#define  EVAC_SIGNATURE                      0xBEAF                  /**< The evacuation signature */


// Packet attributes
#define  PCKT_PREAMBLE_SIZE_MU_TO_SU_P2      8                       /**< The size in bits of the packet preamble */
#define  PCKT_PREAMBLE_SIZE_SU_TO_MU_P2      12                       /**< The size in bits of the packet preamble */

// Convolutional encoding and Viterbi decoding
#define  VITERBI_N                           2                       /**< The rate of the convolutional code used */
#define  VITERBI_K                           5                       /**< The constraint length of the code */
#define  CODING_TAIL_SIZE                    ((VITERBI_K-1)*VITERBI_N) /**< The length of the coding tail - 8 bits in this case*/


/*************************************************************************************************/
/************************************* Defined Macros ********************************************/
/*************************************************************************************************/

#define CODED_TX_PAYLOAD_SIZE( type )     ((U8)((uncoded_tx_data_size_au8[ type ] + 1) * 2 + 1))
#define CODED_RX_PAYLOAD_SIZE( type )     ((U8)((uncoded_rx_data_size_au8[ type ] + 1) * 2 + 1))
#define CODED_TX_PACKET_SIZE( type )      ((U8)((uncoded_tx_data_size_au8[ type ] + 1) * 2 + 3))
#define CODED_RX_PACKET_SIZE( type )      ((U8)((uncoded_rx_data_size_au8[ type ] + 1) * 2 + 3))
#define UNCODED_TX_DATA_SIZE( type )      ((U8)(uncoded_tx_data_size_au8[ type ]))
#define UNCODED_RX_DATA_SIZE( type )      ((U8)(uncoded_rx_data_size_au8[ type ]))
#define UNCODED_TX_PAYLOAD_SIZE( type )   ((U8)(uncoded_tx_data_size_au8[ type ] + 1))
#define UNCODED_RX_PAYLOAD_SIZE( type )   ((U8)(uncoded_rx_data_size_au8[ type ] + 1))

/*************************************************************************************************/
/************************************* Data Structures *******************************************/
/*************************************************************************************************/
/* 
enum VLF_COMMS_PACKET_TYPES {
   VLF_MU_DETECT = 0,
   VLF_MU_MASK,
   VLF_MU_SET_CLASS,
   VLF_MU_PING,
   VLF_MU_LOCATE,
   VLF_MU_IDENTIFY,
   VLF_MU_QUICK_SEARCH,			// formerly VLF_MU_EVACUATE,
   VLF_MAX_PACKET,
   NO_PACKET = 0xFF
};
*/

/////////////////////////////////////////////////// uncomment next line for 2-byte MU-ID /////////////////////////
#define MU_ID_3BYTE

#ifdef MU_ID_3BYTE
struct MU_ID24
{
	U8 val[3];
}	__attribute__((packed));
#endif


enum VLF_COMMS_PACKET_TYPES {
   VLF_MU_DETECT = 0,
   VLF_MU_MASK,
   VLF_MU_QUICK_SEARCH_Y,
   VLF_MU_QUICK_SEARCH_Z,
   VLF_MU_LOCATE,
   VLF_MU_IDENTIFY,
   VLF_MU_QUICK_SEARCH_X,			// formerly VLF_MU_EVACUATE,
   VLF_MU_TEST,
   VLF_MU_FULL_MASK,
   VLF_MAX_PACKET,
   NO_PACKET = 0xFF
};


//
// Actual over-the-air packets, excluding header
//

///////////////////////////////////// PROTOCOL 0 STRUCTURES //////////////////////////////////
typedef struct {              // Used to initiate a search for tags.
   U16 su_id_u16;             // Contains a unique SU ID
   U8  scope_u8;              // Sets the scope of the detect operation 
   U8  seq_u8;                // Gives a unique sequence for the detect operation 
} TagDetectCmdTSt_P1;         // (If the MSB of scope is set, this indicates a new detect sequence, i.e. tags' mask statuses are reset

typedef struct {              // Sent by the tag in response to a detect broacast
   U32 id_u32;                // Contains the tag's 32-bit ID
} TagDetectEvTSt_P1;

typedef struct {              // This is the generic response to all unicast commands
   U8 mobility_u8;            // The mobility score of the tag
   U8 power_u8;               // Used to describe the relative power of the tag in the last packet sent
} TagRespTSt_P1;

typedef struct {              // Used to mask the tag from responding to the current detect request
   U32 id_u32;                // Contains the tag's 32-bit ID
   U16 su_id_u16;             // Unique SU ID
   U8  seq_u8;                // Detect sequence to mask
} TagMaskCmdTSt_P1;

typedef struct {              // Used to set the class of a tag (this affects how it responds to detect requests)
   U32 id_u32;                // Contains the tag's 32-bit ID
   U16 su_id_u16;             // Unique SU ID
   U8  class_u8;              // Class of the tag
} TagSetClassCmdTSt_P1;

typedef struct {              // Ping the tag
   U32 id_u32;                // Contains the tag's 32-bit ID
} TagPingCmdTSt_P1;

typedef struct {              // Instruct the tag to transmit a continuous carrier signal for location purposes
   U32 id_u32;                // Contains the tag's 32-bit ID
   U8  secs_u8;               // Seconds to transmit LOC signal for. A value of zero indicates a transmission
                              // that is prolonged by the SU periodically transmitting a signature as long as it
                              // wants the MU to carry on transmitting
} TagLocateCmdTSt_P1;

typedef struct {              // Used to indicate that the tag has been found and should respond to no more detect requests
   U32 id_u32;                // Contains the tag's 32-bit ID
} TagIdentifyCmdTSt_P1;
         
typedef struct {              // Used to indicate that the tag should notify the wearer to evacuate
   U16   sig_u16;             // The evacuate signature (0xBEAF)
} TagEvacCmdTSt_P1;

typedef struct {              // This is the generic response to all unicast commands
	U32 id_u32;               // Contains the tag's 32-bit ID
	U8 mobility;
	U8 power;
} __attribute__((packed)) MUResponse_P1;         // 1 BYTE PACKING

////////////////////////////////////// PROTOCOL 2 SU TO MU STRUCTURES /////////////////////////////////////////////

typedef struct {              // Used to initiate a search for tags.
   U8 su_id_u8;               // Contains a unique SU ID
   U8  seq_u8;                // Gives a unique sequence for the detect operation 
} DetectCommand_P2;           // 2 BYTES	

typedef struct {              // Used to mask the tag from responding to the current detect request
#ifdef MU_ID_3BYTE			  
	struct MU_ID24 id_u24;		// Contains the tag's 24-bit ID	
#else
   	U16 id_u16;                	// Contains the tag's 16-bit ID
#endif
   U8 su_id_u8;               // Unique SU ID
   U8  seq_u8;                // Detect sequence to mask
} MaskCommand_P2;			  // 6 BYTES	

typedef struct {              // Instruct the tag to transmit a continuous carrier signal for location purposes
#ifdef MU_ID_3BYTE			  
	struct MU_ID24 id_u24;		// Contains the tag's 24-bit ID	
#else
   	U16 id_u16;                	// Contains the tag's 16-bit ID
#endif
   U8  secs_u8;               // Seconds to transmit LOC signal for. A value of zero indicates a transmission
                              // that is prolonged by the SU periodically transmitting a signature as long as it
                              // wants the MU to carry on transmitting
} LocateCommand_P2;			  // 5 BYTES

typedef struct {              // Used to initiate a QuickSearch for tags.
   U8 su_id_u8;               // Contains a unique SU ID
   U8  seq_u8;                // Gives a unique sequence for the detect operation 
} QuickSearchCommand_P2;      // 1 BYTE    

//FULL_MASK
typedef struct {              // Used to fully mask the tag against broadcast messages for a duration
	#ifdef MU_ID_3BYTE			  
		struct MU_ID24 id_u24;		// Contains the tag's 24-bit ID	
	#else
	   	U16 id_u16;                	// Contains the tag's 16-bit ID
	#endif
    U8 minutes_u8;	               // Minutes for which tag is masked against all broadcast messages
} FullMaskCommand_P2;		  // 6 BYTES	
//FULL_MASK

// THESE DEFINES ARE USED FOR ANTENNA DELAYS SINCE WE SEND THE COMMAND ON X,Y AND THEN Z FROM THE SU TO THE MU
// SO THE DELAYS CORRESPOND TO THE TIME IT TAKES TO SEND THE VARIABLE-SIZED COMMAND at 100 bits/sec or 80 ms per byte
#define SU_TO_MU_MS_PER_BYTE 85 
#ifdef MU_ID_3BYTE
#define LOCATE_RX_DELAY_MS_P2 		(((4+3)*SU_TO_MU_MS_PER_BYTE)+10)
#define MASK_RX_DELAY_MS_P2 		(((5+3)*SU_TO_MU_MS_PER_BYTE)+10)
#else
#define LOCATE_RX_DELAY_MS_P2 		(((3+3)*SU_TO_MU_MS_PER_BYTE)+10)
#define MASK_RX_DELAY_MS_P2 		(((4+3)*SU_TO_MU_MS_PER_BYTE)+10)
#endif

#define QUICKSEARCH_RX_DELAY_MS_P2 	(((0+3)*SU_TO_MU_MS_PER_BYTE)+10)
#define DETECT_RX_DELAY_MS_P2 		(((2+3)*SU_TO_MU_MS_PER_BYTE)+10)


//R.S. - this is the data size for different types of packets that are sent by the SU to the MU
//const U8 uncoded_murx_data_size_au8_protocol_1[VLF_MAX_PACKET] = {
//   4, // VLF_MU_DETECT,
//   7, // VLF_MU_MASK,
//   7, // VLF_MU_SET_CLASS,
//   4, // VLF_MU_PING,
//   5, // VLF_MU_LOCATE,
//   4, // VLF_MU_IDENTIFY
//   2  // VLF_MU_QUICK_SEARCH
//};
#define VERTERBI_ENCODED_SIZE( len ) ((U8)((len + 1) * 2 + 1))
#define LOCATE_RX_DELAY_MS_P1 		(((VERTERBI_ENCODED_SIZE( 5 )+2)*SU_TO_MU_MS_PER_BYTE)+20)
#define DETECT_RX_DELAY_MS_P1 		(((VERTERBI_ENCODED_SIZE( 4 )+2)*SU_TO_MU_MS_PER_BYTE)+20)
#define MASK_RX_DELAY_MS_P1 		(((VERTERBI_ENCODED_SIZE( 7 )+2)*SU_TO_MU_MS_PER_BYTE)+20)

////////////////////////////////////// PROTOCOL 2 MU TO SU STRUCTURES /////////////////////////////////////////////



	
typedef struct {              // This is the generic response to all unicast commands
#ifdef MU_ID_3BYTE
	struct MU_ID24 id_u24;
#else
	U16 id_u16;               // Contains the tag's 16-bit ID
#endif
	union {	
		struct {
				U8 mobility:4;
				U8 bit5:1;
				U8 bit6:1;
				U8 batteryproblem:1;
				U8 masked:1;
		};
		U8 status;
	} m ;
} __attribute__((packed)) GenericResponse_P2;         // 1 BYTE PACKING

#define USE_CRC8

#ifdef MU_ID_3BYTE
#define MU_PROTOCOL2_RESPONSE_PAYLOADSIZE 4
#else
#define MU_PROTOCOL2_RESPONSE_PAYLOADSIZE 3
#endif

         
#define MAXIMUM_PACKET_BYTES 8
//R.S. This structure is used to RX and TX a packet. 
struct PacketBufferSt {
   U8                       type_u8;			//R.S. = This character holds the packet type - only the bottom 4 bits are actually needed
   U8                       num_u8;				//R.S. - This is a counter that is used to change the state of a transer based on the number of bits sent
   union {										//R.S. - This is a union of all the different types of message bodies
      U8                    data_au8[ MAXIMUM_PACKET_BYTES ];		//R.S. - This buffer is used as a convenient way to address the following structures
	  
	  /////////////////////////////////////////// PROTOCOL 0 STRUCTURES ///////////////////////////////	
      TagDetectCmdTSt_P1       det_cmd_st_P1;
      TagMaskCmdTSt_P1         mask_cmd_st_P1;
      TagSetClassCmdTSt_P1     setcls_cmd_st_P1;
      TagPingCmdTSt_P1         ping_cmd_st_P1;
      TagLocateCmdTSt_P1       loc_cmd_st_P1;
      TagIdentifyCmdTSt_P1     identify_cmd_st_P1;
      TagEvacCmdTSt_P1         evac_cmd_st_P1;
      TagDetectEvTSt_P1        det_ev_st_P1;
      TagRespTSt_P1            resp_st_P1;
      
      ////////////////////////////////////////// PROTOCOL 2 STRUCTURES //////////////////////////////////
      
      DetectCommand_P2	        detect_cmd_P2;
      LocateCommand_P2			locate_cmd_P2;
      MaskCommand_P2			mask_cmd_P2;
   	  QuickSearchCommand_P2 	quick_search_cmd_P2;
	  FullMaskCommand_P2		fullmask_cmd_P2;	
  	  GenericResponse_P2   		generic_resp_P2;
   }                        u;
};

// THIS IS BASED ON THE size of the preamble plus the notion that the encoded data size plus a checksum is x2 plus 1 byte
#define MAXIMUM_VLF_PREAMBLE_BITS 12
#define MAXIMUM_VLF_ENCODED_BYTES (2+(MAXIMUM_PACKET_BYTES+1)*2+1)
#define PCKT_PREAMBLE_BIT_COUNT (PCKT_PREAMBLE_SIZE*VLF_NUM_RX_BIT_SUBPERIODS)
#define LOCATE_PREAMBLE_BIT_COUNT (LOC_SIGNTAURE_SIZE*VLF_NUM_RX_BIT_SUBPERIODS)
#define PCKT_TYPE_BIT_COUNT (PCKT_TYPE_SIZE*VLF_NUM_RX_BIT_SUBPERIODS)
#define PCKT_DATA_MAX_BIT_COUNT ((MAXIMUM_VLF_ENCODED_BYTES*8) * VLF_NUM_RX_BIT_SUBPERIODS)



extern const U8 open_flag_au8[ PCKT_PREAMBLE_SIZE ];
extern const U8 loc_sig_au8[ LOC_SIGNTAURE_SIZE ];
extern const U8 open_flag_au8Bits[ PCKT_PREAMBLE_BIT_COUNT ];

extern const U8 uncoded_mutx_data_size_au8[];

// UNCOMMENT NEXT LINE TO ALLOW EDITING OF THE ZC BIT THRESHOLDS FOR TESTING
#define ENABLE_ZC_THRESHOLD_TESTING
#ifdef ENABLE_ZC_THRESHOLD_TESTING
extern char vlf_high_threshold; // = VLF_RX_HIGH_THRESHOLD;
extern char vlf_low_threshold;	// = VLF_RX_LOW_THRESHOLD;
#endif
/**
 * An enumeration describing the RX states on the VLF interface
 */
enum VLF_RX_STAGES {
   VLF_RX_STANDBY,					// Not currently decoding 
   VLF_RX_BUFFERING,			// FILLING PREAMBLE BUFFER 
   VLF_RX_PREAMBLE,          	// Seeking A PREAMBLE (OPEN) bit pattern
   VLF_RX_TYPE,             	// looking for preamble 
   VLF_RX_DATA,          		// looking for data payload
   VLF_RX_PACKET_READY,
   VLF_RX_PACKET_ERROR
};

#define PREAMBLE_DECODE 0
#define PREAMBLE_LOCATE 1

// Note, the following structure could be defined as a U8 but this is a structure on purpose in case
// we ever need to store more data than just the ZC - It could be changed to char to optimize real time queueing
typedef struct VLF_SAMPLE
{
	U8 value;		// ZC value for a 1/4 bit
//	U16 rssi;	// not enough memory for this - need to optimize before we can store rssi with 1/4 bit
} __attribute__((packed)) Vlf_Sample;

typedef struct VLF_DECODER_DATA {
	U8 aRefpreamble[PCKT_PREAMBLE_BIT_COUNT];
	U8 aRefPreambleBits[MAXIMUM_VLF_PREAMBLE_BITS];
	U8 u8Preamble_size;
	int iDecodingStage;	
	Vlf_Sample aPreamble_buffer[PCKT_PREAMBLE_BIT_COUNT];
	Vlf_Sample aType_buffer[PCKT_TYPE_BIT_COUNT];
	Vlf_Sample aData_buffer[PCKT_DATA_MAX_BIT_COUNT];	// 10 spare bytes
	int iPreamble_counter;
	int iType_counter;
	int iData_counter;
	int iData_length;
	struct PacketBufferSt vlf_packet;
	U8 preamble_type;
} Vlf_Decoder;


extern const U8 uncoded_murx_data_size_au8_protocol_2[]; 
extern const U8 uncoded_murx_data_size_au8_protocol_1[]; 

extern int CorrelatePreamble(Vlf_Decoder *p);
extern void build_test_preamble(Vlf_Decoder *p, int offset, BOOL bBellShaped);
extern U8 DecodeTypeBuffer(Vlf_Sample buffer[PCKT_TYPE_BIT_COUNT]);
extern void DecodeDataBuffer(Vlf_Decoder* p);
extern int DecodeData(U8 in_buffer[], struct PacketBufferSt* packet, int length);
extern void initVlfPacketDecoder();
extern int DecodeVlfPacket(Vlf_Decoder *p, Vlf_Sample* psample);
extern void test_preamble_correlation();
extern void build_ref_decode_preamble(Vlf_Decoder *p);
extern void build_ref_locate_preamble(Vlf_Decoder *p);
extern void swap16_byteorder(char* buf);
extern void swap32_byteorder(char* buf);
extern unsigned char GetCRC8( unsigned char crc , unsigned char ch);
extern U8 CalcCRC(U8* p, U8 check_u8, U8 len);

#endif
