#include "vlf.h"

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
   4, // VLF_MU_MASK,            Sent to MU with 4 byte MU ID and 1 byte suid and 1 byte sequence = 9*80=720ms
   0, // VLF_MU_QUICK_SEARCH_Y,  QUICK SEARCH ON Y - 1 byte SU ID and 1 byte sequence 3*80=240ms
   0, // VLF_MU_QUICK_SEARCH_Z,  QUICK SEARCH ON Z - 1 byte SU ID and 1 byte sequence 3*80=240ms
   3, // VLF_MU_LOCATE,          Sent to MU with 4 byte MU ID and 1 byte for number of seconds = 8*80=640ms
   0, // VLF_MU_IDENTIFY         NOT USED
   0  // VLF_MU_QUICK_SEARCH_X     Sent to MU with 1 byte SU ID and 1 byte sequence 3*80=240ms
};

//////////////////////////////////////////////// PROTOCOL 1 - NEW PROTOCOL MU TO SU ////////////////////////////
// Add 12 bits for the preamble + 4 bits for the type and 8 bits for the CRC.  Each byte takes 42 ms to send  to the SU
// note that there are 4 bits for type = 16 combinations of which we currently only use 5

#define MU_PROTOCOL2_RESPONSE_PAYLOADSIZE 3
const U8 uncoded_mutx_data_size_au8_protocol_2[VLF_MAX_PACKET] = {
   4, // VLF_MU_DETECT,         Response to detect message with 4 byte MU ID  = 7*42=294ms
   1, // VLF_MU_MASK,           Respone to mask message with 1 byte for mobility = 4*42=168ms
   4, // VLF_MU_QUICK_SEARCH_Y, Response to quick search
   4, // VLF_MU_QUICK_SEARCH_Z, Response to quick search
   1, // VLF_MU_LOCATE,         Response to locate message AND sent at 300ms intervals in locate mode with 1 byte for mobility = 4*42=168ms
   1, // VLF_MU_IDENTIFY        NOT USED
   4  // VLF_MU_QUICK_SEARCH_X  Response to quick search message with 4 byte MU ID  = 7*42=294ms
};

