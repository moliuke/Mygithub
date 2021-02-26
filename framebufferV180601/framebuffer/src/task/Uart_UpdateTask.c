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
//加入检验码生成
#include "../protocol/PTC_common.h"


#include "Uart_UpdateTask.h"
//#include "../module/syslog.h"

//对于TXRX板，任何一套协议的亮度设置范围都是1-64，但是对不同的协议它所定义的
//亮度范围值有可能不同，有点之定义到0-31，我们只需要将0-64的亮度范围转换成0-31、
//的范围在反馈给上位机即可。也就是说处于方便管理，我们自己内部将亮度范围统一成
//1-64,对有需要的协议在转换成0-31反馈给他

uint8_t file_flag = 1;  // 1表示正常，0表示出错
uint8_t screen_state = 0; // 1表示开屏状态，0表示关屏状态
uint8_t ack_flag = 1; // 1 表示没有存在没应答指令，0存在没有应答指令
pthread_mutex_t ack_flag_mutex;


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
	//返回值至少是8个字节
	if(len < 8 )
		return -1;

	//返回数据的头尾字节要符合协议规定
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
		//查询接收卡详细状态（帧类型“50”）
		case 0x3530:
			
			DP_RXcardDataParsing(protocol.data,protocol.length);		
			break;

		//查询发送卡或接收卡配置参数（帧类型“5A”）
		case 0x3541:
			DP_RXTX_ConfigureParsing(protocol.data,protocol.length);						
			break;

		//查询显示参数信息（帧类型“5B”）直接发不了，只能先存
		case 0x3542:
			//debug_printf("enter to get display parameter\n");
			DP_RXTX_Parametermsg(protocol.data,protocol.length);			
			break;

		/*下面关于设备控制 回收到成功或者失败*/
		//设置亮度模式和亮度（帧类型“06”）
		case 0x3036:
			
			break;
		//设置开关屏（帧类型“04”）
		case 0x3034:
			//开关屏正常
			if(protocol.data[0] == 0x01) 
			 	screen_state = 1;	
			//开关屏异常
			else if(protocol.data[1] == 0x00)
				screen_state = 0;
			break;

		//复位发送卡（帧类型“53”）
		case 0x3533:
			
			break;

		//复位接收卡（帧类型“54”）
		case 0x3534:
			
			break;

		//设置测试状态（帧类型“56”）
		case 0x3536:
			//Set_TestState(protocol.data[0]);	
			break;
			
		//设置显示参数（帧类型”57”）
		case 0x3537:
			
			break;

		//修改和保存显示参数（帧类型”58”）
		case 0x3538:

			break;
			
		//设置发送卡和接收卡配置参数（帧类型”59”）	
		case 0x3539:
						
			break;

		//查询像素点信息（帧类型”5C”）
		case 0x3543:
			DP_PixelsDataParsing(protocol.data,protocol.length);
			break;
		case 0x3532:
			
			break;

		//上传文件 到设备（帧类型“20”）
		case 0x3230:
			//文件传输正常
			if(protocol.data[0] == 0x01) 
			 	file_flag = 1;	
			//文件传输异常
			else if(protocol.data[1] == 0x00)
				file_flag = 0;
			break;

		default:
		 	
		    break;
	}	
		//表示已经收到应答
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
		SetParameter(); 	//这里需要修改，初始化配置
		update_timer_register();//定时任务，需要修改

	}
	//upgrade升级模式下关闭屏幕
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
	time.tv_sec = 5;    //2秒超时
	time.tv_usec = 0;
	memset(recv_buf,0,sizeof(recv_buf));
	
	while(1)
	{
		usleep(1 * 1000);
		FD_ZERO(&fs_read);
		FD_SET(COM2_fd,&fs_read);
		
		//使用select实现串口的多路通信 返回值 >0：就绪描述字的正数目 -1：出错 0 ：超时	  
		fs_sel = select(COM2_fd+1,&fs_read,NULL,NULL,&time);
		if(fs_sel <= 0)
			continue;

		//接收一串完整的数据
		Len = read_data(UARTData,&recv_len); 
		if(Len <= 0)
			continue;

		//检查是否有黏包现象
		FirsLen = SplitPackage(UARTData,Len);

		//无黏包
		if(FirsLen == Len)
		{
			dev_dataprocessor(recv_buf,Len);

		}
		//黏包
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





