#include "FileRx_task.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "../threadpool.h"
#include "Dev_tcpserver.h"
#include "debug.h"
#include "common.h"
#include "conf.h"
#include "../Hardware/HW3G_RXTX.h"
#include "../protocol/PTC_init.h"
#include "../protocol/PTC_FileRxTx.h"
#include "../protocol/PTC_common.h"


pthread_mutex_t sersock_mutex;

//Socketobj_t serverStruct;
#define BACKLOG 8
#define BUF_SIZE 2176

client_t NewClient;



struct sockaddr_in servaddr, cliaddr;    
struct rlimit rt;   

#if 1

int epollFd = -1,serverFd = -1;
static struct sockaddr_in addr;
static int OnlyClientFlag = 0;
#define FILE_PORT		6168
#define CLIENT_NUM		1


static int ConfigFileOps(void)
{
	if(access(config_sh,F_OK) >= 0)
		system(config_sh);
	
	conf_file_write(_cls,"netport","ip","192.168.1.11");
	conf_file_write(_cls,"netport","netmask","255.255.255.0");
	conf_file_write(_cls,"netport","gateway","192.168.1.1");
	conf_file_write(_cls,"netport","port","5168");
	conf_file_write(f_cls,"netport","ip","192.168.1.11");
	conf_file_write(f_cls,"netport","netmask","255.255.255.0");
	conf_file_write(f_cls,"netport","gateway","192.168.1.1");
	conf_file_write(f_cls,"netport","port","5168");

	int fd = -1;
	char Wcontent[256];
	char ip[24],netmask[24],gateway[24];
	memset(ip,0,sizeof(ip));
	memset(netmask,0,sizeof(netmask));
	memset(gateway,0,sizeof(gateway));
	memset(Wcontent,0,sizeof(Wcontent));
	conf_file_read(ConFigFile,"netport","ip",ip);
	conf_file_read(ConFigFile,"netport","netmask",netmask);
	conf_file_read(ConFigFile,"netport","gateway",gateway);
	sprintf(Wcontent,"#!/bin/sh\n\nifconfig eth0 %s netmask %s up >/dev/null 2>&1\n/sbin/route add default gw %s\n",ip,netmask,gateway);
	fd = open(IPCONFIG,O_RDWR | O_CREAT,0744);
	if(fd < 0)
		return -1;
	
	debug_printf("Wcontent = %s\n",Wcontent);
	
	lseek(fd,0,SEEK_SET);
	write(fd,Wcontent,strlen(Wcontent));
	close(fd);
	//system(IPCONFIG);
	
	return 0;
}




static int socketcreate(void)
{
    int on = -1;  
    serverFd = socket(AF_INET, SOCK_STREAM, 0);  
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)  
    {  
        debug_printf("setsockopt failed %s\n", strerror(errno));  
        return -1;  
    }  

	//��socket����Ϊ��������ʽ	
    TCP_SetNonBlocking(serverFd);
	
    memset(&addr, 0, sizeof(addr));  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(FILE_PORT);  
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	
    if (bind(serverFd, (struct sockaddr *) &addr, sizeof(addr)) == -1)  
    {  
        debug_printf("bind port %d failed %s\n", serverFd, strerror(errno));  
        return -1;  
    }  
	
    if (listen(serverFd, CLIENT_NUM) == -1)  
    {  
        debug_printf("listen failed %s\n", strerror(errno));  
        return -1;  
    }  
}


static void EpollInit(void)
{
	//����epoll_event�ṹ��ı���,ev����ע���¼� 
    struct epoll_event ev;
	
	//�������ڴ���accept��epollר�õ��ļ�������  
    epollFd = epoll_create(CLIENT_NUM); 
	
	//������Ҫ������¼���ص��ļ�������  
    ev.data.fd = serverFd; 	
	
	//����Ҫ������¼����� 
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; 	
	
	//ע��epoll�¼�  
    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &ev); 
}


int EpollWait(struct epoll_event *events,int maxEV,int timeout)
{
	return epoll_wait(epollFd, events, maxEV, timeout);
}

int ClientAccept(client_t *Client)  
{  
    struct sockaddr_in client_addr;  
    socklen_t len = sizeof(client_addr);  
	
    memset(&client_addr, 0, sizeof(client_addr));  
    Client->new_fd = accept(serverFd, (struct sockaddr *)&client_addr, &len);  

	int iplen = strlen(inet_ntoa(client_addr.sin_addr));
	Client->port = ntohs(client_addr.sin_port);
	memset(Client->ip,0,sizeof(Client->ip));
	memcpy(Client->ip,inet_ntoa(client_addr.sin_addr),iplen);
	Client->ip[strlen(Client->ip)] = '\0';
	debug_printf("Client->port = %d,Client->ip = %s,%d\n",Client->port,Client->ip,strlen(Client->ip));
	
    return Client->new_fd;  
} 





static void NetInit(void)
{
	socketcreate();
	EpollInit();
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
			return ERR_SOCKET_SHUTDOWN;
		}
		
		if (recvlen < 0)  
		{  
			if(errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK)
				return ERR_ERROR;
			
			if(offset <= 0 || RXbuf[offset - 1] != 0x03)
			{
				getout++;
				if(getout > 2048)
					return -1;
				usleep(5);
				continue;
			}
			*RXlen = offset;

			return ERR_OK;
		}

		if(recvlen > 0)
		{
			getout = 0;
			//�ǵ�һ֡���ݣ�������ǵ�һ֡���ݣ����һ�����һ�������Ƿ���0x03
			if(!FirstFrameFlag)
			{
				offset += recvlen;

				if(RXbuf[offset - 1] == 0x03)
				{
					*RXlen = offset;
					return ERR_OK;
				}
				
				recvlen = -1;
				continue;
			}
			
			FirstFrameFlag = 0;


			//��һ֡���ݣ��п��ܵ�һ֡���ݾ���һ�����������ݣ�Ҳ�п��ܲ�����
			//����һ֡���ݵĿ�ͷ�ֽ�0x02���������recvlen���ֽڣ��Ҳ�������Ϊ�����Ǵ����
			while(checkCount < recvlen)
			{
				if(RXbuf[checkCount] == 0x02)
					break;
				checkCount = checkCount + 1;
			}
			if(checkCount == recvlen)
			{
				FirstFrameFlag = 1;
				checkCount = 0;
				offset = 0;
				continue;
			}


			//�����ߵ�����˵��0x02�ֽڿ϶��ҵ��������ڵ�checkCount���ֽڲ��ҵ�
			//���Խ�0x02�����������ȫ����ǰŲ
			int i = 0;
			int byteNum = recvlen - checkCount;
			if(checkCount)
			{
				for(i = 0 ; i < byteNum ; i ++)
					RXbuf[i] = RXbuf[i+checkCount];
				offset = byteNum;
			}
			else
				offset = recvlen;

			
			//�ڵ�һ֡���ݽ������Ҫ���һ�����һ���ֽ��Ƿ���0x03,����˵����һ֡����
			//��һ�����������ݣ�ֱ�ӷ���
			if(RXbuf[offset - 1] == 0x03)
			{
				*RXlen = offset;
				return ERR_OK;
			}
			recvlen = -1;
		}
	}
} 





static int FileRx_2Kperframe(FileTXRX_t *FileTXRX,unsigned int *len)
{
	int retvals = -1;
	uint16_t CurOffset = 0;
	uint8_t FileNameLen = 0;
	char FileNameLenStr[4];
	char FileName[48];
	char *FrameIdStr = NULL;
	uint32_t FrameId = 0;
	uint32_t FrameLen = 0;
	char *FrameContent = NULL;
	//ȡ�ļ�������
	CurOffset += 2;
	memset(FileNameLenStr,0,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,FileTXRX->data + CurOffset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//ȡ�ļ���
	CurOffset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,FileTXRX->data + CurOffset,FileNameLen);
	FileName[FileNameLen] = '\0';

	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//�����λ���ϴ��������ļ�·����û����Ӧ�ļ��оʹ�����Ӧ���ļ���
	char *p = NULL;
	chdir(sys_dir);
	p = strchr(FileName,'/');
	if(p != NULL)
	{
		DEBUG_PRINTF;
		mkdirs(FileName);
		chdir(sys_dir);
	}

	//֡ƫ����
	CurOffset += FileNameLen;
	FrameIdStr = FileTXRX->data + CurOffset;
	FrameId = (FrameIdStr[0] - 0x30) * 1000 + (FrameIdStr[1] - 0x30) * 100 + 
			  (FrameIdStr[2] - 0x30) * 10 + (FrameIdStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",FrameIdStr[0],FrameIdStr[1],FrameIdStr[2],FrameIdStr[3]);
	debug_printf("FrameId = %d\n",FrameId);

	//֡���ݳ���
	CurOffset += 4;
	FrameLen = FileTXRX->length - CurOffset;

	//֡����
	FrameContent = FileTXRX->data + CurOffset;

	//�����ļ�·��
	char FilePwd[64];
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	
	debug_printf("FilePwd = %s\n",FilePwd);

	debug_printf("protocol->usermsg->ip = %s\n",FileTXRX->user->ip);
	//��ʼ���û�����
	FILEUser_t FileUser;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,FileTXRX->user->type,FileTXRX->user->ip,FileTXRX->user->port,FileTXRX->user->uartPort);
	DEBUG_PRINTF;
	//֡���ݴ��ļ�
	retvals = FRTx_FileFrameRx2K(&FileUser,FilePwd,FrameContent,FrameLen,FrameId);
	if(retvals < 0)
		goto EXCEPTION;

	//���ս���
	if(retvals == 1)
	{
		if(strncmp(FileName,"config/cls.conf",15) == 0)
		{
			debug_printf("FileName = %s\n",FileName);
			//copy_file(ConFigFile,ConFigFile_CPY);
			//copy_file(ConFigFile,ConFigFile_Setting);
			//copy_file(ConFigFile,ConFigFile_Setting_config);
			//copy_file(ConFigFile,ConFigFile_Setting_config_cpy);
			//system(config_sh);
			ConfigFileOps();
		}
		DEBUG_PRINTF;
		chmod(FilePwd,0744);
	}
	
	FileTXRX->data[0] = 0x01;
	FileTXRX->length = 1;
	return 0;

	EXCEPTION:
		FileTXRX->data[0] = 0x00;
		FileTXRX->length = 1;
		return -1;
}

#if 1
static int FileTx_2Kperframe(FileTXRX_t *FileTXRX,unsigned int *len)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileNameLenStr[4];
	int FileNameLen = 0;
	char FileName[48];
	char FrameIDStr[4];
	uint32_t FrameID = 0;
	char *FrameContent = NULL;
	char FilePwd[64];
	uint32_t FrameLen = 0;
	
	// ����ļ�������
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,FileTXRX->data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//��ȡ�ļ���
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,FileTXRX->data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//���ļ�·���е�'\'ת����'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//��ȡ�ļ�ƫ�Ƶ�ַ
	Offset += FileNameLen;
	memset(FrameIDStr,0,sizeof(FrameIDStr));
	memcpy(FrameIDStr,FileTXRX->data + Offset,4);
	FrameID = ((uint8_t)FrameIDStr[0] - 0x30) * 1000 + ((uint8_t)FrameIDStr[1] - 0x30) * 100 + ((uint8_t)FrameIDStr[2] - 0x30) * 10 + ((uint8_t)FrameIDStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x,%d\n",FrameIDStr[0],FrameIDStr[1],FrameIDStr[2],FrameIDStr[3],FrameID);
	//�ļ�������һ֡����֡������λ��
	Offset += 4;
	FrameContent = FileTXRX->data + Offset + 1;

	//��ʼ�������û�
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,FileTXRX->user->type,FileTXRX->user->ip,FileTXRX->user->port,FileTXRX->user->uartPort);

	//��ȡҪ��ȡ���ļ�·��
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//��ʼ���ļ�
	ret = FRTx_FileFrameTx2K(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameID);
	if(ret < 0)
		goto EXCEPTION;

	//��֡�ش�
	Offset = 0;
	FileTXRX->data[Offset] = 0x01;
	Offset += 1;
	memcpy(FileTXRX->data + Offset,FileNameLenStr,3);
	Offset += 3;
	memcpy(FileTXRX->data + Offset,FileName,FileNameLen);
	Offset += FileNameLen;
	memcpy(FileTXRX->data + Offset,FrameIDStr,4);
	char *pp = FileTXRX->data + Offset;
	debug_printf("0x%x,0x%x,0x%x,0x%x  0x%x,0x%x,0x%x,0x%x\n",pp[0],pp[1],pp[2],pp[3],FrameIDStr[0],FrameIDStr[1],FrameIDStr[2],FrameIDStr[3]);
	Offset += 4;
	FileTXRX->length = Offset + FrameLen;
	debug_printf("FrameLen = %d,%d\n",FrameLen,FileTXRX->length);

	return 0;


	EXCEPTION:
		FileTXRX->data[0] = 0x00;
		FileTXRX->length = 1;
		return -1;
}
#endif


int FileRx_16Kperframe(FileTXRX_t *FileTXRX,unsigned int *len)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileName[48];
	char FilePwd[64];
	char FileNameLenStr[4];
	int  FileNameLen;
	char *FrameOffsetStr = NULL;
	int  FrameOffset = 0;
	char *FrameContent = NULL;
	int bmpnumber = 0;
	int	 FrameLen = 0;

	//ȡ�ļ�������
	Offset += 2;
	memcpy(FileNameLenStr,FileTXRX->data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//ȡ�ļ���
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,FileTXRX->data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//�ļ�·��'\'ת����'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//�����λ���ϴ��������ļ�·����û����Ӧ�ļ��оʹ�����Ӧ���ļ���
	char *p = NULL;
	chdir(sys_dir);
	p = strchr(FileName,'/');
	if(p != NULL)
	{
		DEBUG_PRINTF;
		mkdirs(FileName);
		chdir(sys_dir);
	}
	

	//ƫ�Ƶ��ļ�ƫ��λ��
	Offset += FileNameLen;
	FrameOffsetStr = FileTXRX->data + Offset;
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];

	debug_printf("FrameOffset = %d\n",FrameOffset);
	//ƫ�Ƶ��ļ�����λ��
	Offset += 4;
	FrameContent = FileTXRX->data + Offset;

	//֡����
	FrameLen = FileTXRX->length - Offset;

	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEUser_t));
	memcpy(FILEuser.ip,FileTXRX->user->ip,strlen(FileTXRX->user->ip));
	FILEuser.port = 5168;
	FILEuser.userType = 0;
	FILEuser.comx = 0;

	//��ȡҪ��ȡ���ļ�·��image/user/100.bmp
	memset(FilePwd,0,sizeof(FilePwd));
	debug_printf("atoi(FileName + 6) = %s\n",FileName + 6);
	if(strstr(FileName,".bmp") != NULL && atoi(FileName + 6) >= 60)
		sprintf(FilePwd,"%s/user/%s",image_dir,FileName + 6);
	else
		sprintf(FilePwd,"%s/%s",sys_dir,FileName);
		
	
	debug_printf("*FilePwd = %s\n",FilePwd);

	ret = FRTx_FileFrameRx16K(&FILEuser,FilePwd,FrameContent,FrameLen,FrameOffset);
	if(ret == 1)
	{
		if(strncmp(FileName,"config/cls.conf",15) == 0)
		{
			debug_printf("FileName = %s\n",FileName);
			//copy_file(ConFigFile,ConFigFile_CPY);
			//copy_file(ConFigFile,ConFigFile_Setting);
			//copy_file(ConFigFile,ConFigFile_Setting_config);
			//copy_file(ConFigFile,ConFigFile_Setting_config_cpy);
			//system(config_sh);
			ConfigFileOps();
		}
		DEBUG_PRINTF;
		chmod(FilePwd,0744);
	}

	FileTXRX->data[0] = 0x01;
	FileTXRX->length = 1;
	return 0;
	
}


#if 1
int FileTx_16Kperframe(FileTXRX_t *FileTXRX,unsigned int *len)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileNameLenStr[4];
	int FileNameLen = 0;
	char FileName[48];
	char FrameOffsetStr[4];
	uint32_t FrameOffset = 0;
	char *FrameContent = NULL;
	char FilePwd[64];
	uint32_t FrameLen = 0;
	
	// ����ļ�������
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,FileTXRX->data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//��ȡ�ļ���
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,FileTXRX->data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//���ļ�·���е�'\'ת����'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//��ȡ�ļ�ƫ�Ƶ�ַ
	Offset += FileNameLen;
	memset(FrameOffsetStr,0,sizeof(FrameOffsetStr));
	memcpy(FrameOffsetStr,FileTXRX->data + Offset,4);
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3]);
	//�ļ�������һ֡����֡������λ��
	Offset += 4;
	FrameContent = FileTXRX->data + Offset + 1;

	//��ʼ�������û�
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,FileTXRX->user->type,FileTXRX->user->ip,FileTXRX->user->port,FileTXRX->user->uartPort);

	//��ȡҪ��ȡ���ļ�·��
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//��ʼ���ļ�
	ret = FRTx_FileFrameTx16K(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameOffset);
	if(ret < 0)
		goto EXCEPTION;

	DEBUG_PRINTF;
	//��֡�ش�
	Offset = 0;
	FileTXRX->data[Offset] = 0x01;
	Offset += 1;
	memcpy(FileTXRX->data + Offset,FileNameLenStr,3);
	Offset += 3;
	memcpy(FileTXRX->data + Offset,FileName,FileNameLen);
	Offset += FileNameLen;
	memcpy(FileTXRX->data + Offset,FrameOffsetStr,4);
	char *pp = FileTXRX->data + Offset;
	debug_printf("0x%x,0x%x,0x%x,0x%x  0x%x,0x%x,0x%x,0x%x\n",pp[0],pp[1],pp[2],pp[3],FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3]);
	Offset += 4;
	FileTXRX->length = Offset + FrameLen;
	debug_printf("FrameLen = %d,%d\n",FrameLen,FileTXRX->length);

	return 0;


	EXCEPTION:
		FileTXRX->data[0] = 0x00;
		FileTXRX->length = 1;
		return -1;
}

#endif


static int get_communitStatus(FileTXRX_t *FileTXRX,unsigned int *len)
{
	//ָ��ִ�����
	FileTXRX->data[0] = 0x01;
	*len = 1;
	FileTXRX->length = *len;	
	return 0;
}


static int ByteToStruct(FileTXRX_t *FileTXRX,unsigned char *Bytestr,unsigned int Bytelen)
{
	if(FileTXRX == NULL || Bytestr == NULL)
		return -1;

	FileTXRX->startByte	= Bytestr[0];
	FileTXRX->endByte	= Bytestr[Bytelen - 1];
	FileTXRX->frameType = (Bytestr[1] << 8) | Bytestr[2];
	FileTXRX->devAddr	= (Bytestr[3] << 8) | Bytestr[4];
	FileTXRX->parity 	= Bytestr[Bytelen - 3] << 8 | Bytestr[Bytelen - 2];
	FileTXRX->length	= Bytelen - 8;
	FileTXRX->data 		= Bytestr + 5;
	return 0;
}

static int StructToBytes(FileTXRX_t *FileTXRX,unsigned char *outputbytes)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	if(FileTXRX == NULL)
		return -1;
	
	outputbytes[0] = 0x02;
	outputbytes[1] = (unsigned char)(FileTXRX->frameType >> 8);
	outputbytes[2] = (unsigned char)(FileTXRX->frameType);
	outputbytes[3] = (unsigned char)(FileTXRX->devAddr >> 8);
	outputbytes[4] = (unsigned char)(FileTXRX->devAddr);

	for(i = 0 ; i < FileTXRX->length; i ++)
	{
		outputbytes[5 + i]	= FileTXRX->data[i];
	}
}


static int FResetTxCard(FileTXRX_t *FileTXRX,unsigned int *len)
{
	if(FileTXRX->data[0] == 0x31)
	{
		TxCardReset();
		usleep(10000);
		//ModulePower(MODULE_ON);
	}
	
	FileTXRX->data[0] = 0x01;
	FileTXRX->length = 1;
	*len = 1;
	return 0;
}

static int FResetRxCard(FileTXRX_t *FileTXRX,unsigned int *len)
{
	if(FileTXRX->data[0] == 0x31)
	{
		//RxCardReset();  ������ʱע��
	}

	FileTXRX->data[0] = 0x01;
	FileTXRX->length = 1;
	*len = 1;
	return 0;
}

int Fset_netport(FileTXRX_t *FileTXRX,unsigned int *len)
{
	unsigned char dev_ip[24] , dev_mask[24] , dev_gw[24] , dev_port[8];
	unsigned char *ip = NULL,*mask = NULL,*gw = NULL,*port = NULL;
	unsigned short _port = 0;

	char conf_file[64];
	memset(conf_file,0x00,sizeof(conf_file));
	sprintf(conf_file,"%s/cls.conf",conf_dir);
	
	ip 	 = FileTXRX->data + 6;
	mask = FileTXRX->data + 10;
	gw   = FileTXRX->data + 14;
	port = FileTXRX->data + 24;

	_port = port[0] << 8 | port[1] << 0;
	if(_port == 0)
		_port = 5168;
	
	memset(dev_ip,0,sizeof(dev_ip));
	memset(dev_mask,0,sizeof(dev_mask));
	memset(dev_gw,0,sizeof(dev_gw));
	memset(dev_port,0,sizeof(dev_port));

	debug_printf("ip[0] = %d\n",ip[0]);
	sprintf(dev_ip,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
	sprintf(dev_mask,"%d.%d.%d.%d",mask[0],mask[1],mask[2],mask[3]);
	sprintf(dev_gw,"%d.%d.%d.%d",gw[0],gw[1],gw[2],gw[3]);
	sprintf(dev_port,"%d",_port);

	debug_printf("dev_ip = %s,dev_mask = %s,dev_gw = %s\n",dev_ip,dev_mask,dev_gw);
	
	if(access(conf_file,F_OK) < 0)
		goto EXEPTION;
	
	conf_file_write(conf_file,"netport","ip",dev_ip);
	conf_file_write(conf_file,"netport","netmask",dev_mask);
	conf_file_write(conf_file,"netport","gateway",dev_gw);
	conf_file_write(conf_file,"netport","port",dev_port);

	ConfigFileOps();
	//system(config_sh);
	
	//set_devip(dev_ip,dev_mask,dev_gw);

	

	FileTXRX->data[2] = 0x31;
	FileTXRX->length = 3;
	*len = 3;

	char ip_port[24];
	char logmsg[96];
	memset(ip_port,0,sizeof(ip_port));
	sprintf(ip_port,"%s:%d",FileTXRX->user->ip,FileTXRX->user->port);
	sprintf(logmsg,"cmd:%x setnet:ip: %s netmask:%s gateway:%s port:%s",
		FileTXRX->frameType,dev_ip,dev_mask,dev_gw,dev_port);
	debug_printf("strlen(logmsg) = %d\n",strlen(logmsg));
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	return 0;

	EXEPTION:
		FileTXRX->data[2] = 0x31;
		FileTXRX->length = 3;
		*len = 3;
		return -1;
}



static int frame_process(user_t *user,uint8_t *recvbuf,uint32_t *Len)
{
	//Protocl_t protocol;
	FileTXRX_t FileTXRX;
	unsigned short parity = 0;
	unsigned char ptcdata[128];
	unsigned int len;
	unsigned int nlength;
	int err = -1;

	unsigned int outlen = 0;

	int output_total_len = 0;

	//��ת��
	err = prtcl_preparsing(recvbuf,*Len,FREEdata,&outlen);
	if(err != 0)
		goto ERRORDEAL;
	DEBUG_PRINTF;
	//У��
	err = ParityCheck_CRC16(FREEdata,outlen);
	if(err != 0)
		goto ERRORDEAL;
	DEBUG_PRINTF;
	//�������input���ֽ���ת����Э��ṹ��protocol
	ByteToStruct(&FileTXRX,FREEdata,outlen);
	FileTXRX.user = user;
	
	//����IP�����ʱ��
	uint16_t setipcmd = 0;
	setipcmd = FileTXRX.devAddr;
	if(setipcmd == 0x3338)
		FileTXRX.frameType	= setipcmd;

	switch(FileTXRX.frameType)
	{
		case 0x3030:
			get_communitStatus(&FileTXRX,&len);
			break;
			
		/*�ļ�����*/
		//����λ�������ļ�
		case 0x3230:
			FileRx_2Kperframe(&FileTXRX,&len);//ok
			break;
			
		//���ļ��ϴ�����λ��
		case 0x3231:
			FileTx_2Kperframe(&FileTXRX,&len);//ok
			break;
			
		//��Դ��ļ�������λ�������ļ�
		case 0x3330:
			FileRx_16Kperframe(&FileTXRX,&len);
			break;
			
		//��Դ��ļ����ϴ��ļ�����λ��
		case 0x3331:
			FileTx_16Kperframe(&FileTXRX,&len);
			break;
			
		//�޸�IP
		case 0x3338:
			Fset_netport(&FileTXRX,&len);
			break;

		//��λ���Ϳ�
		case 0x3533:
			FResetTxCard(&FileTXRX,&len);
			break;

		//��λ���տ�
		case 0x3534:
			FResetRxCard(&FileTXRX,&len);
			break;
			
		default:
			break;
			
	}


	StructToBytes(&FileTXRX,FREEdata);
	//У��ֵ
	parity = XKCalculateCRC(FREEdata+1,4+FileTXRX.length);
	FileTXRX.parity = parity;
	debug_printf("FileTXRX->parity = 0x%x\n",FileTXRX.parity);
	FREEdata[5 + FileTXRX.length + 0] = (unsigned char)(FileTXRX.parity >> 8);
	FREEdata[5 + FileTXRX.length + 1] = (unsigned char)(FileTXRX.parity);
	//����ֽ������Ƿ����0x02��0x03��0x1B�����������Ӧ��ת�����˴�����취�ǽ�ͷβ�����ֽ��ߵ��ڴ���
	DEBUG_PRINTF;
	check_0x02and0x03(FLAG_SEND,FREEdata+1,4+FileTXRX.length+2,recvbuf+1,Len);
	DEBUG_PRINTF;
	
	output_total_len = *Len;
	//����ֽ����ټ���ͷβ�����ֽ�
	recvbuf[0] 				= 0x02;
	recvbuf[output_total_len + 1] 	= 0x03;
	//�����ܳ���Ҫ+2
	output_total_len += 2;
	*Len = output_total_len;
	debug_printf("1*****************protocol msg: send *****************\n");
	
	return 0;
	
	ERRORDEAL:
		DEBUG_PRINTF;
		debug_printf("memory has been free!\n");
		return -1;
	return 0;
}


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


static int NetAckBack(int fd,uint8_t *tx_buf,int tx_len)
{
	int i = 0;
	int ret = -1;
	int remaind_size = tx_len;
	int offset = 0;
	int send_len = 0;
	bool sendable = false;
	debug_printf("for sendding data=====##########************swr\n");

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


int file_recv_process(void *arg)
{
	int ret = -1;
	int timer_id = 0;
	uint32_t FrameLen = 0;
	uint32_t RXlen = 0;
	client_t *FindUser;
	client_t *user = (client_t *)arg;
	//free(arg);
	uint8_t RXbuf[1024 * 24];
	memset(RXbuf,0,sizeof(RXbuf));

	DEBUG_PRINTF;
	ret = socket_recv(NewClient.new_fd,RXbuf,&RXlen);

	int i = 0;
	debug_printf("This data from recv_task_process--------RXlen = %d------\n",RXlen);
	for(i = 0 ; i < RXlen ; i++)
		debug_printf("0x%x ",RXbuf[i]);
	debug_printf("\n\n");

	//Dtos("/home/LEDscr/log/recv.log",NetData,RXlen);

	//�ͻ��˶Ͽ�����ֱ�ӿͻ������е���Ӧ�Ŀͻ���Ϣɾ��������epoll����Ӧ�Ŀͻ���Ϣɾ��
	if(ret == ERR_SOCKET_SHUTDOWN)
	{
		close(NewClient.new_fd);
		TCP_EpollCtl(NewClient.new_fd,&NewClient.ev,EPOLL_CTL_DEL);
		NewClient.busy = 0;
		OnlyClientFlag = 0;
		return 0;
	}

	if(ret != ERR_OK || RXlen == 0)
		goto FREERESOURCE;
	
	user_t Cuser;
	memset(&Cuser,0,sizeof(user_t));
	Cuser.port = NewClient.port;
	Cuser.fd = NewClient.new_fd;
	memcpy(Cuser.ip,NewClient.ip,strlen(NewClient.ip));

	
	//�п�����ճ��������
	if(RXbuf[0] == 0x02 && RXbuf[RXlen - 1] == 0x03)
	{
		FrameLen = SplitPackage(RXbuf,RXlen);
	}

	if(FrameLen == RXlen)
	{
		DEBUG_PRINTF;
		frame_process(&Cuser,RXbuf,&RXlen);
		NetAckBack(NewClient.new_fd,RXbuf,RXlen);
	}
	else if(FrameLen < RXlen)
	{
		uint32_t frameLen = FrameLen;
		uint32_t DataLen = RXlen - FrameLen;
		frame_process(&Cuser,RXbuf,&FrameLen);
		NetAckBack(NewClient.new_fd,RXbuf,RXlen);
		frame_process(&Cuser,RXbuf + frameLen,&DataLen);
		NetAckBack(NewClient.new_fd,RXbuf,RXlen);
	}
	DEBUG_PRINTF;
	NewClient.busy = 0;
	
	return 0;

	FREERESOURCE:
		return -1;
}



void *File_recv_task(void *arg)
{
	int i;	
	int nfds = 0;
	int ServerFD;
	uint32_t RXlen = 0;
	struct epoll_event ev,events[19];
	int fd = 0;
	char RXbuf[4096];
	int timeout = 10 * 60 * 1000;
	int sleeptime = 5 * 1000 * 1000;
	NetInit();
	while(1)
	{
		
		nfds = EpollWait(events,1,timeout);  
		if (nfds == -1)  
		{  
			continue;  
		} 

		for (i = 0; i < nfds; i++)	
		{  
			if (events[i].data.fd < 0)	
				continue;  
			
			if (events[i].data.fd == serverFd) //��⵽һ��SOCKET�û����ӵ��˰󶨵�SOCKET�˿ڣ������µ����ӡ�  
			{  
				if(OnlyClientFlag)
					continue;
				
				OnlyClientFlag = 1;
			   ClientAccept(&NewClient);
				TCP_SetNonBlocking(NewClient.new_fd); 
				NewClient.ev.data.fd = NewClient.new_fd; 
				NewClient.ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; //����Ҫ������¼�����  
				epoll_ctl(epollFd, EPOLL_CTL_ADD, NewClient.new_fd, &NewClient.ev);
				continue;
			}  
			
			if (events[i].events & EPOLLIN) //socket�յ�����  
			{  
				DEBUG_PRINTF;
				debug_printf("NewClient.busy = %d\n",NewClient.busy);
				if(NewClient.busy)
					continue;
				DEBUG_PRINTF;
				NewClient.busy = 1;
				threadpool_add(pthreadpool,file_recv_process,(void*)&NewClient);	
			}  

		}
	
	}
	return NULL;
}

#endif

