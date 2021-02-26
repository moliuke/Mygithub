#include "queue_task.h"
#include "task.h"
#include "queue.h"

extern uint8_t ack_flag;  //应答标志
extern pthread_mutex_t ack_flag_mutex;

void *pthread_queue_task(void *arg)
{
	uint32_t datalen = 0;
	int queuelen = 0;
	int time_count = 0;
	
	uint8_t *queuedata = (uint8_t *)malloc(1024*4);
	memset(queuedata,0,4*1024);
	while(1)
	{

		if(ack_flag)
		{
			
			pthread_mutex_lock(&queue_uart_mutex);
			queuelen = Get_headlen(queuehead); 
			pthread_mutex_unlock(&queue_uart_mutex);
			//time_count = 0;
			if(queuelen == 0)
			{
				usleep(5000);
				continue;
			}		
			pthread_mutex_lock(&queue_uart_mutex);
			DeQueue(queuehead,queuedata,&datalen); 			
			pthread_mutex_unlock(&queue_uart_mutex);	  
			if(datalen == 0)
			{
				usleep(5000);
				continue;
			}
			else
			{
				//
				pthread_mutex_lock(&ack_flag_mutex);
				ack_flag = 0;
				pthread_mutex_unlock(&ack_flag_mutex);
				uart_send(xCOM2,queuedata,datalen);
				time_count = 0;
				debug_printf("time_count is %d\n",time_count);
				
			}
			
		}
		else
		{
			time_count++;
			usleep(5*1000);
			if(time_count == 400)
			{
				debug_printf("time_count is %d\n",time_count);
				pthread_mutex_lock(&queue_uart_mutex);
				queuelen = Get_headlen(queuehead); 
				pthread_mutex_unlock(&queue_uart_mutex);
				time_count = 0;
				if(queuelen == 0)
				{
					usleep(5000);
					continue;
				}
				pthread_mutex_lock(&queue_uart_mutex);
				DeQueue(queuehead,queuedata,&datalen); 	 
				pthread_mutex_unlock(&queue_uart_mutex);
				if(datalen == 0)
				{
					usleep(5000);
					continue;
				}
				else
				{
					//超时发送
					debug_printf("ack is timeout\n");
					pthread_mutex_lock(&ack_flag_mutex);
					ack_flag = 0;
					pthread_mutex_unlock(&ack_flag_mutex);
					uart_send(xCOM2,queuedata,datalen);
					time_count = 0;
				}
				
			}
		}
		usleep(5000);	
	}
	
	

}
