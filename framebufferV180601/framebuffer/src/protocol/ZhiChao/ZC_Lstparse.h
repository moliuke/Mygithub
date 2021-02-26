#ifndef __ZC_LSTPARSE_H
#define __ZC_LSTPARSE_H
#include "config.h"
#include "content.h"



#define AHSTRLEN	1024
typedef struct playstr
{	
	uint8_t  order;
	uint8_t  effectin;
	uint16_t  inspeed;
	uint16_t strLen;
	uint16_t  stoptime;
	uint8_t  playstr[AHSTRLEN];
}AHCurplay_t;


enum{LST_HEAD = 0,LST_TYPE,LST_COUNT,LST_PARSE};
int ZC_Lstparsing(ContentList *head,const char *plist);
void AHGetCurPlaying(AHCurplay_t * curplay);

#endif


