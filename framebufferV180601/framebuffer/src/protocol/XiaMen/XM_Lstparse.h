#ifndef __LST_PARSE_H
#define __XM_LST_PARSE_H

#include "content.h"


#define STRLEN	256
typedef struct playstr
{	
	uint8_t  order;
	uint8_t  effectin;
	uint16_t  inspeed;
	uint16_t  stoptime;
	uint16_t strLen;
	uint8_t  playstr[STRLEN];
}Curplay_t;


enum{LST_HEAD = 0,LST_COUNT,LST_PARSE};

int XM_PLst_parsing(ContentList *head,const char *plist);
void XM_GetCurPlaying(Curplay_t * curplay);

#endif

