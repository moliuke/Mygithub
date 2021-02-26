#ifndef __LST_PARSE_H
#define __LST_PARSE_H

#include "content.h"


enum{LST_HEAD = 0,LST_COUNT,LST_PARSE};

int PPL_PLst_parsing(ContentList *head,const char *plist);


#endif

