#include "ZC_task.h"


void *pthread_ZCuart_monitor_task(void *arg)
{
	fd_set fs_read;
	int COMfd = 0;
	int fs_sel = 1;
	uint8_t subarg1 = 0,subarg2 = 0;
	struct timeval time;
	int len = 0;
	int i = 0 , j = 0;
	uint8_t *recv_buf = NULL;
	int offset = 0;
	int	ReadLen = 1;
	uint8_t Dev_Addr = 0;
	int FrameLen = 0;
	int FrameStartFlag = 0;
	int RemaindSize = RECV_BUF_SIZE;	//标记recv_buf 还能存储多少个字节，防止内存溢出
	int state = SELECT_STATE;			//状态机的初始值
	COMfd = serial_grup[xCOM1].fd;		//串口1
	int BaudRate = serial_grup[xCOM1].baudrate;

		

	DEBUG_PRINTF;
	recv_buf = (uint8_t *)malloc(RECV_BUF_SIZE);
	memset(recv_buf,0,sizeof(recv_buf));
	DEBUG_PRINTF;
	//加上sizeof(user_t)是为了协议处理上的统一，因为显科协议网络数据接收到数据后需要在数据前部加上用户信息user_t
	uint8_t *frameData = recv_buf+sizeof(user_t);	
	while(1)
	{
		switch(state)
		{
			case SELECT_STATE:
				FD_ZERO(&fs_read);
				FD_SET(COMfd,&fs_read);
				time.tv_sec  = 3;
				time.tv_usec = 0;
				DEBUG_PRINTF;
				//debug_printf("Time_s = %d,Time_us = %d,COMfd = %d\n",Time_s,Time_us,COMfd);
				fs_sel = select(COMfd+1,&fs_read,NULL,NULL,&time);
				//DEBUG_PRINTF;
				//错误处理，只要出现错误或者接收超时都会被重新初始化，即接收状态重新开始
				if(fs_sel < 0)
				{
					FrameLen = 0;
					FrameStartFlag = 0;
					offset = 0;
					debug_printf("Sorry,I am wrong!\n");
					continue;
				}
				if(fs_sel == 0)
				{
					FrameLen = 0;
					FrameStartFlag = 0;
					offset = 0;
					debug_printf("UART_recv time out!\n");
					continue;	
				}

				DEBUG_PRINTF;

				if(FrameStartFlag)
					state = RECV_STATE;
				else
				{
					state = FIND_START_BYTE;
					ReadLen = 1;
				}
				
				break;
			case FIND_START_BYTE:
				DEBUG_PRINTF;
				len = uart_Recv(xCOM1, frameData,ReadLen);
				if(len <= 0)
				{
					state = SELECT_STATE;
					continue;
				}
				DEBUG_PRINTF;
				//继续读一个字节检测是否是':'
				if(frameData[0] != 0xFF)
					continue;
				
				DEBUG_PRINTF;

				//检测出':',则标记ascii模式帧的开始，开始接收帧数据
				FrameStartFlag = 1;
				offset = 0;
				state = RECV_STATE;
				break;

			//接收帧数据
			case RECV_STATE:
				DEBUG_PRINTF;
				//一次读取1k的数据
				ReadLen = (RemaindSize > 1024) ? 1024 : RemaindSize;
				len = uart_Recv(xCOM1, frameData + offset,ReadLen);
				if(len <= 0)
				{
					state = SELECT_STATE;
					continue;
				}
				DEBUG_PRINTF;
				//offset  既是偏移也是长度
				offset += len;
				RemaindSize -= len;
				debug_printf("offset = %d,len = %d\n",offset,len);
				
				//一次1k数据，直到读取到小于1K的数据的时候即可认为本次所有数据已经接收完成
				if(len == 1024)
					continue;


				//前两个字节与设备地址相吻合时，开始检测结束符，结束符是连续的0x0跟0x0a，即换行符
				for(i = 0 ; i < offset - 1 ; i++)
				{
					if(frameData[i] == 0xBB)
						break;
				}

				//本次所有数据都检测不到换行符，那么应该继续监听数据等到数据的到来
				if(frameData[i] != 0xBB)
				{
					DEBUG_PRINTF;
					FrameLen += offset;
					state = SELECT_STATE;
					continue;
				}
				
				state = GET_FRAME_BYTES;
				break;

			case GET_FRAME_BYTES:
				DEBUG_PRINTF;
				EnQueue(QueueHead,recv_buf,FrameLen + sizeof(user_t));
				DEBUG_PRINTF;
				ack_back_table = TABLE_UART_PORT;

				FrameLen = 0;
				offset = 0;
				FrameStartFlag = 0;
				memset(recv_buf,0,sizeof(recv_buf));
				DEBUG_PRINTF;
				state = SELECT_STATE;
				break;
		}

	}
	free(recv_buf);
	return 0;
}

