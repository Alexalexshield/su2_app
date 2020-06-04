/*****************************************************************************
 *
 * UART3 Driver functions
 *
 *
 *****************************************************************************/
#include "system.h"
#include "uart3.h"
#include "putfunctions.h"

// UART3 IOs
#define UART3_TX_TRIS   TRISDbits.TRISD9
#define UART3_RX_TRIS   TRISDbits.TRISD8

#define TX3FIFOSIZE 128	// UART_TX to SU TX module
#define RX3FIFOSIZE 64	// UART_RX from the SU TX module 

// comment out next line to use poll-driven RX or uncomment to use int-driven RX
//#define USEINTDRIVENTX 1
#define USEINTDRIVENRX 1

#ifdef USEINTDRIVENTX
#include "queue.h"

unsigned char tx3_fifo[TX3FIFOSIZE];
struct CQUEUE txq3;
#endif

#ifdef USEINTDRIVENRX
#include "queue.h"

unsigned char rx3_fifo[RX3FIFOSIZE];
struct CQUEUE rxq3;
#endif

// Initialize the Uart
void UART3Init(unsigned char brgval,unsigned char bregh)
{
	U3MODEbits.UARTEN = 0; 	// Disable UART
	U3STAbits.UTXEN = 0;	// Disable UART Tx

	UART3_TX_TRIS = 0;				// Set Tx to output
	UART3_RX_TRIS = 1;				// Set RX to input
	
	U3MODE = 0x0808;  	//TX disabled, U1RTS flow control mode,TX and Rx pins enabled,wakeup,no loopback,no autobaud
						//RX idle is 1,high speed BRG, 8 bits no parity, 1 stop
	U3MODEbits.BRGH = bregh; // Speed mode
	U3BRG = brgval; // BAUD Rate Setting 
	
	IFS5bits.U3RXIF = 0;	// reset RX flag
	IFS5bits.U3TXIF = 0; 	// clear the TX flag
	IEC5bits.U3TXIE = 0;	// Disable TX interupt for now 
	
#ifdef USEINTDRIVENRX
	IEC5bits.U3RXIE = 1; // Enable UART Rx interrupt
	U3STAbits.URXISEL = 0;	// Interupt after 1 character is received
	CQueue_init(&rxq3,rx3_fifo,RX3FIFOSIZE);
#endif

#ifdef USEINTDRIVENTX
	U3STAbits.UTXISEL1=0; 	//  Interrupt after 1 character transmitted
	U3STAbits.UTXISEL0=0;
	CQueue_init(&txq3,tx3_fifo,TX3FIFOSIZE);
#endif

	U3MODEbits.UARTEN = 1; 	// Enable UART
	U3STAbits.UTXEN = 1;	// Enable UART Tx

}

// Either send a byte directly or put it into the TX queue 
char  UART3PutCh(unsigned char Ch){
	char error = 0;
	char i;
	
#ifdef USEINTDRIVENTX
	if (CQueue_isEmpty(&txq3) && (U3STAbits.UTXBF==0)) { // if the TX butter is empty then send it otherwise queue it
		U3TXREG = Ch;
	}
	else {
		IEC5bits.U3TXIE = 1; // characters in queue so enable Uart 3 TX interupt
		if (CQueue_isFull(&txq3)) {
			for (i=0;i<10;i++) {
				if (CQueue_isFull(&txq3))
					DelayUsecs(50);
				else
					break;
			}
		}
		if (CQueue_isFull(&txq3)) {
			g_SysError.U3_TXError = 1;
			error=1;
		}
		else {
			IEC5bits.U3TXIE = 0; // disable TX interupts while messing with the TX queue
			CQueue_enqueue(&txq3, Ch);
			IEC5bits.U3TXIE = 1; // characters in queue so enable Uart 2 TX interupt
		}
		IEC5bits.U3TXIE = 1; // ALWAYS ENABLE TX INTERRUPT IN CASE THERE IS A CHARACTER IN QUEUE
	}
#else
    // wait for empty buffer  
   	while(!U3STAbits.TRMT);	//Continue while transmit shift register not empty
	U3TXREG = Ch;			//Transmit data
   
#endif
	return error;
}


// returns 1 if a char is ready otherwise 0 
char UART3ChReady()
{
#ifdef USEINTDRIVENRX
	if (CQueue_isEmpty(&rxq3))
		return 0;
	else
		return 1;
#else
    if(U3STAbits.URXDA == 0)
        return 1;
    else
	    return 0;
#endif    
}

// gets a character from the queue - UART3ChReady() must return 1 before calling this function
unsigned char UART3GetCh(){
	unsigned char temp = 0xff;
#ifdef USEINTDRIVENRX
	IEC5bits.U3RXIE = 0; // Disable UART Rx interrupt while fetching character
	if (!CQueue_isEmpty(&rxq3)) {
		CQueue_dequeue(&rxq3,&temp);
	}
	IEC5bits.U3RXIE = 1; // Re-Enable UART Rx interrupt
#else // polled mode
    while(!U3STAbits.URXDA);
    temp = U3RXREG;
#endif	
    return temp;
}

// returns the remaining size in the Tx buffer if using a buffer otherwise a very large integer size
unsigned int UART3GetTxBufSize()
{
#ifdef USEINTDRIVENTX
	return	CQueue_freesize(&txq3);
#else
	return 30000;
#endif
}

#ifdef USEINTDRIVENTX
void __attribute__((__interrupt__, no_auto_psv)) _U3TXInterrupt(void)
{
	unsigned char ch;
	if (CQueue_isEmpty(&txq3))
		IEC5bits.U3TXIE = 0;	// nothing in the queue so shut off the interupt
	else {
		CQueue_dequeue(&txq3,&ch); // if there is a character in the queue then put it into the TX buffer
		U3TXREG = ch;
	}	
	IFS5bits.U3TXIF = 0; // clear the TX flag
}
#endif

#ifdef USEINTDRIVENRX
void __attribute__((__interrupt__, no_auto_psv)) _U3RXInterrupt(void)
{
	unsigned char rbyte; 

	while (U3STAbits.URXDA == 1) {	// there can be multiple characters in the buffer
		if (U3STAbits.OERR) {
			g_SysError.U3_RXError = 1;
			U3STAbits.OERR = 0;	// clear any overrun errors
		}	
		rbyte = U3RXREG;
		if (!CQueue_isFull(&rxq3)) {
		  	g_SysError.U3_RXError |= U3STAbits.FERR;
	  		g_SysError.U3_RXError |= U3STAbits.OERR;
			CQueue_enqueue(&rxq3,rbyte);					
		}
		else {
			g_SysError.U3_RXError = 1;
		}
	}
	IFS5bits.U3RXIF = 0; // clear the RX flag

}
#endif



	
char UART3Rx_push(unsigned char ch)
{
	char error = 0;
#ifdef USEINTDRIVENRX
	IEC5bits.U3RXIE = 0; // Disable UART Rx interrupt while fetching character
	if (!CQueue_isFull(&rxq3)) {
		CQueue_enqueue(&rxq3,ch);
	}
	else {
		g_SysError.U3_RXError = 1;
		error = 1;
	}	
	IEC5bits.U3RXIE = 1; // Re-Enable UART Rx interrupt
#endif
	return error;
}	

int GetUART3RXpCent() 
{
#ifdef USEINTDRIVENRX
	if (rxq3.len==0)
		return 0;
	else
		return ((rxq3.peakhold * 100)/rxq3.len);
#else
	return 0;
#endif		
}

int GetUART3TXpCent()
{
#ifdef USEINTDRIVENTX
	if (txq3.len==0)
		return 0;
	else
		return ((txq3.peakhold * 100)/txq3.len);
#else
	return 0;
#endif
		
}

void ResetUART3peaks()
{
#ifdef USEINTDRIVENRX
	rxq3.peakhold=0;
#endif
	
#ifdef USEINTDRIVENTX
	txq3.peakhold=0;
#endif
}

/*****************************************************************************
 * EOF
 *****************************************************************************/
