#ifndef __UART_UPDATE_TASK_H
#define __UART_UPDATE_TASK_H

#include <stdio.h>
#include "config.h"
#include "debug.h"
//#include "../Hardware/HW3G_RXTX.h"

#define UPDATE_DEBUG
#ifdef UPDATE_DEBUG
#define UPDATE_DEBUG_PRINTF		DEBUG_PRINTF
#define update_debug_printf		debug_printf
#else
#define UPDATE_DEBUG_PRINTF		
#define update_debug_printf		
#endif 



typedef struct 
{
	uint16_t 	 cmdID;
	uint16_t     devID;
}head_id;

typedef struct 
{
	head_id 		head;
	uint8_t			*data;
	uint16_t 		parity;
	uint16_t		length;
}protcl_msg;


//#define UART_DEV_CMT 0x3030
//#define 





void *pthread_update_task(void *arg);
#endif

