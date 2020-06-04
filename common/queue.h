
#ifndef __QUEUE_H
#define __QUEUE_H

struct CQUEUE {
  unsigned char* pbuf;
  unsigned int len; 
  unsigned int pullix;
  unsigned int pushix;
  unsigned int size;
  unsigned int peakhold;
};

void CQueue_init(struct CQUEUE * pq,unsigned char *pbuf,unsigned int len);
void CQueue_reset(struct CQUEUE * pq); 
char CQueue_enqueue(struct CQUEUE * pq, unsigned char data);
unsigned char CQueue_dequeue(struct CQUEUE * pq, unsigned char * pdata);
char CQueue_isEmpty(struct CQUEUE * pq);
char CQueue_isFull(struct CQUEUE * pq);
unsigned int CQueue_size(struct CQUEUE * pq);
unsigned int CQueue_freesize(struct CQUEUE * pq);

#endif // __QUEUE_H
