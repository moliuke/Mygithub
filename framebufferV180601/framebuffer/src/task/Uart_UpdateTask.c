#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>   
#include <sys/io.h>
#include <stddef.h>
#include <ctype.h>


#include "task.h"

#include "common.h"
#include "queue.h"
#include "Dev_tcpserver.h"
#include "debug.h"
#include "config.h"
#include "conf.h"
#include "wdt.h"
#include "../update.h"
#include "../clientlist.h"
#include "../threadpool.h"
#include "../module/mtimer.h"
#include "../include/display.h"
#include "../module/image_gif.h"

#include "../Hardware/HW3G_RXTX.h"
#include "../Hardware/HW2G_400.h"


#include "../cache.h"
#include "../Hardware/Data_pool.h"
//�������������
#include "../protocol/PTC_common.h"


#include "Uart_UpdateTask.h"
//#include "../module/syslog.h"

//����TXRX�壬�κ�һ��Э����������÷�Χ����1-64�����ǶԲ�ͬ��Э�����������
//���ȷ�Χֵ�п��ܲ�ͬ���е�֮���嵽0-31������ֻ��Ҫ��0-64�����ȷ�Χת����0-31��
//�ķ�Χ�ڷ�������λ�����ɡ�Ҳ����˵���ڷ�����������Լ��ڲ������ȷ�Χͳһ��
//1-64,������Ҫ��Э����ת����0-31��������

uint8_t file_flag = 1;  // 1��ʾ������0��ʾ����
uint8_t screen_state = 0; // 1��ʾ����״̬��0��ʾ����״̬
uint8_t ack_flag = 1; // 1 ��ʾû�д���ûӦ��ָ�0����û��Ӧ��ָ��
pthread_mutex_t ack_flag_mutex;


//������������������ݵ�ʱ��ֻ��⿪ͷ�ֽ�0x02���β�ֽ�0x03���п�����0x02��0x03
//֮�仹����0x02��0x03�������������һ����(�γ�����λ���������������)
//����취:��0x2�������0x3�������1024���ֽڣ�ͬʱ��0x3��ǰ��0x2�������1024���ֽ�
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

static int read_data(uint8_t *RXbuf,uint32_t *RXlen)
{
	uint32_t checkCount = 0;
	uint32_t offset = 0;
	int32_t len = 0;
	int FirstFrameFlag = 1; 
	int loopCount = 0;
	int i = 0,j = 0;
	do
	{
		len = uart_recv(xCOM2, RXbuf + offset,1024);

		//����ѭ��25�ζ����ղ������ݵĻ�����Ϊû�����ݽ�����
		if(len <= 0)
		{
			usleep(10 * 1000);
			loopCount++;
			if(loopCount > 128)
				return offset;
			continue;
		}
		loopCount = 0;
		
		//������ǵ�һ֡,˵����һ֡�Ѿ�ȷ����0x02�Ĵ��ڣ�һ����⵽���һ���ֽ���0x03��
		//����Ϊ���ν��ս���,���������һ��ѭ����������
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

		
		//�յ���һ֡���ݣ���Ҫ�ҵ���ͷ�ֽ�0x02
		for(i = 0 ; i < len ; i++)
		{
			if(RXbuf[i] == 0x02)
				break;
		}
		
		//��һ���ֽھ��ҵ�0x02
		if(i == 0)
		{
			DEBUG_PRINTF;
			FirstFrameFlag = 0;
			offset = len;
			if(offset > 0 && RXbuf[offset - 1] == 0x03)
				return offset;
		}
		
		//�ӵ�0���ֽڿ�ʼ��i���ֽڲ��ҵ�0x02��Ҫ��0x02��ͷ�ĺ������е��ֽ���ǰŲ
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

		//�Ҳ���0x02,�򱾴ν��յ���������Ч
		else
		{
			DEBUG_PRINTF;
			FirstFrameFlag = 1;
			offset = 0;
		}
	}while(1);
}






int Uart_Byte_to_struct(protcl_msg *protocol,unsigned char *Bytestr,unsigned int Bytelen)
{
	if(protocol == NULL || Bytestr == NULL)
		return -1;
	protocol->head.cmdID = (Bytestr[1] << 8) | Bytestr[2];
	protocol->head.devID = (Bytestr[3] << 8) | Bytestr[4];
	protocol->parity = Bytestr[Bytelen - 3] << 8 | Bytestr[Bytelen - 2];
	protocol->length	= Bytelen - 8;
	protocol->data = Bytestr + 5;
	return 1;
}

int dev_dataprocessor(unsigned char *data,unsigned int len)
{
	uint16_t proto_len = 0;
	uint16_t auto_bright = 0;

	uint8_t temp,hum;
	uint8_t alarm;
	uint8_t smoke;
	uint8_t status = 1;
	uint8_t mode,Bright,Bmax,Bmin;
	uint8_t Lbright = 0,SBright;
	int err = -1;
	unsigned int outlen = 0;
	uint8_t outdata[2224];
	protcl_msg protocol;


	memset(outdata,0,sizeof(outdata));
	//����ֵ������8���ֽ�
	if(len < 8 )
		return -1;

	//�������ݵ�ͷβ�ֽ�Ҫ����Э��涨
	if(data[0] != 0x02 || data[len-1] != 0x03)
		return -1;
	debug_printf("This data from dev_dataprocessor=============\n");

#if 1
	int i = 0;
	for(i = 0 ; i < len ; i++)
		debug_printf("%02x ",data[i]);
	debug_printf("\n\n");
#endif
	err = prtcl_preparsing(data,len,outdata,&outlen);

	if(err != 0)
	{
		debug_printf("find error when preparsing the recv data\n");
		return -1;
	}

	err = ParityCheck_CRC16(outdata,outlen);
	if(err != 0)
	{
		debug_printf("find error when parity the recv data\n");
		return -1;
	}

	Uart_Byte_to_struct(&protocol,outdata,outlen);

	switch(protocol.head.cmdID)
	{	
		case 0x0000:
			break;
		//��ѯ���տ���ϸ״̬��֡���͡�50����
		case 0x3530:
			
			DP_RXcardDataParsing(protocol.data,protocol.length);		
			break;

		//��ѯ���Ϳ�����տ����ò�����֡���͡�5A����
		case 0x3541:
			DP_RXTX_ConfigureParsing(protocol.data,protocol.length);						
			break;

		//��ѯ��ʾ������Ϣ��֡���͡�5B����ֱ�ӷ����ˣ�ֻ���ȴ�
		case 0x3542:
			//debug_printf("enter to get display parameter\n");
			DP_RXTX_Parametermsg(protocol.data,protocol.length);			
			break;

		/*��������豸���� ���յ��ɹ�����ʧ��*/
		//��������ģʽ�����ȣ�֡���͡�06����
		case 0x3036:
			
			break;
		//���ÿ�������֡���͡�04����
		case 0x3034:
			//����������
			if(protocol.data[0] == 0x01) 
			 	screen_state = 1;	
			//�������쳣
			else if(protocol.data[1] == 0x00)
				screen_state = 0;
			break;

		//��λ���Ϳ���֡���͡�53����
		case 0x3533:
			
			break;

		//��λ���տ���֡���͡�54����
		case 0x3534:
			
			break;

		//���ò���״̬��֡���͡�56����
		case 0x3536:
			//Set_TestState(protocol.data[0]);	
			break;
			
		//������ʾ������֡���͡�57����
		case 0x3537:
			
			break;

		//�޸ĺͱ�����ʾ������֡���͡�58����
		case 0x3538:

			break;
			
		//���÷��Ϳ��ͽ��տ����ò�����֡���͡�59����	
		case 0x3539:
						
			break;

		//��ѯ���ص���Ϣ��֡���͡�5C����
		case 0x3543:
			DP_PixelsDataParsing(protocol.data,protocol.length);
			break;
		case 0x3532:
			
			break;

		//�ϴ��ļ� ���豸��֡���͡�20����
		case 0x3230:
			//�ļ���������
			if(protocol.data[0] == 0x01) 
			 	file_flag = 1;	
			//�ļ������쳣
			else if(protocol.data[1] == 0x00)
				file_flag = 0;
			break;

		default:
		 	
		    break;
	}	
		//��ʾ�Ѿ��յ�Ӧ��
	pthread_mutex_lock(&ack_flag_mutex);
	ack_flag = 1;
	pthread_mutex_unlock(&ack_flag_mutex);
	return 0;
}

 

void *pthread_update_task(void *arg)
{
	usleep(100*1000);

	uint8_t flag;
	DP_GetProcotol(&flag);
	
	if(flag != UPGRADE)
	{
		SetParameter(); 	//������Ҫ�޸ģ���ʼ������
		update_timer_register();//��ʱ������Ҫ�޸�

	}
	//upgrade����ģʽ�¹ر���Ļ
	else
		SET_LED_STATE(SLED_OFF);

    uint32_t recv_len = 0,Len;
	uint8_t recv_buf[1024 * 4];
	int COM2_fd = serial_grup[xCOM2].fd;
	int32_t FirsLen = 0;
	fd_set fs_read;
	int fs_sel = -1;

	uint8_t *UARTData = recv_buf;
	struct timeval time;
	time.tv_sec = 5;    //2�볬ʱ
	time.tv_usec = 0;
	memset(recv_buf,0,sizeof(recv_buf));
	
	while(1)
	{
		usleep(1 * 1000);
		FD_ZERO(&fs_read);
		FD_SET(COM2_fd,&fs_read);
		
		//ʹ��selectʵ�ִ��ڵĶ�·ͨ�� ����ֵ >0�����������ֵ�����Ŀ -1������ 0 ����ʱ	  
		fs_sel = select(COM2_fd+1,&fs_read,NULL,NULL,&time);
		if(fs_sel <= 0)
			continue;

		//����һ������������
		Len = read_data(UARTData,&recv_len); 
		if(Len <= 0)
			continue;

		//����Ƿ���������
		FirsLen = SplitPackage(UARTData,Len);

		//����
		if(FirsLen == Len)
		{
			dev_dataprocessor(recv_buf,Len);

		}
		//��
		else
		{
			uint32_t SecLen = Len - FirsLen;
			uint8_t *SecFrame = recv_buf + FirsLen;
			dev_dataprocessor(recv_buf,FirsLen);
			usleep(10*1000);
			dev_dataprocessor(SecFrame,SecLen);
			
		}

		memset(recv_buf, 0, sizeof(recv_buf));

	}

	return 0;
	
}





