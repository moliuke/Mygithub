#ifndef __MALAYSIA_CUSTOM_H
#define __MALAYSIA_CUSTOM_H
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "../SWR_init.h"
#include "../SWR_protocol.h"


#define  GET_TempAndHum		0x3530
#define  SET_YLightState	0x3532
char *MalaysiaProject(void);
void MalayTimerInit(void);
int MalayExtendInterf(Protocl_t *protocol,unsigned int *len);
int UpdateTempAndHum();
int malay_defmsgdisplay(void);
void SetYLightSate(void);
void xCOM3_init(void);


//int _ProtocolRelation(void *arg1,void *arg2);
//void _ProtocolRelationDestroy(void);
#endif


