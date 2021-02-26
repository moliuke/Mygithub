#ifndef __MODBUS_TASK_H
#define __MODBUS_TASK_H
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "common.h"
#include "queue.h"
#include "modbus_config.h"
//#define MDBS_TASK_DEBUG
#ifdef MDBS_TASK_DEBUG
#define MDBS_TASK_DEBUG_PRINTF			DEBUG_PRINTF
#define mdbs_task_debug_printf			debug_printf
#else
#define MDBS_TASK_DEBUG_PRINTF			
#define mdbs_task_debug_printf			
#endif



#define 	RECV_BUF_SIZE	(1024 * 4)
void *pthread_MODBUS_monitor_task(void *arg);
int ModbusTCPIP_recv(int RXfd,char *RXbuf,uint32_t *RXlen);
#endif

