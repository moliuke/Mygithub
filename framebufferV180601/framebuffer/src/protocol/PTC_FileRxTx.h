#ifndef __FILERXTX_H
#define __FILERXTX_H
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "config.h"
#include "../include/debug.h"


#define 	FILE_USER_MAXNUM	20

#define 	USERTYPE_NET		0
#define 	USERTYPE_UART		1

#define 	RWFLAG_READ			0
#define 	RWFLAG_WRITE		1

typedef struct __fileUser
{
	uint8_t 	userType;	//uart or net
	
	char 		ip[16];
	uint16_t 	port;

	uint16_t 	comx;
	
}FILEUser_t;

typedef struct __FileRxTx
{
	uint8_t 		busyFlag;
	FILEUser_t		fileUser;
	
	FILE 			*Fp;
	uint8_t 		RWflag;
	char 			fileName[64];
	char 			fileTmp[64];
	uint32_t 		FrameId;
	uint32_t 		FrameSize;
	uint32_t 		Offset;
	uint32_t 		fileSize;
	uint32_t 		remdSize;
}FILERTXStruct_t;

int FRTx_FileFrameTx(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t *len,uint32_t Offset);
int FRTx_FileFrameRx(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t len,uint32_t Offset);
void FRTx_FileUserInit(FILEUser_t *FileUser,uint8_t userType,char *ip,uint32_t port,uint8_t comx);

int FRTx_FileFrameRx2K(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t len,uint32_t FrameId);
int FRTx_FileFrameTx2K(FILEUser_t *FileUser,char *fileName,char *FrameData,uint32_t *FrameLen,uint32_t FrameID);
int FRTx_FileFrameRx16K(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t len,uint32_t FrameOffset);
int FRTx_FileFrameTx16K(FILEUser_t *FileUser,char *fileName,char *FrameData,uint32_t *FrameLen,uint32_t FrameOffset);
int FRTx_FileFrameRx20K(FILEUser_t *FileUser,char *fileName,char *filedata,uint32_t len,uint32_t FrameOffset);
int FRTx_FileFrameTx20K(FILEUser_t *FileUser,char *fileName,char *FrameData,uint32_t *FrameLen,uint32_t FrameOffset);

#endif
