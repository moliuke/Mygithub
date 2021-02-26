#ifndef __PPL_NET_H
#define __PPL_NET_H
#include <stdio.h>
#include "config.h"
#include "../../include/common.h"

//#define PPL_NET_DEBUG
#ifdef PPL_NET_DEBUG
#define PPL_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define ppl_net_debug_printf		debug_printf
#else
#define PPL_NET_DEBUG_PRINTF		
#define ppl_net_debug_printf		
#endif



#define PPL_RXBUF_SIZE 1024 * 24

int PPL_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int PPL_recv_process(void *arg);

#endif

