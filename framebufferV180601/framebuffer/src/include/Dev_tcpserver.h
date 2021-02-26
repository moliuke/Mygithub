#ifndef __TCPSERVER_H
#define __TCPSERVER_H
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h> /* epoll function */
#include <sys/resource.h> /*setrlimit */
#include <stdbool.h>

#include "mylist.h"
#include "myerror.h"

#define MAXBUF 512

#define servPort 5168
#define LISTENMAX 1024
#define MAXEPOLLSIZE 65536
#define CLIENT_MAX_NUMBER	20



typedef struct epoll_list
{
	int  				kdpfd;    	//句柄操作描述符
	int  				nfds;      	//epoll活动唤醒的描述符个数
	int 				curfds;  	//当前epoll fd集合数   
	struct epoll_event 	ev;    
	struct epoll_event 	events[MAXEPOLLSIZE];  
}epolllist;



typedef struct __client
{
	int 	new_fd;
	int 	port;
	char 	ip[20];

	uint8_t busy;
	struct epoll_event ev;
}client_t;

typedef struct 
{
	int	 			sockfd;
	int 			new_fd;
	int 			listen_num;
	unsigned int 	port;
	char			ip[24];

	
	char			mask[24];
	char			gateway[24];

	fd_set 			fdsr;			//监听客户端的集合
	int 			maxsock;		//监听最大套接字
	struct timeval 	tv;				//监听超时设置
	socklen_t 		sin_size;		//
	int 			conn_amount; 
		
	int 			fd_A[CLIENT_MAX_NUMBER];	// accepted connection fd


	
	int 				ServerFd;
	int 				EpollFd;
	int 				ClientFd;
	int 				ListenNum;
	int 				NetPort;

	char 				IP[20];
	client_t			*user[20];
	
}TCPIPtruct_t;



extern TCPIPtruct_t socketobj;

enum{type_net = 0,type_uart};

typedef struct _user
{
	uint8_t 			type;		//网络或者串口
			
	char 				ip[20];		//网络IP地址
	uint16_t 			port;		//网络端口号

	uint8_t 			uartPort;

	int 				fd;			//描述符

	struct list_head 	list;

	pthread_mutex_t 	mutex;
}userlist_t;

void tcp_client_setsocket(int clientFd,int modeflag);
void tcp_close(void);
err_t tcp_client_accept(int serfd,client_t *client);
int net_tcp_init(void);
void set_net_port(const char *ip,const char *mask,const char *gateway,uint32_t port);
err_t tcp_select(int *aim_Set,int aim_fdn, int *rw_fdset,int *rw_fdn,struct timeval *timeout);

int create_sockfd();
int setnonblocking(int sockfd);
void move_fd(struct epoll_event *ev,int movefdfd,int *ctl_fd,int *cur_fd);
void remove_fd(struct epoll_event *ev,int movefd,int *ctl_fd,int *cur_fd);
int add_fd_epoll(struct epoll_event *ev,int addfd,int *ctl_fd,int *cur_fd);

void TCP_SetNetArgment(char *IP,uint32_t port);
int TCP_UserDEL(client_t *user);
int TCP_AllUserDel(void);

int TCP_ServerInit(void);

int TCP_UserADD(client_t *user);

int TCP_UserPrintf(void);
int TCP_EpollCtl(int fd,struct epoll_event *ev,int ops);
client_t * TCPIP_FindUser(int fd);
int TCPIP_ServerInit(void);
int TCP_EpollWait(struct epoll_event *events,int maxEV,int timeout);
void TCP_SetNonBlocking(int fd);//将socket设置为非阻塞  ;
int TCP_ClientAccept(client_t *Client);

int UDP_EpollWait(struct epoll_event *events,int maxEV,int timeout);
int UDP_EpollCtl(int fd,struct epoll_event *ev,int ops);
int UDP_ServerInit(void);


#endif
