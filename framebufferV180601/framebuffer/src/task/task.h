#ifndef __TASK_H
#define __TASK_H

#include "display.h"
#include "Dev_serial.h"
#include "content.h"
#include "character.h"
#include "bitmap.h"
#include "config.h"
#include "common.h"


#ifdef CONFIG_VIDEO_TEST
void *pthread_SDL_task(void *arg)
#endif

extern pthread_mutex_t content_lock;
//extern pthread_cond_t user_cond;
extern pthread_mutex_t queue_mutex;
extern pthread_mutex_t queue_uart_mutex;
extern int displayFlag;
//int protocol_send_back(user_t *user,uint8_t *tx_buf,int tx_len);
void TASK_SystemStart(void);
void TASK_SystemExit(void);
void SetModulePowerUp(void);

#endif
