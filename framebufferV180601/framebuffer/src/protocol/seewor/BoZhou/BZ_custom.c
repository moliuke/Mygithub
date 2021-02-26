#include "BZ_custom.h"
#include "../SWR_protocol.h"
#include "../SWR_init.h"
#include "../../PTC_common.h"
#include "../../PTC_FileCopy.h"


int ZC_FrameRxFrmUpper2K(Protocl_t *protocol,unsigned int *len)
{
	int retvals = -1;
	uint16_t CurOffset = 0;
	uint8_t FileNameLen = 0;
	char FileNameLenStr[4];
	char FileName[48];
	
	char *FrameOffsetStr = NULL;
	uint32_t FrameOffset = 0;
	
	uint32_t FrameLen = 0;
	char *FrameContent = NULL;

	//取文件名长度
	CurOffset += 2;
	memset(FileNameLenStr,0,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,protocol->protcmsg.data + CurOffset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//取文件名
	CurOffset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,protocol->protcmsg.data + CurOffset,FileNameLen);
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
	FrameOffsetStr = protocol->protcmsg.data + CurOffset;
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | 
		(uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];
	
	debug_printf("# 0x%x,0x%x,0x%x,0x%x\n",FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3]);
	debug_printf("FrameOffset = %d\n",FrameOffset);

	//帧内容长度
	CurOffset += 4;
	FrameLen = protocol->protcmsg.length - CurOffset;

	//帧内容
	FrameContent = protocol->protcmsg.data + CurOffset;

	//创建文件路径
	char FilePwd[64];
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("FilePwd = %s\n",FilePwd);

	debug_printf("protocol->usermsg->ip = %s\n",protocol->usermsg->ip);
	//初始化用户所属
	FILEUser_t FileUser;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,protocol->usermsg->type,protocol->usermsg->ip,protocol->usermsg->port,protocol->usermsg->uartPort);

	//帧数据存文件
	retvals = FRTx_FileFrameRx(&FileUser,FilePwd,FrameContent,FrameLen,FrameOffset);
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
	
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	return 0;

	EXCEPTION:
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
		return -1;
}


int ZC_FrameTxToUpper2K(Protocl_t *protocol,unsigned int *len)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileNameLenStr[4];
	int FileNameLen = 0;
	char FileName[48];
	char FrameIDStr[4];
	uint32_t FrameID = 0;

	char *FrameOffsetStr = NULL;
	uint32_t FrameOffset = 0;
	char *FrameContent = NULL;
	char FilePwd[64];
	uint32_t FrameLen = 0;
	
	// 获得文件名长度
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,protocol->protcmsg.data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//获取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,protocol->protcmsg.data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//将文件路径中的'\'转化成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//获取文件偏移地址
	Offset += FileNameLen;
	//memset(FrameIDStr,0,sizeof(FrameIDStr));
	//memcpy(FrameIDStr,protocol->protcmsg.data + Offset,4);

	FrameOffsetStr = protocol->protcmsg.data + Offset;
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | 
		(uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];
	
	//FrameID = ((uint8_t)FrameIDStr[0] - 0x30) * 1000 + ((uint8_t)FrameIDStr[1] - 0x30) * 100 + ((uint8_t)FrameIDStr[2] - 0x30) * 10 + ((uint8_t)FrameIDStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x,%d\n",FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3],FrameOffset);
	//文件内容在一帧返回帧数据中位置
	Offset += 4;
	FrameContent = protocol->protcmsg.data + Offset + 1;

	//初始化所属用户
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,protocol->usermsg->type,protocol->usermsg->ip,protocol->usermsg->port,protocol->usermsg->uartPort);

	//获取要读取的文件路径
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//开始读文件
	ret = FRTx_FileFrameTx(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameOffset);
	if(ret < 0)
		goto EXCEPTION;

	//组帧回传
	Offset = 0;
	protocol->protcmsg.data[Offset] = 0x01;
	Offset += 1;
	memcpy(protocol->protcmsg.data + Offset,FileNameLenStr,3);
	Offset += 3;
	memcpy(protocol->protcmsg.data + Offset,FileName,FileNameLen);
	Offset += FileNameLen;
	memcpy(protocol->protcmsg.data + Offset,FrameIDStr,4);
	char *pp = protocol->protcmsg.data + Offset;
	debug_printf("0x%x,0x%x,0x%x,0x%x  0x%x,0x%x,0x%x,0x%x\n",pp[0],pp[1],pp[2],pp[3],FrameIDStr[0],FrameIDStr[1],FrameIDStr[2],FrameIDStr[3]);
	Offset += 4;
	protocol->protcmsg.length = Offset + FrameLen;
	debug_printf("FrameLen = %d,%d\n",FrameLen,protocol->protcmsg.length);

	return 0;


	EXCEPTION:
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
		return -1;
}


char *BoZhouProject(void)
{
	return "BoZhou";
}


#if 0
int _ProtocolRelation(void *arg1,void *arg2)
{
	projectstr = BoZhouProject;
	PROTOCOLStruct.FileFrameRX2K 		= ZC_FrameRxFrmUpper2K;
	PROTOCOLStruct.FileFrameTX2K		= ZC_FrameTxToUpper2K;
}

void _ProtocolRelationDestroy(void)
{
	
}
#endif



