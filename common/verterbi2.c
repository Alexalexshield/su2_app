
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
	
	U8 encoded_buffer[MAX_ENCODED_BUFFER_SIZE<<3];	
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// TODO - REMOVE THIS SECTION IF NOT NEEDED /////////////////////////////
#ifdef INCLUDE_VITERBI_2
#undef SLOWACS
#define FASTACS
#undef NORM
#define MAXMETRIC 128
 
void deci2bin(int d, int size, int *b);
int bin2deci(int *b, int size);
int nxt_stat(int current_state, int input, int *memory_contents);
void init_quantizer(void);
void init_adaptive_quant(float es_ovr_n0);
int soft_quant(float channel_symbol);
int soft_metric(int data, int guess);


void cnv_encd(int g[2][K], long input_len, int *in_array, int *out_array) {

    int m;                     /* K - 1 */
    long t, tt;                /* bit time, symbol time */
    int j, k;                  /* loop variables */
    int *unencoded_data;       /* pointer to data array */
    int shift_reg[K];          /* the encoder shift register */
    int sr_head;               /* index to the first elt in the sr */
    int p, q;                  /* the upper and lower xor gate outputs */

    m = K - 1;
 
    /* allocate space for the zero-padded input data array */
    unencoded_data = malloc( (input_len + m) * sizeof(int) );
    if (unencoded_data == NULL) {
        printf("\ncnv_encd.c: Can't allocate enough memory for unencoded data!  Aborting...");
        exit(1);
    }

    /* read in the data and store it in the array */
    for (t = 0; t < input_len; t++)
        unencoded_data[t] = in_array[t];

    /* zero-pad the end of the data */
    for (t = 0; t < m; t++) {
        unencoded_data[t + input_len] = 0;
    }
 
    /* Initialize the shift register */
    for (j = 0; j < K; j++) {
        shift_reg[j] = 0;
    }
 
    /* To try to speed things up a little, the shift register will be operated
       as a circular buffer, so it needs at least a head pointer. It doesn't
       need a tail pointer, though, since we won't be taking anything out of
       it--we'll just be overwriting the oldest entry with the new data. */
    sr_head = 0;

    /* initialize the channel symbol output index */
    tt = 0;

    /* Now start the encoding process */
    /* compute the upper and lower mod-two adder outputs, one bit at a time */
    for (t = 0; t < input_len + m; t++) {
        shift_reg[sr_head] = unencoded_data[t];
        p = 0;
        q = 0;
        for (j = 0; j < K; j++) {
            k = (j + sr_head) % VITERBI_K;
            p ^= shift_reg[k] & g[0][j];
            q ^= shift_reg[k] & g[1][j];
        }

        /* write the upper and lower xor gate outputs as channel symbols */
        out_array[tt++] = p;
        out_array[tt++] = q;

        sr_head -= 1;    /* equivalent to shifting everything right one place */
        if (sr_head < 0) /* but make sure we adjust pointer modulo K */
            sr_head = m;

    }
 
    /* free the dynamically allocated array */
    free(unencoded_data);
 
}

 
void sdvd(int g[2][K], float es_ovr_n0, long channel_length,
            float *channel_output_vector, int *decoder_output_matrix) {
 
    int i, j, l, ll;                          /* loop variables */
    long t;                                   /* time */
    int memory_contents[K];                   /* input + conv. encoder sr */
    int input[TWOTOTHEM][TWOTOTHEM];          /* maps current/nxt sts to input */
    int output[TWOTOTHEM][2];                 /* gives conv. encoder output */
    int nextstate[TWOTOTHEM][2];              /* for current st, gives nxt given input */
    int accum_err_metric[TWOTOTHEM][2];       /* accumulated error metrics */
    int state_history[TWOTOTHEM][VITERBI_K * 5 + 1];  /* state history table */
    int state_sequence[K * 5 + 1];            /* state sequence list */
 
    int *channel_output_matrix;               /* ptr to input matrix */

    int binary_output[2];                     /* vector to store binary enc output */
    int branch_output[2];                     /* vector to store trial enc output */
 
    int m, n, number_of_states, depth_of_trellis, step, branch_metric,
        sh_ptr, sh_col, x, xx, h, hh, next_state, last_stop; /* misc variables */
 
/* ************************************************************************** */
 
    /* n is 2^1 = 2 for rate 1/2 */
    n = 2;
 
    /* m (memory length) = K - 1 */
    m = K - 1;

    /* number of states = 2^(K - 1) = 2^m for k = 1 */
    number_of_states = (int) pow(2, m);

    /* little degradation in performance achieved by limiting trellis depth
       to K * 5--interesting to experiment with smaller values and measure
       the resulting degradation. */
    depth_of_trellis = K * 5;

    /* initialize data structures */
    for (i = 0; i < number_of_states; i++) {
        for (j = 0; j < number_of_states; j++)
            input[i][j] = 0;

        for (j = 0; j < n; j++) {
            nextstate[i][j] = 0;
            output[i][j] = 0;
        }

        for (j = 0; j <= depth_of_trellis; j++) {
            state_history[i][j] = 0;
        }

        /* initial accum_error_metric[x][0] = zero */
        accum_err_metric[i][0] = 0;
        /* by setting accum_error_metric[x][1] to MAXINT, we don't need a flag */
        /* so I don't get any more questions about this:  */
        /* MAXINT is simply the largest possible integer, defined in values.h */
        accum_err_metric[i][1] = MAXINT;

    }

    /* generate the state transition matrix, output matrix, and input matrix
       - input matrix shows how FEC encoder bits lead to next state
       - next_state matrix shows next state given current state and input bit
       - output matrix shows FEC encoder output bits given current presumed
       encoder state and encoder input bit--this will be compared to actual
       received symbols to determine metric for corresponding branch of trellis
    */

    for (j = 0; j < number_of_states; j++) {
        for (l = 0; l < n; l++) {
            next_state = nxt_stat(j, l, memory_contents);
            input[j][next_state] = l;

            /* now compute the convolutional encoder output given the current
               state number and the input value */
            branch_output[0] = 0;
            branch_output[1] = 0;

            for (i = 0; i < K; i++) {
                branch_output[0] ^= memory_contents[i] & g[0][i];
                branch_output[1] ^= memory_contents[i] & g[1][i];
            }

            /* next state, given current state and input */
            nextstate[j][l] = next_state;
            /* output in decimal, given current state and input */
            output[j][l] = bin2deci(branch_output, 2);

        } /* end of l for loop */

    } /* end of j for loop */

    #ifdef DEBUG
    printf("\nInput:");
    for (j = 0; j < number_of_states; j++) {
        printf("\n");
        for (l = 0; l < number_of_states; l++)
            printf("%2d ", input[j][l]);
    } /* end j for-loop */

    printf("\nOutput:");
    for (j = 0; j < number_of_states; j++) {
        printf("\n");
        for (l = 0; l < n; l++)
            printf("%2d ", output[j][l]);
    } /* end j for-loop */

    printf("\nNext State:");
    for (j = 0; j < number_of_states; j++) {
        printf("\n");
        for (l = 0; l < n; l++)
            printf("%2d ", nextstate[j][l]);
    } /* end j for-loop */
    #endif

    channel_output_matrix = malloc( channel_length * sizeof(int) );
    if (channel_output_matrix == NULL) {
        printf(
        "\nsdvd.c: Can't allocate memory for channel_output_matrix! Aborting...");
        exit(1);
    }

    /* now we're going to rearrange the channel output so it has n rows,
       and n/2 columns where each row corresponds to a channel symbol for
       a given bit and each column corresponds to an encoded bit */
    channel_length = channel_length / n;

    /* interesting to compare performance of fixed vs adaptive quantizer */
    /* init_quantizer(); */
    init_adaptive_quant(es_ovr_n0);

    /* quantize the channel output--convert float to short integer */
    /* channel_output_matrix = reshape(channel_output, n, channel_length) */
    for (t = 0; t < (channel_length * n); t += n) {
        for (i = 0; i < n; i++)
            *(channel_output_matrix + (t / n) + (i * channel_length) ) =
                soft_quant( *(channel_output_vector + (t + i) ) );
    } /* end t for-loop */
 
/* ************************************************************************** */
 
    /* End of setup. Start decoding of channel outputs with forward
        traversal of trellis!  Stop just before encoder-flushing bits.  */
    for (t = 0; t < channel_length - m; t++) {

       if (t <= m)
           /* assume starting with zeroes, so just compute paths from all-zeroes state */
           step = pow(2, m - t * 1);
        else
            step = 1;

        /* we're going to use the state history array as a circular buffer so
           we don't have to shift the whole thing left after each bit is
           processed so that means we need an appropriate pointer */
        /* set up the state history array pointer for this time t */
        sh_ptr = (int) ( ( t + 1 ) % (depth_of_trellis + 1) );

        /* repeat for each possible state */
        for (j = 0; j < number_of_states; j+= step) {
            /* repeat for each possible convolutional encoder output n-tuple */
            for (l = 0; l < n; l++) {
                branch_metric = 0;

                /* compute branch metric per channel symbol, and sum for all
                    channel symbols in the convolutional encoder output n-tuple */

                #ifdef SLOWACS
                /* convert the decimal representation of the encoder output to binary */
                deci2bin(output[j][l], n, binary_output);

                /* compute branch metric per channel symbol, and sum for all
                    channel symbols in the convolutional encoder output n-tuple */
                for (ll = 0; ll < n; ll++) {
                    branch_metric = branch_metric + soft_metric( *(channel_output_matrix +
                    ( ll * channel_length + t )), binary_output[ll] );

                } /* end of 'll' for loop */
                #endif
                
                #ifdef FASTACS
                /* this only works for n = 2, but it's fast! */

                /* convert the decimal representation of the encoder output to binary */
                binary_output[0] = ( output[j][l] & 0x00000002 ) >> 1;
                binary_output[1] = output[j][l] & 0x00000001;

                /* compute branch metric per channel symbol, and sum for all
                    channel symbols in the convolutional encoder output n-tuple */
                branch_metric = branch_metric + abs( *( channel_output_matrix +
                    ( 0 * channel_length + t ) ) - 7 * binary_output[0] ) +
                                                abs( *( channel_output_matrix +
                    ( 1 * channel_length + t ) ) - 7 * binary_output[1] );
                #endif

                /* now choose the surviving path--the one with the smaller accumlated
                    error metric... */
                if ( accum_err_metric[ nextstate[j][l] ] [1] > accum_err_metric[j][0] +
                    branch_metric ) {

                    /* save an accumulated metric value for the survivor state */
                    accum_err_metric[ nextstate[j][l] ] [1] = accum_err_metric[j][0] +
                        branch_metric;
 
                    /* update the state_history array with the state number of
                       the survivor */
                    state_history[ nextstate[j][l] ] [sh_ptr] = j;
 
                } /* end of if-statement */
            } /* end of 'l' for-loop */
        } /* end of 'j' for-loop -- we have now updated the trellis */
 
        /* for all rows of accum_err_metric, move col 2 to col 1 and flag col 2 */
        for (j = 0; j < number_of_states; j++) {
            accum_err_metric[j][0] = accum_err_metric[j][1];
            accum_err_metric[j][1] = MAXINT;
        } /* end of 'j' for-loop */
 
 
        /* now start the traceback, if we've filled the trellis */
        if (t >= depth_of_trellis - 1) {
 
            /* initialize the state_sequence vector--probably unnecessary */
            for (j = 0; j <= depth_of_trellis; j++)
                state_sequence[j] = 0;
 
            /* find the element of state_history with the min. accum. error metric */
            /* since the outer states are reached by relatively-improbable runs
               of zeroes or ones, search from the top and bottom of the trellis in */
            x = MAXINT;

            for (j = 0; j < ( number_of_states / 2 ); j++) {

                if ( accum_err_metric[j][0] < accum_err_metric[number_of_states - 1 - j][0] ) {
                    xx = accum_err_metric[j][0];
                    hh = j;
                }
                else {
                    xx = accum_err_metric[number_of_states - 1 - j][0];
                    hh = number_of_states - 1 - j;
                }
                if ( xx < x) {
                    x = xx;
                    h = hh;
                }

            } /* end 'j' for-loop */
 
            #ifdef NORM
            /* interesting to experiment with different numbers of bits in the
               accumulated error metric--does performance decrease with fewer  bits? */
            /* if the smallest accum. error metric value is > MAXMETRIC, normalize the
               accum. errror metrics by subtracting the value of the smallest one from
               all of them (making the smallest = 0) and saturate all other metrics
               at MAXMETRIC */
            if (x > MAXMETRIC) {
                for (j = 0; j < number_of_states; j++) {
                    accum_err_metric[j][0] = accum_err_metric[j][0] - x;
                    if (accum_err_metric[j][0] > MAXMETRIC)
                        accum_err_metric[j][0] = MAXMETRIC;
                } /* end 'j' for-loop */
            }
            #endif
 
            /* now pick the starting point for traceback */
            state_sequence[depth_of_trellis] = h;
 
            /* now work backwards from the end of the trellis to the oldest state
               in the trellis to determine the optimal path. The purpose of this
               is to determine the most likely state sequence at the encoder
                based on what channel symbols we received. */
            for (j = depth_of_trellis; j > 0; j--) {
                sh_col = j + ( sh_ptr - depth_of_trellis );
                if (sh_col < 0)
                   sh_col = sh_col + depth_of_trellis + 1;

                state_sequence[j - 1] = state_history[ state_sequence[j] ] [sh_col];
            } /* end of j for-loop */

            /* now figure out what input sequence corresponds to the state sequence
             in the optimal path */
            *(decoder_output_matrix + t - depth_of_trellis + 1) =
                input[ state_sequence[0] ] [ state_sequence[1] ];

       } /* end of if-statement */
        
    } /* end of 't' for-loop */
 
/* ************************************************************************** */
 
    /* now decode the encoder flushing channel-output bits */
    for (t = channel_length - m; t < channel_length; t++) {

        /* set up the state history array pointer for this time t */
        sh_ptr = (int) ( ( t + 1 ) % (depth_of_trellis + 1) );

        /* don't need to consider states where input was a 1, so determine
         what is the highest possible state number where input was 0 */
        last_stop = number_of_states / pow(2, t - channel_length + m);

        /* repeat for each possible state */
        for (j = 0; j < last_stop; j++) {
 
            branch_metric = 0;
            deci2bin(output[j][0], n, binary_output);
 
            /* compute metric per channel bit, and sum for all channel bits
                in the convolutional encoder output n-tuple */
            for (ll = 0; ll < n; ll++) {
                branch_metric = branch_metric + soft_metric( *(channel_output_matrix +
                (ll * channel_length + t)), binary_output[ll] );
            } /* end of 'll' for loop */

            /* now choose the surviving path--the one with the smaller total
                metric... */
            if ( (accum_err_metric[ nextstate[j][0] ][1] > accum_err_metric[j][0] +
                branch_metric) /*|| flag[ nextstate[j][0] ] == 0*/) {
 
                /* save a state metric value for the survivor state */
                accum_err_metric[ nextstate[j][0] ][1] = accum_err_metric[j][0] +
                    branch_metric;
 
                /* update the state_history array with the state number of
                    the survivor */
                state_history[ nextstate[j][0] ][sh_ptr] = j;
 
            } /* end of if-statement */
 
        } /* end of 'j' for-loop */
 
        /* for all rows of accum_err_metric, swap columns 1 and 2 */
        for (j = 0; j < number_of_states; j++) {
            accum_err_metric[j][0] = accum_err_metric[j][1];
            accum_err_metric[j][1] = MAXINT;
        } /* end of 'j' for-loop */
 
        /* now start the traceback, if i >= depth_of_trellis - 1*/
        if (t >= depth_of_trellis - 1) {
 
            /* initialize the state_sequence vector */
            for (j = 0; j <= depth_of_trellis; j++) state_sequence[j] = 0;
 
            /* find the state_history element with the minimum accum. error metric */
            x = accum_err_metric[0][0];
            h = 0;
            for (j = 1; j < last_stop; j++) {
                if (accum_err_metric[j][0] < x) {
                    x = accum_err_metric[j][0];
                    h = j;
                } /* end if */
            } /* end 'j' for-loop */
 
            #ifdef NORM
            /* if the smallest accum. error metric value is > MAXMETRIC, normalize the
               accum. errror metrics by subtracting the value of the smallest one from
               all of them (making the smallest = 0) and saturate all other metrics
               at MAXMETRIC */
            if (x > MAXMETRIC) {
                for (j = 0; j < number_of_states; j++) {
                    accum_err_metric[j][0] = accum_err_metric[j][0] - x;
                    if (accum_err_metric[j][0] > MAXMETRIC) {
                     accum_err_metric[j][0] = MAXMETRIC;
                    } /* end if */
                } /* end 'j' for-loop */
            }
            #endif
 
            state_sequence[depth_of_trellis] = h;
 
            /* now work backwards from the end of the trellis to the oldest state
                in the trellis to determine the optimal path. The purpose of this
                is to determine the most likely state sequence at the encoder
                based on what channel symbols we received. */
            for (j = depth_of_trellis; j > 0; j--) {

                sh_col = j + ( sh_ptr - depth_of_trellis );
                if (sh_col < 0)
                   sh_col = sh_col + depth_of_trellis + 1;

                state_sequence[j - 1] = state_history[ state_sequence[j] ][sh_col];
            } /* end of j for-loop */

            /* now figure out what input sequence corresponds to the
                optimal path */
 
             *(decoder_output_matrix + t - depth_of_trellis + 1) =
                input[ state_sequence[0] ][ state_sequence[1] ];

       } /* end of if-statement */
    } /* end of 't' for-loop */
 
    for (i = 1; i < depth_of_trellis - m; i++)
        *(decoder_output_matrix + channel_length - depth_of_trellis + i) =
            input[ state_sequence[i] ] [ state_sequence[i + 1] ];

    /* free the dynamically allocated array storage area */
    free(channel_output_matrix);
 
    return;
} /* end of function sdvd */


void testsdvd(void) {
 
    long iter, t, msg_length, channel_length; /* loop variables, length of I/O files */

    int *onezer;
    int *encoded;                    /* original, encoded, & decoded data arrays */
    int *sdvdout;

    int start;

    float *splusn;                   /* noisy data array */

    int i_rxdata, m;                 /* int rx data , m = K - 1*/
    float es_ovr_n0, number_errors_encoded, number_errors_unencoded,
            e_threshold, ue_threshold, e_ber, ue_ber; /* various statistics */

    #if K == 3        /* polynomials for K = 3 */
    int g[2][K] = {{1, 1, 1},     /* 7 */
                   {1, 0, 1}};    /* 5 */
    #endif

    #if K == 5        /* polynomials for K = 5 */
    int g[2][K] = {{1, 1,  1, 0, 1},  /* 35 */
                   {1, 0,  0, 1, 1}}; /* 23 */
    #endif
 
    #if K == 7        /* polynomials for K = 7 */
    int g[2][K] = {{1,  1, 1, 1,  0, 0, 1},  /* 171 */
                   {1,  0, 1, 1,  0, 1, 1}}; /* 133 */
    #endif

    #if K == 9        /* polynomials for K = 9 */
    int g[2][K] = {{1, 1, 1,  1, 0, 1,  0, 1, 1}, /* 753 */
                   {1, 0, 1,  1, 1, 0,  0, 0, 1}}; /* 561 */
    #endif
 
    printf("\nK = %d", K);

    #if K == 3
    printf("\ng1 = %d%d%d", g[0][0], g[0][1], g[0][2] );
    printf("\ng2 = %d%d%d\n", g[1][0], g[1][1], g[1][2] );
    #endif

    #if K == 5
    printf("\ng1 = %d%d %d%d%d", g[0][0], g[0][1], g[0][2], g[0][3], g[0][4] );
    printf("\ng2 = %d%d %d%d%d\n", g[1][0], g[1][1], g[1][2], g[1][3], g[1][4] );
    #endif

    #if K == 7
    printf("\ng1 = %d %d%d%d %d%d%d", g[0][0], g[0][1], g[0][2], g[0][3], g[0][4],
                     g[0][5], g[0][6] );
    printf("\ng2 = %d %d%d%d %d%d%d\n", g[1][0], g[1][1], g[1][2], g[1][3], g[1][4],
                     g[1][5], g[1][6] );
    #endif

    #if K == 9
    printf("\ng1 = %d%d%d %d%d%d %d%d%d", g[0][0], g[0][1], g[0][2], g[0][3], g[0][4],
                     g[0][5], g[0][6], g[0][7], g[0][8] );
    printf("\ng2 = %d%d%d %d%d%d %d%d%d\n", g[1][0], g[1][1], g[1][2], g[1][3], g[1][4],
                     g[1][5], g[1][6], g[1][7], g[1][8] );
    #endif

    m = K - 1;
    msg_length = MSG_LEN;
    channel_length = ( msg_length + m ) * 2;

    onezer = malloc( msg_length * sizeof( int ) );
    if (onezer == NULL) {
        printf("\n testsdvd.c:  error allocating onezer array, aborting!");
        exit(1);
    }

    encoded = malloc( channel_length * sizeof(int) );
    if (encoded == NULL) {
        printf("\n testsdvd.c:  error allocating encoded array, aborting!");
        exit(1);
    }

    splusn = malloc( channel_length * sizeof(float) );
    if (splusn == NULL) {
        printf("\n testsdvd.c:  error allocating splusn array, aborting!");
        exit(1);
    }

    sdvdout = malloc( msg_length * sizeof( int ) );
    if (sdvdout == NULL) {
        printf("\n testsdvd.c:  error allocating sdvdout array, aborting!");
        exit(1);
    }


    for (es_ovr_n0 = LOESN0; es_ovr_n0 <= HIESN0; es_ovr_n0 += ESN0STEP) {

        start = time(NULL);
        
        number_errors_encoded = 0.0;
        e_ber = 0.0;
        iter = 0;

        #ifdef DOENC
        if (es_ovr_n0 <= 9)
            e_threshold = 100; /* +/- 20% */
        else
            e_threshold = 20; /* +/- 100 % */
 
        while (number_errors_encoded < e_threshold) {
            iter += 1;
 
            /*printf("Generating one-zero data\n");*/
            gen01dat(msg_length, onezer);
 
            /*printf("Convolutionally encoding the data\n");*/
            cnv_encd(g, msg_length, onezer, encoded);
 
            /*printf("Adding noise to the encoded data\n");*/
            addnoise(es_ovr_n0, channel_length, encoded, splusn);
 
 
            /*printf("Decoding the BSC data\n");*/
            sdvd(g, es_ovr_n0, channel_length, splusn, sdvdout);

            for (t = 0; t < msg_length; t++) {
                if ( *(onezer + t) != *(sdvdout + t) ) {
                   /*printf("\n error occurred at location %ld", t);*/
                       number_errors_encoded += 1;
                } /* end if */
            } /* end t for-loop */
 
        }

        e_ber = number_errors_encoded / (msg_length * iter);

//        printf("\nThe elapsed time was %d seconds for %d iterations",time(NULL) - start, iter);
        #endif
 
        number_errors_unencoded = 0.0;
        ue_ber = 0.0;
        iter = 0;

        #ifdef DONOENC
        if (es_ovr_n0 <= 12)
            ue_threshold = 100;
        else
            ue_threshold = 20;
 
 
        while (number_errors_unencoded < ue_threshold) {
            iter += 1;
 
            /*printf("Generating one-zero data\n");*/
            gen01dat(msg_length, onezer);
 
            /*printf("Adding noise to the unencoded data\n");*/
            addnoise(es_ovr_n0, msg_length, onezer, splusn);
 
            for (t = 0; t < msg_length; t++) {

                if ( *(splusn + t) < 0.0 )
                    i_rxdata = 1;
                else
                    i_rxdata = 0;
 
                if ( *(onezer + t) != i_rxdata )
                    number_errors_unencoded += 1;
            }
 
            if (kbhit()) exit(0);
            /*printf("\nDone!");*/

        }

        ue_ber = number_errors_unencoded / (msg_length * iter);
        #endif

//        printf("\nAt %1.1fdB Es/No, ", es_ovr_n0);

        #ifdef DOENC
//        printf("the e_ber was %1.1e ", e_ber);
        #ifdef DONOENC
        printf("and ");
        #endif
        #endif

        #ifdef DONOENC
        printf("the ue_ber was %1.1e", ue_ber);
        #endif


    }

    free(onezer);
    free(encoded);
    free(splusn);
    free(sdvdout);

    while ( !kbhit() ) {
    }

    exit(0);
}

int quantizer_table[256];

void init_quantizer(void) {

    int i;
    for (i = -128; i < -31; i++)
        quantizer_table[i + 128] = 7;
    for (i = -31; i < -21; i++)
        quantizer_table[i + 128] = 6;
    for (i = -21; i < -11; i++)
        quantizer_table[i + 128] = 5;
    for (i = -11; i < 0; i++)
        quantizer_table[i + 128] = 4;
    for (i = 0; i < 11; i++)
        quantizer_table[i + 128] = 3;
    for (i = 11; i < 21; i++)
        quantizer_table[i + 128] = 2;
    for (i = 21; i < 31; i++)
        quantizer_table[i + 128] = 1;
    for (i = 31; i < 128; i++)
        quantizer_table[i + 128] = 0;
}


/* this initializes a quantizer that adapts to Es/No */
void init_adaptive_quant(float es_ovr_n0) {

    int i, d;
    float es, sn_ratio, sigma;

    es = 1;
    sn_ratio = (float) pow(10.0, ( es_ovr_n0 / 10.0 ) );
    sigma =  (float) sqrt( es / ( 2.0 * sn_ratio ) );

    d = (int) ( 32 * 0.5 * sigma );

    for (i = -128; i < ( -3 * d ); i++)
        quantizer_table[i + 128] = 7;

    for (i = ( -3 * d ); i < ( -2 * d ); i++)
        quantizer_table[i + 128] = 6;

    for (i = ( -2 * d ); i < ( -1 * d ); i++)
        quantizer_table[i + 128] = 5;

    for (i = ( -1 * d ); i < 0; i++)
        quantizer_table[i + 128] = 4;

    for (i = 0; i < ( 1 * d ); i++)
        quantizer_table[i + 128] = 3;

    for (i = ( 1 * d ); i < ( 2 * d ); i++)
        quantizer_table[i + 128] = 2;

    for (i = ( 2 * d ); i < ( 3 * d ); i++)
        quantizer_table[i + 128] = 1;

    for (i = ( 3 * d ); i < 128; i++)
        quantizer_table[i + 128] = 0;
}


/* this quantizer assumes that the mean channel_symbol value is +/- 1,
   and translates it to an integer whose mean value is +/- 32 to address
   the lookup table "quantizer_table". Overflow protection is included.
*/
 int soft_quant(float channel_symbol) {
    int x;
 
    x = (int) ( 32.0 * channel_symbol );
    if (x < -128) x = -128;
    if (x > 127) x = 127;
 
    return(quantizer_table[x + 128]);
}


/* this metric is based on the algorithm given in Michelson and Levesque,
   page 323. */
int soft_metric(int data, int guess) {

    return(abs(data - (guess * 7)));
}


/* this function calculates the next state of the convolutional encoder, given
   the current state and the input data.  It also calculates the memory
   contents of the convolutional encoder. */
int nxt_stat(int current_state, int input, int *memory_contents) {
 
    int binary_state[K - 1];              /* binary value of current state */
    int next_state_binary[K - 1];         /* binary value of next state */
    int next_state;                       /* decimal value of next state */
    int i;                                /* loop variable */

    /* convert the decimal value of the current state number to binary */
    deci2bin(current_state, K - 1, binary_state);

    /* given the input and current state number, compute the next state number */
    next_state_binary[0] = input;
    for (i = 1; i < K - 1; i++)
        next_state_binary[i] = binary_state[i - 1];
 
    /* convert the binary value of the next state number to decimal */
    next_state = bin2deci(next_state_binary, K - 1);

    /* memory_contents are the inputs to the modulo-two adders in the encoder */
    memory_contents[0] = input;
    for (i = 1; i < K; i++)
        memory_contents[i] = binary_state[i - 1];
 
    return(next_state);
}


/* this function converts a decimal number to a binary number, stored
   as a vector MSB first, having a specified number of bits with leading
   zeroes as necessary */
void deci2bin(int d, int size, int *b) {
    int i;
 
    for(i = 0; i < size; i++)
        b[i] = 0;
 
    b[size - 1] = d & 0x01;
 
    for (i = size - 2; i >= 0; i--) {
        d = d >> 1;
        b[i] = d & 0x01;
    }
}


/* this function converts a binary number having a specified
   number of bits to the corresponding decimal number
   with improvement contributed by Bryan Ewbank 2001.11.28 */
int bin2deci(int *b, int size) {
    int i, d;
 
    d = 0;
 
    for (i = 0; i < size; i++)
        d += b[i] << (size - i - 1);
 
    return(d);
}

 
void gen01dat( long data_len, int *out_array ) {
 
    long t;            /* time */

    /* re-seed the random number generator */
    randomize();

    /* generate the random data and write it to the output array */
    for (t = 0; t < data_len; t++)
        *( out_array + t ) = (int)( rand() / (RAND_MAX / 2) > 0.5 );

}

float gngauss(float mean, float sigma);
 
void addnoise(float es_ovr_n0, long channel_len, int *in_array, float *out_array) {

    long t;
    float mean, es, sn_ratio, sigma, signal;
 
    /* given the desired Es/No (for BPSK, = Eb/No - 3 dB), calculate the
    standard deviation of the additive white gaussian noise (AWGN). The
    standard deviation of the AWGN will be used to generate Gaussian random
    variables simulating the noise that is added to the signal. */

    mean = 0;
    es = 1;
    sn_ratio = (float) pow(10, ( es_ovr_n0 / 10) );
    sigma =  (float) sqrt (es / ( 2 * sn_ratio ) );
    
    /* now transform the data from 0/1 to +1/-1 and add noise */
    for (t = 0; t < channel_len; t++) {

        /*if the binary data value is 1, the channel symbol is -1; if the
        binary data value is 0, the channel symbol is +1. */
        signal = 1 - 2 * *( in_array + t );
 
        /*  now generate the gaussian noise point, add it to the channel symbol,
            and output the noisy channel symbol */

        *( out_array + t ) = signal + gngauss(mean, sigma);
    }

}

float gngauss(float mean, float sigma) {

    /* This uses the fact that a Rayleigh-distributed random variable R, with
    the probability distribution F(R) = 0 if R < 0 and F(R) =
    1 - exp(-R^2/2*sigma^2) if R >= 0, is related to a pair of Gaussian
    variables C and D through the transformation C = R * cos(theta) and
    D = R * sin(theta), where theta is a uniformly distributed variable
    in the interval (0, 2*pi()). From Contemporary Communication Systems
    USING MATLAB(R), by John G. Proakis and Masoud Salehi, published by
    PWS Publishing Company, 1998, pp 49-50. This is a pretty good book. */
 
    double u, r;            /* uniform and Rayleigh random variables */
 
    /* generate a uniformly distributed random number u between 0 and 1 - 1E-6*/
    u = (double)_lrand() / LRAND_MAX;
    if (u == 1.0) u = 0.999999999;
 
    /* generate a Rayleigh-distributed random number r using u */
    r = sigma * sqrt( 2.0 * log( 1.0 / (1.0 - u) ) );
 
    /* generate another uniformly-distributed random number u as before*/
    u = (double)_lrand() / LRAND_MAX;
    if (u == 1.0) u = 0.999999999;
 
    /* generate and return a Gaussian-distributed random number using r and u */
    return( (float) ( mean + r * cos(2 * PI * u) ) );
}

#endif //#ifdef INCLUDE_VITERBI_2
