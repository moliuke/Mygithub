#ifndef __SWR_NET_H
#define __SWR_NET_H
#include <stdio.h>
#include "debug.h"
#include "../../include/common.h"




//#define SWR_NET_DEBUG
#ifdef SWR_NET_DEBUG
#define SWR_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define swr_net_debug_printf		debug_printf
#else
#define SWR_NET_DEBUG_PRINTF		
#define swr_net_debug_printf		
#endif



#define SWR_RXBUF_SIZE 1024 * 24

int swr_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
	int swr_recv_process(void *arg);


#endif

