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

  
/*构造一个空队列*/  
Queue *InitQueue();  
  
/*销毁一个队列*/  
void DestroyQueue(Queue *pqueue);  
  
/*清空一个队列*/  
void ClearQueue(Queue *pqueue);  
  
/*判断队列是否为空*/  
int IsEmpty(Queue *pqueue);  
  
/*返回队列大小*/  
int GetSize(Queue *pqueue);  
  
/*返回队头元素*/  
PNode GetFront(Queue *pqueue,Item *pitem,unsigned int *len);  
  
/*返回队尾元素*/  
PNode GetRear(Queue *pqueue,Item *pitem,unsigned int *len);  
  
/*将新元素入队*/  
PNode EnQueue(Queue *pqueue,Item *item,unsigned int len);  
  
/*队头元素出队*/  
PNode DeQueue(Queue *pqueue,Item *pitem,unsigned int *len);  
  
/*遍历队列并对各数据项调用visit函数*/  
void QueueTraverse(Queue *pqueue,void (*visit)(Item item));  

int Get_headlen(Queue *pqueue);



  
#endif  

