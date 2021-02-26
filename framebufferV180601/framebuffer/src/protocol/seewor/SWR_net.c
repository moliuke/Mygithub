#include <stdio.h>
#include <errno.h>


#include "../../task/task.h"
#include "SWR_net.h"
#include "common.h"
#include "queue.h"
#include "myerror.h"
#include "Dev_tcpserver.h"
#include "../../clientlist.h"
#include "../PTC_common.h"

static bool net_sendable(int fd)
{
	int ret = -1;
	char *errstr = NULL;
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
		perror("net send");
		return false;
	} 
	else if (ret == 0)
	{
		debug_printf("timeout\n");
		return false;
	}
	
	return true;
	
}


static int netport_sandback(int fd,uint8_t *tx_buf,int tx_len)
{
	int i = 0;  
	int ret = -1;
	int remaind_size = tx_len;
	int offset = 0;
	int send_len = 0;
	bool sendable = false;
	client_t *client = NULL;
	debug_printf("for sendding data=====##########************swr\n");
	debug_printf("netport_sandback : fd = %d,tx_len = %d,tx_buf = 0x%x\n",fd,tx_len,tx_buf[0]);
	for(i = 0 ; i < tx_len ; i ++)
		debug_printf("0x%02x ",tx_buf[i]);
	debug_printf("\n");
	//debug_printf("fd = %d,tx_len = %d,remaind_size = %d\n",fd,tx_len,remaind_size);
	
	//close(fd);

	//此处应该加上监听，否则程序会莫名退出
	while(remaind_size > 0)
	{
		send_len = (remaind_size >= 1024) ? 1024 : remaind_size;
		sendable = net_sendable(fd);
		if(sendable == false)
		{
			return -1;
		}
		ret = send(fd,tx_buf + offset,send_len,MSG_DONTWAIT);
		//ret = send(fd,tx_buf + offset,send_len,MSG_NOSIGNAL);
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
			return -1;
		}
		else
		{
			remaind_size -= ret;
			offset += ret;
			//debug_printf("ret = %d,remaind_size = %d\n",ret,remaind_size);
			if(remaind_size == 0)
				return 0;
		}
	}
	return 0;
}
int swr_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len)
{
	//DEBUG_PRINTF;
	//debug_printf("1protocol_send_back : user->fd = %d\n",user->fd);
	//在别的地方已经回复，此处不需要再回复
	if(user->ackFlag)
		return 0;

	
	DEBUG_PRINTF;
	if(user->type == type_net)
	{
		//debug_printf("2protocol_send_back : user->fd = %d\n",user->fd);
		return netport_sandback(user->fd,tx_buf,tx_len);
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
		SWR_NET_DEBUG_PRINTF;
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
	SWR_NET_DEBUG_PRINTF;
	return RXlen;
}


static void Dtos(char *file,uint8_t *data,uint16_t Len)
{
	FILE *fp = NULL;
	int i = 0;
	uint8_t wdata[2048];
	char DD[4];
	uint8_t Hbit = 0;
	uint8_t Lbit = 0;

	memset(wdata,0,sizeof(wdata));
	for(i = 0 ; i < Len; i++)
	{
		Hbit = ((data[i] & 0xf0) >> 4);
		Lbit = (data[i] & 0x0f);
		wdata[3*i + 0] = (Hbit > 9) ? (Hbit + 0x57) : (Hbit + 0x30);
		wdata[3*i + 1] = (Lbit > 9) ? (Lbit + 0x57) : (Lbit + 0x30);
		wdata[3*i + 2] = 0x20;
	}
	wdata[3 * Len + 0] = '\0';
	wdata[3 * Len + 1] = '\n';

	

	fp = fopen(file,"a");
	fwrite(wdata,1,3 * Len + 2,fp);
	fflush(fp);
	fclose(fp);
	
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


int swr_recv_process(void *arg)
{
	int ret = -1;
	int timer_id = 0;
	int32_t FrameLen = 0;
	uint32_t RXlen = 0;
	client_t *FindUser;
	client_t *user = (client_t *)arg;
	//free(arg);
	SWR_NET_DEBUG_PRINTF;
	uint8_t RXbuf[SWR_RXBUF_SIZE];
	uint8_t *NetData = RXbuf + sizeof(user_t);
	memset(RXbuf,0,sizeof(RXbuf));
	//显科协议或者紫光协议等以0x2开头以0x03结尾的数据帧
	ret = socket_recv(user->new_fd,RXbuf + sizeof(user_t),&RXlen);
	debug_printf("ret = %d,RXlen = %d\n",ret,RXlen);
	//Dtos("/home/LEDscr/log/recv.log",NetData,RXlen);
	//客户端断开连接直接客户管理中的相应的客户信息删除，并将epoll中相应的客户信息删除
	if(ret == ERR_SOCKET_SHUTDOWN)
	{
		pthread_mutex_lock(&user_mutex);
		TCP_UserDEL(user);
		if(user != NULL)
			TCP_EpollCtl(user->new_fd,&user->ev,EPOLL_CTL_DEL);
		pthread_mutex_unlock(&user_mutex);	
		swr_net_debug_printf("user has exit\n");
		free(user);
		user = NULL;
		return 0;
	}

	//接收的数据错误或者数据长度为0，重新初始化客户的忙状态为闲状态
	DEBUG_PRINTF;
	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	
	DEBUG_PRINTF;
	swr_net_debug_printf("ret = %d\n",ret);


	//初始化数据包的头部客户信息
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	pthread_mutex_lock(&user_mutex);
	Cuser.port = user->port;
	Cuser.fd = user->new_fd;
	Cuser.protocol = PROTOCOL_SEEWOR;
	memcpy(Cuser.ip,user->ip,strlen(user->ip));
	pthread_mutex_unlock(&user_mutex);
	swr_net_debug_printf("Cuser.ip = %s-------------user->ip = %s---------\r\n",Cuser.ip,user->ip);
	//将数据包与客户信息合并成一个完整包加载到链表中	
	PNode pnode =  NULL;

	
	//有可能有粘包的现象
	if(NetData[0] == 0x02 && NetData[RXlen - 1] == 0x03)
	{
		SWR_NET_DEBUG_PRINTF;
		FrameLen = SplitPackage(NetData,RXlen);
	}
	else
	{
		SWR_NET_DEBUG_PRINTF;
		FrameLen = RXlen;
	}


	//将粘包拆分成两包数据分别加入到队列中
	swr_net_debug_printf("FrameLen = %d\n",FrameLen);
	if(FrameLen < 0)
	{
		goto FREERESOURCE;
	}

	memcpy(RXbuf,(uint8_t *)&Cuser,sizeof(user_t));

	if(FrameLen == RXlen)
	{
		SWR_NET_DEBUG_PRINTF;
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,RXbuf,RXlen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);
	}
	else if(FrameLen < RXlen)
	{
		uint8_t *SecFrame = RXbuf + FrameLen;
		uint32_t SecLen = RXlen - FrameLen;
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,RXbuf,FrameLen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);

		pthread_mutex_lock(&queue_mutex);
		memset(SecFrame,0x00,sizeof(user_t));
		memcpy(SecFrame,(uint8_t *)&Cuser,sizeof(user_t));
		EnQueue(QueueHead,SecFrame,SecLen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);
	}

	else
	{

	}
	

	//protocol_select = PROTOCOL_SEEWOR;
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



