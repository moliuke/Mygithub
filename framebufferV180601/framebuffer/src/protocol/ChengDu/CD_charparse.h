#ifndef __CD_CHARPARSE_H
#define __CD_CHARPARSE_H
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "content.h"


#define PARSE_DEBUG

#ifdef PARSE_DEBUG
#define PARSE_DEBUG_PRINTF	DEBUG_PRINTF
#define parse_debug_printf 	debug_printf 
#else
#define PARSE_DEBUG_PRINTF	
#define parse_debug_printf 	
#endif


#define CDCURPLAYSIZE	64
typedef struct
{
	uint8_t  order;
	uint8_t  effectin;
	uint16_t  inspeed;
	uint16_t  stoptime;
	uint16_t strLen;
	uint8_t  playstr[CDCURPLAYSIZE];
}CDCurplay_t;


enum{CD_LSTHEAD = 0,CD_LSTCOUNT,CD_LSTPARSE};

int CD_Lstparsing(ContentList *head,const char *plist);

int CD_PNGMEMmalloc(uint32_t mallocSize);

void CDGetCurPlaying(CDCurplay_t * curplay);

#endif

