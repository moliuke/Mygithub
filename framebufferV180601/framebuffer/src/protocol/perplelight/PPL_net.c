#include <stdio.h>
#include <stdbool.h>


#include "debug.h"
#include "config.h"
#include "common.h"
#include "display.h"
#include "Dev_tcpserver.h"
#include "PPL_Lstparse.h"
#include "PPL_datapool.h"
#include "PPL_display.h"
#include "../../include/content.h"

#include "../../task/task.h"
#include "PPL_net.h"
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
	
	struct timeval timeout = {0,10000};
	ret = select(fd + 1,NULL,&write_set,NULL, &timeout);
	if (ret < 0)
	{
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

static int PLL_send(int fd,uint8_t *tx_buf,int tx_len)
{
	int i = 0;
	int ret = -1;
	int offset = 0;
	int send_len = 0;
	bool sendable = false;
	debug_printf("for sendding data=====##########************\n");
	//debug_printf("netport_sandback : fd = %d\n",fd);

	sendable = net_sendable(fd);
	PPL_NET_DEBUG_PRINTF;
	if(sendable == false)
	{
		PPL_NET_DEBUG_PRINTF;
		debug_printf("This fd can not write at the moment!!\n");
		return -1;
	}

	
	ret = send(fd,tx_buf,tx_len,MSG_DONTWAIT);
	if(ret < 0)
	{
		PPL_NET_DEBUG_PRINTF;
		perror("send");
	}

	if(ret == tx_len)
		return 0;

	return -1;
}



int PPL_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len)
{
	PPL_NET_DEBUG_PRINTF;
	//debug_printf("1protocol_send_back : user->fd = %d\n",user->fd);
	if(user->type == type_net)
	{
		//debug_printf("2protocol_send_back : user->fd = %d\n",user->fd);
		return PLL_send(user->fd,tx_buf,tx_len);
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
		PPL_NET_DEBUG_PRINTF;
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
	PPL_NET_DEBUG_PRINTF;
	return RXlen;
}


static int socket_recv(int RXfd,char *RXbuf,uint32_t *RXlen)  
{  
	int i = 0;
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
			PPL_NET_DEBUG_PRINTF;
			return ERR_SOCKET_SHUTDOWN;
		}
		
		if (recvlen < 0)  
		{  
			if(errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK)
				return ERR_ERROR;
			//检查上一次读到的数据的最后一个字节是否是03,是03表示上次已接收完毕，退出
			//debug_printf("offset = %d\n",offset);
			//if(offset > 0)
			//	debug_printf("fd = %d,offset = %d,recvbuf[offset - 1] = 0x%x\n",RXfd,offset,RXbuf[offset - 1]);
			
			if(offset <= 0 || RXbuf[offset - 1] != 0x03)
			{
				//debug_printf("RXbuf[0] = 0x%x,offset = %d,recvlen = %d,getout = %d\n",RXbuf[0],offset,recvlen,getout);
				getout++;
				if(getout > 2048)
					return -1;
				usleep(5);
				continue;
				//return ERR_ERROR;
			}
			PPL_NET_DEBUG_PRINTF;
			*RXlen = offset;


			//for(i = 0 ; i < offset ; i ++)
			//	debug_printf("0x%x ",RXbuf[i]);
			//debug_printf("\n");
			return ERR_OK;
		}


		
		if(recvlen > 0)
		{
			PPL_NET_DEBUG_PRINTF;
			getout = 0;

			for(i = 0 ; i < recvlen ; i ++)
				ppl_net_debug_printf("0x%x ",RXbuf[i]);
			ppl_net_debug_printf("\n");

			//非第一帧数据，如果不是第一帧数据，检查一下最后一个数据是否是0x03
			if(!FirstFrameFlag)
			{
				ppl_net_debug_printf("recvlen = %d\n",recvlen);
				offset += recvlen;

				if(RXbuf[offset - 1] == 0x03)
				{
					*RXlen = offset;
					return ERR_OK;
				}
				
				recvlen = -1;
				continue;
			}
			

			PPL_NET_DEBUG_PRINTF;
			FirstFrameFlag = 0;


			//第一帧数据，有可能第一帧数据就是一包完整的数据，也有可能不完整
			
			//查找一帧数据的开头字节0x02，往后查找recvlen个字节，找不到就认为数据是错误的
			while(checkCount < recvlen)
			{
				if(RXbuf[checkCount] == 0x02)
					break;
				checkCount = checkCount + 1;
			}
			PPL_NET_DEBUG_PRINTF;
			if(checkCount == recvlen)
			{
				PPL_NET_DEBUG_PRINTF;
				FirstFrameFlag = 1;
				checkCount = 0;
				offset = 0;
				continue;
			}
			PPL_NET_DEBUG_PRINTF;


			//程序走到这里说明0x02字节肯定找到，但是在第checkCount个字节才找到
			//所以讲0x02及后面的数据全部往前挪
			int i = 0;
			int byteNum = recvlen - checkCount;
			if(checkCount)
			{
				for(i = 0 ; i < byteNum ; i ++)
					RXbuf[i] = RXbuf[i+checkCount];
				offset = byteNum;
				//recvlen = -1;
				//continue;
			}
			else
				offset = recvlen;

			
			//在第一帧数据接收完后要检查一下最后一个字节是否是0x03,是则说明第一帧数据
			//是一包完整的数据，直接返回
			if(RXbuf[offset - 1] == 0x03)
			{
				PPL_NET_DEBUG_PRINTF;
				*RXlen = offset;
				return ERR_OK;
			}
			//DEBUG_PRINTF;
			//offset += recvlen;
			recvlen = -1;
		}
	}
}  

int PPL_recv_process(void *arg)
{
	int ret = -1;
	int timer_id = 0;
	int32_t FrameLen = 0;
	uint32_t RXlen = 0;
	client_t *FindUser;
	client_t *user = (client_t *)arg;
	//free(arg);
	PPL_NET_DEBUG_PRINTF;
	uint8_t RXbuf[PPL_RXBUF_SIZE];
	uint8_t *NetData = RXbuf + sizeof(user_t);
	memset(RXbuf,0,sizeof(RXbuf));
	gettimeofday (&StartTime, NULL);

	//显科协议或者紫光协议等以0x2开头以0x03结尾的数据帧
	ret = socket_recv(user->new_fd,RXbuf + sizeof(user_t),&RXlen);


	int i = 0;
	debug_printf("PPL recv data---------------------\n");
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
		ppl_net_debug_printf("user has exit\n");
		if(user != NULL)
			free(user);
		user = NULL;
		return 0;
	}

	//接收的数据错误或者数据长度为0，重新初始化客户的忙状态为闲状态
	PPL_NET_DEBUG_PRINTF;
	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	
	PPL_NET_DEBUG_PRINTF;
	ppl_net_debug_printf("ret = %d\n",ret);


	//初始化数据包的头部客户信息
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	pthread_mutex_lock(&user_mutex);
	Cuser.port = user->port;
	Cuser.fd = user->new_fd;
	memcpy(Cuser.ip,user->ip,strlen(user->ip));
	pthread_mutex_unlock(&user_mutex);
	ppl_net_debug_printf("Cuser.ip = %s-------------user->ip = %s---------\r\n",Cuser.ip,user->ip);
	//将数据包与客户信息合并成一个完整包加载到链表中	
	PNode pnode =  NULL;

	
	//有可能有粘包的现象
	if(NetData[0] == 0x02 && NetData[RXlen - 1] == 0x03)
	{
		PPL_NET_DEBUG_PRINTF;
		FrameLen = SplitPackage(NetData,RXlen);
	}
	else
	{
		PPL_NET_DEBUG_PRINTF;
		FrameLen = RXlen;
	}


	//将粘包拆分成两包数据分别加入到队列中
	ppl_net_debug_printf("FrameLen = %d\n",FrameLen);
	if(FrameLen < 0)
	{
		goto FREERESOURCE;
	}


	memcpy(RXbuf,(uint8_t *)&Cuser,sizeof(user_t));

	if(FrameLen == RXlen)
	{
		PPL_NET_DEBUG_PRINTF;
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,RXbuf,RXlen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);
	}
	else if(FrameLen < RXlen)
	{
		PPL_NET_DEBUG_PRINTF;
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,RXbuf,FrameLen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);

		PPL_NET_DEBUG_PRINTF;
		uint8_t *SecFrame = RXbuf + FrameLen;
		uint32_t SecLen = RXlen - FrameLen;
		ppl_net_debug_printf("==============SecFrame[sizeof(user_t)] = 0x%x\n",SecFrame[sizeof(user_t)]);
		memset(SecFrame,0x00,sizeof(user_t));
		memcpy(SecFrame,(uint8_t *)&Cuser,sizeof(user_t));
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,SecFrame,SecLen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);
	}

	else
	{

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





