#include <stdio.h>
#include <stdbool.h>


#include "debug.h"
#include "config.h"
#include "common.h"
#include "display.h"
#include "Dev_tcpserver.h"
#include "XM_Lstparse.h"
#include "XM_display.h"
#include "../../include/content.h"

#include "../../task/task.h"
#include "XM_net.h"
#include "common.h"
#include "queue.h"
#include "myerror.h"
#include "Dev_tcpserver.h"
#include "../../clientlist.h"

static bool net_sendable(int fd)
{
	int ret = -1;
	fd_set write_set;
	FD_ZERO(&write_set);
	FD_SET(fd, &write_set);
	client_t *client = NULL;
	struct timeval timeout = {0,10000};
	ret = select(fd + 1,NULL,&write_set,NULL, &timeout);
	if (ret < 0)
	{
		pthread_mutex_lock(&user_mutex);	
		client = TCPIP_FindUser(fd);
		if(client != NULL)
		{
			TCP_UserDEL(client);
			TCP_EpollCtl(client->new_fd,&client->ev,EPOLL_CTL_DEL);
		}
		pthread_mutex_unlock(&user_mutex);	
		perror("net send select");
		return false;
	} 
	else if (ret == 0)
	{
		debug_printf("timeout\n");
		return false;
	}
	
	return true;
	
}

static int XM_send(int fd,uint8_t *tx_buf,int tx_len)
{
	int i = 0;
	int ret = -1;
	int offset = 0;
	int send_len = 0;
	bool sendable = false;
	debug_printf("for sendding data=====##########************\n");
	client_t *client = NULL;
	sendable = net_sendable(fd);
	XM_NET_DEBUG_PRINTF;
	if(sendable == false)
	{
		XM_NET_DEBUG_PRINTF;
		debug_printf("This fd can not write at the moment!!\n");
		return -1;
	}

	
	ret = send(fd,tx_buf,tx_len,MSG_DONTWAIT);
	if(ret < 0)
	{
		pthread_mutex_lock(&user_mutex);
		client = TCPIP_FindUser(fd);
		if(client != NULL)
		{
			TCP_UserDEL(client);
			TCP_EpollCtl(client->new_fd,&client->ev,EPOLL_CTL_DEL);
		}
		pthread_mutex_unlock(&user_mutex);	
		perror("send");
	}

	if(ret == tx_len)
		return 0;

	return -1;
}



int XM_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len)
{
	XM_NET_DEBUG_PRINTF;
	//debug_printf("1protocol_send_back : user->fd = %d\n",user->fd);
	if(user->type == type_net)
	{
		//debug_printf("2protocol_send_back : user->fd = %d\n",user->fd);
		return XM_send(user->fd,tx_buf,tx_len);
	}
	else
	{
		
	}
	return 0;
}

//拆分黏包
static int SplitPackage(char *RXbuf,uint32_t RXlen)
{
	int i = 0;
	for(i = 1 ; i < RXlen - 1 ; i++)
		if(RXbuf[i] == 0x03)
			break;
	if(i == RXlen - 1)
	{
		return RXlen;
	}
	
	return i+1;
}


static int socket_recv(int RXfd,char *RXbuf,uint32_t *RXlen)  
{  
	int i = 0,j = 0;
	uint32_t offset = 0;
	int FirstFrameFlag = 1;		//标记读取的第一帧1024个字节
	int checkCount = 0;
    int recvlen = 0; 
	int getout = 0;
	while(1)
	{
		recvlen = recv(RXfd, RXbuf + offset, 1024, 0);
		if(recvlen == 0)
		{
			DEBUG_PRINTF;
			return ERR_SOCKET_SHUTDOWN;
		}
		
		if (recvlen < 0)  
		{  
			DEBUG_PRINTF;
			if(errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK);
				return ERR_ERROR;
			
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
			{
				*RXlen = offset;
				return ERR_OK;
			}
			DEBUG_PRINTF;
			getout++;
			if(getout > 2048)
				return ERR_ERROR;
			usleep(5);
			continue;
		}
		DEBUG_PRINTF;
		
		getout = 0;

		int i = 0; 
		for(i = 0 ; i < recvlen ; i++)
			debug_printf("%02x ",(uint8_t)(RXbuf + offset)[i]);
		debug_printf("\n");

		//程序运行到这里说明是接收到了有效的数据
		//如果不是第一帧,说明第一帧已经确认了0x02的存在，一旦检测到最后一个字节是0x03，
		//则认为本次接收结束,否则进入下一次循环接收数据
		if(!FirstFrameFlag)
		{
			offset += recvlen;
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
			{
				*RXlen = offset;
				return ERR_OK;
			}
			continue;
		}

		//收到第一帧数据，需要找到开头字节0x02
		for(i = 0 ; i < recvlen ; i++)
		{
			if(RXbuf[i] == 0x02)
				break;
		}

		//第一个字节就找到0x02
		if(i == 0)
		{
			DEBUG_PRINTF;
			FirstFrameFlag = 0;
			offset = recvlen;
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
			{
				*RXlen = offset;
				return ERR_OK;
			}
		}

		//从第0个字节开始隔i个字节才找到0x02，要将0x02开头的后面所有的字节往前挪
		else if(i < recvlen)
		{
			DEBUG_PRINTF;
			for(j = 0 ; j < recvlen - i ; j++)
				RXbuf[j] = RXbuf[j + i];
			
			FirstFrameFlag = 0;
			offset = recvlen - i;
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
			{
				*RXlen = offset;
				return ERR_OK;
			}
		}


		//找不到0x02,则本次接收到的数据无效
		else
		{
			DEBUG_PRINTF;
			FirstFrameFlag = 1;
			offset = 0;
		}

		recvlen = -1;

	}
} 


int XM_recv_process(void *arg)
{
	int ret = -1;
	int timer_id = 0;
	int32_t FrameLen = 0;
	uint32_t RXlen = 0;
	client_t *FindUser;
	client_t *user = (client_t *)arg;
	uint8_t RXbuf[XM_RXBUF_SIZE];
	uint8_t *NetData = RXbuf + sizeof(user_t);
	memset(RXbuf,0,sizeof(RXbuf));
	gettimeofday (&StartTime, NULL);

	//显科协议或者紫光协议等以0x2开头以0x03结尾的数据帧
	ret = socket_recv(user->new_fd,RXbuf + sizeof(user_t),&RXlen);

	int i = 0;
	debug_printf("XM recv data---------------------\n");
	for(i = 0 ; i < RXlen ; i++)
		debug_printf("0x%x ",NetData[i]);
	debug_printf("\n\n\n");

	//客户端断开连接直接客户管理中的相应的客户信息删除，并将epoll中相应的客户信息删除
	if(ret == ERR_SOCKET_SHUTDOWN)
	{
		pthread_mutex_lock(&user_mutex);
		TCP_UserDEL(user);
		if(user != NULL)
			TCP_EpollCtl(user->new_fd,&user->ev,EPOLL_CTL_DEL);
		pthread_mutex_unlock(&user_mutex);	
		xm_net_debug_printf("user has exit\n");
		if(user != NULL)
			free(user);
		user = NULL;
		return 0;
	}

	//接收的数据错误或者数据长度为0，重新初始化客户的忙状态为闲状态
	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	

	//初始化数据包的头部客户信息
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	pthread_mutex_lock(&user_mutex);
	Cuser.port = user->port;
	Cuser.fd = user->new_fd;
	memcpy(Cuser.ip,user->ip,strlen(user->ip));
	pthread_mutex_unlock(&user_mutex);
	xm_net_debug_printf("Cuser.ip = %s-------------user->ip = %s---------\r\n",Cuser.ip,user->ip);
	//将数据包与客户信息合并成一个完整包加载到链表中	
	PNode pnode =  NULL;

	
	FrameLen = SplitPackage(NetData,RXlen);
	memcpy(RXbuf,(uint8_t *)&Cuser,sizeof(user_t));

	pthread_mutex_lock(&queue_mutex);
	EnQueue(QueueHead,RXbuf,RXlen + sizeof(user_t));
	pthread_mutex_unlock(&queue_mutex);

	//存在黏包
	if(FrameLen < RXlen)
	{
		uint8_t *SecFrame = RXbuf + FrameLen;
		uint32_t SecLen = RXlen - FrameLen;
		memset(SecFrame,0x00,sizeof(user_t));
		memcpy(SecFrame,(uint8_t *)&Cuser,sizeof(user_t));
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,SecFrame,SecLen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);
	}

	
	pthread_mutex_lock(&ackback_mutex);
	ack_back_table = TABLE_NET_PORT;
	pthread_mutex_unlock(&ackback_mutex);
	//加载完数据，同时重新初始化该客户的忙状态
	FREERESOURCE:
		pthread_mutex_lock(&user_mutex);	
		FindUser = (client_t *)TCPIP_FindUser(user->new_fd);
		if(FindUser != NULL)
			FindUser->busy = 0;
		pthread_mutex_unlock(&user_mutex);
		free(user);
		user = NULL;
		return 0;
}





