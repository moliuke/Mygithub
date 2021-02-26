#ifndef __PTCCOMMON_H
#define __PTCCOMMON_H
#include "config.h"

#define FLAG_RECV		0
#define FLAG_SEND		1


enum
{
	state_ip = 0,
	state_netmask,
	state_gw
};

#define PROTOCOL_SEEWOR		0x1688
extern int protocol_select;



unsigned char IsInCMDLIST(unsigned int cmd);
int check_0x02and0x03(uint8_t flag,uint8_t *input,uint32_t inputlen,uint8_t *output,uint32_t *outputlen);
unsigned short XKCalculateCRC(uint8_t *TempString,uint32_t nDataLengh);
int ParityCheck_CRC16(unsigned char *CRCdata,int len);
int prtcl_preparsing(unsigned char *pDataBuf,unsigned int nLength,unsigned char *output,unsigned int *len);
int COM_CopyFile(const char *srcPath,const char *desPath);
int set_devip(uint8_t *IP,uint8_t *netmask,uint8_t *gateway);
void COM_PROTOCOLMemoryMalloc(void);


void dir_wintolinux(const char *dir);
void Dir_LetterBtoL(char *dir);
void mkdirs(const char *dirstr);
#endif

