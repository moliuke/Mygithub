
#include "debug.h"
#include <stdarg.h>


int debug(void)
{
	return 0;
}


int DEBUG_PRINTF_ToFile(const char *func,int line)
{	
	FILE *fp = NULL;
	//printf("filename = %s\n",debugfile);
	fp = fopen(debugfile,"a");
	if(fp == NULL)
	{
		return -1;
	}
	fprintf(fp,"[ %s ] printf in line [ %d ]\n",func,line);
	fflush(fp);
	fclose(fp);
	return 0;
}

#if 1
int debug_printf_tofile(const char *format,...)
{
	FILE *fp = NULL;
	fp = fopen(debugfile,"a");
	if(fp == NULL)
	{
		return -1;
	}
	va_list ap;
	va_start(ap,format);
	vfprintf(fp,format,ap); // Ð´ÎÄ¼þ
	va_end(ap); 
	fflush(fp);
	fclose(fp);
	return 0;
	
}
#endif

