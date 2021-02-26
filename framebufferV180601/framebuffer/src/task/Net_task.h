#ifndef __NET_TASK_H
#define __NET_TASK_H
#include <stdio.h>
#include "config.h"
#include "debug.h"


//#define NETTASK_DEBUG
#ifdef NETTASK_DEBUG
#define NETTASK_DEBUG_PRINTF		DEBUG_PRINTF
#define nettask_debug_printf		debug_printf
#else
#define NETTASK_DEBUG_PRINTF		
#define nettask_debug_printf		
#endif










#define RXBUF_SIZE 1024 * 24

void *pthread_monitor_task(void *arg);
#endif

