#ifndef __SDFUNCTIONS_H
#define __SDFUNCTIONS_H

#include "csv_packet.h"
#include "su_slcd.h"

extern MemProperties g_SDProperties;
extern long g_MemUsed;
extern long g_MemFolders;
extern long g_MemFiles;

char *FS_GetlasterrMsg();
void SetCurrentFileTime();
int SD_readtest(  int source_port);
int SD_writetest(  int source_port);
char * FS_FindNextLogName();
char * FS_FindNextLogDir();
void SD_power(char power);
char getSD_power();
int SD_OpenLogFile(int source_port);
void SD_CloseLogFile(int source_port);
int SD_RefreshProperties(int source_port);
long SD_CopyFiles(char *cmd, int source_port);
long SD_EraseFile(char * cmnd , int source_port);
size_t SD_Write(char *pbuf,size_t len, int destport);
int CheckForSDCardPresence();
long SD_CountLogFiles(int source_port);
long SD_GetLastNFilenames(LogFileData* pf, int len, int source_port);
int SD_countfilelines(LogFileData fd, int source_port);
int SD_getLogFileLines(LogFileData fd, char pbuf[FILE_MAX_LINES_PER_PAGE][MAX_DISPLAY_LINE_WIDTH], int startline, int linecount, int source_port);
void SD_GetStatusMessage(char szStatus[40]);

#endif // __SDFUNCTIONS_H
