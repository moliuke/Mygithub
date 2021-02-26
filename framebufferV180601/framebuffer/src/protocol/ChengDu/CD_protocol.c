#include "CD_protocol.h"
#include "../PTC_common.h"
#include "../PTC_FileRxTx.h"
#include "../../Hardware/Data_pool.h"
#include "content.h"
#include "aes/aes_interf.h"
#include "CD_timer.h"
#include "../PTC_init.h"
#include "CD_charparse.h"
#include <sys/socket.h>
#include "../../Hardware/HW3G_RXTX.h"
#include "../../Hardware/HW2G_400.h"
#include "conf.h"


static pthread_once_t init_create = PTHREAD_ONCE_INIT; 

static void PROTOCOLInterfInit(void)
{
	/*1、别的接口的预留位置*/

	/*2、分配反转义所需要分配的固定内存块*/
	COM_PROTOCOLMemoryMalloc();
}
static inline void PROTOCOLStructInit(void)
{
	pthread_once(&init_create,PROTOCOLInterfInit);
}

static void CDStructPrintf(CDPTCStruct_t *CDStruct)
{
	int i = 0; 
	debug_printf(
		"ip:				%s\n"
		"port:				%d\n"
		"ack:				%d\n"
		"fd:				%d\n"
		"startbyte:			0x%x\n"
		"DevAddr:			0x%x\n"
		"CmdCode:			0x%x\n",
		CDStruct->user->ip,
		CDStruct->user->port,
		CDStruct->user->ackFlag,
		CDStruct->user->fd,
		CDStruct->StartByte,
		CDStruct->DevAddr,
		CDStruct->CmdCode);	
	debug_printf("data:				");
	for(i = 0 ; i < CDStruct->DataLen ; i ++)
	{
		debug_printf("%02x ",CDStruct->Data[i]);
	}

	debug_printf("\n"
		"CRC16:				0x%x\n"
		"EndByte:			0x%x\n"
		"length:			0x%x\n",
		CDStruct->CRC16,
		CDStruct->EndByte,
		CDStruct->DataLen
		);
	debug_printf("=====================================\n\n");
	
}

static void CD_ProtocolStrInit(CDPTCStruct_t *CDPTCStruct,user_t *user,uint8_t *Data,uint32_t Len)
{
	CDPTCStruct->user		= user;
	CDPTCStruct->StartByte 	= Data[0];
	CDPTCStruct->DevAddr	= Data[1] << 8 | Data[2];
	CDPTCStruct->CmdCode	= Data[3] << 8 | Data[4];
	CDPTCStruct->CRC16		= Data[Len - 3] << 8 | Data[Len - 2];
	CDPTCStruct->DataLen	= Len - 8;
	CDPTCStruct->Data		= Data + 5;
	CDPTCStruct->EndByte	= Data[Len - 1];
}

static int CD_StructToByte(CDPTCStruct_t *CDPTCStruct,uint8_t *Output)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	
	if(CDPTCStruct == NULL || Output == NULL)
		return -1;
	
	Output[CD_START_BYTE_POS] = CDPTCStruct->StartByte;
	Output[CD_DEVID_BYTE_POS + 0] = (uint8_t)(CDPTCStruct->DevAddr >> 8);
	Output[CD_DEVID_BYTE_POS + 1] = (uint8_t)(CDPTCStruct->DevAddr);

	Output[CD_CMD_BYTE_POS + 0] = (uint8_t)(CDPTCStruct->CmdCode >> 8);
	Output[CD_CMD_BYTE_POS + 1] = (uint8_t)(CDPTCStruct->CmdCode);

	for(i = 0 ; i < CDPTCStruct->DataLen; i ++)
	{
		//debug_printf("0x%x\t",protocol->protcmsg.data[i]);
		Output[CD_DATAS_BYTE_POS + i]	= CDPTCStruct->Data[i];
	}

	Output[CD_PARITY_BYTE_POS(CDPTCStruct->DataLen) + 0] = (uint8_t)(CDPTCStruct->CRC16 >> 8);
	Output[CD_PARITY_BYTE_POS(CDPTCStruct->DataLen) + 1] = (uint8_t)(CDPTCStruct->CRC16);
	Output[CD_END_BYTE_POS(CDPTCStruct->DataLen)] = CDPTCStruct->EndByte;
}



static uint8_t CD_HexToByte(uint8_t HexData)
{
	if(HexData >= 0x0 && HexData <= 0x9)
		return HexData + 0x30;

	if(HexData >= 0xa && HexData <= 0xf)
		return HexData + 0x37;
}



static int CD_SetDevIP(CDPTCStruct_t *CDPTCStruct)
{
	unsigned char dev_ip[24] , dev_mask[24] , dev_gw[24] , dev_port[8];
	unsigned char *ip = NULL,*mask = NULL,*gw = NULL,*port = NULL;
	unsigned short _port = 0;

	char conf_file[64];
	memset(conf_file,0x00,sizeof(conf_file));
	sprintf(conf_file,"%s/cls.conf",conf_dir);
	
	ip 	 = CDPTCStruct->Data + 6;
	mask = CDPTCStruct->Data + 10;
	gw   = CDPTCStruct->Data + 14;
	port = CDPTCStruct->Data + 24;

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
	

	CDPTCStruct->Data[2] = 0x31;
	CDPTCStruct->DataLen = 3;

	char ip_port[24];
	char logmsg[96];
	memset(ip_port,0,sizeof(ip_port));
	sprintf(ip_port,"%s:%d",CDPTCStruct->user->ip,CDPTCStruct->user->port);
	sprintf(logmsg,"cmd:%x setnet:ip: %s netmask:%s gateway:%s port:%s",
		CDPTCStruct->CmdCode,dev_ip,dev_mask,dev_gw,dev_port);
	debug_printf("strlen(logmsg) = %d\n",strlen(logmsg));
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	return 0;
}



static int CD_GetDetailStatus(CDPTCStruct_t *CDPTCStruct)
{
	//被置1的位表示对应的设备状态有故障
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
	//status = 0x31;
	//DetailStatus = (status == 0x30) ? (DetailStatus | 1 << 3) : (DetailStatus);
	//debug_printf("status = 0x%x\n",status);
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
	CDPTCStruct->Data[0] = CD_HexToByte(HexData);

	HexData = (uint8_t)((0x0F00 & DetailStatus) >> 8);
	CDPTCStruct->Data[1] = CD_HexToByte(HexData);

	HexData = (uint8_t)((0x00F0 & DetailStatus) >> 4);
	CDPTCStruct->Data[2] = CD_HexToByte(HexData);

	HexData = (uint8_t)((0x000F & DetailStatus) >> 0);
	CDPTCStruct->Data[3] = CD_HexToByte(HexData);

	CDPTCStruct->DataLen = 4;
	return 0;
}



static int CD_RecvFileFromUpper(CDPTCStruct_t *CDPTCStruct)
{
	int ret = -1;
	DEBUG_PRINTF;
	uint8_t *Flag0x2B = NULL;
	uint8_t *Offset = CDPTCStruct->Data;
	char *filedata = NULL;
	uint32_t filedataLen = 0;
	char filename[24];
	uint8_t filenameLen = 0;
	uint32_t frameOffset = 0;

	static int fileLen = 0;

	FILEUser_t FileUser;

	//取文件名
	Flag0x2B = strchr(Offset,0x2b);
	if(Flag0x2B == NULL)
	{
		debug_printf("recv file data : bad frame data");
		goto EXCEPTION;
	}
	filenameLen = Flag0x2B - Offset;
	memset(filename,0,sizeof(filename));
	memcpy(filename,Offset,filenameLen);
	debug_printf("filename = %s,filenameLen = %d\n",filename,filenameLen);

	dir_wintolinux(filename);
	Dir_LetterBtoL(filename);

	//文件偏移
	Offset = Flag0x2B + 1;
	frameOffset = (uint8_t)Offset[0] << 24 | (uint8_t)Offset[1] << 16 | (uint8_t)Offset[2] << 8 | (uint8_t)Offset[3];
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",Offset[0],Offset[1],Offset[2],Offset[3]);
	debug_printf("frameOffset = %d\n",frameOffset);

	//文件数据
	Offset += 4;
	filedata = Offset;
	filedataLen = CDPTCStruct->DataLen - filenameLen - 1 - 4;
	debug_printf("filedataLen = %d\n",filedataLen);

	fileLen += filedataLen;
	debug_printf("filedataLen = %d,fileLen = %d\n",filedataLen,fileLen);

	//文件存放路径
	char filePath[64];
	memset(filePath,0,sizeof(filePath));


	uint8_t *pp = NULL;
	uint8_t filenumber[4];
	pp = strchr(filename,'/');
	if(pp != NULL)
	{
		memset(filenumber,0,sizeof(filenumber));
		memcpy(filenumber,pp+1,3);
		filenumber[3] = '\0';
	}
	
	//组装图片与播放表的存放路径
	if(strstr(filename,"png") != NULL)
		sprintf(filePath,"%s/%s.png",png_dir,filenumber);
	else if(strstr(filename,"bmp") != NULL)
		sprintf(filePath,"%s/%s.bmp",bmp_dir,filenumber);
	else if(strstr(filename,"jpg") != NULL)
		sprintf(filePath,"%s/%s.jpg",jpg_dir,filenumber);
	else if(strstr(filename,"gif") != NULL)
		sprintf(filePath,"%s/%s.gif",gif_dir,filenumber);
	else if(strstr(filename,"playlist") != NULL )
		sprintf(filePath,"%s/%s.lst",list_dir_1,filenumber);
	else
		goto EXCEPTION;
	
	filePath[strlen(filePath)] = '\0';
	debug_printf("filePath = %s\n",filePath);
	
	DEBUG_PRINTF;
	//文件帧数据处理
	debug_printf("CDPTCStruct->user->ip = %s\n",CDPTCStruct->user->ip);
	memset(&FileUser,0,sizeof(FILEUser_t));
	FRTx_FileUserInit(&FileUser,CDPTCStruct->user->type,CDPTCStruct->user->ip,CDPTCStruct->user->port,CDPTCStruct->user->uartPort);
	DEBUG_PRINTF;
	if((ret = FRTx_FileFrameRx20K(&FileUser,filePath,filedata,filedataLen,frameOffset)) < 0)
		goto EXCEPTION;
	DEBUG_PRINTF;


	CDPTCStruct->Data[0] 	= 0x30; 	//0表示成功
	CDPTCStruct->DataLen	= 1;
	DEBUG_PRINTF;
	return 0;

	EXCEPTION:
		CDPTCStruct->Data[0] = 0x31;	//非0表示失败
		CDPTCStruct->DataLen  = 1;
		DEBUG_PRINTF;
		return -1;
		
}



static int CD_UploadFileToUpper(CDPTCStruct_t *CDPTCStruct)
{
	char FileName[24];
	uint8_t FilenameLen = 0;
	char filePath[64];
	uint8_t *FrameOffsetStr = NULL;
	uint32_t frameOffset = 0;
	FILEUser_t FileUser;
	char *dataPoint = CDPTCStruct->Data;
	char *OffsetPos = NULL;
	char *Flag0x2B = NULL;
	DEBUG_PRINTF;
	//取文件名并确定文件所属路径
	memset(FileName,0,sizeof(FileName));
	memset(filePath,0,sizeof(filePath));
	debug_printf("dataPoint = %s\n",dataPoint);
	DEBUG_PRINTF;
	
	FilenameLen = CDPTCStruct->DataLen - 4;
	memcpy(FileName,dataPoint,FilenameLen);
	debug_printf("filename = %s\n",FileName);
	DEBUG_PRINTF;

	//将文件路径中的'\'转化成'/'并将存在的大写字母转换成小写字母
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);

	uint8_t *pp = NULL;
	uint8_t filenumber[4];
	pp = strchr(FileName,'/');
	if(pp != NULL)
	{
		memset(filenumber,0,sizeof(filenumber));
		memcpy(filenumber,pp+1,3);
		filenumber[3] = '\0';
	}
	
	//组装图片与播放表的存放路径
	if(strstr(FileName,"png") != NULL)
		sprintf(filePath,"%s/%s.png",png_dir,filenumber);
	else if(strstr(FileName,"bmp") != NULL)
		sprintf(filePath,"%s/%s.bmp",bmp_dir,filenumber);
	else if(strstr(FileName,"jpg") != NULL)
		sprintf(filePath,"%s/%s.jpg",jpg_dir,filenumber);
	else if(strstr(FileName,"gif") != NULL)
		sprintf(filePath,"%s/%s.gif",gif_dir,filenumber);
	else if(strstr(FileName,"playlist") != NULL )
		sprintf(filePath,"%s/%s.lst",list_dir_1,filenumber);
	else
		goto EXCEPTION;

	debug_printf("filePath = %s\n",filePath);

	//读取文件的路径
	//sprintf(filePath,"%s/%s",sys_dir,FileName);

	//获取偏移地址
	FrameOffsetStr = dataPoint + FilenameLen;
	frameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];
	debug_printf("frameOffset = %d\n",frameOffset);

	
	//文件数据处理
	char *frameData = CDPTCStruct->Data;
	uint32_t frameDataLen = 0;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,CDPTCStruct->user->type,CDPTCStruct->user->ip,CDPTCStruct->user->port,CDPTCStruct->user->uartPort);
	DEBUG_PRINTF;
	if(FRTx_FileFrameTx20K(&FileUser,filePath,frameData,&frameDataLen,frameOffset) < 0)
		goto EXCEPTION;
	DEBUG_PRINTF;

	//ZCPTCStruct->Data[0] = 0x30; 	//0表示成功
	CDPTCStruct->DataLen = frameDataLen;
	debug_printf("frameDataLen = %d\n",frameDataLen);

	return 0;

	EXCEPTION:
		CDPTCStruct->Data[0] = 0x31;	//非0表示失败
		CDPTCStruct->DataLen  = 1;
		return -1;
}


static int CD_SetPlayList(CDPTCStruct_t *CDPTCStruct)
{
	debug_printf("preset play list\n");
	char playlist[8];
	char ListName[64];
	
	memset(playlist,0,sizeof(playlist));
	memcpy(playlist,CDPTCStruct->Data,3);
	
	sprintf(ListName,"%s/%s.lst",list_dir_1,playlist);
	
	ListName[strlen(ListName)] = '\0';
	debug_printf("ListName = %s\n",ListName);

	char curlist[8];
	memset(curlist,0,sizeof(curlist));
	sprintf(curlist,"%s.lst",playlist);
	curlist[7] = '\0';
	DP_SetCurPlayList(curlist,7);

	//检查该播放表是否存在
	if(access(ListName,F_OK) < 0)
	{
		debug_printf("The list [%s]file is not exist!\n",ListName);
		goto EXCEPTION;
	}

	//复制位系统目录下的play.lst
	if(COM_CopyFile(ListName,CD_PLAY_LST) < 0)
		goto EXCEPTION;

	//解析播放表
	DEBUG_PRINTF;
	CD_Lstparsing(&content,ListName);
	
	uint8_t CurPLst[8]; 
	uint8_t Len;
	sprintf(CurPLst,"%s.lst",playlist);
	Len = strlen(CurPLst);

	//记录当前设置的播放表并写到文件中
	debug_printf("playlist = %s,CurPLst = %s\n",playlist,CurPLst);
	//DP_CurPlayList(OPS_MODE_SET,CurPLst,&Len);
	DP_SetCurPlayList(CurPLst,Len);
	DP_GetCurPlayList(Playlst,&Len);
	conf_file_write(ConFigFile,"playlist","list",CurPLst);

	CDPTCStruct->Data[0] = 0x30;
	CDPTCStruct->DataLen= 1;
	DEBUG_PRINTF;
	return 0;

	EXCEPTION:
		DEBUG_PRINTF;
		CDPTCStruct->Data[0] = 0x31;//0xff;
		CDPTCStruct->DataLen	 = 1;
		return -1;
	
}


static int CD_GetPlayContent(CDPTCStruct_t *CDPTCStruct)
{

	CDCurplay_t CDCurplay;
	memset(&CDCurplay,0,sizeof(CDCurplay_t));
	CDGetCurPlaying(&CDCurplay);

	//当前播放内容在播放表中的序号
	CDPTCStruct->Data[0] = CDCurplay.order / 100 + 0x30;
	CDPTCStruct->Data[1] = CDCurplay.order / 10 % 10 + 0x30;
	CDPTCStruct->Data[2] = CDCurplay.order % 10 + 0x30;

	//停留时间
	CDPTCStruct->Data[3] = CDCurplay.stoptime / 10000 + 0x30;
	CDPTCStruct->Data[4] = CDCurplay.stoptime / 1000 % 10 + 0x30;
	CDPTCStruct->Data[5] = CDCurplay.stoptime / 100 % 10 + 0x30;
	CDPTCStruct->Data[6] = CDCurplay.stoptime % 100 / 10 + 0x30;
	CDPTCStruct->Data[7] = CDCurplay.stoptime % 100 % 10 + 0x30;

	//出字方式
	CDPTCStruct->Data[8] = CDCurplay.effectin / 10 + 0x30;
	CDPTCStruct->Data[9] = CDCurplay.effectin % 10 + 0x30;

	//出字速度
	CDPTCStruct->Data[10] = CDCurplay.inspeed / 10000 + 0x30;
	CDPTCStruct->Data[11] = CDCurplay.inspeed / 1000 % 10 + 0x30;
	CDPTCStruct->Data[12] = CDCurplay.inspeed / 100 % 10 + 0x30;
	CDPTCStruct->Data[13] = CDCurplay.inspeed % 100 / 10 + 0x30;
	CDPTCStruct->Data[14] = CDCurplay.inspeed % 100 % 10 + 0x30;

	//显示内容，图片为地址信息，文本为发布内容
	memcpy(CDPTCStruct->Data + 15,CDCurplay.playstr,CDCurplay.strLen);
	
	CDPTCStruct->DataLen = 15 + CDCurplay.strLen;

	return 0;
}

static int CD_SetBrightMode(CDPTCStruct_t *CDPTCStruct)
{
	uint8_t BrightMode = 0;
	
	BrightMode = CDPTCStruct->Data[0];

	BrightMode = (BrightMode == CD_BRIGHT_HAND) ? BRIGHT_HAND : BRIGHT_AUTO;
	
	//DP_BrightAndMode(OPS_MODE_SET,&BrightMode,NULL,NULL,NULL);
	DP_SetBrightMode(BrightMode);

	if(BrightMode == BRIGHT_HAND)
		conf_file_write(ConFigFile,"brightmode","mode","31");

	if(BrightMode == BRIGHT_AUTO)
		conf_file_write(ConFigFile,"brightmode","mode","30");
	
	CDPTCStruct->Data[0] = 0x30;//0xff;
	CDPTCStruct->DataLen	 = 1;
	
	return 0;
}

static int CD_SetBrightVal(CDPTCStruct_t *CDPTCStruct)
{
	DEBUG_PRINTF;
	uint8_t bright_mode = 0;
	uint8_t bright_max = 0,bright_min = 0;
	uint8_t R_bright = 0,G_bright = 0,B_bright = 0;
	
	uint16_t width,height,count;
	
	DP_GetBrightMode(&bright_mode);
	debug_printf("bright_mode = 0x%x\n",bright_mode);

	//0表示自动调节方式，无需设置数值
	if(bright_mode == BRIGHT_AUTO)
		goto EXCEPTION;
	
	DEBUG_PRINTF;
	//三基色亮度值
	R_bright = (CDPTCStruct->Data[0]-0x30)*10+(CDPTCStruct->Data[1]-0x30);
	G_bright = (CDPTCStruct->Data[2]-0x30)*10+(CDPTCStruct->Data[3]-0x30);
	B_bright = (CDPTCStruct->Data[4]-0x30)*10+(CDPTCStruct->Data[5]-0x30);
	debug_printf("R_bright = %d,G_bright = %d,B_bright = %d,bright_min = %d,bright_max = %d\n",R_bright,G_bright,B_bright,bright_min,bright_max);

	//亮度分成32级(0-31)
	if(R_bright < 0) R_bright = 0;
	if(R_bright > 31)R_bright = 31;
	DP_SaveBrightVals(R_bright);

	//针对扫描版设置亮度值
	HW2G400_SETLEDbright(R_bright);

	//针对TXRX板设置亮度值
	uint8_t Bmax,Bmin;
	float div = 0.0,fbright = 0.0;
	DP_GetBrightRange(&Bmax,&Bmin);
	div = (Bmax - Bmin) / (float)32;
	fbright = R_bright * div + Bmin;
	R_bright = (fbright = (uint8_t)fbright > 0.5) ? ((uint8_t)fbright + 1) : ((uint8_t)fbright);
	R_bright = (R_bright <= Bmin) ? Bmin : R_bright;
	R_bright = (R_bright >= Bmax) ? Bmax : R_bright;
	debug_printf("R_bright = %d,G_bright = %d,B_bright = %d\n",R_bright,G_bright,B_bright);
	Set_LEDBright(R_bright);

	//写入配置文件
	uint8_t BrightVals[4];
	memset(BrightVals,0,4);
	sprintf(BrightVals,"%d",R_bright);
	conf_file_write(ConFigFile,"brightmode","bright",BrightVals);

	CDPTCStruct->Data[0] = 0x30; 
	CDPTCStruct->DataLen = 1;
	return 0;

	EXCEPTION:
		CDPTCStruct->Data[0] = 0x30; 
		CDPTCStruct->DataLen = 1;
		return 0;
		
}

static int CD_GetBrightModeAndVal(CDPTCStruct_t *CDPTCStruct)
{
	uint8_t BrightMode;
	uint8_t BrightVals;
	
	//DP_BrightAndMode(OPS_MODE_GET,&BrightMode,&BrightVals,NULL,NULL);
	DP_GetBrightMode(&BrightMode);
	DP_ReadBrightVals(&BrightVals);

	CDPTCStruct->Data[0] = BrightMode;
	CDPTCStruct->Data[1] = BrightVals / 10 + 0x30;
	CDPTCStruct->Data[2] = BrightVals % 10 + 0x30;

	debug_printf("BrightMode = %d,BrightVals = %d\n",BrightMode,BrightVals);

	CDPTCStruct->DataLen = 3;
	return 0;
}

static int CD_SetVirtConnect(CDPTCStruct_t *CDPTCStruct)
{
	uint32_t IntervTime = (CDPTCStruct->Data[0] - 0x30) * 1000 
						+ (CDPTCStruct->Data[1] - 0x30) * 100
						+ (CDPTCStruct->Data[2] - 0x30) * 10
						+ (CDPTCStruct->Data[3] - 0x30);
	DP_SetIntervTime(IntervTime * 60);

	CDPTCStruct->Data[0] = 0x30;
	CDPTCStruct->DataLen = 1;
	return 0;
}

static int CD_GetVirtConnect(CDPTCStruct_t *CDPTCStruct)
{
	uint32_t IntervTime = 0;
	
	DP_GetIntervTime(&IntervTime);
	IntervTime = IntervTime / 60;
	CDPTCStruct->Data[0] = (IntervTime / 1000 + 0x30);
	CDPTCStruct->Data[1] = (IntervTime / 100 % 10 + 0x30);
	CDPTCStruct->Data[2] = (IntervTime % 100 / 10 + 0x30);
	CDPTCStruct->Data[3] = (IntervTime % 10 + 0x30);
	
	CDPTCStruct->DataLen = 4;
	return 0;
}


static uint8_t AES_data[1024 * 24];
int CD_protocol_processor(user_t *user,uint8_t *input,uint32_t *inputlen)
{
	int i = 0;
	uint16_t aesLen = 0;
	int err = -1;
	int output_total_len = 0;
	unsigned short CRC16 = 0;
	unsigned int outlen = 0;
	CDPTCStruct_t CDPTCStruct;
	//PROTOCOLStructInit();
	DEBUG_PRINTF;


	/*************************************************************************************/
	//每个协议都可以加这条,进入维护模式
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
#if 0
	int ii = 0;
	unsigned short enLen = 0; 
	char Send[36] = {0x02,0x30,0x30};
	char enc[16] = {0x02,0x30,0x30,0x31,0x30,0x30};
	AES_ECB_encrypt(enc + 3,3,Send + 3,&enLen);

	CRC16 = XKCalculateCRC(Send+1,2+enLen);
	debug_printf("priorty = 0x%x\n",CRC16);
	
	for(ii = 0 ; ii < enLen + 3 ; ii++)
		debug_printf("0x%x ",(uint8_t)Send[ii]);
	debug_printf("----------\n\n");

	char DD[16];
	unsigned short LL = 0;
	AES_ECB_decrypt(Send + 3,16,DD,&LL,1);
	for(ii = 0 ; ii < LL ; ii++)
		debug_printf("0x%x, ",DD[ii]);
	debug_printf("\n\n");
	
#endif

	err = prtcl_preparsing(input,*inputlen,FREEdata,&outlen);
	debug_printf("*inputlen = %d,outlen = %d\n",*inputlen,outlen);
	if(err != 0)
	{
		debug_printf("find error when preparsing the recv data\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"prtcl_preparsing failed",strlen("prtcl_preparsing failed"));

		goto ERRORDEAL;
	}

	err = ParityCheck_CRC16(FREEdata,outlen);
	if(err < 0)
	{
		debug_printf("CRC16 check error\n");	
		return -1;
	}

	ptc_debug_printf("outlen = %d\n",outlen);
	AES_ECB_decrypt(FREEdata + 3,16,AES_data + 3,&aesLen,1);
	debug_printf("AES_data[3] = 0x%x,AES_data[4] = 0x%x\n",AES_data[3],AES_data[4]);
	
	AES_ECB_decrypt(FREEdata + 3,outlen-6,AES_data + 3,&aesLen,1);
	debug_printf("aesLen = %d\n",aesLen);
	PTC_DEBUG_PRINTF;


	//aes_de_char(FREEdata + 1,AES_data + 1,outlen-4,&aesLen);
	
	AES_data[0] = 0x02;
	AES_data[1] = 0x30;
	AES_data[2] = 0x30;
	AES_data[aesLen + 3 + 0] = FREEdata[outlen-3];
	AES_data[aesLen + 3 + 1] = FREEdata[outlen-2];
	AES_data[aesLen + 3 + 2] = 0x03;
	
	DEBUG_PRINTF;
	memset(&CDPTCStruct,0,sizeof(CDPTCStruct));
	CD_ProtocolStrInit(&CDPTCStruct,user,AES_data,aesLen + 6);
	ptc_debug_printf("=================recv data==================\n");
	ptc_debug_printf("aesLen = %d\n",aesLen);
	CDStructPrintf(&CDPTCStruct);
	DEBUG_PRINTF;

	//设置IP命令。临时的
	uint16_t setipcmd = 0;
	setipcmd = CDPTCStruct.CmdCode;
	if(setipcmd == 0x3338)
		CDPTCStruct.CmdCode = setipcmd;

	//每来一个数据包就对定时器清0一次，通信间隔时间也将被清0
	CD_ClearTimer();


	switch(CDPTCStruct.CmdCode)
	{
		case CD_SET_DEV_IP:
			CD_SetDevIP(&CDPTCStruct);
			break;

		case CD_GET_DEV_STATUS:
			CD_GetDetailStatus(&CDPTCStruct);
			break;
			
		case CD_RX_FILE:
			CD_RecvFileFromUpper(&CDPTCStruct);
			return 0;
			//break;

		case CD_TX_FILE:
			CD_UploadFileToUpper(&CDPTCStruct);
			break;

		case CD_SET_PLAY_LIST:
			CD_SetPlayList(&CDPTCStruct);
			break;

		case CD_GET_PLAY_CONTENT:
			CD_GetPlayContent(&CDPTCStruct);
			break;

		case CD_SET_BRIGHT_MODE:
			DEBUG_PRINTF;
			CD_SetBrightMode(&CDPTCStruct);
			break;

		case CD_SET_BRIGHT_VAL:
			CD_SetBrightVal(&CDPTCStruct);
			break;

		case CD_GET_BRIGHT_MODE_VAL:
			CD_GetBrightModeAndVal(&CDPTCStruct);
			break;
		case CD_SET_VIRT_CONNECT:
			CD_SetVirtConnect(&CDPTCStruct);
			break;
		case CD_GET_VIRT_CONNECT:
			CD_GetVirtConnect(&CDPTCStruct);
			break;
		default:
			break;
	}
	debug_printf("=================send data==================\n");
	CDStructPrintf(&CDPTCStruct);

	CD_StructToByte(&CDPTCStruct,FREEdata);
	DEBUG_PRINTF;
	//加密
	unsigned short encryptLen = 0;
	AES_ECB_encrypt(FREEdata + 3,CDPTCStruct.DataLen + 2,AES_data + 3,&encryptLen);
	AES_data[0] = 0x02;
	AES_data[1] = 0x30;
	AES_data[2] = 0x30;
	
	//校验值
	CRC16 = XKCalculateCRC(AES_data+1,2+encryptLen);
	ptc_debug_printf("CRC16 = 0x%x\n",CRC16);
	AES_data[encryptLen + 3 + 0] = (uint8_t)(CRC16>> 8);
	AES_data[encryptLen + 3 + 1] = (uint8_t)(CRC16);

	//检测字节序中是否存在0x02、0x03、0x1B，有则进行相应的转换，此处处理办法是将头尾两个字节踢掉在处理
	check_0x02and0x03(FLAG_SEND,AES_data+1,2+encryptLen+2,input+1,inputlen);
	PTC_DEBUG_PRINTF;
	
	output_total_len = *inputlen;
	//输出字节序再加上头尾两个字节
	input[0]						= 0x02;
	input[output_total_len + 1] 	= 0x03;
	debug_printf("output_total_len = %d,input[%d] = %d\n",output_total_len,output_total_len + 1,input[output_total_len + 1]);
	//所以总长度要+2
	output_total_len += 2;
	*inputlen = output_total_len;

	//用于确认返回的数据是否完整正确
#if 1
	for(i = 0 ; i < *inputlen ; i++)
		ptc_debug_printf("0x%x ",input[i]);
	ptc_debug_printf("\n");
#endif

#if 0
		uint8_t test1[] = {0x02,0x30,0x30,0x31,0x30,0x30,0x00,0x00,0x03};
		uint8_t test2[] = {0x02,0x30,0x30,0x30};
		//debug_printf("CRC = 0X%x\n",XKCalculateCRC(test+1,5));
		AES_ECB_encrypt(test2 + 3,1,AES_data + 3,&aesLen);
		AES_data[0] = 0x02;
		AES_data[1] = 0x30;
		AES_data[2] = 0x30;
		
		//校验值
		CRC16 = XKCalculateCRC(AES_data+1,2+aesLen);
		debug_printf("aesLen = %d,CRC16 = 0x%x\n",aesLen,CRC16);
		AES_data[aesLen + 3 + 0] = (uint8_t)(CRC16>> 8);
		AES_data[aesLen + 3 + 1] = (uint8_t)(CRC16);
		AES_data[aesLen + 3 + 2] = 0x03;
		for(i = 0 ; i < aesLen + 6 ; i++)
			debug_printf("0x%x ",AES_data[i]);
		debug_printf("\n");
	
#endif
	

	CDPTCStruct.Data = NULL;
	PTC_DEBUG_PRINTF;
	
	return 0;
	
	ERRORDEAL:
		PTC_DEBUG_PRINTF;
		debug_printf("memory has been free!\n");
		return -1;
	
}


