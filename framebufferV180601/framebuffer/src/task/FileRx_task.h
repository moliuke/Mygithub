#ifndef __FileRx_task_h
#define __FileRx_task_h

#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "common.h"



typedef struct _filetxrx
{
	user_t			*user;
	uint8_t 	 	startByte;
	uint16_t 		frameType;
	uint16_t 		devAddr;
	uint8_t 		*data;
	uint16_t 		parity;
	uint16_t		length;
	uint8_t		 	endByte;	
}FileTXRX_t;


void *File_recv_task(void *arg);


#endif

