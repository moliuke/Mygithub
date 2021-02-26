#ifndef __ZH_CHARPARSE_H
#define __ZH_CHARPARSE_H

#include <stdio.h>
#include "debug.h"
#include "config.h"
#include "content.h"

int ZH_Lstparsing(uint8_t *charStr,uint16_t Len,ContentList *head);
int ZH_PLst_parsing(ContentList *head,const char *plist);


#endif


