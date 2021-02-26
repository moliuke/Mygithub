#include <stdio.h>
#include <sys/socket.h>
#include "modbus_task.h"
#include "Dev_serial.h"
#include "modbus_config.h"

#define		SELECT_STATE		0x00
#define 	RECV_STATE			0x01
#define 	FIND_START_BYTE		0x02
#define 	FIND_DEV_ADDR		0x03
#define 	FIND_END_BYTE		0x04
#define		GET_FRAME_BYTES		0x05

#define 	BITS_BYTE			8


//#define MODBUS_ASCII	



uint8_t senddata[248] = {
0x1,	0x10,	0x15,	0x0,	0x0,	0x64,	0xc8,	0x0,	0x2,	0x1,	
0x3,	0x2,	0x4,	0x0,	0x0,	0x1b,	0x22,	0x1b,	0x39,	0x32,	
0x1b,	0x3a,	0x34,	0xd3,	0xea,	0xcc,	0xec,	0xc2,	0xb7,	0xbb,	
0xac,	0x1b,	0xa,	0x1b,	0xd,	0x1b,	0x39,	0x32,	0x1b,	0x3a,	
0x34,	0x1b,	0x21,	0xd0,	0xa1,	0xd0,	0xc4,	0xc2,	0xfd,	0xd0,	
0xd0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	
0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0x0,	0xf7,	0x80
};


#if 0
static void *pthread_MDBRTU_monitor_task(void *arg)
{
	fd_set fs_read;
	int COMfd = 0;
	int Time_s = 10,Time_us = 0;
	int fs_sel = 1;
	struct timeval time;
	int len = 0,total_len = 0;
	int i = 0 , j = 0;
	uint8_t recv_buf[2048+256];
	int offset = 0;
	int TimeOutTs = 0;
	int FrameStartFlag = 0;
	COMfd = serial_grup[xCOM1].fd;
	int BaudRate = serial_grup[xCOM1].baudrate;
	float CharTime = (1000 * 1000) / (BaudRate / BITS_BYTE);
	float Timeout_1P5 = 1.5 * CharTime;
	float Timeout_0P5 = 0.5 * CharTime;
	uint8_t *frameData = recv_buf+sizeof(user_t);
	memset(recv_buf,0,sizeof(recv_buf));
	while(1)
	{
		MDBS_TASK_DEBUG_PRINTF;
	#if 1
		FD_ZERO(&fs_read);
		FD_SET(COMfd,&fs_read);
		time.tv_sec  = Time_s;
		time.tv_usec = Time_us;
		
		//debug_printf("Time_s = %d,Time_us = %d,COMfd = %d\n",Time_s,Time_us,COMfd);
		fs_sel = select(COMfd+1,&fs_read,NULL,NULL,&time);
		//DEBUG_PRINTF;
		//���������³�ʼ��������ʱʱ�䣬��������¿�ʼ
		if(fs_sel < 0)
		{
			Time_s = 10;
			Time_us = 0;
			FrameStartFlag = 0;
			mdbs_task_debug_printf("Sorry,I am wrong!\n");
			continue;
		}
		if(fs_sel == 0)
		{
			MDBS_TASK_DEBUG_PRINTF;
			//֡δ��ʼ����ʱ(����һ�γ�ʱ1.5���ַ�ʱ��)��ֱ�ӷ���
			if(!FrameStartFlag)
			{
				mdbs_task_debug_printf("frame not start and UART_recv time out!\n");
				continue;
			}
			
			MDBS_TASK_DEBUG_PRINTF;
			//֡�ѿ�ʼ
			TimeOutTs += 1;
			MDBS_TASK_DEBUG_PRINTF;
			
			//��ʱ����(����3.5���ַ�ʱ��)����־��һ֡���ݵ����
			if(TimeOutTs == 3)
				goto FRAME_END;

			
			//���ۼƳ�ʱ����(3.0���ַ�ʱ��)�����趨һ��0.5���ַ�ʱ��
			if(TimeOutTs == 2)
			{
				MDBS_TASK_DEBUG_PRINTF;
				Time_s = 0;
				Time_us = Timeout_0P5;
			}
			//DEBUG_PRINTF;
			mdbs_task_debug_printf("TimeOutTs = %d,Time_s = %d,Time_us = %d\n",TimeOutTs,Time_s,Time_us);
			continue;
			
		}

		
		len = uart_Recv(xCOM1, frameData + offset,1024);
#if 0
		debug_printf("recv data is :\n");
		for(j = 0 ; j < len ; j ++)
		{
			debug_printf("#%x ",frameData[j]);
		}
		debug_printf("\n");
#endif		
		//�������������³�ʼ��
		if(len <= 0)
		{
			usleep(500);
			Time_s  = 10;
			Time_us = 0;
			FrameStartFlag = 0;
			offset = 0;
			continue;
		}
		MDBS_TASK_DEBUG_PRINTF;

		//����ո��ַ����Ƿ񷢸��Լ�
		if(TimeOutTs == 0 && frameData[0] != 0x01)
		{
			MDBS_TASK_DEBUG_PRINTF;
			Time_s	= 10;
			Time_us = 0;
			FrameStartFlag = 1;
			offset = 0;
			memset(recv_buf,0,sizeof(recv_buf));
			continue;
		}
		else
		{
			Time_s	= 0;
			Time_us = Timeout_1P5;
			FrameStartFlag = 1;
			offset += len;
			//debug_printf("frameData[0] = 0x%x\n",frameData[0]);
			continue;
		}
		MDBS_TASK_DEBUG_PRINTF;
		//����1.5���ַ�ʱ��С��3.5���ַ�ʱ�䣬��Ϊ���ν��յ�����������һ����Ϣ������
		if(TimeOutTs <= 2)
		{
			offset += len;
			continue;
		}
		MDBS_TASK_DEBUG_PRINTF;
		//��1.5���ַ�ʱ���ڽ��յ������ݶ���Ϊ��ͬһ֡�����ݣ�ֱ�����ƫ��һ���ֽڼ���
		offset += len;
		continue;


		FRAME_END:
			

			EnQueue(QueueHead,recv_buf,offset+sizeof(user_t));
			MDBS_TASK_DEBUG_PRINTF;
			ack_back_table = TABLE_UART_PORT;

			offset = 0;
			FrameStartFlag = 0;
			Time_s	= 10;
			Time_us = 0;
			TimeOutTs = 0;
			memset(recv_buf,0,sizeof(recv_buf));
			MDBS_TASK_DEBUG_PRINTF;
	#else
		DEBUG_PRINTF;
		memcpy(recv_buf + sizeof(user_t),senddata,209);
		DEBUG_PRINTF;
		EnQueue(QueueHead,recv_buf,209+sizeof(user_t));
		ack_back_table = TABLE_UART_PORT;
		DEBUG_PRINTF;
		usleep(200000);
		DEBUG_PRINTF;
	#endif
	}
	return 0;
}


static void *pthread_MDBASCII_monitor_task(void *arg)
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

		

	MDBS_TASK_DEBUG_PRINTF;
	recv_buf = (uint8_t *)malloc(RECV_BUF_SIZE);
	memset(recv_buf,0,sizeof(recv_buf));
	MDBS_TASK_DEBUG_PRINTF;
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
				MDBS_TASK_DEBUG_PRINTF;
				//debug_printf("Time_s = %d,Time_us = %d,COMfd = %d\n",Time_s,Time_us,COMfd);
				fs_sel = select(COMfd+1,&fs_read,NULL,NULL,&time);
				//DEBUG_PRINTF;
				//������ֻҪ���ִ�����߽��ճ�ʱ���ᱻ���³�ʼ����������״̬���¿�ʼ
				if(fs_sel < 0)
				{
					FrameLen = 0;
					FrameStartFlag = 0;
					offset = 0;
					mdbs_task_debug_printf("Sorry,I am wrong!\n");
					continue;
				}
				if(fs_sel == 0)
				{
					FrameLen = 0;
					FrameStartFlag = 0;
					offset = 0;
					mdbs_task_debug_printf("UART_recv time out!\n");
					continue;	
				}

				MDBS_TASK_DEBUG_PRINTF;

				if(FrameStartFlag)
					state = RECV_STATE;
				else
				{
					state = FIND_START_BYTE;
					ReadLen = 1;
				}
				
				break;
			case FIND_START_BYTE:
				MDBS_TASK_DEBUG_PRINTF;
				len = uart_Recv(xCOM1, frameData,ReadLen);
				if(len <= 0)
				{
					state = SELECT_STATE;
					continue;
				}
				MDBS_TASK_DEBUG_PRINTF;
				//������һ���ֽڼ���Ƿ���':'
				if(frameData[0] != ':')
					continue;
				MDBS_TASK_DEBUG_PRINTF;

				//����':',����asciiģʽ֡�Ŀ�ʼ����ʼ����֡����
				FrameStartFlag = 1;
				offset = 0;
				state = RECV_STATE;
				break;

			//����֡����
			case RECV_STATE:
				MDBS_TASK_DEBUG_PRINTF;
				//һ�ζ�ȡ1k������
				ReadLen = (RemaindSize > 1024) ? 1024 : RemaindSize;
				len = uart_Recv(xCOM1, frameData + offset,ReadLen);
				if(len <= 0)
				{
					state = SELECT_STATE;
					continue;
				}
				MDBS_TASK_DEBUG_PRINTF;
				//offset  ����ƫ��Ҳ�ǳ���
				offset += len;
				RemaindSize -= len;
				mdbs_task_debug_printf("offset = %d,len = %d\n",offset,len);
				
				//һ��1k���ݣ�ֱ����ȡ��С��1K�����ݵ�ʱ�򼴿���Ϊ�������������Ѿ��������
				if(len == 1024)
					continue;

				//����������ݺ󣬿�ʼ�ж�ǰ�������ֽڣ�������Լ����豸��ַ���ǺϾͰѸ�֡���ݶ���
				subarg1 = (frameData[0] < 0x3a) ? 0x30 : 0x37;
				subarg2 = (frameData[1] < 0x3a) ? 0x30 : 0x37;
				Dev_Addr = (frameData[0] - subarg1) << 4 | (frameData[1] - subarg2);
				mdbs_task_debug_printf("Dev_Addr = %d,0x%x\n",Dev_Addr,Dev_Addr);
				if(Dev_Addr != 0x01)
				{
					FrameStartFlag = 0;
					state = SELECT_STATE;
					continue;
				}
				MDBS_TASK_DEBUG_PRINTF;


				//ǰ�����ֽ����豸��ַ���Ǻ�ʱ����ʼ������������������������0x0��0x0a�������з�
				for(i = 0 ; i < offset - 2 ; i += 2)
				{
					mdbs_task_debug_printf("offset - 1 = %d,i = %d,frameData[i] = 0x%x\n",offset - 1,i,frameData[i]);
					if(frameData[i] == 0x0d && frameData[i+1] == 0x0a)
						break;
				}

				//�����������ݶ���ⲻ�����з�����ôӦ�ü����������ݵȵ����ݵĵ���
				if(frameData[i] != 0x0d && frameData[i+1] != 0x0a)
				{
					MDBS_TASK_DEBUG_PRINTF;
					FrameLen += offset;
					state = SELECT_STATE;
					continue;
				}

				MDBS_TASK_DEBUG_PRINTF;
				
				FrameLen = i;
				
				state = GET_FRAME_BYTES;
				break;

			case GET_FRAME_BYTES:
				MDBS_TASK_DEBUG_PRINTF;
				EnQueue(QueueHead,recv_buf,FrameLen + sizeof(user_t));
				MDBS_TASK_DEBUG_PRINTF;
				ack_back_table = TABLE_UART_PORT;

				FrameLen = 0;
				offset = 0;
				FrameStartFlag = 0;
				memset(recv_buf,0,sizeof(recv_buf));
				MDBS_TASK_DEBUG_PRINTF;
				state = SELECT_STATE;
				break;
		}

	}
	free(recv_buf);
	return 0;
}
#endif

#define PROTOCOL_SELECT_SWOR	0
#define PROTOCOL_SELECR_MDBS	1



static int modbus_recv(int RXfd,char *RXbuf,uint32_t ReadLen,uint32_t *RXlen)
{
	int i = 0;
	int offset = 0;
	int FirstFrameFlag = 1;		//��Ƕ�ȡ�ĵ�һ֡1024���ֽ�
	int checkCount = 0;
    int recvlen = 0; 
	int getout = 0;
	int  ToreadCNT = 1024;
	int ALLlen = ReadLen;

	while(1)
	{
		recvlen = recv(RXfd, RXbuf + offset, ToreadCNT, 0);
		
		//���ճ���Ϊ0��ʾ�Է��Ѿ��Ͽ�
		if(recvlen == 0)
		{
			return ERR_SOCKET_SHUTDOWN;
		}
		
		//���ճ���С��0��ʾ����
		if (recvlen < 0)  
		{  
			if(errno != EAGAIN && errno != EINTR && errno != EWOULDBLOCK)
				return ERR_ERROR;
		
			//����2048�ν��մ���Ļ���Ϊ�������
			if(offset <= 0)
			{
				getout++;
				if(getout > 256)
					return -1;
				usleep(10);
				continue;
				//return ERR_ERROR;
			}
			*RXlen = offset;
			return ERR_OK;
		}

		getout = 0;
		offset += recvlen;
		ALLlen -= recvlen;
		if(ALLlen > 0)
		{
			ToreadCNT = (ALLlen > 1024) ? 1024 : ALLlen;
			continue;
		}
		else
		{
			*RXlen = offset;
			return ERR_OK;
		}
				

	}
}


int ModbusTCPIP_recv(int RXfd,char *RXbuf,uint32_t *RXlen)  
{  
	int i = 0;
	int offset = 0;
	int FirstFrameFlag = 1;		//��Ƕ�ȡ�ĵ�һ֡1024���ֽ�
	int checkCount = 0;
    int recvlen = 0; 
	int getout = 0;
	int  ToreadCNT = 6;
	int ALLlen = 0;
	uint32_t _RXlen = 0;
	int ret = -1;
	//����modbus����ᷢ��6�������ֽڣ���Ҫ����
	recvlen = recv(RXfd, RXbuf + offset, ToreadCNT, 0);
	
	//���ճ���Ϊ0��ʾ�Է��Ѿ��Ͽ�
	if(recvlen == 0)
	{
		return ERR_SOCKET_SHUTDOWN;
	}

	//���ճ���С��0��ʾ����
	if (recvlen < 0)  
	{  
		return ERR_ERROR;
	}

	//debug_printf("RXbuf[4] = 0x%x,RXbuf[5] = 0x%x\n",RXbuf[4],RXbuf[5]);
	ALLlen = (uint8_t)RXbuf[4] << 8 | (uint8_t)RXbuf[5];
	ToreadCNT = (ALLlen > 1024) ? 1024 : ALLlen;

	ret = modbus_recv(RXfd,RXbuf + 6,ALLlen,&_RXlen);

	*RXlen = 6 + _RXlen;
	debug_printf("*RXlen = %d\n",*RXlen);
	return ret;
}  







#if 0
void *pthread_MODBUS_monitor_task(void *arg)
{
#ifdef MODBUS_ASCII
	MDBS_TASK_DEBUG_PRINTF;
	pthread_MDBASCII_monitor_task(arg);
#else
	MDBS_TASK_DEBUG_PRINTF;
	pthread_MDBRTU_monitor_task(arg);
#endif
}
#endif



