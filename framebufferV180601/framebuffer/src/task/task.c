
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>   


#include "task.h"
#include "debug.h"
#include "config.h"
#include "../Hardware/Data_pool.h"

#include "Frame_task.h"
#include "Net_task.h"
#include "Uart_UpdateTask.h"
#include "Protocol_task.h"
#include "Display_task.h"
#include "Uart_Task.h"
#include "FileRx_task.h"
#include "udp_task.h"
#include "ping_task.h"
#include "queue_task.h"


#define outp(a, b) outb(b, a)
#define inp(a) inb(a)


int displayFlag = 0;


#ifdef CONFIG_VIDEO_TEST
#include "SDL/SDL.h" 
#endif



//pthread_cond_t  user_cond;
pthread_mutex_t queue_mutex;

pthread_mutex_t queue_uart_mutex;


#if 1
static pthread_t tid_ping_task;
static pthread_t tid_update_task;
static pthread_t tid_protocol_task; 
static pthread_t tid_net_monitor_task;
static pthread_t tid_udp_task;
static pthread_t tid_file_recv_task;
static pthread_t tid_display_task;
static pthread_t tid_SDL_task;
static pthread_t tid_uart_monitor_task;
static pthread_t tid_frame_break_task;

static pthread_t tid_uart_task; //add by mo 2020.07.11
//static void *pthread_protocol_task(void *arg);
//static void *pthread_display_task(void *arg);
//static void *pthread_update_task(void *arg);
//static void *pthread_monitor_task(void *arg);
//static void *pthread_framebreak_task(void *arg);
//static void *pthread_uart_monitor_task(void *arg);
static void TASK_PthreadExit(void);
static int TASK_PthreadCreate(void);
int protocol_send_back_net(user_t *user,uint8_t *tx_buf,int tx_len);



void TASK_SystemStart(void)
{
	if(TASK_PthreadCreate() < 0)
		exit(1);
}

void TASK_SystemExit(void)
{
	TASK_PthreadExit();
}







static void TASK_PthreadExit(void)
{
#ifdef CONFIG_TASK_UPDATE
	pthread_join(tid_update_task,NULL);
#endif
#ifdef CONFIG_TASK_PROTOCOL
	pthread_join(tid_protocol_task,NULL);
#endif
#ifdef CONFIG_TASK_MONITOR
	pthread_join(tid_net_monitor_task,NULL);
#endif
#ifdef CONFIG_UDP_TASK
	pthread_join(tid_udp_task,NULL);
#endif

#ifdef CONFIG_TASK_UART_MONITOR
	pthread_join(tid_uart_monitor_task,NULL);
#endif
#ifdef CONFIG_TASK_DISPLAY
	pthread_join(tid_display_task,NULL);
#endif
#ifdef CONFIG_TASK_FRAMEBREAK
	pthread_join(tid_frame_break_task,NULL);
#endif
#ifdef CONFIG_FILE_RECV
	pthread_join(tid_file_recv_task,NULL);
#endif
	
#ifdef CONFIG_VIDEO_TEST
	pthread_join(tid_SDL_task,NULL);
#endif

}


static int TASK_PthreadCreate(void)
{


	uint8_t flag;
	DP_GetProcotol(&flag);
	int ret = -1;
	
#ifdef CONFIG_TASK_UPDATE
	ret = pthread_create(&tid_update_task,NULL,pthread_update_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
#endif

#ifdef CONFIG_TASK_PROTOCOL
	ret = pthread_create(&tid_protocol_task,NULL,pthread_protocol_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail");
		return -1;
	}
#endif
#if 0
#ifdef CONFIG_TASK_MONITOR
	ret = pthread_create(&tid_net_monitor_task,NULL,pthread_monitor_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
#endif

#ifdef CONFIG_UDP_TASK
ret = pthread_create(&tid_udp_task,NULL,pthread_udp_task,NULL);
if(ret!=0)
{
	perror("pthread_create fail"); 
	return -1;
}
#endif
#endif
//UDP通信方式
if(flag == ZHONGHAIZC)
{
	ret = pthread_create(&tid_udp_task,NULL,pthread_udp_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
}
//TCP通信方式
else
{
	ret = pthread_create(&tid_net_monitor_task,NULL,pthread_monitor_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
}

#ifdef CONFIG_FILE_RECV
ret = pthread_create(&tid_file_recv_task,NULL,File_recv_task,NULL);
if(ret!=0)
{
	perror("pthread_create fail"); 
	return -1;
}
#endif
//change by mo 20201229 通过配置文件决定是否开启
char KeyVal[8];
memset(KeyVal,0,sizeof(KeyVal));
conf_file_read(CHECKPATH,"ping","ping",KeyVal);
//#ifdef CONFIG_TASK_PING
if(strncmp(KeyVal,"ON",2) == 0)
{
	ret = pthread_create(&tid_ping_task,NULL,ping_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	} 
}

//#endif



//pthread_MODBUS_monitor_task:	modbus入口
//pthread_uart_monitor_task:		显示协议的串口1
#ifdef CONFIG_TASK_UART_MONITOR
	ret = pthread_create(&tid_uart_monitor_task,NULL,pthread_uart_monitor_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
#endif

//pthread_Mdbsframebreak_task
//pthread_framebreak_task


#ifdef CONFIG_TASK_FRAMEBREAK
	if(flag != UPGRADE)
	{
		ret = pthread_create(&tid_frame_break_task,NULL,pthread_Mdbsframebreak_task,NULL);
		if(ret!=0)
		{
			perror("pthread_create fail"); 
			return -1;
		}
	}	
#endif

#ifdef CONFIG_TASK_DISPLAY
	if(flag != UPGRADE)
	{
		ret = pthread_create(&tid_display_task,NULL,pthread_display_task,NULL);
		if(ret!=0)
		{
			perror("pthread_create fail");
			return -1;
		}
	}
#endif
	
#ifdef CONFIG_VIDEO_TEST
	ret = pthread_create(&tid_SDL_task,NULL,pthread_SDL_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail");
		return -1;
	}
#endif
//针对TX串口入队列的数据处理
	ret = pthread_create(&tid_uart_task,NULL,pthread_queue_task,NULL);
	if(ret != 0)
	{
		perror("pthread_create fail");
		return -1;
	}



	return 0;
}

#endif





