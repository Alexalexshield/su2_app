
#include "queue.h"

// for example, define the following 
// #define FIFOSIZE 30
// unsigned char rx_fifo[FIFOSIZE];
// struct QUEUE rxq;
// and then call
// initQueue(&rxq,rx_fifo,FIFOSIZE);
void CQueue_init(struct CQUEUE * pq,unsigned char *pbuf,unsigned int len)
{
  pq->pbuf=pbuf;
  pq->pushix=0;
  pq->pullix=0;
  pq->size = 0;
  pq->peakhold = 0;
  pq->len = len;
}

// using the above definitions, reset the queue with a call like 
// resetQueue(&rxq);
void CQueue_reset(struct CQUEUE * pq) 
{
  unsigned int i;
  pq->pushix = 0;
  pq->pullix = 0;
  pq->size=0;
  for (i=0;i<pq->len;i++) 
	pq->pbuf[i]=0;
}

// returns 0 if no errors else 1 if errors such as the queue is full
char CQueue_enqueue(struct CQUEUE * pq, unsigned char data)
{
  if (pq->size < pq->len) {
	  pq->size++;
  	  pq->pbuf[pq->pushix++] = data;
	  if (pq->pushix >= pq->len) {
	      pq->pushix = 0;
      }	
	  if (pq->size > pq->peakhold)	// keep a peak hold on buffer
	  	pq->peakhold = pq->size;
      return 0;  
  }
  return 1;
}

// returns true if the queue is empty  
char CQueue_isEmpty(struct CQUEUE * pq)
{
  return (pq->size == 0)?1:0;
}

// returns true if the queue is full
char CQueue_isFull(struct CQUEUE * pq)
{
  return (pq->size >= pq->len)?1:0;
}

// returns the number of characters in the queue
unsigned int CQueue_size(struct CQUEUE * pq)
{
  return (pq->size);
}


// pushes a character into the front of the queue and returns 0 if no errors 
unsigned char CQueue_dequeue(struct CQUEUE * pq, unsigned char * pdata)
{
  if (pq->size > 0) {
	  pq->size--;
  	  *pdata = pq->pbuf[pq->pullix++];
	  if (pq->pullix >= pq->len) {
	      pq->pullix = 0;
      }	
      return 0;  
  }
  return 1;
}

unsigned int CQueue_freesize(struct CQUEUE * pq)
{
	return (pq->len - pq->size);
}	

