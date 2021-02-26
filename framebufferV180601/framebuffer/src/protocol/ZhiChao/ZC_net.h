#ifndef __ZC_NET_H
#define __ZC_NET_H

#include <stdio.h>
#include "debug.h"
#include "../../include/common.h"




//#define SWR_NET_DEBUG
#ifdef ZC_NET_DEBUG
#define ZC_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define zc_net_debug_printf		debug_printf
#else
#define ZC_NET_DEBUG_PRINTF		
#define zc_net_debug_printf		
#endif



#define ZC_RXBUF_SIZE 1024 * 24

int ZC_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int ZC_recv_process(void *arg);

#endif

