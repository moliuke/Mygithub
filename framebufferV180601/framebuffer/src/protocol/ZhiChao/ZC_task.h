#ifndef __ZC_TASK_H
#define __ZC_TASK_H
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "Dev_serial.h"
#include "common.h"
#include "queue.h"


#define		SELECT_STATE		0x00
#define 	RECV_STATE			0x01
#define 	FIND_START_BYTE		0x02
#define 	FIND_DEV_ADDR		0x03
#define 	FIND_END_BYTE		0x04
#define		GET_FRAME_BYTES		0x05

#define 	RECV_BUF_SIZE	(1024 * 4)


void *pthread_ZCuart_monitor_task(void *arg);



#endif

