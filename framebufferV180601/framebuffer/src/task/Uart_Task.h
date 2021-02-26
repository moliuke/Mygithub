#ifndef __UART_TASK_H
#define __UART_TASK_H

#include <stdio.h>
#include "config.h"
#include "debug.h"


#ifdef UART_MONIT_DEBUG
#define UART_MONIT_DEBUG_PRINTF		DEBUG_PRINTF
#define uart_monit_debug_printf		debug_printf
#else
#define UART_MONIT_DEBUG_PRINTF		
#define uart_monit_debug_printf		
#endif


void *pthread_uart_monitor_task(void *arg);

#endif

