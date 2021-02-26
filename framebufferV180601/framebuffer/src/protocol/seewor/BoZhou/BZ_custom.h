#ifndef __BZ_PROTOCOL_H
#define __BZ_PROTOCOL_H
#include <stdio.h>
#include "debug.h"
#include "config.h"
#include "../SWR_protocol.h"
#include "../../PTC_FileRxTx.h"


//#define PROJ_ZC
char *BoZhouProject(void);
int ZC_FrameRxFrmUpper2K(Protocl_t *protocol,unsigned int *len);
int ZC_FrameTxToUpper2K(Protocl_t *protocol,unsigned int *len);
//int _ProtocolRelation(void *arg1,void *arg2);
//void _ProtocolRelationDestroy(void);

#endif


