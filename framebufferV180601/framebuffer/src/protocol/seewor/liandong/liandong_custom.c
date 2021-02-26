#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "liandong_custom.h"
#include "content.h"
#include "../SWR_init.h"
#include "../SWR_protocol.h"
#include "../../PTC_init.h"
#include "../../../update.h"



static pthread_t tid_test_task;
static pthread_t tid_check_task;
uint8_t refresh_state = 0;//关屏

int data_process(uint8_t *buf, uint16_t len)
{
	uint16_t DataLen = len;
	uint8_t *ptr = buf;
	uint8_t data = 0;
	while(DataLen)
	{
		if(ptr[0] != 0x02)
		{
			ptr += 1;
			DataLen -= 1;
			continue;
		}
		if(ptr[1] != 0x30)
		{
			ptr += 1;
			DataLen -= 1;
			continue;
		}
		data = ptr[1];
		break;
	}
	
	return data;
}

void *pthread_test_task(void *arg)
{
	struct timeval time;
	fd_set fd_read;
	int fs_sel = -1;
	uint8_t RXbuf[1024];
	uint16_t Len;
	uint8_t signalstate;
	uint8_t TXbuf[2] = {0x02,0x31};
	
	serial_param_set(xCOM3,9600,8,1,'N','N');
	uart_init(xCOM3);
	int COM3_fd = serial_grup[xCOM3].fd;
	while(1)
	{
		usleep(20*1000);
		FD_ZERO(&fd_read);
		FD_SET(COM3_fd,&fd_read);
		time.tv_sec = 5;
		time.tv_usec = 0;
		

		fs_sel = select(COM3_fd+1,&fd_read,NULL,NULL,&time);
		if(fs_sel < 0)
		{
			continue;
		}
		if(fs_sel == 0)
		{
			refresh_state = 0;
			continue;
		}

		if(FD_ISSET(COM3_fd,&fd_read) <= 0)
		{
			continue;
		}

		Len = uart_recv(xCOM3, RXbuf,1024);		
		int i = 0;
		#if 0
		printf("recv is : \n");
		for(i=0;i<Len;i++)
		{
			printf("%02X ",RXbuf[i]);
			
		}
		printf("\n");
		#endif
		signalstate = data_process(RXbuf,Len);
		
		if(signalstate == 0x30)
		{
			//开屏
			SET_LED_STATE(0x01);
			uart_send(xCOM3,TXbuf,2);
			refresh_state = 1;
			LEDstateRecord(0x01);
			
		}
		
		
	}
	
	
}
void *pthread_check_task(void *arg)
{
	uint8_t count = 0;
	while(1)
	{
		
		if(refresh_state == 1)
		{
			count = 0;
		}
		count ++;
		sleep(1);
		if(count == 15)
		{
			SET_LED_STATE(0x00);
			LEDstateRecord(0);
		}
		
	}
}
int test_pthread_create(void)
{
	int ret = -1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//设置线程属性为分离状态
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	
	ret = pthread_create(&tid_test_task,&attr,pthread_test_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
	ret = pthread_create(&tid_check_task,&attr,pthread_check_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
	return 0;
}
char *liandongProject(void)
{
	return "liandong";
}


