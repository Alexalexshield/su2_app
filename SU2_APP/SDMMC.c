/*
** SD/MMC card interface 
** 
*/

#include "system.h"
#include "sdmmc.h"

// SD card commands
#define RESET                       0 // a.k.a. GO_IDLE (CMD0)
#define INIT                        1 // a.k.a. SEND_OP_COND (CMD1)
#define SEND_CSD                    9
#define SEND_CID                    10
#define SET_BLEN                    16
#define READ_SINGLE                 17
#define WRITE_SINGLE                24
#define APP_CMD                     55
#define SEND_APP_OP                 41

// SD card responses
#define DATA_START                  0xFE
#define DATA_ACCEPT                 0x05


void initSD( void)
{
    SD_CD_TRIS = INPUT;            //Card Detect - input
    SD_CS_DESELECT();              //Initialize Chip Select line
    SD_CS_TRIS = OUTPUT;           //Card Select - output
    SD_WE_TRIS = INPUT;            //Write Protect - input

//    SDCS = 1;
//    _TRISF0 = 0;        // make Card select an output pin
    
    // init the spi module for a slow (safe) clock speed first
    SD_SPICON1 = 0x013c;  // CKE=1; CKP=0, sample middle, prescale 1:64

    SD_SPISTAT = 0x8000;  // enable the peripheral
}   // initSD


// send one byte of data and receive one back at the same time
unsigned char writeSPI( unsigned char b)
{
    SD_SPIBUF = b;					// write to buffer for TX
    while( !SD_SPISTATbits.SPIRBF);	// wait for transfer to complete
    return SD_SPIBUF;    				// read the received value
}// writeSPI

#define readSPI()   writeSPI( 0xFF)
#define clockSPI()  writeSPI( 0xFF)
#define disableSD() SD_CS_DESELECT(); clockSPI()
#define enableSD()  SD_CS_SELECT();


int detectSD( void)
{
    return ( !SD_CD);  
} // detect SD

int detectWP( void)
{
	return FALSE;	// NO WRITE PRODECT ON mini cards
//    return ( !SDWP);  
} // detect WP


int sendSDCmd( unsigned char c, LBA a)
{
    int i, r;

    // enable SD card
    enableSD();

    // send a comand packet (6 bytes)
    writeSPI( c | 0x40);    // send command 
    writeSPI( a>>24);       // msb of the address
    writeSPI( a>>16);       
    writeSPI( a>>8);
    writeSPI( a);           // lsb
    
    writeSPI( 0x95);        // send CMD0 CRC 

    // now wait for a response (allow for up to 8 bytes delay)
    i = 9;
    do {
        r = readSPI();      // check if ready   
        if ( r != 0xFF) break;
    } while ( --i > 0);

    return ( r);         

/* return response
    FF - timeout 
    00 - command accepted
    01 - command received, card in idle state after RESET

other codes:
    bit 0 = Idle state
    bit 1 = Erase Reset
    bit 2 = Illegal command
    bit 3 = Communication CRC error
    bit 4 = Erase sequence error
    bit 5 = Address error
    bit 6 = Parameter error
    bit 7 = Always 0
*/
    // NOTE CSCD is still low
} // sendSDCmd


int readSECTOR( LBA a, char *p)
// a        LBA of sector requested
// p        pointer to sector buffer
// returns  TRUE if successful
{
    int r, i;
    
//    READ_LED = 1;

    r = sendSDCmd( READ_SINGLE, ( a << 9));
    if ( r == 0)    // check if command was accepted
    {  
        // wait for a response
        i = 10000;
        do{
            r = readSPI();     
            if ( r == DATA_START) break;
        }while( --i>0);

        // if it did not timeout, read a 512 byte sector of data - defined in SECTOR_SIZE
        if ( i)
        {
        register unsigned i asm( "w5");
        register char * q asm("w6");
        q = p;
            i = SECTOR_SIZE;
            do{ 
                SD_SPIBUF = 0xFF;					// write to buffer for TX
                while( !(SD_SPISTAT&1));	// wait for transfer to complete
                *q++ = SD_SPIBUF;    				// read the received value
            } while (--i>0);

            // ignore CRC
            readSPI();
            readSPI();

        } // data arrived

    } // command accepted

    // remember to disable the card
    disableSD();
//    READ_LED = 0;

    return ( r == DATA_START);      // return TRUE if successful
} // readSECTOR


int writeSECTOR( LBA a, char *p)
// a        LBA of sector requested
// p        pointer to sector buffer
// returns  TRUE if successful
{
    unsigned r, i;
    
//    WRITE_LED = 1;

    r = sendSDCmd( WRITE_SINGLE, ( a << 9));
    if ( r == 0)    // check if command was accepted
    {  
        writeSPI( DATA_START);
        for( i=0; i<SECTOR_SIZE; i++)
            writeSPI( *p++);

        // send dummy CRC
        clockSPI();
        clockSPI();
    
        // check if data accepted
        if ( (r = readSPI() & 0xf) == DATA_ACCEPT)
        {   
            for( i=0xffff; i>0; i--)
            { // wait for end of write
                if ( (r = readSPI()))
                    break;
            } 
        } // accepted
        else
            r = FALSE;

    } // command accepted

    // remember to disable the card
    disableSD();
//    WRITE_LED = 0;

    return ( r);      // return TRUE if successful

} // writeSECTOR



int initMedia( void)
{
    int i, r;

    // 1. with the card not selected     
    disableSD(); 

    // 2. send 80 clock cycles start up
    for ( i=0; i<10; i++)
        clockSPI();

    // 3. now select the card
    enableSD();

    // 4. send a reset command
    r = sendSDCmd( RESET, 0); disableSD();
    if ( r != 1)  
        return 0x84;

    // 5. send repeatedly INIT until receive a 0 
    i = 10000;  // up to .3 sec
    do {
        r = sendSDCmd( INIT, 0); disableSD();
        if ( !r) break; 
    } while( --i > 0);
    if ( i==0)   
        return 0x85;       // timed out 
    
    // 10. increase speed 
    SD_SPISTAT = 0;           // disable momentarily the SPI2 module
    SD_SPICON1 = 0x013b;      // change prescaler to 1:2
    SD_SPISTAT = 0x8000;      // re-enable the SPI2 module
    
    return 0;           

} // init media
