#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include "mtimer.h"

#if 0
#include "config.h"
#include "debug.h"
#include "mylist.h"
#include "myerror.h"
#else
#include "config.h" 
#include "debug.h"
#include "mylist.h"
#include "myerror.h"
#endif

#define PTHREAD_MAX_NUM		10
static pthread_t ntid[PTHREAD_MAX_NUM];

pthread_t tid4;

static int ntid_pos = 0;

pthread_mutex_t timerlist_mutex;

struct list_head mtimer_head;

static stimer_t *timerout_obj[40] = {NULL};

ttimer_t timer[2] = 
{
	{2  * 1000 * 1000,0},
	{20 * 1000 * 1000,0}
};

void task_timer_add(void)
{
	timer[0].counter += 200 * 1000;
	timer[1].counter += 200 * 1000;
}



static inline void mtimer_list_head_init(void)
{
	INIT_LIST_HEAD(&mtimer_head);
	pthread_mutex_init(&timerlist_mutex,NULL);
}

static int test_pthread(pthread_t tid) /*pthread_kill的返回值：成功（0） 线程不存在（ESRCH） 信号不合法（EINVAL）*/
{
	int pthread_kill_err;

	pthread_kill_err = pthread_kill(tid,0);
	if(pthread_kill_err == ESRCH)
	{
		timer_debug_printf("pthread ID : 0x%x is not exist or has finish\n",(unsigned int)tid);
		return -1;
	}
	else if(pthread_kill_err == EINVAL)
	{
		timer_debug_printf("illigal signal!\n");
		return -2;
	}
	else
	{
		timer_debug_printf("pthread ID : 0x%x is running\n",(unsigned int)tid);
		return 0;
	}
}



static void _timer_action(int signo)   
{

	struct list_head *pos = NULL;
	stimer_t *get_mtimer = NULL;
	int n = 0,i = 0;

	callback_t callback_fun = NULL;
	void *callback_arg = NULL;

	pthread_attr_t attr;  
	pthread_t tid;	
	pthread_attr_init (&attr);	
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);	

	
	pthread_mutex_lock(&timerlist_mutex);
	if(list_empty(&mtimer_head))
	{
		pthread_mutex_unlock(&timerlist_mutex);
		return;
	}

	
	list_for_each(pos, &mtimer_head)//这里的pos会自动被赋新值
	{
		get_mtimer = list_entry(pos,stimer_t,list);
		if(get_mtimer == NULL)
		{
			pthread_mutex_unlock(&timerlist_mutex);
			return;
		}
		get_mtimer->counter += 1;
		//printf("get_mtimer->id = %d,get_mtimer->counter = %d,get_mtimer->ref_vals = %d\n",get_mtimer->id,get_mtimer->counter,get_mtimer->ref_vals);
		if(get_mtimer->counter < get_mtimer->ref_vals)
		{
			continue;
		}
		TIMER_DEBUG_PRINTF;

		if(get_mtimer->function == NULL || get_mtimer->arg == NULL)
		{
			continue;
		}
		
		TIMER_DEBUG_PRINTF;
		get_mtimer->counter = 0;
		callback_fun = get_mtimer->function;
		callback_arg = get_mtimer->arg;
		TIMER_DEBUG_PRINTF;
		int ret = -1;

		
		ret = pthread_create (&tid, &attr, callback_fun,callback_arg);  
		pthread_attr_destroy (&attr);	
	
		TIMER_DEBUG_PRINTF;
		if(ret!=0)
		{
			TIMER_DEBUG_PRINTF;
			perror("pthread_create fail");
			continue;
		}
		TIMER_DEBUG_PRINTF;
	}
	pthread_mutex_unlock(&timerlist_mutex);
} 




static void timer_action(int signo)   
{

	struct list_head *pos = NULL;
	stimer_t *get_mtimer = NULL;
	int n = 0,i = 0;

	callback_t callback_fun = NULL;
	void *callback_arg = NULL;

	pthread_attr_t attr;  
	pthread_t tid;	
	pthread_attr_init (&attr);	
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);	

	pthread_mutex_lock(&timerlist_mutex);
	if(list_empty(&mtimer_head))
	{
		pthread_mutex_unlock(&timerlist_mutex);
		return;
	}
	pthread_mutex_unlock(&timerlist_mutex);

	
	list_for_each(pos, &mtimer_head)//这里的pos会自动被赋新值
	{
		pthread_mutex_lock(&timerlist_mutex);
		get_mtimer = list_entry(pos,stimer_t,list);
		if(get_mtimer == NULL)
		{
			pthread_mutex_unlock(&timerlist_mutex);
			return;
		}

		if(get_mtimer->logout == 1)
		{
			list_del(&(get_mtimer->list));
			//if(get_mtimer->arg !=  NULL)
			//	free(get_mtimer->arg);

			if(get_mtimer != NULL)
				free(get_mtimer);
			get_mtimer->arg = NULL;
			get_mtimer = NULL;
			pthread_mutex_unlock(&timerlist_mutex);
			continue;
		}

		
		get_mtimer->counter += 1;
		//printf("get_mtimer->id = %d,get_mtimer->counter = %d,get_mtimer->ref_vals = %d,get_mtimer->logout = %d\n",get_mtimer->id,get_mtimer->counter,get_mtimer->ref_vals,get_mtimer->logout);
		if(get_mtimer->counter < get_mtimer->ref_vals)
		{
			pthread_mutex_unlock(&timerlist_mutex);
			continue;
		}

		if(get_mtimer->function == NULL || get_mtimer->arg == NULL)
		{
			pthread_mutex_unlock(&timerlist_mutex);
			continue;
		}
		 
		get_mtimer->counter = 0;
		callback_fun = get_mtimer->function;
		callback_arg = get_mtimer->arg;

		pthread_mutex_unlock(&timerlist_mutex);
		
		int ret = -1;

		
		ret = pthread_create (&tid, &attr, callback_fun,callback_arg);  
		pthread_attr_destroy (&attr);	
	
		if(ret!=0)
		{
			perror("pthread_create fail");
			continue;
		}
	}
} 



static void init_sigaction(void)   
{   
    struct sigaction tact;   
    tact.sa_handler = timer_action;   
    tact.sa_flags = 0;   
    sigemptyset(&tact.sa_mask);   
    sigaction(SIGALRM, &tact, NULL);   
}   
static void init_time()    
{   
    struct itimerval value;   
    value.it_value.tv_sec = 1;   
    value.it_value.tv_usec = 0;   
    value.it_interval = value.it_value;   
    setitimer(ITIMER_REAL, &value, NULL);   
}


//extern function

void mtimer_init(void)
{
	mtimer_list_head_init();
	init_sigaction();
	init_time();
}

void mtimer_clear(int timer_id)
{
	struct list_head *pos = NULL;
	stimer_t *get_mtimer = NULL;

	timer_debug_printf("mtimer_clear : timer_id = %d\n",timer_id);
	
	if(list_empty(&mtimer_head))
	{
		return;
	}

	

	list_for_each(pos, &mtimer_head)//这里的pos会自动被赋新值
	{
		get_mtimer = list_entry(pos,stimer_t,list);
		if(get_mtimer == NULL)
		{
			return;
		}

		if(get_mtimer->id == timer_id)
		{
			get_mtimer->counter = 0;
			TIMER_DEBUG_PRINTF;
		}
	}
}

int mtimer_requestID(void)
{
	int max_id = 0;
	int ready_id = 0;
	int pre_id = 0,cur_id = 0;
	int new_id = 0;
	

	struct list_head *pos = NULL;
	stimer_t *get_mtimer = NULL;

	
	if(list_empty(&mtimer_head))
	{
		return 1;
	}

	list_for_each(pos, &mtimer_head)//这里的pos会自动被赋新值
	{
		get_mtimer = list_entry(pos,stimer_t,list);
		if(get_mtimer == NULL)
		{
			return -1;
		}

		cur_id = get_mtimer->id;
		//debug_printf("cur_id = %d,pre_id = %d\n",cur_id,pre_id);

		//寻找最大的ID号
		if(cur_id > max_id)
		{
			max_id = cur_id;
		}

		pre_id = cur_id;
	}

	return (max_id + 1);

}

err_t mtimer_unregister(int timer_id)
{
	struct list_head *pos = NULL;
	stimer_t *get_mtimer = NULL;
	pthread_mutex_lock(&timerlist_mutex);
	if(list_empty(&mtimer_head))
	{
		pthread_mutex_unlock(&timerlist_mutex);
		TIMER_DEBUG_PRINTF;
		return ERR_ISEMPTY;
	}
	pthread_mutex_unlock(&timerlist_mutex);
	
	list_for_each(pos, &mtimer_head)//这里的pos会自动被赋新值
	{
		pthread_mutex_lock(&timerlist_mutex);
		get_mtimer = list_entry(pos,stimer_t,list);
		if(get_mtimer == NULL)
		{
			pthread_mutex_unlock(&timerlist_mutex);
			return ERR_NOTEXIST;
		}
		
		timer_debug_printf("get_mtimer->id = %d\n",get_mtimer->id);
		if(get_mtimer->id == timer_id)
		{
			timer_debug_printf("list_del(&(get_mtimer->list));\n");
			list_del(&(get_mtimer->list));
			if(get_mtimer->arg !=  NULL)
				free(get_mtimer->arg);

			if(get_mtimer != NULL)
				free(get_mtimer);
			get_mtimer->arg = NULL;
			get_mtimer = NULL;
			pthread_mutex_unlock(&timerlist_mutex);
			return ERR_OK;
		}
		pthread_mutex_unlock(&timerlist_mutex);
		
	}
	
}

err_t mtimer_register(stimer_t *timer)
{
	debug_printf("timer->data = %d,timer->ref_vals = %d\n",timer->data,timer->ref_vals);
	stimer_t *new_timer = (stimer_t *)malloc(sizeof(stimer_t));
	if(new_timer == NULL)
	{
		timer_debug_printf("malloc timer failed!\n");
		return ERR_MOLLOC_FAILED;
	}

	new_timer->id = timer->id;
	new_timer->data = timer->data;
	new_timer->ref_vals = timer->ref_vals;
	new_timer->function = timer->function;
	new_timer->logout = timer->logout;
	new_timer->counter = timer->counter;
	new_timer->reserd = timer->reserd;
	//debug_printf("new_timer->data = %d,timer->data = %d\n",new_timer->data,timer->data);
	//new_timer->arg 	  = timer->arg;
	//相应函数的参数传入定时器结构本身，方便在相应函数里面操作定时器的相关参数
	new_timer->arg 	  = (void *)new_timer;
	list_add(&(new_timer->list),&mtimer_head);
	new_timer = NULL;
}

stimer_t *mtimer_search(int timer_id)
{
	stimer_t *timer;
	struct list_head *pos = NULL;
	if(list_empty(&mtimer_head))
		return NULL;
	
	TIMER_DEBUG_PRINTF;
	list_for_each(pos, &mtimer_head)//这里的pos会自动被赋新值
	{
		timer = list_entry(pos,stimer_t,list);
		if(timer == NULL)
			return NULL;
		
		if(timer->id == timer_id)
			break;
	}
	TIMER_DEBUG_PRINTF;
	if(timer->id != timer_id)
	{
		timer_debug_printf("timer not exist!\n");
		return NULL;
	}
	TIMER_DEBUG_PRINTF;
	return timer;
}

void TIMER_init(stimer_t *timer)
{
	timer->counter = 0;
}


#if 0

void action(void *arg)
{
	int id = 1;
	mtimer_unregister(id);
	DEBUG_PRINTF;
}
int main(void)
{
	pthread_t tid1,tid2,tid3[2];
	pthread_t tid4;
	
	stimer_t new_timer;
	int timer_id = 1;
	
	int *arg_fd = (int *)malloc(sizeof(int));
	debug_printf("malloc : arg_fd = 0x%x\n",(uint32_t)arg_fd);

	mtimer_init();
	
	*arg_fd = 34;
	new_timer.id = mtimer_requestID();
	new_timer.ref_vals = 1000 * 1000;
	new_timer.function = action;
	new_timer.arg = arg_fd;
	TIMER_init(&new_timer);
	mtimer_register(&new_timer);
	DEBUG_PRINTF;
	test_pthread(tid4);
	DEBUG_PRINTF;
	while(1)
	{
		debug_printf("-----\n");
		sleep(5);
	}
	
	return 0;
}

#endif

