#ifndef Queue_H  
#define Queue_H  



//#define QUEUE_DEBUG
#ifdef QUEUE_DEBUG
#define QUEUE_DEBUG_PRINTF 		DEBUG_PRINTF
#define queue_debug_printf		debug_printf 	
#else
#define QUEUE_DEBUG_PRINTF 		
#define queue_debug_printf		 	
#endif



#define DATA_CACH	64
//typedef int Item;  
typedef unsigned char Item;
typedef struct node * PNode;  
typedef struct node  
{  
    //Item data;  
    Item  			*data;
	unsigned int 	datalen;
    PNode 			next;  
}Node;  
  
typedef struct  
{  
    PNode front;  
    PNode rear;  
    int size;  
}Queue;  


//extern Queue *pqueue;
extern Queue *QueueHead; 
extern Queue *queuehead;

  
/*����һ���ն���*/  
Queue *InitQueue();  
  
/*����һ������*/  
void DestroyQueue(Queue *pqueue);  
  
/*���һ������*/  
void ClearQueue(Queue *pqueue);  
  
/*�ж϶����Ƿ�Ϊ��*/  
int IsEmpty(Queue *pqueue);  
  
/*���ض��д�С*/  
int GetSize(Queue *pqueue);  
  
/*���ض�ͷԪ��*/  
PNode GetFront(Queue *pqueue,Item *pitem,unsigned int *len);  
  
/*���ض�βԪ��*/  
PNode GetRear(Queue *pqueue,Item *pitem,unsigned int *len);  
  
/*����Ԫ�����*/  
PNode EnQueue(Queue *pqueue,Item *item,unsigned int len);  
  
/*��ͷԪ�س���*/  
PNode DeQueue(Queue *pqueue,Item *pitem,unsigned int *len);  
  
/*�������в��Ը����������visit����*/  
void QueueTraverse(Queue *pqueue,void (*visit)(Item item));  

int Get_headlen(Queue *pqueue);



  
#endif  

