#ifndef swr_init_h
#define swr_init_h
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "../PTC_init.h"


char *SWRGetProjectStr(void);
char *SWRGetProtocolStr(void);

//int PlaylistItemDecoder(ContentList *head,char *itemContent,uint8_t ItemOder);
int (*itemDecoder)(ContentList *,char *,uint8_t);

//void ProtocolRelation(void *arg1,void *arg2);
void ProtocolRelationDestroy(void);

#endif

