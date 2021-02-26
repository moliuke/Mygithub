#ifndef __UG_NET_H
#define __UG_NET_H
#include <stdio.h>
//#include "config.h"
#include "../../include/common.h"

//#define UG_NET_DEBUG
#ifdef UG_NET_DEBUG
#define UG_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define ug_net_debug_printf		debug_printf
#else
#define UG_NET_DEBUG_PRINTF		
#define ug_net_debug_printf		
#endif



#define UG_RXBUF_SIZE 1024 * 24

int UG_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int UG_recv_process(void *arg);

#endif

