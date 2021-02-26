#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "conf.h"

#include "../PTC_init.h"
#include "../../include/myerror.h"
#include "../../include/debug.h"
#include "UG_protocol.h"
#include "../../include/file_trmit.h"
#include "../../include/wdt.h"
#include "../threadpool.h"
#include "../PTC_common.h"
#include "../PTC_FileRxTx.h"
#include "../../Hardware/Data_pool.h"
#include "../Hardware/HW3G_RXTX.h"



uint8_t upgrade_mode = 0x34; // 30表示升级完成  31表示升级中 32升级失败 33正常模式 34维护模式

static int ConfigFileOps(void)
{
	//备份一个恢复IP用的_cls.conf以及一个正常使用的备份cpy_cls.conf,防止
	//cls.conf被意外修改或者损坏时恢复
	if(access(config_sh,F_OK) >= 0)
		system(config_sh);

	//修改恢复IP的cls.conf的ip、网关、掩码为默认值
	conf_file_write(_cls,"netport","ip","192.168.1.11");
	conf_file_write(_cls,"netport","netmask","255.255.255.0");
	conf_file_write(_cls,"netport","gateway","192.168.1.1");
	conf_file_write(_cls,"netport","port","5168");
	conf_file_write(f_cls,"netport","ip","192.168.1.11");
	conf_file_write(f_cls,"netport","netmask","255.255.255.0");
	conf_file_write(f_cls,"netport","gateway","192.168.1.1");
	conf_file_write(f_cls,"netport","port","5168");


	//接受到上位机发送的配置文件后，要把ip、掩码、网关写入一个脚本文件ipconfig.sh中，再系统
	//启动后就会自动调用这个文件来配置IP信息
	int fd = -1;
	char Wcontent[256];
	char ip[24],netmask[24],gateway[24];
	memset(ip,0,sizeof(ip));
	memset(netmask,0,sizeof(netmask));
	memset(gateway,0,sizeof(gateway));
	memset(Wcontent,0,sizeof(Wcontent));
	conf_file_read(ConFigFile,"netport","ip",ip);
	conf_file_read(ConFigFile,"netport","netmask",netmask);
	conf_file_read(ConFigFile,"netport","gateway",gateway);
	sprintf(Wcontent,"#!/bin/sh\n\nifconfig eth0 %s netmask %s up >/dev/null 2>&1\n/sbin/route add default gw %s\n",ip,netmask,gateway);
	fd = open(IPCONFIG,O_RDWR | O_CREAT,0744);
	if(fd < 0)
	{
		return -1;
	}
	
	//printf("Wcontent = %s\n",Wcontent);
	
	lseek(fd,0,SEEK_SET);
	write(fd,Wcontent,strlen(Wcontent));
	close(fd);
	//system(IPCONFIG);
	
	return 0;
}




static void recvmsg_printf(unsigned char *input,unsigned int inputlen)
{
	int i = 0;
	debug_printf("===============recv data============\n");
	for(i = 0 ; i < inputlen ; i ++)
	{
		debug_printf("0x%x,",input[i]);
	}
	debug_printf("\n");
}


static void prtclmsg_printf(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	int i = 0; 
	debug_printf("\n\n=================================\n");
	debug_printf(
		"ip:				%s\n"
		"port:				%d\n"
		"fd:				%d\n"
		"startbyte:			0x%x\n"
		"cmdID:				0x%x\n"
		"devID:				0x%x\n"
		"length:			%u\n",
		PROTOCOLStruct->user->ip,
		PROTOCOLStruct->user->port,
		PROTOCOLStruct->user->fd,
		PROTOCOLStruct->startByte,
		PROTOCOLStruct->CMDID,
		PROTOCOLStruct->DEVID,
		PROTOCOLStruct->length);	
	debug_printf("data:				");
	for(i = 0 ; i < PROTOCOLStruct->length ; i ++)
	{
		debug_printf("0x%x\t",PROTOCOLStruct->data[i]);
	}

	debug_printf("\n"
		"parity:				0x%x\n"
		"endbyte:			0x%x\n",
		PROTOCOLStruct->parity,
		PROTOCOLStruct->endByte
		);
	debug_printf("=====================================\n\n");
	
}

static UGError_t struct_to_bytes(UG_PROTOCOLStruct_t *PROTOCOLStruct,uint8_t *outputbytes)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	if(PROTOCOLStruct == NULL || outputbytes == NULL)
		return UG_ERR_ARGUMENT;
	
	outputbytes[UG_START_BYTE_POS] = PROTOCOLStruct->startByte;
	outputbytes[UG_CMDID_BYTE_POS + 0] = (unsigned char)(PROTOCOLStruct->CMDID >> 8);
	outputbytes[UG_CMDID_BYTE_POS + 1] = (unsigned char)(PROTOCOLStruct->CMDID);
	outputbytes[UG_DEVID_BYTE_POS + 0] = (unsigned char)(PROTOCOLStruct->DEVID >> 8);
	outputbytes[UG_DEVID_BYTE_POS + 1] = (unsigned char)(PROTOCOLStruct->DEVID);

	for(i = 0 ; i < PROTOCOLStruct->length; i ++)
	{
		//debug_printf("0x%x ",ROTOCOLStruct->data[i]);
		outputbytes[UG_DATAS_BYTE_POS + i]	= PROTOCOLStruct->data[i];
	}

	outputbytes[UG_PARITY_BYTE_POS(PROTOCOLStruct->length) + 0] = (uint8_t)(PROTOCOLStruct->parity >> 8);
	outputbytes[UG_PARITY_BYTE_POS(PROTOCOLStruct->length) + 1] = (uint8_t)(PROTOCOLStruct->parity);
	outputbytes[UG_END_BYTE_POS(PROTOCOLStruct->length)] = PROTOCOLStruct->endByte;
}



void UG_PTCINITstruct(UG_PROTOCOLStruct_t *PROTOCOLStruct,user_t *user,uint8_t *data,uint32_t len)
{
	PROTOCOLStruct->user		= user;
	PROTOCOLStruct->startByte   = data[0];
	PROTOCOLStruct->CMDID		= (uint16_t)data[1] << 8 | data[2];
	PROTOCOLStruct->DEVID		= (uint16_t)data[3] << 8 | data[4];
	
	PROTOCOLStruct->length 		= len - 8;
	PROTOCOLStruct->data		= data + 5;
	PROTOCOLStruct->parity		= data[len - 3] << 8 | data[len - 2];
	PROTOCOLStruct->endByte		= data[len - 1];
}




static int UG_RxFileFrmUpper(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	DEBUG_PRINTF;
	int ret = -1;
	char *Flag0x2B = NULL;

	char *Flag = NULL; //用于清除/
	char *dataPoint = PROTOCOLStruct->data;
	char *filedata = NULL;
	uint32_t filedataLen = 0;
	char filename[24];
	char file_name[24];  //仅仅是文件名
	uint8_t filenameLen = 0;
	uint32_t frameOffset = 0;

	FILEUser_t FileUser;
	uint8_t tmpLen = 0;
	
	//取文件名
	Flag0x2B = strchr(dataPoint,0x2b);
	if(Flag0x2B == NULL)
	{
		debug_printf("recv file data : bad frame data");
		goto EXCEPTION;
	}
	filenameLen = Flag0x2B - dataPoint;
	
	memset(filename,0,sizeof(filename));
	memcpy(filename,dataPoint,filenameLen);
	filename[filenameLen] = '\0';
	debug_printf("filename = %s\n",filename);

	dir_wintolinux(filename);
	Dir_LetterBtoL(filename);

	if((Flag = strchr(filename,'/')) != NULL)
	{
		memset(file_name,0,sizeof(file_name));
		Flag = Flag + 1;
		tmpLen = Flag - filename;
		//printf("tmpLen is %d\n",tmpLen);
		memcpy(file_name,filename+tmpLen,filenameLen-tmpLen);
		memset(filename,0,sizeof(filename));
		memcpy(filename,file_name,filenameLen-tmpLen);
	}
	

	//文件偏移
	dataPoint = Flag0x2B + 1;
	frameOffset = (uint8_t)dataPoint[0] << 24 | (uint8_t)dataPoint[1] << 16 | (uint8_t)dataPoint[2] << 8 | (uint8_t)dataPoint[3];
	//printf("0x%x,0x%x,0x%x,0x%x\n",dataPoint[0],dataPoint[1],dataPoint[2],dataPoint[3]);
	//printf("frameOffset = %d\n",frameOffset);

	//文件数据
	dataPoint += 4;
	filedata = dataPoint;
	filedataLen = PROTOCOLStruct->length - filenameLen - 1 - 4;
	//printf("filedataLen = %d\n",filedataLen);
		
	//文件存放路径
	char filePath[64];
	memset(filePath,0,sizeof(filePath));
	if(strstr(filename,".lst") != NULL)
		sprintf(filePath,"%s/%s",list_dir_1,filename);
	else if(strstr(filename,".bmp") != NULL)
		sprintf(filePath,"%s/%s",image_dir,filename);
	else if(strstr(filename,".conf") != NULL)
		sprintf(filePath,"%s/%s",conf_dir,filename);
	else if(strstr(filename,"bootup") != NULL)
		sprintf(filePath,"%s/%s",boot_dir,filename);
	else
		sprintf(filePath,"%s/%s",sys_dir,filename);

	//printf("*filePath = %s\n",filePath);
	

	//文件帧数据处理
	//printf("*PROTOCOLStruct->user->ip = %s\n",PROTOCOLStruct->user->ip);
	memset(&FileUser,0,sizeof(FILEUser_t));
	FRTx_FileUserInit(&FileUser,PROTOCOLStruct->user->type,PROTOCOLStruct->user->ip,PROTOCOLStruct->user->port,PROTOCOLStruct->user->uartPort);
	if((ret = FRTx_FileFrameRx(&FileUser,filePath,filedata,filedataLen,frameOffset)) < 0)
		goto EXCEPTION;

	//如果是配置文件则复制到几个地方
	#if 1
	if(ret == 1)
	{
		chmod(filePath,0744);
	}
	
	#endif
	PROTOCOLStruct->data[0] = 0x30; 	//0表示成功
	PROTOCOLStruct->length	= 1;
	return 0;

	EXCEPTION:
		printf("a failed frame!\n");
		PROTOCOLStruct->data[0] = 0x31;	//非0表示失败
		PROTOCOLStruct->length  = 1;
		return -1;
		
}


static int UG_TxFileToUpper(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	
	char filename[24];
	char filePath[64];
	uint32_t frameOffset = 0;
	FILEUser_t FileUser;
	char *dataPoint = PROTOCOLStruct->data;
	char *OffsetPos = NULL;
	char *Flag0x2B = NULL;
	char tmpbuf[10];
	char *Flag = NULL;
	int tmplen;
	//取文件名并确定文件所属路径
	memset(filename,0,sizeof(filename));
	memset(filePath,0,sizeof(filePath));
	memset(tmpbuf,0,sizeof(tmpbuf));

	
	//debug_printf("dataPoint = %s\n",dataPoint);
	//printf("dataPoint = %s\n",dataPoint);

    memcpy(tmpbuf,dataPoint,10); 
	dir_wintolinux(tmpbuf);
	if((Flag = strchr(tmpbuf,'/')) != NULL)
	{
		//memset(file_name,0,sizeof(file_name));
		Flag = Flag + 1;
		tmplen = Flag - tmpbuf;
		//printf("tmpLen is %d\n",tmplen);
		memset(filename,0,sizeof(filename));
		memcpy(filename,dataPoint+tmplen,7);
		if((strncmp(filename + 4,"lst",3) == 0) || (strncmp(filename + 4,"LST",3) == 0))
		{
			dataPoint = dataPoint + tmplen + 7;
			Dir_LetterBtoL(filename);
			//debug_printf("filename = %s\n",filename);
			//printf("filename = %s\n",filename);
			filename[7] = '\0';
			sprintf(filePath,"%s/%s",list_dir_1,filename);
		}
		
		else if((strncmp(filename + 4,"bmp",3) == 0) || (strncmp(filename + 4,"BMP",3) == 0))
		{
			dataPoint = dataPoint + tmplen + 7;
			Dir_LetterBtoL(filename);
			//debug_printf("filename = %s\n",filename);
			//printf("filename = %s\n",filename);
			filename[7] = '\0';
			sprintf(filePath,"%s/%s",image_dir,filename);
		}
	}

	else
	{
		if((strncmp(dataPoint,"PLAY.LST",8) == 0) || (strncmp(dataPoint,"play.lst",8) == 0))
		{
			memcpy(filename,dataPoint,8); 
			Dir_LetterBtoL(filename);
			//debug_printf("filename = %s\n",filename);
			//printf("filename = %s\n",filename);
			filename[8] = '\0';

			sprintf(filePath,"%s/%s",list_dir_1,filename);
			dataPoint += 8;
		}


		
		else if((strncmp(dataPoint + 4,"lst",3) == 0) || (strncmp(dataPoint + 4,"LST",3) == 0))
		{
			
			memcpy(filename,dataPoint,7); 
			Dir_LetterBtoL(filename);
			debug_printf("filename = %s\n",filename);
			filename[7] = '\0';
			
			sprintf(filePath,"%s/%s",list_dir_1,filename);
			dataPoint += 7;
		}
		else if((strncmp(dataPoint + 4,"bmp",3) == 0) || (strncmp(dataPoint + 4,"BMP",3) == 0))
		{
			memcpy(filename,dataPoint,7);

			Dir_LetterBtoL(filename);
			
			sprintf(filePath,"%s/%s",image_dir,filename);
			dataPoint += 7;
		}
		else
		{
			if(strncmp(dataPoint,"asc",3) == 0)
			{
				memcpy(filename,dataPoint,7);
				dataPoint += 7;
			}
			else
			{
				memcpy(filename,dataPoint,8);
				Dir_LetterBtoL(filename);
				dataPoint += 8;
			}
			sprintf(filePath,"%s/%s",font_dir,filename);
		}
	}
	debug_printf("filename = %s,filePath = %s\n",filename,filePath);

	//检测文件是否存在
	if(access(filePath,F_OK) < 0)
	{
		PROTOCOLStruct->data[0] = 0x32;//文件已被删除
		PROTOCOLStruct->length  = 1;
		return -1;
	}
	OffsetPos = dataPoint;
	frameOffset = OffsetPos[0] << 24 | OffsetPos[1] << 16 | OffsetPos[2] << 8 | OffsetPos[3];
	debug_printf("frameOffset = %d,OffsetPos[0] = %d\n",frameOffset,OffsetPos[0]);
	debug_printf("PROTOCOLStruct->user->ip = %s,strlen(PROTOCOLStruct->user->ip) = %d\n",PROTOCOLStruct->user->ip,strlen(PROTOCOLStruct->user->ip));
	
	//文件数据处理
	//char *frameData = PROTOCOLStruct->data + 1;
	char *frameData = PROTOCOLStruct->data;
	uint32_t frameDataLen = 0;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,PROTOCOLStruct->user->type,PROTOCOLStruct->user->ip,PROTOCOLStruct->user->port,PROTOCOLStruct->user->uartPort);
	if(FRTx_FileFrameTx(&FileUser,filePath,frameData,&frameDataLen,frameOffset) < 0)
		goto EXCEPTION;

	//PROTOCOLStruct->data[0] = 0x30; 	//0表示成功
	//PROTOCOLStruct->length = frameDataLen + 1;
	PROTOCOLStruct->length = frameDataLen;
	debug_printf("frameDataLen = %d\n",frameDataLen);

	return 0;

	EXCEPTION:
		PROTOCOLStruct->data[0] = 0x00;	//非0表示失败
		PROTOCOLStruct->length  = 1;
		return -1;
}


static int UG_SaveParameter(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	Set_ParameterState(PROTOCOLStruct->data[0]);

	PROTOCOLStruct->data[0] = 0x01;	//非0表示失败
	PROTOCOLStruct->length  = 1;
}


//设置显示参数
static int UG_Set_DisplayParameter(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	Set_DisplayParameter(PROTOCOLStruct->data,&(PROTOCOLStruct->length));

	PROTOCOLStruct->data[0] = 0x01;
	PROTOCOLStruct->length = 1;
}

static int UG_Get_DisplayParameter(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{

	Get_DisplayParameter();
	
	
	usleep(200*1000);
	int ret;
	ret = DP_Get_Display_Parameter(PROTOCOLStruct->data,&(PROTOCOLStruct->length));
	if(ret == -1)
	{
		PROTOCOLStruct->data[0] = 0x00;
		PROTOCOLStruct->length = 1;
	}
	
#if 0
	int i = 0;

	debug_printf("length is %d Get_DisplayParameter is ",PROTOCOLStruct->length);
	int len = PROTOCOLStruct->length;
	for(;i<len;i++)
	{
	debug_printf("%02X ",PROTOCOLStruct->data[i]);

	}
	
	debug_printf("\n");
#endif
}


static int UG_Set_UpgradeParameter(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	//先存数据再取数据
	Set_UpgradeParameter(PROTOCOLStruct->data,&(PROTOCOLStruct->length));
	
	PROTOCOLStruct->data[0] = 0x01;
	PROTOCOLStruct->length = 1;
	

}

/*****************************************************************************
 * 函 数 名  : UG_Get_UpgradeParameter
 * 负 责 人  : QQ
 * 创建日期  : 2020年4月13日
 * 函数功能  : 查询升级参数
 * 输入参数  : uint8_t *data  有效数据
               uint32_t *len  有效数据长度
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static int UG_Get_UpgradeParameter(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t value = 0;
	Get_UpgradeParameter();
	usleep(200*1000);
	PROTOCOLStruct->data[0] = 0x01;	//非0表示失败
	DP_Get_TX_Mode(&value);
	PROTOCOLStruct->data[1] = value;	

	DP_Get_TX_Flag(&value);
	PROTOCOLStruct->data[2] = value;
	
	DP_Get_TX_Boot_Address(&value);
	PROTOCOLStruct->data[3] = value;
	
	DP_Get_TX_Upgrade_Address(&value);
	PROTOCOLStruct->data[4] = value;
	
	DP_Get_RX_Mode(&value);
	PROTOCOLStruct->data[5] = value;	
	
	DP_Get_RX_Flag(&value);
	PROTOCOLStruct->data[6] = value;
	
	DP_Get_RX_Boot_Address(&value);
	PROTOCOLStruct->data[7] = value;
	
	DP_Get_RX_Upgrade_Address(&value);
	PROTOCOLStruct->data[8] = value;

	PROTOCOLStruct->length = 9;
}

static int UG_Resest_RX(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t adrdata = PROTOCOLStruct->data[0];
	RxCardReset(&adrdata);
	PROTOCOLStruct->data[0] = 0x01;
	PROTOCOLStruct->length  = 1;
}

static int UG_Resest_TX(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	TxCardReset();
	PROTOCOLStruct->data[0] = 0x01;
	PROTOCOLStruct->length  = 1;
}

static int UG_Set_ScreenState(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{

	RXTX_SetScreenStatus(PROTOCOLStruct->data[0]);
	PROTOCOLStruct->data[0] = 0x01;
	PROTOCOLStruct->length  = 1;

}

static int UG_Set_TestState(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	Set_TestState(PROTOCOLStruct->data[0]);
	PROTOCOLStruct->data[0] = 0x01;
	PROTOCOLStruct->length  = 1;

}
static int UG_Comnication_state(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	PROTOCOLStruct->data[0] = 0x01;
	PROTOCOLStruct->length  = 1;
}



static int UG_GK_Upgrade(uint8_t *filepath)
{
	char ugcmd[64];
	memset(ugcmd,0,sizeof(ugcmd));
	if(access(filepath,F_OK) < 0)
	{
		debug_printf("file is not exist\n");
		return -1;
	}
	system("cp /bin/ledscreen /home/LEDscr/ledscreen_back");
	system("rm /bin/ledscreen");
	sprintf(ugcmd,"cp %s /bin/ledscreen",filepath);
	system(ugcmd);
	
	memset(ugcmd,0,sizeof(ugcmd));
	sprintf(ugcmd,"cp %s /home/LEDscr/ledscreen",filepath);
	system(ugcmd);
	system("chmod 774 /bin/ledscreen /home/LEDscr/ledscreen");
	
	upgrade_mode = 0x30;
	//system("killall ledscreen");
	return 1;
}

void *routine(void *arg)
{	
	uint8_t ugflag  = *(uint8_t *)arg;
	debug_printf("ugflag is %02X\n",ugflag);
		//升级TX
	if(ugflag == 0x32)
	{
		upgrade_mode = 0x31;
		debug_printf("enter to TX_upgrade_patch send\n");
		UpgradeFile_TX_2K(TxFile,1);
		
	}
	//升级RX
	else if(ugflag == 0x33)
	{
		upgrade_mode = 0x31;
		debug_printf("enter to RX_upgrade_patch send\n");
		UpgradeFile_TX_2K(RxFile,0);
	}
	
	
	pthread_exit(NULL);//由于已分离，该线程退出后自动释放资源
}

//升级设备（帧类型“90”）

static int UG_Deal_ModeOrFile(UG_PROTOCOLStruct_t *PROTOCOLStruct,user_t *user)
{
	int ret = 0;
	uint8_t reply[9] = {0x02,0x39,0x30,0x30,0x30,0x01,0x58,0x6A,0x03};
		//回复上位机	
	if(PROTOCOLStruct->data[0] == 0x34)
	{	
		
		debug_printf("user is type is %d\n",ack_back_table);
		if(ack_back_table == TABLE_NET_PORT)
			send(user->fd,reply,9,0);
		else if(ack_back_table == TABLE_UART_PORT)
			uart_send(xCOM1,reply,9);
		
		char frist_buf[16];
		char second_buf[16];
		memset(frist_buf,0,sizeof(frist_buf));
		memset(second_buf,0,sizeof(second_buf));
		
		conf_file_read(RecordPtcFile,"protocol","protocol",frist_buf);
		conf_file_read(RecordPtcFile,"protocol","swr_protocol",second_buf);
		//保存升级之前的协议，用于升级完成后，切换回原来的协议
		debug_printf("frist protocol is %s  second protocol is %s\n",frist_buf,second_buf);
		conf_file_write(CurrentPtcFile,"protocol","protocol",frist_buf);	
		conf_file_write(CurrentPtcFile,"protocol","swr_protocol",second_buf);
		log_write("Enter to normal reboot",strlen("Enter to normal reboot"));
		system("killall ledscreen");
	}
	//升级工控机
	else if(PROTOCOLStruct->data[0] == 0x31)
	{	
		upgrade_mode = 0x31;
		debug_printf("enter to IPC upgrade\n");
		ret = UG_GK_Upgrade(GkFile);
	}

	else if(PROTOCOLStruct->data[0] == 0x32 || PROTOCOLStruct->data[0] == 0x33)
	{
		//初始化一个属性变量，并将分离属性加入该变量
		upgrade_mode = 0x31;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		//用该属性变量产生一条新的线程
		pthread_t tid;
		uint8_t Ugflag = PROTOCOLStruct->data[0];
		ret = pthread_create(&tid,&attr,routine,&Ugflag);
	}
	if(ret == -1)
	{
		PROTOCOLStruct->data[0] = 0x00;
		PROTOCOLStruct->length = 1; 
		//debug_printf("creating pthread to send file is failed\n");
		upgrade_mode = 0x32;	//升级失败
	}
	else
	{
		PROTOCOLStruct->data[0] = 0x01;
		PROTOCOLStruct->length = 1;
		//upgrade_mode = 0x31;
		//debug_printf("createing pthread\n");
	}
}

void UG_Get_UpgradeState(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	if(upgrade_mode == 0x30)  //升级完成
	{
		PROTOCOLStruct->data[0] = 0x01;
		PROTOCOLStruct->data[1] = upgrade_mode;
		PROTOCOLStruct->length = 2;
		debug_printf("升级完成\n");
		upgrade_mode = 0x34;
		
	}
	else if(upgrade_mode ==0x31)  //正在升级
	{
		PROTOCOLStruct->data[0] = 0x01;
		PROTOCOLStruct->data[1] = upgrade_mode;
		PROTOCOLStruct->length= 2;
		debug_printf("正在升级\n");
		//upgrade_mode = 0x34;
	}
	else if(upgrade_mode == 0x32) //升级失败
	{
		PROTOCOLStruct->data[0] = 0x01;
		PROTOCOLStruct->data[1] = upgrade_mode;
		PROTOCOLStruct->length = 2;
		debug_printf("升级失败\n");
		upgrade_mode = 0x34;

	}
	else if(upgrade_mode == 0x34)  //维护模式
	{
		PROTOCOLStruct->data[0] = 0x01;
		PROTOCOLStruct->data[1] = upgrade_mode;
		PROTOCOLStruct->length = 2;
		debug_printf("维护模式\n");
	}

}

void UG_Get_Version(UG_PROTOCOLStruct_t *PROTOCOLStruct)
{
	Get_UpdateDate();
	usleep(300*1000);

	PROTOCOLStruct->data[0] = 0x01;
	
	//工控机守护程序版本
	DP_Get_MonitorVersion(PROTOCOLStruct->data);

	//工控机版本
	DP_Get_APPVersion(PROTOCOLStruct->data);

	//发送卡版本
	DP_Get_TXVersion(PROTOCOLStruct->data);

    //接收卡版本	
    DP_Get_RXVersion(PROTOCOLStruct->data);

	PROTOCOLStruct->length = (PROTOCOLStruct->data[1] * 2 + 8);
	debug_printf("RXnum is %d ,length is %d\n",PROTOCOLStruct->data[1],PROTOCOLStruct->length);
	
}


int UG_protocolProcessor(user_t *user,uint8_t *PTCdata,uint32_t *PTClenth)
{
	UG_PROTOCOLStruct_t PROTOCOLStruct;
	unsigned short parity = 0;
	unsigned char ptcdata[128];
	uint32_t  len;
	unsigned int nlength;
	unsigned int err = -1;

	unsigned char *parsing_output = NULL;
	unsigned int outlen = 0;

	int output_total_len = 0;
	
	//反转义
	//printf("1*PTClenth = %d\n",*PTClenth);
	err = prtcl_preparsing(PTCdata,*PTClenth,FREEdata,&outlen);
	if(err != 0)
	{
		debug_printf("find error when preparsing the recv data\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"prtcl_preparsing failed",strlen("prtcl_preparsing failed"));
		goto ERRORDEAL;
	}
	//printf("2*PTClenth = %d\n",*PTClenth);
	err = ParityCheck_CRC16(FREEdata,outlen);

	
	//debug_printf("err = %d\n",err);
	if(err != 0)
	{
		debug_printf("find error when parity the recv data\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"CRC failed",strlen("CRC failed"));
		goto ERRORDEAL;
	}
	//给协议信息增加用户信息
	//protocol.usermsg = user;
	//printf("3*PTClenth = %d\n",*PTClenth);
 	int ret = 0; 
	int k = 0;
	UG_PTCINITstruct(&PROTOCOLStruct,user,FREEdata,outlen);

	debug_printf("----------------------recv data-----------------------\n");
	prtclmsg_printf(&PROTOCOLStruct);
	//下面的可忽略不计
	//printf("PROTOCOLStruct.CMDID = 0x%x\n",PROTOCOLStruct.CMDID);
	switch(PROTOCOLStruct.CMDID)
	{
		//通信处理
		case UG_COMMUNICATION_STATE:
			UG_Comnication_state(&PROTOCOLStruct);
			break;
		
		//上位机发送过来
		case UG_FILE_RX:
			UG_RxFileFrmUpper(&PROTOCOLStruct);
			break;

		//下载文件---发送给上位机
		case UG_FILE_TX:
			UG_TxFileToUpper(&PROTOCOLStruct);
			break;
#if 0
		case UG_TX_DETAIL_STATE:
			UG_TxDetailState(&PROTOCOLStruct);
			break;
#endif

		case UG_RESET_TX:
			UG_Resest_TX(&PROTOCOLStruct);
			break;

		case UG_RESET_RX:
			UG_Resest_RX(&PROTOCOLStruct);

			break;

		case UG_SET_TESTMODE:
			UG_Set_TestState(&PROTOCOLStruct);
			break;

		case UG_SAVE_PARAMETER:		
			UG_SaveParameter(&PROTOCOLStruct);			
			break;

		case SET_DISPLAY_PARAMETER:
			UG_Set_DisplayParameter(&PROTOCOLStruct);		
			break;

		case GET_DISPLAY_PARAMETER:
			UG_Get_DisplayParameter(&PROTOCOLStruct);
			break;

		case SET_UPGRADE_PARAMETER:
			UG_Set_UpgradeParameter(&PROTOCOLStruct);
			break;
			
		case GET_UPGRADE_PARAMETER:
			UG_Get_UpgradeParameter(&PROTOCOLStruct);
			break;

		case UG_SET_MODE:
			UG_Deal_ModeOrFile(&PROTOCOLStruct,user);		
			break;
			
		case GET_UPGRADE_STATE:			
			UG_Get_UpgradeState(&PROTOCOLStruct);
			break;

		case UG_GET_VERSION:
			UG_Get_Version(&PROTOCOLStruct);
			break;
		default:
			PROTOCOLStruct.data[0] = 0x00;
			PROTOCOLStruct.length = 1;
			break;
	}
	debug_printf("PROTOCOLStruct.length = %d\n",PROTOCOLStruct.length);
	debug_printf("----------------------send data-----------------------\n");
	//prtclmsg_printf(&PROTOCOLStruct);
	
	struct_to_bytes(&PROTOCOLStruct,FREEdata);
	//校验值
	parity = XKCalculateCRC(FREEdata+1,4+PROTOCOLStruct.length);
	//debug_printf("PROTOCOLStruct.parity = 0x%x\n",PROTOCOLStruct.parity);
	FREEdata[UG_PARITY_BYTE_POS(PROTOCOLStruct.length) + 0] = (uint8_t)(parity >> 8);
	FREEdata[UG_PARITY_BYTE_POS(PROTOCOLStruct.length) + 1] = (uint8_t)(parity);
	//检测字节序中是否存在0x02、0x03、0x1B，有则进行相应的转换，此处处理办法是将头尾两个字节踢掉在处理
	DEBUG_PRINTF;
	check_0x02and0x03(FLAG_SEND,FREEdata+1,4+PROTOCOLStruct.length+2,PTCdata+1,PTClenth);
	DEBUG_PRINTF;
	
	output_total_len = *PTClenth;
	//输出字节序再加上头尾两个字节
	PTCdata[0] = 0x02;
	PTCdata[output_total_len + 1] 	= 0x03;
	//debug_printf("output_total_len = %d,PTCdata[%d] = %d\n",output_total_len,output_total_len + 1,PTCdata[output_total_len + 1]);
	//所以总长度要+2
	output_total_len += 2;
	*PTClenth = output_total_len;

	PROTOCOLStruct.data = NULL;
	//free(FREEdata);
	//FREEdata = NULL;
	
	DEBUG_PRINTF;
	return 0;

	ERRORDEAL:
		
		DEBUG_PRINTF;
		//DEBUG_PRINTF;
		//free(FREEdata);
		//FREEdata = NULL;
		debug_printf("memory has been free!\n");
		return -1;
}





