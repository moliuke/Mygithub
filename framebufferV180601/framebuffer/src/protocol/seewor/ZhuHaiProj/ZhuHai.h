#ifndef __ZHUHAI_H
#define __ZHUHAI_H

#include "config.h"

#define DefMSG 		"Item00=2,1,0,1,1,\\C000020\\Fh64\\T255000000000\\B000000000000\\U��ȫ��һ\\C000105\\Fh64\\T255000000000\\B000000000000\\UԤ��Ϊ��"

#define 	SCREEN_STATUS_OPEN		1
#define 	SCREEN_STATUS_CLOSE 	0

#define 	STATUS_ENABLE			1
#define 	STATUS_DISABLE			0

#define 	TIMEOUT_CLOSELED		0
#define 	TIMEOUT_DSPDEFMSG		1

char *ZhuHaiProject(void);
int ZhuHai_GetCommunitStatus(Protocl_t *protocol,unsigned int *len);
int ZhuHai_SetPresetPlayLst(Protocl_t *protocol,unsigned int *len);
void ZHUHAI_timerInit(void);


#endif

