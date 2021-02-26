#ifndef __MTIMER_H
#define __MTIMER_H
#include <sys/time.h> 
#include <signal.h>

#if 1
#include "../include/mylist.h"
#include "../include/myerror.h"
#else
#include "mylist.h"
#include "myerror.h"
#endif



//#define TIMER_DEBUG
#ifdef TIMER_DEBUG
#define TIMER_DEBUG_PRINTF	DEBUG_PRINTF
#define timer_debug_printf 	debug_printf 
#else
#define TIMER_DEBUG_PRINTF	
#define timer_debug_printf 	 
#endif





typedef struct _timer
{
	int 				id;						//定时器的唯一id号
	int 				logout;					//时间到是否把定时器注销掉
	int 				ref_vals;				//定时器的参考值
	int 				counter;				//定时器的实时值
	int 				data;
	int 				reserd;
	void 				*(*function)(void *);	//定时器的回调函数
	void				*arg;					//回调函数的参数
	struct list_head 	list;					//链表节点
}stimer_t;

typedef void *(*callback_t)(void *);


typedef struct __timer
{
	int 				ref_vals;				//定时器的参考值
	int 				counter;				//定时器的实时值
}ttimer_t;

extern ttimer_t timer[2];

extern struct list_head mtimer_head;
extern pthread_mutex_t timerlist_mutex;
void mtimer_init(void);
int mtimer_requestID(void);
stimer_t *mtimer_search(int timer_id);
void mtimer_clear(int timer_id);
err_t mtimer_unregister(int timer_id);
err_t mtimer_register(stimer_t *timer);
void TIMER_init(stimer_t *timer);
void mtimer_init(void);

#endif

