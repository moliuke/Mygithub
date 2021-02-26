#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>   
//#include <sys/io.h>
#include <stddef.h>
#include <ctype.h>


#include "task.h"
#include "../protocol/PTC_init.h"
#include "common.h"
#include "queue.h"
#include "Dev_tcpserver.h"
#include "debug.h"
#include "config.h"
#include "conf.h"
#include "wdt.h"
#include "../clientlist.h"
#include "../threadpool.h"
#include "../module/mtimer.h"
#include "../include/display.h"
#include "../module/image_gif.h"

#include "../Hardware/HW3G_RXTX.h"
#include "../Hardware/HW2G_400.h"


#include "../cache.h"
#include "../Hardware/Data_pool.h"

#include "Protocol_task.h"
#include "../protocol/PTC_common.h"
#include "../protocol/seewor/SWR_protocol.h"


struct timeval StartTime,endTime;


static inline int send_net_port(uint8_t *buf,int len)
{	int ret = 0;
	ret = send(socketobj.new_fd,buf,len,MSG_DONTWAIT);
	if(ret <= 0)
	{
		debug_printf("socketobj.new_fd = %d\n",socketobj.new_fd);
		DEBUG_PRINTF;
	}
	else
	{
		DEBUG_PRINTF;
	}
	return ret;
}
static inline int send_uart_port(uint8_t *buf,int len)
{
	DEBUG_PRINTF;
	return uart_send(xCOM1,buf,len);
}


typedef int (*protocol_ack_t)(uint8_t *buf,int len);
protocol_ack_t ack_back[2] = 
{
	send_net_port,
	send_uart_port
};

static int _protocol_send_back(int table,uint8_t *buf,int len)
{
	debug_printf("socketobj.new_fd = %d\n",socketobj.new_fd);
	ack_back[table](buf,len);
}

int protocol_send_back_uart(int table,uint8_t *buf,int len)
{
	ack_back[table](buf,len);
}


/**
		 协议处理线程，该线程从队列中获取数据，解析数据，并作相应操作
*/
typedef int (*func_t)(user_t *,uint8_t *,uint32_t *);

int back_call(func_t func,user_t *user,uint8_t *input,uint32_t *inputlen)
{
	DEBUG_PRINTF;
	return func(user,input,inputlen);
}



uint8_t Senddata[248] = {
0x1,	0x10,	0x15,	0x0,	0x0,	0x64,	0xc8,	0x0,	0x2,	0x1,	
0x3,	0x2,	0x4,	0x0,	0x0,	0x1b,	0x22,	0x1b,	0x39,	0x32,	
0x1b,	0x3a,	0x34,	0xd3,	0xea,	0xcc,	0xec,	0xc2,	0xb7,	0xbb,	
0xac,	0x1b,	0xa,	0x1b,	0xd,	0x1b,	0x39,	0x32,	0x1b,	0x3a,	
0x34,	0x1b,	0x21,	0xd0,	0xa1,	0xd0,	0xc4,	0xc2,	0xfd,	0xd0,	
0xd0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0xf7,	0x80
};





void *pthread_protocol_task(void *arg)
{
	unsigned int data_len;
	int ret = -1;
	uint32_t PTClenth = 0;
	uint32_t PKGlenth = 0;
	uint8_t *PROTOCOLdata = NULL;
	
	uint8_t *PACKAGEdata = (uint8_t *)malloc(24 * 1024);
	if(PACKAGEdata == NULL)
	{
		perror("malloc PROTOCOLdata");
		exit(1);
	}

	
	pthread_mutex_init(&ackback_mutex,NULL);
	

	while(1)
	{
	
		PKGlenth = 0;
		PTClenth = 0;
		memset(PACKAGEdata,0,24*1024);

		//取链表节点数据长度
		pthread_mutex_lock(&queue_mutex);
		data_len = Get_headlen(QueueHead);
		pthread_mutex_unlock(&queue_mutex);
		if(data_len == 0)
		{
			usleep(5000);
			continue;
		}
		//根据上面读到的数据长度，将数据拷贝出来
		pthread_mutex_lock(&queue_mutex);
		DeQueue(QueueHead,PACKAGEdata,&PKGlenth);
		pthread_mutex_unlock(&queue_mutex);
		if(PKGlenth == 0)
		{
			usleep(5000);
			continue;
		}
		//将上面拷贝出来的拆解处数据拥有者，数据从哪里来回哪里去
		user_t *user = (user_t *)PACKAGEdata;
		PROTOCOLdata = PACKAGEdata + sizeof(user_t);
		PTClenth = PKGlenth - sizeof(user_t);
		if(user->protocol == PROTOCOL_SEEWOR)
		{
			//protocol = swr_protocolProcessor;
			//netSendback = swr_NetSendback;
		}
		//将上面拷贝出来的数包交给协议出来接口去处理(协议接口，不同协议的入口都是一样的，
		//但具体的协议处理函数是不一样的，在系统启动初始化的时候就初始化了)
		uint32_t distTime = 0;
		ret = back_call(protocol_processor,user,PROTOCOLdata,&PTClenth);
		if(ret < 0)
		{
			debug_printf("bad data!\n");
			//continue;
		}

		//数据是串口发来的就用串口发回去，网络发来的就用网络发回去
		pthread_mutex_lock(&ackback_mutex);
		if(ack_back_table == TABLE_NET_PORT)
		{		
			//debug_printf("net send\n");

			debug_printf("PTClenth = %d\n",PTClenth);

			//ret = LEDVersion.NETAckBackEntry(user,PROTOCOLdata,PTClenth);
			ret = NetSendback(user,PROTOCOLdata,PTClenth);
			if(ret < 0)
			{
				debug_printf("the user has exit\n");
			}
		}
		else
		{
			//debug_printf("uart send\n");
			protocol_send_back_uart(ack_back_table,PROTOCOLdata,PTClenth);
		}
		pthread_mutex_unlock(&ackback_mutex);
		
	}
	
}



