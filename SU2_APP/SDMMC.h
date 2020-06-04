#ifndef __SDMMC_H
#define __SDMMC_H


// IO definitions for the SD card
//#define SD_LED                      _RA0
//#define READ_LED                    _RA1
//#define WRITE_LED                   _RA2

#define SD_SPICON1             SPI1CON1  		// The main SPI control register
#define SD_SPISTAT             SPI1STAT 		// The SPI status register
#define SD_SPIBUF              SPI1BUF  		// The SPI Buffer
#define SD_SPISTAT_RBF         SPI1STATbits.SPIRBF	// The receive buffer full bit in the SPI status register
#define SD_SPICON1bits         SPI1CON1bits	// The bitwise define for the SPI control register (i.e. _____bits)
#define SD_SPISTATbits         SPI1STATbits	// The bitwise define for the SPI status register (i.e. _____bits)
#define SD_SPIENABLE           SPI1STATbits.SPIEN	// The enable bit for the SPI module

//#define SDPOWER_TRIS		TRISGbits.TRISG6
//#define SDPOWER				(PORTGbits.RG6) 
//#define SDPOWER_LATCH		(_LATG6) 	

#define SD_SPIIN           TRISBbits.TRISB14  	// The TRIS bit for the SDI pin
#define SD_SPIOUT          TRISFbits.TRISF5		// The TRIS bit for the SDO pin
#define SD_SPICLOCK        TRISBbits.TRISB15   	// The TRIS bit for the SCK pin
//#define SD_CS            (PORTFbits.RF0)		// Description: SD-SPI Chip Select Output bit
#define SD_CS_SELECT()     (_LATB11 = 0)		// Description: SD-SPI Chip Select Output bit
#define SD_CS_DESELECT()   (_LATB11 = 1)		// Description: SD-SPI Chip Select Output bit
#define SD_CS_TRIS         TRISBbits.TRISB11	// Description: SD-SPI Chip Select TRIS bit

#define SD_CD               (PORTFbits.RF4)			// Description: SD-SPI Card Detect Input bit
#define SD_CD_TRIS          TRISFbits.TRISF4	// Description: SD-SPI Card Detect TRIS bit
//#define SD_CD_LATCH        (_LATF4)			// Description: SD-SPI Card Detect Input bit
//#define SD_CD               (PORTBbits.RB13)			// Description: SD-SPI Card Detect Input bit
//#define SD_CD_TRIS          TRISBbits.TRISB13	// Description: SD-SPI Card Detect TRIS bit

#define SD_WE              SD_CD				// Description: SD-SPI Write Protect Check Input bit
#define SD_WE_LATCH        (SD_CD_LATCH)		// Description: SD-SPI Write Protect Check Input bit
#define SD_WE_TRIS         SD_CD_TRIS	// Description: SD-SPI Write Protect Check TRIS bit

// SD card activity LED - these are currently blank but could be filled in with ports
#define SD_ACT_TRIS 0
#define SD_READ_LED(value)     (;)
#define SD_WRITE_LED(value)    (;)


typedef unsigned long LBA;      // logic block address, 32 bit wide

#define SECTOR_SIZE             512    					// sector/data block size

void initSD( void);     // initializes the I/O pins for the SD/MMC interface 
int initMedia( void);   // initializes the SD/MMC memory device

int detectSD( void);    // detects the card presence 
int detectWP( void);    // detects the position of the write protect switch

int readSECTOR ( LBA, char *);  // reads a block of data 
int writeSECTOR( LBA, char *);  // writes a block of data

#endif // __SDMMC_H
