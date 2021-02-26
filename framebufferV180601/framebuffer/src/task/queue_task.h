#ifndef __QUEUE_TASK_H
#define __QUEUE_TASK_H
#include <stdio.h>
#include <pthread.h>
extern pthread_mutex_t queue_uart_mutex;

void *pthread_queue_task(void *arg);


#endif