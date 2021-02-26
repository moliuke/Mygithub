#ifndef __JX_NET_H
#define __JX_NET_H
#include <stdio.h>
#include "config.h"
#include "../../include/common.h"

//#define JX_NET_DEBUG
#ifdef JX_NET_DEBUG
#define JX_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define jx_net_debug_printf		debug_printf
#else
#define JX_NET_DEBUG_PRINTF		
#define jx_net_debug_printf		
#endif



#define JX_RXBUF_SIZE 1024 * 24

int JX_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int JX_recv_process(void *arg);

#endif

