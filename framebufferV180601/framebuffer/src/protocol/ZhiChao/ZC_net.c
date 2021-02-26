#include <stdio.h>
#include <errno.h>


#include "../../task/task.h"
#include "ZC_net.h"
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


static int netport_sandback(int fd,uint8_t *tx_buf,int tx_len)
{
	int i = 0;
	int ret = -1;
	int remaind_size = tx_len;
	int offset = 0;
	int send_len = 0;
	bool sendable = false;
	client_t *client = NULL;
	debug_printf("for sendding data=====##########************\n");
	//debug_printf("netport_sandback : fd = %d\n",fd);
	for(i = 0 ; i < tx_len ; i ++)
	{
		debug_printf("0x%x ",tx_buf[i]);
	}
	//debug_printf("fd = %d,tx_len = %d,remaind_size = %d\n",fd,tx_len,remaind_size);
	

	//�˴�Ӧ�ü��ϼ�������������Ī���˳�
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
			break;
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
int ZC_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len)
{
	//DEBUG_PRINTF;
	//debug_printf("1protocol_send_back : user->fd = %d\n",user->fd);
	//�ڱ�ĵط��Ѿ��ظ����˴�����Ҫ�ٻظ�
	if(user->ackFlag)
		return 0;

	
	ZC_NET_DEBUG_PRINTF;
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
		ZC_NET_DEBUG_PRINTF;
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
	ZC_NET_DEBUG_PRINTF;
	return RXlen;
}

static int socket_recv(int RXfd,char *RXbuf,uint32_t *RXlen)  
{  
	int i = 0,j = 0;
	uint32_t offset = 0;
	int FirstFrameFlag = 1;		//��Ƕ�ȡ�ĵ�һ֡1024���ֽ�
	int checkCount = 0;
    int recvlen = 0; 
	int getout = 0;
	while(1)
	{
		recvlen = recv(RXfd, RXbuf + offset, 1024, 0);
		if(recvlen == 0)
			return ERR_SOCKET_SHUTDOWN;
		
		if (recvlen < 0)  
		{  
			if(errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK);
				return ERR_ERROR;
			
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
			{
				*RXlen = offset;
				return ERR_OK;
			}
			getout++;
			if(getout > 2048)
				return ERR_ERROR;
			usleep(5);
			continue;
		}
		
		getout = 0;

		int i = 0; 
		for(i = 0 ; i < recvlen ; i++)
			debug_printf("%02x ",(uint8_t)(RXbuf + offset)[i]);
		debug_printf("\n");

		//�������е�����˵���ǽ��յ�����Ч������
		//������ǵ�һ֡,˵����һ֡�Ѿ�ȷ����0x02�Ĵ��ڣ�һ����⵽���һ���ֽ���0x03��
		//����Ϊ���ν��ս���,���������һ��ѭ����������
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

		//�յ���һ֡���ݣ���Ҫ�ҵ���ͷ�ֽ�0x02
		for(i = 0 ; i < recvlen ; i++)
		{
			if(RXbuf[i] == 0x02)
				break;
		}

		//��һ���ֽھ��ҵ�0x02
		if(i == 0)
		{
			FirstFrameFlag = 0;
			offset = recvlen;
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
			{
				*RXlen = offset;
				return ERR_OK;
			}
		}

		//�ӵ�0���ֽڿ�ʼ��i���ֽڲ��ҵ�0x02��Ҫ��0x02��ͷ�ĺ������е��ֽ���ǰŲ
		else if(i < recvlen)
		{
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


		//�Ҳ���0x02,�򱾴ν��յ���������Ч
		else
		{
			FirstFrameFlag = 1;
			offset = 0;
		}

		recvlen = -1;

	}
} 


int ZC_recv_process(void *arg)
{
	int ret = -1;
	int timer_id = 0;
	int32_t FrameLen = 0;
	uint32_t RXlen = 0;
	client_t *FindUser;
	client_t *user = (client_t *)arg;
	//free(arg);
	ZC_NET_DEBUG_PRINTF;
	uint8_t RXbuf[ZC_RXBUF_SIZE];
	uint8_t *NetData = RXbuf + sizeof(user_t);
	memset(RXbuf,0,sizeof(RXbuf));
	gettimeofday (&StartTime, NULL);

	ret = socket_recv(user->new_fd,RXbuf + sizeof(user_t),&RXlen);

	//�ͻ��˶Ͽ�����ֱ�ӿͻ������е���Ӧ�Ŀͻ���Ϣɾ��������epoll����Ӧ�Ŀͻ���Ϣɾ��
	if(ret == ERR_SOCKET_SHUTDOWN)
	{
		pthread_mutex_lock(&user_mutex);
		TCP_UserDEL(user);
		if(user != NULL)
			TCP_EpollCtl(user->new_fd,&user->ev,EPOLL_CTL_DEL);
		pthread_mutex_unlock(&user_mutex);	
		if(user != NULL)
		{
			free(user);
			user = NULL;
		}
		zc_net_debug_printf("user has exit\n");
		return 0;
	}

	//���յ����ݴ���������ݳ���Ϊ0�����³�ʼ���ͻ���æ״̬Ϊ��״̬
	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	
	//��ʼ�����ݰ���ͷ���ͻ���Ϣ
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	pthread_mutex_lock(&user_mutex);
	Cuser.port = user->port;
	Cuser.fd = user->new_fd;
	memcpy(Cuser.ip,user->ip,strlen(user->ip));
	pthread_mutex_unlock(&user_mutex);
	zc_net_debug_printf("Cuser.ip = %s-------------user->ip = %s---------\r\n",Cuser.ip,user->ip);
	//�����ݰ���ͻ���Ϣ�ϲ���һ�����������ص�������	
	PNode pnode =  NULL;

	
	FrameLen = SplitPackage(NetData,RXlen);
	memcpy(RXbuf,(uint8_t *)&Cuser,sizeof(user_t));

	pthread_mutex_lock(&queue_mutex);
	EnQueue(QueueHead,RXbuf,RXlen + sizeof(user_t));
	pthread_mutex_unlock(&queue_mutex);

	//������
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


