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
	int RemaindSize = RECV_BUF_SIZE;	//���recv_buf ���ܴ洢���ٸ��ֽڣ���ֹ�ڴ����
	int state = SELECT_STATE;			//״̬���ĳ�ʼֵ
	COMfd = serial_grup[xCOM1].fd;		//����1
	int BaudRate = serial_grup[xCOM1].baudrate;

		

	DEBUG_PRINTF;
	recv_buf = (uint8_t *)malloc(RECV_BUF_SIZE);
	memset(recv_buf,0,sizeof(recv_buf));
	DEBUG_PRINTF;
	//����sizeof(user_t)��Ϊ��Э�鴦���ϵ�ͳһ����Ϊ�Կ�Э���������ݽ��յ����ݺ���Ҫ������ǰ�������û���Ϣuser_t
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
				//������ֻҪ���ִ�����߽��ճ�ʱ���ᱻ���³�ʼ����������״̬���¿�ʼ
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
				//������һ���ֽڼ���Ƿ���':'
				if(frameData[0] != 0xFF)
					continue;
				
				DEBUG_PRINTF;

				//����':',����asciiģʽ֡�Ŀ�ʼ����ʼ����֡����
				FrameStartFlag = 1;
				offset = 0;
				state = RECV_STATE;
				break;

			//����֡����
			case RECV_STATE:
				DEBUG_PRINTF;
				//һ�ζ�ȡ1k������
				ReadLen = (RemaindSize > 1024) ? 1024 : RemaindSize;
				len = uart_Recv(xCOM1, frameData + offset,ReadLen);
				if(len <= 0)
				{
					state = SELECT_STATE;
					continue;
				}
				DEBUG_PRINTF;
				//offset  ����ƫ��Ҳ�ǳ���
				offset += len;
				RemaindSize -= len;
				debug_printf("offset = %d,len = %d\n",offset,len);
				
				//һ��1k���ݣ�ֱ����ȡ��С��1K�����ݵ�ʱ�򼴿���Ϊ�������������Ѿ��������
				if(len == 1024)
					continue;


				//ǰ�����ֽ����豸��ַ���Ǻ�ʱ����ʼ������������������������0x0��0x0a�������з�
				for(i = 0 ; i < offset - 1 ; i++)
				{
					if(frameData[i] == 0xBB)
						break;
				}

				//�����������ݶ���ⲻ�����з�����ôӦ�ü����������ݵȵ����ݵĵ���
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

