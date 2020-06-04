/**********************************************************************

**********************************************************************/
#ifndef __I2C_RTC_H__
#define __I2C_RTC_H__ 
#include "time.h"

//The first four bits of the Slave Address Byte
//specify access to either the EEPROM array or to the CCR.
//Slave bits ‘1010’ access the EEPROM array. Slave bits
//‘1101’ access the CCR.
// Bit 3 through Bit 1 of the slave byte specify the device select bits. These are set to ‘111’.
// The last bit of the Slave Address Byte defines the operation
// to be performed. When this R/W bit is a one, then a read
// operation is selected. A zero selects a write operation
#define ISL_CCR_REGISTERS 0xDE
#define ISL_EEPROM_REGISTERS 0xC0
#define ISL_RTC_READ 1
#define ISL_RTC_WRITE 0

void I2C_Start();
void I2C_Stop();
void I2C_WaitForReady();
char I2C_SelectRTCRegisters(unsigned char device, unsigned char mode);
char I2C_SelectAddress(unsigned int addr);

#define STATUS_REGISTER_ADDRESS 0x003f
#define ISL_ENABLE_WRITES 0x02
#define ISL_DISABLE_WRITES 0x00

#define ISL_TIME_ADDRESS 0x30
#define ISL_TIME_BYTES 8

#define ISL_POWER_REGISTER_ADDRESS 0x0014


typedef struct ISL_TIME
{
	unsigned char SC;		// 0 TO 59
	unsigned char MN;		// 0 TO 59
	unsigned char HR;		// 0 TO 23
	unsigned char DT;		// 1 TO 31
	unsigned char MO;		// 1-12
	unsigned char YR;		// 0 TO 99
	unsigned char DW;		// DEFAULT VALUE IS 0; CYCLES FROM 0 TO 6 AND BACK TO 0
	unsigned char Y2K;		// set to 0x20 for year 2000 and 0x19 for year 1900
	
} ISLTime;


// calculate baud rate of I2C based on desired speed as specified in I2C_FSCK
#define I2C_FSCKK	400000ul
#define   I2C_BRG(freq)    ((FCY/freq)-(FCY/1111111))-1 

#define ISL_I2C_BRG 37
 

void I2C_Init( unsigned long brate);
char ISL_RTC_Writetime_t_Time(time_t seconds);
unsigned char DecToBCD(int dec);
unsigned char BCDToDec(unsigned char bcd);
char ISL_RTC_WriteTMTime(struct tm* tminfo);
int ISL_ReadTime(volatile time_t* p);

void ISL_Init();

#endif // __I2C_RTC_H__
        
