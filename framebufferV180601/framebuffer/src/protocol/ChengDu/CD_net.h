#ifndef __CD_NET_H
#define __CD_NET_H

#include <stdio.h>
#include "debug.h"
#include "common.h"
#include "../PTC_init.h"





//#define CD_NET_DEBUG
#ifdef CD_NET_DEBUG
#define CD_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define cd_net_debug_printf		debug_printf
#else
#define CD_NET_DEBUG_PRINTF		
#define cd_net_debug_printf		
#endif



#define CD_RXBUF_SIZE 1024 * 24

int CD_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int CD_recv_process(void *arg);

#endif

