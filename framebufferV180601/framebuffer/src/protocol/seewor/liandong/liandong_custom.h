#ifndef __LIANDONG_CUSTOM_H
#define __LIANDONG_CUSTOM_H
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "../SWR_init.h"
#include "../SWR_protocol.h"


int test_pthread_create(void);

int data_process(uint8_t *buf, uint16_t len);
char *liandongProject(void);


//int _ProtocolRelation(void *arg1,void *arg2);
//void _ProtocolRelationDestroy(void);
#endif


