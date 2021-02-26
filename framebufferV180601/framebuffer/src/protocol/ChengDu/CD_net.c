#include <stdio.h>
#include <errno.h>

#include "aes/aes_interf.h"
#include "../../task/task.h"
#include "CD_net.h"
#include "common.h"
#include "queue.h"
#include "myerror.h"
#include "Dev_tcpserver.h"
#include "../../clientlist.h"
#include "../PTC_common.h"

static bool net_sendable(int fd)
{
	int ret = -1;
	client_t *client = NULL;
	fd_set write_set;
	FD_ZERO(&write_set);
	FD_SET(fd, &write_set);
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
	debug_printf("for sendding data=====##########***********tx_len = %d\n",tx_len);
	//debug_printf("netport_sandback : fd = %d\n",fd);
	for(i = 0 ; i < tx_len ; i ++)
	{
		debug_printf("0x%x ",tx_buf[i]);
	}
	debug_printf("\n\n");
	//debug_printf("fd = %d,tx_len = %d,remaind_size = %d\n",fd,tx_len,remaind_size);
	

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
int CD_NetSendback(user_t *user,uint8_t *tx_buf,int tx_len)
{
	//DEBUG_PRINTF;
	//debug_printf("1protocol_send_back : user->fd = %d\n",user->fd);
	//在别的地方已经回复，此处不需要再回复
	if(user->ackFlag)
		return 0;

	debug_printf("tx_len = %d\n",tx_len);
	
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
		CD_NET_DEBUG_PRINTF;
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
	CD_NET_DEBUG_PRINTF;
	return RXlen;
}


static int CD_Check0X02And0X03(uint8_t *input,uint8_t ILen,uint8_t *output,uint8_t *OLen)
{
	int i = 0;
	int IsEscOK = 0;
	int m = 0;
	
	for(i = 0 ; i < ILen ; i ++ )
	{
		//检测数据是否包含0x02或者0x03,包含两者之一则数据错误，放弃数据
		if(input[i] == 0x02 || input[i] == 0x03)
		{
			debug_printf("the data recv contains 0x02 or 0x03\n");
			return -1;
		}
	
		//检测是否包含0x1B
		if(input[i] == 0x1B)
		{
			IsEscOK = 1;
			continue;
		}
	
	
		if(!IsEscOK)
			output[m] = input[i];
		else
		{
			IsEscOK = 0;
			output[m] = input[i]+0x1B;
		}
		m++;
	}
	
	*OLen = m;
	return m;
}


static int CD_Ackback(user_t *user,uint8_t *recvData,uint16_t Len)
{
	uint8_t i = 0;
	uint8_t aesLen = 0;
	uint8_t aesData[32];
	uint8_t output[28];
	uint8_t OLen = 0;
	
	//uint8_t ackback[24] = {0x2, 0x30, 0x30, 0x31, 0x30, 0xcd, 0x32, 0xd9, 0xb6, 0xc8, 0x5a, 
	//	0x9a, 0x39, 0x30, 0x57, 0x20, 0xba, 0xb5, 0xe2, 0x7c, 0xd0, 0x6d, 0x5f, 0x3};
	uint8_t ackback[24] = {0x2, 0x30, 0x30, 0x73, 0xe0, 0xf0, 0xd8, 0xa2, 0x86, 0x55, 
		0xf6, 0x83, 0xe5, 0xb9, 0xb6, 0x3d, 0x95, 0xcb, 0x18, 0x19, 0xec, 0x03};
	//debug_printf("Check if this frame is file frame\n");
	//for(i = 0 ; i < 27 ; i++)
	//	debug_printf("0x%x ",recvData[i]);
	//debug_printf("\n");

	uint8_t CheckLen = (Len > 24) ? 24 : Len;
	CD_Check0X02And0X03(recvData + 3,CheckLen,output,&OLen);

	uint16_t decLen = (OLen > 16) ? 16 : OLen;
	AES_ECB_decrypt(output,decLen,aesData,&aesLen,1);
	debug_printf("aesData[0] = 0x%02x,aesData[1] = 0x%02x\n\n\n",aesData[0],aesData[1]);
	if(aesData[0] != 0x31 || aesData[1] != 0x30) 
		return -1;

	debug_printf("This frame is File frame\n");
	user->ackFlag = 1;
	return send(user->fd,ackback,22,MSG_DONTWAIT);
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


int CD_recv_process(void *arg)
{
	int i = 0;
	int ret = -1;
	int timer_id = 0;
	int32_t FrameLen = 0;
	uint32_t RXlen = 0;
	client_t *FindUser;
	client_t *user = (client_t *)arg;
	//free(arg);
	uint8_t RXbuf[CD_RXBUF_SIZE];
	uint8_t *NetData = RXbuf + sizeof(user_t);
	memset(RXbuf,0,sizeof(RXbuf));
	gettimeofday (&StartTime, NULL);
	DEBUG_PRINTF;
	//显科协议或者紫光协议等以0x2开头以0x03结尾的数据帧
	ret = socket_recv(user->new_fd,RXbuf + sizeof(user_t),&RXlen);

	DEBUG_PRINTF;
	//客户端断开连接直接客户管理中的相应的客户信息删除，并将epoll中相应的客户信息删除
	if(ret == ERR_SOCKET_SHUTDOWN)
	{
		pthread_mutex_lock(&user_mutex);
		TCP_UserDEL(user);
		if(user != NULL)
			TCP_EpollCtl(user->new_fd,&user->ev,EPOLL_CTL_DEL);
		pthread_mutex_unlock(&user_mutex);	
		free(user);
		user = NULL;
		return 0;
	}
	DEBUG_PRINTF;
	debug_printf("ret = %d,RXlen = %d\n",ret,RXlen);
	//接收的数据错误或者数据长度为0，重新初始化客户的忙状态为闲状态
	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	
	DEBUG_PRINTF;
	//初始化数据包的头部客户信息
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	pthread_mutex_lock(&user_mutex);
	Cuser.port = user->port;
	Cuser.fd = user->new_fd;
	memcpy(Cuser.ip,user->ip,strlen(user->ip));
	pthread_mutex_unlock(&user_mutex);
	DEBUG_PRINTF;
	//将数据包与客户信息合并成一个完整包加载到链表中	
	PNode pnode =  NULL;

	
	//有可能有粘包的现象
	if(NetData[0] == 0x02 && NetData[RXlen - 1] == 0x03)
	{
		CD_NET_DEBUG_PRINTF;
		FrameLen = SplitPackage(NetData,RXlen);
	}
	else
	{
		CD_NET_DEBUG_PRINTF;
		FrameLen = RXlen;
	}

	DEBUG_PRINTF;
	//将粘包拆分成两包数据分别加入到队列中
	debug_printf("FrameLen = %d\n",FrameLen);
	if(FrameLen < 0)
	{
		goto FREERESOURCE;
	}
	
#if 1
	int ii = 0;
	unsigned short CRC16 = 0;
	unsigned short enLen = 0; 
	char Send[36] = {0x02,0x30,0x30};
	char enc[32] = {0x02,0x30,0x30,0x30,0x34,0x30};
	AES_ECB_encrypt(enc + 3,3,Send + 3,&enLen);

	CRC16 = XKCalculateCRC(Send+1,2+enLen);
	debug_printf("priorty = 0x%x\n",CRC16);
	
	for(ii = 0 ; ii < enLen + 3 ; ii++)
		debug_printf("%02x ",(uint8_t)Send[ii]);
	debug_printf("----------\n\n");

	char DD[16];
	unsigned short LL = 0;
	AES_ECB_decrypt(Send + 3,16,DD,&LL,1);
	for(ii = 0 ; ii < LL ; ii++)
		debug_printf("%02x, ",DD[ii]);
	debug_printf("===\n\n");
	//exit(1);
#endif

	//因为文件数据量较大，20k一帧，还涉及加密解密，还有校验、转义
	//为了快速完成文件的传输，这里接收到数据后先解密前面的部分字节，判断是文件数据帧
	//就直接回应上位机
	CD_Ackback(&Cuser,NetData,FrameLen);
	memcpy(RXbuf,(uint8_t *)&Cuser,sizeof(user_t));

	if(FrameLen == RXlen)
	{
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,RXbuf,RXlen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);
	}
	else if(FrameLen < RXlen)
	{
		pthread_mutex_lock(&queue_mutex);
		EnQueue(QueueHead,RXbuf,FrameLen + sizeof(user_t));
		pthread_mutex_unlock(&queue_mutex);

		uint8_t *SecFrame = RXbuf + FrameLen;
		uint32_t SecLen = RXlen - FrameLen;
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
		DEBUG_PRINTF;
		pthread_mutex_lock(&user_mutex);	
		FindUser = (client_t *)TCPIP_FindUser(user->new_fd);
		if(FindUser != NULL)
			FindUser->busy = 0;
		pthread_mutex_unlock(&user_mutex);
		DEBUG_PRINTF;
		free(user);
		DEBUG_PRINTF;
		user = NULL;
		return 0;
}


