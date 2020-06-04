#ifndef __WiInterface_H
#define __WiInterface_H

#include "wi.h"
#include "queue.h"

extern WORD_VAL g_LastSourcePANID;
extern WORD_VAL g_LastSourceShortAddress;
//extern BYTE g_LastSourceLongAddress[8];
extern long g_MiWi_BytesProcessed;
extern unsigned long g_wireless_counter;
extern MAC_ADDR8 g_destMACAddress;
extern unsigned long tx_elapsed_ms;

#define MIWI_TX_FIFOSIZE (16)
#define MiWi_RX_FIFOSIZE (1024)
#define CALC_PERCENTAGE(a,b) ((((long)a)*100ul)/b)


int MiWi_isTxQueueEmpty();
int GetMiWi_TXpCent();
int MiWi_GetTxBufFreeSize();
unsigned int MiWi_GetTxBufSize();
void ResetMiWipeaks();
int MiWi_tx_enque(unsigned char *pbuf,int len);
int MiWi_tx_deque(unsigned char *pbuf,int len);
int init_Wireless();
//int MiWi_PutChar(char ch);
char MiWi_isCharReady();
int MiWi_GetRxBufFreeSize();
unsigned int MiWi_GetRxBufSize();
int MiWi_rx_enque(unsigned char *pbuf,int len);
int MiWi_rx_deque(unsigned char *pbuf,int len);
int Get_MiWi_RXpCent();
int GetMiWi_RXpeak();
int MiWi_isRxQueueEmpty();
char MiWi_GetChar();

void checkFor_WirelessMessages();
void process_WirelessTasks(char ch, char *msg, int len, MAC_ADDR8* dest);
void SendTextNetworkTable();
unsigned char* getLastSourceLongAddress();
char* GetMyFormattedMACAddress();
char* GetFormattedMACAddress(MAC_ADDR8* address);
char* FormatDeviceType(unsigned char device);
MAC_ADDR8* getMyMACAddress();
void setDestWiAddress(unsigned char type, unsigned long id);	
void setWiAddress(MAC_ADDR8* dest, unsigned char type, unsigned long id);	
void SendWirelessTextMsg(char* msg);
void SendWirelessTextCRMsg(char* msg);
void WaitForWiTXNotBusy(int ms);

#endif // __WiInterface_H
