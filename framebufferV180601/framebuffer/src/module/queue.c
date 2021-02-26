#include"queue.h"  
#include<malloc.h>  
#include<stdio.h> 
#include <string.h>

#include "debug.h"


//Queue *pqueue = NULL; 
Queue *QueueHead = NULL;
Queue *queuehead = NULL;    //add by mo 2020.07.11
/*构造一个空队列*/  
Queue *InitQueue()  
{  
    Queue *pqueue = (Queue *)malloc(sizeof(Queue));  
    if(pqueue!=NULL)  
    {  
        pqueue->front = NULL;  
        pqueue->rear = NULL;  
        pqueue->size = 0;  
    }  
    return pqueue;  
}  
  
/*销毁一个队列*/  
void DestroyQueue(Queue *pqueue)  
{  
    if(IsEmpty(pqueue)!=1)  
        ClearQueue(pqueue);  
    free(pqueue);  
}  
  
/*清空一个队列*/  
void ClearQueue(Queue *pqueue)  
{  
    while(IsEmpty(pqueue)!=1)  
    {  
        DeQueue(pqueue,NULL,NULL);  
    }  
  
}  
  
/*判断队列是否为空*/  
int IsEmpty(Queue *pqueue)  
{  
    if(pqueue->front==NULL&&pqueue->rear==NULL&&pqueue->size==0)  
    {
		//debug_printf("if :(pqueue->front==NULL):%d,(pqueue->rear==NULL):%d,pqueue->size = %d\n",pqueue->front==NULL,pqueue->rear==NULL,pqueue->size);
		return 1;  
	}
    else
    {
		//debug_printf("else :(pqueue->front==NULL):%d,(pqueue->rear==NULL):%d,pqueue->size = %d\n",pqueue->front==NULL,pqueue->rear==NULL,pqueue->size);
		return 0;  
	}
}  
  
/*返回队列大小*/  
int GetSize(Queue *pqueue)  
{  
    return pqueue->size;  
}  
  
/*返回队头元素*/  
PNode GetFront(Queue *pqueue,Item *pitem,unsigned int *len)  
{  
    if(IsEmpty(pqueue)!=1&&pitem!=NULL)  
    {  
        //*pitem = pqueue->front->data; 
		memcpy(pitem,pqueue->front->data,pqueue->front->datalen);
		*len = pqueue->front->datalen;
    }  
    return pqueue->front;  
}  
  
/*返回队尾元素*/  
  
PNode GetRear(Queue *pqueue,Item *pitem,unsigned int *len)  
{  
    if(IsEmpty(pqueue)!=1&&pitem!=NULL)  
    {  
        //*pitem = pqueue->rear->data;
        memcpy(pitem,pqueue->rear->data,pqueue->rear->datalen);
		*len = pqueue->rear->datalen;
    }  
    return pqueue->rear;  
}


void Dequeue_specify(Queue *pqueue,Node *node)
{
	Node *pre_node = pqueue->front;
	Node *cur_node = pqueue->front;
	
	if(IsEmpty(pqueue))
		return ;

	while(cur_node != NULL)
	{
		if(cur_node == node)
			break;
		
		pre_node = cur_node;
		cur_node = cur_node->next;
	}

	//找不到节点
	if(cur_node == NULL)
	{
		return ;
	}

	//找到节点

	
	return ;
	
}


/*将新元素入队,插入队列尾部*/  
PNode EnQueue(Queue *pqueue,Item *item,unsigned int len)  
{  
	int i = 0 ; 
	//Item  *item_data = (Item *)malloc(len + 24);
	
    PNode pnode = (PNode)malloc(sizeof(Node)+4); 
	pnode->data = (Item *)malloc(len + 24);

	if(pnode == NULL || pnode->data == NULL)
	{
		perror("malloc");
		queue_debug_printf("there is not enought memory for malloc!\n");
		return NULL;
	}
	
	//pnode->data = item; 
    //pnode->data = item_data;
	memcpy(pnode->data,item,len);
	pnode->datalen = len;
    pnode->next = NULL;  
   
    if(IsEmpty(pqueue))  
    {  
    	QUEUE_DEBUG_PRINTF;
        pqueue->front = pnode; 
    } 
    else  
    {  
    	QUEUE_DEBUG_PRINTF;
        pqueue->rear->next = pnode;  
    }  
    
	pqueue->rear = pnode;  
	pqueue->size++; 
	queue_debug_printf("pqueue->size = %d\n",pqueue->size);

	QUEUE_DEBUG_PRINTF;
    return pnode;  
} 

  
/*队头元素出队*/  
PNode DeQueue(Queue *pqueue,Item *pitem,unsigned int *len)  
{  
	int i;
    PNode pnode = pqueue->front;

	if(IsEmpty(pqueue) || pnode == NULL)
		return NULL;

	if(pitem == NULL || len == NULL)
		return NULL;
	
	memcpy(pitem,pnode->data,pnode->datalen);
	*len = pnode->datalen;

	//debug_printf("1:(pqueue->front==NULL):%d,(pqueue->rear==NULL):%d,pqueue->size = %d\n",pqueue->front==NULL,pqueue->rear==NULL,pqueue->size);
    pqueue->size--;  
    pqueue->front = pnode->next;
	
	free(pnode->data);
	pnode->data = NULL;
    free(pnode); 
	pnode = NULL;
	
	//debug_printf("2:(pqueue->front==NULL):%d,(pqueue->rear==NULL):%d,pqueue->size = %d\n",pqueue->front==NULL,pqueue->rear==NULL,pqueue->size);
	if(pqueue->front==NULL && pqueue->rear!=NULL && pqueue->size != 0)
	{
		//在这里写日志吧
		//debug_printf("pqueue->front==NULL && pqueue->rear!=NULL && pqueue->size != 0\n");
	}

	
	if(pqueue->size==0)
	{
        pqueue->rear  = NULL; 
		pqueue->front = NULL;
	}

	
    return pnode;  
}

int Get_headlen(Queue *pqueue)
{
    PNode pnode = pqueue->front;
    if(IsEmpty(pqueue)!=1&&pnode!=NULL)
		return pnode->datalen;
	else
		return 0;
}

#if 0  
/*遍历队列并对各数据项调用visit函数*/  
void QueueTraverse(Queue *pqueue,void (*visit)(Item item))  
{  
    PNode pnode = pqueue->front;  
    int i = pqueue->size;  
    while(i--)  
    {  
        visit(pnode->data);  
        pnode = pnode->next;  
    }  
          
} 

#endif


#if 0
void printf(Item i)  
{  
    debug_printf("该节点元素为%d\n",i);  
}  
main()  
{  
    Queue *pq = InitQueue();  
    int i,item;  
    debug_printf("0-9依次入队并输出如下：\n");  
    for(i=0;i<10;i++)  
    {  
        EnQueue(pq,i);  
        GetRear(pq,&item);  
        debug_printf("%d ",item);  
    }  
  
    debug_printf("\n从队头到队尾遍历并对每个元素执行print函数：\n");   
    QueueTraverse(pq,print);  
  
    debug_printf("队列中元素依次出队列并输出如下：\n");  
    for(i=0;i<10;i++)  
    {  
        DeQueue(pq,&item);  
        debug_printf("%d ",item);  
    }  
    ClearQueue(pq);  
    if(IsEmpty(pq))  
        debug_printf("\n将队列置空成功\n");  
    DestroyQueue(pq);  
    debug_printf("队列已被销毁\n");  
}  


#endif

