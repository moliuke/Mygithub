#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h> 
#include <errno.h> 
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "udp_task.h"
#include "Dev_tcpserver.h"

#include "../protocol/PTC_init.h"

void *pthread_udp_task(void *arg)
{
	
	DEBUG_PRINTF;
	int fd = -1,n = 0;
	int nfds = 0;
	int ServerFD;
	uint32_t RXlen = 0;
	struct epoll_event ev,events[19];
	char RXbuf[1024];
    char szAddr[256]="\0";
    struct sockaddr_in clientaddr;  
    socklen_t addrlen = sizeof(clientaddr);
	if((ServerFD = UDP_ServerInit()) < 0)
	{
	}

	while (1)  
	{  
		DEBUG_PRINTF;
		//等待epoll事件的发生 
		nfds = UDP_EpollWait(events,19,5*1000);
		DEBUG_PRINTF;
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

			fd = events[i].data.fd;

            if(events[i].events&EPOLLIN)                
            {
            	
				client_t user;
				user.new_fd = fd;
				user.ev = events[i];
				DEBUG_PRINTF;
				recv_task_process((void *)&user);
				DEBUG_PRINTF;
            }

		}  
	}  
	//close(epfd);	
	return NULL;  
}
