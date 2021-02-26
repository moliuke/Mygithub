#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>   
//#include <sys/io.h>
#include <stddef.h>
#include <ctype.h>


#include "task.h"
//#include "../protocol/seewor/SWR_protocol.h"
//#include "../protocol/seewor/SWR_display.h"
//#include "../protocol/perplelight/PPL_display.h"
//#include "../protocol/perplelight/PPL_datapool.h"
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


#include "Uart_Task.h"



//拆分网络黏包，接收数据的时候只检测开头字节0x02与结尾字节0x03，有可能在0x02与0x03
//之间还存在0x02与0x03，即两个包黏在一起了(治超的上位机存在这样的情况)
//处理办法:从0x2往后查找0x3，最多找1024个字节；同时从0x3往前找0x2，最多找1024个字节
static int SplitPackage(char *RXbuf,uint32_t RXlen)
{
	int i = 0;
	for(i = 1 ; i < RXlen - 1 ; i++)
		if(RXbuf[i] == 0x03)
			break;
	if(i == RXlen - 1)
	{
		return RXlen;
	}
	
	return i+1;
}



static int Read_data(uint8_t *RXbuf,uint32_t *RXlen)
{
	uint32_t checkCount = 0;
	uint32_t offset = 0;
	int32_t len = 0;
	int FirstFrameFlag = 1; 
	int loopCount = 0;
	int i = 0,j = 0;
	do
	{
		len = uart_recv(xCOM1, RXbuf + offset,1024);

		//连续循环25次都接收不到数据的话则认为没有数据接收了
		if(len <= 0)
		{
			usleep(10 * 1000);
			loopCount++;
			if(loopCount > 128)
				return offset;
			continue;
		}
		loopCount = 0;
		
		//如果不是第一帧,说明第一帧已经确认了0x02的存在，一旦检测到最后一个字节是0x03，
		//则认为本次接收结束,否则进入下一次循环接收数据
		if(!FirstFrameFlag)
		{
			DEBUG_PRINTF;
			offset += len;
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
			{
				*RXlen = offset;
				return offset;
			}
			continue;
		}

		
		//收到第一帧数据，需要找到开头字节0x02
		for(i = 0 ; i < len ; i++)
		{
			if(RXbuf[i] == 0x02)
				break;
		}
		
		//第一个字节就找到0x02
		if(i == 0)
		{
			DEBUG_PRINTF;
			FirstFrameFlag = 0;
			offset = len;
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
				return offset;
		}
		
		//从第0个字节开始隔i个字节才找到0x02，要将0x02开头的后面所有的字节往前挪
		else if(i < len)
		{
			DEBUG_PRINTF;
			for(j = 0 ; j < len - i ; j++)
				RXbuf[j] = RXbuf[j + i];
			
			FirstFrameFlag = 0;
			offset = len - i;
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
				return offset;
		}

		//找不到0x02,则本次接收到的数据无效
		else
		{
			DEBUG_PRINTF;
			FirstFrameFlag = 1;
			offset = 0;
		}
	}while(1);
}



void *pthread_uart_monitor_task(void *arg)
{
	uint32_t recv_len = 0,Len;
	uint8_t recv_buf[1024 * 4];
	int COM1_fd = serial_grup[xCOM1].fd;
	int32_t FirsLen = 0;
	fd_set fs_read;
	int fs_sel = -1;
	
	uint8_t *UARTData = recv_buf + sizeof(user_t);
	struct timeval time;
	time.tv_sec = 5;
	time.tv_usec = 0;
	memset(recv_buf,0,sizeof(recv_buf));
	
	while(1)
	{
		FD_ZERO(&fs_read);
		FD_SET(COM1_fd,&fs_read);
		
		usleep(10000);
		//使用select实现串口的多路通信	  
		fs_sel = select(COM1_fd+1,&fs_read,NULL,NULL,&time);
		if(fs_sel <= 0 || FD_ISSET(COM1_fd,&fs_read) <= 0)
			continue;

		//接收一串完整的数据
		Len = Read_data(UARTData,&recv_len); 
		if(Len <= 0)
			continue;

		//检查是否有黏包现象
		FirsLen = SplitPackage(UARTData,Len);

		//无黏包
		if(FirsLen == Len)
		{
			pthread_mutex_lock(&queue_mutex);
			EnQueue(QueueHead,recv_buf,Len + sizeof(user_t));
			pthread_mutex_unlock(&queue_mutex);
		}

		//有黏包
		else
		{
			uint32_t SecLen = Len - FirsLen;
			uint8_t *SecFrame = recv_buf + FirsLen;
			pthread_mutex_lock(&queue_mutex);
			EnQueue(QueueHead,recv_buf,FirsLen + sizeof(user_t));
			EnQueue(QueueHead,SecFrame,SecLen + sizeof(user_t));
			pthread_mutex_unlock(&queue_mutex);
		}
		
		//告知协议线程，用串口回复消息
		pthread_mutex_lock(&ackback_mutex);
		ack_back_table = TABLE_UART_PORT;
		pthread_mutex_unlock(&ackback_mutex);
		memset(recv_buf,0,sizeof(recv_buf));

	}
	return 0;
}





