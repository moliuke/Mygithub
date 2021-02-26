
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
#include "JX_protocol.h"
#include "file_trmit.h"
#include "wdt.h"
#include "conf.h"

#include "../threadpool.h"
#include "../PTC_common.h"
#include "../PTC_FileRxTx.h"
#include "../../Hardware/Data_pool.h"
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


static void prtclmsg_printf(JX_PROTOCOLStruct_t *PROTOCOLStruct)
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

static JXError_t struct_to_bytes(JX_PROTOCOLStruct_t *PROTOCOLStruct,uint8_t *outputbytes)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	if(PROTOCOLStruct == NULL || outputbytes == NULL)
		return JX_ERR_ARGUMENT;
	
	outputbytes[JX_START_BYTE_POS] = PROTOCOLStruct->startByte;
	outputbytes[JX_DEVID_BYTE_POS + 0] = (uint8_t)(PROTOCOLStruct->DEVID >> 8);
	outputbytes[JX_DEVID_BYTE_POS + 1] = (uint8_t)(PROTOCOLStruct->DEVID);
	for(i = 0 ; i < PROTOCOLStruct->length; i ++)
	{
		//debug_printf("0x%x ",ROTOCOLStruct->data[i]);
		outputbytes[JX_DATAS_BYTE_POS + i]	= PROTOCOLStruct->data[i];
	}

	outputbytes[JX_PARITY_BYTE_POS(PROTOCOLStruct->length) + 0] = (uint8_t)(PROTOCOLStruct->parity >> 8);
	outputbytes[JX_PARITY_BYTE_POS(PROTOCOLStruct->length) + 1] = (uint8_t)(PROTOCOLStruct->parity);
	outputbytes[JX_END_BYTE_POS(PROTOCOLStruct->length)] = PROTOCOLStruct->endByte;
}



void JX_PTCINITstruct(JX_PROTOCOLStruct_t *PROTOCOLStruct,user_t *user,uint8_t *data,uint32_t len)
{
	PROTOCOLStruct->user		= user;
	PROTOCOLStruct->startByte   = data[0];
	PROTOCOLStruct->DEVID		= (uint16_t)data[1] << 8 | data[2];
	PROTOCOLStruct->CMDID		= (uint16_t)data[3] << 8 | data[4];
	PROTOCOLStruct->length 		= len - 8;
	PROTOCOLStruct->data		= data + 5;
	PROTOCOLStruct->parity		= data[len - 3] << 8 | data[len - 2];
	PROTOCOLStruct->endByte		= data[len - 1];
}

static int JX_CheckDevStatus(JX_PROTOCOLStruct_t *PROTOCOLStruct)
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
static int JX_PlayPreSetLst(JX_PROTOCOLStruct_t *PROTOCOLStruct)
{
	debug_printf("preset play list\n");
	char playlist[8];
	//char wrlist[10];
	char listname[64];
	uint8_t Len;
	
	memset(playlist,0,sizeof(playlist));
	memcpy(playlist,PROTOCOLStruct->data,3);
	debug_printf("playlist = %s\n",playlist);

	Len = strlen(playlist);
	DP_SetCurPlayList(playlist,Len);

	sprintf(listname,"%s/%s.lst",list_dir_1,playlist);
	listname[strlen(listname)] = '\0';
	
	if(access(listname,F_OK) < 0)
	{
		debug_printf("The list [%s.lst]file is not exist!\n",playlist);
		goto EXCEPTION;
	}

	if(COM_CopyFile(listname,JXLIST) < 0)  
		goto EXCEPTION;

	if(JX_PLst_parsing(&content,listname) != 0)
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


static int JX_GetCurPlayLst(JX_PROTOCOLStruct_t *PROTOCOLStruct)
{
	Curplay_t Curplay;
	memset(&Curplay,0,sizeof(Curplay_t));
	JX_GetCurPlaying(&Curplay);
	
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


static int JX_RxFileFrmUpper(JX_PROTOCOLStruct_t *PROTOCOLStruct)
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
	if(ret == 1)
	{
		chmod(filePath,0744);
		//名为play.lst的文件名为插播播放列表
		if(strncmp(filename,"play.lst",8) == 0)
		{
			debug_printf("parsing play.lst\n");
			if(JX_PLst_parsing(&content,filePath) < 0)
				goto EXCEPTION;
		}
	}
	

	PROTOCOLStruct->data[0] = 0x30; 	//0表示成功
	PROTOCOLStruct->length	= 1;
	return 0;

	EXCEPTION:
		//printf("a failed frame!\n");
		PROTOCOLStruct->data[0] = 0x31;	//非0表示失败
		PROTOCOLStruct->length  = 1;
		return -1;
		
}


static int JX_TxFileToUpper(JX_PROTOCOLStruct_t *PROTOCOLStruct)
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

	//帧偏移
	//Flag0x2B = strchr(dataPoint,0x2b);
	//if(Flag0x2B == NULL)
	//	goto EXCEPTION;
	//dataPoint += 1;
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
		PROTOCOLStruct->data[0] = 0x31;	//非0表示失败
		PROTOCOLStruct->length  = 1;
		return -1;
}



static int JX_TxDirToUpper(JX_PROTOCOLStruct_t *PROTOCOLStruct)
{
	return 0;
}

static int JX_DelFile(JX_PROTOCOLStruct_t *PROTOCOLStruct)
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

static int JX_SetDevBright(JX_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t Bmode,Bright,Bmax,Bmin,Brank = 0;
	uint8_t RGB_R,RGB_G,RGB_B;
	float divbright = 0.0,sbright = 0.0;
	char brightVals[2];
	

	//取亮度模式与亮度值
	Bmode = PROTOCOLStruct->data[0];
	Bright = (PROTOCOLStruct->data[1] - 0x30) * 10 + (PROTOCOLStruct->data[2] - 0x30);
	memcpy(brightVals,PROTOCOLStruct->data+1,2);
	Bmode = (Bmode == JX_BRIGHT_AUTO) ? BRIGHT_AUTO : BRIGHT_HAND;
	//将亮度模式记录到文件
	if(Bmode == BRIGHT_AUTO)
		conf_file_write(ConFigFile,"brightmode","mode","31");
	else
		conf_file_write(ConFigFile,"brightmode","mode","30");

	//保存亮度模式
	DP_SetBrightMode(Bmode);
	

	//保证亮度值在0-31范围内
	Bright = (Bright <= JX_BRIGHT_MIN) ? JX_BRIGHT_MIN : Bright;
	Bright = (Bright >= JX_BRIGHT_MAX) ? JX_BRIGHT_MAX : Bright;
	//DP_SaveBrightVals(Bright);
	
	//针对扫描版的设定亮度值,使用原始的值，即0-31
	//HW2G400_SETLEDbright(Bright);

	//先转换成1-64在保存亮度值
	DP_GetBrightRange(&Bmax,&Bmin);
	divbright = (Bmax - Bmin) / (float)JX_BRIGHT_RANK;
	sbright = divbright * Bright + Bmin;
	Bright = (sbright - (uint8_t)sbright > 0.5) ? ((uint8_t)sbright + 1) : ((uint8_t)sbright);
	//针对TXRX板设定亮度值，使用转换后的值，0-64
	if(Bmode == BRIGHT_HAND)
	{
		//printf("Bright is %d\n",Bright);
		Set_LEDBright(Bright);
	}
	DP_SaveBrightVals(Bright);
	conf_file_write(ConFigFile,"brightmode","bright",brightVals);
	PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	return 0;

}

//获取屏幕亮度模式与相应的亮度值
static int JX_GetDevBright(JX_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t Bmax,Bmin;
	uint8_t brightMode = 0;
	uint8_t Bright = 0;
	float divbright = 0.0,sbright = 0.0;

	//获取亮度模式
	DP_GetBrightMode(&brightMode);
	brightMode = (brightMode == BRIGHT_AUTO) ? JX_BRIGHT_AUTO : JX_BRIGHT_HAND;
	PROTOCOLStruct->data[0] = brightMode;

	//亮度值，亮度值要先转换成0-31的范围
	DP_ReadBrightVals(&Bright);
	DP_GetBrightRange(&Bmax,&Bmin);
	divbright = (Bmax - Bmin) / (float)JX_BRIGHT_RANK;
	sbright = Bright / divbright;
	Bright = (sbright - (uint8_t)sbright > 0.5) ? ((uint8_t)sbright + 1) : ((uint8_t)sbright);
	debug_printf("brightMode = 0x%x,sbright = %f\n",brightMode,sbright);

	PROTOCOLStruct->data[1] = Bright / 10 + 0x30;
	PROTOCOLStruct->data[2] = Bright % 10 + 0x30;

	PROTOCOLStruct->length = 3;
	return 0;
}
static int JX_GetScrTrouble(JX_PROTOCOLStruct_t *PROTOCOLStruct)
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



int JX_protocolProcessor(user_t *user,uint8_t *PTCdata,uint32_t *PTClenth)
{
	JX_PROTOCOLStruct_t PROTOCOLStruct;
	unsigned short parity = 0;
	unsigned char ptcdata[128];
	uint32_t  len;
	unsigned int nlength;
	unsigned int err = -1;

	unsigned char *parsing_output = NULL;
	unsigned int outlen = 0;

	int output_total_len = 0;
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
	//反转义
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
	JX_PTCINITstruct(&PROTOCOLStruct,user,FREEdata,outlen);

	debug_printf("----------------------recv data-----------------------\n");
	prtclmsg_printf(&PROTOCOLStruct);
	//下面的可忽略不计
	//printf("PROTOCOLStruct.CMDID = 0x%x\n",PROTOCOLStruct.CMDID);
	switch(PROTOCOLStruct.CMDID)
	{
		//查询屏幕故障
		case JX_CHECK_TROUBLE:
			JX_GetScrTrouble(&PROTOCOLStruct);
			break;

		//上位机发送过来
		case JX_FILE_RX:
			JX_RxFileFrmUpper(&PROTOCOLStruct);
			break;

		//下载文件---发送给上位机
		case JX_FILE_TX:
			JX_TxFileToUpper(&PROTOCOLStruct);
			break;

		//当前显示内容
		case JX_CUR_DSP:
			JX_GetCurPlayLst(&PROTOCOLStruct);
			break;
		//显示播放列表
		case JX_SET_DSP:
			JX_PlayPreSetLst(&PROTOCOLStruct);
			break;	

		//获取当前亮度模式与亮度值
		case JX_GET_BRIGHT:
			JX_GetDevBright(&PROTOCOLStruct);
			break;
		
		//设置亮度模式与亮度值
		case JX_SET_BRIGHT:
			JX_SetDevBright(&PROTOCOLStruct);
			break;

		default:
			break;
	}
	debug_printf("PROTOCOLStruct.length = %d\n",PROTOCOLStruct.length);
	debug_printf("----------------------send data-----------------------\n");
	//prtclmsg_printf(&PROTOCOLStruct);
	
	struct_to_bytes(&PROTOCOLStruct,FREEdata);
	//校验值
	parity = XKCalculateCRC(FREEdata+1,2+PROTOCOLStruct.length);
	//debug_printf("PROTOCOLStruct.parity = 0x%x\n",PROTOCOLStruct.parity);
	FREEdata[JX_PARITY_BYTE_POS(PROTOCOLStruct.length) + 0] = (uint8_t)(parity >> 8);
	FREEdata[JX_PARITY_BYTE_POS(PROTOCOLStruct.length) + 1] = (uint8_t)(parity);
	//检测字节序中是否存在0x02、0x03、0x1B，有则进行相应的转换，此处处理办法是将头尾两个字节踢掉在处理
	DEBUG_PRINTF;
	check_0x02and0x03(FLAG_SEND,FREEdata+1,2+PROTOCOLStruct.length+2,PTCdata+1,PTClenth);
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





