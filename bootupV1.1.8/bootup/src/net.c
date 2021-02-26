#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/io.h>
#include <errno.h>

#include "debug.h" 
#include "net.h"

#define SERVER_NET_POT		5168
static struct sockaddr_in client_addr;  
static struct sockaddr_in server_addr;
static int client_socket;


static void net_InitClient(void)
{
	//设置一个socket地址结构client_addr,代表客户机internet地址, 端口  
	bzero(&client_addr,sizeof(client_addr)); //把一段内存区的内容全部设置为0  
	client_addr.sin_family = AF_INET;	 //internet协议族  
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);//INADDR_ANY表示自动获取本机地址  
	client_addr.sin_port = htons(0);	//0表示让系统自动分配一个空闲端口  
}

static void net_InitServer(void)
{
	int PORT = 0;
	FILE *FP = NULL;
	char port[8];

	FP = fopen("/home/LEDscr/boot/port","r");
	if(FP == NULL)
		exit(1);

	memset(port,0,sizeof(port));
	fread(port,1,4,FP);
	PORT = atoi(port);
	//printf("port = %d\n",PORT);
    //设置一个socket地址结构server_addr,代表服务器的internet地址, 端口    
    bzero(&server_addr,sizeof(server_addr));  
    server_addr.sin_family = AF_INET;  
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);//INADDR_ANY表示自动获取本机地址  
    server_addr.sin_port = htons(PORT);  
}

void net_Init(void)
{
	net_InitClient();
	net_InitServer();
}

int net_ConnectServer(void)
{

	//创建用于internet的流协议(TCP)socket,用client_socket代表客户机socket  
	client_socket = socket(AF_INET,SOCK_STREAM,0);	
	if( client_socket < 0)	
	{  
		printf("Create Socket Failed!\n");
		return -1;
	} 


	//把客户机的socket和客户机的socket地址结构联系起来	
	if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))  
	{  
		printf("Client Bind Port Failed!\n");	
		return -1;
	} 

	usleep(10000);
    if(connect(client_socket,(struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)  
    {  
    	perror("connect");
        //debug_printf("1Can Not Connect to server!\n"); 
		return -1;
    } 
	
	int flags = 0;
	flags = fcntl(client_socket, F_GETFL);						 //获取文件的flags值。
	fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);   //设置成非阻塞模式；
	
	return 0;
}


void net_CloseSocket(void)
{
	close(client_socket);
}

int net_Send(char *tx_buf,int len)
{
	return send(client_socket,tx_buf,10,MSG_DONTWAIT);
}

int net_Recv(char *rx_buf,int len)
{
	return recv(client_socket,rx_buf,8,MSG_DONTWAIT);
}
