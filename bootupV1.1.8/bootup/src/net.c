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
	//����һ��socket��ַ�ṹclient_addr,����ͻ���internet��ַ, �˿�  
	bzero(&client_addr,sizeof(client_addr)); //��һ���ڴ���������ȫ������Ϊ0  
	client_addr.sin_family = AF_INET;	 //internetЭ����  
	client_addr.sin_addr.s_addr = htons(INADDR_ANY);//INADDR_ANY��ʾ�Զ���ȡ������ַ  
	client_addr.sin_port = htons(0);	//0��ʾ��ϵͳ�Զ�����һ�����ж˿�  
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
    //����һ��socket��ַ�ṹserver_addr,�����������internet��ַ, �˿�    
    bzero(&server_addr,sizeof(server_addr));  
    server_addr.sin_family = AF_INET;  
	server_addr.sin_addr.s_addr = htons(INADDR_ANY);//INADDR_ANY��ʾ�Զ���ȡ������ַ  
    server_addr.sin_port = htons(PORT);  
}

void net_Init(void)
{
	net_InitClient();
	net_InitServer();
}

int net_ConnectServer(void)
{

	//��������internet����Э��(TCP)socket,��client_socket����ͻ���socket  
	client_socket = socket(AF_INET,SOCK_STREAM,0);	
	if( client_socket < 0)	
	{  
		printf("Create Socket Failed!\n");
		return -1;
	} 


	//�ѿͻ�����socket�Ϳͻ�����socket��ַ�ṹ��ϵ����	
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
	flags = fcntl(client_socket, F_GETFL);						 //��ȡ�ļ���flagsֵ��
	fcntl(client_socket, F_SETFL, flags | O_NONBLOCK);   //���óɷ�����ģʽ��
	
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
