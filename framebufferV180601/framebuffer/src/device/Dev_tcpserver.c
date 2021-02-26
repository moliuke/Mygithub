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

#include "Dev_tcpserver.h"
#include "debug.h"
#include "../clientlist.h"


pthread_mutex_t sersock_mutex;

//Socketobj_t serverStruct;
TCPIPtruct_t socketobj;
static TCPIPtruct_t SERVERStruct,UDPserver;

#define BACKLOG 8
#define BUF_SIZE 2176


struct sockaddr_in servaddr, cliaddr;    
struct rlimit rt;   


void set_net_port(const char *ip,const char *mask,const char *gateway,uint32_t port)
{
	memset(&socketobj,0,sizeof(socketobj));

	memcpy(socketobj.ip,ip,strlen(ip));
	if(mask != NULL)
		memcpy(socketobj.mask,mask,strlen(mask));
	if(gateway != NULL)
		memcpy(socketobj.gateway,gateway,strlen(gateway));

	socketobj.port = port;
}













void TCP_SetNetArgment(char *IP,uint32_t port)
{
	memcpy(SERVERStruct.IP,IP,strlen(IP));
	SERVERStruct.NetPort = port;
}



static int CheckIfClientExist(client_t *Client)
{
	int i = 0;
	for(i = 0 ; i < CLIENT_MAX_NUMBER ; i ++)
	{
		if(SERVERStruct.user[i] == NULL)
			continue;
		
		if(strcmp(Client->ip,SERVERStruct.user[i]->ip) == 0 ||
		   Client->new_fd == SERVERStruct.user[i]->new_fd || 
		   Client->port == SERVERStruct.user[i]->port)
		{
			pthread_mutex_lock(&user_mutex);	
			TCP_EpollCtl(SERVERStruct.user[i]->new_fd,&SERVERStruct.user[i]->ev,EPOLL_CTL_DEL);
			close(SERVERStruct.user[i]->new_fd);
			free(SERVERStruct.user[i]);
			SERVERStruct.user[i] = NULL;
			pthread_mutex_unlock(&user_mutex); 
			return 0;
		}
	}
	return -1;
}


  
int TCP_ClientAccept(client_t *Client)  
{  
    struct sockaddr_in client_addr;  
    socklen_t len = sizeof(client_addr);  
	
    memset(&client_addr, 0, sizeof(client_addr));  
    Client->new_fd = accept(SERVERStruct.ServerFd, (struct sockaddr *)&client_addr, &len);  

	int iplen = strlen(inet_ntoa(client_addr.sin_addr));
	Client->port = ntohs(client_addr.sin_port);
	memset(Client->ip,0,sizeof(Client->ip));
	memcpy(Client->ip,inet_ntoa(client_addr.sin_addr),iplen);
	Client->ip[strlen(Client->ip)] = '\0';
	debug_printf("ip = %s,port = %d,fd = %d\n",Client->ip,Client->port,Client->new_fd);

	CheckIfClientExist(Client);
    return Client->new_fd;  
}

int TCP_EpollWait(struct epoll_event *events,int maxEV,int timeout)
{
	return epoll_wait(SERVERStruct.EpollFd, events, maxEV, timeout);
}

int TCP_EpollCtl(int fd,struct epoll_event *ev,int ops)
{
	return epoll_ctl(SERVERStruct.EpollFd, ops, fd, ev);  
}




client_t * TCPIP_FindUser(int fd)
{
	int i = 0;
	for(i = 0 ; i < CLIENT_MAX_NUMBER ; i ++)
	{
		if(SERVERStruct.user[i] == NULL)
			continue;
		
		if(fd == SERVERStruct.user[i]->new_fd)
			break;
	}

	if(i == CLIENT_MAX_NUMBER)
		return NULL;

	return SERVERStruct.user[i];
}


int TCP_UserADD(client_t *user)
{
	uint8_t i = 0; 
	if(user == NULL)
		return -1;
	
	for(i = 0 ; i < CLIENT_MAX_NUMBER ; i ++)
	{
		if(SERVERStruct.user[i] == NULL)
			break;
	}

	if(i == CLIENT_MAX_NUMBER)
		return -1;

	SERVERStruct.user[i] = user;
	return 0;
}

int TCP_UserPrintf(void)
{
	uint8_t i = 0 ;
	for(i = 0 ; i < CLIENT_MAX_NUMBER ; i ++)
	{
		if(SERVERStruct.user[i] != NULL)
			debug_printf("SERVERStruct.user[i]->ip = %s,SERVERStruct.user[i]->port = %d\n\n\n\n",SERVERStruct.user[i]->ip,SERVERStruct.user[i]->port);
	}
}

int TCP_AllUserDel(void)
{
	uint8_t i = 0 ;
	//printf("delet user\n");
	for(i = 0 ; i < CLIENT_MAX_NUMBER ; i ++)
	{
		if(SERVERStruct.user[i] == NULL)
			continue;

		TCP_EpollCtl(SERVERStruct.user[i]->new_fd,&SERVERStruct.user[i]->ev,EPOLL_CTL_DEL);
		close(SERVERStruct.user[i]->new_fd);
		free(SERVERStruct.user[i]);
		SERVERStruct.user[i] = NULL;
	}
}

int TCP_UserDEL(client_t *user)
{
	uint8_t i = 0 ; 
	if(user == NULL)
		return -1;

	for(i = 0 ; i < CLIENT_MAX_NUMBER ; i ++)
	{
		if(SERVERStruct.user[i] == NULL)
			continue;
		
		if(SERVERStruct.user[i] == user || strcmp(SERVERStruct.user[i]->ip,user->ip) == 0 || 
			SERVERStruct.user[i]->new_fd == user->new_fd || SERVERStruct.user[i]->port == user->port)
		{
			DEBUG_PRINTF;
			break;
		}
	}

	if(i == CLIENT_MAX_NUMBER)
		return -1;

	close(SERVERStruct.user[i]->new_fd);

	free(SERVERStruct.user[i]);
	DEBUG_PRINTF;
	SERVERStruct.user[i] = NULL;
	return 0;
	
}
  
void TCP_SetNonBlocking(int fd) //将socket设置为非阻塞  
{  
    int opts = fcntl(fd, F_GETFL);  
    if (opts < 0)  
    {  
        debug_printf("fcntl failed %s\n", strerror(errno));  
    }  
    opts = opts | O_NONBLOCK;  
    if (fcntl(fd, F_SETFL, opts) < 0)  
    {  
        debug_printf("fcntl failed %s\n", strerror(errno));  
    }  
}  

#if 0
static int TCPIP_SocketCreate(void)  
{  
    int on = 1;  
    struct sockaddr_in addr;
	
    SERVERStruct.ServerFd = socket(AF_INET, SOCK_STREAM, 0);  
    if (setsockopt(SERVERStruct.ServerFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)  
    {  
        debug_printf("setsockopt failed %s\n", strerror(errno));  
        return -1;  
    }  

	//把socket设置为非阻塞方式	
    TCPIP_SetNonBlocking(SERVERStruct.ServerFd);
	
    memset(&addr, 0, sizeof(addr));  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(SERVERStruct.NetPort);  
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	
    if (bind(SERVERStruct.ServerFd, (struct sockaddr *) &addr, sizeof(addr)) == -1)  
    {  
        debug_printf("bind port %d failed %s\n", SERVERStruct.NetPort, strerror(errno));  
        return -1;  
    }  
	
    if (listen(SERVERStruct.ServerFd, CLIENT_MAX_NUMBER) == -1)  
    {  
        debug_printf("listen failed %s\n", strerror(errno));  
        return -1;  
    }  
	
    return SERVERStruct.ServerFd;  
} 

#endif

//#define TCPIP_TCP
static int TCP_SocketCreate(void)  
{  
    int on = 1;  
    struct sockaddr_in addr;

    SERVERStruct.ServerFd = socket(AF_INET, SOCK_STREAM, 0);
//端口复用
    if (setsockopt(SERVERStruct.ServerFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)  
    {  
        //printf("setsockopt failed %s\n", strerror(errno));  
        return -1;  
    }  

	//把socket设置为非阻塞方式	
    TCP_SetNonBlocking(SERVERStruct.ServerFd);
	
    memset(&addr, 0, sizeof(addr));  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(SERVERStruct.NetPort);  
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	
    if (bind(SERVERStruct.ServerFd, (struct sockaddr *) &addr, sizeof(addr)) == -1)  
    {  
        //printf("bind port %d failed %s\n", SERVERStruct.NetPort, strerror(errno));  
        return -1;  
    }  
    if (listen(SERVERStruct.ServerFd, CLIENT_MAX_NUMBER) == -1)  
    {  
        //printf("listen failed %s\n", strerror(errno));  
        return -1;  
    }  
	
    return SERVERStruct.ServerFd;  
}  





//使用epoll的方法监视文件描述符
static void TCP_EpollInit(void)
{
	//声明epoll_event结构体的变量,ev用于注册事件 
    struct epoll_event ev;
	
	//生成用于处理accept的epoll专用的文件描述符  
    SERVERStruct.EpollFd = epoll_create(CLIENT_MAX_NUMBER -1); 
	
	//设置与要处理的事件相关的文件描述符  
    ev.data.fd = SERVERStruct.ServerFd; 	
	
	//设置要处理的事件类型 
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP; 	
	
	//注册epoll事件  
    epoll_ctl(SERVERStruct.EpollFd, EPOLL_CTL_ADD, SERVERStruct.ServerFd, &ev); 
}

static void TCP_UserInit(void)
{
	uint8_t i = 0; 
	for(i = 0 ; i < CLIENT_MAX_NUMBER ; i ++)
		SERVERStruct.user[i] = NULL;
}


int TCP_ServerInit(void)
{
	int serfd = TCP_SocketCreate();
	if(serfd < 0)
		return -1;
	
	
	TCP_EpollInit();
	TCP_UserInit();

	return serfd;
}



static int UDP_SocketCreate(void)  
{  
    int on = 1;  
    struct sockaddr_in addr;

    UDPserver.ServerFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (setsockopt(UDPserver.ServerFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1)  
    {  
        debug_printf("setsockopt failed %s\n", strerror(errno));  
        return -1;  
    }  

	//把socket设置为非阻塞方式	
    TCP_SetNonBlocking(UDPserver.ServerFd);
	
    memset(&addr, 0, sizeof(addr));  
    addr.sin_family = AF_INET;  
    addr.sin_port = htons(SERVERStruct.NetPort);  
    addr.sin_addr.s_addr = htonl(INADDR_ANY); 
	
    if (bind(UDPserver.ServerFd, (struct sockaddr *) &addr, sizeof(addr)) == -1)  
    {  
        debug_printf("bind port %d failed %s\n", UDPserver.NetPort, strerror(errno));  
        return -1;  
    }  	
    return UDPserver.ServerFd;  
}  


int UDP_EpollWait(struct epoll_event *events,int maxEV,int timeout)
{
	return epoll_wait(UDPserver.EpollFd, events, maxEV, timeout);
}
int UDP_EpollCtl(int fd,struct epoll_event *ev,int ops)
{
	return epoll_ctl(UDPserver.EpollFd, ops, fd, ev);  
}

static void UDP_EpollInit(void)
{
	//声明epoll_event结构体的变量,ev用于注册事件 
    struct epoll_event ev;
	
	//生成用于处理accept的epoll专用的文件描述符  
    UDPserver.EpollFd = epoll_create(CLIENT_MAX_NUMBER -1); 
	
	//设置与要处理的事件相关的文件描述符  
    ev.data.fd = UDPserver.ServerFd; 	
	
	//设置要处理的事件类型 
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; 	
	
	//注册epoll事件  
    epoll_ctl(UDPserver.EpollFd, EPOLL_CTL_ADD, UDPserver.ServerFd, &ev); 
}

int UDP_ServerInit(void)
{
	int serfd = UDP_SocketCreate();
	if(serfd < 0)
		return -1;
	UDP_EpollInit();
}




