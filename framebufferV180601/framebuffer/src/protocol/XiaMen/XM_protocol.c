
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/socket.h>

#include "../PTC_init.h"
#include "myerror.h"
#include "debug.h"
#include "XM_protocol.h"
#include "file_trmit.h"
#include "wdt.h"
#include "conf.h"

#include "../threadpool.h"
#include "../PTC_common.h"
#include "../PTC_FileRxTx.h"
#include "../../Hardware/Data_pool.h"
#include "../../Hardware/HW2G_400.h"
#include "../../Hardware/HW3G_RXTX.h"


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


static void prtclmsg_printf(XM_PROTOCOLStruct_t *PROTOCOLStruct)
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
		"timestamp:			%d:%d\n"
		"length:			%u\n",
		PROTOCOLStruct->user->ip,
		PROTOCOLStruct->user->port,
		PROTOCOLStruct->user->fd,
		PROTOCOLStruct->startByte,
		PROTOCOLStruct->CMDID,
		PROTOCOLStruct->DEVID,
		PROTOCOLStruct->timestamp[0] - 0x20,PROTOCOLStruct->timestamp[1] - 0x20,
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

static XMError_t struct_to_bytes(XM_PROTOCOLStruct_t *PROTOCOLStruct,uint8_t *outputbytes)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	if(PROTOCOLStruct == NULL || outputbytes == NULL)
		return XM_ERR_ARGUMENT;
	
	outputbytes[XM_START_BYTE_POS] = PROTOCOLStruct->startByte;
	outputbytes[XM_DEVID_BYTE_POS + 0] = (uint8_t)(PROTOCOLStruct->DEVID >> 8);
	outputbytes[XM_DEVID_BYTE_POS + 1] = (uint8_t)(PROTOCOLStruct->DEVID);
	outputbytes[XM_CMDID_BYTE_POS + 0] = (uint8_t)(PROTOCOLStruct->CMDID >> 8);
	outputbytes[XM_CMDID_BYTE_POS + 1] = (uint8_t)(PROTOCOLStruct->CMDID);


	for(i = 0 ; i < PROTOCOLStruct->length; i ++)
	{
		//debug_printf("0x%x\t",protocol->protcmsg.data[i]);
		outputbytes[XM_DATAS_BYTE_POS + i]	= PROTOCOLStruct->data[i];
	}

	outputbytes[XM_PARITY_BYTE_POS(PROTOCOLStruct->length) + 0] = (uint8_t)(PROTOCOLStruct->parity >> 8);
	outputbytes[XM_PARITY_BYTE_POS(PROTOCOLStruct->length) + 1] = (uint8_t)(PROTOCOLStruct->parity);
	outputbytes[XM_END_BYTE_POS(PROTOCOLStruct->length)] = PROTOCOLStruct->endByte;
}



void XM_PTCINITstruct(XM_PROTOCOLStruct_t *PROTOCOLStruct,user_t *user,uint8_t *data,uint32_t len)
{
	PROTOCOLStruct->user		= user;
	PROTOCOLStruct->startByte   = data[0];
	PROTOCOLStruct->DEVID		= (uint16_t)data[1] << 8 | data[2];
	PROTOCOLStruct->CMDID		= (uint16_t)data[3] << 8 | data[4];
	PROTOCOLStruct->timestamp[0] = data[5];
	PROTOCOLStruct->timestamp[1] = data[6];
	PROTOCOLStruct->length 		= len - 10;
	PROTOCOLStruct->data		= data + 7;
	PROTOCOLStruct->parity		= data[len - 3] << 8 | data[len - 2];
	PROTOCOLStruct->endByte		= data[len - 1];
}

static int XM_CheckDevStatus(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t Trouble_sofWare 		= 1;
	uint8_t Trouble_hardWare 		= 1;
	uint8_t Trouble_showModule 		= 1;
	uint8_t Trouble_showModulePower = 1;
	uint8_t Trouble_checkSystem 	= 1;
	uint8_t Trouble_inputPower 		= 1;
	uint8_t Trouble_temperature 	= 1;
	uint8_t Trouble_communicate 	= 1;

	uint8_t Status_HByte;
	uint8_t Status_LByte;

	uint16_t DetailStatus = 0x00;
	uint8_t status,vals;
	
	//通讯故障
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 10) : (DetailStatus);
	debug_printf("status = 0x%x,DetailStatus = 0x%x\n",status,DetailStatus);

	//温度故障
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 9) : (DetailStatus);
	debug_printf("status = 0x%x,DetailStatus = 0x%x\n",status,DetailStatus);

	//输入电源故障
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 6) : (DetailStatus);
	debug_printf("status = 0x%x,DetailStatus = 0x%x\n",status,DetailStatus);

	//检测系统能够故障
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 5) : (DetailStatus);
	debug_printf("status = 0x%x,DetailStatus = 0x%x\n",status,DetailStatus);

	//显示模块电源故障
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 3) : (DetailStatus);
	debug_printf("status = 0x%x,DetailStatus = 0x%x\n",status,DetailStatus);


	//显示模块故障
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 2) : (DetailStatus);
	debug_printf("status = 0x%x,DetailStatus = 0x%x\n",status,DetailStatus);

	//硬件故障
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 1) : (DetailStatus);
	debug_printf("status = 0x%x,DetailStatus = 0x%x\n",status,DetailStatus);

	//控制软件故障
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 0) : (DetailStatus);
	debug_printf("status = 0x%x,DetailStatus = 0x%x\n",status,DetailStatus);
	
	Status_HByte = (uint8_t)((DetailStatus & 0xff00) >> 8);
	Status_LByte = (uint8_t)((DetailStatus & 0x00ff) >> 0);
	
	PROTOCOLStruct->data[1] = Status_HByte;
	PROTOCOLStruct->data[0] = Status_LByte;
	debug_printf("PROTOCOLStruct->data[0] = 0x%x,PROTOCOLStruct->data[1] = 0x%x\n",PROTOCOLStruct->data[0],PROTOCOLStruct->data[1]);
	PROTOCOLStruct->length  = 2;
	
	return 0;
}


//预置播放列表
static int XM_PlayPreSetLst(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	debug_printf("preset play list\n");
	char playlist[8];
	char wrlist[10];
	char listname[64];
	uint8_t Len;
	
	memset(playlist,0,sizeof(playlist));
	memcpy(playlist,PROTOCOLStruct->data,3);
	debug_printf("playlist = %s\n",playlist);

	//PPL_SETCurDspLst(playlist,strlen(playlist));

	Len = strlen(playlist);
	//DP_CurPlayList(OPS_MODE_SET,playlist,&Len);
	DP_SetCurPlayList(playlist,Len);

	memset(wrlist,0,sizeof(wrlist));
	sprintf(wrlist,"%s.lst",playlist);
	wrlist[7] = '\0';
	conf_file_write(ConFigFile,"playlist","list",wrlist);
	
	
	sprintf(listname,"%s/%s.lst",list_dir_1,playlist);
	listname[strlen(listname)] = '\0';
	
	if(access(listname,F_OK) < 0)
	{
		debug_printf("The list [%s.lst]file is not exist!\n",playlist);
		goto EXCEPTION;
	}

	if(COM_CopyFile(listname,XMLIST) < 0)  
		goto EXCEPTION;

	if(XM_PLst_parsing(&content,listname) != 0)
		goto EXCEPTION;
	PROTOCOLStruct->data[0] = 0x30;//0表示成功
	PROTOCOLStruct->length = 1;
	DEBUG_PRINTF;
	return 0;

	EXCEPTION:
		PROTOCOLStruct->data[0]	 = 0x31;//非0表示失败
		PROTOCOLStruct->length	 = 1;
		return -1;
} 


static int XM_GetCurPlayLst(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	Curplay_t Curplay;
	memset(&Curplay,0,sizeof(Curplay_t));
	XM_GetCurPlaying(&Curplay);
	
	PROTOCOLStruct->data[0] = Curplay.order /100 + 0x30;
	PROTOCOLStruct->data[1] = Curplay.order /100 / 10 + 0x30;
	PROTOCOLStruct->data[2] = Curplay.order % 10 + 0x30;
	
	PROTOCOLStruct->data[3] = Curplay.stoptime / 10000 		+ 0x30;
	PROTOCOLStruct->data[4] = Curplay.stoptime / 1000 % 10  + 0x30;
	PROTOCOLStruct->data[5] = Curplay.stoptime / 100 % 10  	+ 0x30;
	PROTOCOLStruct->data[6] = Curplay.stoptime / 10 % 10  	+ 0x30;
	PROTOCOLStruct->data[7] = Curplay.stoptime % 10  		+ 0x30;

	PROTOCOLStruct->data[8] = Curplay.effectin / 10  			+ 0x30;
	PROTOCOLStruct->data[9] = Curplay.effectin % 10  			+ 0x30;

	PROTOCOLStruct->data[10] = Curplay.inspeed / 10000 		+ 0x30;
	PROTOCOLStruct->data[11] = Curplay.inspeed / 1000 % 10  + 0x30;
	PROTOCOLStruct->data[12] = Curplay.inspeed / 100 % 10  	+ 0x30;
	PROTOCOLStruct->data[13] = Curplay.inspeed / 10 % 10  	+ 0x30;
	PROTOCOLStruct->data[14] = Curplay.inspeed % 10  		+ 0x30;

	memcpy(PROTOCOLStruct->data + 15,Curplay.playstr,Curplay.strLen);
	PROTOCOLStruct->length   = 15 + Curplay.strLen;	
	return 0;
}


static int XM_RxFileFrmUpper(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	DEBUG_PRINTF;
	int ret = -1;
	char *Flag0x2B = NULL;
	char *dataPoint = PROTOCOLStruct->data;
	char *filedata = NULL;
	uint32_t filedataLen = 0;
	char filename[24];
	uint8_t filenameLen = 0;
	uint32_t frameOffset = 0;

	FILEUser_t FileUser;

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

	//文件偏移
	dataPoint = Flag0x2B + 1;
	frameOffset = (uint8_t)dataPoint[0] << 24 | (uint8_t)dataPoint[1] << 16 | (uint8_t)dataPoint[2] << 8 | (uint8_t)dataPoint[3];
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",dataPoint[0],dataPoint[1],dataPoint[2],dataPoint[3]);
	debug_printf("frameOffset = %d\n",frameOffset);

	//文件数据
	dataPoint += 4;
	filedata = dataPoint;
	filedataLen = PROTOCOLStruct->length - filenameLen - 1 - 4;
	debug_printf("filedataLen = %d\n",filedataLen);
		
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

	debug_printf("*filePath = %s\n",filePath);
	

	//文件帧数据处理
	debug_printf("*PROTOCOLStruct->user->ip = %s\n",PROTOCOLStruct->user->ip);
	memset(&FileUser,0,sizeof(FILEUser_t));
	FRTx_FileUserInit(&FileUser,PROTOCOLStruct->user->type,PROTOCOLStruct->user->ip,PROTOCOLStruct->user->port,PROTOCOLStruct->user->uartPort);
	if((ret = FRTx_FileFrameRx(&FileUser,filePath,filedata,filedataLen,frameOffset)) < 0)
		goto EXCEPTION;

	//如果是配置文件则复制到几个地方
	if(ret == 1)
	{
		chmod(filePath,0744);
		//名为play.lst的文件名为插播播放列表
		if(strncmp(filename,"play.lst",8) == 0)
		{
			debug_printf("parsing play.lst\n");
			if(XM_PLst_parsing(&content,filePath) < 0)
				goto EXCEPTION;
		}
	}
	

	PROTOCOLStruct->data[0] = 0x30; 	//0表示成功
	PROTOCOLStruct->length	= 1;
	return 0;

	EXCEPTION:
		debug_printf("a failed frame!\n");
		PROTOCOLStruct->data[0] = 0x31;	//非0表示失败
		PROTOCOLStruct->length  = 1;
		return -1;
		
}


static int XM_TxFileToUpper(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	
	char filename[24];
	char filePath[64];
	uint32_t frameOffset = 0;
	FILEUser_t FileUser;
	char *dataPoint = PROTOCOLStruct->data;
	char *OffsetPos = NULL;
	char *Flag0x2B = NULL;
	
	//取文件名并确定文件所属路径
	memset(filename,0,sizeof(filename));
	memset(filePath,0,sizeof(filePath));
	debug_printf("dataPoint = %s\n",dataPoint);

	if(strncmp(dataPoint + 4,"lst",3) == 0)
	{
		memcpy(filename,dataPoint,7); 
		debug_printf("filename = %s\n",filename);
		filename[7] = '\0';
		
		sprintf(filePath,"%s/%s",list_dir_1,filename);
		dataPoint += 7;
	}
	else if((strncmp(dataPoint + 4,"bmp",3) == 0) || (strncmp(dataPoint + 4,"BMP",3) == 0))
	{
		memcpy(filename,dataPoint,7);
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
			dataPoint += 8;
		}
		sprintf(filePath,"%s/%s",font_dir,filename);
	}
	
	debug_printf("filename = %s,filePath = %s\n",filename,filePath);

	//检测文件是否存在
	if(access(filePath,F_OK) < 0)
	{
		PROTOCOLStruct->data[0] = 0x32;//文件已被删除
		PROTOCOLStruct->length  = 1;
		return -1;
	}

	//帧偏移
	//Flag0x2B = strchr(dataPoint,0x2b);
	//if(Flag0x2B == NULL)
	//	goto EXCEPTION;

	OffsetPos = dataPoint;
	frameOffset = OffsetPos[0] << 24 | OffsetPos[1] << 16 | OffsetPos[2] << 8 | OffsetPos[3];
	debug_printf("frameOffset = %d,OffsetPos[0] = %d\n",frameOffset,OffsetPos[0]);
	debug_printf("PROTOCOLStruct->user->ip = %s,strlen(PROTOCOLStruct->user->ip) = %d\n",PROTOCOLStruct->user->ip,strlen(PROTOCOLStruct->user->ip));
	
	//文件数据处理
	char *frameData = PROTOCOLStruct->data + 1;
	uint32_t frameDataLen = 0;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,PROTOCOLStruct->user->type,PROTOCOLStruct->user->ip,PROTOCOLStruct->user->port,PROTOCOLStruct->user->uartPort);
	if(FRTx_FileFrameTx(&FileUser,filePath,frameData,&frameDataLen,frameOffset) < 0)
		goto EXCEPTION;

	PROTOCOLStruct->data[0] = 0x30; 	//0表示成功
	PROTOCOLStruct->length = frameDataLen + 1;
	debug_printf("frameDataLen = %d\n",frameDataLen);

	return 0;

	EXCEPTION:
		PROTOCOLStruct->data[0] = 0x31;	//非0表示失败
		PROTOCOLStruct->length  = 1;
		return -1;
}



static int XM_TxDirToUpper(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	return 0;
}

static int XM_DelFile(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t filename[24];
	memset(filename,0,sizeof(filename));
	memcpy(filename,PROTOCOLStruct->data,PROTOCOLStruct->length);
	filename[PROTOCOLStruct->length] = '\0';

	dir_wintolinux(filename);
	Dir_LetterBtoL(filename);
	debug_printf("filename = %s\n",filename);
	
	//文件存放路径
	char filePath[64];
	memset(filePath,0,sizeof(filePath));
	if(strstr(filename,".lst") != NULL)
		sprintf(filePath,"%s/%s",list_dir_1,filename);
	else if(strstr(filename,".bmp") != NULL)
		sprintf(filePath,"%s/%s",image_dir,filename);
	else
		sprintf(filePath,"%s/%s",sys_dir,filename);

	debug_printf("remove pwd : %s\n",filePath);
	remove(filePath);

	PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	
	return 0;
}



static int PPL_SetModeOfBright(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t brightMode = 0;
	uint8_t Bright;
	
	brightMode = PROTOCOLStruct->data[0];
	debug_printf("brightMode = 0x%x\n",brightMode);
	if(brightMode != BRIGHT_AUTO && brightMode != BRIGHT_HAND)
		brightMode = BRIGHT_AUTO;
	debug_printf("brightMode = 0x%x\n",brightMode);
	DEBUG_PRINTF;
	//PPL_SETBrightMode(brightMode);
	//DP_BrightAndMode(OPS_MODE_SET,&brightMode,NULL,NULL,NULL);
	DP_SetBrightMode(brightMode);
	
	//下面是真正设置设备的亮度调节方式
	if(brightMode == BRIGHT_HAND)
		conf_file_write(ConFigFile,"brightmode","mode","31");

	if(brightMode == BRIGHT_AUTO)
		conf_file_write(ConFigFile,"brightmode","mode","30");
		
	PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	return 0;
}

static int XM_SetDevBright(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t Bmode,Bright,Bmax,Bmin,Brank = 0;
	uint8_t RGB_R,RGB_G,RGB_B;
	float divbright = 0.0,sbright = 0.0;
	char brightVals[4];

	//取亮度模式与亮度值
	Bmode = PROTOCOLStruct->data[0];
	Bright = (PROTOCOLStruct->data[1] - 0x30) * 10 + (PROTOCOLStruct->data[2] - 0x30);

	Bmode = (Bmode == XM_BRIGHT_AUTO) ? BRIGHT_AUTO : BRIGHT_HAND;
	//将亮度模式记录到文件
	if(Bmode == BRIGHT_AUTO)
		conf_file_write(ConFigFile,"brightmode","mode","31");
	else
		conf_file_write(ConFigFile,"brightmode","mode","30");

	//保存亮度模式
	DP_SetBrightMode(Bmode);
	

	//保证亮度值在0-31范围内
	Bright = (Bright <= XM_BRIGHT_MIN) ? XM_BRIGHT_MIN : Bright;
	Bright = (Bright >= XM_BRIGHT_MAX) ? XM_BRIGHT_MAX : Bright;
	DP_SaveBrightVals(Bright);
	//针对扫描版的设定亮度值,使用原始的值，即0-31
	HW2G400_SETLEDbright(Bright);

	//先转换成1-64在保存亮度值
	DP_GetBrightRange(&Bmax,&Bmin);
	divbright = (Bmax - Bmin) / (float)XM_BRIGHT_RANK;
	sbright = divbright * Bright + Bmin;
	Bright = (sbright - (uint8_t)sbright > 0.5) ? ((uint8_t)sbright + 1) : ((uint8_t)sbright);
	//针对TXRX板设定亮度值，使用转换后的值，0-64
	Set_LEDBright(Bright);

	PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	return 0;

}


static int XM_AdjustTime(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	struct timeval timestamp;
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;
	
	timestamp.tv_sec = (uint8_t)PROTOCOLStruct->data[0] << 24 | (uint8_t)PROTOCOLStruct->data[1] << 16 | (uint8_t)PROTOCOLStruct->data[2] << 8 | (uint8_t)PROTOCOLStruct->data[3];

	//值为0表示查询时间
	if(timestamp.tv_sec == 0)
	{
		gettimeofday(&timestamp, NULL);
		PROTOCOLStruct->data[0] = (uint8_t)((timestamp.tv_sec & 0xff000000) >> 24);
		PROTOCOLStruct->data[1] = (uint8_t)((timestamp.tv_sec & 0x00ff0000) >> 16);
		PROTOCOLStruct->data[2] = (uint8_t)((timestamp.tv_sec & 0x0000ff00) >> 8);
		PROTOCOLStruct->data[3] = (uint8_t)((timestamp.tv_sec & 0x000000ff) >> 0);
	}
	//值非0表示校正时间
	else
	{
		settimeofday(&timestamp,NULL);
	}

	PROTOCOLStruct->length = 4;
	return 0;
}



static int XM_SetScreenStatus(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t status = 0;
	int operation = PROTOCOLStruct->data[0];
	
	//0x33表示操作失败，先初始化成失败的，如果下面的操作不成功自然是里面填的是失败的值
	PROTOCOLStruct->data[0] = 0x33;
	
	switch(operation)
	{
		//查询
		case XM_SCREEN_CHECK:
			DP_GetScreenStatus(&status);
			PROTOCOLStruct->data[0] = (status == SLED_ON) ? XM_SCREEN_ON : XM_SCREEN_OFF;
			break;
		//开屏
		case XM_SCREEN_ON:
			//针对TXRX板开屏
			SET_LED_STATE(SLED_ON);
			//针对扫描版开屏
			HW2G400_SetScreenStatus(SLED_ON);
			//记录大屏开关状态
			LEDstateRecord(SLED_ON);
			PROTOCOLStruct->data[0] = XM_SCREEN_ON;
			break;
		//关屏
		case XM_SCREEN_OFF://说明同上
			SET_LED_STATE(SLED_OFF);
			HW2G400_SetScreenStatus(SLED_OFF);
			LEDstateRecord(SLED_OFF);
			PROTOCOLStruct->data[0] = XM_SCREEN_OFF;
			break;
	}
	PROTOCOLStruct->length = 1;
	return 0;
}


static int XM_SetIP(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	char IP[16];
	char port[8];
	int netport = 0;

	//获取IP以及端口号数据
	memset(IP,0,sizeof(IP));
	memset(port,0,sizeof(port));
	sprintf(IP,"%d.%d.%d.%d",PROTOCOLStruct->data[0],PROTOCOLStruct->data[1],PROTOCOLStruct->data[2],PROTOCOLStruct->data[3]);
	netport = PROTOCOLStruct->data[4] << 8 | PROTOCOLStruct->data[5];
	sprintf(port,"%d",netport);

	debug_printf("IP = %s,port = %s\n",IP,port);
	//将ip以及端口号写入配置文件中
	conf_file_write(ConFigFile,"netport","ip",IP);
	conf_file_write(ConFigFile,"netport","port",port);
	
	ConfigFileOps();

	PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	return 0;
}


static int XM_Reset(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	int ret = -1;
	PROTOCOLStruct->data[0] = 0x31;

	//关屏
	SET_LED_STATE(SLED_OFF);
	HW2G400_SetScreenStatus(SLED_OFF);
	//重启
	
	ret = open(sys_dir"/sys/sys_reboot.lock",O_WRONLY | O_CREAT,0744);
	if(ret < 0)
	{
		PROTOCOLStruct->data[0] = 0x31;
		perror("set_devReset open");
	}
	else
		PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	return 0;
}


//获取屏幕亮度模式与相应的亮度值
static int PPL_GetDevBright(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t Bmax,Bmin;
	uint8_t brightMode = 0;
	uint8_t Bright = 0;
	float divbright = 0.0,sbright = 0.0;

	//获取亮度模式
	DP_GetBrightMode(&brightMode);
	brightMode = (brightMode == BRIGHT_AUTO) ? XM_BRIGHT_AUTO : XM_BRIGHT_HAND;
	PROTOCOLStruct->data[0] = brightMode;

	//亮度值，亮度值要先转换成0-31的范围
	DP_ReadBrightVals(&Bright);
	DP_GetBrightRange(&Bmax,&Bmin);
	divbright = (Bmax - Bmin) / (float)XM_BRIGHT_RANK;
	sbright = Bright / divbright;
	Bright = (sbright - (uint8_t)sbright > 0.5) ? ((uint8_t)sbright + 1) : ((uint8_t)sbright);
	debug_printf("brightMode = 0x%x,sbright = %f\n",brightMode,sbright);

	PROTOCOLStruct->data[1] = Bright / 10 + 0x30;
	PROTOCOLStruct->data[2] = Bright % 10 + 0x30;

	PROTOCOLStruct->length = 3;
	return 0;
}


static int XM_GetRunStatus(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t LSdataA,LSdataB;
	DP_GetLSData(&LSdataA,&LSdataB);

	//环境亮度
	PROTOCOLStruct->data[0] = 0x00;
	PROTOCOLStruct->data[1] = LSdataA;
	PROTOCOLStruct->data[2] = 0x00;
	PROTOCOLStruct->data[3] = LSdataB;

	//温度，哪里的温度啊~_~随便填个值
	PROTOCOLStruct->data[4] = 0x14;
	PROTOCOLStruct->data[5] = 0x15;

	//检测到的单点检测点数，这是什么鬼????填0吧~ _~
	PROTOCOLStruct->data[6] = 0x00;
	PROTOCOLStruct->data[7] = 0x00;
	PROTOCOLStruct->data[8] = 0x00;
	PROTOCOLStruct->data[9] = 0x00;

	//配电柜开关，强制填1
	PROTOCOLStruct->data[10] = 0x01;
	
	//门磁开关，强制填1
	PROTOCOLStruct->data[11] = 0x01;

	PROTOCOLStruct->length = 12;
	return 0;
}


static int XM_GetScrTrouble(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	
	uint16_t DetailStatus = 0x00;
	uint8_t LSdataA,LSdataB;
	DP_GetLSData(&LSdataA,&LSdataB);
	if(LSdataA < 0 || LSdataA > 64 || LSdataB < 0 || LSdataB > 64)
	DetailStatus = (1 << 8);

	uint8_t Byte0,Byte1,Byte2,Byte3;
	Byte0 = (uint8_t)((DetailStatus & 0xf000) >> 12);
	Byte1 = (uint8_t)((DetailStatus & 0x0f00) >> 8);
	Byte2 = (uint8_t)((DetailStatus & 0x00f0) >> 4);
	Byte3 = (uint8_t)((DetailStatus & 0x000f) >> 0);

	PROTOCOLStruct->data[0] = (Byte0 < 58) ? (Byte0 + 0x30) : (Byte0 + 0x37);
	PROTOCOLStruct->data[1] = (Byte1 < 58) ? (Byte1 + 0x30) : (Byte1 + 0x37);
	PROTOCOLStruct->data[2] = (Byte2 < 58) ? (Byte2 + 0x30) : (Byte2 + 0x37);
	PROTOCOLStruct->data[3] = (Byte3 < 58) ? (Byte3 + 0x30) : (Byte3 + 0x37);
	PROTOCOLStruct->length = 4;
	return 0;
}


//查询屏幕故障点检测
static int XM_GetPixsStatus(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint32_t Swidth,Sheight;
	DP_GetScreenSize(&Swidth,&Sheight);

	//屏幕宽高
	PROTOCOLStruct->data[0] = (uint8_t)((Sheight & 0x0000ff00) >> 8);
	PROTOCOLStruct->data[1] = (uint8_t)((Sheight & 0x000000ff) >> 0);
	PROTOCOLStruct->data[2] = (uint8_t)((Swidth & 0x0000ff00) >> 8);
	PROTOCOLStruct->data[3] = (uint8_t)((Swidth & 0x000000ff) >> 0);

	//模组宽高
	PROTOCOLStruct->data[4] = 0x08;
	PROTOCOLStruct->data[5] = 0x10;

	//屏的控制通道数
	PROTOCOLStruct->data[6] = Swidth / 16;

	//屏的故障检测类型数,不懂什么鬼
	PROTOCOLStruct->data[7] = 0x00;

	//通道分量，什么鬼???
	PROTOCOLStruct->data[8] = 0xff;
	PROTOCOLStruct->data[9] = 0xff;
	PROTOCOLStruct->data[10] = 0xff;

	//故障类型
	PROTOCOLStruct->data[11] = 0x00;

	//所有像素状态
	int Len = Swidth * Sheight / 8;
	memset(PROTOCOLStruct->data + 12,0x00,Len);

	PROTOCOLStruct->length = Len + 12;
	return 0;
}


//查询屏幕硬件参数
static int XM_GetScrSize(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint32_t Swidth,Sheight;
	DP_GetScreenSize(&Swidth,&Sheight);
	debug_printf("Swidth = %d,Sheight = %d\n",Swidth,Sheight);
	PROTOCOLStruct->data[0] = Sheight / 100 + 0x30;
	PROTOCOLStruct->data[1] = Sheight / 10 % 10 + 0x30;
	PROTOCOLStruct->data[2] = Sheight % 10 + 0x30;

	PROTOCOLStruct->data[3] = Swidth / 100 + 0x30;
	PROTOCOLStruct->data[4] = Swidth / 10 % 10 + 0x30;
	PROTOCOLStruct->data[5] = Swidth % 10 + 0x30;

	PROTOCOLStruct->length = 6;
	return 0;
}



static int PPL_SetDevIP(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	unsigned char dev_ip[24] , dev_mask[24] , dev_gw[24] , dev_port[8];
	unsigned char *ip = NULL,*mask = NULL,*gw = NULL,*port = NULL;
	unsigned short _port = 0;

	char conf_file[64];
	memset(conf_file,0x00,sizeof(conf_file));
	sprintf(conf_file,"%s/cls.conf",conf_dir);
	
	ip 	 = PROTOCOLStruct->data + 6;
	mask = PROTOCOLStruct->data + 10;
	gw   = PROTOCOLStruct->data + 14;
	port = PROTOCOLStruct->data + 24;

	_port = port[0] << 8 | port[1] << 0;
	debug_printf("_port = %d\n",_port);
	if(_port == 0)
		_port = 5168;
	memset(dev_ip,0,sizeof(dev_ip));
	memset(dev_mask,0,sizeof(dev_mask));
	memset(dev_gw,0,sizeof(dev_gw));
	memset(dev_port,0,sizeof(dev_port));

	debug_printf("ip[0] = %d\n",ip[0]);
	sprintf(dev_ip,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
	sprintf(dev_mask,"%d.%d.%d.%d",mask[0],mask[1],mask[2],mask[3]);
	sprintf(dev_gw,"%d.%d.%d.%d",gw[0],gw[1],gw[2],gw[3]);
	sprintf(dev_port,"%d",_port);

	debug_printf("dev_ip = %s,dev_mask = %s,dev_gw = %s\n",dev_ip,dev_mask,dev_gw);

	conf_file_write(conf_file,"netport","ip",dev_ip);
	conf_file_write(conf_file,"netport","netmask",dev_mask);
	conf_file_write(conf_file,"netport","gateway",dev_gw);
	conf_file_write(conf_file,"netport","port",dev_port);
	
	set_devip(dev_ip,dev_mask,dev_gw);
	

	PROTOCOLStruct->data[2] = 0x31;
	PROTOCOLStruct->length  = 3;

	char ip_port[24];
	char logmsg[96];
	memset(ip_port,0,sizeof(ip_port));
	sprintf(ip_port,"%s:%d",PROTOCOLStruct->user->ip,PROTOCOLStruct->user->port);
	sprintf(logmsg,"cmd:%x setnet:ip: %s netmask:%s gateway:%s port:%s",
		PROTOCOLStruct->CMDID,dev_ip,dev_mask,dev_gw,dev_port);
	debug_printf("strlen(logmsg) = %d\n",strlen(logmsg));
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	return 0;
}

static int PPL_SetDevReset(XM_PROTOCOLStruct_t *PROTOCOLStruct)
{
	/*设置设备重新启动*/
	char systemstr[48];
	char cur_workdir[128];
	int ret = 0;
	memset(cur_workdir,0x00,sizeof(cur_workdir));
	//getcwd(cur_workdir,sizeof(cur_workdir));
	sprintf(cur_workdir,"%s",sys_dir);
	chdir(cur_workdir);

	sprintf(systemstr,"sh %s","sh.sh");
	system(systemstr);

	PROTOCOLStruct->data[0] = 0x01;
	PROTOCOLStruct->length = 1;
	
	//wdt_feed(WDT_FEED_STOP);
	//wdt_stop();
	DEBUG_PRINTF;
	ret = open(sys_dir"/sys/sys_reboot.lock",O_WRONLY | O_CREAT,0744);
	if(ret < 0)
		perror("set_devReset open");

	char ip_port[24];
	char logmsg[96];
	//sprintf(ip_port,"%s:%d",PROTOCOLStruct->user->ip,PROTOCOLStruct->user->port);
	//sprintf(logmsg,"cmd:%x reset the device",PROTOCOLStruct->protcmsg.head.cmdID);
	//_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	
	return 0;
}



int XM_protocolProcessor(user_t *user,uint8_t *PTCdata,uint32_t *PTClenth)
{
	XM_PROTOCOLStruct_t PROTOCOLStruct;
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
	/*************************************************************************************/
//每个协议都可以加这条
	uint8_t vindicate[9] = {0x02,0x39,0x30,0x30,0x30,0x30,0x7E,0x18,0x03};
	uint8_t reply[9] = {0x02,0x39,0x30,0x30,0x30,0x01,0x58,0x6A,0x03};
	if(memcmp(PTCdata,vindicate,9)==0)
	{

		//回复上位机 
		if(user->type == 0) //网口
			send(user->fd,reply,9,0);
		else if(user->type == 1)
			uart_send(xCOM1,reply,9);

		
		char buf_frist[16];
		char buf_second[16];
		char content[256];

		memset(buf_frist,0,sizeof(buf_frist));
		memset(buf_second,0,sizeof(buf_second));
		memset(content,0,sizeof(content));

		conf_file_read(CurrentPtcFile,"protocol","protocol",buf_frist);
		conf_file_read(CurrentPtcFile,"protocol","swr_protocol",buf_second);

		debug_printf("buf_frist is %s  buf_second is %s\n",buf_frist,buf_second);
		sprintf(content,"[protocol]\nprotocol = %s\nswr_protocol = %s\n",buf_frist,buf_second);
		debug_printf("%s\n",content);
		//保存升级之前的协议，用于升级完成后，切换回原来的协议
		FILE *IPF = fopen(RecordPtcFile,"wb+");
		if(IPF == NULL)
			return -1;
		fwrite(content,1,sizeof(content),IPF);
		fflush(IPF);
		fclose(IPF);
		
		conf_file_write(CurrentPtcFile,"protocol","protocol","upgrade");
		conf_file_write(CurrentPtcFile,"protocol","swr_protcol","general");
		system("killall ledscreen");
	}	

/*******************************************************************************************/
	err = prtcl_preparsing(PTCdata,*PTClenth,FREEdata,&outlen);
	if(err != 0)
	{
		debug_printf("find error when preparsing the recv data\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"prtcl_preparsing failed",strlen("prtcl_preparsing failed"));
		goto ERRORDEAL;
	}
	debug_printf("2*PTClenth = %d\n",*PTClenth);
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
	debug_printf("3*PTClenth = %d\n",*PTClenth);
 	int ret = 0; 
	int k = 0;
	XM_PTCINITstruct(&PROTOCOLStruct,user,FREEdata,outlen);

	debug_printf("----------------------recv data-----------------------\n");
	prtclmsg_printf(&PROTOCOLStruct);
	//下面的可忽略不计
	debug_printf("PROTOCOLStruct.CMDID = 0x%x\n",PROTOCOLStruct.CMDID);
	switch(PROTOCOLStruct.CMDID)
	{
		//设置亮度模式与亮度值
		case XM_SET_BRIGHT:
			XM_SetDevBright(&PROTOCOLStruct);
			break;

		//查询或者获取时间，按秒计算，从1970-1-1 00:00:00开始计算
		case XM_ADJUST_TIME:
			XM_AdjustTime(&PROTOCOLStruct);
			break;

		//设置开关屏
		case XM_SET_SCREEN:
			XM_SetScreenStatus(&PROTOCOLStruct);
			break;
			
		//设置IP与端口号
		case XM_SET_IP:
			XM_SetIP(&PROTOCOLStruct);
			break;

		//重启屏幕
		case XM_RESET:
			XM_Reset(&PROTOCOLStruct);
			break;

		//下载文件---发送给上位机
		case XM_FILE_TX:
			XM_TxFileToUpper(&PROTOCOLStruct);
			break;

		//上传文件---上位机发送过来
		case XM_FILE_RX:
			XM_RxFileFrmUpper(&PROTOCOLStruct);
			break;

		//删除文件
		case XM_FILE_DELETE:
			XM_DelFile(&PROTOCOLStruct);
			break;

		//当前显示内容
		case XM_CUR_DSP:
			XM_GetCurPlayLst(&PROTOCOLStruct);
			break;

		//显示播放列表
		case XM_SET_DSP:
			XM_PlayPreSetLst(&PROTOCOLStruct);
			break;

		//获取当前亮度模式与亮度值
		case XM_GET_BRIGHT:
			PPL_GetDevBright(&PROTOCOLStruct);
			break;

		//查询屏幕运行状态
		case XM_GET_RUNSTATE:
			XM_GetRunStatus(&PROTOCOLStruct);
			break;

		//查询屏幕故障
		case XM_CHECK_TRONBLE:
			XM_GetScrTrouble(&PROTOCOLStruct);
			break;

		//查询屏幕故障点检测
		case XM_PIX_TROUBLE:
			XM_GetPixsStatus(&PROTOCOLStruct);
			break;

		//查询屏幕硬件信息
		case XM_GET_ARG:
			XM_GetScrSize(&PROTOCOLStruct);
			
			break;

		default:
			break;
	}
	debug_printf("PROTOCOLStruct.length = %d\n",PROTOCOLStruct.length);
	debug_printf("----------------------send data-----------------------\n");
	//prtclmsg_printf(&PROTOCOLStruct);
	
	struct_to_bytes(&PROTOCOLStruct,FREEdata);
	//校验值
	parity = XKCalculateCRC(FREEdata+1,6+PROTOCOLStruct.length);
	//debug_printf("PROTOCOLStruct.parity = 0x%x\n",PROTOCOLStruct.parity);
	FREEdata[XM_PARITY_BYTE_POS(PROTOCOLStruct.length) + 0] = (uint8_t)(parity >> 8);
	FREEdata[XM_PARITY_BYTE_POS(PROTOCOLStruct.length) + 1] = (uint8_t)(parity);
	//检测字节序中是否存在0x02、0x03、0x1B，有则进行相应的转换，此处处理办法是将头尾两个字节踢掉在处理
	DEBUG_PRINTF;
	check_0x02and0x03(FLAG_SEND,FREEdata+1,6+PROTOCOLStruct.length+2,PTCdata+1,PTClenth);
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





