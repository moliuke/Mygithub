#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "../PTC_init.h"
#include "myerror.h"
#include "debug.h"
#include "PPL_protocol.h"
#include "file_trmit.h"
#include "wdt.h"
#include "conf.h"

#include "../threadpool.h"
#include "../PTC_common.h"
#include "../PTC_FileRxTx.h"
#include "../PTC_FileCopy.h"
#include "PPL_datapool.h"
#include "../../Hardware/Data_pool.h"
#include "../../Hardware/HW3G_RXTX.h"
#include "../../Hardware/HW2G_400.h"



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


static void prtclmsg_printf(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
{
	int i = 0; 
	debug_printf("\n\n=================================\n");
	debug_printf(
		"ip:				%s\n"
		"port:				%d\n"
		"fd:				%d\n"
		"startbyte:			0x%x\n"
		"cmdID:				0x%x\n"
		"devID:				0x%x\n",
		PROTOCOLStruct->user->ip,
		PROTOCOLStruct->user->port,
		PROTOCOLStruct->user->fd,
		PROTOCOLStruct->startByte,
		PROTOCOLStruct->CMDID,
		PROTOCOLStruct->DEVID);	
	debug_printf("data:				");
	for(i = 0 ; i < PROTOCOLStruct->length ; i ++)
	{
		debug_printf("0x%x\t",PROTOCOLStruct->data[i]);
	}

	debug_printf("\n"
		"parity:				0x%x\n"
		"endbyte:			0x%x\n"
		"length:				0x%x\n",
		PROTOCOLStruct->parity,
		PROTOCOLStruct->endByte,
		PROTOCOLStruct->length
		);
	debug_printf("=====================================\n\n");
	
}

static PPLError_t struct_to_bytes(PPL_PROTOCOLStruct_t *PROTOCOLStruct,uint8_t *outputbytes)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	
	if(PROTOCOLStruct == NULL || outputbytes == NULL)
		return PPL_ERR_ARGUMENT;
	
	outputbytes[PLL_START_BYTE_POS] = PROTOCOLStruct->startByte;
	outputbytes[PLL_DEVID_BYTE_POS + 0] = (uint8_t)(PROTOCOLStruct->DEVID >> 8);
	outputbytes[PLL_DEVID_BYTE_POS + 1] = (uint8_t)(PROTOCOLStruct->DEVID);
	outputbytes[PLL_CMDID_BYTE_POS + 0] = (uint8_t)(PROTOCOLStruct->CMDID >> 8);
	outputbytes[PLL_CMDID_BYTE_POS + 1] = (uint8_t)(PROTOCOLStruct->CMDID);

	for(i = 0 ; i < PROTOCOLStruct->length; i ++)
	{
		//debug_printf("0x%x\t",protocol->protcmsg.data[i]);
		outputbytes[PLL_DATAS_BYTE_POS + i]	= PROTOCOLStruct->data[i];
	}

	outputbytes[PLL_PARITY_BYTE_POS(PROTOCOLStruct->length) + 0] = (uint8_t)(PROTOCOLStruct->parity >> 8);
	outputbytes[PLL_PARITY_BYTE_POS(PROTOCOLStruct->length) + 1] = (uint8_t)(PROTOCOLStruct->parity);
	outputbytes[PLL_END_BYTE_POS(PROTOCOLStruct->length)] = PROTOCOLStruct->endByte;
}



void PPL_PTCINITstruct(PPL_PROTOCOLStruct_t *PROTOCOLStruct,user_t *user,uint8_t *data,uint32_t len)
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

static int PPL_CheckDevStatus(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
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
static int PPL_PlayPreSetLst(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
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
	
	if(COM_CopyFile(listname,PLAY_LIST) < 0)  
		goto EXCEPTION;

	debug_printf("listname = %s\n",listname); 

	if(access(listname,F_OK) < 0)
	{
		debug_printf("The list [%s.lst]file is not exist!\n",playlist);
		goto EXCEPTION;
	}

	if(PPL_PLst_parsing(&content,listname) != 0)
		goto EXCEPTION;
	DEBUG_PRINTF;
	PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	DEBUG_PRINTF;
	return 0;

	EXCEPTION:
		PROTOCOLStruct->data[0]	 = 0x30;//0xff;
		PROTOCOLStruct->length	 = 1;
		return -1;
} 


static int PPL_GetCurPlayLst(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t Order = 0,Font,Size = 0;
	uint8_t Len;
	uint8_t inform = 0;
	uint16_t CurLen = 0;
	char CurStr[1024];
	char list[8];
	uint8_t listLen = 0;
	uint32_t inSpeed = 0,stayTime = 0;
	memset(list,0,sizeof(list));
	memset(CurStr,0,1024);
	//PPL_GETCurDspLst(list,&listLen);
	//DP_CurPlayList(OPS_MODE_GET,list,&Len);
	DP_GetCurPlayList(list,&Len);
	
	debug_printf("list = %s\n",list);
	memcpy(PROTOCOLStruct->data,"list",listLen);
	//PPL_GETCurDspContent(CurStr,&CurLen,&inform,&inSpeed,&stayTime);
	DP_GetCurPlayContent(CurStr,&CurLen,&Font,&Size,&inform,&inSpeed,&stayTime,&Order);
	stayTime = stayTime * 100 / (1000 * 1000);
	debug_printf("stayTime = %d,CurLen = %d\n",stayTime,CurLen);
	
	PROTOCOLStruct->data[0] = Order /100 + 0x30;
	PROTOCOLStruct->data[1] = Order /100 / 10 + 0x30;
	PROTOCOLStruct->data[2] = Order % 10 + 0x30;
	
	PROTOCOLStruct->data[3] = stayTime / 10000 		+ 0x30;
	PROTOCOLStruct->data[4] = stayTime / 1000 % 10  + 0x30;
	PROTOCOLStruct->data[5] = stayTime / 100 % 10  	+ 0x30;
	PROTOCOLStruct->data[6] = stayTime / 10 % 10  	+ 0x30;
	PROTOCOLStruct->data[7] = stayTime % 10  		+ 0x30;

	PROTOCOLStruct->data[8] = inform / 10  			+ 0x30;
	PROTOCOLStruct->data[9] = inform % 10  			+ 0x30;

	PROTOCOLStruct->data[10] = inSpeed / 10000 		+ 0x30;
	PROTOCOLStruct->data[11] = inSpeed / 1000 % 10  + 0x30;
	PROTOCOLStruct->data[12] = inSpeed / 100 % 10  	+ 0x30;
	PROTOCOLStruct->data[13] = inSpeed / 10 % 10  	+ 0x30;
	PROTOCOLStruct->data[14] = inSpeed % 10  		+ 0x30;

	memcpy(PROTOCOLStruct->data + 15,CurStr,CurLen);
	DEBUG_PRINTF;
	PROTOCOLStruct->length   = 15 + CurLen;	
	return 0;
}






static int PPL_RxFileFrmUpper(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
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
		DEBUG_PRINTF;
		if(strstr(filePath,"cls.conf") != NULL)
		{
			DEBUG_PRINTF;
			debug_printf("filePath = %s\n",filePath);
			copy_file(ConFigFile,ConFigFile_CPY);
			copy_file(ConFigFile,ConFigFile_Setting);
			copy_file(ConFigFile,ConFigFile_Setting_config);
			copy_file(ConFigFile,ConFigFile_Setting_config_cpy);
		}
		chmod(filePath,0744);
	}

#if 1
	DEBUG_PRINTF;
	//非PLAY.LST则直接返回
	if(strncmp(filename,"play.lst",8) != 0)
	{
		PROTOCOLStruct->data[0] = 0x30; 	//0表示成功
		PROTOCOLStruct->length	= 1;
		return 0;
	}
#endif
	DEBUG_PRINTF;


	//如果上传的文件是PALY.LST,则同时显示该播放列表
	//char srcPath[64];
	//memset(srcPath,0,sizeof(filename));
	//sprintf(srcPath,"%s/%s",list_dir_1,filename);
	//debug_printf("srcPath = %s\n",srcPath);
	//DEBUG_PRINTF;
	//PPL_SETCurDspLst(filename,strlen(filename));
	debug_printf("filename = %s\n",filename);
	DP_SetCurPlayList(filename,strlen(filename));
	conf_file_write(ConFigFile,"playlist","list",filename);
	//if(COM_CopyFile(srcPath,PLAY_LIST) < 0)
	//	goto EXCEPTION;
	DEBUG_PRINTF;
	if(PPL_PLst_parsing(&content,PLAY_LIST) != 0)
		goto EXCEPTION;
	DEBUG_PRINTF;
	PROTOCOLStruct->data[0] = 0x30; 	//0表示成功
	PROTOCOLStruct->length	= 1;
	return 0;

	EXCEPTION:
		PROTOCOLStruct->data[0] = 0x31;	//非0表示失败
		PROTOCOLStruct->length  = 1;
		return -1;
		
}

static int PPL_TxFileToUpper(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
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

	//帧偏移
	Flag0x2B = strchr(dataPoint,0x2b);
	if(Flag0x2B == NULL)
		goto EXCEPTION;

	OffsetPos = Flag0x2B + 1;
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

static int PPL_SetModeOfBright(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t brightMode = 0;
	uint8_t Bright;
	
	brightMode = PROTOCOLStruct->data[0];
	debug_printf("brightMode = 0x%x\n",brightMode);
	brightMode = (brightMode == PPL_BRIGHT_HAND) ? BRIGHT_HAND : BRIGHT_AUTO;
	debug_printf("brightMode = 0x%x\n",brightMode);
	DEBUG_PRINTF;
	//PPL_SETBrightMode(brightMode);
	//DP_BrightAndMode(OPS_MODE_SET,&brightMode,NULL,NULL,NULL);
	DP_SetBrightMode(brightMode);
	
	//下面是真正设置设备的亮度调节方式
	if(brightMode == BRIGHT_HAND)
		conf_file_write(ConFigFile,"brightmode","mode","30");

	if(brightMode == BRIGHT_AUTO)
		conf_file_write(ConFigFile,"brightmode","mode","31");
		
	PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	return 0;
}

static int PPL_SetDevBright(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t Bmode,Bright,Bmax,Bmin;
	uint8_t RGB_R,RGB_G,RGB_B;
	char brightVals[4];
	DEBUG_PRINTF;
	//DP_GetBrightAndMode(&Bmode,&Bright);
	DP_GetBrightMode(&Bmode);
	debug_printf("Bmode = %x\n",Bmode);
	if(Bmode == BRIGHT_AUTO)
		goto EXCEPTION;
	DEBUG_PRINTF;

	RGB_R = (PROTOCOLStruct->data[0] - 0x30) * 10 + (PROTOCOLStruct->data[1] - 0x30);
	RGB_G = (PROTOCOLStruct->data[2] - 0x30) * 10 + (PROTOCOLStruct->data[3] - 0x30);
	RGB_B = (PROTOCOLStruct->data[4] - 0x30) * 10 + (PROTOCOLStruct->data[5] - 0x30);

	//亮度分成32级(0-31)
	RGB_R = (RGB_R <= 0) ? 0 : RGB_R;
	RGB_R = (RGB_R >= 31) ? 31 : RGB_R;
	DP_SaveBrightVals(RGB_R);

	//针对扫描版的亮度设置
	HW2G400_SETLEDbright(RGB_R);
	
	//针对TXRX板的亮度设置
	float div = 0.0,fbright = 0.0;
	DP_GetBrightRange(&Bmax,&Bmin);
	div = (Bmax - Bmin) / (float)32;
	fbright = RGB_R * div + Bmin;
	RGB_R = (fbright - (uint8_t)fbright > 0.5) ? ((uint8_t)fbright + 1) : ((uint8_t)fbright);
	RGB_R = (RGB_R <= Bmin) ? Bmin : RGB_R;
	RGB_R = (RGB_R >= Bmax) ? Bmax : RGB_R;
	Set_LEDBright(RGB_R);

	memset(brightVals,0,sizeof(brightVals));
	sprintf(brightVals,"%d",RGB_R);
	conf_file_write(ConFigFile,"brightmode","bright",brightVals);


	PROTOCOLStruct->data[0] = 0x30;
	PROTOCOLStruct->length = 1;
	return 0;

	EXCEPTION:
		PROTOCOLStruct->data[0] = 0x31;
		PROTOCOLStruct->length = 1;
		return -1;
}


static int PPL_GetModeAndBright(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
{
	uint8_t brightMode = 0;
	uint8_t RGB_R,RGB_G,RGB_B;

	//PPL_GETBrightMode(&brightMode);
	//DP_BrightAndMode(OPS_MODE_GET,&brightMode,NULL,NULL,NULL);
	DP_GetBrightMode(&brightMode);
	debug_printf("brightMode = 0x%x\n",brightMode);

	brightMode = (brightMode == BRIGHT_AUTO) ? PPL_BRIGHT_AUTO : PPL_BRIGHT_HAND;
	PROTOCOLStruct->data[0] = brightMode;

	//PPL_GETCurBright(&RGB_R,&RGB_G,&RGB_B);
	//DP_BrightAndMode(OPS_MODE_GET,NULL,&RGB_R,NULL,NULL);
	DP_ReadBrightVals(&RGB_R);
	
	PROTOCOLStruct->data[1] = RGB_R / 10 + 0x30;
	PROTOCOLStruct->data[2] = RGB_R % 10 + 0x30;

	PROTOCOLStruct->length = 3;
	return 0;
}



static int PPL_SetDevIP(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
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

static int PPL_SetDevReset(PPL_PROTOCOLStruct_t *PROTOCOLStruct)
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



//下面的协议是紫光的协议
int PPL_protocolProcessor(user_t *user,uint8_t *PTCdata,uint32_t *PTClenth)
{
	PPL_PROTOCOLStruct_t PROTOCOLStruct;
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
	debug_printf("*PTClenth = %d\n",*PTClenth);
	err = prtcl_preparsing(PTCdata,*PTClenth,FREEdata,&outlen);
	if(err != 0)
	{
		debug_printf("find error when preparsing the recv data\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"prtcl_preparsing failed",strlen("prtcl_preparsing failed"));
		goto ERRORDEAL;
	}

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
	
 	int ret = 0; 
	int k = 0;
	PPL_PTCINITstruct(&PROTOCOLStruct,user,FREEdata,outlen);

	//debug_printf("----------------------recv data-----------------------\n");
	//prtclmsg_printf(&PROTOCOLStruct);
	//下面的可忽略不计
	switch(PROTOCOLStruct.CMDID)
	{
		case 0x3030:
			PPL_SetDevReset(&PROTOCOLStruct);
			break;
		case 0x3338:
			PPL_SetDevIP(&PROTOCOLStruct);
			break;
		case 0x3031:
			PPL_CheckDevStatus(&PROTOCOLStruct);
			break;

		case 0x3938:
			PPL_PlayPreSetLst(&PROTOCOLStruct);
			break;

		case 0x3937:
			PPL_GetCurPlayLst(&PROTOCOLStruct);
			break;

		case 0x3039:
			PPL_TxFileToUpper(&PROTOCOLStruct);
			break;

		case 0x3130:
			PPL_RxFileFrmUpper(&PROTOCOLStruct);
			break;

		case 0x3034:
			PPL_SetModeOfBright(&PROTOCOLStruct);
			break;

		case 0x3035:
			PPL_SetDevBright(&PROTOCOLStruct);
			break;

		case 0x3036:
			PPL_GetModeAndBright(&PROTOCOLStruct);
			debug_printf("#PROTOCOLStruct->length = %d\n",PROTOCOLStruct.length);
			break;

		default:
			break;
	}
	debug_printf("PROTOCOLStruct.length = %d\n",PROTOCOLStruct.length);
	debug_printf("----------------------send data-----------------------\n");
	//prtclmsg_printf(&PROTOCOLStruct);
	
	struct_to_bytes(&PROTOCOLStruct,FREEdata);
	//校验值
	parity = XKCalculateCRC(FREEdata+1,4+PROTOCOLStruct.length);
	//debug_printf("PROTOCOLStruct.parity = 0x%x\n",PROTOCOLStruct.parity);
	FREEdata[PLL_PARITY_BYTE_POS(PROTOCOLStruct.length) + 0] = (uint8_t)(parity >> 8);
	FREEdata[PLL_PARITY_BYTE_POS(PROTOCOLStruct.length) + 1] = (uint8_t)(parity);
	//检测字节序中是否存在0x02、0x03、0x1B，有则进行相应的转换，此处处理办法是将头尾两个字节踢掉在处理
	DEBUG_PRINTF;
	check_0x02and0x03(FLAG_SEND,FREEdata+1,4+PROTOCOLStruct.length+2,PTCdata+1,PTClenth);
	DEBUG_PRINTF;
	
	output_total_len = *PTClenth;
	//输出字节序再加上头尾两个字节
	PTCdata[0] 					= 0x02;
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





