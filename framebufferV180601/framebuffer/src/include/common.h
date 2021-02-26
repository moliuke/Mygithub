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
	uint8_t 			type;		//������ߴ���
			
	char 				ip[20];		//����IP��ַ
	uint16_t 			port;		//����˿ں�

	uint8_t 			uartPort;

	int 				fd;			//������

	int 				busyState;	//æ״̬

	int 				recvflag;	//����������״̬��־������ֻҪ���ջ����������ݿɶ�
									//�ͻᴥ��select���أ�Ϊ�˷�ֹͬһ�����ݱ���δ��������ô˱���
	int 				deadtime;	//���ڷǻ�Ծ״̬��ʱ�䣬���û����ڷǻ�Ծ״̬ʱ�䳬��
									//һ��ʱ�佫�������

	int 				ackFlag;

	int 				id;

	int 				protocol;

	struct list_head 	list;

	pthread_mutex_t 	mutex;
	
}user_t;



extern int ack_back_table;
extern pthread_mutex_t ackback_mutex;
#endif


