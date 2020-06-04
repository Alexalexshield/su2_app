/*****************************************************************************
 *
 * UART2 Driver functions
 *
 *
 *****************************************************************************/
#include "system.h"
#include "uart2.h"
#include "putfunctions.h"

// UART2 IOs
#ifdef SU_APP
#define UART2_TX_TRIS   TRISDbits.TRISD4
#define UART2_RX_TRIS   TRISDbits.TRISD5
#define TX2FIFOSIZE 1024	// to LCD_PORT
#define RX2FIFOSIZE 512	// from LCD_PORT 
// comment out next line to use poll-driven RX or uncomment to use int-driven RX
//#define USEINTDRIVENTX 1
#define USEINTDRIVENRX 1
#endif

#ifdef SU_TX
#define UART2_TX_TRIS   TRISDbits.TRISD9
#define UART2_RX_TRIS   TRISDbits.TRISD8
#define TX2FIFOSIZE 512	// to su
#define RX2FIFOSIZE 512	// from su 
// comment out next line to use poll-driven RX or uncomment to use int-driven RX
//#define USEINTDRIVENTX 1
#define USEINTDRIVENRX 1
#endif

#ifdef SU_RX
#define UART2_TX_TRIS   TRISDbits.TRISD9
#define UART2_RX_TRIS   TRISDbits.TRISD8
#define TX2FIFOSIZE 512	// to su
#define RX2FIFOSIZE 1024	// from su 
// comment out next line to use poll-driven RX or uncomment to use int-driven RX
//#define USEINTDRIVENTX 1
#define USEINTDRIVENRX 1
#endif

#ifdef MU_APP
#define UART2_TX_TRIS   TRISBbits.TRISB4
#define UART2_RX_TRIS   TRISBbits.TRISB13
#define TX2FIFOSIZE 1024	// to su
#define RX2FIFOSIZE 1024	// from su 
// comment out next line to use poll-driven RX or uncomment to use int-driven RX
#define USEINTDRIVENTX 1
#define USEINTDRIVENRX 1
#endif


#ifdef USEINTDRIVENTX
#include "queue.h"

unsigned char tx2_fifo[TX2FIFOSIZE];
struct CQUEUE txq2;
#endif

#ifdef USEINTDRIVENRX
#include "queue.h"

unsigned char rx2_fifo[RX2FIFOSIZE];
struct CQUEUE rxq2;
#endif

// Initialize the Uart
void UART2Init(unsigned char brgval,unsigned char bregh)
{
	U2MODEbits.UARTEN = 0; 	// Disable UART
	U2STAbits.UTXEN = 0;	// Disable UART Tx

	UART2_TX_TRIS = 0;				// Set Tx to output
	UART2_RX_TRIS = 1;				// Set RX to input
	
	U2MODE = 0x0808;  	//TX disabled, U1RTS flow control mode,TX and Rx pins enabled,wakeup,no loopback,no autobaud
						//RX idle is 1,high speed BRG, 8 bits no parity, 1 stop
	U2MODEbits.BRGH = bregh; // Speed mode
	U2BRG = brgval; // BAUD Rate Setting 
	
	IFS1bits.U2RXIF = 0;	// reset RX flag
	IFS1bits.U2TXIF = 0; 	// clear the TX flag
	IEC1bits.U2TXIE = 0;	// Disable TX interupt for now 
	
#ifdef USEINTDRIVENRX
	_U2RXIP = 4;
	IEC1bits.U2RXIE = 1; // Enable UART Rx interrupt
	U2STAbits.URXISEL = 0;	// Interupt after 1 character is received
	CQueue_init(&rxq2,rx2_fifo,RX2FIFOSIZE);
#endif

#ifdef USEINTDRIVENTX
	U2STAbits.UTXISEL1=0; 	//  Interrupt after 1 character transmitted
	U2STAbits.UTXISEL0=0;
	CQueue_init(&txq2,tx2_fifo,TX2FIFOSIZE);
#endif

	U2MODEbits.UARTEN = 1; 	// Enable UART
	U2STAbits.UTXEN = 1;	// Enable UART Tx

}

// Either send a byte directly or put it into the TX queue 
char  UART2PutCh(unsigned char Ch){
	char error = 0;
	
#ifdef USEINTDRIVENTX
	char i;
	if (CQueue_isEmpty(&txq2) && (U2STAbits.UTXBF==0)) { // if the TX butter is empty then send it otherwise queue it
		U2TXREG = Ch;
	}
	else {
		IEC1bits.U2TXIE = 1; // characters in queue so enable Uart 2 TX interupt
		if (CQueue_isFull(&txq2)) {
			for (i=0;i<10;i++) {
				if (CQueue_isFull(&txq2))
					DelayUsecs(50);
				else
					break;
			}
		}
		if (CQueue_isFull(&txq2)) {
			g_SysError.TX2_Overun = 1;
			error=1;
		}
		else {
			IEC1bits.U2TXIE = 0; // disable TX interupts while messing with the queue
			CQueue_enqueue(&txq2, Ch);
			IEC1bits.U2TXIE = 1; // characters in queue so enable Uart 2 TX interupt
		}
		IEC1bits.U2TXIE = 1; // always make sure the interrupt is enabled in case there are characters in the queue
	}
#else
    // wait for empty buffer  
   	while(!U2STAbits.TRMT);	//Continue while transmit shift register not empty
	U2TXREG = Ch;			//Transmit data
   
#endif
	return error;
}


// returns 1 if a char is ready otherwise 0 
char UART2ChReady()
{
#ifdef USEINTDRIVENRX
	if (CQueue_isEmpty(&rxq2))
		return 0;
	else
		return 1;
#else
    if(U2STAbits.URXDA == 0)
        return 1;
    else
	    return 0;
#endif    
}

// gets a character from the queue - UART2ChReady() must return 1 before calling this function
unsigned char UART2GetCh(){
	unsigned char temp = 0xff;
#ifdef USEINTDRIVENRX
	IEC1bits.U2RXIE = 0; // Disable UART Rx interrupt while fetching character
	if (!CQueue_isEmpty(&rxq2)) {
		CQueue_dequeue(&rxq2,&temp);
	}
	IEC1bits.U2RXIE = 1; // Re-Enable UART Rx interrupt
#else // polled mode
    while(!U2STAbits.URXDA);
    temp = U2RXREG;
#endif	
    return temp;
}

// returns the remaining size in the Tx buffer if using a buffer otherwise a very large integer size
unsigned int UART2GetTxBufSize()
{
#ifdef USEINTDRIVENTX
	return	CQueue_freesize(&txq2);
#else
	return 30000;
#endif
}


#ifdef USEINTDRIVENTX
void __attribute__((__interrupt__, no_auto_psv)) _U2TXInterrupt(void)
{
	unsigned char c2;
	if (CQueue_isEmpty(&txq2))
		IEC1bits.U2TXIE = 0;	// nothing in the queue so shut off the interupt
	else {
		CQueue_dequeue(&txq2,&c2); // if there is a character in the queue then put it into the TX buffer
		U2TXREG = c2;
	}	
	IFS1bits.U2TXIF = 0; // clear the TX flag
}
#endif

#ifdef USEINTDRIVENRX
void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt(void)
{
	unsigned char rbyte; 

	while (U2STAbits.URXDA == 1) {	// there can be multiple characters in the buffer
		if (U2STAbits.OERR) {
			g_SysError.U2_OverunError |= U2STAbits.OERR;
			U2STAbits.OERR = 0;	// clear any overrun errors
		}	
		rbyte = U2RXREG;
		if (!CQueue_isFull(&rxq2)) {
			g_SysError.U2_ParityError |= U2STAbits.PERR;
		  	g_SysError.U2_FramingError |= U2STAbits.FERR;
	  		g_SysError.U2_OverunError |= U2STAbits.OERR;
			CQueue_enqueue(&rxq2,rbyte);					
		}
		else {
			g_SysError.RX2_Overun = 1;
		}
	}
	IFS1bits.U2RXIF = 0; // clear the RX flag

}
#endif



	
char UART2Rx_push(unsigned char ch)
{
	char error = 0;
#ifdef USEINTDRIVENRX
	IEC1bits.U2RXIE = 0; // Disable UART Rx interrupt while fetching character
	if (!CQueue_isFull(&rxq2)) {
		CQueue_enqueue(&rxq2,ch);
	}
	else {
		g_SysError.RX2_Overun = 1;
		error = 1;
	}	
	IEC1bits.U2RXIE = 1; // Re-Enable UART Rx interrupt
#endif
	return error;
}	

int GetUART2RXpCent() 
{
#ifdef USEINTDRIVENRX
	if (rxq2.len==0)
		return 0;
	else
		return ((rxq2.peakhold * 100)/rxq2.len);
#else
	return 0;
#endif		
}

int GetUART2TXpCent()
{
#ifdef USEINTDRIVENTX
	if (txq2.len==0)
		return 0;
	else
		return ((txq2.peakhold * 100)/txq2.len);
#else
	return 0;
#endif
		
}

void ResetUART2peaks()
{
#ifdef USEINTDRIVENRX
	rxq2.peakhold=0;
#endif
	
#ifdef USEINTDRIVENTX
	txq2.peakhold=0;
#endif
}

/*****************************************************************************
 * EOF
 *****************************************************************************/
