#ifndef __TIME_H
#define __TIME_H

#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "config.h"


int set_sys_time(uint8_t *timestr,uint8_t len);
int get_sys_time(uint8_t *timestr,uint8_t *len);
#endif
