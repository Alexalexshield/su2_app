
#ifndef __CSV_PACKET_H
#define __CSV_PACKET_H

#define CSV_FRAMECHAR '$'

#define CSV_PACKET_MAX_HEADER_SIZE 6


typedef enum {
	CSV_PACKET_LOOKING=0, 
	CSV_PACKET_HEADER, 
	CSV_PACKET_DATASTREAM, 
	CSV_PACKET_CHECKSUM, 
	CSV_PACKET_READY
} eCSVPacketStage;


struct CSV_PACKET_HANDLER {
	int hdrcount;
	unsigned int datacount;
	unsigned char checksum;
	eCSVPacketStage m_PacketStage;
	unsigned long long start_tick;
	int packet_timeouts;
	unsigned int buffersize;
	unsigned char header[CSV_PACKET_MAX_HEADER_SIZE+1];
	unsigned char* pbuf;
};


unsigned char process_CSV_packetbyte(struct CSV_PACKET_HANDLER* phandler, unsigned char rdByte);
void init_CSV_packethandler(struct CSV_PACKET_HANDLER* phandler,unsigned char * buffer, unsigned int length);
void reset_CSV_packethandler(struct CSV_PACKET_HANDLER* phandler);
int GetNthCSV(const char *p,int n, char* dest, int destlen);

#endif // __CSV_PACKET_H
