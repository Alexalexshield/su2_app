#include "system.h"
#include "vlf_pkt.h"
#include "verterbi.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "trace.h"


extern U8 g_protocol;	// this holds the last protocol 
#ifdef ENABLE_ZC_THRESHOLD_TESTING
char vlf_high_threshold = VLF_RX_PREAMBLE_HIGH_THRESHOLD;
char vlf_low_threshold = VLF_RX_PREAMBLE_LOW_THRESHOLD;
#endif

//R.S. - Pack the preamble ( 0x5A5) into a 12-byte array - this makes it easier to walk through the bits than shifting
const U8 open_flag_au8[ PCKT_PREAMBLE_SIZE ] = {
   (OPEN_FLAG) & 0x1,
   (OPEN_FLAG >> 1) & 0x1,
   (OPEN_FLAG >> 2) & 0x1,
   (OPEN_FLAG >> 3) & 0x1,
   (OPEN_FLAG >> 4) & 0x1,
   (OPEN_FLAG >> 5) & 0x1,
   (OPEN_FLAG >> 6) & 0x1,
   (OPEN_FLAG >> 7) & 0x1,
   (OPEN_FLAG >> 8) & 0x1,
   (OPEN_FLAG >> 9) & 0x1,
   (OPEN_FLAG >> 10) & 0x1,
   (OPEN_FLAG >> 11) & 0x1 
};

//R.S. - Pack the location signature command ( 0x6B) into a 7-byte array - this makes it easier to walk through the bits than shifting
const U8 loc_sig_au8[ LOC_SIGNTAURE_SIZE ] = {
   (LOC_SIGNATURE) & 0x1,
   (LOC_SIGNATURE >> 1) & 0x1,
   (LOC_SIGNATURE >> 2) & 0x1,
   (LOC_SIGNATURE >> 3) & 0x1,
   (LOC_SIGNATURE >> 4) & 0x1,
   (LOC_SIGNATURE >> 5) & 0x1,
   (LOC_SIGNATURE >> 6) & 0x1 
};

const U8 open_flag_au8Bits[ PCKT_PREAMBLE_BIT_COUNT ] = {
   ((OPEN_FLAG) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,    
   ((OPEN_FLAG) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 1) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 1) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 1) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 1) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 2) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 2) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 2) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 2) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 3) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 3) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 3) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 3) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 4) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 4) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 4) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 4) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 5) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 5) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 5) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 5) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 6) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 6) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 6) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 6) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 7) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 7) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 7) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 7) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 8) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 8) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 8) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 8) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 9) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 9) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 9) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   ((OPEN_FLAG >> 9) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
   
  ((OPEN_FLAG >> 10) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
  ((OPEN_FLAG >> 10) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
  ((OPEN_FLAG >> 10) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,
  ((OPEN_FLAG >> 10) & 0x1) * VLF_RX_CYCLES_PER_BIT/4,

   ((OPEN_FLAG >> 11) & 0x1) * VLF_RX_CYCLES_PER_BIT/4, 
   ((OPEN_FLAG >> 11) & 0x1) * VLF_RX_CYCLES_PER_BIT/4, 
   ((OPEN_FLAG >> 11) & 0x1) * VLF_RX_CYCLES_PER_BIT/4, 
   ((OPEN_FLAG >> 11) & 0x1) * VLF_RX_CYCLES_PER_BIT/4 
   
};

//////////////////////////////////////////////// PROTOCOL 1 - OLD PROTOCOL ////////////////////////////

//R.S. - this is the data size for different types of packets that are sent by the SU to the MU
const U8 uncoded_murx_data_size_au8_protocol_1[VLF_MAX_PACKET] = {
   4, // VLF_MU_DETECT,
   7, // VLF_MU_MASK,
   7, // VLF_MU_SET_CLASS,
   4, // VLF_MU_PING,
   5, // VLF_MU_LOCATE,
   4, // VLF_MU_IDENTIFY
   2  // VLF_MU_QUICK_SEARCH
};


//R.S. - this is the data size for different types of responses sent by the MU to the SU
const U8 uncoded_mutx_data_size_au8_protocol_1[VLF_MAX_PACKET] = {
   4, // VLF_MU_DETECT,
   2, // VLF_MU_MASK,
   2, // VLF_MU_SET_CLASS,
   2, // VLF_MU_PING,
   2, // VLF_MU_LOCATE,
   2, // VLF_MU_IDENTIFY
   0  // VLF_MU_QUICK_SEARCH
};


//////////////////////////////////////////////// PROTOCOL 2 - NEW PROTOCOL SU TO MU ////////////////////////////
// Add 12 bits for the preamble + 4 bits for the type and 8 bits for the CRC.  Each byte takes 80 ms to send  to the MU
// note that there are 4 bits for type = 16 combinations of which we currently only use 4

const U8 uncoded_murx_data_size_au8_protocol_2[VLF_MAX_PACKET] = {
   2, // VLF_MU_DETECT,          Sent to MU with 1 byte SU ID and 1 byte sequence = 5*80=400ms
#ifdef MU_ID_3BYTE
   5, // VLF_MU_MASK,            Sent to MU with 3 byte MU ID and 1 byte suid and 1 byte sequence = 8*80=640ms
#else
   4, // VLF_MU_MASK,            Sent to MU with 2 byte MU ID and 1 byte suid and 1 byte sequence = 7*80=560ms
#endif
   0, // VLF_MU_QUICK_SEARCH_Y,      Sent to MU 0 BYTES = 3*80=240ms 
   0, // VLF_MU_QUICK_SEARCH_Z,      Sent to MU 0 BYTES = 3*80=240ms      
#ifdef MU_ID_3BYTE
   4, // VLF_MU_LOCATE,          Sent to MU with 3 byte MU ID and 1 byte for number of seconds = 7*80=560ms
#else
   3, // VLF_MU_LOCATE,          Sent to MU with 2 byte MU ID and 1 byte for number of seconds = 6*80=480ms
#endif
   0, // VLF_MU_IDENTIFY         NOT USED
   0, // VLF_MU_QUICK_SEARCH_X     Sent to MU 0 BYTES = 3*80=240ms
   0, // VLF_MU_TEST
#ifdef MU_ID_3BYTE
   4 // VLF_MU_FULL_MASK,          Sent to MU with 3 byte MU ID and 1 byte for number of minutes = 7*80=560ms
#else
   3 // VLF_MU_FULL_MASK,          Sent to MU with 2 byte MU ID and 1 byte for number of minutes = 6*80=480ms
#endif
};


//////////////////////////////////////////////// PROTOCOL 1 - NEW PROTOCOL MU TO SU ////////////////////////////
// Add 12 bits for the preamble + 4 bits for the type and 8 bits for the CRC.  Each byte takes 42 ms to send  to the SU
// note that there are 4 bits for type = 16 combinations of which we currently only use 5



const U8 uncoded_mutx_data_size_au8_protocol_2[VLF_MAX_PACKET] = {
   4, // VLF_MU_DETECT,         Response to detect message with 4 byte MU ID  = 7*42=294ms
   1, // VLF_MU_MASK,           Respone to mask message with 1 byte for mobility = 4*42=168ms
   1, // VLF_MU_SET_CLASS,      NOT USED
   1, // VLF_MU_PING,           NOT USED
   1, // VLF_MU_LOCATE,         Response to locate message AND sent at 300ms intervals in locate mode with 1 byte for mobility = 4*42=168ms
   1, // VLF_MU_IDENTIFY        NOT USED
   4, // VLF_MU_QUICK_SEARCH    Response to quick search message with 4 byte MU ID  = 7*42=294ms
   0  // VLF_FULL_MU_MASK NOT USED
};

/////////////////////////////////////////////////////////////////////////////////////////////////


void dumpPreamble(Vlf_Decoder *p, int synccount);
void dumpData(Vlf_Decoder *p);

extern char g_strbuffer[256];

void swap16_byteorder(char* buf) { char a = *buf; buf[0]=buf[1]; buf[1] = a; };
// swap bytes to convert between intel and motorolla format
void swap32_byteorder(char* buf) { char a = *buf; buf[0]=buf[3]; buf[3] = a; a = buf[1]; buf[1]=buf[2]; buf[2]=a;};


void build_ref_decode_preamble(Vlf_Decoder *p)
{
	int i;
	int j=0;
	int k;
	U8* preamble = p->aRefpreamble;
#ifdef SU_RX
	if (g_protocol == VLF_PROTOCOL_2) 			
		p->u8Preamble_size = PCKT_PREAMBLE_SIZE_MU_TO_SU_P2*VLF_NUM_RX_BIT_SUBPERIODS;
	else
		p->u8Preamble_size = PCKT_PREAMBLE_SIZE*VLF_NUM_RX_BIT_SUBPERIODS;
#else
	p->u8Preamble_size = PCKT_PREAMBLE_SIZE*VLF_NUM_RX_BIT_SUBPERIODS;
#endif
	p->preamble_type = PREAMBLE_DECODE;

	for (i=0;i<PCKT_PREAMBLE_SIZE;i++) {
		p->aRefPreambleBits[i] = open_flag_au8[i];
		for (k=0;k<VLF_NUM_RX_BIT_SUBPERIODS;k++) {
			preamble[j++] = open_flag_au8[i] * VLF_RX_CYCLES_PER_BIT/VLF_NUM_RX_BIT_SUBPERIODS;
		}
	}
}

void build_ref_locate_preamble(Vlf_Decoder *p)
{
	int i;
	int j=0;
	int k;
	U8* preamble = p->aRefpreamble;
	p->u8Preamble_size = LOC_SIGNTAURE_SIZE*VLF_NUM_RX_BIT_SUBPERIODS;
	p->preamble_type = PREAMBLE_LOCATE;

	for (i=0;i<LOC_SIGNTAURE_SIZE;i++) {
		p->aRefPreambleBits[i] = loc_sig_au8[i];
		for (k=0;k<VLF_NUM_RX_BIT_SUBPERIODS;k++) {
			preamble[j++] = loc_sig_au8[i] * VLF_RX_CYCLES_PER_BIT/VLF_NUM_RX_BIT_SUBPERIODS;
		}
	}
}


// IN DECODE AND LOCATE MODE, WE COUNT ZERO CROSSINGS AT A 1/4 BIT RATE AND THEN DO A CORRELATION OF THE INCOMING 1/4 BIT STREAM
// WITH THE EXPECTED BIT PREAMBLE FOR EITHER DECODE OR LOCATE
// ONCE WE GET A SYNC, WE EITHER DECODE THE PACKET OR WE RETURN THE LOCATE STATS.
// IN BOTH DECODE AND LOCATE STATES, WE ALWAYS INCLUDE AN RSSI READING FOR THE MESSAGE
void initVlfPacketDecoder(Vlf_Decoder *p)
{
	p->iPreamble_counter = 0;
	p->iType_counter = 0;
	p->iData_counter = 0;
	p->iData_length = 0;
	p->iDecodingStage = VLF_RX_BUFFERING;
}

int DecodeVlfPacket(Vlf_Decoder *p, Vlf_Sample* psample)
{
	int i;
	Vlf_Sample* pSamples;
	int synccount;
	int bytes;
	
	switch (p->iDecodingStage)
	{
		case VLF_RX_STANDBY:
			break;
			
		case VLF_RX_BUFFERING:
			p->aPreamble_buffer[p->iPreamble_counter] = *psample;
			p->iPreamble_counter++;
			if (p->iPreamble_counter == (p->u8Preamble_size-1)) {
				p->iDecodingStage = VLF_RX_PREAMBLE;
			}
			break;
			
		case VLF_RX_PREAMBLE:	// buffer is full so shift and correlate to find preamble
			pSamples = p->aPreamble_buffer;
			for (i=0;i< (p->u8Preamble_size-1);i++) {
				pSamples[i] = pSamples[i+1];
			}
			pSamples[i] = *psample;

			// perform correlation to look for a preamble - if good then move to reading the packet type
			synccount = CorrelatePreamble(p);
#ifdef SU_RX
			if ((g_UnitFlags.DebugMisc) && (synccount > p->u8Preamble_size/2))  dumpPreamble(p,synccount);
			if (synccount == p->u8Preamble_size) {
				if (p->preamble_type == PREAMBLE_LOCATE) {		// a locate preamble has no payload to it
			 		traceS("VLF_RX_PACKET_READY");	
					p->iDecodingStage = VLF_RX_PACKET_READY;
				}
				else {
					if (g_protocol == VLF_PROTOCOL_2) {			
	///////////// this is for protocol 2 only in which case there is no type since there is only a generic detect response //	
						p->vlf_packet.type_u8 = VLF_MU_DETECT;
						bytes = MU_PROTOCOL2_RESPONSE_PAYLOADSIZE + 1;		// just one response size for MU to SU messages
						p->iData_length = 8 * bytes * VLF_NUM_RX_BIT_SUBPERIODS;
						p->iData_counter = 0;
						p->iDecodingStage = VLF_RX_DATA;
						//LED_SIGNAL2;
			 			traceS("VLF_RX_DATA P2");	
						if (g_UnitFlags.DebugMisc) {
							sprintf(g_strbuffer,"RX data len= %d buffer=%d",p->iData_length, sizeof(p->aData_buffer));
							PortPutCRStr(g_strbuffer,PC_PORT,1);
						}
	///////////// end of this is for protocol 2 only ////////////////////////////////////////////////////	
					}
					else 	// protocol 0 searches for the type
					{
						p->iType_counter = 0;
						p->iDecodingStage = VLF_RX_TYPE;
						//LED_SIGNAL2;
			 			traceS("processing VLF_RX_TYPE P1");	
					}	
				}
			}
#endif

#ifdef MU_APP
			if ((g_UnitFlags.DebugMisc) && (synccount > p->u8Preamble_size/2))  dumpPreamble(p,synccount);
			if (synccount == p->u8Preamble_size) {
				if (p->preamble_type == PREAMBLE_LOCATE) {		// a locate preamble has no payload to it
					p->iDecodingStage = VLF_RX_PACKET_READY;
				}
				else {
					p->iDecodingStage = VLF_RX_TYPE;
	 				traceS("VLF_RX_TYPE");		
	 			}
			}
#endif
			break;
			
		case VLF_RX_TYPE:
			p->aType_buffer[p->iType_counter++] = *psample;
			if (p->iType_counter == PCKT_TYPE_BIT_COUNT) {
				p->vlf_packet.type_u8 = DecodeTypeBuffer(p->aType_buffer);
				
#ifdef MU_APP
				// limit type to those that are supported
				if ((p->vlf_packet.type_u8 == VLF_MU_DETECT) || (p->vlf_packet.type_u8 == VLF_MU_MASK) || 
					(p->vlf_packet.type_u8 == VLF_MU_LOCATE) || (p->vlf_packet.type_u8 == VLF_MU_QUICK_SEARCH) ) {
						bytes = uncoded_murx_data_size_au8_protocol_2[p->vlf_packet.type_u8] + 1; // bytes + 1 byte for checksum
				}
				else {
					bytes = 1;
				}
#endif
#ifdef SU_RX
				if (g_protocol == VLF_PROTOCOL_2) {
					bytes = MU_PROTOCOL2_RESPONSE_PAYLOADSIZE + 1;		// just one response size for MU to SU messages
				}
				else {
					bytes = uncoded_mutx_data_size_au8_protocol_1[p->vlf_packet.type_u8] + 1; // bytes + 1 byte for checksum			
				}	
#endif				
				if (g_protocol == VLF_PROTOCOL_1) {
					bytes = bytes*2 + 1;
				}
				p->iData_length = 8 * bytes * VLF_NUM_RX_BIT_SUBPERIODS;
				p->iData_counter = 0;
				p->iDecodingStage = VLF_RX_DATA;
	 			traceS("VLF_RX_DATA");	
				if (g_UnitFlags.DebugMisc) {
					sprintf(g_strbuffer,"RX data len= %d buffer=%d",p->iData_length, sizeof(p->aData_buffer));
					PortPutCRStr(g_strbuffer,PC_PORT,1);
				}
			}
			break;
			
		case VLF_RX_DATA:
			p->aData_buffer[p->iData_counter++] = *psample;
			if (p->iData_counter == p->iData_length) {
				p->iDecodingStage = VLF_RX_PACKET_READY;
	 			traceS("VLF_RX_PACKET_READY");
	 			if (g_UnitFlags.DebugMisc) {
		 			dumpData(p);
		 		}		
			}			
			break;
		default:
			break;	
	}
	return p->iDecodingStage;	
}

// extracts the packet type from the buffer of 1/4 bits each of which is represented by a count of zero crossings of the carrier frequency
U8 DecodeTypeBuffer(Vlf_Sample buffer[PCKT_TYPE_BIT_COUNT])
{
	int i;
	int sum;
	U8 type = 0;
	U8 shifter = 1;
	
	for (i=0;i<	PCKT_TYPE_BIT_COUNT;i+=VLF_NUM_RX_BIT_SUBPERIODS) {
//		sum = 0;
//		for (j=0;j< VLF_NUM_RX_BIT_SUBPERIODS;j++) {
//			sum += buffer[i+j];
//		}
/////////////// hardcoded faster loop ///////////////////////////	
		sum = buffer[i].value;	
		sum += buffer[i+1].value;	
		sum += buffer[i+2].value;	
		sum += buffer[i+3].value;	
		if (sum > VLF_RX_CYCLES_PER_BIT/2)
			type |= shifter;
		shifter <<=1;	
	}
	return type;
}

// extracts the packet date from the buffer of 1/4 bits each of which is represented by a count of zero crossings of the carrier frequency
void DecodeDataBuffer(Vlf_Decoder* p)
{
/////////////////////////// Compress quarter bits back to bits during time of read ///////////////////
	U8 decoded_buffer[MAX_ENCODED_BUFFER_SIZE];
	U8 bitcount_array[MAX_ENCODED_BUFFER_SIZE*8];
	int i,j;
	int sum;
	
	U8 ix = 0;
	for (i=0;i<	p->iData_length;i+=VLF_NUM_RX_BIT_SUBPERIODS) {
//		sum = 0;
//		for (j=0;j< VLF_NUM_RX_BIT_SUBPERIODS;j++) {
//			sum += p->aData_buffer[i+j];
//		}
/////////////// hardcoded faster loop ///////////////////////////	
		sum = p->aData_buffer[i].value;	
		sum += p->aData_buffer[i+1].value;	
		sum += p->aData_buffer[i+2].value;	
		sum += p->aData_buffer[i+3].value;	
		bitcount_array[ix++] = sum/VLF_NUM_RX_BIT_SUBPERIODS;
	}
	// note: ix contains size of bitcount_array;

	if (g_UnitFlags.DebugMisc) {
		sprintf(g_strbuffer,"dc=%d ba=%d ix=%d ",sizeof(decoded_buffer),sizeof(bitcount_array),ix);
		PortPutCRStr(g_strbuffer,PC_PORT,1);
		PortPutStr("RX Packet data bits =",PC_PORT,1);
		for (i=0;i<ix;i++) {
			sprintf(g_strbuffer,"%2d ",bitcount_array[i]);
			PortPutStr(g_strbuffer,PC_PORT,2);
		}
		PortPutStr("\r\n",PC_PORT,1);
	} 	
		
	
	/////////////////////////// Decode array of encoded bits ///////////////////

	if (g_protocol == VLF_PROTOCOL_1) {
		p->vlf_packet.num_u8 = Verterbi_Decode(bitcount_array, ix, decoded_buffer);
		for (ix=0;ix< p->vlf_packet.num_u8;ix++) {
			p->vlf_packet.u.data_au8[ix] = decoded_buffer[ix];
			if (ix > sizeof(p->vlf_packet.u.data_au8)) {
				break;
			}
		}
	}
	else {
		p->vlf_packet.num_u8 = 0;
		U8 shifter;
		for (i=0;i<	ix;i+=8) {
			p->vlf_packet.u.data_au8[p->vlf_packet.num_u8] = 0;
			shifter = 0x01;
			for (j=0;j< 8;j++) {
				if (bitcount_array[i+j] > VLF_RX_MID_THRESHOLD)	// if it is a one
					p->vlf_packet.u.data_au8[p->vlf_packet.num_u8] |= (shifter<<j);
			}
			p->vlf_packet.num_u8++;
		}
	}


	if (g_UnitFlags.DebugMisc) {
		if (p->vlf_packet.num_u8 == 0) {
			PortPutCRStr("Zero length on packet data decode!",PC_PORT,1);
		}
	} 		

}	


// Sum the differential between the reference and the recorded signal
int CorrelatePreamble(Vlf_Decoder *p)
{
	
/*
	int i,k;
	int bitvalue;
	int zerocrossings;
	int synccount = 0;
	int refbit;
	int onebits;

			for (i=0;i<p->u8Preamble_size;i+=VLF_NUM_RX_BIT_SUBPERIODS) {
		zerocrossings = 0;
		refbit = p->aRefPreambleBits[i/VLF_NUM_RX_BIT_SUBPERIODS];
		onebits = 0;
		for (k=0;k<VLF_NUM_RX_BIT_SUBPERIODS;k++) {
			zerocrossings = p->aPreamble_buffer[i+k];
			if (zerocrossings > VLF_RX_HIGH_THRESHOLD) 
				onebits++;
		}
		if (refbit) {
			if (onebits > 2)
				synccount+=4;
			else
				return synccount;
		}
		else {
			if (onebits < 2)
				synccount+=4;
			else
				return synccount;			
		}
	}
*/
///////////////////////// hard-coded faster version ///////////////////
	int i;
	int synccount = 0;
	int bitmatch;

	for (i=0;i<p->u8Preamble_size;i+=VLF_NUM_RX_BIT_SUBPERIODS) {
		bitmatch = 0;

		if (p->aRefPreambleBits[i/VLF_NUM_RX_BIT_SUBPERIODS]) 	// if it is a 1 bit
		{
#ifdef ENABLE_ZC_THRESHOLD_TESTING
			if (p->aPreamble_buffer[i+0].value > vlf_high_threshold) bitmatch++;
			if (p->aPreamble_buffer[i+1].value > vlf_high_threshold) bitmatch++;
			if (p->aPreamble_buffer[i+2].value > vlf_high_threshold) bitmatch++;
			if (p->aPreamble_buffer[i+3].value > vlf_high_threshold) bitmatch++;
#else
			if (p->aPreamble_buffer[i+0].value > VLF_RX_PREAMBLE_HIGH_THRESHOLD) bitmatch++;
			if (p->aPreamble_buffer[i+1].value > VLF_RX_PREAMBLE_HIGH_THRESHOLD) bitmatch++;
			if (p->aPreamble_buffer[i+2].value > VLF_RX_PREAMBLE_HIGH_THRESHOLD) bitmatch++;
			if (p->aPreamble_buffer[i+3].value > VLF_RX_PREAMBLE_HIGH_THRESHOLD) bitmatch++;
#endif
			if (bitmatch > 2)		// if at least 3 out of 4 bits match
				synccount+=4;
			else
				return synccount;
		}
		else 	// if it is a zero bit
		{
#ifdef ENABLE_ZC_THRESHOLD_TESTING
			if (p->aPreamble_buffer[i+0].value < vlf_low_threshold) bitmatch++;
			if (p->aPreamble_buffer[i+1].value < vlf_low_threshold) bitmatch++;
			if (p->aPreamble_buffer[i+2].value < vlf_low_threshold) bitmatch++;
			if (p->aPreamble_buffer[i+3].value < vlf_low_threshold) bitmatch++;
#else
			if (p->aPreamble_buffer[i+0].value < VLF_RX_PREAMBLE_LOW_THRESHOLD) bitmatch++;
			if (p->aPreamble_buffer[i+1].value < VLF_RX_PREAMBLE_LOW_THRESHOLD) bitmatch++;
			if (p->aPreamble_buffer[i+2].value < VLF_RX_PREAMBLE_LOW_THRESHOLD) bitmatch++;
			if (p->aPreamble_buffer[i+3].value < VLF_RX_PREAMBLE_LOW_THRESHOLD) bitmatch++;
#endif
			if (bitmatch > 2)		// if at least 3 out of 4 bits match
				synccount+=4;
			else
				return synccount;			
		}
	}	

	return synccount;
}

#ifdef USE_CRC8
unsigned char GetCRC8( unsigned char crc , unsigned char ch)
{
   int i ;

   for ( i = 0 ; i < 8 ; i++ ) {
      if ( crc & 0x80 ) {
         crc<<=1;
         if ( ch & 0x80 ) {
            crc = crc | 0x01;
         } else {
            crc =crc & 0xfe;
         } 
         crc = crc ^ 0x85;
      } else {
        crc<<=1;
        if ( ch & 0x80 ) {
            crc = crc | 0x01;
        } else {
            crc = crc & 0xfe;
        }
      }
      ch<<=1;
   }
   return ( crc );
}

U8 CalcCRC(U8* p, U8 check_u8, U8 len)
{
	U8 ii_u8;
	for ( ii_u8 = 0; ii_u8 < len; ii_u8++ ) {
    	check_u8 = GetCRC8(check_u8,p[ ii_u8 ]);
   	}
   	check_u8 = GetCRC8(check_u8,0x00);
   	return check_u8;
}

#else

unsigned char GetCRC8( unsigned char crc , unsigned char ch)
{
	return (crc ^ ch);
}

U8 CalcCRC(U8* p, U8 check_u8, U8 len)
{
    U8 ii_u8;
    if (len > 0) {
	    for ( ii_u8 = 0; ii_u8 < len; ii_u8++ ) {
	       check_u8 = GetCRC8(check_u8, p[ ii_u8 ]);
	   }
   }
   return check_u8;
}

#endif



////////////////////////////////////////// TEST ROUTINES //////////////////////////////////////
void test_preamble_correlation()
{
	int i;
	int count=0;
	
	Vlf_Decoder vlf;
	long results[PCKT_PREAMBLE_BIT_COUNT];
	build_ref_decode_preamble(&vlf);
	
	for (i=0;i<vlf.u8Preamble_size;i++) {
		build_test_preamble(&vlf, i, TRUE);
		results[i] = CorrelatePreamble(&vlf);
		count += results[i];
		if (g_UnitFlags.DebugMisc) {
			sprintf(g_strbuffer,"correlation result %d = %ld",i, results[i]);
			PortPutCRStr(g_strbuffer,PC_PORT,1);
		}
	}
	if (g_UnitFlags.DebugMisc) {
		sprintf(g_strbuffer,"matches = %d",count);
		PortPutCRStr(g_strbuffer,PC_PORT,1);
	}
}





void build_test_preamble(Vlf_Decoder *p, int offset, BOOL bBellShaped)
{
	int i;
	int j=0;
	
	Vlf_Sample* preamble = p->aPreamble_buffer;
	
	for (i=0;i<offset;i++) 
	{
		preamble[i].value = 0;
	}

	for (;i<PCKT_PREAMBLE_BIT_COUNT;i++,j++) 
	{
		preamble[i].value = open_flag_au8Bits[j];
		if (bBellShaped) 
		{
			if (j>0) {
				if ((open_flag_au8Bits[j] > 0) && (open_flag_au8Bits[j-1] == 0))
					preamble[i].value -= VLF_RX_MID_THRESHOLD/2;
				else if ((open_flag_au8Bits[j] == 0) && (open_flag_au8Bits[j-1] > 0))
					preamble[i].value += VLF_RX_MID_THRESHOLD/2;
			}
		}
	}
}

void dumpPreamble(Vlf_Decoder *p, int synccount)
{
	int i,k;
	sprintf(g_strbuffer,"sync ended on bit %d of %d: %s",
		synccount/VLF_NUM_RX_BIT_SUBPERIODS,
		p->u8Preamble_size/VLF_NUM_RX_BIT_SUBPERIODS,
		(synccount==p->u8Preamble_size)?"Match":"Mismatch"
		);
	PortPutCRStr(g_strbuffer,PC_PORT,1);
	
	for (i=0;i<p->u8Preamble_size;i+=VLF_NUM_RX_BIT_SUBPERIODS) {
		for (k=0;k<VLF_NUM_RX_BIT_SUBPERIODS;k++) 
		{
			sprintf(g_strbuffer,"%3d ",p->aPreamble_buffer[i+k].value);
			PortPutStr(g_strbuffer,PC_PORT,1);
		}
		PortPutStr("\r\n",PC_PORT,1);
	}
	
}

void dumpData(Vlf_Decoder *p)
{
	int i,k;
	sprintf(g_strbuffer,"data length= %d",p->iData_length);
	PortPutCRStr(g_strbuffer,PC_PORT,1);
	
	for (i=0;i<p->iData_length;i+=VLF_NUM_RX_BIT_SUBPERIODS) {
		for (k=0;k<VLF_NUM_RX_BIT_SUBPERIODS;k++) 
		{
			sprintf(g_strbuffer,"%3d ",p->aData_buffer[i+k].value);
			PortPutStr(g_strbuffer,PC_PORT,1);
		}
		PortPutStr("\r\n",PC_PORT,1);
	}
	
}



