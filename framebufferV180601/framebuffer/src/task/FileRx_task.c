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

	//把socket设置为非阻塞方式	
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
	//声明epoll_event结构体的变量,ev用于注册事件 
    struct epoll_event ev;
	
	//生成用于处理accept的epoll专用的文件描述符  
    epollFd = epoll_create(CLIENT_NUM); 
	
	//设置与要处理的事件相关的文件描述符  
    ev.data.fd = serverFd; 	
	
	//设置要处理的事件类型 
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; 	
	
	//注册epoll事件  
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
			//非第一帧数据，如果不是第一帧数据，检查一下最后一个数据是否是0x03
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


			//第一帧数据，有可能第一帧数据就是一包完整的数据，也有可能不完整
			//查找一帧数据的开头字节0x02，往后查找recvlen个字节，找不到就认为数据是错误的
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


			//程序走到这里说明0x02字节肯定找到，但是在第checkCount个字节才找到
			//所以讲0x02及后面的数据全部往前挪
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

			
			//在第一帧数据接收完后要检查一下最后一个字节是否是0x03,是则说明第一帧数据
			//是一包完整的数据，直接返回
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
	//取文件名长度
	CurOffset += 2;
	memset(FileNameLenStr,0,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,FileTXRX->data + CurOffset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//取文件名
	CurOffset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,FileTXRX->data + CurOffset,FileNameLen);
	FileName[FileNameLen] = '\0';

	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//检查上位机上传过来的文件路径，没有响应文件夹就创建相应的文件夹
	char *p = NULL;
	chdir(sys_dir);
	p = strchr(FileName,'/');
	if(p != NULL)
	{
		DEBUG_PRINTF;
		mkdirs(FileName);
		chdir(sys_dir);
	}

	//帧偏移量
	CurOffset += FileNameLen;
	FrameIdStr = FileTXRX->data + CurOffset;
	FrameId = (FrameIdStr[0] - 0x30) * 1000 + (FrameIdStr[1] - 0x30) * 100 + 
			  (FrameIdStr[2] - 0x30) * 10 + (FrameIdStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",FrameIdStr[0],FrameIdStr[1],FrameIdStr[2],FrameIdStr[3]);
	debug_printf("FrameId = %d\n",FrameId);

	//帧内容长度
	CurOffset += 4;
	FrameLen = FileTXRX->length - CurOffset;

	//帧内容
	FrameContent = FileTXRX->data + CurOffset;

	//创建文件路径
	char FilePwd[64];
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	
	debug_printf("FilePwd = %s\n",FilePwd);

	debug_printf("protocol->usermsg->ip = %s\n",FileTXRX->user->ip);
	//初始化用户所属
	FILEUser_t FileUser;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,FileTXRX->user->type,FileTXRX->user->ip,FileTXRX->user->port,FileTXRX->user->uartPort);
	DEBUG_PRINTF;
	//帧数据存文件
	retvals = FRTx_FileFrameRx2K(&FileUser,FilePwd,FrameContent,FrameLen,FrameId);
	if(retvals < 0)
		goto EXCEPTION;

	//接收结束
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
	
	// 获得文件名长度
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,FileTXRX->data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//获取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,FileTXRX->data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//将文件路径中的'\'转化成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//获取文件偏移地址
	Offset += FileNameLen;
	memset(FrameIDStr,0,sizeof(FrameIDStr));
	memcpy(FrameIDStr,FileTXRX->data + Offset,4);
	FrameID = ((uint8_t)FrameIDStr[0] - 0x30) * 1000 + ((uint8_t)FrameIDStr[1] - 0x30) * 100 + ((uint8_t)FrameIDStr[2] - 0x30) * 10 + ((uint8_t)FrameIDStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x,%d\n",FrameIDStr[0],FrameIDStr[1],FrameIDStr[2],FrameIDStr[3],FrameID);
	//文件内容在一帧返回帧数据中位置
	Offset += 4;
	FrameContent = FileTXRX->data + Offset + 1;

	//初始化所属用户
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,FileTXRX->user->type,FileTXRX->user->ip,FileTXRX->user->port,FileTXRX->user->uartPort);

	//获取要读取的文件路径
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//开始读文件
	ret = FRTx_FileFrameTx2K(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameID);
	if(ret < 0)
		goto EXCEPTION;

	//组帧回传
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

	//取文件名长度
	Offset += 2;
	memcpy(FileNameLenStr,FileTXRX->data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,FileTXRX->data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//文件路径'\'转换成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//检查上位机上传过来的文件路径，没有响应文件夹就创建相应的文件夹
	char *p = NULL;
	chdir(sys_dir);
	p = strchr(FileName,'/');
	if(p != NULL)
	{
		DEBUG_PRINTF;
		mkdirs(FileName);
		chdir(sys_dir);
	}
	

	//偏移到文件偏移位置
	Offset += FileNameLen;
	FrameOffsetStr = FileTXRX->data + Offset;
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];

	debug_printf("FrameOffset = %d\n",FrameOffset);
	//偏移到文件内容位置
	Offset += 4;
	FrameContent = FileTXRX->data + Offset;

	//帧长度
	FrameLen = FileTXRX->length - Offset;

	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEUser_t));
	memcpy(FILEuser.ip,FileTXRX->user->ip,strlen(FileTXRX->user->ip));
	FILEuser.port = 5168;
	FILEuser.userType = 0;
	FILEuser.comx = 0;

	//获取要读取的文件路径image/user/100.bmp
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
	
	// 获得文件名长度
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,FileTXRX->data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//获取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,FileTXRX->data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//将文件路径中的'\'转化成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//获取文件偏移地址
	Offset += FileNameLen;
	memset(FrameOffsetStr,0,sizeof(FrameOffsetStr));
	memcpy(FrameOffsetStr,FileTXRX->data + Offset,4);
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3]);
	//文件内容在一帧返回帧数据中位置
	Offset += 4;
	FrameContent = FileTXRX->data + Offset + 1;

	//初始化所属用户
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,FileTXRX->user->type,FileTXRX->user->ip,FileTXRX->user->port,FileTXRX->user->uartPort);

	//获取要读取的文件路径
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//开始读文件
	ret = FRTx_FileFrameTx16K(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameOffset);
	if(ret < 0)
		goto EXCEPTION;

	DEBUG_PRINTF;
	//组帧回传
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
	//指令执行情况
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
		//RxCardReset();  这里暂时注释
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

	//反转义
	err = prtcl_preparsing(recvbuf,*Len,FREEdata,&outlen);
	if(err != 0)
		goto ERRORDEAL;
	DEBUG_PRINTF;
	//校验
	err = ParityCheck_CRC16(FREEdata,outlen);
	if(err != 0)
		goto ERRORDEAL;
	DEBUG_PRINTF;
	//将输入的input的字节序转换成协议结构体protocol
	ByteToStruct(&FileTXRX,FREEdata,outlen);
	FileTXRX.user = user;
	
	//设置IP命令。临时的
	uint16_t setipcmd = 0;
	setipcmd = FileTXRX.devAddr;
	if(setipcmd == 0x3338)
		FileTXRX.frameType	= setipcmd;

	switch(FileTXRX.frameType)
	{
		case 0x3030:
			get_communitStatus(&FileTXRX,&len);
			break;
			
		/*文件传输*/
		//从上位机下载文件
		case 0x3230:
			FileRx_2Kperframe(&FileTXRX,&len);//ok
			break;
			
		//将文件上传给上位机
		case 0x3231:
			FileTx_2Kperframe(&FileTXRX,&len);//ok
			break;
			
		//针对大文件，从上位机下载文件
		case 0x3330:
			FileRx_16Kperframe(&FileTXRX,&len);
			break;
			
		//针对大文件，上传文件到上位机
		case 0x3331:
			FileTx_16Kperframe(&FileTXRX,&len);
			break;
			
		//修改IP
		case 0x3338:
			Fset_netport(&FileTXRX,&len);
			break;

		//复位发送卡
		case 0x3533:
			FResetTxCard(&FileTXRX,&len);
			break;

		//复位接收卡
		case 0x3534:
			FResetRxCard(&FileTXRX,&len);
			break;
			
		default:
			break;
			
	}


	StructToBytes(&FileTXRX,FREEdata);
	//校验值
	parity = XKCalculateCRC(FREEdata+1,4+FileTXRX.length);
	FileTXRX.parity = parity;
	debug_printf("FileTXRX->parity = 0x%x\n",FileTXRX.parity);
	FREEdata[5 + FileTXRX.length + 0] = (unsigned char)(FileTXRX.parity >> 8);
	FREEdata[5 + FileTXRX.length + 1] = (unsigned char)(FileTXRX.parity);
	//检测字节序中是否存在0x02、0x03、0x1B，有则进行相应的转换，此处处理办法是将头尾两个字节踢掉在处理
	DEBUG_PRINTF;
	check_0x02and0x03(FLAG_SEND,FREEdata+1,4+FileTXRX.length+2,recvbuf+1,Len);
	DEBUG_PRINTF;
	
	output_total_len = *Len;
	//输出字节序再加上头尾两个字节
	recvbuf[0] 				= 0x02;
	recvbuf[output_total_len + 1] 	= 0x03;
	//所以总长度要+2
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

	//客户端断开连接直接客户管理中的相应的客户信息删除，并将epoll中相应的客户信息删除
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

	
	//有可能有粘包的现象
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
			
			if (events[i].data.fd == serverFd) //监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。  
			{  
				if(OnlyClientFlag)
					continue;
				
				OnlyClientFlag = 1;
			   ClientAccept(&NewClient);
				TCP_SetNonBlocking(NewClient.new_fd); 
				NewClient.ev.data.fd = NewClient.new_fd; 
				NewClient.ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; //设置要处理的事件类型  
				epoll_ctl(epollFd, EPOLL_CTL_ADD, NewClient.new_fd, &NewClient.ev);
				continue;
			}  
			
			if (events[i].events & EPOLLIN) //socket收到数据  
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

