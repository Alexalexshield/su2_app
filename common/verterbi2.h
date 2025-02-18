#ifndef _VERTERBI_H
#define _VERTERBI_H
#include "vlf.h"

#define INCLUDE_VITERBI_1

// Convolutional encoding and Viterbi decoding constants

#define MAXIMUM_DATA_LENGTH			7		// MAXIUMUM DATA SIZE TO ENCODE
#define VITERBI_N					2                       // The rate of the convolutional code used 
#define VITERBI_K					5                       // The constraint length of the code 
#define CODING_TAIL_SIZE			((VITERBI_K-1)*VITERBI_N) // The length of the coding tail - 8 bits in this case
#define PCKT_PREAMBLE_SIZE			12                      // The size in bits of the packet preamble 
#define ENCODED_SIZE( len )			((U8)((len + 1) * 2 + 1))
#define MAX_ENCODED_BUFFER_SIZE		ENCODED_SIZE(MAXIMUM_DATA_LENGTH)
#define SCORE_METRIC_SHIFT			3			// The viterbi metric score shift value 
#define SCORE_METRIC_DIV			8			// The viterbi metric score shift value 

#define  SCORE_METRIC_FS            (VLF_RX_CYCLES_PER_BIT/SCORE_METRIC_DIV) // A full scale metric 

#define  SCORE_METRIC_LOW_THRESH     (SCORE_METRIC_FS*4/5)   // The maximum score metric for a low bit 
#define  SCORE_METRIC_HIGH_THRESH    (SCORE_METRIC_FS*2/5)   // The minimum score metric for a high bit 
#define  SCORE_METRIC_MID_THRESH     (SCORE_METRIC_FS*3/5)   // Somewhere in between 



#ifdef INCLUDE_VITERBI_2
// definitions for cnv_encd and sdvd
#define MAXINT 0x7FFF
#define K 3              /* constraint length */
#define TWOTOTHEM 4      /* 2^(K - 1) -- change as required */
#define PI 3.141592654   /* circumference of circle divided by diameter */

#define MSG_LEN 100000l  /* how many bits in each test message */
#define DOENC            /* test with convolutional encoding/Viterbi decoding */
#undef  DONOENC          /* test with no coding */
#define LOESN0 0.0       /* minimum Es/No at which to test */
#define HIESN0 3.5       /* maximum Es/No at which to test */
#define ESN0STEP 0.5     /* Es/No increment for test driver */

#endif

///////////////////////////////////////////PROTOTYPES////////////////////
U8 test_verterbi();
U8  Verterbi_Decode(U8* in_array, U8 in_data_len, U8* out_array);
U8  Verterbi_Encode(U8* in_array, U8 in_data_len, U8* out_array);


#endif //_VERTERBI_H

