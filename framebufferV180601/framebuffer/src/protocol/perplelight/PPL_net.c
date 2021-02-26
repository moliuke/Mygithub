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



//������������������ݵ�ʱ��ֻ��⿪ͷ�ֽ�0x02���β�ֽ�0x03���п�����0x02��0x03
//֮�仹����0x02��0x03�������������һ����(�γ�����λ���������������)
//����취:��0x2�������0x3�������1024���ֽڣ�ͬʱ��0x3��ǰ��0x2�������1024���ֽ�
static int SplitPackage(char *RXbuf,uint32_t RXlen)
{
	
	uint32_t CheckNum = 0;
	uint32_t counter = 0;

	//������ʾ��������һ����һ�������İ�������
	if(RXbuf[0] != 0x02 || RXbuf[RXlen - 1] != 0x03)
	{
		PPL_NET_DEBUG_PRINTF;
		return -1;
	}

	CheckNum = (RXlen < 256) ? RXlen : 256;

	
	//��ͷ������0x3
	for(counter = 1 ; counter < CheckNum ; counter++)	
	{
		if(RXbuf[counter] == 0x03)
			break;
	}
	if(counter < CheckNum)
		return counter + 1;

	//��β����ǰ��0x2
	for(counter = 1; counter < CheckNum ; counter++)
	{
		if(RXbuf[RXlen - 1 - counter] == 0x02)
			break;
	}
	if(counter < CheckNum)
		return RXlen - counter - 1;

	//����0��ʾֻ��һ�����������ݰ�
	PPL_NET_DEBUG_PRINTF;
	return RXlen;
}


static int socket_recv(int RXfd,char *RXbuf,uint32_t *RXlen)  
{  
	int i = 0;
	uint32_t offset = 0;
	int FirstFrameFlag = 1;		//��Ƕ�ȡ�ĵ�һ֡1024���ֽ�
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
			//�����һ�ζ��������ݵ����һ���ֽ��Ƿ���03,��03��ʾ�ϴ��ѽ�����ϣ��˳�
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

			//�ǵ�һ֡���ݣ�������ǵ�һ֡���ݣ����һ�����һ�������Ƿ���0x03
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


			//��һ֡���ݣ��п��ܵ�һ֡���ݾ���һ�����������ݣ�Ҳ�п��ܲ�����
			
			//����һ֡���ݵĿ�ͷ�ֽ�0x02���������recvlen���ֽڣ��Ҳ�������Ϊ�����Ǵ����
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


			//�����ߵ�����˵��0x02�ֽڿ϶��ҵ��������ڵ�checkCount���ֽڲ��ҵ�
			//���Խ�0x02�����������ȫ����ǰŲ
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

			
			//�ڵ�һ֡���ݽ������Ҫ���һ�����һ���ֽ��Ƿ���0x03,����˵����һ֡����
			//��һ�����������ݣ�ֱ�ӷ���
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

	//�Կ�Э������Ϲ�Э�����0x2��ͷ��0x03��β������֡
	ret = socket_recv(user->new_fd,RXbuf + sizeof(user_t),&RXlen);


	int i = 0;
	debug_printf("PPL recv data---------------------\n");
	for(i = 0 ; i < RXlen ; i++)
		debug_printf("0x%x ",NetData[i]);
	debug_printf("\n\n\n");

	//�ͻ��˶Ͽ�����ֱ�ӿͻ������е���Ӧ�Ŀͻ���Ϣɾ��������epoll����Ӧ�Ŀͻ���Ϣɾ��
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

	//���յ����ݴ���������ݳ���Ϊ0�����³�ʼ���ͻ���æ״̬Ϊ��״̬
	PPL_NET_DEBUG_PRINTF;
	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	
	PPL_NET_DEBUG_PRINTF;
	ppl_net_debug_printf("ret = %d\n",ret);


	//��ʼ�����ݰ���ͷ���ͻ���Ϣ
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	pthread_mutex_lock(&user_mutex);
	Cuser.port = user->port;
	Cuser.fd = user->new_fd;
	memcpy(Cuser.ip,user->ip,strlen(user->ip));
	pthread_mutex_unlock(&user_mutex);
	ppl_net_debug_printf("Cuser.ip = %s-------------user->ip = %s---------\r\n",Cuser.ip,user->ip);
	//�����ݰ���ͻ���Ϣ�ϲ���һ�����������ص�������	
	PNode pnode =  NULL;

	
	//�п�����ճ��������
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


	//��ճ����ֳ��������ݷֱ���뵽������
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
	//���������ݣ�ͬʱ���³�ʼ���ÿͻ���æ״̬
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





