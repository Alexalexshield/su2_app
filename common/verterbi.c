
#include "system.h"
#include "verterbi.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "math.h"

//These two polynomials are used 
const unsigned char g_verterbi_polynomials[VITERBI_N] = {0x19, 0x17};
#define END_VERTERBI_STATE 0					//R.S. - this is the last state for encoding 
#define START_VERTERBI_STATE 0 					//R.S. - this is the initial state for encoding

U16 Decode( U8 prev_bit, U8 this_bit );
U8 Encode( U8 bit_u8, U8 poly_index_u8 , U8* state);
U8 Parity( U8 val_u8 );
U8 VOutputMap( U8 state_u8, U8 input_u8 );
U8 AssemblePacket( U16* traceback_buffer, U8 len, U8* out_buffer );


static U8 vit_metric_au8[ 1 << VITERBI_N ];
static U8 vit_pmetric_au8[ 1 << (VITERBI_K-1) ];
static U8 vit_tmetric_au8[ 1 << (VITERBI_K-1) ];
U8 test_buffer[MAXIMUM_DATA_LENGTH]= {0,0,2,5,4,6,77};
	
#ifdef INCLUDE_VITERBI_1
	
U8 test_verterbi()
{
	//////////////////////////// TRANSMITTER PORTION /////////////////////////////////
	
	U8 encoded_buffer[MAX_ENCODED_BUFFER_SIZE*8];	
	U8 coded_len = Verterbi_Encode(test_buffer, MAXIMUM_DATA_LENGTH, encoded_buffer);
	
	
	/////////////////////////////// RECEIVER PORTION///////////////////////
	
	////////////////////////////// Sync on preamble and get packet type/////////////////////////
	
	////////////////////////////// Read in quarter bits for packet length //////////////////////

	/////////////////////////////// simulate by converting encoded data from above into quarter bits/////////////////
	char quarterbit_encoded_buffer[MAX_ENCODED_BUFFER_SIZE*VLF_NUM_RX_BIT_SUBPERIODS*8];	
	U16 quarterbit_len;
	U16 ix;
	U8 value;
	U8 j;
	
	quarterbit_len = 0;
	for (ix = 0;ix < coded_len;ix++) {
		value = (encoded_buffer[ix])?SCORE_METRIC_FS:0;
		for (j=0;j<VLF_NUM_RX_BIT_SUBPERIODS;j++) {
			quarterbit_encoded_buffer[quarterbit_len++] = value;
		}
	}
	
	/////////////////////////// Compress quarter bits back to bits during time of read ///////////////////
	U8 bitcount_array[MAX_ENCODED_BUFFER_SIZE<<3];
	U8 bitcount_array_size = 0;
	ix = 0;
	do {
		bitcount_array[bitcount_array_size] = quarterbit_encoded_buffer[ix++];
		for (j=0;j<VLF_NUM_RX_BIT_SUBPERIODS-1;j++)
			bitcount_array[bitcount_array_size] += quarterbit_encoded_buffer[ix++];	
		bitcount_array[bitcount_array_size]>>=VITERBI_N;	
		bitcount_array_size++;		
	} while (ix < quarterbit_len);
	
	/////////////////////////// Decode array of encoded bits ///////////////////
	
	U8 decoded_buffer[MAX_ENCODED_BUFFER_SIZE];
	
	U8 decoded_len = Verterbi_Decode(bitcount_array, bitcount_array_size, decoded_buffer);
	
	U8 error=0;
	if (decoded_len != (MAXIMUM_DATA_LENGTH+1))
		error = 1;
	else {
		for (j=0;j<MAXIMUM_DATA_LENGTH;j++)
			if (test_buffer[j] != decoded_buffer[j])
				error = 1;
	}
	return error;
}

static const U8 parity_4bit_au8[] = { 0, 1, 1, 0,  1, 0, 0, 1,  1, 0, 0, 1,  0, 1, 1, 0 };

//R.S. Applies the Verterbi encoding on the bit based on the polynomial index
U8 Encode( U8 bit_u8, U8 poly_index_u8 , U8* state)
{
   poly_index_u8 = poly_index_u8 & (VITERBI_N >> 1);	// make sure the index is never bigger than the array size

   if ( poly_index_u8 == 0 ) {
      *state = (*state << 1) | (bit_u8 & 0x1);
   }

   return ( (parity_4bit_au8[ ((*state & g_verterbi_polynomials[ poly_index_u8 ]) >> 4 ) & 0xf ]^
             parity_4bit_au8[ (*state & g_verterbi_polynomials[ poly_index_u8 ]  ) & 0xf ]) & 0x1 );
}


//R.S. Calculates the parity by XORing the top and bottom nibbles and doing an AND with 1
U8 Parity( U8 val_u8 )
{
   return ( (parity_4bit_au8[ (val_u8 >> 4 ) & 0xf ]^
             parity_4bit_au8[ (val_u8      ) & 0xf ]) & 0x1 );
}


//R.S Returns 2-bit parity
U8 VOutputMap( U8 state_u8, U8 input_u8 )
{
   U8 ret_u8 = 0;
   U8 count_u8;

   state_u8 = (state_u8 << 1) | (input_u8 & 1);		//R.S. Shift new bit into previous state
   for ( count_u8 = 0; count_u8 < VITERBI_N; count_u8++ )	//R.S. Walk through bits 
      ret_u8 = (ret_u8 << 1) | Parity( state_u8 & g_verterbi_polynomials[count_u8] ); //R.S. Parity is based on state AND even or odd polynomial

   return ( ret_u8 );	// returns 2 bits
}


// This function calls the Encode function to convolutionally encode the data.
U8  Verterbi_Encode(U8* in_array, U8 in_data_len, U8* out_array)
{
	U8 value;
	U8 out_data_len = (ENCODED_SIZE(in_data_len))<< 3; 	//R.S. - Sets the number of bits for the type;
	U8 ix = 0;
	U8 input_bits[MAX_ENCODED_BUFFER_SIZE<<3];
	U8 in_count = 0;
	U8 j;
	U8 state = START_VERTERBI_STATE;
	
	//R.S. Move the input bytes into an array of bits with LSB first for each byte
	for (ix = 0; ix < in_data_len;ix++) {
		for (j=0;j<8;j++) {
			input_bits[in_count++] = (in_array[ix] & (0x01<<j))?1:0; 
		}
	}
	
    in_count = 0;  
    ix = 0;
    j = out_data_len;
    do {     
    	if ( j > CODING_TAIL_SIZE )		//R.S This is for sending the Coding Tail
        {
	        out_array[ix++] = ( Encode( input_bits[in_count], j & 0x1 , &state) );
        }
        else {
	        value = END_VERTERBI_STATE >> (((j)/VITERBI_N) - 1); //R.S. IN THIS CASE, THE ACTUAL VALUE IS ALWAYS 0
	        out_array[ix++] = ( Encode( value, j & 0x1  , &state) );
	    }
            
        if ( j & 0x1 ) {		//R.S. - If an odd bit
	        in_count++;
        }   
    }
    while (--j);
    return out_data_len;
}




// R.S. does the verterbii decoding for two consecutive bits



U8  Verterbi_Decode(U8 *in_array, U8 in_data_len, U8 *out_array)
{
	U8 ix;
	U8 count;
	U16 traceback_buffer[MAX_ENCODED_BUFFER_SIZE<<3];
	/** Reset Viterbi metric arrays*/

   	for ( ix = 0; ix < sizeof( vit_pmetric_au8 ); ix++ )
    	vit_pmetric_au8[ ix ] = (U8)(~0) - (VITERBI_N*SCORE_METRIC_FS);

   	vit_pmetric_au8[ 0 ] = START_VERTERBI_STATE;   /* Prebias the initial state */

	count = 0;
	for (ix = 0;ix < in_data_len; ix++) {
	   if ((ix & 0x01) == 0)
		   traceback_buffer[count++] = Decode( in_array[ix+0], in_array[ix+1] );
	}
	
   	U8 packetlen = AssemblePacket(traceback_buffer, count, out_array );
	return packetlen;
}


U16 Decode( U8 prev_bit, U8 this_bit )
{
	U8  ii_u8;
	U16 traceback_u16 = 0;

	/** Compute metrics associated with each input*/
   	for ( ii_u8 = 0; ii_u8 < (1 << VITERBI_N); ii_u8++ )
   	{
      	vit_metric_au8[ii_u8] = 0;
      	if ( (ii_u8 >> (VITERBI_N - 1)) & 0x1 ) 
 	     	vit_metric_au8[ii_u8] += SCORE_METRIC_FS - prev_bit;
 	    else
 	    	vit_metric_au8[ii_u8] += prev_bit; 
 	    if ( (ii_u8 >> (VITERBI_N - 2)) & 0x1 )		
	      	vit_metric_au8[ii_u8] += SCORE_METRIC_FS - this_bit;
	    else
	    	vit_metric_au8[ii_u8] += this_bit; 
   	}

	/** Find trellis connections and update metrics by looping through each butterfly ACS */
   	for ( ii_u8 = 0; ii_u8 < (1 << (VITERBI_K-2)); ii_u8++ )
      {
      if ( (vit_pmetric_au8[ ii_u8] + vit_metric_au8[ VOutputMap(ii_u8, 0 ) ]) >
           (vit_pmetric_au8[ ii_u8 + (1 << (VITERBI_K-2)) ] + vit_metric_au8[ VOutputMap( ii_u8 + (1 << (VITERBI_K-2)), 0 ) ]) )
         {
         traceback_u16 |= 0x1 << (ii_u8 << 1);
         vit_tmetric_au8[ ii_u8 << 1 ] = (vit_pmetric_au8[ ii_u8 + (1 << (VITERBI_K-2)) ] + 
                                          vit_metric_au8[ VOutputMap( ii_u8 + (1 << (VITERBI_K-2)), 0 ) ]);
         }
      else
         vit_tmetric_au8[ ii_u8 << 1 ] = (vit_pmetric_au8[ ii_u8 ] + vit_metric_au8[ VOutputMap( ii_u8, 0 ) ]);

      if ( vit_tmetric_au8[ ii_u8 << 1 ] > ((U8)(~0) - VITERBI_N*SCORE_METRIC_FS) )
         vit_tmetric_au8[ ii_u8 << 1 ] = ((U8)(~0) - VITERBI_N*SCORE_METRIC_FS);

      if ( (vit_pmetric_au8[ ii_u8                        ] + vit_metric_au8[ VOutputMap( ii_u8                       , 1 ) ]) >
           (vit_pmetric_au8[ ii_u8 + (1 << (VITERBI_K-2)) ] + vit_metric_au8[ VOutputMap( ii_u8 + (1 << (VITERBI_K-2)), 1 ) ]) )
         {
         traceback_u16 |= 0x2 << (ii_u8 << 1);
         vit_tmetric_au8[ (ii_u8 << 1) + 1 ] = (vit_pmetric_au8[ ii_u8 + (1 << (VITERBI_K-2)) ] + 
                                                vit_metric_au8[ VOutputMap( ii_u8 + (1 << (VITERBI_K-2)), 1 ) ]);
         }
      else
         vit_tmetric_au8[ (ii_u8 << 1) + 1 ] = (vit_pmetric_au8[ ii_u8 ] + vit_metric_au8[ VOutputMap( ii_u8, 1 ) ]);

      if ( vit_tmetric_au8[ (ii_u8 << 1) + 1 ] > ((U8)(~0) - VITERBI_N*SCORE_METRIC_FS) )
         vit_tmetric_au8[ (ii_u8 << 1) + 1 ] = ((U8)(~0) - VITERBI_N*SCORE_METRIC_FS);
      }

	/** Copy temporary metrics to path metrics*/
   	for ( ii_u8 = 0; ii_u8 < (1 << (VITERBI_K-1)); ii_u8++ )
    	vit_pmetric_au8[ ii_u8 ] = vit_tmetric_au8[ ii_u8 ];


   return traceback_u16;		/** Return traceback value*/
}




//R.S. The packet is assembled from the Viterbi traceback buffer 
U8 AssemblePacket( U16* traceback_buffer, U8 len, U8* out_buffer )
{
	U8  ix;
	U8 count;

   /** Traceback through trellis connections*/
   U8 vit_state_u8 = END_VERTERBI_STATE;	//R.S. The last characer (which is a constant) is used to prime the traceback

   for ( ix = len-1; ix < len; ix-- )	//R.S. work backwards from end of packet to beginning
   {
      if ( ix < (len - (VITERBI_K-1)) )	//R.S. Ignore coding tail which is 4 pairs of bits
      {
         if ( vit_state_u8 & 1 )		//R.S.  check if bit 0 is set
            out_buffer[ ix/8 ] |= (0x1 << (ix & 0x7)); //R.S. If bit set, set the corresponding bit pos
         else
            out_buffer[ ix/8 ] &= ~(0x1 << (ix & 0x7)); //R.S. If bit not set, clear the corresponding bit pos
      }

      if ( traceback_buffer[ix] & (((U16)1) << vit_state_u8) )  
         vit_state_u8 = (vit_state_u8 >> 1) | (1 << (VITERBI_K-2)); //R.S. If the bit position is set, shift bit position right and OR with 0x80
      else
         vit_state_u8 = (vit_state_u8 >> 1); //R.S. If the bit position is clear, shift bit position right
   }
   
   count = len/8;
   return count;

}
#endif // #ifdef INCLUDE_VITERBI_1

