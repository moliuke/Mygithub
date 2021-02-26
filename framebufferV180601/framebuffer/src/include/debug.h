#ifndef __DEBUG_H
#define __DEBUG_H

#include "config.h"

//#define CONFIG_VIDEO_TEST		1
#define CONFIG_CHARACTER_TEST	1
#define CONFIG_SCREEN_TEST		1
//#define CONFIG_BITMAP_TEST		1
//#define CONFIG_TCPIP_TEST		1
//#define CONFIG_SERIAL			1

int debug_printf_tofile(const char *format,...);
int DEBUG_PRINTF_ToFile(const char *func,int line);

#define DEBUG_MSG_TO_FILE DEBUG_PRINTF_ToFile(__func__,__LINE__)
//#define debug_msg_to_file debug_printf_tofile(debugfile,fmt, ## args)

//#define __debug 1
#ifdef __debug
#define __debug_printf printf("[ %s ] printf in line [ %d ]\n",__func__,__LINE__)
#else 
#define __debug_printf 
#endif
//this will decide wether our printf message is print on the screen or not!!!!!
#define PRINTF_COPY 1

#ifdef PRINTF_DEB
#ifdef DEBUG_TO_FILE
#define DEBUG_PRINTF DEBUG_MSG_TO_FILE
#else
#define DEBUG_PRINTF printf("[ %s ] printf in line [ %d ]\n",__func__,__LINE__)
#endif
#else
#define DEBUG_PRINTF 
#endif
//int printf(const char *format,...);



#if 0
#define FILE_OPEN  printf("open file in : [ %s ] printf in line [ %d ]\n",__func__,__LINE__)
#define FILE_CLOSE printf("close file in : [ %s ] printf in line [ %d ]\n",__func__,__LINE__)
#else
#define FILE_OPEN  
#define FILE_CLOSE 
#endif


//#define __DEBUG
//#define __func_DEBUG
#ifdef  __DEBUG  
#ifdef  __func_DEBUG
#define debug_printf(fmt, args...) printf("%s: "fmt,__func__, ## args)
#else
#ifdef DEBUG_TO_FILE
#define debug_printf(fmt, args...) debug_printf_tofile(fmt, ## args)
#else
#define debug_printf(fmt, args...) printf(fmt, ## args)
#endif
#endif
#else   
#define debug_printf(fmt, args...) { } 
#endif  //end DBG_AP_STA



#endif

