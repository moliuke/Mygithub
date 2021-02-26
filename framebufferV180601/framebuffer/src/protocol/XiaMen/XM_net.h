#ifndef __XM_NET_H
#define __XM_NET_H
#include <stdio.h>
#include "config.h"
#include "../../include/common.h"

//#define XM_NET_DEBUG
#ifdef XM_NET_DEBUG
#define XM_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define xm_net_debug_printf		debug_printf
#else
#define XM_NET_DEBUG_PRINTF		
#define xm_net_debug_printf		
#endif



#define XM_RXBUF_SIZE 1024 * 24

int XM_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int XM_recv_process(void *arg);

#endif

