/**********************
 * Copyrigth (c) 2013,lonelyc++
 * All rights reserved
 *
 *   @file:threadpool.h
 *   @desc:threadpool definion
 * @author:liyuanchi
 *   @date:2013-10-19
 **********************/

#ifndef __THREADPOOL_H_
#define __THREADPOOL_H_

#include "myerror.h"


//#define PTHREAD_DEBUG
#ifdef PTHREAD_DEBUG
#define PTHREAD_DEBUG_PRINTF	DEBUG_PRINTF
#define pthread_debug_printf 	debug_printf 
#else
#define PTHREAD_DEBUG_PRINTF	
#define pthread_debug_printf 	 
#endif



typedef struct
{
	int 	(*function)(void *);
	void 	*arg;
} threadpool_task_t;


typedef struct threadpool
{
	pthread_mutex_t lock;// mutex for the taskpool
	pthread_mutex_t thread_counter;//mutex for count the busy thread
	pthread_cond_t queue_not_full;
	pthread_cond_t queue_not_empty;
	pthread_t *threads;
	pthread_t adjust_tid;
	threadpool_task_t *task_queue;
	int min_thr_num;
	int max_thr_num;
	int live_thr_num;
	int busy_thr_num;
	int wait_exit_thr_num;
	int queue_front;
	int queue_rear;
	int queue_size;
	int queue_max_size;
	bool shutdown;
}threadpool_t;

extern threadpool_t *pthreadpool;



/**
 * @function threadpool_create
 * @descCreates a threadpool_t object.
 * @param thr_num  thread num
 * @param max_thr_num  max thread size
 * @param queue_max_size   size of the queue.
 * @return a newly created thread pool or NULL
 */
threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size);

/**
 * @function threadpool_add
 * @desc add a new task in the queue of a thread pool
 * @param pool     Thread pool to which add the task.
 * @param function Pointer to the function that will perform the task.
 * @param argument Argument to be passed to the function.
 * @return 0 if all goes well,else -1
 */
int threadpool_add(threadpool_t *pool, int (*function)(void *arg), void *arg);

/**
 * @function threadpool_destroy
 * @desc Stops and destroys a thread pool.
 * @param pool  Thread pool to destroy.
 * @return 0 if destory success else -1
 */
int threadpool_destroy(threadpool_t *pool);

/**
 * @desc get the thread num
 * @pool pool threadpool
 * @return # of the thread
 */
int threadpool_all_threadnum(threadpool_t *pool);

/**
 * desc get the busy thread num
 * @param pool threadpool
 * return # of the busy thread
 */
int threadpool_busy_threadnum(threadpool_t *pool);

extern threadpool_t *threadpool;


#endif

