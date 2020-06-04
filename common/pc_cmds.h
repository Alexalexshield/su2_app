
#ifndef PC_CMDS_H
#define PC_CMDS_H

void Process_PC_CSV_Packet(struct CSV_PACKET_HANDLER* phandler, int port);
void ProcessPCStream();
void InitPCStream();
void init_unitflags();
void PutHelp(int port);
unsigned int axtoi(char* hexStg);

#endif // PC_CMDS_H

