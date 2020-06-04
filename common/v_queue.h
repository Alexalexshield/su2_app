
#ifndef __V_QUEUE_H
#define __V_QUEUE_H

struct VQUEUE {
  void* pbuf;
  unsigned int len; 
  unsigned int pullix;
  unsigned int pushix;
  unsigned int size;
  unsigned int peakhold;
  unsigned int element_size;
};

void v_Queue_init(struct VQUEUE * pq, void* pbuf, unsigned int element_size, unsigned int len);
void v_Queue_reset(struct VQUEUE * pq); 
char v_Queue_enqueue(struct VQUEUE * pq, void* pdata);
unsigned char v_Queue_dequeue(struct VQUEUE * pq, void * pdata);
char v_Queue_isEmpty(struct VQUEUE * pq);
char v_Queue_isFull(struct VQUEUE * pq);
unsigned int v_Queue_size(struct VQUEUE * pq);
unsigned int v_Queue_freesize(struct VQUEUE * pq);

#endif // __V_QUEUE_H
