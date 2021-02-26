#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>

#include "../../task/task.h"
#include "modbus_net.h"
#include "common.h"
#include "queue.h"
#include "myerror.h"
#include "Dev_tcpserver.h"
#include "../../clientlist.h"
#include "modbus_task.h"

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


static int mdbs_sandback(int fd,uint8_t *tx_buf,int tx_len)
{
	int i = 0;
	int ret = -1;
	int remaind_size = tx_len;
	int offset = 0;
	int send_len = 0;
	bool sendable = false;
	client_t *client = NULL;
	while(remaind_size > 0)
	{
		send_len = (remaind_size >= 1024) ? 1024 : remaind_size;
		sendable = net_sendable(fd);
		if(sendable == false)
		{
			return -1;
		}
		ret = send(fd,tx_buf + offset,send_len,MSG_DONTWAIT);
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
			break;
		}
		else
		{
			remaind_size -= ret;
			offset += ret;
			if(remaind_size == 0)
				return 0;
		}
	}
	return 0;
}


int mdbs_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len)
{
	//DEBUG_PRINTF;
	//debug_printf("1protocol_send_back : user->fd = %d\n",user->fd);
	//在别的地方已经回复，此处不需要再回复
	if(user->ackFlag)
		return 0;

	
	DEBUG_PRINTF;
	if(user->type == type_net)
	{
		return mdbs_sandback(user->fd,tx_buf,tx_len);
	}
	else
	{
		
	}
	return 0;
}



//拆分网络黏包，接收数据的时候只检测开头字节0x02与结尾字节0x03，有可能在0x02与0x03
//之间还存在0x02与0x03，即两个包黏在一起了(治超的上位机存在这样的情况)
//处理办法:从0x2往后查找0x3，最多找1024个字节；同时从0x3往前找0x2，最多找1024个字节
static int SplitPackage(char *RXbuf,uint32_t RXlen)
{
	
	uint32_t CheckNum = 0;
	uint32_t counter = 0;

	//负数表示整包数据一不是一个完整的包，作废
	if(RXbuf[0] != 0x02 || RXbuf[RXlen - 1] != 0x03)
	{
		MDS_NET_DEBUG_PRINTF;
		return -1;
	}

	CheckNum = (RXlen < 256) ? RXlen : 256;

	
	//从头往后找0x3
	for(counter = 1 ; counter < CheckNum ; counter++)	
	{
		if(RXbuf[counter] == 0x03)
			break;
	}
	if(counter < CheckNum)
		return counter + 1;

	//从尾部往前找0x2
	for(counter = 1; counter < CheckNum ; counter++)
	{
		if(RXbuf[RXlen - 1 - counter] == 0x02)
			break;
	}
	if(counter < CheckNum)
		return RXlen - counter - 1;

	//返回0表示只有一个完整的数据包
	MDS_NET_DEBUG_PRINTF;
	return RXlen;
}



int mdbs_recv_process(void *arg)
{
	int ret = -1;
	int timer_id = 0;
	int32_t FrameLen = 0;
	uint32_t RXlen = 0;
	client_t *FindUser;
	client_t *user = (client_t *)arg;
	//free(arg);
	MDS_NET_DEBUG_PRINTF;
	uint8_t RXbuf[MDS_RXBUF_SIZE];
	uint8_t *NetData = RXbuf + sizeof(user_t);
	memset(RXbuf,0,sizeof(RXbuf));
	gettimeofday (&StartTime, NULL);

	ret = ModbusTCPIP_recv(user->new_fd,RXbuf + sizeof(user_t),&RXlen);

	//客户端断开连接直接客户管理中的相应的客户信息删除，并将epoll中相应的客户信息删除
	if(ret == ERR_SOCKET_SHUTDOWN)
	{
		pthread_mutex_lock(&user_mutex);
		TCP_UserDEL(user);
		if(user != NULL)
			TCP_EpollCtl(user->new_fd,&user->ev,EPOLL_CTL_DEL);
		pthread_mutex_unlock(&user_mutex);	
		TCP_EpollCtl(user->new_fd,&user->ev,EPOLL_CTL_DEL);
		mds_net_debug_printf("user has exit\n");
		free(user);
		user = NULL;
		return 0;
	}

	//接收的数据错误或者数据长度为0，重新初始化客户的忙状态为闲状态
	MDS_NET_DEBUG_PRINTF;
	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	
	MDS_NET_DEBUG_PRINTF;
	mds_net_debug_printf("ret = %d\n",ret);


	//初始化数据包的头部客户信息
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	pthread_mutex_lock(&user_mutex);
	Cuser.port = user->port;
	Cuser.fd = user->new_fd;
	memcpy(Cuser.ip,user->ip,strlen(user->ip));
	pthread_mutex_unlock(&user_mutex);
	mds_net_debug_printf("Cuser.ip = %s-------------user->ip = %s---------\r\n",Cuser.ip,user->ip);
	//将数据包与客户信息合并成一个完整包加载到链表中	
	PNode pnode =  NULL;


	memcpy(RXbuf,(uint8_t *)&Cuser,sizeof(user_t));

	pthread_mutex_lock(&queue_mutex);
	EnQueue(QueueHead,RXbuf,RXlen + sizeof(user_t));
	pthread_mutex_unlock(&queue_mutex);

	
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



