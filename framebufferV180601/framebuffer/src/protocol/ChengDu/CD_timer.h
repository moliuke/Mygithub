#ifndef __CD_TIMER_H
#define __CD_TIMER_H
#include <stdio.h>
#include "config.h"
#define EXCEPT_LIST	0
#define NORMAL_LIST	1

#define NET_EXCEPT 	0
#define NET_NORMAL	1
extern char Playlst[8];

void CD_timerInit(void);
void CD_ClearTimer(void);
void CD_SetDefList(uint32_t Swidth,uint32_t Sheight);
#endif

