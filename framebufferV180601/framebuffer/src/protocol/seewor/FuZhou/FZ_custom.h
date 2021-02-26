#ifndef __FZ_PROTOCOL_H
#define __FZ_PROTOCOL_H
#include <stdio.h>
#include "debug.h"
#include "config.h"
#include "../SWR_protocol.h"


int GetTimingConfig(void);
int FZ_extendcmd(Protocl_t *protocol,unsigned int *len);
void FZTimer_init(void);
char *FuZhouProject(void);

#endif