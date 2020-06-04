#ifndef __VLF_H
#define __VLF_H


// The number of periods (cycles) in an RX bit
#define  VLF_NUM_RX_BIT_SUBPERIODS        (4)    

#ifdef MU_APP
#define VLF_TRANSMIT_FREQ 			(35714.2857)
#define VLF_TRANSMIT_BITRATE		(190.7349)
#define VLF_RECEIVE_FREQ			(8000)	
#define VLF_RECEIVE_BITRATE			(100)
#else	// SU
#define VLF_RECEIVE_FREQ 			(35714.2857)
#define VLF_RECEIVE_BITRATE			(190.7349)
#define VLF_TRANSMIT_FREQ			(8000)	
#define VLF_TRANSMIT_BITRATE		(100)
#endif

#define VLF_RX_CYCLES_PER_BIT		(VLF_RECEIVE_FREQ/VLF_RECEIVE_BITRATE)
#define VLF_TX_CYCLES_PER_BIT		(VLF_TRANSMIT_FREQ/VLF_TRANSMIT_BITRATE)


#ifdef MU_APP
//#define VLF_RX_HIGH_THRESHOLD      (VLF_RX_CYCLES_PER_BIT/VLF_NUM_RX_BIT_SUBPERIODS/2 + 1) // The threshold between a high and a low bit 
#define VLF_RX_MID_THRESHOLD      (VLF_RX_CYCLES_PER_BIT/VLF_NUM_RX_BIT_SUBPERIODS/2 -1) // The threshold between a high and a low bit 
#define VLF_RX_HIGH_THRESHOLD      ((((VLF_RX_CYCLES_PER_BIT/VLF_NUM_RX_BIT_SUBPERIODS)*3)/5) - 1) // The threshold between a high and a low bit 
#else
#define VLF_RX_MID_THRESHOLD      (VLF_RX_CYCLES_PER_BIT/VLF_NUM_RX_BIT_SUBPERIODS/2 - 1) // The threshold between a high and a low bit for a data bit
#define VLF_RX_PREAMBLE_HIGH_THRESHOLD      ((((VLF_RX_CYCLES_PER_BIT/VLF_NUM_RX_BIT_SUBPERIODS)*3)/5) - 1) // The threshold between a high and a low bit for a preamble bit
#define VLF_RX_PREAMBLE_1_BIT_THRESHOLD      35  // The threshold for a high bit for a preamble bit FROM WHICH RSSI IS MEASURED
#define VLF_RX_PREAMBLE_LOW_THRESHOLD      ((((VLF_RX_CYCLES_PER_BIT/VLF_NUM_RX_BIT_SUBPERIODS)*2)/5) - 1) // The threshold for a low bit for a preamble bit

#endif

#endif // __VLF_H

