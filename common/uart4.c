/*****************************************************************************
 *
 * UART4 Driver functions
 *
 *
 *****************************************************************************/
#include "system.h"
#include "uart2.h"
#include "putfunctions.h"


// UART4 IOs
#define UART4_TX_TRIS   TRISBbits.TRISB9
#define UART4_RX_TRIS   TRISBbits.TRISB8
//#define UART4_TX_TRIS   TRISBbits.TRISB8
//#define UART4_RX_TRIS   TRISBbits.TRISB9


//#define TX4FIFOSIZE 128	// UART_TX to SU RX module
#define RX4FIFOSIZE 256	// UART_RX from the SU RX module 

// comment out next line to use poll-driven RX or uncomment to use int-driven RX
//#define USEINTDRIVENTX 1
#define USEINTDRIVENRX 1

#ifdef USEINTDRIVENTX
#include "queue.h"

unsigned char tx4_fifo[TX4FIFOSIZE];
struct CQUEUE txq4;
#endif

#ifdef USEINTDRIVENRX
#include "queue.h"

unsigned char rx4_fifo[RX4FIFOSIZE];
struct CQUEUE rxq4;
#endif

// Initialize the Uart
void UART4Init(unsigned char brgval,unsigned char bregh)
{
	U4MODEbits.UARTEN = 0; 	// Disable UART
	U4STAbits.UTXEN = 0;	// Disable UART Tx

	UART4_TX_TRIS = 0;				// Set Tx to output
	UART4_RX_TRIS = 1;				// Set RX to input
	
	U4MODE = 0x0808;  	//TX disabled, U1RTS flow control mode,TX and Rx pins enabled,wakeup,no loopback,no autobaud
						//RX idle is 1,high speed BRG, 8 bits no parity, 1 stop
	U4MODEbits.BRGH = bregh; // Speed mode
	U4BRG = brgval; // BAUD Rate Setting 
	
	IFS5bits.U4RXIF = 0;	// reset RX flag
	IFS5bits.U4TXIF = 0; 	// clear the TX flag
	IEC5bits.U4TXIE = 0;	// Disable TX interupt for now 
	
#ifdef USEINTDRIVENRX
	IEC5bits.U4RXIE = 1; // Enable UART Rx interrupt
	U4STAbits.URXISEL = 0;	// Interupt after 1 character is received
	CQueue_init(&rxq4,rx4_fifo,RX4FIFOSIZE);
#endif

#ifdef USEINTDRIVENTX
	U4STAbits.UTXISEL1=0; 	//  Interrupt after 1 character transmitted
	U4STAbits.UTXISEL0=0;
	CQueue_init(&txq4,tx4_fifo,TX4FIFOSIZE);
#endif

	U4MODEbits.UARTEN = 1; 	// Enable UART
	U4STAbits.UTXEN = 1;	// Enable UART Tx

}

// Either send a byte directly or put it into the TX queue 
char  UART4PutCh(unsigned char Ch){
	char error = 0;
	
#ifdef USEINTDRIVENTX
	char i;
	if (CQueue_isEmpty(&txq4) && (U4STAbits.UTXBF==0)) { // if the TX butter is empty then send it otherwise queue it
		U4TXREG = Ch;
	}
	else {
		IEC5bits.U4TXIE = 1; // characters in queue so enable Uart 2 TX interupt
		if (CQueue_isFull(&txq4)) {
			for (i=0;i<10;i++) {
				if (CQueue_isFull(&txq4))
					DelayUsecs(50);
				else
					break;
			}
		}
		if (CQueue_isFull(&txq4)) {
			g_SysError.U4_TXError = 1;
			error=1;
		}
		else {
			IEC5bits.U4TXIE = 0; // disable TX interupts while messing with the queue
			CQueue_enqueue(&txq4, Ch);
			IEC5bits.U4TXIE = 1; // characters in queue so enable Uart 2 TX interupt
		}
		IEC5bits.U4TXIE = 1;  // ALWAYS ENABLE TX INTERRUPT IN CASE THERE IS A CHARACTER IN QUEUE
	}
#else
    // wait for empty buffer  
//    while(U4STAbits.UTXBF == 1);
//    U4TXREG = Ch;
     // wait for empty buffer  
   	while(!U4STAbits.TRMT);	//Continue while transmit shift register not empty
	U4TXREG = Ch;			//Transmit data
   
#endif
	return error;
}


// returns 1 if a char is ready otherwise 0 
char UART4ChReady()
{
#ifdef USEINTDRIVENRX
	if (CQueue_isEmpty(&rxq4))
		return 0;
	else {
		return 1;
	}
#else
    if(U4STAbits.URXDA == 0)
        return 1;
    else
	    return 0;
#endif    
}

// gets a character from the queue - UART4ChReady() must return 1 before calling this function
unsigned char UART4GetCh(){
	unsigned char temp = 0xff;
#ifdef USEINTDRIVENRX
	IEC5bits.U4RXIE = 0; // Disable UART Rx interrupt while fetching character
	if (!CQueue_isEmpty(&rxq4)) {
		CQueue_dequeue(&rxq4,&temp);
	}
	IEC5bits.U4RXIE = 1; // Re-Enable UART Rx interrupt
#else // polled mode
    while(!U4STAbits.URXDA);
    temp = U4RXREG;
#endif	
    return temp;
}

// returns the remaining size in the Tx buffer if using a buffer otherwise a very large integer size
unsigned int UART4GetTxBufSize()
{
#ifdef USEINTDRIVENTX
	return	CQueue_freesize(&txq4);
#else
	return 30000;
#endif
}

#ifdef USEINTDRIVENTX
void __attribute__((__interrupt__, no_auto_psv)) _U4TXInterrupt(void)
{
	unsigned char ch;
	if (CQueue_isEmpty(&txq4))
		IEC5bits.U4TXIE = 0;	// nothing in the queue so shut off the interupt
	else {
		CQueue_dequeue(&txq4,&ch); // if there is a character in the queue then put it into the TX buffer
		U4TXREG = ch;
	}	
	IFS5bits.U4TXIF = 0; // clear the TX flag
}
#endif

#ifdef USEINTDRIVENRX
void __attribute__((__interrupt__, no_auto_psv)) _U4RXInterrupt(void)
{
	unsigned char rbyte; 

	while (U4STAbits.URXDA == 1) {	// there can be multiple characters in the buffer
		if (U4STAbits.OERR) {
			g_SysError.U4_RXError = 1;
			U4STAbits.OERR = 0;	// clear any overrun errors
		}	
		rbyte = U4RXREG;
		if (!CQueue_isFull(&rxq4)) {
		  	g_SysError.U4_RXError |= U4STAbits.FERR;
	  		g_SysError.U4_RXError |= U4STAbits.OERR;
			CQueue_enqueue(&rxq4,rbyte);					
		}
		else {
			g_SysError.U4_RXError = 1;
		}
	}
	IFS5bits.U4RXIF = 0; // clear the RX flag

}
#endif



	
char UART4Rx_push(unsigned char ch)
{
	char error = 0;
#ifdef USEINTDRIVENRX
	IEC5bits.U4RXIE = 0; // Disable UART Rx interrupt while fetching character
	if (!CQueue_isFull(&rxq4)) {
		CQueue_enqueue(&rxq4,ch);
	}
	else {
		g_SysError.U4_RXError = 1;
		error = 1;
	}	
	IEC5bits.U4RXIE = 1; // Re-Enable UART Rx interrupt
#endif
	return error;
}	

int GetUART4RXpCent() 
{
#ifdef USEINTDRIVENRX
	if (rxq4.len==0)
		return 0;
	else
		return ((rxq4.peakhold * 100)/rxq4.len);
#else
	return 0;
#endif		
}

int GetUART4TXpCent()
{
#ifdef USEINTDRIVENTX
	if (txq4.len==0)
		return 0;
	else
		return ((txq4.peakhold * 100)/txq4.len);
#else
	return 0;
#endif
		
}

void ResetUART4peaks()
{
#ifdef USEINTDRIVENRX
	rxq4.peakhold=0;
#endif
	
#ifdef USEINTDRIVENTX
	txq4.peakhold=0;
#endif
}

/*****************************************************************************
 * EOF
 *****************************************************************************/
