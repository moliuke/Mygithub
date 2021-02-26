 #include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>   
//#include <sys/io.h>
#include <stddef.h>
#include <ctype.h>


#include "task.h"
#include "../protocol/PTC_init.h"
//#include "../protocol/seewor/SWR_protocol.h"
//#include "../protocol/seewor/SWR_display.h"
//#include "../protocol/perplelight/PPL_display.h"
//#include "../protocol/perplelight/PPL_datapool.h"
//
//#include "../protocol/perplelight/PPL_net.h"
#include "common.h"
#include "queue.h"
#include "Dev_tcpserver.h"
#include "debug.h"
#include "config.h"
#include "conf.h"
#include "wdt.h"
#include "../clientlist.h"
#include "../threadpool.h"
#include "../module/mtimer.h"
#include "../include/display.h"
#include "../module/image_gif.h"

//#include "../protocol/Modbus/modbus_task.h"
//#include "../protocol/Modbus/modbus_protocol.h"

//#include "../protocol/ZhiChao/ZC_task.h"
//#include "../protocol/ZhiChao/ZC_protocol.h"
//#include "../protocol/ZhiChao/ZC_display.h"

#include "../Hardware/HW3G_RXTX.h"
#include "../Hardware/HW2G_400.h"


#include "../cache.h"
#include "../Hardware/Data_pool.h"

#include "Net_task.h"

//#include "../protocol/ChengDu/CD_net.h"
//#include "../protocol/seewor/SWR_net.h"
//#include "../protocol/Modbus/modbus_net.h"

/**
		此线程用于监听上位机的网络端口，接收到上位机的消息后，将把消息加入
		队列中，在另一个线程(协议处理线程)里面处理
*/
void *pthread_monitor_task(void *arg)
{
	int nfds = 0;
	int ServerFD;
	uint32_t RXlen = 0;
	struct epoll_event ev,events[19];
	int fd = 0;
	char RXbuf[4096];
	if((ServerFD = TCP_ServerInit()) < 0)
	{
		log_write("TCP Server Init fail reboot",strlen("TCP Server Init fail reboot"));
		SET_LED_STATE(SLED_OFF);
		sleep(5);
		system("killall ledscreen");   //出现绑定失败时，重启 add 2020.7.2 mo
		DEBUG_PRINTF;
	}
	DEBUG_PRINTF;
	while (1)  
	{  
		//等待epoll事件的发生 
		nfds = TCP_EpollWait(events,19,-1);  
		if (nfds == -1)  
		{  
			debug_printf("epoll_wait failed %s\n", strerror(errno));	
			continue;  
		}  
		int i;
		for (i = 0; i < nfds; i++)	
		{  
			if (events[i].data.fd < 0)	
				continue;  
			if (events[i].data.fd == ServerFD) //监测到一个SOCKET用户连接到了绑定的SOCKET端口，建立新的连接。  
			{  
			   // st = socket_accept(listen_st);
			   client_t *FindUser = NULL;
			   int new_fd = -1;
			   client_t *NewClient = (client_t *)malloc(sizeof(client_t));
			   memset(NewClient,0,sizeof(client_t));
			   new_fd = TCP_ClientAccept(NewClient);
			   if(new_fd < 0)
					continue;
			 	TCP_SetNonBlocking(new_fd); 
				NewClient->busy = 0;
				NewClient->ev.data.fd = new_fd; 
				NewClient->ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; //设置要处理的事件类型  
				//epoll_ctl(epfd, EPOLL_CTL_ADD, st, &ev);	
				TCP_EpollCtl(new_fd,&NewClient->ev,EPOLL_CTL_ADD);
				pthread_mutex_lock(&user_mutex);	
				TCP_UserADD(NewClient);
				TCP_UserPrintf();
				pthread_mutex_unlock(&user_mutex);
				continue;
			}  
			
			//client端的socket有数据到达  
			if (events[i].events & EPOLLIN) //socket收到数据  
			{  
				client_t *FindUser,*user;
				pthread_mutex_lock(&user_mutex);	//	
				if((FindUser = (client_t *)TCPIP_FindUser(events[i].data.fd)) == NULL || FindUser->busy == 1)
				{
					pthread_mutex_unlock(&user_mutex);	
					continue;
				}
				FindUser->busy = 1;
				user = (client_t *)malloc(sizeof(client_t));
				memset(user,0,sizeof(client_t));
				memcpy(user,FindUser,sizeof(client_t));
				pthread_mutex_unlock(&user_mutex);	
				
				nettask_debug_printf("===user->ip = %s\n",user->ip);
				threadpool_add(pthreadpool,recv_task_process,(void*)user);	
				user = NULL;
			}  


			if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP || events[i].events & EPOLLRDHUP) //socket错误  
			{  
				client_t *client = NULL;
				pthread_mutex_lock(&user_mutex);
				client = TCPIP_FindUser(events[i].data.fd);
				if(client != NULL) 
				{
					TCP_UserDEL(client);
					TCP_EpollCtl(client->new_fd,&client->ev,EPOLL_CTL_DEL);
				}
				pthread_mutex_unlock(&user_mutex);	
			}  
		}  
	}  
	//close(epfd);	
	return NULL;  
}









