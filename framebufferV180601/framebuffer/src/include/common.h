#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <pthread.h>
#include "config.h"
#include "mylist.h"
#include "common.h"


#define 	USER_IS_FREE		0
#define     USER_IS_BUSY		1

#define TABLE_NET_PORT	0x00
#define TABLE_UART_PORT	0x01

#define UDP_SELECT		0x00
#define TCP_SELECT      0x01


typedef struct __user
{
	uint8_t 			type;		//网络或者串口
			
	char 				ip[20];		//网络IP地址
	uint16_t 			port;		//网络端口号

	uint8_t 			uartPort;

	int 				fd;			//描述符

	int 				busyState;	//忙状态

	int 				recvflag;	//允许进入接收状态标志，由于只要接收缓冲区有数据可读
									//就会触发select返回，为了防止同一包数据被多次触发而设置此变量
	int 				deadtime;	//处于非活跃状态的时间，当用户处于非活跃状态时间超过
									//一定时间将被清理掉

	int 				ackFlag;

	int 				id;

	int 				protocol;

	struct list_head 	list;

	pthread_mutex_t 	mutex;
	
}user_t;



extern int ack_back_table;
extern pthread_mutex_t ackback_mutex;
#endif


