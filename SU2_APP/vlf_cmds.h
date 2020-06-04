/**************************************************************************************************

  Copyright (c) 2008 Embedded IQ cc. All rights reserved.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR EMBEDDED IQ BE LIABLE FOR ANY DIRECT, INDIRECT, 
  INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**************************************************************************************************/

#ifndef VLF_PKT_H
#define VLF_PKT_H

/*************************************************************************************************/
/************************************* Included Files ********************************************/
/*************************************************************************************************/

#include "system.h"

/*************************************************************************************************/
/************************************ Defined Literals *******************************************/
/*************************************************************************************************/

// Hardware constants
//R.S. - Chenged Symbol to bit to make documentation easier to understand
// Frequencies and periods
#define  VLF_TX_FREQUENCY                    (35714.2857)     /**< The VLF TX frequency */
#define  VLF_TX_BIT_FREQUENCY             (190.7349)       /**< The VLF TX BIT frequency */
#define  VLF_RX_FREQUENCY                    (8000)           /**< The VLF RX frequency */
#define  VLF_RX_BIT_FREQUENCY             (100)            /**< The VLF RX bit frequency */
#define  VLF_NUM_RX_BIT_SUBPERIODS        (4)              /**< The number of periods (cycles) in an RX bit */
#define  VLF_RX_HIGH_THRESHOLD               (VLF_RX_FREQUENCY/VLF_RX_BIT_FREQUENCY/VLF_NUM_RX_BIT_SUBPERIODS/2 - 1) /**< The threshold between a high and a low bit */
#define  VLF_LOC_ONESHOT_TX_PERIOD_MS        ((U32)1000)           /**< The period for which to transmit a location signal in one shot mode */                
#define  VLF_LOC_CONT_TX_PERIOD_MS           ((U32)300)            /**< The period for which to transmit a location signal in continuous mode */                
#define  VLF_LOC_CONT_WAIT_PERIOD_MS         ((U32)101)            /**< The period for which to wait for a location signature in continuous mode (+1 to correct for TX BIT timer granularity)*/                
#define  VLF_LOC_CONT_INIT_TX_HOLDOFF_MS     ((U32)10)
#define  VLF_LOC_CONT_INIT_WAIT_PERIOD_MS    ((U32)1000)
#define  VLF_LOC_CONT_MISSED_SIGS            ((U32)6)              /**< The number of missed signatures before cancelling continuous location mode */                

// Packet sizes
//R.S. - renamed the word symbols to bits - the use of symbols is otherwise very confusing
#define  PCKT_PREAMBLE_SIZE                  12                      /**< The size in bits of the packet preamble */
#define  PCKT_LOC_SIG_SIZE                   7                       /**< The size in bits of the location signature */
#define  PCKT_TYPE_SIZE                      4                       /**< The size in bits of the packet type */

// Packet attributes
#define  OPEN_FLAG                           0x5A5                   /**< The opening flag value */
#define  LOC_SIGNATURE                       0x6B                    /**< The location signature value */
#define  LOC_SIGNTAURE_SIZE                  7                       /**< The size in bits of the location signature */
#define  EVAC_SIGNATURE                      0xBEAF                  /**< The evacuation signature */

// Convolutional encoding and Viterbi decoding
#define  VITERBI_N                           2                       /**< The rate of the convolutional code used */
#define  VITERBI_K                           5                       /**< The constraint length of the code */
#define  SCORE_METRIC_SHIFT                  3                       /**< The viterbi metric score shift value */
#define  SCORE_METRIC_FS                     ((VLF_RX_FREQUENCY/VLF_RX_BIT_FREQUENCY) >> SCORE_METRIC_SHIFT) /**< A full scale metric */
#define  SCORE_METRIC_LOW_THRESH             (SCORE_METRIC_FS*4/5)   /**< The maximum score metric for a low bit */
#define  SCORE_METRIC_HIGH_THRESH            (SCORE_METRIC_FS*2/5)   /**< The minimum score metric for a high bit */
#define  SCORE_METRIC_MID_THRESH             (SCORE_METRIC_FS*3/5)   /**< Somewhere in between */
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
 
enum VLF_COMMS_PACKET_TYPES {
   VLF_MU_DETECT = 0,
   VLF_MU_MASK,
   VLF_MU_SET_CLASS,
   VLF_MU_PING,
   VLF_MU_LOCATE,
   VLF_MU_IDENTIFY,
   VLF_MU_EVACUATE,
   VLF_MAX_PACKET,
   NO_PACKET = 0xFF
};


//
// Actual over-the-air packets, excluding header
//

typedef struct {              // Used to initiate a search for tags.
   U16 su_id_u16;             // Contains a unique SU ID
   U8  scope_u8;              // Sets the scope of the detect operation 
   U8  seq_u8;                // Gives a unique sequence for the detect operation 
} TagDetectCmdTSt;            // (If the MSB of scope is set, this indicates a new detect sequence, i.e. tags' mask statuses are reset

typedef struct {              // Sent by the tag in response to a detect broacast
   U32 id_u32;                // Contains the tag's 32-bit ID
} TagDetectEvTSt;

typedef struct {              // This is the generic response to all unicast commands
   U8 mobility_u8;            // The mobility score of the tag
   U8 power_u8;               // Used to describe the relative power of the tag in the last packet sent
} TagRespTSt;

typedef struct {              // Used to mask the tag from responding to the current detect request
   U32 id_u32;                // Contains the tag's 32-bit ID
   U16 su_id_u16;             // Unique SU ID
   U8  seq_u8;                // Detect sequence to mask
} TagMaskCmdTSt;

typedef struct {              // Used to set the class of a tag (this affects how it responds to detect requests)
   U32 id_u32;                // Contains the tag's 32-bit ID
   U16 su_id_u16;             // Unique SU ID
   U8  class_u8;              // Class of the tag
} TagSetClassCmdTSt;

typedef struct {              // Ping the tag
   U32 id_u32;                // Contains the tag's 32-bit ID
} TagPingCmdTSt;

typedef struct {              // Instruct the tag to transmit a continuous carrier signal for location purposes
   U32 id_u32;                // Contains the tag's 32-bit ID
   U8  secs_u8;               // Seconds to transmit LOC signal for. A value of zero indicates a transmission
                              // that is prolonged by the SU periodically transmitting a signature as long as it
                              // wants the MU to carry on transmitting
} TagLocateCmdTSt;

typedef struct {              // Used to indicate that the tag has been found and should respond to no more detect requests
   U32 id_u32;                // Contains the tag's 32-bit ID
} TagIdentifyCmdTSt;
         
typedef struct {              // Used to indicate that the tag should notify the wearer to evacuate
   U16   sig_u16;             // The evacuate signature (0xBEAF)
} TagEvacCmdTSt;
         

//R.S. This structure is used to RX and TX a packet. 
struct PacketBufferSt {
   U8                       type_u8;			//R.S. = This character holds the packet type - only the bottom 4 bits are actually needed
   U8                       num_u8;				//R.S. - This is a counter that is used to change the state of a transer based on the number of bits sent
   union {										//R.S. - This is a union of all the different types of message bodies
      U8                    data_au8[ 8 ];		//R.S. - This buffer is used as a convenient way to address the following structures
      TagDetectCmdTSt       det_cmd_st;
      TagMaskCmdTSt         mask_cmd_st;
      TagSetClassCmdTSt     setcls_cmd_st;
      TagPingCmdTSt         ping_cmd_st;
      TagLocateCmdTSt       loc_cmd_st;
      TagIdentifyCmdTSt     identify_cmd_st;
      TagEvacCmdTSt         evac_cmd_st;

      TagDetectEvTSt        det_ev_st;
      TagRespTSt            resp_st;
   }                        u;
};

/*************************************************************************************************/
/*************************** Function Prototypes & class declarations ****************************/
/*************************************************************************************************/
//R.S. - These two references are to arrays that provide the size of the message type
//R.S. - It might have been a little easier to understand had the size been assigned dynamically based on the packet type
extern const U8 uncoded_rx_data_size_au8[]; 
extern const U8 uncoded_tx_data_size_au8[];

#define STX 2
#define ETX 3
#define TX_STREAM_MSG 1
#define SEARCH_TIMEOUT_SECONDS 7

#define SUDE_TRIS (TRISFbits.TRISF6)
#define SUDE_IO  (PORTFbits.RF6)
#define SUDE_LATCH _LATF6

#define MUDE_TRIS	(TRISBbits.TRISB10)
#define MUDE_IO		(PORTBbits.RB10)
#define MUDE_LATCH	_LATB10

#define MUTX_POWER_ON 	(_LATF6 = 1)
#define MUTX_POWER_OFF	(_LATF6 = 0)
#define MURX_POWER_ON	(_LATB10 = 1)
#define MURX_POWER_OFF	(_LATB10 = 0)

#define MURX_MODE_STANDBY 0
#define MURX_MODE_DECODE 1
#define MURX_MODE_LOCATE 2

extern void vlf_SetNewSearchSequence();
extern void vlf_SendDetectMsg(int antenna);
extern void vlf_SendMaskMsg(U32 id, int antenna);
extern void vlf_SendLocateMsg(U32 id, U8 seconds, int antenna);
extern void InitMURXStream();
extern void InitMUTXStream();
extern void ProcessMURXStream();
extern void vlf_SetMUID(U32 id);
extern void vlf_SetTXAntenna(int antenna);
extern int vld_SetMURXMode(int mode);
void clearMURXResponseCount();
unsigned int getMURXResponseCount();


#endif
