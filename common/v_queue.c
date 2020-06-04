
#include "v_queue.h"
#include "string.h"

// define the queue parameters such as size and element size
void v_Queue_init(struct VQUEUE * pq, void* pbuf, unsigned int element_size, unsigned int len)
{
  pq->pbuf = pbuf;
  pq->pushix=0;
  pq->pullix=0;
  pq->size = 0;
  pq->peakhold = 0;
  pq->len = len;
  pq->element_size = element_size;
}

// using the above definitions, reset the queue with a call like 
// resetQueue(&rxq);
void v_Queue_reset(struct VQUEUE * pq) 
{
  pq->pushix = 0;
  pq->pullix = 0;
  pq->size=0;
}

// returns 0 if no errors else 1 if errors such as the queue is full
char v_Queue_enqueue(struct VQUEUE * pq, void* pdata)
{
  if (pq->size < pq->len) 
  {
	  pq->size++;
  	  memcpy((((char*)pq->pbuf)+(pq->pushix*pq->element_size)), pdata, pq->element_size);
  	  pq->pushix++;
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
char v_Queue_isEmpty(struct VQUEUE * pq)
{
  return (pq->size == 0)?1:0;
}

// returns true if the queue is full
char v_Queue_isFull(struct VQUEUE * pq)
{
  return (pq->size >= pq->len)?1:0;
}

// returns the number of characters in the queue
unsigned int v_Queue_size(struct VQUEUE * pq)
{
  return (pq->size);
}


// pushes a character into the front of the queue and returns 0 if no errors 
unsigned char v_Queue_dequeue(struct VQUEUE * pq, void* pdata)
{
  if (pq->size > 0) {
	  pq->size--;
   	  memcpy(pdata, (((char*)pq->pbuf)+(pq->pullix*pq->element_size)),  pq->element_size);
	  pq->pullix++;	
	  if (pq->pullix >= pq->len) {
	      pq->pullix = 0;
      }	
      return 0;  
  }
  return 1;
}

unsigned int v_Queue_freesize(struct VQUEUE * pq)
{
	return (pq->len - pq->size);
}	

