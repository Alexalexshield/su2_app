// RETURNS TEXT VERSION OF FS ERROR MESSAGE
#include "system.h"
#include "putfunctions.h"
#include "FSIO.h"
#include "sdfunctions.h"
#include "stdio.h"
#include "string.h"
#include "port.h"
#include "trace.h"
#include "stdlib.h"
#if (RTCC_SOURCE == RTCC_INTERNAL)	
#include "rtcc.h"
#elif (RTCC_SOURCE == RTCC_EXTERNAL)	
#include "i2c_ISL12024_rtc.h"
#endif

#ifdef SU_APP
#define SOURCE_PREFIX "SU"
#endif

int g_lastSD_Debugport = PC_PORT;

// put current memory properties
void PutMemoryProperties(int port)
{
	char strbuffer[80];
	sprintf(strbuffer,"$%sEMP,type=%d,sectors=%lx,sectorbytes=%x,sectorsused=%lx,folders=%lx,files=%lx\r\n",
		SOURCE_PREFIX,
		g_SDProperties.MemType,
		g_SDProperties.MemSectorCapacity,
		g_SDProperties.MemBytesPerSector,
		g_SDProperties.MemSectorUsed,
		g_SDProperties.MemFolders,
		g_SDProperties.MemFiles
		);
	PortPutStr(strbuffer,port,1);
}	


void PutFileNumCopied(unsigned long fcopied,int port)
{
	char strbuffer[50];
	sprintf(strbuffer,"$%sCPD,%ld\r\n",SOURCE_PREFIX,fcopied);
	PortPutStr(strbuffer,port,1);
}

void PutFileToCopy(long count, char *dir, char* name,int port)
{
	char strbuffer[50];
	sprintf(strbuffer,"$%sCPY,%ld,%s,%s\r\n",SOURCE_PREFIX,count,dir,name);
	PortPutStr(strbuffer,port,1);
}

void PutFileNumber(long count, unsigned long lindex, int port)
{
	char strbuffer[50];
	sprintf(strbuffer,"$%sCPY,%ld,%ld,\r\n",SOURCE_PREFIX,count,lindex);
	PortPutStr(strbuffer,port,1);
}

// in this hardware the SD is always powered so this function doesn't do anything
void SD_power(char power)
{
//	SDPOWER_TRIS = 0;				// SET THE SDPOWER LINE TO OUTUT
//	SDPOWER = power?1:0;			// POWER ON THE SDCARD
//	DelayMsecs(100);				// delay before powerup
}

char getSD_power()
{
//	return SDPOWER_LATCH;
	return 1;						// in this hardware the SD is always powered
}
	
void SD_GetStatusMessage(char szStatus[40])
{
	szStatus[0] = 0;

	if (g_SysError.SDNoCard)
		strcat(szStatus,"SD na    ");
	else {
		if (g_SysError.SDMountError)
			strcat(szStatus,"SD MntErr   ");
		else if (g_SysError.SDError_write_error)
			strcat(szStatus,"SD WrErr   ");
		else
			strcat(szStatus,"SD Ok     ");
	}
}



int MountMedia();

#define PROCESS_TASKS()			


const char* FS_errormsgs[CE_NUM_ERRORS] = {
	"CE_GOOD",                    // No error
    "CE_ERASE_FAIL",                  // An erase failed
    "CE_NOT_PRESENT",                 // No device was present
    "CE_NOT_FORMATTED",               // The disk is of an unsupported format
    "CE_BAD_PARTITION",               // The boot record is bad
    "CE_UNSUPPORTED_FS",              // The file system type is unsupported
    "CE_INIT_ERROR",                  // An initialization error has occured
    "CE_NOT_INIT",                    // An operation was performed on an uninitialized device
    "CE_BAD_SECTOR_READ",             // A bad read of a sector occured
    "CE_WRITE_ERROR",                 // Could not write to a sector
    "CE_INVALID_CLUSTER",             // Invalid cluster value > maxcls
    "CE_FILE_NOT_FOUND",              // Could not find the file on the device
    "CE_DIR_NOT_FOUND",               // Could not find the directory
    "CE_BAD_FILE",                    // File is corrupted
    "CE_DONE",                        // No more files in this directory
    "CE_COULD_NOT_GET_CLUSTER",       // Could not load/allocate next cluster in file
    "CE_FILENAME_2_LONG",             // A specified file name is too long to use
    "CE_FILENAME_EXISTS",             // A specified filename already exists on the device
    "CE_INVALID_FILENAME",            // Invalid file name
    "CE_DELETE_DIR",                  // The user tried to delete a directory with FSremove
    "CE_DIR_FULL",                    // All root dir entry are taken
    "CE_DISK_FULL",                   // All clusters in partition are taken
    "CE_DIR_NOT_EMPTY",               // This directory is not empty yet, remove files before deleting
    "CE_NONSUPPORTED_SIZE",           // The disk is too big to format as FAT16
    "CE_WRITE_PROTECTED",             // Card is write protected
    "CE_FILENOTOPENED",               // File not opened for the write
    "CE_SEEK_ERROR",                  // File location could not be changed successfully
    "CE_BADCACHEREAD",                // Bad cache read
    "CE_CARDFAT32",                   // FAT 32 - card not supported
    "CE_READONLY",                    // The file is read-only
    "CE_WRITEONLY",                   // The file is write-only
    "CE_INVALID_ARGUMENT",            // Invalid argument
    "CE_TOO_MANY_FILES_OPEN",          // Too many files are already open	
    "CE_FLUSH_ERROR",
    "CE_CLUSTER_WRITE_ERROR",
    "CE_FILE_ENTRY_WRITE_ERROR",
    "CE_CLOSE_WRITE_ERROR",
    "CE_FATWRITE_ERROR",
    "CE_MBRWRITE_ERROR",
    "CE_RENAMEWRITE_ERROR",
    "CE_SECTORERASE_ERROR"
};	


// Set the date and time to 2:35:20 PM, January 12, 2007
//year - The year, from 1980 to 2107.
//month - The month, from 1-12.
//day - The day, from 1-31.
//hour - The hour of the day, from 0 (midnight) to 23.
//minute - The current minute, from 0 to 59.
//second - The current second, from 0 to 59.
void SetCurrentFileTime()
{
#if (RTCC_SOURCE == RTCC_INTERNAL)	
	RTCCProcessEvents();		// read the clock
	if (SetClockVars (mRTCCGetBinYear()+2000, mRTCCGetBinMonth(), mRTCCGetBinDay(), 
	mRTCCGetBinHour(), mRTCCGetBinMin(), mRTCCGetBinSec()))
	{
		traceS("Error setting file time"); // invalid parameters passed in
	}
#else
	ISL_ReadTime(&g_seconds);
	struct tm* pt = gmtime((const time_t * )&g_seconds);
	if (SetClockVars (pt->tm_year+1900, pt->tm_mon + 1, pt->tm_mday, pt->tm_hour, pt->tm_min, pt->tm_sec))
	{
		traceS("Error setting file time"); // invalid parameters passed in
	}
#endif	

}


/*
        unsigned IN_IDLE_STATE:1;
        unsigned ERASE_RESET:1;
        unsigned ILLEGAL_CMD:1;
        unsigned CRC_ERR:1;
        unsigned ERASE_SEQ_ERR:1;
        unsigned ADDRESS_ERR:1;
        unsigned PARAM_ERR:1;
        unsigned B7:1;
        unsigned CARD_IS_LOCKED:1;
        unsigned WP_ERASE_SKIP_LK_FAIL:1;
        unsigned ERROR:1;
        unsigned CC_ERROR:1;
        unsigned CARD_ECC_FAIL:1;
        unsigned WP_VIOLATION:1;
        unsigned ERASE_PARAM:1;
        unsigned OUTRANGE_CSD_OVERWRITE:1;
*/
extern MMC_RESPONSE g_SDresponse; 

void traceR1Errors()
{
	if (g_SDresponse.r1._byte == 0) {
		traceS("SD R1 response is ok");
	}
	if (g_SDresponse.r1._byte == 0xff) {
		traceS("SD R1 response is 0xff");
	}
	else {	
		if (g_SDresponse.r1.IN_IDLE_STATE) traceS("SD IN_IDLE_STATE");
        if (g_SDresponse.r1.ERASE_RESET) traceS("SD ERASE_RESET");
        if (g_SDresponse.r1.ILLEGAL_CMD) traceS("SD ILLEGAL_CMD");
        if (g_SDresponse.r1.CRC_ERR) traceS("SD CRC_ERR");
        if (g_SDresponse.r1.ERASE_SEQ_ERR) traceS("SD ERASE_SEQ_ERR");
        if (g_SDresponse.r1.ADDRESS_ERR) traceS("SD ADDRESS_ERR");
        if (g_SDresponse.r1.PARAM_ERR) traceS("SD PARAM_ERR");
        if (g_SDresponse.r1.B7) traceS("SD B7");
	}
}	

void traceSDErrors()
{
	if (g_SDresponse.r2._word != 0) {
		if (g_SDresponse.r2.IN_IDLE_STATE) traceS("SD IN_IDLE_STATE");
        if (g_SDresponse.r2.ERASE_RESET) traceS("SD ERASE_RESET");
        if (g_SDresponse.r2.ILLEGAL_CMD) traceS("SD ILLEGAL_CMD");
        if (g_SDresponse.r2.CRC_ERR) traceS("SD CRC_ERR");
        if (g_SDresponse.r2.ERASE_SEQ_ERR) traceS("SD ERASE_SEQ_ERR");
        if (g_SDresponse.r2.ADDRESS_ERR) traceS("SD ADDRESS_ERR");
        if (g_SDresponse.r2.PARAM_ERR) traceS("SD PARAM_ERR");
        if (g_SDresponse.r2.B7) traceS("SD B7");
        if (g_SDresponse.r2.CARD_IS_LOCKED) traceS("SD CARD_IS_LOCKED");
        if (g_SDresponse.r2.WP_ERASE_SKIP_LK_FAIL) traceS("SD WP_ERASE_SKIP_LK_FAIL");
        if (g_SDresponse.r2.ERROR) traceS("SD ERROR");
        if (g_SDresponse.r2.CC_ERROR) traceS("SD CC_ERROR");
        if (g_SDresponse.r2.CARD_ECC_FAIL) traceS("SD CARD_ECC_FAIL");
        if (g_SDresponse.r2.WP_VIOLATION) traceS("SD WP_VIOLATION");
        if (g_SDresponse.r2.ERASE_PARAM) traceS("SD ERASE_PARAM");
        if (g_SDresponse.r2.OUTRANGE_CSD_OVERWRITE) traceS("SD OUTRANGE_CSD_OVERWRITE");
	}
	else
	{
		traceS("SD response is 0");
	}			
}	


char *FS_GetlasterrMsg()
{
	int error = FSerror();
	if ((error >= 0) && (error < CE_NUM_ERRORS))
		return (char *)(FS_errormsgs[error]);
	else
		return "unknown error";
}

int CheckForSDCardPresence()
{
	MDD_InitIO();
	if (!MDD_MediaDetect())
		g_SysError.SDNoCard = 1;
	else
		g_SysError.SDNoCard = 0;
	return g_SysError.SDNoCard?FALSE:TRUE;
}

int MountMedia()
{
	int retry;

	MDD_InitIO();
	for (retry = 0; retry < 2; retry++) {
		if (MDD_MediaDetect())
			break;
   		DelayMsecs(5);          // wait for card to power up
		traceS("Waiting for card to be inserted");
	}
	if (retry >= 2) {
	    traceS("Card is not detected!");
		MDD_ShutdownMedia();
		g_SysError.SDNoCard = 1;
	    return 1;
	}		
	
	g_SysError.SDNoCard = 0;
	
	if (!FSInit()) {			// Initialize the library
	    traceS2("Media could not be mounted:", FS_GetlasterrMsg());
//	    traceSDErrors();
		g_SysError.SDMountError = 1;	// set SD error flag
		MDD_ShutdownMedia();
	    return 1;
	}  
	g_SysError.SDMountError = 0;  
	traceS("media mounted");
	return 0;
}

#define B_SIZE  10
int SD_readtest(int source_port)
{
	FSFILE * pointer;
	unsigned char i;
	char SD_rd_data[ B_SIZE];
	size_t r;
	g_lastSD_Debugport = source_port;

	traceS("Read test");
	if (MountMedia()) return FSerror();

   	if (FSchdir("\\") != 0)	{		// move to the root directory
	    traceS2("Chdir to root failed", FS_GetlasterrMsg());
//	    traceSDErrors();
	}	    

	// Open file 1 in read mode
	traceS("opening source.txtfor read");
	pointer = FSfopen ("source.txt", "r");
	if (pointer == NULL) {
	  traceS2("Could not open file:", FS_GetlasterrMsg());
//	  traceSDErrors();
		MDD_ShutdownMedia();
	  return FSerror();
	}  

	traceS("source.txt opened for read");

	do{
		r = FSfread (SD_rd_data, 1, B_SIZE, pointer); 
	    for( i=0; i<r; i++)
	        traceC( SD_rd_data[i]);
	} while( r==B_SIZE);
	FSfclose (pointer);
//    traceSDErrors();
	DelayMsecs(10);
	traceS("\r\nfile closed");
 	traceS("read test done!");
	MDD_ShutdownMedia();

	return 0;
    
} 

char * FS_FindNextLogName(int source_port) {
	static char namebuf[15];
	SearchRec file;
	unsigned char attributes = ATTR_HIDDEN | ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_VOLUME | ATTR_ARCHIVE;
	int error;
	int ix = 0;
	namebuf[0] = 0;
	g_lastSD_Debugport = source_port;
	do {
		sprintf(namebuf,"dest%04d.txt",ix++);
		traceS2("searching for ",namebuf);
		// Find any non-directory file that has a name starting with the letters FILE
		error = FindFirst (namebuf, attributes, &file);
	}
	while (error==0);
	return namebuf;	
}

char * FS_FindNextLogDir() {
	static char namebuf[15];
	SearchRec file;
	unsigned char attributes = ATTR_HIDDEN | ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_VOLUME | ATTR_ARCHIVE | ATTR_DIRECTORY;
	int error;
	int ix = 0;
	namebuf[0] = 0;
	do {
		sprintf(namebuf,"FM%06d",ix++);
		traceS2("searching for ",namebuf);
		error = FindFirst (namebuf, attributes, &file);
	}
	while (error==0);
	return namebuf;	
}	


	

#define WB_SIZE   256
int SD_writetest( int source_port)
{
//	FSFILE * fs = NULL;
	FSFILE * fd = NULL;
	char SD_wr_data[WB_SIZE];
	size_t r;
	int ix=0;
	char msgbuf[80];
	char fname[15];
	g_lastSD_Debugport = source_port;

	MDD_InitIO();	
	traceS("Write test");

	if (MountMedia()) return FSerror();

   	if (FSchdir("\\") != 0)	{		// move to the root directory
	    traceS2("Chdir to root failed", FS_GetlasterrMsg());
	    traceSDErrors();
	}	    
	    
	char *pdir = FS_FindNextLogDir();
	
    SetCurrentFileTime();
    
   	if (FSmkdir(pdir)) {
		sprintf(msgbuf,"Error making directory %s",pdir);
	    traceS2(msgbuf, FS_GetlasterrMsg());
	    traceSDErrors();
   }	
   else {
   		g_SDProperties.MemFolders++;
	   	if (FSchdir(pdir) != 0)	{		
			sprintf(msgbuf,"Error in changing to directory %s",pdir);
			traceSDErrors();
		}
		else {
		
		//	char *pdest = FS_FindNextLogName();
			
			for (ix=0;ix<1;ix++) {
				sprintf(fname, "dest%04d.txt",ix);
		    	traceS2("trying to open for writing:",fname);
		    	fd = FSfopen( fname, "w");
		
			    if (fd) {
//				    if (FSfseek(fs, 0l, SEEK_SET) != 0) {
//				   		traceS2("seek to start of source file failed:", FS_GetlasterrMsg());
//				   		traceSDErrors();
//				   	}	
		
				    traceS2("trying to write to :",fname);
			        do {
				        sprintf(SD_wr_data,"This is a test\r\n");
				        r = strlen(SD_wr_data);	        
//						r = FSfread( SD_wr_data, 1, WB_SIZE, fs);
						if (r) {
							sprintf(msgbuf,"Writing %d bytes to %s",r,fname);
							traceS(msgbuf);
				            r = FSfwrite( SD_wr_data, 1, r, fd);
				            traceC('.');
				  		}          
			        } while( r == WB_SIZE);
			
			        SetCurrentFileTime();
			        if (FSfclose(fd)) {
				   		traceS2("file close failed:", FS_GetlasterrMsg());
				   		traceSDErrors();
				   	}
				   	else {
				   		traceS2(fname, " closed");
   						g_SDProperties.MemFiles++;
				   	}	
		 	
			    }
			    else { 
			   		traceS2("failed to open file for writing:", FS_GetlasterrMsg());
			   		break;
				}
			}	
		}	
	}
		  
	DelayMsecs(10);
	MDD_ShutdownMedia();

 	traceS("write test done!");
	  
 	return 0;   
} 




FSFILE* g_search_logfd = NULL;
FSFILE* g_event_logfd = NULL;
char* g_pdir = NULL;

#define SEARCHLOG_FORMAT "SU%06ld.csv"
#define SEARCHLOG_SEARCH_FORMAT "SU*.csv"
#define SEARCHLOG_DELIMETER ".csv"
#define SEARCHLOG_VERSION 2
#define MAX_FILE_NUMBER 9999999l
#define SEARCHLOG_NAME_NUMBERIX 2
#define SEARCHLOG_DIRECTORY_PATHNAME "\\SU_LOGS"
#define SEARCHLOG_DIRECTORY_NAME "SU_LOGS"
#define SEARCHLOG_FOLDER "SU_LOGS"
static char searchlog_namebuf[TOTAL_FILE_SIZE + 3] = {0};	

int SD_OpenLogFile(int source_port)
{
	static unsigned long last_index = 0;
	g_search_logfd = NULL;
	g_lastSD_Debugport = source_port;
	unsigned char fileattributes = ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_ARCHIVE;
	SearchRec file;
	unsigned long ix;
	int ferror;


	traceS("Opening log file");
	if (MountMedia()) {
		if (MountMedia()) {		// if it fails once then try again
			return FSerror();
		}
	}	

    SetCurrentFileTime();

	if (FSchdir("\\") != 0)	{		// move to the root directory
	    traceS2("Chdir to root failed", FS_GetlasterrMsg());
	    traceSDErrors();
	}	    
	  
	  
    // Create a new directory if necessary
   	if (FSmkdir(SEARCHLOG_FOLDER)) {
		traceS("FSmkdir Error ");
	    traceS2(SEARCHLOG_FOLDER, FS_GetlasterrMsg());
	    traceSDErrors();
	   }	
   else {
   		g_SDProperties.MemFolders++;
	}   		
  
    
	// change to the existing or new directory relative to root
   	if (FSchdir(SEARCHLOG_FOLDER) != 0)	{		
		traceS("FSchdir Error ");
		traceS2(SEARCHLOG_FOLDER, FS_GetlasterrMsg());
		traceSDErrors();
	}

	if (last_index == 0) {
		ferror = FindFirst (SEARCHLOG_SEARCH_FORMAT, fileattributes, &file);
		while (ferror == 0) {
			ix = atol(&file.filename[SEARCHLOG_NAME_NUMBERIX]);  
			if (ix > last_index) 
				last_index = ix;
			ferror = FindNext (&file);
		}
	}
	last_index++;
	sprintf(searchlog_namebuf,SEARCHLOG_FORMAT,last_index);	// automatically increment index for next time
	
   	traceS2("open for writing:",searchlog_namebuf);
//  APPENDPLUS: the file being opened will
//              be created if it doesn't exist.  If it does exist, it's file information will be loaded and the
//              current location in the file will be set to the end.  The user will then be able to write to the file
//              or read from the file.
   	g_search_logfd = FSfopen( searchlog_namebuf, APPENDPLUS);

    SetCurrentFileTime();
	  
	PutVersion("Start", PORT_SDLOG);

 	return 0;   
} 


// writes footer and closes file
void SD_CloseLogFile(int source_port)
{
	g_lastSD_Debugport = source_port;
	if (g_search_logfd != NULL) {
		PutVersion("End", PORT_SDLOG);
	    if (FSfclose(g_search_logfd)) {
   			traceS2("Log file close failed:", FS_GetlasterrMsg());
   			traceSDErrors();
			g_SysError.SDCloseError = 1;	// set last SD errors
   		}
   		else {
   			traceS("Log file closed");
   			g_SDProperties.MemFiles++;
   			g_SysError.SDCloseError = 0;
   		}
 		DelayMsecs(20);
 		g_search_logfd = NULL;
	}	
	MDD_ShutdownMedia();
}


size_t SD_Write(char *pbuf,size_t len, int destport)
{	
	size_t r = 0;
	char msg[40];
	FSFILE* pfile = NULL;
	
	switch (destport) {
#ifdef PORT_SDLOG		
		case PORT_SDLOG:
			pfile = g_search_logfd;
			break;
#endif
			
#ifdef PORT_SDEVENTLOG
		case PORT_SDEVENTLOG:
			pfile = g_event_logfd;
			break;
#endif			
		default:
			break;	
	}

	if (pfile != NULL) {
//	sprintf(msgbuf,"Writing %d bytes to %s",r,fname);
//	traceS(msgbuf);
  	  r = FSfwrite( pbuf, 1, len, pfile);
  	  if (r!= len){
		g_SysError.SDError_write_error = 1;	// set last SD errors
	  }
	  if (g_UnitFlags.DebugSD) 
	  {
	  	  sprintf(msg,"%d bytes written",r);
    	  traceS(msg);
   	  } 	  
    } 	
    else {
//		g_SysError.SDError = 1;	// set last SD errors
	} 
   return r;  
}


	


int SD_RefreshProperties(int source_port)
{
	g_lastSD_Debugport = source_port;
	traceS("Getting Memory Properties");
	if (MountMedia()) return FSerror();

	DISK* pdisk = getDiskInfo();

	if (pdisk->mount) {
//    BYTE    *   buffer;         // Address of the global data buffer used to read and write file information
//    DWORD       firsts;         // Logical block address of the first sector of the FAT partition on the device
//    DWORD       fat;            // Logical block address of the FAT
//    DWORD       root;           // Logical block address of the root directory
//    DWORD       data;           // Logical block address of the data section of the device.
//    WORD        maxroot;        // The maximum number of entries in the root directory.
//    DWORD       maxcls;         // The maximum number of clusters in the partition.
//    WORD        fatsize;        // The number of sectors in the FAT
//    BYTE        fatcopy;        // The number of copies of the FAT in the partition
//    BYTE        SecPerClus;     // The number of sectors per cluster in the data region
//    BYTE        type;           // The file system type of the partition (FAT12, FAT16 or FAT32)
//    BYTE        mount;          // Device mount flag (TRUE if disk was mounted successfully, FALSE otherwise)
//long long g_MemUsed=0l;
//long long g_MemFolders=0l;
//long long g_MemFiles=0l;		
		g_SDProperties.MemType = pdisk->type;
		g_SDProperties.MemSectorCapacity = pdisk->maxcls;
		g_SDProperties.MemSectorCapacity *= pdisk->SecPerClus;
		g_SDProperties.MemBytesPerSector = pdisk->bytespersector;
		

		// Count the number of root folders - since we only build root folders
		SearchRec dir;
		SearchRec file;
		g_SDProperties.MemFolders=0l;
		g_SDProperties.MemFiles = 0l;
		g_SDProperties.MemSectorUsed = 0l;
		
		long clustersize = g_SDProperties.MemBytesPerSector * pdisk->SecPerClus;
		long clustersused;
		int ferror;
		if (clustersize == 0) clustersize = 512;			// prevent division by zero
		if (pdisk->SecPerClus == 0) pdisk->SecPerClus = 8;	// prevent division by zero
		
		unsigned char dirattributes = ATTR_HIDDEN | ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_VOLUME | ATTR_ARCHIVE | ATTR_DIRECTORY;
		unsigned char fileattributes = ATTR_HIDDEN | ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_VOLUME | ATTR_ARCHIVE;
		unsigned long long start_tick = g_tick;
		
		int direrror = 0;
		BOOL bFirstTime = TRUE;

		strcpy(dir.filename,"\\");  // begin with files in root directory
		while (direrror ==0) {
			FSchdir(dir.filename);
			g_SDProperties.MemFolders++;
			g_SDProperties.MemSectorUsed += pdisk->SecPerClus;	// assume 1 cluster per directory entry
			PROCESS_TASKS();						
			{
				ferror = FindFirst ("*.*", fileattributes, &file);
				while (ferror == 0) {
					g_SDProperties.MemFiles++;
					clustersused = file.filesize/clustersize;		// files are stored in at the cluster resolution
					if (file.filesize%clustersize) clustersused++;
					g_SDProperties.MemSectorUsed += clustersused * pdisk->SecPerClus;

					ferror = FindNext (&file);
					if ((g_tick - start_tick) > 250) {
						PROCESS_TASKS();						
						if ((g_tick - start_tick) > 500) {
							start_tick = g_tick;
							PutMemoryProperties(source_port);
						}	
						PROCESS_TASKS();						
					}	
				}
			}
			FSchdir("\\");	
			if (bFirstTime == TRUE) {
				direrror = FindFirst ("*", dirattributes, &dir);
				bFirstTime = FALSE;
			}	
			else {
				direrror = FindNext (&dir);  
			}	
		} 
	}		
	return 0;    	
}


long SD_CopyFiles(char *cmd , int source_port )
{
	long count=0;
	FSFILE * pointer;
	char SD_rd_data[ B_SIZE];
	size_t r;
	int i;
	char rxbyte=0;
	g_lastSD_Debugport = source_port;

	traceS("Getting Ready to transfer files");
	PROCESS_TASKS();						
	if (MountMedia()) return count;
	
	DISK* pdisk = getDiskInfo();

	if (pdisk->mount) {
		SearchRec dir;
		SearchRec file;
		unsigned char dirattributes = ATTR_HIDDEN | ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_VOLUME | ATTR_ARCHIVE | ATTR_DIRECTORY;
		unsigned char fileattributes = ATTR_HIDDEN | ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_VOLUME | ATTR_ARCHIVE;
		
		unsigned long long start_tick = g_tick;
		int ferror;
		int direrror = 0;
		BOOL bFirstTime = TRUE;
	
		// Limit the count to root files and root folders - since we only build root folders
		strcpy(dir.filename,"\\");  // begin with files in root directory
		while (direrror == 0) {
			PROCESS_TASKS();						
			FSchdir(dir.filename);
			ferror = FindFirst ("*.*", fileattributes, &file);
			while (ferror == 0) {
				PROCESS_TASKS();						
				PortDelayForBytes(source_port,40);	// make sure the UART port is ready to receive this many characters
				PutFileToCopy(count++, dir.filename, file.filename,source_port);
				PROCESS_TASKS();						
			    start_tick = g_tick;

				pointer = FSfopen (file.filename, "r");
				if (pointer == NULL) {
				  traceS2("Could not open file:", file.filename);
				  traceS(FS_GetlasterrMsg());
				}  
				else {
					traceS2(file.filename," opened for read");
					do{
						r = FSfread (SD_rd_data, 1, B_SIZE, pointer); 
						PortDelayForBytes(source_port,r);	// make sure the UART port is ready to receive this many characters
						for( i=0; i<r; i++)
					        PortPutChar( SD_rd_data[i],source_port);
					        
						if ((g_tick - start_tick) > 40) {	// make sure wirless is sending what it has
							PROCESS_TASKS();						
							start_tick = g_tick;
						}
						else {
							PROCESS_TASKS();						
						}
		    
				        rxbyte = PortGetCh(source_port); 
				        if (rxbyte == ESCAPE) break;		// break on any other command
					} while( r==B_SIZE);
					FSfclose (pointer);
				}	

		        if (rxbyte == ESCAPE) break;		// break on any other command
				ferror = FindNext (&file);
				
			    start_tick = g_tick;
			    while ((g_tick - start_tick) < 5000) {
			 		DelayMsecs(25);
					PROCESS_TASKS();						
				    rxbyte = PortGetCh(source_port); 
					if (rxbyte == ESCAPE) return count;		// break on escape
					if (PortGetTxBufSize(source_port) > 80)
						break;
				}		
			}
			FSchdir("\\");	
			if (bFirstTime == TRUE) {
				direrror = FindFirst ("*", dirattributes, &dir);
				bFirstTime = FALSE;
			}	
			else {
				direrror = FindNext (&dir);  
			}	
		}
	}	
	return count;	    	
}


long SD_EraseFile(char * cmnd, int source_port)
{
	long count=0;
	char rxbyte=0;
	g_lastSD_Debugport = source_port;
	
	if (strcmp(cmnd,"ALL") == 0) {	// currently we only support erasing all files

		traceS("Erasing all files");
		if (MountMedia()) return count;
		
		DISK* pdisk = getDiskInfo();
	
		if (pdisk->mount) 
		{
			FSchdir("..");
		   	if (FSchdir("\\") != 0)	
			{		// move to the root directory
			    traceS2("Chdir to root failed", FS_GetlasterrMsg());
		    	traceSDErrors();
			}
			// Count the number of root folders - since we only build root folders
					// Count the number of root folders - since we only build root folders
			SearchRec dir;
			SearchRec file;
			
			unsigned char fileattributes = ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_VOLUME | ATTR_ARCHIVE;
			int ferror = FindFirst ("*.*", fileattributes, &file);
			// delete all non-hidden files
			while (ferror == 0) 
			{
				PROCESS_TASKS();
				FSremove (file.filename);
				traceS2("Removing ",file.filename);
				PROCESS_TASKS();						
				ferror = FindNext (&file);
			}
		
			// Remove all root directories along with their contents
			unsigned char dirattributes = ATTR_HIDDEN | ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_VOLUME | ATTR_ARCHIVE | ATTR_DIRECTORY;
			int error = FindFirst ("*", dirattributes, &dir);
			while (error ==0) 
			{
				count++;
				traceS2("Removing ",dir.filename);
				PROCESS_TASKS();						
				if (FSrmdir (dir.filename, TRUE)) 
				{	// if remove directory and all subdirs failed, 
					traceS2("Remove directory failed: ", FS_GetlasterrMsg());
					PROCESS_TASKS();						
				}
				else 
				{	
					if (g_SDProperties.MemFolders)
						g_SDProperties.MemFolders--;
				}		
		        rxbyte = PortGetCh(source_port); 
				PutMemoryProperties(source_port);
		        if (rxbyte == ESCAPE) break;;		// break on escape
				PROCESS_TASKS();						
				error = FindNext (&dir);  
			}
		}	
	}		
	SD_RefreshProperties(source_port);
	return count;	    	
}	


long SD_CountLogFiles(int source_port)
{
	unsigned char fileattributes = ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_ARCHIVE;
	long count=0;
	SearchRec file;
	int ferror;
	static LogFileData fe;
	char* curpath;
	char * filepattern = SEARCHLOG_SEARCH_FORMAT;
	char * pdir =  SEARCHLOG_DIRECTORY_PATHNAME;

	traceS2("Finding ",filepattern);
	traceS2("in ",pdir);
	
	fe.id = 0;
	fe.filename[0] = 0;
	fe.size = 0l;
	fe.ftime = 0l;
	
	if (MountMedia()) return count;

	ferror = CE_GOOD;	// start with no errors in setting current directory
	curpath = FSgetcwd (NULL, 0);
	if ((curpath == NULL) || (strcmp(curpath,pdir) !=0)) {
		ferror = FSchdir(pdir);
	}
	if (ferror == CE_GOOD) {
		ferror = FindFirst (filepattern, fileattributes, &file);
		if (ferror) {
	    	traceS2("FindFirst: ", FS_GetlasterrMsg());
		}
		while (ferror == 0) {
			traceS2("Found ",file.filename);
			strncpy(fe.filename,file.filename,sizeof(fe.filename));
			fe.id = count;
			fe.size = file.filesize;
			fe.ftime = file.timestamp;
			count++;
			ferror = FindNext (&file);
		}
	}
	
	return count;	    	
}	


long SD_GetLastNFilenames(LogFileData* pf, int maxlen, int source_port)
{
	unsigned char fileattributes = ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_ARCHIVE;
	long count=0;
	SearchRec file;
	int ferror;
	static LogFileData fe;
	char* curpath;
	char * filepattern = SEARCHLOG_SEARCH_FORMAT;
	char * pdir =  SEARCHLOG_DIRECTORY_PATHNAME;
	int ix=0;
	long startcount;
	
	
	long filecount = SD_CountLogFiles(source_port);
	if (filecount > maxlen)
		startcount = filecount - maxlen;
	else
		startcount = 0;
	
	traceS2("Finding ",filepattern);
	traceS2("in ",pdir);
	
	fe.id = 0;
	fe.filename[0] = 0;
	fe.size = 0l;
	fe.ftime = 0l;
	
	if (MountMedia()) return count;

	ferror = CE_GOOD;	// start with no errors in setting current directory
	curpath = FSgetcwd (NULL, 0);
	if ((curpath == NULL) || (strcmp(curpath,pdir) !=0)) {
		ferror = FSchdir(pdir);
	}
	if (ferror == CE_GOOD) {
		ferror = FindFirst (filepattern, fileattributes, &file);
		if (ferror) {
	    	traceS2("FindFirst: ", FS_GetlasterrMsg());
		}
		while (ferror == 0) {
			traceS2("Found ",file.filename);
			strncpy(fe.filename,file.filename,sizeof(fe.filename));
			fe.id = count;
			fe.size = file.filesize;
			fe.ftime = file.timestamp;
			if ((count >= startcount) && (ix < maxlen)) {
				pf[ix++] = fe;
			}
			count++;
			ferror = FindNext (&file);
		}
	}
	
	return count;	    		
}

int SD_countfilelines(LogFileData fd, int source_port)
{
	FSFILE * pointer;
	char* curpath;
	unsigned char i;
	char SD_rd_data[ B_SIZE];
	size_t r;
	g_lastSD_Debugport = source_port;
	int count = 0;
	char * pdir =  SEARCHLOG_DIRECTORY_PATHNAME;
	int ferror;

	traceS("Count file lines");
	if (MountMedia()) return count;

	ferror = CE_GOOD;	// start with no errors in setting current directory
	curpath = FSgetcwd (NULL, 0);
	if ((curpath == NULL) || (strcmp(curpath,pdir) !=0)) {
		ferror = FSchdir(pdir);
	}
	if (ferror == CE_GOOD) {
		pointer = FSfopen (fd.filename, "r");
		if (pointer == NULL) {
		  traceS2("Could not open file:", FS_GetlasterrMsg());
	//	  traceSDErrors();
		  MDD_ShutdownMedia();
		  return count;
		}  
		do{
			r = FSfread (SD_rd_data, 1, B_SIZE, pointer); 
		    for( i=0; i<r; i++) {
		    	if (SD_rd_data[i] == '\n')
		    		count++;
		        traceC( SD_rd_data[i]);
		    }    
		} while( r==B_SIZE);
		FSfclose (pointer);
		traceS("\r\nfile closed");
		MDD_ShutdownMedia();
	}
	char buffer[30];
	sprintf(buffer,"Count = %d",count);
	traceS(buffer);
	return count;
}


int SD_getLogFileLines(LogFileData fd, char pbuf[FILE_MAX_LINES_PER_PAGE][MAX_DISPLAY_LINE_WIDTH], int startline, int endline, int source_port)
{
	FSFILE * pointer;
	char* curpath;
	unsigned char i;
	char SD_rd_data[ B_SIZE];
	size_t r;
	g_lastSD_Debugport = source_port;
	int rowpos;
	char * pdir =  SEARCHLOG_DIRECTORY_PATHNAME;
	int ferror;
	int ix=0;
	char rdbyte;
	int rows;
	
	// begin by null-terminating all lines
	for (rows=0;rows < FILE_MAX_LINES_PER_PAGE; rows++) {
		pbuf[rows][0] = 0;	
	}
	rows = 0;
	
	char buffer[40];
	sprintf(buffer,"Fetch file lines %d to %d",startline,endline);
	traceS(buffer);

	if (MountMedia()) return rows;

	ferror = CE_GOOD;	// start with no errors in setting current directory
	curpath = FSgetcwd (NULL, 0);
	if ((curpath == NULL) || (strcmp(curpath,pdir) !=0)) {
		ferror = FSchdir(pdir);
	}
	if (ferror == CE_GOOD) {
		pointer = FSfopen (fd.filename, "r");
		if (pointer == NULL) {
		  traceS2("Could not open file:", FS_GetlasterrMsg());
	//	  traceSDErrors();
		  MDD_ShutdownMedia();
		  return rows;
		}  
		
		ix = 0;
		rows = 0;
		rowpos = 0;
		do{
			r = FSfread (SD_rd_data, 1, B_SIZE, pointer); 
		    for( i=0; i<r; i++) {
			    rdbyte = SD_rd_data[i];
		        if ((rowpos >= startline) && (rowpos < endline)) {
			        if (rdbyte == '\n') {
	    				rows++;
	    				ix = 0;
				        if (FILE_MAX_LINES_PER_PAGE == rows)
			    			break;
				    }
				    else if ((rdbyte != '\r') && (ix < MAX_DISPLAY_LINE_WIDTH-2)) {
			        	pbuf[rows][ix++] = rdbyte;
			        	pbuf[rows][ix] = 0;
			        }
			    }
		    	if (rdbyte == '\n') {
		    		rowpos++;  		
		    	}
		        traceC( rdbyte);
		    }
	        if (FILE_MAX_LINES_PER_PAGE == rows)
    			break;
		} while( r==B_SIZE);
		
		FSfclose (pointer);
		traceS("\r\nfile closed");
		MDD_ShutdownMedia();
	}
	sprintf(buffer,"Rows = %d",rows);
	traceS(buffer);
	return rows;
}



