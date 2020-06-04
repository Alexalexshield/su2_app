/**********************************************************************
**********************************************************************/
#include "system.h"
#include "i2c_ISL12024_rtc.h"
#include "trace.h"


char ISL_RTC_CCR_PageRead( unsigned char * pbuf,int len, unsigned int addr);

unsigned int MasterputaI2C1(unsigned char * wrptr, int len);
char MasterWriteI2C1(unsigned char data_out);
void IdleI2C1(void);
unsigned int MastergetsI2C1(unsigned int length, unsigned char * rdptr, unsigned int i2c1_data_wait);
void RestartI2C1(void);
void StopI2C1(void);
void StartI2C1(void);
char DataRdyI2C1(void);


void i2c_wait(unsigned int cnt)
{
	while(--cnt)
	{
		asm( "nop" );
		asm( "nop" );
	}
}

/************************************************************************
*    Function Name:  IdleI2C	
*    Description:    This routine generates wait condition intil I2C 
*                    bus is Idle.					 
*    Parameters:     void
*    Return Value:   void
*************************************************************************/

void IdleI2C1(void)
{
    /* Wait until I2C Bus is Inactive */
   	i2c_wait(PROCESSOR_MIPS);
    while(I2C1CONbits.SEN || I2C1CONbits.PEN || I2C1CONbits.RCEN || 
          I2C1CONbits.ACKEN || I2C1STATbits.TRSTAT);	
}


/***********************************************************************
*    Function Name:  MasterputsI2C
*    Description:    This routine is used to write out an array to the 
*                    I2C bus.If write collision occurs,-1 is sent.If 
*                    Nack is received, -2 is sent.If string is written 
*                    and null char reached, 0 is returned.
*    Parameters:     unsigned char * : wrptr
*    Return Value:   unsigned int 
************************************************************************/


unsigned int MasterputaI2C1(unsigned char * wrptr, int len)
{
    while(len--)                           //transmit data until lengh reached
    {
        if(MasterWriteI2C1(*wrptr) == -1)	{    // write a byte
//	        traceS("I2C_RTC Write collision");
        	return -1;                          //return with write collison error
		}
		
        IdleI2C1();							// wait for the I2C bus to be idle
    
        wrptr++;
   		if( I2C1STATbits.ACKSTAT ) {
//	        traceS("I2C_RTC Nack on write");
			return -2;
		}
    }
    return 0;			
}



/************************************************************************
*    Function Name:  MasterWriteI2C1
*    Description:    This routine is used to write a byte to the I2C bus.
*                    The input parameter data_out is written to the 
*                    I2CTRN register. If IWCOL bit is set,write collision 
*                    has occured and -1 is returned, else 0 is returned.
*    Parameters:     unsigned char : data_out
*    Return Value:   unsigned int
*************************************************************************/
char MasterWriteI2C1(unsigned char data_out)
{
    I2C1TRN = data_out;

    if(I2C1STATbits.IWCOL)        /* If write collision occurs,return -1 */
        return -1;
    else
    {
        return 0;
    }
}

/*********************************************************************
*    Function Name:  StartI2C1
*    Description:    This routine generates Start condition 
*                    during master mode.
*    Parameters:     void
*    Return Value:   void
*********************************************************************/
void StartI2C1(void)
{
     I2C1CONbits.SEN = 1;	/* initiate Start on SDA and SCL pins */
}

/*********************************************************************
*    Function Name:  StopI2C
*    Description:    This routine generates Stop condition 
*                    during master mode.
*    Parameters:     void
*    Return Value:   void
*********************************************************************/
void StopI2C1(void)
{
     I2C1CONbits.PEN = 1;	/* initiate Stop on SDA and SCL pins */
}

/*********************************************************************
*    Function Name:  RestartI2C1
*    Description:    This routine generates Restart condition 
*                    during master mode.
*    Parameters:     void    
*    Return Value:   void
*********************************************************************/
void RestartI2C1(void)
{ 
    I2C1CONbits.RSEN = 1;	/* initiate restart on SDA and SCL pins	*/
}

/************************************************************************
*    Function Name:  MastergetsI2C
*    Description:    This routine reads predetermined data string length
*                    from the I2C bus.
*    Parameters:     unsigned int    : length
*                    unsigned char * : rdptr
*    Return Value:   unsigned int
*************************************************************************/
unsigned int MastergetsI2C1(unsigned int length, unsigned char * rdptr, 
                            unsigned int i2c1_data_wait)
{
    int wait = 0;
    while(length)                    /* Receive length bytes */
    {
        I2C1CONbits.RCEN = 1;
        while(!DataRdyI2C1())
        {
            if(wait < i2c1_data_wait)
                wait++ ;                 
            else
            	return(length);          /* Time out, 
                                        return number of byte/word to be read */
        }
        wait = 0;
        *rdptr = I2C1RCV;            /* save byte received */

        rdptr++;
        length--;
        if(length == 0)              /* If last char, generate NACK sequence */
        {
            I2C1CONbits.ACKDT = 1;
            I2C1CONbits.ACKEN = 1;
        }
        else                         /* For other chars,generate ACK sequence */
        {
            I2C1CONbits.ACKDT = 0;
            I2C1CONbits.ACKEN = 1;
        }
            while(I2C1CONbits.ACKEN == 1);    /* Wait till ACK/NACK sequence 
                                                 is over */
    }
    /* return status that number of bytes specified by length was received */
    return 0;
}

/************************************************************************
*    Function Name:  DataRdyI2C1	
*    Description:    This routine provides the status whether the receive 
*                    buffer is full by returning the RBF bit.
*    Parameters:     void
*    Return Value:   RBF bit status
*************************************************************************/
char DataRdyI2C1(void)
{
     return I2C1STATbits.RBF;
}


/*=============================================================================
I2C Peripheral Initialisation
=============================================================================*/   

//#define TEST_IO_PINS

void I2C_Init(unsigned long brate)
{   
// Configre SCL/SDA pin as open-drain
	ODCGbits.ODG2=1;
	ODCGbits.ODG3=1;

	I2C1CONbits.I2CEN=0;	// Disable the I2Cx module 
#ifdef TEST_IO_PINS
	_TRISG2 = 0;
	int i;
	for (i=0;i<10;i++)			// TOGGLE SCL LINE
	{
		_LATG2 = 1;		
		DelayMsecs(1);
		_LATG2 = 0;
		DelayMsecs(1);
	}
#endif	
	I2C1CONbits.A10M=0;		// 7-bit slave address mode
//	brate = I2C_BRG(brate);
//	I2C1BRG = brate & 0xffff;
	I2C1BRG = ISL_I2C_BRG;
	
	I2C1ADD=0;				// Set slave address to 0
	I2C1MSK=0;				// Disable masking for bit x; bit match required in this position

	I2C1CONbits.I2CEN=1;	// Enables the I2Cx module and configures the SDAx and SCLx pins as serial port pins
		
}



// 
char ISL_RTC_CCR_PageRead( unsigned char * pbuf,int len, unsigned int addr)
{
	char status;
	unsigned char i2cData[10];

	I2C_Init(I2C_FSCKK);

	// Start a block read by writing start address
	i2cData[0] = ISL_CCR_REGISTERS + ISL_RTC_WRITE;	 
	i2cData[1] = (addr >> 8) & 0x00ff;		//RTC high address byte
	i2cData[2] = addr & 0x00ff;				//RTC low address byte

	StopI2C1();	//Send the Stop condition	- make sure the bus is idle	
	IdleI2C1();	//Wait to complete

	StartI2C1();	//Send the Start Bit
	IdleI2C1();		//Wait to complete
	status = MasterputaI2C1(i2cData,1);		// send the device address in write mode

	status = MasterputaI2C1(&i2cData[1],2);	// send the address from which to read from

	RestartI2C1();	//Send the Restart condition
	//wait for this bit to go back to zero
	IdleI2C1();	//Wait to complete

	status = MasterWriteI2C1( ISL_CCR_REGISTERS + ISL_RTC_READ); //transmit read command
	IdleI2C1();		//Wait to complete

	// read some bytes back
	status = MastergetsI2C1(len, pbuf, 200);
	
	StopI2C1();	//Send the Stop condition
	IdleI2C1();	//Wait to complete


	return status;
}	


char ISL_RTC_CCR_ByteRead( unsigned char * pbyte, unsigned int addr)
{
	char status;
	unsigned char i2cData[10];

	I2C_Init(I2C_FSCKK);

	// Start a block read by writing start address
	i2cData[0] = ISL_CCR_REGISTERS + ISL_RTC_WRITE;	 
	i2cData[1] = (addr >> 8) & 0x00ff;		//RTC high address byte
	i2cData[2] = addr & 0x00ff;				//RTC low address byte

	StopI2C1();	//Send the Stop condition	- make sure the bus is idle	
	IdleI2C1();	//Wait to complete

	StartI2C1();	//Send the Start Bit
	IdleI2C1();		//Wait to complete
	status = MasterputaI2C1(i2cData,1);		// send the device address in write mode

	status = MasterputaI2C1(&i2cData[1],2);	// send the address from which to read from

	RestartI2C1();	//Send the Restart condition
	//wait for this bit to go back to zero
	IdleI2C1();	//Wait to complete

	status = MasterWriteI2C1( ISL_CCR_REGISTERS + ISL_RTC_READ); //transmit read command
	IdleI2C1();		//Wait to complete

	// read some bytes back
	status = MastergetsI2C1(1, pbyte, 200);
	
	StopI2C1();	//Send the Stop condition
	IdleI2C1();	//Wait to complete


	return status;
}	


unsigned char DecToBCD(int dec)
{
    return ((dec/10 * 16) + dec%10);	
}	

unsigned char BCDToDec(unsigned char bcd)
{
    return ((bcd/16 * 10) + bcd%16);	
}

// struct tm 
//  int tm_sec;  /* seconds, 0-60 */
//  int tm_min;  /* minutes, 0-59 */
//  int tm_hour; /* hours, 0-23 */
//  int tm_mday; /* day of the month, 1-31 */
//  int tm_mon;  /* months since Jan, 0-11 */
//  int tm_year; /* years from 1900 */
//  int tm_wday; /* days since Sunday, 0-6 */
//  int tm_yday; /* days since Jan 1, 0-365 */
//  int tm_isdst /* Daylight Saving Time indicator */

// convert from tm struct to ISLTime
void tm_to_ISLTime(struct tm* tminfo, ISLTime* p)
{
	p->SC = min(tminfo->tm_sec,59);		// 0 TO 59	
	p->MN = tminfo->tm_min;				// 0 TO 59
	p->HR = tminfo->tm_hour;				// 0 TO 23
	p->DT = tminfo->tm_mday;				// 1 TO 31
	p->MO = tminfo->tm_mon+1;				// 1-12
	p->YR = tminfo->tm_year-100;			// 0 TO 99	
	p->DW = 0;	// DEFAULT VALUE IS 0; CYCLES FROM 0 TO 6 AND BACK TO 0
	p->Y2K = 20;		// set to 20 for year 2000 

}

// convert from ISLTime TO tm struct 
void ISLTime_to_tm( ISLTime* p, struct tm* tminfo)
{
	tminfo->tm_sec = p->SC;				// 0 TO 59	
	tminfo->tm_min = p->MN;				// 0 TO 59
	tminfo->tm_hour = p->HR;				// 0 TO 23
	tminfo->tm_mday = p->DT;				// day of the month, 1-31
	tminfo->tm_mon = p->MO-1;				// months since Jan, 0-11
	tminfo->tm_year = p->YR + 100;		// years from 1900	
	tminfo->tm_wday = p->DW;		// days since Sunday, 0-6
	tminfo->tm_isdst = 0;
}


char ISL_WriteRegisterByte(unsigned int address, unsigned char value)
{
	char status;
	unsigned char tx_data[10];
	I2C_Init(I2C_FSCKK);
	
	tx_data[0] = ISL_CCR_REGISTERS + ISL_RTC_WRITE;			// Device Address & write
	tx_data[1] = (address >> 8) & 0x00ff;	//RTC high address byte
	tx_data[2] = address & 0x00ff;			//RTC low address byte
	tx_data[3] = value;
	
	StopI2C1();	//Send the Stop condition to make sure the bus is idle
	IdleI2C1();	//Wait to complete

	StartI2C1();	//Send the Start Bit
	IdleI2C1();		//Wait to complete

	status = MasterputaI2C1(tx_data,4);

	StopI2C1();	//Send the Stop condition
	IdleI2C1();	//Wait to complete
	return status;	
}


// IMPORTANT NOTE: MAKE SURE WRITES ARE ENABLED BEFORE CALLING THIS FUNCTION
// make sure 	I2C_Init(I2C_FSCKK) is called before this function
char ISL_WriteTime(ISLTime* p)
{
	char status;
	unsigned char tx_data[12];

	tx_data[0] = ISL_CCR_REGISTERS + ISL_RTC_WRITE;	// Device Address & WRITE
	tx_data[1] = (ISL_TIME_ADDRESS >> 8) & 0x00ff;	//RTC high address byte
	tx_data[2] = ISL_TIME_ADDRESS & 0x00ff;			//RTC low address byte

	// pack binary date in BCD format
	tx_data[3] = DecToBCD(p->SC);			// // 0 TO 59
	tx_data[4] = DecToBCD(p->MN);			// 0 TO 59
	tx_data[5] = DecToBCD(p->HR) | 0x80;	// 0 TO 23 set the MIL bit to use 24-hour time
	tx_data[6] = DecToBCD(p->DT);		// 1 TO 31
	tx_data[7] = DecToBCD(p->MO);		// 1-12
	tx_data[8] = DecToBCD(p->YR);		// 0 TO 99
	tx_data[9] = DecToBCD(p->DW);		// DEFAULT VALUE IS 0; CYCLES FROM 0 TO 6 AND BACK TO 0
	tx_data[10] = DecToBCD(p->Y2K);		// set to 0x20 for year 2000 and 0x19 for year 1900

       
	StopI2C1();	//Send the Stop condition to make sure the bus is idle
	IdleI2C1();	//Wait to complete

	StartI2C1();	//Send the Start Bit
	IdleI2C1();		//Wait to complete

	status = MasterputaI2C1(tx_data, 3 + ISL_TIME_BYTES);
	
	StopI2C1();	//Send the Stop condition
	IdleI2C1();	//Wait to complete
	return status;
}

char ISL_RTC_WriteTMTime(struct tm* tminfo)
{
	char status=0;
	ISLTime isl_tim;

	I2C_Init(I2C_FSCKK);		// initialize I2C hardware

	ISL_WriteRegisterByte(STATUS_REGISTER_ADDRESS, ISL_ENABLE_WRITES);		// unlock clock to enable writes
	ISL_WriteRegisterByte(STATUS_REGISTER_ADDRESS, 0x06);					// unlock clock to enable writes
	
	tm_to_ISLTime(tminfo, &isl_tim);				// convert to ISL FORMAt
	status = ISL_WriteTime(&isl_tim);

	ISL_WriteRegisterByte(STATUS_REGISTER_ADDRESS, ISL_DISABLE_WRITES);	// lock clock to disable accidental writes


	return status;
}



char ISL_RTC_Writetime_t_Time(time_t seconds)
{
 	struct tm *tminfo;
  	tminfo = gmtime ( &seconds );
  	return ISL_RTC_WriteTMTime(tminfo);
}


// reads the time and returns it in time_t format
time_t ISL_RTC_ReadTime()
{
	unsigned char clockbuf[ISL_TIME_BYTES];
	unsigned char status=0;
	ISLTime isl_tim;
    struct tm t;
    time_t t_of_day = 0;
    char error;
    int i;
    
    error = ISL_RTC_CCR_PageRead( &status, 1, STATUS_REGISTER_ADDRESS);

    
	for (i=0;i<10;i++) 			// try to read it up to 10 times if it fails
	{
		error = ISL_RTC_CCR_PageRead( clockbuf, ISL_TIME_BYTES, ISL_TIME_ADDRESS);
		if (error == 0)		// no errors so stop trying to read it
			  	break;   
	}

	if (error==0) 
	{
		// unpack from BCD to Decimal
	    isl_tim.SC = BCDToDec(clockbuf[0]&0x7f);   
	    isl_tim.MN = BCDToDec(clockbuf[1]&0x7f);
	    isl_tim.HR = BCDToDec(clockbuf[2]&0x3f);
	    isl_tim.DT = BCDToDec(clockbuf[3]&0x3f);
	    isl_tim.MO = BCDToDec(clockbuf[4]&0x1f);
	    isl_tim.YR = BCDToDec(clockbuf[5]);
	    isl_tim.DW = BCDToDec(clockbuf[6]&0x07);
	    isl_tim.Y2K = BCDToDec(clockbuf[7]);
	    ISLTime_to_tm(&isl_tim,&t);
	    t_of_day = mktime(&t);    
//		traceS("Read 8 RTC bytes ok");
	}  
	else
	{
//		traceHex(error);
//		traceS(" out of 8 RTC bytes not read");
	}  

	return t_of_day;
}


int ISL_ReadTime(volatile time_t* p)
{
	time_t now = ISL_RTC_ReadTime();	
	if (now > 0) 
	{
		*p = now;
		return 1;
	}
	return 0;
}


// INT IM AL1E AL0E FO1 FO0 0 0 0
// 
#define DEFAULT_ICR_1HZ 0x18
#define ICR_REGISTER 0x11


void ISL_Init()
{
	char error;
	char status = 0xff;
	
	I2C_Init(I2C_FSCKK);		// initialize I2C hardware
	
    error = ISL_RTC_CCR_PageRead( &status, 1, ISL_POWER_REGISTER_ADDRESS);

	ISL_WriteRegisterByte(STATUS_REGISTER_ADDRESS, ISL_ENABLE_WRITES);		// unlock clock to enable writes
	ISL_WriteRegisterByte(STATUS_REGISTER_ADDRESS, 0x06);					// unlock clock to enable writes
	
//	if ((error) || (status != 0))
		ISL_WriteRegisterByte(ISL_POWER_REGISTER_ADDRESS, 0);	// Clear BSW bit for standard mode and enable serial interface
	
	ISL_WriteRegisterByte(ICR_REGISTER, DEFAULT_ICR_1HZ);	// enable 1 hz output - this is not used yet

	ISL_WriteRegisterByte(STATUS_REGISTER_ADDRESS, ISL_DISABLE_WRITES);	// lock clock to disable accidental writes
	
}

