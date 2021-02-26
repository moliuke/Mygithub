#ifndef __MODBUS_NET_H
#define __MODBUS_NET_H
#include <stdio.h>
#include "debug.h"
#include "../../include/common.h"




//#define MDS_NET_DEBUG
#ifdef MDS_NET_DEBUG
#define MDS_NET_DEBUG_PRINTF		DEBUG_PRINTF
#define mds_net_debug_printf		debug_printf
#else
#define MDS_NET_DEBUG_PRINTF		
#define mds_net_debug_printf		
#endif



#define MDS_RXBUF_SIZE 1024 * 24

int mdbs_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len);
int mdbs_recv_process(void *arg);

#endif

