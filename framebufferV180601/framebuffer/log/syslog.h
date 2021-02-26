#ifndef _syslog_h
#define _syslog_h

#include <stdio.h>
#include "../include/debug.h"

#define ERROR	0
#define WARN	1
#define	INFO 	2
#define DEBUG	3


typedef struct _dirmsg
{
	char dir[8];
	char maxfile;
	char oldfile;
	uint8_t filenum;

	uint32_t cursize;	
}Logmsg_t;


void LogMsgWrite(uint8_t Grade,char *info);

#endif

