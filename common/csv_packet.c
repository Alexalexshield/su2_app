#include "system.h"
#include "csv_packet.h"
#include "port.h"

void init_CSV_packethandler(struct CSV_PACKET_HANDLER* phandler,unsigned char * buffer, unsigned int length)
{
	phandler->hdrcount=0;
	phandler->datacount=0;
	phandler->checksum=0;
	phandler->m_PacketStage = CSV_PACKET_LOOKING;
	phandler->packet_timeouts = 0;
	phandler->header[0] = 0;
	phandler->buffersize = length;
	phandler->pbuf = buffer;
	buffer[0]=0;
}

void reset_CSV_packethandler(struct CSV_PACKET_HANDLER* phandler)
{
	phandler->hdrcount=0;
	phandler->datacount=0;
	phandler->checksum=0;
	phandler->m_PacketStage = CSV_PACKET_LOOKING;
	phandler->packet_timeouts = 0;
	phandler->header[0] = 0;
	phandler->pbuf[0] = 0;
}

unsigned char process_CSV_packetbyte(struct CSV_PACKET_HANDLER* phandler, unsigned char rdByte)
{
	unsigned char retbyte=0;
	unsigned long long elapsed_ms;
	
	if (rdByte == CSV_FRAMECHAR) {
		if (phandler->m_PacketStage != CSV_PACKET_LOOKING) {
			g_SysError.RX2_Packet_Error = 1;
		}	
		phandler->datacount=0;
		phandler->m_PacketStage = CSV_PACKET_HEADER;
		phandler->header[0] = rdByte;
		phandler->hdrcount = 1;
		phandler->checksum=0;
		phandler->start_tick = g_tick;
		phandler->pbuf[0]=0;
		return 0;
	}	
	
	switch (phandler->m_PacketStage) {
		case CSV_PACKET_LOOKING:
			retbyte = rdByte;
			break;
		case CSV_PACKET_HEADER:
			if (rdByte == '\n') {
				g_SysError.RX2_Packet_Error = 1;
				phandler->m_PacketStage = CSV_PACKET_LOOKING;
			}
			else {	
				if (phandler->hdrcount < CSV_PACKET_MAX_HEADER_SIZE) {
					phandler->header[phandler->hdrcount++] = rdByte;
					phandler->checksum+=rdByte; 
				}	
				if (phandler->hdrcount == CSV_PACKET_MAX_HEADER_SIZE) {
					phandler->header[CSV_PACKET_MAX_HEADER_SIZE] = 0;	// terminate the string
					phandler->datacount=0;
					phandler->m_PacketStage = CSV_PACKET_DATASTREAM;
					phandler->pbuf[0] = 0;
				}
			}		
			break;
		case CSV_PACKET_DATASTREAM:
			phandler->checksum+=rdByte;	
			phandler->pbuf[phandler->datacount++] = rdByte;
			if ((rdByte == '\n') || (rdByte == '\r')) {
				phandler->m_PacketStage = CSV_PACKET_READY;
				phandler->pbuf[phandler->datacount] = 0;
			}	
			else {
				if (phandler->datacount >= phandler->buffersize) {
					phandler->m_PacketStage = CSV_PACKET_READY;
					phandler->pbuf[phandler->buffersize]=0;
				}
			}	
			break;
		default:
			break;
	}

	if ((phandler->m_PacketStage != CSV_PACKET_LOOKING) && (phandler->m_PacketStage != CSV_PACKET_READY)) {
		elapsed_ms = g_tick - phandler->start_tick;
		if (elapsed_ms > 10000) {	// packets should not take longer than 10 seconds to build
			phandler->packet_timeouts++;
			phandler->m_PacketStage = CSV_PACKET_LOOKING;
		}
	}

	return retbyte;
}	



// gets nth comma seperated value from 1 to n fields
int GetNthCSV(const char *p,int n, char* dest, int destlen)
{
	int error = 1;
	int i;
	const char *pstart,*pend;
	if (destlen <= 0) return 1;
	dest[0]=0;
	
	if (p != NULL) {
		if (*p == ',') p++;	// skip over first character if it is a comma since it is for the header
		
		for (i=0;i<n;i++) {
			pstart=p;
			while ((*p) && (*p!='\r') && (*p!= '\n') && (*p != ',')) p++; // move to the next , or end whichever comes first
			if ( (*p ==0) || (*p =='\r') || (*p == '\n')) {
				i++;
				p++;
				break;
			}	
			p++;	
		}	
		if ((i == n) && (p>pstart)) { 		
			pend=p-1;	// save pointer 
			i=0;
			while ((pstart < pend) && (i< (destlen-1))) {
				*dest++ = *pstart++;
				i++;
			}
			*dest = 0;	
			error = 0;
		}
	}	
	return error;
}

