#ifndef __ZH_NET_H
#define __ZH_NET_H
#include <stdio.h>
#include "debug.h"
#include "config.h"
#include "common.h"


//#define SWR_NET_DEBUG
#ifdef SWR_NET_DEBUG
#define ZH_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define zh_net_debug_printf		debug_printf
#else
#define ZH_NET_DEBUG_PRINTF		
#define zh_net_debug_printf		
#endif


#define ZHONGHAI_RXBUF_SIZE 1024

int ZH_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int ZH_recv_process(void *arg);
#endif

