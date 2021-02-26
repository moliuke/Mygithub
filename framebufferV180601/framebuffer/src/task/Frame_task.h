#ifndef __FRAME_TASK_H
#define __FRAME_TASK_H
#include <stdio.h>
#include "config.h"
#include "debug.h"

 
//#define FRAME_TASK_DEBUG

#ifdef FRAME_TASK_DEBUG
#define FTASK_DEBUGPRINTF	DEBUG_PRINTF
#define frame_debug_printf 	debug_printf 
#else
#define FTASK_DEBUGPRINTF	
#define frame_debug_printf 	
#endif


void *pthread_Mdbsframebreak_task(void *arg);
void *pthread_framebreak_task(void *arg);

#endif

