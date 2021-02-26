#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "myerror.h"
#include "ZH_net.h"
#include "common.h"
#include "queue.h"
#include "Dev_tcpserver.h"
#include "../../clientlist.h"
#include "../../task/task.h"


static client_t user;

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


static int netport_sandback(int fd,uint8_t *tx_buf,int tx_len)
{
	int i = 0;
	int ret = -1;
	int remaind_size = tx_len;
	int offset = 0;
	int send_len = 0;
	bool sendable = false;
    struct sockaddr_in clientaddr;  
    socklen_t addrlen = sizeof(clientaddr);


	// local.sin_addr.s_addr = inet_addr(argv[1])����
    clientaddr.sin_family = AF_INET;  
    clientaddr.sin_port = user.port;  
    clientaddr.sin_addr.s_addr = htonl(user.ip); 
    //�����޸�Ϊinet_addr
    //clientaddr.sin_addr.s_addr = inet_addr(user.ip);
	
	//debug_printf("for sendding data=====##########************\n");
	debug_printf("netport_sandback : fd = %d\n",fd);
	for(i = 0 ; i < tx_len ; i ++)
		debug_printf("%x, ",tx_buf[i]);
	debug_printf("\n");
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
		ret = sendto(fd,tx_buf + offset,send_len,0,(struct sockaddr*)&clientaddr,addrlen);
			//ret = send(fd,tx_buf + offset,send_len,MSG_DONTWAIT);
			
		//ret = send(fd,tx_buf + offset,send_len,MSG_NOSIGNAL);
		if(ret < 0)
		{
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
int ZH_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len)
{
	//DEBUG_PRINTF;
	//debug_printf("1protocol_send_back : user->fd = %d\n",user->fd);
	//�ڱ�ĵط��Ѿ��ظ����˴�����Ҫ�ٻظ�
	if(user->ackFlag)
		return 0;

	
	ZH_NET_DEBUG_PRINTF;
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


//������������������ݵ�ʱ��ֻ��⿪ͷ�ֽ�0x54���β�ֽ�0x00���п�����0x54��0x00
//֮�仹����0x54��0x00�������������һ����(�γ�����λ���������������)
//����취:��0x54�������0x3�������1024���ֽڣ�ͬʱ��0x3��ǰ��0x2�������1024���ֽ�
static int SplitPackage(char *RXbuf,uint32_t RXlen)
{
	
	uint32_t CheckNum = 0;
	uint32_t counter = 0;

	//������ʾ��������һ����һ�������İ�������
	if(RXlen < 17 || RXbuf[0] != 0x54 || RXbuf[RXlen - 1] != 0x00)
	{
		DEBUG_PRINTF;
		return -1;
	}

	CheckNum = (RXlen < 256) ? RXlen : 256;

	DEBUG_PRINTF;
	
	//��ͷ������0x54
	for(counter = 1 ; counter < CheckNum ; counter++)	
	{
		if(RXbuf[counter] == 0x54)
			break;
	}
	DEBUG_PRINTF;
	if(counter > 17 && counter < CheckNum && RXbuf[counter - 1] == 0x00)
		return counter;
	DEBUG_PRINTF;
	//��β����ǰ��0x54
	for(counter = 0; counter < CheckNum ; counter++)
	{
		if(RXbuf[RXlen - counter - 1] == 0x54)
			break;
	}
	DEBUG_PRINTF;
	debug_printf("RXlen = %d,counter = %d\n",RXlen,counter);
	if(counter > 17 && counter < CheckNum && RXbuf[RXlen - 1 - counter - 1] == 0x00)
		return (RXlen - counter - 1 == 0) ? RXlen : (RXlen - counter);
	DEBUG_PRINTF;
	//����0��ʾֻ��һ�����������ݰ�
	return RXlen;
}



static int socket_recv(int RXfd,char *RXbuf,uint32_t *RXlen)  
{  
	int i = 0,j = 0;
	int offset = 0;
	int FirstFrameFlag = 1;		//��Ƕ�ȡ�ĵ�һ֡1024���ֽ�
	int checkCount = 0;
    int recvlen = 0; 
	int getout = 0;
    struct sockaddr_in clientaddr;  
    socklen_t addrlen = sizeof(clientaddr);
	DEBUG_PRINTF;
	while(1)
	{
		//recvfrom(fd, RXbuf, sizeof(RXbuf),0,(struct sockaddr*)&clientaddr,&addrlen)) <= 0
		recvlen = recvfrom(RXfd, RXbuf + offset, 1024, 0,(struct sockaddr*)&clientaddr,&addrlen);
		if(recvlen == 0)
		{
			return ERR_SOCKET_SHUTDOWN;
		}
		
		if (recvlen < 0)  
		{  
			if(errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK)
				return ERR_ERROR;

			if(offset > 1 && RXbuf[offset - 1] == 0x00)
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

		char ip[24];
		char *p = inet_ntoa(clientaddr.sin_addr);
		user.port = clientaddr.sin_port;
		memset(user.ip,0,sizeof(user.ip));
		memcpy(user.ip,p,strlen(p));
		
		//printf("p = %s,port = %d,strlen(p) = %d,RXfd = %d\n",p,user.port,strlen(p),RXfd);

		if(!FirstFrameFlag)
		{
			offset += recvlen;
			if(offset > 0 && RXbuf[offset - 1] == 0x00)
			{
				*RXlen = offset;
				return ERR_OK;
			}
			continue;
		}


		//�յ���һ֡���ݣ���Ҫ�ҵ���ͷ�ֽ�0x54
		for(i = 0 ; i < recvlen ; i++)
		{
			if(RXbuf[i] == 0x54 || RXbuf[i] == 0x02)  //add 02
				break;
		}

		//��һ���ֽھ��ҵ�0x54
		if(i == 0)
		{
			DEBUG_PRINTF;
			FirstFrameFlag = 0;
			offset = recvlen;
			if(offset > 0 && (RXbuf[offset - 1] == 0x00 || RXbuf[offset - 1] == 0x03))
			{
				DEBUG_PRINTF;
				*RXlen = offset;
				return ERR_OK;
			}
		}
		//�ӵ�0���ֽڿ�ʼ��i���ֽڲ��ҵ�0x54��Ҫ��0x54��ͷ�ĺ������е��ֽ���ǰŲ
		else if(i < recvlen)
		{
			DEBUG_PRINTF;
			for(j = 0 ; j < recvlen - i ; j++)
				RXbuf[j] = RXbuf[j + i];
			
			FirstFrameFlag = 0;
			offset = recvlen - i;
			if(offset > 0 && (RXbuf[offset - 1] == 0x00 || RXbuf[offset - 1] == 0x03))//add 03
			{
				*RXlen = offset;
				return ERR_OK;
			}
		}
		
		//�Ҳ���0x54,�򱾴ν��յ���������Ч
		else
		{
			DEBUG_PRINTF;
			FirstFrameFlag = 1;
			offset = 0;
		}

		recvlen = -1;
		
	}
}  




int ZH_recv_process(void *arg)
{
	int ret = -1;
	int timer_id = 0;
	int32_t FrameLen = 0;
	uint32_t RXlen = 0;
	memset(&user,0,sizeof(user));
	memcpy(&user,(client_t *)arg,sizeof(user));
	uint8_t RXbuf[ZHONGHAI_RXBUF_SIZE];
	uint8_t *NetData = RXbuf + sizeof(user_t);
	DEBUG_PRINTF;
	memset(RXbuf,0,sizeof(RXbuf));
	ret = socket_recv(user.new_fd,RXbuf + sizeof(user_t),&RXlen);

	int i = 0 ;
	for(i = 0 ; i < RXlen ; i++)
		debug_printf("%x ",NetData[i]);
	debug_printf("\n");


	
	DEBUG_PRINTF;
	//�ͻ��˶Ͽ�����ֱ�ӿͻ������е���Ӧ�Ŀͻ���Ϣɾ��������epoll����Ӧ�Ŀͻ���Ϣɾ��
	if(ret == ERR_SOCKET_SHUTDOWN)
	{
		close(user.new_fd);
		return 0;
	}
	DEBUG_PRINTF;
	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	
	//��ʼ�����ݰ���ͷ���ͻ���Ϣ
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	Cuser.port = user.port;
	Cuser.fd = user.new_fd;
	memcpy(Cuser.ip,user.ip,strlen(user.ip));

	//�����ݰ���ͻ���Ϣ�ϲ���һ�����������ص�������	
	PNode pnode =  NULL;
	DEBUG_PRINTF;
	
	//�п�����ճ��������
	if(NetData[0] == 0x54 && NetData[RXlen - 1] == 0x00)
	{
		DEBUG_PRINTF;
		FrameLen = SplitPackage(NetData,RXlen);
	}
	else
	{
		DEBUG_PRINTF;
		FrameLen = RXlen;
	}

	DEBUG_PRINTF;
	//��ճ����ֳ��������ݷֱ���뵽������
	if(FrameLen < 0)
	{
		goto FREERESOURCE;
	}
	DEBUG_PRINTF;
	memcpy(RXbuf,(uint8_t *)&Cuser,sizeof(user_t));

	if(FrameLen == RXlen)
	{
		DEBUG_PRINTF;
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,RXbuf,RXlen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);
	}
	else if(FrameLen < RXlen)
	{
		DEBUG_PRINTF;
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,RXbuf,FrameLen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);

		DEBUG_PRINTF;
		uint8_t *SecFrame = RXbuf + FrameLen;
		uint32_t SecLen = RXlen - FrameLen;
		debug_printf("==============SecFrame[sizeof(user_t)] = 0x%x\n",SecFrame[sizeof(user_t)]);
		memset(SecFrame,0x00,sizeof(user_t));
		memcpy(SecFrame,(uint8_t *)&Cuser,sizeof(user_t));
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,SecFrame,SecLen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);
	}

	else
	{

	}
	DEBUG_PRINTF;
	//printf("haha\n");
	pthread_mutex_lock(&ackback_mutex);
	ack_back_table = TABLE_NET_PORT;
	pthread_mutex_unlock(&ackback_mutex);
	
	return 0;
	//���������ݣ�ͬʱ���³�ʼ���ÿͻ���æ״̬
	FREERESOURCE:
		close(user.new_fd);
		return 0;
}


