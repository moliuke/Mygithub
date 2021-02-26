#include <stdio.h>

#include "PTC_FileRxTx.h"

#define FILE_BUSY	0x01
#define FILE_FREE	0x00



void FILE_GetTmpFileName(char *GetName)
{
	time_t timer;
	struct tm* t_tm; 
	
	time(&timer);	
	t_tm = localtime(&timer);
	
	debug_printf("Rx TIME : %02d%02d%02d\n",t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);	

	sprintf(GetName,"%02d%02d%02d",t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);
	GetName[strlen(GetName)] = '\0';
	debug_printf("GetName = %s\n",GetName);
}




//最多容许20个用户同时传输文件
static FILERTXStruct_t *FileUsr[FILE_USER_MAXNUM] = {NULL};

static int FRTx_FileUserInsert(FILERTXStruct_t *FILERTXStruct)
{
	uint8_t i = 0;
	for(i = 0 ; i < FILE_USER_MAXNUM ; i++)
	{
		if(FileUsr[i] == NULL)
			break;
	}
	
	if(i == FILE_USER_MAXNUM)
		return -1;
	debug_printf("FILERTXStruct->fileUser.ip = %s\n",FILERTXStruct->fileUser.ip);
	FileUsr[i] = FILERTXStruct;
	return 0;
}

static void FRTx_FileUserRemove(FILERTXStruct_t *FILERTXStruct)
{
	uint8_t i = 0; 
	for(i = 0 ; i < FILE_USER_MAXNUM ; i++)
	{
		if(FileUsr[i] == FILERTXStruct)
		{
			free(FILERTXStruct);
			FileUsr[i] = NULL;
		}
	}
}

static FILERTXStruct_t *FRTx_FileUserFind(FILEUser_t *FileUser,char *fileName)
{
	uint8_t i = 0;
	if(FileUser == NULL || fileName == NULL)
		return NULL;
	for(i = 0 ; i < FILE_USER_MAXNUM ; i++)
	{
		if(FileUsr[i] == NULL || FileUsr[i]->busyFlag == FILE_FREE)
			continue;

		DEBUG_PRINTF;
		
		//串口类型的用户，暂预留
		if(FileUsr[i]->fileUser.userType == USERTYPE_UART)
		{
			debug_printf("This is a uart user!\n");
			continue;
		}
		
		//根据IP来查询
		if(strncmp(FileUsr[i]->fileUser.ip,FileUser->ip,strlen(FileUser->ip)) != 0)
		{
			debug_printf("##FileUsr[i]->fileUser.ip = %s,FileUser->ip = %s\n",FileUsr[i]->fileUser.ip,FileUser->ip);
			continue;
		}
		
		//IP一致则查询文件名是否一致
		if(strncmp(FileUsr[i]->fileName,fileName,strlen(fileName)) != 0)
		{
			fclose(FileUsr[i]->Fp);
			free(FileUsr[i]);
			FileUsr[i] = NULL;
			return NULL;
		}
		return FileUsr[i];
	}

	return NULL;
}


void FRTx_FileUserInit(FILEUser_t *FileUser,uint8_t userType,char *ip,uint32_t port,uint8_t comx)
{
	FileUser->userType	= 0; // userType;
	debug_printf("ip = %s\n",ip);
	if(userType == USERTYPE_NET)
	{
		memcpy(FileUser->ip,ip,strlen(ip));
		FileUser->port = port;
		return ;
	}

	FileUser->comx = comx;
	return ;
}

//获取文件所在的路径
static void Get_FilePwd(char *fileAPath,char *fileDir)
{
	char *pathStr = fileAPath;
	char *charp = NULL;
	char *cp = NULL;
	uint8_t fileDirLen = 0;
	while((charp = strchr(pathStr,'/')) != NULL)
	{
		cp = charp;
		pathStr = charp + 1;
	}

	fileDirLen = cp - fileAPath;
	strncpy(fileDir,fileAPath,fileDirLen);
	debug_printf("fileDir = %s\n",fileDir);
}
static int FRTx_FILERTXStructInit(FILERTXStruct_t *FILERTXStruct,FILEUser_t *FileUser,char *fileName)
{
	char filePWD[64];
	char TmpFile[24];
	struct stat statbuf;
	FILERTXStruct->busyFlag		= FILE_BUSY;
	FILERTXStruct->Offset		= 0;
	FILERTXStruct->FrameId		= 0;
	DEBUG_PRINTF;
	memcpy(FILERTXStruct->fileName,fileName,strlen(fileName));
	memcpy(&FILERTXStruct->fileUser,FileUser,sizeof(FILEUser_t));
	debug_printf("FileUser->ip = %s\n",FileUser->ip);
	DEBUG_PRINTF;
	if(FILERTXStruct->RWflag == RWFLAG_WRITE)
	{
		DEBUG_PRINTF;
		memset(filePWD,0,sizeof(filePWD));
		memset(TmpFile,0,sizeof(TmpFile));
		Get_FilePwd(fileName,filePWD);
		FILE_GetTmpFileName(TmpFile);
		strcat(filePWD,"/");
		strcat(filePWD,TmpFile);
		filePWD[strlen(filePWD)] = '\0';
		debug_printf("filePWD = %s\n",filePWD);
		memcpy(FILERTXStruct->fileTmp,filePWD,strlen(filePWD));
		FILERTXStruct->Fp = fopen(FILERTXStruct->fileTmp,"wb+");
	}
	else
	{
		DEBUG_PRINTF;
		if(access(fileName,F_OK) < 0)
		{
			debug_printf("FRTx_FILERTXStructInit : The file [ %s ] is not exist!\n ",fileName);
			return -1;
		}
		DEBUG_PRINTF;
		stat(fileName,&statbuf);
		FILERTXStruct->fileSize = statbuf.st_size;
		FILERTXStruct->remdSize = statbuf.st_size;
		FILERTXStruct->Fp = fopen(fileName,"r+");
		debug_printf("FILERTXStruct->remdSize = %d\n",FILERTXStruct->remdSize);
	}
	DEBUG_PRINTF;
	if(FILERTXStruct->Fp == NULL)
	{
		DEBUG_PRINTF;
		perror("FRTx_FILERTXStructInit open file");
		return -1;
	}

	return 0;
}



int FRTx_FileFrameRx(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t len,uint32_t Offset)
{
	uint8_t i = 0;
	FILERTXStruct_t *FILERTXStruct = NULL;
	
	if(FileUser == NULL || fileName == NULL || filedata == NULL)
		return -1;
	debug_printf("Offset = %d\n",Offset);
	DEBUG_PRINTF;
	if(Offset == 0)
	{
		FILERTXStruct = (FILERTXStruct_t *)malloc(sizeof(FILERTXStruct_t));
		memset(FILERTXStruct,0,sizeof(FILERTXStruct_t));
		FILERTXStruct->RWflag = RWFLAG_WRITE;
		if(FRTx_FILERTXStructInit(FILERTXStruct,FileUser,fileName) < 0)
			return -1;
		debug_printf("FILERTXStruct->fileName = %s\n",FILERTXStruct->fileName);
		if(FRTx_FileUserInsert(FILERTXStruct) != 0)
		{
			DEBUG_PRINTF;
			return -1;
		}
	}
	else
	{
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct == NULL)
		{
			DEBUG_PRINTF;
			return -1;
		
		}
		
		if(Offset != FILERTXStruct->Offset + 2048)
		{
			debug_printf("Offset = %d,FILERTXStruct->Offset = %d\n",Offset,FILERTXStruct->Offset);
			DEBUG_PRINTF;
			return -1;
		
		}
	}

	debug_printf("---FILERTXStruct->Offset = %d\n",FILERTXStruct->Offset);
	FILERTXStruct->Offset = Offset;
	fseek(FILERTXStruct->Fp,FILERTXStruct->Offset,SEEK_SET);
	fwrite(filedata,len,1,FILERTXStruct->Fp);
	fflush(FILERTXStruct->Fp);
	DEBUG_PRINTF;
	if(len < 2048)
	{
		debug_printf("recv file ok!\n");
		fclose(FILERTXStruct->Fp);
		rename(FILERTXStruct->fileTmp,FILERTXStruct->fileName);
		FRTx_FileUserRemove(FILERTXStruct);
		DEBUG_PRINTF;
		return 1;
	}
	return 0;
}


int FRTx_FileFrameTx(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t *len,uint32_t Offset)
{
	uint8_t i = 0;
	uint32_t readLen = 0;
	FILERTXStruct_t *FILERTXStruct = NULL;
	
	if(FileUser == NULL || fileName == NULL || filedata == NULL)
		return -1;
	
	debug_printf("Offset = %d\n",Offset);
	if(Offset == 0)
	{
		FILERTXStruct = (FILERTXStruct_t *)malloc(sizeof(FILERTXStruct_t));
		memset(FILERTXStruct,0,sizeof(FILERTXStruct_t));
		FILERTXStruct->RWflag = RWFLAG_READ;
		if(FRTx_FILERTXStructInit(FILERTXStruct,FileUser,fileName) < 0)
		{
			DEBUG_PRINTF;
			return -1;
		}
		
		debug_printf("FILERTXStruct->fileName = %s\n",FILERTXStruct->fileName);
		if(FRTx_FileUserInsert(FILERTXStruct) != 0)
		{
			DEBUG_PRINTF;
			return -1;
		}
	}
	else
	{
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct == NULL)
		{
			DEBUG_PRINTF;
			return -1;
		}
		
		if(Offset != FILERTXStruct->Offset + 2048)
		{
			DEBUG_PRINTF;
			debug_printf("Offset = %d,FILERTXStruct->Offset = %d,",Offset,FILERTXStruct->Offset);
			return -1;
		}
	}

	uint32_t read_size = 0;
	FILERTXStruct->Offset = Offset;

	fseek(FILERTXStruct->Fp,FILERTXStruct->Offset,SEEK_SET);
	read_size = (FILERTXStruct->remdSize >= 2048) ? 2048 : FILERTXStruct->remdSize;
	//printf("FILERTXStruct->remdSize = %d,read_size = %d\n",FILERTXStruct->remdSize,read_size);
	readLen = fread(filedata,1,read_size,FILERTXStruct->Fp);
	if(readLen != read_size)
	{
		DEBUG_PRINTF;
		return -1;
	}
	
	*len = readLen;
	FILERTXStruct->remdSize -= read_size;
	if(read_size < 2048)
	{
		DEBUG_PRINTF;
		fclose(FILERTXStruct->Fp);
		FRTx_FileUserRemove(FILERTXStruct);
	}
	return 0;
}

int FRTx_FileFrameRx2K(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t len,uint32_t FrameId)
{
	uint8_t i = 0;
	FILERTXStruct_t *FILERTXStruct = NULL;
	
	if(FileUser == NULL || fileName == NULL || filedata == NULL)
		return -1;
	
	if(FrameId == 0)
	{
		DEBUG_PRINTF;
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct != NULL)
		{
			DEBUG_PRINTF;
			fclose(FILERTXStruct->Fp);
			remove(FILERTXStruct->fileTmp);
			FRTx_FileUserRemove(FILERTXStruct);
			FILERTXStruct = NULL;
		}
		if(FILERTXStruct != NULL)
			return -1;
		DEBUG_PRINTF;
		FILERTXStruct = (FILERTXStruct_t *)malloc(sizeof(FILERTXStruct_t));
		memset(FILERTXStruct,0,sizeof(FILERTXStruct_t));
		FILERTXStruct->RWflag = RWFLAG_WRITE;
		DEBUG_PRINTF;
		if(FRTx_FILERTXStructInit(FILERTXStruct,FileUser,fileName) < 0)
			return -1;
		DEBUG_PRINTF;
		debug_printf("FILERTXStruct->fileName = %s\n",FILERTXStruct->fileName);
		if(FRTx_FileUserInsert(FILERTXStruct) != 0)
			return -1;
		DEBUG_PRINTF;
	}
	else
	{
		DEBUG_PRINTF;
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct == NULL)
			return -1;
		DEBUG_PRINTF;
		
		if(FrameId != FILERTXStruct->FrameId + 1)
		{
			debug_printf("FrameId = %d,FILERTXStruct->FrameId = %d\n",FrameId,FILERTXStruct->FrameId);
			return -1;
		}
	}
	DEBUG_PRINTF;
	FILERTXStruct->FrameId = FrameId;
	fseek(FILERTXStruct->Fp,FILERTXStruct->FrameId * 2048,SEEK_SET);
	fwrite(filedata,len,1,FILERTXStruct->Fp);
	fflush(FILERTXStruct->Fp);
	debug_printf("len = %d\n",len);
	if(len < 2048)
	{
		DEBUG_PRINTF;
		fclose(FILERTXStruct->Fp);
		rename(FILERTXStruct->fileTmp,FILERTXStruct->fileName);
		debug_printf("FILERTXStruct->fileTmp = %s,FILERTXStruct->fileName = %s\n",FILERTXStruct->fileTmp,FILERTXStruct->fileName);
		FRTx_FileUserRemove(FILERTXStruct);
		return 1;
	}
	return 0;
}


int FRTx_FileFrameTx2K(FILEUser_t *FileUser,char *fileName,char *FrameData,uint32_t *FrameLen,uint32_t FrameID)
{
	uint8_t i = 0;
	uint32_t ReadSize = 0;
	FILERTXStruct_t *FILERTXStruct = NULL;
	
	if(FileUser == NULL || fileName == NULL || FrameData == NULL)
		return -1;
	
	DEBUG_PRINTF;
	if(FrameID == 0)
	{
		
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct != NULL)
		{
			DEBUG_PRINTF;
			fclose(FILERTXStruct->Fp);
			remove(FILERTXStruct->fileTmp);
			FRTx_FileUserRemove(FILERTXStruct);
			FILERTXStruct = NULL;
		}
		if(FILERTXStruct != NULL)
			return -1;

		DEBUG_PRINTF;
		FILERTXStruct = (FILERTXStruct_t *)malloc(sizeof(FILERTXStruct_t));
		memset(FILERTXStruct,0,sizeof(FILERTXStruct_t));
		FILERTXStruct->RWflag = RWFLAG_READ;
		if(FRTx_FILERTXStructInit(FILERTXStruct,FileUser,fileName) < 0)
			return -1;
		
		debug_printf("FILERTXStruct->fileName = %s\n",FILERTXStruct->fileName);
		if(FRTx_FileUserInsert(FILERTXStruct) != 0)
			return -1;
		DEBUG_PRINTF;
	}
	else
	{
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct == NULL)
			return -1;
		DEBUG_PRINTF;
		debug_printf("1FrameID = %d,FILERTXStruct->FrameId = %d\n",FrameID,FILERTXStruct->FrameId);
		if(FrameID != FILERTXStruct->FrameId + 1)
		{
			debug_printf("2FrameID = %d,FILERTXStruct->FrameId = %d\n",FrameID,FILERTXStruct->FrameId);
			return -1;
		}
		DEBUG_PRINTF;
	}

	//开始按每帧2k数据开读文件
	FILERTXStruct->FrameId = FrameID;
	fseek(FILERTXStruct->Fp,FILERTXStruct->FrameId * 2048,SEEK_SET);
	ReadSize = (FILERTXStruct->remdSize > 2048) ? 2048 : FILERTXStruct->remdSize;
	DEBUG_PRINTF;
	fread(FrameData,ReadSize,1,FILERTXStruct->Fp);
	DEBUG_PRINTF;
	FILERTXStruct->remdSize = FILERTXStruct->remdSize - ReadSize;
	*FrameLen = ReadSize;
	DEBUG_PRINTF;
	//fflush(FILERTXStruct->Fp);
	//文件剩余长度不足16k，读完即返回并释放相关资源
	if(ReadSize < 2048)
	{
		DEBUG_PRINTF;
		fclose(FILERTXStruct->Fp);
		FRTx_FileUserRemove(FILERTXStruct);
		return 1;
	}
	DEBUG_PRINTF;
	return 0;
}



int FRTx_FileFrameRx16K(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t len,uint32_t FrameOffset)
{
	uint8_t i = 0;
	FILERTXStruct_t *FILERTXStruct = NULL;
	
	if(FileUser == NULL || fileName == NULL || filedata == NULL)
		return -1;
	
	DEBUG_PRINTF;
	if(FrameOffset == 0)
	{
		
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct != NULL)
		{
			DEBUG_PRINTF;
			fclose(FILERTXStruct->Fp);
			remove(FILERTXStruct->fileTmp);
			FRTx_FileUserRemove(FILERTXStruct);
			FILERTXStruct = NULL;
		}
		if(FILERTXStruct != NULL)
			return -1;

		DEBUG_PRINTF;
		FILERTXStruct = (FILERTXStruct_t *)malloc(sizeof(FILERTXStruct_t));
		memset(FILERTXStruct,0,sizeof(FILERTXStruct_t));
		FILERTXStruct->RWflag = RWFLAG_WRITE;
		if(FRTx_FILERTXStructInit(FILERTXStruct,FileUser,fileName) < 0)
			return -1;
		debug_printf("FILERTXStruct->fileName = %s\n",FILERTXStruct->fileName);
		if(FRTx_FileUserInsert(FILERTXStruct) != 0)
			return -1;
	}
	else
	{
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct == NULL)
			return -1;
		DEBUG_PRINTF;
		
		if(FrameOffset != FILERTXStruct->Offset + 16384)
		{
			debug_printf("FrameOffset = %d,FILERTXStruct->Offset = %d\n",FrameOffset,FILERTXStruct->Offset);
			return -1;
		}
		DEBUG_PRINTF;
	}

	FILERTXStruct->Offset = FrameOffset;
	fseek(FILERTXStruct->Fp,FILERTXStruct->Offset,SEEK_SET);
	fwrite(filedata,len,1,FILERTXStruct->Fp);
	fflush(FILERTXStruct->Fp);
	if(len < 16384)
	{
		fclose(FILERTXStruct->Fp);
		rename(FILERTXStruct->fileTmp,FILERTXStruct->fileName);
		debug_printf("FILERTXStruct->fileTmp = %s,FILERTXStruct->fileName = %s\n",FILERTXStruct->fileTmp,FILERTXStruct->fileName);
		FRTx_FileUserRemove(FILERTXStruct);
		return 1;
	}
	return 0;
}

int FRTx_FileFrameTx16K(FILEUser_t *FileUser,char *fileName,char *FrameData,uint32_t *FrameLen,uint32_t FrameOffset)
{
	uint8_t i = 0;
	uint32_t ReadSize = 0;
	FILERTXStruct_t *FILERTXStruct = NULL;
	
	if(FileUser == NULL || fileName == NULL || FrameData == NULL)
		return -1;
	
	DEBUG_PRINTF;
	if(FrameOffset == 0)
	{
		
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct != NULL)
		{
			DEBUG_PRINTF;
			fclose(FILERTXStruct->Fp);
			remove(FILERTXStruct->fileTmp);
			FRTx_FileUserRemove(FILERTXStruct);
			FILERTXStruct = NULL;
		}
		if(FILERTXStruct != NULL)
			return -1;

		DEBUG_PRINTF;
		FILERTXStruct = (FILERTXStruct_t *)malloc(sizeof(FILERTXStruct_t));
		memset(FILERTXStruct,0,sizeof(FILERTXStruct_t));
		FILERTXStruct->RWflag = RWFLAG_READ;
		if(FRTx_FILERTXStructInit(FILERTXStruct,FileUser,fileName) < 0)
			return -1;
		debug_printf("FILERTXStruct->fileName = %s\n",FILERTXStruct->fileName);
		if(FRTx_FileUserInsert(FILERTXStruct) != 0)
			return -1;
	}
	else
	{
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct == NULL)
			return -1;
		DEBUG_PRINTF;
		
		if(FrameOffset != FILERTXStruct->Offset + 16384)
		{
			debug_printf("FrameOffset = %d,FILERTXStruct->Offset = %d\n",FrameOffset,FILERTXStruct->Offset);
			return -1;
		}
		DEBUG_PRINTF;
	}

	//开始按每帧16k数据开读文件
	FILERTXStruct->Offset = FrameOffset;
	fseek(FILERTXStruct->Fp,FILERTXStruct->Offset,SEEK_SET);
	ReadSize = (FILERTXStruct->remdSize > 16384) ? 16384 : FILERTXStruct->remdSize;
	fread(FrameData,ReadSize,1,FILERTXStruct->Fp);
	
	FILERTXStruct->remdSize = FILERTXStruct->remdSize - ReadSize;
	*FrameLen = ReadSize;

	//fflush(FILERTXStruct->Fp);
	//文件剩余长度不足16k，读完即返回并释放相关资源
	if(ReadSize < 16384)
	{
		fclose(FILERTXStruct->Fp);
		FRTx_FileUserRemove(FILERTXStruct);
		return 1;
	}

	return 0;
}



int FRTx_FileFrameRx20K(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t len,uint32_t FrameOffset)
{
	uint8_t i = 0;
	FILERTXStruct_t *FILERTXStruct = NULL;
	
	if(FileUser == NULL || fileName == NULL || filedata == NULL)
		return -1;
	
	DEBUG_PRINTF;
	if(FrameOffset == 0)
	{
		
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct != NULL)
		{
			DEBUG_PRINTF;
			fclose(FILERTXStruct->Fp);
			remove(FILERTXStruct->fileTmp);
			FRTx_FileUserRemove(FILERTXStruct);
			FILERTXStruct = NULL;
		}
		if(FILERTXStruct != NULL)
			return -1;

		DEBUG_PRINTF;
		FILERTXStruct = (FILERTXStruct_t *)malloc(sizeof(FILERTXStruct_t));
		memset(FILERTXStruct,0,sizeof(FILERTXStruct_t));
		FILERTXStruct->RWflag = RWFLAG_WRITE;
		if(FRTx_FILERTXStructInit(FILERTXStruct,FileUser,fileName) < 0)
			return -1;
		debug_printf("FILERTXStruct->fileName = %s\n",FILERTXStruct->fileName);
		if(FRTx_FileUserInsert(FILERTXStruct) != 0)
			goto FCLOSEFILE;
	}
	else
	{
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct == NULL)
			return -1;
		DEBUG_PRINTF;
		
		if(FrameOffset != FILERTXStruct->Offset + 20480)
			goto FCLOSEFILE;
		DEBUG_PRINTF;
	}

	FILERTXStruct->Offset = FrameOffset;
	fseek(FILERTXStruct->Fp,FILERTXStruct->Offset,SEEK_SET);
	fwrite(filedata,len,1,FILERTXStruct->Fp);
	fflush(FILERTXStruct->Fp);
	if(len < 20480)
	{
		fclose(FILERTXStruct->Fp);
		rename(FILERTXStruct->fileTmp,FILERTXStruct->fileName);
		debug_printf("FILERTXStruct->fileTmp = %s,FILERTXStruct->fileName = %s\n",FILERTXStruct->fileTmp,FILERTXStruct->fileName);
		FRTx_FileUserRemove(FILERTXStruct);
		return 1;
	}
	return 0;

	FCLOSEFILE:
		fclose(FILERTXStruct->Fp);
		remove(FILERTXStruct->fileTmp);
		FRTx_FileUserRemove(FILERTXStruct);
		return -1;
}


int FRTx_FileFrameTx20K(FILEUser_t *FileUser,char *fileName,char *FrameData,uint32_t *FrameLen,uint32_t FrameOffset)
{
	uint8_t i = 0;
	uint32_t ReadSize = 0;
	FILERTXStruct_t *FILERTXStruct = NULL;
	
	if(FileUser == NULL || fileName == NULL || FrameData == NULL)
		return -1;
	
	DEBUG_PRINTF;
	if(FrameOffset == 0)
	{
		
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct != NULL)
		{
			DEBUG_PRINTF;
			fclose(FILERTXStruct->Fp);
			remove(FILERTXStruct->fileTmp);
			FRTx_FileUserRemove(FILERTXStruct);
			FILERTXStruct = NULL;
		}
		if(FILERTXStruct != NULL)
			return -1;

		DEBUG_PRINTF;
		FILERTXStruct = (FILERTXStruct_t *)malloc(sizeof(FILERTXStruct_t));
		memset(FILERTXStruct,0,sizeof(FILERTXStruct_t));
		FILERTXStruct->RWflag = RWFLAG_READ;
		if(FRTx_FILERTXStructInit(FILERTXStruct,FileUser,fileName) < 0)
			return -1;
		
		debug_printf("FILERTXStruct->fileName = %s\n",FILERTXStruct->fileName);
		if(FRTx_FileUserInsert(FILERTXStruct) != 0)
			goto FCLOSEFILE;
	}
	else
	{
		FILERTXStruct = FRTx_FileUserFind(FileUser,fileName);
		if(FILERTXStruct == NULL)
			return -1;
		DEBUG_PRINTF;
		
		if(FrameOffset != FILERTXStruct->Offset + 20480)
			goto FCLOSEFILE;
		DEBUG_PRINTF;
	}

	//开始按每帧16k数据开读文件
	FILERTXStruct->Offset = FrameOffset;
	fseek(FILERTXStruct->Fp,FILERTXStruct->Offset,SEEK_SET);
	ReadSize = (FILERTXStruct->remdSize > 20480) ? 20480 : FILERTXStruct->remdSize;
	fread(FrameData,ReadSize,1,FILERTXStruct->Fp);
	
	FILERTXStruct->remdSize = FILERTXStruct->remdSize - ReadSize;
	*FrameLen = ReadSize;

	//fflush(FILERTXStruct->Fp);
	//文件剩余长度不足16k，读完即返回并释放相关资源
	if(ReadSize < 20480)
	{
		fclose(FILERTXStruct->Fp);
		FRTx_FileUserRemove(FILERTXStruct);
		return 1;
	}

	return 0;

	FCLOSEFILE:
		fclose(FILERTXStruct->Fp);
		remove(FILERTXStruct->fileTmp);
		FRTx_FileUserRemove(FILERTXStruct);
		return -1;
}




