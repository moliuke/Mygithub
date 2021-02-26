#ifndef __FILE_TRMIT_H
#define __FILE_TRMIT_H

#include <stdio.h>
#include "config.h"
#include "myerror.h"
//#include "protocol.h"
#include "common.h"

typedef struct
{
	user_t 		*user;
	
	FILE 		*fp;
	char 		filename[128];
	uint16_t 	fr_id;
	uint32_t	fr_len;
	uint32_t 	fr_offset;
	uint8_t 	*fr_data;
	
}file_fr_t;

//接收与上传文件
int ReceiveFile(file_fr_t *file_fr);
int fileupload(file_fr_t *file_fr);

int BigFile_Write(file_fr_t *file_fr);
err_t BigFile_Read(file_fr_t *file_fr);
//可递归创建目录
void mkdirs(const char *dir);
#endif

