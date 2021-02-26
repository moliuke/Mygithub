#ifndef __MTIMER_H
#define __MTIMER_H

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

typedef signed char		int8_t;
typedef unsigned char 	uint8_t;
typedef signed short	int16_t;
typedef unsigned short	uint16_t;
typedef	signed int 		int32_t; 
typedef unsigned int	uint32_t;

int set_sys_time(uint8_t *timestr,uint8_t len);
int get_sys_time(uint8_t *timestr,uint8_t *len);

#endif

