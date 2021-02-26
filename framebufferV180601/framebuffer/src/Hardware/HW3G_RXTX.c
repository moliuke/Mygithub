#include <stdio.h>
#include <string.h>
#include <sys/io.h>

#include "debug.h"
#include "HW3G_RXTX.h"
#include "queue.h"

#include "../task/task.h"
#include "config.h"
#include "Dev_serial.h"
#include "content.h"
#include "mtime.h"
#include "Data_pool.h"
#include "../Task/Uart_UpdateTask.h"
#include "../protocol/PTC_common.h"
#include "../update.h"

#define QUEUE_MAX 10
DEVStatusmsg_t DEV_statusmsg;

//这里加入校验检测  2020.3.16

extern uint8_t file_flag;  // 1表示正常，0表示出错
extern uint8_t upgrade_mode; // 0表示正常  1表示升级中


//extern uint8_t file_flag;



int Set_LEDBright(uint8_t Bright)
{
	uint8_t i = 0;
	uint8_t testmode = 0;
	uint8_t BrightM = 0;

	uint8_t brightVals = 0;
	uint8_t scrstate = 0;
	unsigned short parity = 0;
	uint8_t brightstate[9] = {0x02,0x30,0x36,0x30,0x30,0x00,0x00,0x00,0x03};
	uint8_t output[16] = {0};
	uint32_t outputlen;
	brightVals = Bright;
	
	DP_GetBrightMode(&BrightM);
#if 0
	DP_GetTestMode(&testmode);
	if(brightVals > 0x1F)
		brightVals = 0x1F;
	
	if(testmode == 1)
		scrstate |= 0x40;
	else
		scrstate &= 0xBF;

	if(BrightM == BRIGHT_AUTO)
		scrstate |= 0x80;
	else
		scrstate &= 0x7F;
	
	if(testmode && BrightM == BRIGHT_HAND)
		brightstate[5] = 0x5F;
	else
		brightstate[5] = brightVals|scrstate;
#endif
	brightstate[5] = brightVals;	

	//add by mo 2020.12.10
	if(Lbrightflag == 1)
	{
		brightstate[5] = Bright;
		//printf("bright is %d\n",brightstate[5]);
	}

	parity = XKCalculateCRC(brightstate+1,5);
	brightstate[6] = (unsigned char)(parity >> 8);
	brightstate[7] = (unsigned char)(parity);

	check_0x02and0x03(1,brightstate+1,7,output+1,&outputlen);
	output[0] = 0x02;
	output[outputlen+1] = 0x03;

	debug_printf("send msg to recv card:\n");
	for(i = 0 ; i < outputlen+2 ; i ++)
	{
		debug_printf("%02X ",output[i]);
	}
	debug_printf("\n");
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return -1;
	}
	//uart_send(xCOM2,output,outputlen+2);
	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,output,outputlen+2);
	pthread_mutex_unlock(&queue_uart_mutex);
	return 0;

}

//开屏和关屏指令
int RXTX_SetScreenStatus(uint8_t state)
{
	unsigned short parity = 0;
	uint8_t openScreen[9] = {0x02,0x30,0x34,0x30,0x30,0x00,0x00,0x00,0x03};
	uint8_t output[16] = {0};
	uint32_t outputlen;
	//iopl(3);

	if(state == 1)
	{
		openScreen[5] = 0x01;
	}
	if(state == 0)
	{
		openScreen[5] = 0x00;
	}
	DP_SetScreenStatus(openScreen[5]);

	parity = XKCalculateCRC(openScreen+1,5);
	openScreen[6] = (unsigned char)(parity >> 8);
	openScreen[7] = (unsigned char)(parity);

	check_0x02and0x03(1,openScreen+1,7,output+1,&outputlen);
	output[0] = 0x02;
	output[outputlen+1] = 0x03;
	//uart_send(xCOM2,output,outputlen+2);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return -1;
	}
	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,output,outputlen+2);
	pthread_mutex_unlock(&queue_uart_mutex);
	return 0;
}


void RxCardReset(uint8_t *data)
{
	unsigned short parity = 0;
	uint8_t Rxreset[9] = {0x02,0x35,0x34,0x30,0x30,0x00,0x00,0x00,0x03};
	uint8_t output[16] = {0};
	uint32_t outputlen = 0;
	//memcpy(Rxreset+5,data,1);
	Rxreset[5] = *data;
	parity = XKCalculateCRC(Rxreset+1,5);
	Rxreset[6] = (uint8_t)(parity >> 8);
	Rxreset[7] = (uint8_t)(parity);

	check_0x02and0x03(1,Rxreset+1,7,output+1,&outputlen);
	output[0] = 0x02;
	output[outputlen+1] = 0x03;
	//uart_send(xCOM2,output,outputlen+2);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}
	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,output,outputlen+2);
	pthread_mutex_unlock(&queue_uart_mutex);
}

void TxCardReset(void)
{
	uint8_t rebootTx[9] = {0x02,0x35,0x33,0x30,0x30,0x31,0x7E,0xCE,0x03};
	//uart_send(xCOM2,rebootTx,9);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}
	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,rebootTx,9);
	pthread_mutex_unlock(&queue_uart_mutex);
}

//5.2.5设置测试状态（帧类型“56”）

void Set_TestState(uint8_t state)
{
	unsigned short parity = 0;
	uint8_t TestState[9] = {0x02,0x35,0x36,0x30,0x30,0x00,0x00,0x00,0x03};
	uint8_t output[16];
	uint32_t outputlen;
	memset(output,0,sizeof(output));
	TestState[5] = state;
	parity = XKCalculateCRC(TestState+1,5);
	TestState[6] = (unsigned char)(parity >> 8);
	TestState[7] = (unsigned char)(parity);
	check_0x02and0x03(1,TestState+1,7,output+1,&outputlen);
	output[0] = 0x02;
	output[outputlen+1] = 0x03;
	//uart_send(xCOM2,output,outputlen+2);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}
	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,output,outputlen+2);
	pthread_mutex_unlock(&queue_uart_mutex);
}

//修改和保存显示参数（帧类型”58”）
void  Set_ParameterState(uint8_t state)
{

	unsigned short parity = 0;
	uint8_t patState[9] = {0x02,0x35,0x38,0x30,0x30,0x00,0x00,0x00,0x03};
	patState[5] = state;

	uint8_t output[16];
	uint32_t outputlen;

	memset(output,0,sizeof(output));
	parity = XKCalculateCRC(patState+1,5);
	patState[6] = (unsigned char)(parity >> 8);
	patState[7] = (unsigned char)(parity);
	check_0x02and0x03(1,patState+1,7,output+1,&outputlen);
	#if 0
	int i = 0;
	for(i=0;i<outputlen;i++)
		debug_printf("0x%02X ",output[i+1]);
	debug_printf("\n");
	#endif
	output[0] = 0x02;
	output[outputlen+1] = 0x03;
	//debug_printf("1*****************COM2 msg: send *****************\n");
	//uart_send(xCOM2,output,outputlen+2);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}

	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,output,outputlen+2);
	pthread_mutex_unlock(&queue_uart_mutex);
}

//设置显示参数（帧类型”57”）
void Set_DisplayParameter(uint8_t *data, uint32_t *len)
{

	unsigned short parity = 0;
	uint8_t parameter[512]={0x02,0x35,0x37,0x30,0x30,0x00};
	uint8_t output[512];
	uint32_t datalen = *len;
	uint32_t outputlen;
	//memset(parameter,0,sizeof(parameter));
	memset(output,0,sizeof(output));
	memcpy(parameter+5,data,datalen);
	parity = XKCalculateCRC(parameter+1,datalen+4);

	parameter[datalen+5] = (unsigned char)(parity >> 8);
	parameter[datalen+6] = (unsigned char)(parity);
	check_0x02and0x03(1,parameter+1,datalen+6,output+1,&outputlen);
	output[0] = 0x02;
	output[outputlen+1] = 0x03;
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}

	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,output,outputlen+2);
	pthread_mutex_unlock(&queue_uart_mutex);
	//uart_send(xCOM2,output,outputlen+2);	
	
}

//查询显示参数信息（帧类型“5B”）
void Get_DisplayParameter()
{
	uint8_t parameter[8] = {0x02,0x35,0x42,0x30,0x30,0xD0,0xA7,0x03};
	//uart_send(xCOM2,parameter,8);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}

	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,parameter,8);
	pthread_mutex_unlock(&queue_uart_mutex);	
}
//add by mo 20201214
void SetAndGetcmd(uint8_t *data,uint8_t len)
{
	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,data,len);
	pthread_mutex_unlock(&queue_uart_mutex);

}


//设置发送卡和接收卡配置参数（帧类型”59”）
void Set_UpgradeParameter(uint8_t *data, uint32_t *len)
{
	unsigned short parity = 0;
	uint8_t Ugpat[16] = {0x02,0x35,0x39,0x30,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03};
	uint32_t data_len = *len; 
	memcpy(Ugpat+5,data,data_len);
	uint8_t output[32];
	memset(output,0,sizeof(output));
	uint32_t outputlen;
	int datalen;
	
	parity = XKCalculateCRC(Ugpat+1,4+data_len);
	Ugpat[5+data_len] = (unsigned char)(parity >> 8);
	Ugpat[6+data_len] = (unsigned char)(parity);

	//debug_printf("CRC is 0x%04X\n",parity);
	datalen = check_0x02and0x03(1,Ugpat+1,6+data_len,output+1,&outputlen);
	//debug_printf("datalen is %d\n",datalen);
	output[0] = 0x02;
	output[datalen+1] = 0x03;
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}

	//uart_send(xCOM2,output,datalen+2);
	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,output,datalen+2);
	pthread_mutex_unlock(&queue_uart_mutex);
}



//查询发送卡或接收卡配置参数（帧类型“5A”）
void Get_UpgradeParameter()
{
	uint8_t Ugpat[8] = {0x02,0x35,0x41,0x30,0x30,0x89,0xF7,0x03};
	//uart_send(xCOM2,Ugpat,8);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}

	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,Ugpat,8);
	pthread_mutex_unlock(&queue_uart_mutex);
	
}

#if 1

//查询接收卡详细状态（帧类型“50”）

void Get_UpdateDate(void)
{
	uint8_t update[8] = {0x02,0x35,0x30,0x30,0x30,0x66,0xCF,0x03};
	//uart_send(xCOM2,update,8);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}

	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,update,8);
	pthread_mutex_unlock(&queue_uart_mutex);

}

#endif
//设置像素点检测模式
void Set_Pixels_Mode(uint8_t pixmode)
{
	DP_SetPixelsMode(pixmode);
	unsigned short parity = 0;
	uint8_t pix[9] = {0x02,0x35,0x32,0x30,0x30,0x00,0x00,0x00,0x03};
	pix[5] = pixmode;

	uint8_t output[16];
	uint32_t outputlen;

	memset(output,0,sizeof(output));
	parity = XKCalculateCRC(pix+1,5);
	pix[6] = (unsigned char)(parity >> 8);
	pix[7] = (unsigned char)(parity);
	check_0x02and0x03(1,pix+1,7,output+1,&outputlen);
	#if 0
	int i = 0;
	for(i=0;i<outputlen;i++)
		debug_printf("0x%02X ",output[i+1]);
	debug_printf("\n");
	#endif
	output[0] = 0x02;
	output[outputlen+1] = 0x03;
	//debug_printf("1*****************COM2 msg: send *****************\n");
	//uart_send(xCOM2,output,outputlen+2);	
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
//队列成员满10个后，就不再入队了
	if(queue_num > QUEUE_MAX)
	{
		return ;
	}

	pthread_mutex_lock(&queue_uart_mutex);
	EnQueue(queuehead,output,outputlen+2);
	pthread_mutex_unlock(&queue_uart_mutex);

}
/*****************************************************************************
 * 函 数 名  : UpgradeFile_TX_2K
 * 负 责 人  : QQ
 * 创建日期  : 2020年4月10日
 * 函数功能  : 用于与下位传输升级文件，传输帧2k
 * 输入参数  : char *filepath  升级文件路径
               int flag        表示升级TX  1还是RX  0
 * 输出参数  : 无
 * 返 回 值  : 1:传送成功 -1:读写文件失败  0:表示下位机回复超时
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
int UpgradeFile_TX_2K(char *filepath ,int flag)
{
	FILE *filefd;
    struct stat statbuf;
	uint8_t check_buffer[2100];
	uint8_t output[3000];
	uint32_t outputlen;
	int count = 0; //统计回复超时
	memset(check_buffer,0,sizeof(check_buffer));
	memset(output,0,sizeof(output));
	
	int filesize;
	uint32_t datalen;
	int overplus;
	unsigned short parity = 0;
	int readlen;

	uint8_t fileoffset[4];
	int i = 0;

	uint8_t framehead_chose[2][21] = {
		{0x02,0x32,0x30,0x30,0x30,0x31,0x30,0x30,0x31,0x31,0x52,0x58,0x5F,0x43,0x41,0x52,0x44,0x2E,0x62,0x69,0x6E },  //接收卡RX_CARD.bin
		{0x02,0x32,0x30,0x30,0x30,0x31,0x30,0x30,0x31,0x31,0x54,0x58,0x5F,0x43,0x41,0x52,0x44,0x2E,0x62,0x69,0x6E }    //发送卡TX_CARD.bin
		};

	uint8_t framehead[21] = {0};

	if(flag == 0)
		memcpy(framehead,framehead_chose[0],21);
	else if(flag == 1)
		memcpy(framehead,framehead_chose[1],21);

	//判断文件是否存在
	if(access(filepath,F_OK) < 0)
	{
		debug_printf("FRTx_FILERTXStructInit : The file [ %s ] is not exist!\n ",filepath);
		return -1;
	}
	//取文件大小
	stat(filepath,&statbuf);
	filesize = statbuf.st_size;
	overplus = statbuf.st_size;
	debug_printf("filesize is %d\n",filesize);
	filefd=fopen(filepath,"r");	
	if(filefd==NULL)
	{
		perror("打开文件失败!\n");
		return -1;
	}
	
	while(1)
	{
	//这里需要加入标记位,只有得到回复后，才继续重发，每隔发100ms检测一次
		if(file_flag)
		{
			file_flag = 0;
			count = 0;
			if(overplus >= 2048)
			{
				
				memcpy(check_buffer,framehead,21);
				//文件偏移4字节
				if(i<10)
				{
					fileoffset[0] = 0x30;
					fileoffset[1] = 0x30;
					fileoffset[2] = 0x30;
					fileoffset[3] = 0x30 + i;
				}
				
				if(i >= 10 && i < 100) 
				{
					sprintf(fileoffset,"%d",i);
					fileoffset[3] = fileoffset[1];
					fileoffset[2] = fileoffset[0];
					fileoffset[0] = 0x30;
					fileoffset[1] = 0x30;
				}
				
				if(i >=100 && i <1000)
				{
					sprintf(fileoffset,"%d",i);
					fileoffset[3] = fileoffset[2];
					fileoffset[2] = fileoffset[1];
					fileoffset[1] = fileoffset[0];
					fileoffset[0] = 0x30;
					
				}
				if(i >=1000 && i <10000)
				{
					sprintf(fileoffset,"%d",i);
				}	


				
				
				memcpy(check_buffer+21,fileoffset,4);

				fseek(filefd,i*2048,SEEK_SET);
				readlen = fread(check_buffer+25,1,2048,filefd);
				if(readlen != 2048)
				{
					return -1;
				}

				parity = XKCalculateCRC(check_buffer+1,2072);

				check_buffer[2073] = (uint8_t)(parity >> 8);
				check_buffer[2074] = (uint8_t)(parity);
				#if 0
				debug_printf("\n\n");
				int k=0;
				for(;k < 2075;k++)
				{
					debug_printf("%02X ",check_buffer[k]);
				}
				debug_printf("\n\n");
				debug_printf("CRC is 0x%02X 0x%02X\n",check_buffer[2073],check_buffer[2074]);
				#endif

				
				check_0x02and0x03(1,check_buffer+1,2074,output+1,&outputlen);
				
				output[0] = 0x02;
				output[outputlen+1] = 0x03;
				uart_send(xCOM2,output,outputlen+2);
				memset(check_buffer,0,sizeof(check_buffer));
				memset(output,0,sizeof(output));

			}
			else if(overplus < 2048 && overplus > 0)
			{
				memcpy(check_buffer,framehead,21);
				//文件偏移4字节
				if(i<10)
				{
					fileoffset[3] = 0x30 + i;
					
				}
				
				if(i >= 10 && i < 100) 
				{
					sprintf(fileoffset,"%d",i);
					fileoffset[3] = fileoffset[1];
					fileoffset[2] = fileoffset[0];
					fileoffset[0] = 0x30;
					fileoffset[1] = 0x30;
				}
				
				if(i >=100 && i <1000)
				{
					sprintf(fileoffset,"%d",i);
					fileoffset[3] = fileoffset[2];
					fileoffset[2] = fileoffset[1];
					fileoffset[1] = fileoffset[0];
					fileoffset[0] = 0x30;
					
				}
				if(i >=1000 && i <10000)
				{
					sprintf(fileoffset,"%d",i);
				}	
				
				memcpy(check_buffer+21,fileoffset,4);

				fseek(filefd,i*2048,SEEK_SET);
				readlen = fread(check_buffer+25,1,overplus,filefd);

				if(readlen != overplus)
				{
					fclose(filefd);
					file_flag = 1;
					return -1;
				}
				//memcpy(check_buffer+16,filebuf+i*2048,overplus);
				parity = XKCalculateCRC(check_buffer+1,overplus+24);

				
				
				check_buffer[overplus+25] = (unsigned char)(parity >> 8);
				check_buffer[overplus+26] = (unsigned char)(parity);

				datalen = check_0x02and0x03(1,check_buffer+1,overplus+26,output+1,&outputlen);
				output[0] = 0x02;
				output[outputlen+1] = 0x03;
				uart_send(xCOM2,output,outputlen+2);
				memset(check_buffer,0,sizeof(check_buffer));
				memset(output,0,sizeof(output));
				fclose(filefd);
				upgrade_mode = 0x30;
				return 1;
			}
			//最后一帧刚好为2048时，这里发送一个空帧
			if(overplus == 0)
			{
				#if 1
				memcpy(check_buffer,framehead,21);
				//文件偏移4字节
				if(i<10)
				{
					fileoffset[3] = 0x30 + i;
					
				}
				
				if(i >= 10 && i < 100) 
				{
					sprintf(fileoffset,"%d",i);
					fileoffset[3] = fileoffset[1];
					fileoffset[2] = fileoffset[0];
					fileoffset[0] = 0x30;
					fileoffset[1] = 0x30;
				}
				
				if(i >=100 && i <1000)
				{
					sprintf(fileoffset,"%d",i);
					fileoffset[3] = fileoffset[2];
					fileoffset[2] = fileoffset[1];
					fileoffset[1] = fileoffset[0];
					fileoffset[0] = 0x30;
					
				}
				if(i >=1000 && i <10000)
				{
					sprintf(fileoffset,"%d",i);
				}
				memcpy(check_buffer+21,fileoffset,4);
				
				parity = XKCalculateCRC(check_buffer+1,24);
				check_buffer[25] = (unsigned char)(parity >> 8);
				check_buffer[26] = (unsigned char)(parity);
				datalen = check_0x02and0x03(1,check_buffer+1,26,output+1,&outputlen);
				
				output[0] = 0x02;
				output[outputlen+1] = 0x03;
				uart_send(xCOM2,output,outputlen+2);
				memset(check_buffer,0,sizeof(check_buffer));
				memset(output,0,sizeof(output));
				fclose(filefd);
				upgrade_mode = 0x30;
				return 1;
				#endif
				//return 1;
			}
			
			i++;
			overplus = overplus - 2048;
			
			usleep(50*1000);
		}
		else
		{
			usleep(100*1000);
			count ++;
		}
		//超过2秒没有收到TX回复，则表示传输失败
		if(count >= 20)
		{
			fclose(filefd);
			file_flag = 1;
			upgrade_mode = 0x32;
			return 0;

		}
	}

}









