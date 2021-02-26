#ifndef __SDL_TASK_H
#define __SDL_TASK_H
#include <stdio.h>
#include "config.h"
#include "debug.h"

#define RXBUF_SIZE 1024 * 24

void *pthread_SDL_task(void *arg);
#endif


