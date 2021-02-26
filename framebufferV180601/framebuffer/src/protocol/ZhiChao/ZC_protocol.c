#include <sys/io.h>
#include <sys/socket.h>
#include "ZC_protocol.h"

#include "../PTC_common.h"

#include "config.h"
#include "debug.h"
#include "conf.h"
#include "mtime.h"
#include "ZC_Lstparse.h"

#include "../PTC_FileRxTx.h"
#include "content.h"
#include "../../Hardware/Data_pool.h"
#include "../../Hardware/HW2G_400.h"
#include "../../Hardware/HW3G_RXTX.h"
#include "../PTC_init.h"
#include "../../protocol/PTC_FileCopy.h"


#define outp(a, b) outb(b, a)
#define inp(a) inb(a)

static void ZCStructPrintf(ZCPTCStruct_t *ZCStruct)
{
	int i = 0; 
	debug_printf(
		"ip:				%s\n"
		"port:				%d\n"
		"fd:				%d\n"
		"startbyte:			0x%x\n"
		"DevAddr:			0x%x\n"
		"CmdCode:			0x%x\n",
		ZCStruct->user->ip,
		ZCStruct->user->port,
		ZCStruct->user->fd,
		ZCStruct->StartByte,
		ZCStruct->DevAddr,
		ZCStruct->CmdCode);	
	debug_printf("data:				");
	for(i = 0 ; i < ZCStruct->DataLen ; i ++)
	{
		debug_printf("%02x ",ZCStruct->Data[i]);
	}

	debug_printf("\n"
		"CRC16:				0x%x\n"
		"EndByte:			0x%x\n"
		"length:			0x%x\n",
		ZCStruct->CRC16,
		ZCStruct->EndByte,
		ZCStruct->DataLen
		);
	debug_printf("=====================================\n\n");
	
}


////////////////////////////////////////////////////////////////////////
//一、为协议反转义分配一块固定的内存块
/////////////////////////////////////////////////////////////////////////
#if 0
static void MemoryMalloc(void)
{
	FREEdata = (uint8_t *)malloc(MEMSIZE);
	if(FREEdata == NULL)
	{
		perror("MemoryMalloc malloc");
		debug_printf("allocate memory failed\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"parsing_output malloc failed",strlen("parsing_output malloc failed"));
		exit(1);
	}
	memset(FREEdata,0,MEMSIZE);
}
#endif




static void ZC_ProtocolStrInit(ZCPTCStruct_t *ZCPTCStruct,user_t *user,uint8_t *Data,uint32_t Len)
{
	ZCPTCStruct->user		= user;
	ZCPTCStruct->StartByte 	= Data[0];
	ZCPTCStruct->DevAddr	= Data[1] << 8 | Data[2];
	ZCPTCStruct->CmdCode	= Data[3] << 8 | Data[4];
	ZCPTCStruct->CRC16		= Data[Len - 3] << 8 | Data[Len - 2];
	ZCPTCStruct->DataLen	= Len - 8;
	ZCPTCStruct->Data		= Data + 5;
	ZCPTCStruct->EndByte	= Data[Len - 1];
}

static int ZC_StructToByte(ZCPTCStruct_t *ZCPTCStruct,uint8_t *Output)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	
	if(ZCPTCStruct == NULL || Output == NULL)
		return -1;
	
	Output[ZC_START_BYTE_POS] = ZCPTCStruct->StartByte;
	Output[ZC_DEVID_BYTE_POS + 0] = (uint8_t)(ZCPTCStruct->DevAddr >> 8);
	Output[ZC_DEVID_BYTE_POS + 1] = (uint8_t)(ZCPTCStruct->DevAddr);

	for(i = 0 ; i < ZCPTCStruct->DataLen; i ++)
	{
		//debug_printf("0x%x\t",protocol->protcmsg.data[i]);
		Output[ZC_DATAS_BYTE_POS + i]	= ZCPTCStruct->Data[i];
	}

	Output[ZC_PARITY_BYTE_POS(ZCPTCStruct->DataLen) + 0] = (uint8_t)(ZCPTCStruct->CRC16 >> 8);
	Output[ZC_PARITY_BYTE_POS(ZCPTCStruct->DataLen) + 1] = (uint8_t)(ZCPTCStruct->CRC16);
	Output[ZC_END_BYTE_POS(ZCPTCStruct->DataLen)] = ZCPTCStruct->EndByte;
}


//返回信息重新组装时使用显科的协议的字节序
static int ZC_StructToByte_SWRcompatible(ZCPTCStruct_t *ZCPTCStruct,uint8_t *Output)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	
	if(ZCPTCStruct == NULL || Output == NULL)
		return -1;
	
	Output[COMPAT_START_BYTE] = ZCPTCStruct->StartByte;
	Output[COMPAT_SWR_CMD_POS + 0] = (uint8_t)(ZCPTCStruct->DevAddr >> 8);
	Output[COMPAT_SWR_CMD_POS + 1] = (uint8_t)(ZCPTCStruct->DevAddr);

	Output[COMPAT_SWR_DEV_POS + 0] = (uint8_t)(ZCPTCStruct->CmdCode >> 8);
	Output[COMPAT_SWR_DEV_POS + 0] = (uint8_t)(ZCPTCStruct->CmdCode);

	for(i = 0 ; i < ZCPTCStruct->DataLen; i ++)
	{
		//debug_printf("0x%x\t",protocol->protcmsg.data[i]);
		Output[COMPAT_SWR_DATA_POS + i]	= ZCPTCStruct->Data[i];
	}

	Output[COMPAT_PARITY_BYTE_POS(ZCPTCStruct->DataLen) + 0] = (uint8_t)(ZCPTCStruct->CRC16 >> 8);
	Output[ZC_PARITY_BYTE_POS(ZCPTCStruct->DataLen) + 1] = (uint8_t)(ZCPTCStruct->CRC16);
	Output[COMPAT_END_BYTE_POS(ZCPTCStruct->DataLen)] = ZCPTCStruct->EndByte;
}



static int ZC_SetDevIP(ZCPTCStruct_t *ZCPTCStruct)
{
	unsigned char dev_ip[24] , dev_mask[24] , dev_gw[24] , dev_port[8];
	unsigned char *ip = NULL,*mask = NULL,*gw = NULL,*port = NULL;
	unsigned short _port = 0;

	char conf_file[64];
	memset(conf_file,0x00,sizeof(conf_file));
	sprintf(conf_file,"%s/cls.conf",conf_dir);
	
	ip 	 = ZCPTCStruct->Data + 6;
	mask = ZCPTCStruct->Data + 10;
	gw   = ZCPTCStruct->Data + 14;
	port = ZCPTCStruct->Data + 24;

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
	

	ZCPTCStruct->Data[2] = 0x31;
	ZCPTCStruct->DataLen = 3;

	char ip_port[24];
	char logmsg[96];
	memset(ip_port,0,sizeof(ip_port));
	sprintf(ip_port,"%s:%d",ZCPTCStruct->user->ip,ZCPTCStruct->user->port);
	sprintf(logmsg,"cmd:%x setnet:ip: %s netmask:%s gateway:%s port:%s",
		ZCPTCStruct->CmdCode,dev_ip,dev_mask,dev_gw,dev_port);
	debug_printf("strlen(logmsg) = %d\n",strlen(logmsg));
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	return 0;
}



static uint8_t ZC_HexToByte(uint8_t HexData)
{
	if(HexData >= 0x0 && HexData <= 0x9)
		return HexData + 0x30;

	if(HexData >= 0xa && HexData <= 0xf)
		return HexData + 0x37;
}


static int ZC_GetDetailStatus(ZCPTCStruct_t *ZCPTCStruct)
{
	uint16_t DetailStatus = 0x00;
	uint8_t status,vals;
	
	//控制器故障
	//DP_GetSysDataAndStatus(PID_CTROLLER,&status,&vals);
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 1) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);

	//显示模组故障
	//DP_GetSysDataAndStatus(PID_MODEL,&status,&vals);
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 2) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);
	//显示模组电源故障
	//DP_GetSysDataAndStatus(PID_MODEL_POWER,&status,&vals);
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 3) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);
	//单像素管故障
	//DP_GetSysDataAndStatus(PID_MODEL_POWER,&status,&vals);
	uint8_t PixStatus;
	uint32_t PixBads;
	//DP_GetPixelsStatus(&PixStatus,&PixBads);
	status = 0x31;
	DetailStatus = (PixStatus == 0x30) ? (DetailStatus | 1 << 4) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);
	//检测系统故障
	//DP_GetSysDataAndStatus(PID_SYSCHECK,&status,&vals);
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 5) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);
	//输入220V交流电故障
	//DP_GetSysDataAndStatus(PID_POWER,&status,&vals);
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 6) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);
	//防雷器故障
	DP_GetSysDataAndStatus(PID_THANDER,&status,&vals);
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 7) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);
	//光敏部件故障
	DP_GetSysDataAndStatus(PID_LIGHT_SENSITIVE,&status,&vals);
	debug_printf("LS status = 0x%x\n",status);
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 8) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);
	//温度异常故障
	DP_GetSysDataAndStatus(PID_TEMP,&status,&vals);
	status = 0x31;
	DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 9) : (DetailStatus);
	debug_printf("status = 0x%x\n",status);
	

	uint8_t HexData = 0;
	HexData = (uint8_t)((0xF000 & DetailStatus) >> 12);
	ZCPTCStruct->Data[0] = ZC_HexToByte(HexData);

	HexData = (uint8_t)((0x0F00 & DetailStatus) >> 8);
	ZCPTCStruct->Data[1] = ZC_HexToByte(HexData);

	HexData = (uint8_t)((0x00F0 & DetailStatus) >> 4);
	ZCPTCStruct->Data[2] = ZC_HexToByte(HexData);

	HexData = (uint8_t)((0x000F & DetailStatus) >> 0);
	ZCPTCStruct->Data[3] = ZC_HexToByte(HexData);

	ZCPTCStruct->DataLen = 4;
	return 0;
}


static int ZC_RecvFileFromUpper(ZCPTCStruct_t *ZCPTCStruct)
{
	int ret = -1;
	uint8_t *Flag0x2B = NULL;
	uint8_t *dataPoint = ZCPTCStruct->Data;
	char *filedata = NULL;
	uint32_t filedataLen = 0;
	char filename[24];
	uint8_t filenameLen = 0;
	uint32_t frameOffset = 0;

	static int fileLen = 0;

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
	debug_printf("filename = %s,filenameLen = %d\n",filename,filenameLen);

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
	filedataLen = ZCPTCStruct->DataLen - filenameLen - 1 - 4;
	debug_printf("filedataLen = %d\n",filedataLen);

	fileLen += filedataLen;
	debug_printf("filedataLen = %d,fileLen = %d\n",filedataLen,fileLen);

	//文件存放路径
	char filePath[64];
	memset(filePath,0,sizeof(filePath));

	if(strstr(filename,".bmp") != NULL)
		sprintf(filePath,"%s/%s",image_dir,filename);
	else if(strstr(filename,".lst") != NULL )
		sprintf(filePath,"%s/%s",list_dir_1,filename);
	else if(strstr(filename,".conf") != NULL)
		sprintf(filePath,"%s/%s",conf_dir,filename);
	else if(strstr(filename,"bootup") != NULL)
		sprintf(filePath,"%s/%s",boot_dir,filename);
	else
		sprintf(filePath,"%s/%s",sys_dir,filename);
		
	DEBUG_PRINTF;
	//文件帧数据处理
	debug_printf("ZCPTCStruct->user->ip = %s\n",ZCPTCStruct->user->ip);
	memset(&FileUser,0,sizeof(FILEUser_t));
	FRTx_FileUserInit(&FileUser,ZCPTCStruct->user->type,ZCPTCStruct->user->ip,ZCPTCStruct->user->port,ZCPTCStruct->user->uartPort);
	if((ret = FRTx_FileFrameRx(&FileUser,filePath,filedata,filedataLen,frameOffset)) < 0)
		goto EXCEPTION;

	debug_printf("filename = %s\n",filename);

	if(ret == 1)
	{
		debug_printf("recv file [play.lst] ok\n ");
		chmod(filePath,0744);
		//名为play.lst的文件名为插播播放列表
		if(strncmp(filename,"play.lst",8) == 0)
		{
			debug_printf("parsing play.lst\n");
			if(ZC_Lstparsing(&content,filePath) < 0)
				goto EXCEPTION;
		}
	}

	ZCPTCStruct->Data[0] 	= 0x30; 	//0表示成功
	ZCPTCStruct->DataLen	= 1;
	return 0;

	EXCEPTION:
		ZCPTCStruct->Data[0] = 0x31;	//非0表示失败
		ZCPTCStruct->DataLen  = 1;
		return -1;
		
}

static int ZC_UploadFileToUpper(ZCPTCStruct_t *ZCPTCStruct)
{
	char FileName[24];
	uint8_t FilenameLen = 0;
	char filePath[64];
	uint8_t *FrameOffsetStr = NULL;
	uint32_t frameOffset = 0;
	FILEUser_t FileUser;
	char *dataPoint = ZCPTCStruct->Data;
	char *OffsetPos = NULL;
	char *Flag0x2B = NULL;
	DEBUG_PRINTF;
	//取文件名并确定文件所属路径
	memset(FileName,0,sizeof(FileName));
	memset(filePath,0,sizeof(filePath));
	debug_printf("dataPoint = %s\n",dataPoint);
	DEBUG_PRINTF;
	
	FilenameLen = ZCPTCStruct->DataLen - 4;
	memcpy(FileName,dataPoint,FilenameLen);
	debug_printf("filename = %s\n",FileName);
	DEBUG_PRINTF;

	//将文件路径中的'\'转化成'/'并将存在的大写字母转换成小写字母
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);


	//读取文件的路径
	sprintf(filePath,"%s/%s",sys_dir,FileName);

	//获取偏移地址
	FrameOffsetStr = dataPoint + FilenameLen;
	frameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];
	debug_printf("frameOffset = %d\n",frameOffset);

	
	//文件数据处理
	char *frameData = ZCPTCStruct->Data;
	uint32_t frameDataLen = 0;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,ZCPTCStruct->user->type,ZCPTCStruct->user->ip,ZCPTCStruct->user->port,ZCPTCStruct->user->uartPort);
	DEBUG_PRINTF;
	if(FRTx_FileFrameTx(&FileUser,filePath,frameData,&frameDataLen,frameOffset) < 0)
		goto EXCEPTION;
	DEBUG_PRINTF;

	//ZCPTCStruct->Data[0] = 0x30; 	//0表示成功
	ZCPTCStruct->DataLen = frameDataLen;
	debug_printf("frameDataLen = %d\n",frameDataLen);

	return 0;

	EXCEPTION:
		ZCPTCStruct->Data[0] = 0x31;	//非0表示失败
		ZCPTCStruct->DataLen  = 1;
		return -1;
}




static int ZC_SetPlayList(ZCPTCStruct_t *ZCPTCStruct)
{
	int Is_CutIn_Plist = -1;
	debug_printf("preset play list\n");
	char playlist[8];
	char ListName[64];
	
	memset(playlist,0,sizeof(playlist));
	memcpy(playlist,ZCPTCStruct->Data,3);
	
	sprintf(ListName,"%s/%s.lst",list_dir_1,playlist);
	
	ListName[strlen(ListName)] = '\0';
	debug_printf("ListName = %s\n",ListName);
	if(access(ListName,F_OK) < 0)
	{
		debug_printf("The list [%s]file is not exist!\n",ListName);
		goto EXCEPTION;
	}
	if(COM_CopyFile(ListName,ZC_PLAY_LST) < 0)
		goto EXCEPTION;
	DEBUG_PRINTF;
	if((Is_CutIn_Plist = ZC_Lstparsing(&content,ListName)) < 0)
	{
		DEBUG_PRINTF;
		goto EXCEPTION;
	}
	
	uint8_t CurPLst[8];
	uint8_t Len;
	sprintf(CurPLst,"%s.lst",playlist);
	Len = strlen(CurPLst);

	//DP_CurPlayList(OPS_MODE_SET,CurPLst,&Len);
	DP_SetCurPlayList(CurPLst,Len);
	
	debug_printf("playlist = %s,CurPLst = %s\n",playlist,CurPLst);
	
	if(Is_CutIn_Plist == 0)
		conf_file_write(ConFigFile,"playlist","list",CurPLst);

	ZCPTCStruct->Data[0] = 0x30;
	ZCPTCStruct->DataLen= 1;
	DEBUG_PRINTF;
	return 0;

	EXCEPTION:
		DEBUG_PRINTF;
		ZCPTCStruct->Data[0] = 0x31;//0xff;
		ZCPTCStruct->DataLen	 = 1;
		return -1;
	
}

static int ZC_GetPlayContent(ZCPTCStruct_t *ZCPTCStruct)
{
	AHCurplay_t AHCurplay;
	memset(&AHCurplay,0,sizeof(AHCurplay_t));
	AHGetCurPlaying(&AHCurplay);

	ZCPTCStruct->Data[0] = AHCurplay.order / 100 + 0x30;
	ZCPTCStruct->Data[1] = AHCurplay.order / 10 % 10 + 0x30;
	ZCPTCStruct->Data[2] = AHCurplay.order % 10 + 0x30;

	debug_printf("AHCurplay.stoptime = %d\n",AHCurplay.stoptime);
	ZCPTCStruct->Data[3] = AHCurplay.stoptime / 10000 + 0x30;
	ZCPTCStruct->Data[4] = AHCurplay.stoptime / 1000 % 10 + 0x30;
	ZCPTCStruct->Data[5] = AHCurplay.stoptime / 100 % 10 + 0x30;
	ZCPTCStruct->Data[6] = AHCurplay.stoptime % 100 / 10 + 0x30;
	ZCPTCStruct->Data[7] = AHCurplay.stoptime % 100 % 10 + 0x30;

	ZCPTCStruct->Data[8] = AHCurplay.effectin / 10 + 0x30;
	ZCPTCStruct->Data[9] = AHCurplay.effectin % 10 + 0x30;

	ZCPTCStruct->Data[10] = AHCurplay.inspeed / 10000 + 0x30;
	ZCPTCStruct->Data[11] = AHCurplay.inspeed / 1000 % 10 + 0x30;
	ZCPTCStruct->Data[12] = AHCurplay.inspeed / 100 % 10 + 0x30;
	ZCPTCStruct->Data[13] = AHCurplay.inspeed % 100 / 10 + 0x30;
	ZCPTCStruct->Data[14] = AHCurplay.inspeed % 100 % 10 + 0x30;

	memcpy(ZCPTCStruct->Data + 15,AHCurplay.playstr,AHCurplay.strLen);

	ZCPTCStruct->DataLen = 15 + AHCurplay.strLen;
	return 0;
}

static int ZC_SetBrightMode(ZCPTCStruct_t *ZCPTCStruct)
{
	uint8_t BrightMode = 0;

	//保存亮度模式时，自动亮度统一用BRIGHT_AUTO，手动亮度统一用BRIGHT_HAND表示
	BrightMode = ZCPTCStruct->Data[0];
	//printf("BrightMode = %x\n",BrightMode);
	BrightMode = (BrightMode == ZC_BRIGHT_HAND) ? BRIGHT_HAND : BRIGHT_AUTO;
	DP_SetBrightMode(BrightMode);

	//将亮度模式写入配置文件
	if(BrightMode == BRIGHT_AUTO)
		conf_file_write(ConFigFile,"brightmode","mode","31");
	if(BrightMode == BRIGHT_HAND)
		conf_file_write(ConFigFile,"brightmode","mode","30");
	
	ZCPTCStruct->Data[0] = 0x30;//0xff;
	ZCPTCStruct->DataLen	 = 1;
	
	return 0;
}

static int ZC_SetBrightVal(ZCPTCStruct_t *ZCPTCStruct)
{
	DEBUG_PRINTF;
	uint8_t bright_mode = 0;
	uint8_t bright_max = 0,bright_min = 0;
	uint8_t R_bright = 0,G_bright = 0,B_bright = 0;
	float Fbright = 0.0,Sbright = 0.0;
	
	uint16_t width,height,count;
	
	DP_GetBrightMode(&bright_mode);

	//0表示自动调节方式，无需设置数值
	if(bright_mode == BRIGHT_AUTO)
		goto EXCEPTION;

	//三基色亮度值
	R_bright = (ZCPTCStruct->Data[0]-0x30)*10+(ZCPTCStruct->Data[1]-0x30);
	G_bright = (ZCPTCStruct->Data[2]-0x30)*10+(ZCPTCStruct->Data[3]-0x30);
	B_bright = (ZCPTCStruct->Data[4]-0x30)*10+(ZCPTCStruct->Data[5]-0x30);
	debug_printf("R_bright = %d,G_bright = %d,B_bright = %d,bright_min = %d,bright_max = %d\n",R_bright,G_bright,B_bright,bright_min,bright_max);

	//亮度分成32级(0-31)
	R_bright = (R_bright <= 0) ? 0 : R_bright;
	R_bright = (R_bright >= 31) ? 31 : R_bright;

	//针对扫描版设置亮度值
	HW2G400_SETLEDbright(R_bright);
	

	//先转换成1-64的范围在保存亮度值
	DP_GetBrightRange(&bright_max,&bright_min);
	Fbright = (bright_max - bright_min) / (float)ZC_BRIGHT_RANK;
	Sbright = Fbright * R_bright + bright_min;
	R_bright = (Sbright - (uint8_t)Sbright > 0.5) ? ((uint8_t)Sbright + 1) : ((uint8_t)Sbright);
	DP_SaveBrightVals(R_bright);
	
	//针对TXRX板设定亮度值
	Set_LEDBright(R_bright);

	//将亮度模式写入配置文件
	uint8_t BrightVals[4];
	memset(BrightVals,0,4);
	sprintf(BrightVals,"%d",R_bright);
	conf_file_write(ConFigFile,"brightmode","bright",BrightVals);
	

	ZCPTCStruct->Data[0] = 0x30; 
	ZCPTCStruct->DataLen = 1;
	return 0;

	EXCEPTION:
		ZCPTCStruct->Data[0] = 0x31; 
		ZCPTCStruct->DataLen = 1;
		return 0;
		
}

static int ZC_GetBrightModeAndVal(ZCPTCStruct_t *ZCPTCStruct)
{
	uint8_t BrightMode,Bright;
	uint8_t BrightVals;
	uint8_t Bmax,Bmin;
	float Fbright = 0.0,Sbright = 0.0;
	//获取亮度模式
	DP_GetBrightMode(&BrightMode);
	BrightMode = (BrightMode == BRIGHT_AUTO) ? ZC_BRIGHT_AUTO : ZC_BRIGHT_HAND;
	ZCPTCStruct->Data[0] = BrightMode;

	//由于亮度值在保存时已经统一转换成1-64的值，这里要重新转换成0-31	
	DP_ReadBrightVals(&BrightVals);
	BrightVals = (BrightVals <= 0) ? 0 : BrightVals;
	BrightVals = (BrightVals >= 31) ? 31 : BrightVals;
	ZCPTCStruct->Data[1] = BrightVals / 10 + 0x30;
	ZCPTCStruct->Data[2] = BrightVals % 10 + 0x30;
	//printf("Fbright = %f,Sbright = %f\n",Fbright,Sbright);

	debug_printf("BrightMode = %d,BrightVals = %d\n",BrightMode,BrightVals);

	ZCPTCStruct->DataLen = 3;
	return 0;
}

static int ZC_SetCurTime(ZCPTCStruct_t *ZCPTCStruct)
{
	uint8_t timestr[24];
	uint32_t year = 0;
	uint8_t month = 0;
	uint8_t day   = 0;
	uint8_t hour  = 0,min = 0,sec = 0;
	DEBUG_PRINTF;
	memset(timestr,0x00,sizeof(timestr));

	year = ((ZCPTCStruct->Data[0] - 0x30)*10 + (ZCPTCStruct->Data[1] - 0x30)) * 100 
			+ (ZCPTCStruct->Data[2] - 0x30) * 10 + (ZCPTCStruct->Data[3] - 0x30);

	month = (ZCPTCStruct->Data[4] - 0x30) * 10 + (ZCPTCStruct->Data[5] - 0x30);
	day	  = (ZCPTCStruct->Data[6] - 0x30) * 10 + (ZCPTCStruct->Data[7] - 0x30);
	hour  = (ZCPTCStruct->Data[8] - 0x30) * 10 + (ZCPTCStruct->Data[9] - 0x30);
	min   = (ZCPTCStruct->Data[10] - 0x30) * 10 + (ZCPTCStruct->Data[11] - 0x30);
	sec   = (ZCPTCStruct->Data[12] - 0x30) * 10 + (ZCPTCStruct->Data[13] - 0x30);

	sprintf(timestr,"%4d/%02d/%02d-%02d:%02d:%02d",year,month,day,hour,min,sec);

	if(set_sys_time(timestr,strlen(timestr)) < 0)
		ZCPTCStruct->Data[0] = 0x31;
	else
		ZCPTCStruct->Data[0] = 0X30;

	ZCPTCStruct->DataLen = 1;
	return 0;
}


int ZC_SysReset(ZCPTCStruct_t *ZCPTCStruct)
{
	/*设置设备重新启动*/
	char systemstr[48];
	char cur_workdir[128];
	int ret = 0;
	//memset(cur_workdir,0x00,sizeof(cur_workdir));
	//getcwd(cur_workdir,sizeof(cur_workdir));
	//sprintf(cur_workdir,"%s",sys_dir);
	chdir(sys_dir);

	sprintf(systemstr,"sh %s","sh.sh");
	system(systemstr);

	ZCPTCStruct->Data[0] = 0x01;
	ZCPTCStruct->DataLen = 1;
	
	//wdt_feed(WDT_FEED_STOP);
	//wdt_stop();
	open(sys_dir"/sys/sys_reboot.lock",O_WRONLY | O_CREAT,0744);
	
	return 0;
}

int ZC_ScreenStatus(ZCPTCStruct_t *ZCPTCStruct)
{
	uint8_t ScreenStatus = 0;
	uint8_t operat_state = 0;
	uint8_t power 		 = 0;

	power 		 = ZCPTCStruct->Data[0];
	operat_state = ZCPTCStruct->Data[1];
	// 命令执行, 打开设备或者关闭设备
	//XKOpenDevice((protocol->protcmsg.data[0]-0x30),(protocol->protcmsg.data[1]-0x30));
	if(operat_state == 0x31)
	{
		/*在这里通知其他模块，说明设备已经打开*/
	#ifdef HW3G_RXTX
		DEBUG_PRINTF_ToFile(__func__,__LINE__);
		//RXTX_SetScreenStatus(LED_STATUS_ON);
		SET_LED_STATE(SLED_ON);
		LEDstateRecord(SLED_ON)
		
	#else
		HW2G400_SetScreenStatus(LED_STATUS_ON);
	#endif
		ScreenStatus = LED_STATUS_ON;
	}
	else
	{
	  	/*可以在这里通知其他模块设备已经关闭*/
	#ifdef HW3G_RXTX
		DEBUG_PRINTF_ToFile(__func__,__LINE__);
		//RXTX_SetScreenStatus(LED_STATUS_OFF);
		SET_LED_STATE(SLED_OFF);
		LEDstateRecord(SLED_OFF)
	#else
		HW2G400_SetScreenStatus(LED_STATUS_OFF);
	#endif
		ScreenStatus = LED_STATUS_OFF;
	}

	DP_SetScreenStatus(ScreenStatus);
	
	// 组建回传应用数据
	ZCPTCStruct->Data[2]= (BYTE)(ZCPTCStruct->Data[1]);
	ZCPTCStruct->Data[1]= (BYTE)(ZCPTCStruct->Data[0]);
	ZCPTCStruct->Data[0]= 0x01;

	ZCPTCStruct->DataLen = 3;

	return 0;
}


static int SWR_FrameRxFrmUpper2K(ZCPTCStruct_t *ZCPTCStruct)
{
	int retvals = -1;
	uint16_t CurOffset = 0;
	uint8_t FileNameLen = 0;
	char FileNameLenStr[4];
	char FileName[48];
	char *FrameIdStr = NULL;
	uint32_t FrameId = 0;
	uint32_t FrameLen = 0;
	char *FrameContent = NULL;

	//取文件名长度
	CurOffset += 2;
	memset(FileNameLenStr,0,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,ZCPTCStruct->Data + CurOffset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//取文件名
	CurOffset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,ZCPTCStruct->Data + CurOffset,FileNameLen);
	FileName[FileNameLen] = '\0';

	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//检查上位机上传过来的文件路径，没有响应文件夹就创建相应的文件夹
	char *p = NULL;
	chdir(sys_dir);
	p = strchr(FileName,'/');
	if(p != NULL)
	{
		DEBUG_PRINTF;
		mkdirs(FileName);
		chdir(sys_dir);
	}

	//帧偏移量
	CurOffset += FileNameLen;
	FrameIdStr = ZCPTCStruct->Data + CurOffset;
	FrameId = (FrameIdStr[0] - 0x30) * 1000 + (FrameIdStr[1] - 0x30) * 100 + 
			  (FrameIdStr[2] - 0x30) * 10 + (FrameIdStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",FrameIdStr[0],FrameIdStr[1],FrameIdStr[2],FrameIdStr[3]);
	debug_printf("FrameId = %d\n",FrameId);

	//帧内容长度
	CurOffset += 4;
	FrameLen = ZCPTCStruct->DataLen - CurOffset;

	//帧内容
	FrameContent = ZCPTCStruct->Data + CurOffset;

	//创建文件路径
	char FilePwd[64];
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("FilePwd = %s\n",FilePwd);

	debug_printf("ZCPTCStruct->usder->ip = %s\n",ZCPTCStruct->user->ip);
	//初始化用户所属
	FILEUser_t FileUser;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,ZCPTCStruct->user->type,ZCPTCStruct->user->ip,ZCPTCStruct->user->port,ZCPTCStruct->user->uartPort);

	//帧数据存文件
	retvals = FRTx_FileFrameRx2K(&FileUser,FilePwd,FrameContent,FrameLen,FrameId);
	if(retvals < 0)
		goto EXCEPTION;
	
	if(retvals == 1)
	{
		if(strncmp(FileName,"config/cls.conf",15) == 0)
		{
			debug_printf("FileName = %s\n",FileName);
			copy_file(ConFigFile,ConFigFile_CPY);
			copy_file(ConFigFile,ConFigFile_Setting);
			copy_file(ConFigFile,ConFigFile_Setting_config);
			copy_file(ConFigFile,ConFigFile_Setting_config_cpy);
		}
		DEBUG_PRINTF;
		chmod(FilePwd,0744);
	}
	
	ZCPTCStruct->Data[0] = 0x01;
	ZCPTCStruct->DataLen = 1;
	return 0;

	EXCEPTION:
		ZCPTCStruct->Data[0] = 0x00;
		ZCPTCStruct->DataLen= 1;
		return -1;
}


static int SWR_FrameTxToUpper2K(ZCPTCStruct_t *ZCPTCStruct)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileNameLenStr[4];
	int FileNameLen = 0;
	char FileName[48];
	char FrameIDStr[4];
	uint32_t FrameID = 0;
	char *FrameContent = NULL;
	char FilePwd[64];
	uint32_t FrameLen = 0;
	
	// 获得文件名长度
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,ZCPTCStruct->Data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//获取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,ZCPTCStruct->Data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//将文件路径中的'\'转化成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//获取文件偏移地址
	Offset += FileNameLen;
	memset(FrameIDStr,0,sizeof(FrameIDStr));
	memcpy(FrameIDStr,ZCPTCStruct->Data + Offset,4);
	FrameID = ((uint8_t)FrameIDStr[0] - 0x30) * 1000 + ((uint8_t)FrameIDStr[1] - 0x30) * 100 + ((uint8_t)FrameIDStr[2] - 0x30) * 10 + ((uint8_t)FrameIDStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x,%d\n",FrameIDStr[0],FrameIDStr[1],FrameIDStr[2],FrameIDStr[3],FrameID);
	//文件内容在一帧返回帧数据中位置
	Offset += 4;
	FrameContent = ZCPTCStruct->Data + Offset + 1;

	//初始化所属用户
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,ZCPTCStruct->user->type,ZCPTCStruct->user->ip,ZCPTCStruct->user->port,ZCPTCStruct->user->uartPort);

	//获取要读取的文件路径
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//开始读文件
	ret = FRTx_FileFrameTx2K(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameID);
	if(ret < 0)
		goto EXCEPTION;

	//组帧回传
	Offset = 0;
	ZCPTCStruct->Data[Offset] = 0x01;
	Offset += 1;
	memcpy(ZCPTCStruct->Data + Offset,FileNameLenStr,3);
	Offset += 3;
	memcpy(ZCPTCStruct->Data + Offset,FileName,FileNameLen);
	Offset += FileNameLen;
	memcpy(ZCPTCStruct->Data + Offset,FrameIDStr,4);
	char *pp = ZCPTCStruct->Data + Offset;
	debug_printf("0x%x,0x%x,0x%x,0x%x  0x%x,0x%x,0x%x,0x%x\n",pp[0],pp[1],pp[2],pp[3],FrameIDStr[0],FrameIDStr[1],FrameIDStr[2],FrameIDStr[3]);
	Offset += 4;
	ZCPTCStruct->DataLen = Offset + FrameLen;
	debug_printf("FrameLen = %d,%d\n",FrameLen,ZCPTCStruct->DataLen);

	return 0;


	EXCEPTION:
		ZCPTCStruct->Data[0] = 0x00;
		ZCPTCStruct->DataLen = 1;
		return -1;
}


static int SWR_FrameRxFrmUpper16K(ZCPTCStruct_t *ZCPTCStruct)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileName[48];
	char FilePwd[64];
	char FileNameLenStr[4];
	int  FileNameLen;
	char *FrameOffsetStr = NULL;
	int  FrameOffset = 0;
	char *FrameContent = NULL;

	int	 FrameLen = 0;

	//取文件名长度
	Offset += 2;
	memcpy(FileNameLenStr,ZCPTCStruct->Data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,ZCPTCStruct->Data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//文件路径'\'转换成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//检查上位机上传过来的文件路径，没有响应文件夹就创建相应的文件夹
	char *p = NULL;
	chdir(sys_dir);
	p = strchr(FileName,'/');
	if(p != NULL)
	{
		DEBUG_PRINTF;
		mkdirs(FileName);
		chdir(sys_dir);
	}
	

	//偏移到文件偏移位置
	Offset += FileNameLen;
	FrameOffsetStr = ZCPTCStruct->Data + Offset;
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];

	debug_printf("FrameOffset = %d\n",FrameOffset);
	//偏移到文件内容位置
	Offset += 4;
	FrameContent = ZCPTCStruct->Data + Offset;

	//帧长度
	FrameLen = ZCPTCStruct->DataLen - Offset;

	#if 0
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEUser_t));
	memcpy(FILEuser.ip,ZCPTCStruct->user->ip,strlen(ZCPTCStruct->user->ip));
	FILEuser.port = 5168;
	FILEuser.userType = 0;
	FILEuser.comx = 0;
	#else
	//初始化所属用户
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,ZCPTCStruct->user->type,ZCPTCStruct->user->ip,ZCPTCStruct->user->port,ZCPTCStruct->user->uartPort);
	#endif

	//获取要读取的文件路径
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);

	ret = FRTx_FileFrameRx16K(&FILEuser,FilePwd,FrameContent,FrameLen,FrameOffset);
	if(ret == 1)
	{
		if(strncmp(FileName,"config/cls.conf",15) == 0)
		{
			debug_printf("FileName = %s\n",FileName);
			copy_file(ConFigFile,ConFigFile_CPY);
			copy_file(ConFigFile,ConFigFile_Setting);
			copy_file(ConFigFile,ConFigFile_Setting_config);
			copy_file(ConFigFile,ConFigFile_Setting_config_cpy);
		}
		DEBUG_PRINTF;
		chmod(FilePwd,0744);
	}

	ZCPTCStruct->Data[0] = 0x01;
	ZCPTCStruct->DataLen = 1;
	return 0;
	
}

static int SWR_FrameTxToUpper16K(ZCPTCStruct_t *ZCPTCStruct)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileNameLenStr[4];
	int FileNameLen = 0;
	char FileName[48];
	char FrameOffsetStr[4];
	uint32_t FrameOffset = 0;
	char *FrameContent = NULL;
	char FilePwd[64];
	uint32_t FrameLen = 0;
	
	// 获得文件名长度
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,ZCPTCStruct->Data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//获取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,ZCPTCStruct->Data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//将文件路径中的'\'转化成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//获取文件偏移地址
	Offset += FileNameLen;
	memset(FrameOffsetStr,0,sizeof(FrameOffsetStr));
	memcpy(FrameOffsetStr,ZCPTCStruct->Data + Offset,4);
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3]);
	//文件内容在一帧返回帧数据中位置
	Offset += 4;
	FrameContent = ZCPTCStruct->Data + Offset + 1;

	//初始化所属用户
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,ZCPTCStruct->user->type,ZCPTCStruct->user->ip,ZCPTCStruct->user->port,ZCPTCStruct->user->uartPort);

	//获取要读取的文件路径
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//开始读文件
	ret = FRTx_FileFrameTx16K(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameOffset);
	if(ret < 0)
		goto EXCEPTION;

	DEBUG_PRINTF;
	//组帧回传
	Offset = 0;
	ZCPTCStruct->Data[Offset] = 0x01;
	Offset += 1;
	memcpy(ZCPTCStruct->Data + Offset,FileNameLenStr,3);
	Offset += 3;
	memcpy(ZCPTCStruct->Data + Offset,FileName,FileNameLen);
	Offset += FileNameLen;
	memcpy(ZCPTCStruct->Data + Offset,FrameOffsetStr,4);
	char *pp = ZCPTCStruct->Data + Offset;
	debug_printf("0x%x,0x%x,0x%x,0x%x  0x%x,0x%x,0x%x,0x%x\n",pp[0],pp[1],pp[2],pp[3],FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3]);
	Offset += 4;
	ZCPTCStruct->DataLen = Offset + FrameLen;
	debug_printf("FrameLen = %d,%d\n",FrameLen,ZCPTCStruct->DataLen);

	return 0;


	EXCEPTION:
		ZCPTCStruct->Data[0] = 0x00;
		ZCPTCStruct->DataLen = 1;
		return -1;
}







int ZC_protocolProcessor(user_t *user,uint8_t *input,uint32_t *inputlen)
{
	int err = -1;
	int output_total_len = 0;
	unsigned short CRC16 = 0;
	unsigned int outlen = 0;
	ZCPTCStruct_t ZCPTCStruct;
	//PROTOCOLStructInit();
	DEBUG_PRINTF;
	/*************************************************************************************/
	//每个协议都可以加这条
		uint8_t vindicate[9] = {0x02,0x39,0x30,0x30,0x30,0x30,0x7E,0x18,0x03};
		uint8_t reply[9] = {0x02,0x39,0x30,0x30,0x30,0x01,0x58,0x6A,0x03};
		if(memcmp(input,vindicate,9)==0)
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

	
	err = prtcl_preparsing(input,*inputlen,FREEdata,&outlen);
	debug_printf("*inputlen = %d,outlen = %d\n",*inputlen,outlen);
	if(err != 0)
	{
		debug_printf("find error when preparsing the recv data\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"prtcl_preparsing failed",strlen("prtcl_preparsing failed"));
		DEBUG_PRINTF;
		goto ERRORDEAL;
	}
	DEBUG_PRINTF;
	err = ParityCheck_CRC16(FREEdata,outlen);
	if(err < 0)
	{
		debug_printf("CRC16 check error\n");	
		return -1;
	}
	DEBUG_PRINTF;
	memset(&ZCPTCStruct,0,sizeof(ZCPTCStruct));
	ZC_ProtocolStrInit(&ZCPTCStruct,user,FREEdata,outlen);
	debug_printf("=================recv data==================\n");
	ZCStructPrintf(&ZCPTCStruct);
	DEBUG_PRINTF;

	//设置IP命令。临时的
	uint16_t setipcmd = 0;
	setipcmd = ZCPTCStruct.CmdCode;
	if(setipcmd == 0x3338)
		ZCPTCStruct.CmdCode = setipcmd;

	switch(ZCPTCStruct.CmdCode)
	{
		case SET_DEV_IP:
			ZC_SetDevIP(&ZCPTCStruct);
			break;
		case GET_DETAIL_STATUS:
			ZC_GetDetailStatus(&ZCPTCStruct);
			break;
		case RECV_FILE_FROMUPPER:
			ZC_RecvFileFromUpper(&ZCPTCStruct);//OK
			break;
		case UPLOAD_FILE_TOUPPER:
			ZC_UploadFileToUpper(&ZCPTCStruct);//OK
			break;
		case SET_PLAY_LIST:
			ZC_SetPlayList(&ZCPTCStruct);//OK
			break;
		case GET_PLAY_CONTENT:
			ZC_GetPlayContent(&ZCPTCStruct);//OK
			break;
		case SET_BRIGHT_MODE:
			ZC_SetBrightMode(&ZCPTCStruct);//OK
			break;
		case SET_BRIGHT_VAL:
			ZC_SetBrightVal(&ZCPTCStruct);//OK
			break;
		case GET_BRIGHTMODE_AND_BRIGHTVAL:
			ZC_GetBrightModeAndVal(&ZCPTCStruct);//OK
			break;
		case SET_CURRENT_TIME:
			ZC_SetCurTime(&ZCPTCStruct);//OK
			break;
		case COMPATIBLEMODE:
			break;
		default:
			return -1;
	}


	
	debug_printf("=================send data==================\n");
	ZCStructPrintf(&ZCPTCStruct);

	ZC_StructToByte(&ZCPTCStruct,FREEdata);
	
	//校验值
	CRC16 = XKCalculateCRC(FREEdata+1,2+ZCPTCStruct.DataLen);
	debug_printf("CRC16 = 0x%x\n",CRC16);
	FREEdata[ZC_PARITY_BYTE_POS(ZCPTCStruct.DataLen) + 0] = (uint8_t)(CRC16>> 8);
	FREEdata[ZC_PARITY_BYTE_POS(ZCPTCStruct.DataLen) + 1] = (uint8_t)(CRC16);

	//检测字节序中是否存在0x02、0x03、0x1B，有则进行相应的转换，此处处理办法是将头尾两个字节踢掉在处理
	DEBUG_PRINTF;
	check_0x02and0x03(FLAG_SEND,FREEdata+1,2+ZCPTCStruct.DataLen+2,input+1,inputlen);
	DEBUG_PRINTF;
	
	output_total_len = *inputlen;
	//输出字节序再加上头尾两个字节
	input[0] 						= 0x02;
	input[output_total_len + 1] 	= 0x03;
	debug_printf("output_total_len = %d,input[%d] = %d\n",output_total_len,output_total_len + 1,input[output_total_len + 1]);
	//所以总长度要+2
	output_total_len += 2;
	*inputlen = output_total_len;

	//用于确认返回的数据是否完整正确
#if 0
	uint32_t i ;
	for(i = 0 ; i < *inputlen ; i++)
		debug_printf("%02x ",input[i]);
	debug_printf("\n");
#endif

	ZCPTCStruct.Data = NULL;
	DEBUG_PRINTF;
	
	return 0;


	
	ERRORDEAL:
		DEBUG_PRINTF;
		debug_printf("memory has been free!\n");
		return -1;
}



