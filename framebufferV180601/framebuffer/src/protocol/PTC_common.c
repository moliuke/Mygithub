#include <stdio.h>


#include "PTC_common.h"
#include "debug.h"
#include "../update.h"

int protocol_select;


unsigned short crc_table[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,0x8108, 0x9129,
	0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,0x1231, 0x0210, 0x3273, 0x2252,
	0x52B5, 0x4294, 0x72F7, 0x62D6,0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C,
	0xF3FF, 0xE3DE,0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,0x3653, 0x2672,
	0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,0xB75B, 0xA77A, 0x9719, 0x8738,
	0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861,
	0x2802, 0x3823,0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,0xDBFD, 0xCBDC,
	0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,0x6CA6, 0x7C87, 0x4CE4, 0x5CC5,
	0x2C22, 0x3C03, 0x0C60, 0x1C41,0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B,
	0x8D68, 0x9D49,0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,0x9188, 0x81A9,
	0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,0x1080, 0x00A1, 0x30C2, 0x20E3,
	0x5004, 0x4025, 0x7046, 0x6067,0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C,
	0xE37F, 0xF35E,0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,0x34E2, 0x24C3,
	0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,0xA7DB, 0xB7FA, 0x8799, 0x97B8,
	0xE75F, 0xF77E, 0xC71D, 0xD73C,0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676,
	0x4615, 0x5634,0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,0xCB7D, 0xDB5C,
	0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,0x4A75, 0x5A54, 0x6A37, 0x7A16,
	0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B,
	0x9DE8, 0x8DC9,0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,0x6E17, 0x7E36,
	0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};


unsigned int  CMDLIST[]=
{
	//0段指令
	0x3030,0x3031,0x3032,0x3033,0x3034,0x3035,0x3036,0x3037,0x3038,0x3039,
	//2段指令
	0x3230,0x3231,0x3232,0x3233,0x3234,
	//3段指令
	0x3330,0x3331,
	//4段指令
	0x3430,0x3431,0x3432,0x3433,0x3434,0x3435
}; 

unsigned char IsInCMDLIST(unsigned int cmd)
{
	int ii = 0;
	for(ii=0; ii < 43; ii++)
	{
		if(CMDLIST[ii] == cmd)
		{
			return 1;
		}
	}
	
	return 0;
}

//<<<表调用函数>>>
unsigned short gen_crc(const unsigned char *buffer, int buffer_length)
{
	unsigned short crc = 0;
	int i;
	for (i = 0; i < buffer_length; i++)
		crc = crc_table[((crc >> 8) ^ buffer[i]) & 0xFF] ^ (crc << 8);
	return crc;
}

//   计算CRC校验
unsigned short XKCalculateCRC(uint8_t *TempString,uint32_t nDataLengh)
{
	uint8_t c,treat,bcrc;
	uint16_t warc =0;
	uint16_t ii,jj;
	uint32_t n = 0;
	DEBUG_PRINTF;
	for(ii=0; ii < nDataLengh; ii++)
	{
		c= TempString[ii];
		for(jj=0; jj<8; jj++)
		{
			treat = c & 0x80;
			c <<= 1;
			bcrc = (warc >>8) & 0x80;
			warc <<=1;
			if(treat != bcrc)
			{
				warc ^= 0x1021;
			}
		}
	}
	return warc;
}

int ParityCheck_CRC16(unsigned char *CRCdata,int len)
{
	unsigned short parity;
	unsigned int time_us = 0;
	parity = CRCdata[len - 3] << 8 | CRCdata[len - 2];
	DEBUG_PRINTF;
#if 1
	if(parity != gen_crc(CRCdata + 1,len-4))
	{
		debug_printf("parity = 0x%x,gen_crc(CRCdata + 1,len-4) = 0x%x\n",parity,gen_crc(CRCdata + 1,len-4));
		debug_printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n\n\n");
		return -1;
	}
#else
	if(parity != XKCalculateCRC(output + 1,outputlen-4))
	{
		return -1;
	}
#endif
	debug_printf("parity = 0x%x\n",parity);
	return 0;
}



void dir_wintolinux(const char *dir)
{	
	char *p = NULL;
	debug_printf("B:dir = %s\n",dir);
	
	while((p = strchr(dir,'\\')) != NULL)
	{
		*p = '/';
		p += 1;
	}

	char *pp = NULL;
	pp = strstr(dir,"//");
	if(pp != NULL)
	{
		memcpy(pp,pp+1,9);
		pp[9] = '\0';
	}
	DEBUG_PRINTF;
	debug_printf("F:dir = %s\n",dir);
}
#if 1
void dir_linuxtowin(const char *dir)
{
	char *p = NULL;
	debug_printf("B:dir = %s\n",dir);

	while((p = strchr(dir,'/')) != NULL)
	{
		*p = '\\';
		p += 1;		
	}
	DEBUG_PRINTF;
	debug_printf("F:dir = %s\n",dir);

}
#endif
void Dir_LetterBtoL(char *dir)
{
	char *charStr = dir;
	char *charp = charStr;
	while(*charp != '\0')
	{
		if(*charp >= 'A' && *charp <= 'Z')
			*charp = *charp + 0x20;
		charp += 1;
	}
}

void mkdirs(const char *dirstr)
{
	char tmp[128]; 
	char *p = NULL;
	char *q = NULL;
	int ret = 0;

	if (strlen(dirstr) == 0 || dirstr == NULL) {
        debug_printf("strlen(dir) is 0 or dir is NULL./n");
        return;
	}

	memset(tmp,0x00,sizeof(tmp));
	memcpy(tmp,dirstr,strlen(dirstr));

	if (tmp[0] == '/') 
        p = strchr(tmp + 1, '/');
	else 
        p = strchr(tmp, '/');

	if (p)
	{
		*p = '\0';
		debug_printf("tmp = %s\n",tmp);
		//这里应该先判断将要创建的目录是否存在
		ret = access(tmp,F_OK);
		if(ret < 0)
		{
			mkdir(tmp,0777);
		}

		chdir(tmp);
		
	} 
	else 
	{
		//最后如果不存在'/',该文件名有可能是普通文件也可能是目录，不知道还有没有好办法解决
		//就我们项目来说，在找不到'/'时，该文件就是一个普通文件
		//不再创建目录
        return;
	}
	mkdirs(p + 1);
}


int check_0x02and0x03(uint8_t flag,uint8_t *input,uint32_t inputlen,uint8_t *output,uint32_t *outputlen)
{
	int i = 0,j;
	int m = 0;		
	int IsEscOK = 0;

	debug_printf("######################################## inputlen = %d\n",inputlen);
	
	if(flag == FLAG_RECV)
	{
		for(i = 0 ; i < inputlen ; i ++ )
		{
			//检测数据是否包含0x02或者0x03,包含两者之一则数据错误，放弃数据
			if(input[i] == 0x02 || input[i] == 0x03)
			{
				debug_printf("the data recv contains 0x02 or 0x03\n");
				return -1;
			}
		
			//检测是否包含0x1B
			if(input[i] == 0x1B)
			{
				IsEscOK = 1;
				continue;
			}
		
		
			if(!IsEscOK)
				output[m] = input[i];
			else
			{
				IsEscOK = 0;
				output[m] = input[i]+0x1B;
			}
			m++;
		}
		
		*outputlen = m;
		return m;
	}

	while(i < inputlen)
	{
		switch(input[i])
		{
			
			case 0x02:
				output[m]	= 0x1B;
				output[m+1] = 0xe7;
				m += 2;
				break;
			case 0x03:
				output[m]	= 0x1B;
				output[m+1] = 0xE8;
				m += 2;
				break;
			case 0x1B:
				output[m]	= 0x1B;
				output[m+1] = 0x00;
				m += 2;
				break;
			default:
				output[m]	= input[i];
				m += 1;
				break;
		}
		
		output[m]	= 0;
		i++;
	}
	*outputlen = m;
	return m;
}



int prtcl_preparsing(unsigned char *pDataBuf,unsigned int nLength,unsigned char *output,unsigned int *len)
{
	int TempLength = 0;
	uint8_t TempByte;
	//bool IsParseBegin = 0;
	int TempDataLength =0;
	int IsEscOK = 0;
	unsigned short parity;
	unsigned char parity_tmp[4];
	unsigned short data_counter = 0;
	int i = 0,n = 0,m = 0;
	unsigned char *data_field;
	uint32_t outputlen = 0;
	int ret = -1;
	
	debug_printf("*nLength = %d\n",nLength);
	//recvmsg_printf(pDataBuf,nLength);
	
	//判断头尾及长度是否合理
	if(nLength < 8)
	{
		debug_printf("recv data length error 1\n");
		
		return -1;
	}
	DEBUG_PRINTF;
	//判断头尾正确
	if(pDataBuf[0] != 0x02 || pDataBuf[nLength-1] != 0x03)
	{
		debug_printf("the start byte or end byte of the recv data error\n");
		return -1;
	}
	DEBUG_PRINTF;
	data_counter = nLength - 2;
	DEBUG_PRINTF;
	//接收到的数据有可能有很多转义字符0x02或者0x03，或者0x1B，那么经过将数据还原后
	//数据量应该变小，暂且多分配256个字节吧，不多
	//data_field = (unsigned char *)malloc(nLength + nLength / 2);
	//memset(data_field,0,nLength + nLength / 2);

	output[0] = 0x02;
	DEBUG_PRINTF;
	ret = check_0x02and0x03(FLAG_RECV,pDataBuf+1,data_counter,output+1,&outputlen);
	if(ret < 0)
	{
		debug_printf("check_0x02and0x03:data error!\n");
		return -1;
	}
	DEBUG_PRINTF;
	output[outputlen + 1] = 0x03;
	DEBUG_PRINTF;
	*len = outputlen + 2;
	
	DEBUG_PRINTF;
	return 0;

}





#define BUFFER_SIZE	1024
/** 
 * 功能：拷贝文件函数 
 * 参数： 
 *      sourceFileNameWithPath：源文件名（带路径） 
 *      targetFileNameWithPath：目标文件名（带路径） 
 * 返回值： 
 *      SUCCESS: 拷贝成功 
 *      FAILURE：拷贝失败 
 * author:wangchangshuai jlu 
 */  
int COM_CopyFile(const char *srcPath,const char *desPath)  
{  
    FILE *fpR, *fpW;  
    char buffer[BUFFER_SIZE];  
    int lenR, lenW;  
    if ((fpR = fopen(srcPath, "r")) == NULL)  
    {  
        debug_printf("The file '%s' can not be opened! \n", srcPath);  
        return -1;  
    }  
    if ((fpW = fopen(desPath, "w")) == NULL)  
    {  
        debug_printf("The file '%s' can not be opened! \n", desPath);  
        fclose(fpR);  
        return -1;  
    }  
  
    memset(buffer, 0, BUFFER_SIZE);  
    while ((lenR = fread(buffer, 1, BUFFER_SIZE, fpR)) > 0)  
    {  
        if ((lenW = fwrite(buffer, 1, lenR, fpW)) != lenR)  
        {  
            debug_printf("Write to file '%s' failed!\n", desPath);  
            fclose(fpR);  
            fclose(fpW);  
            return -1;  
        }  
        memset(buffer, 0, BUFFER_SIZE);  
    }  

	DEBUG_PRINTF;
    fclose(fpR);  
    fclose(fpW);  
    return 0;  
		
}  


#if 0
int set_devip(char *ip,char *mask,char *gw)
{

	int read_ct = 0;
	char file_content[1024 * 4];
	FILE *rcS_fp = NULL;
	int state = state_ip;
	int flag_ok = 0;

	char cpy_cmd[48];

	//检查文件是否存在
	if(access("/etc/init.d/rcS",F_OK) < 0)
	{
		_log_file_write_("protolog","set_devip",strlen("set_devip"),"/etc/init.d/rcS is not exist",strlen("/etc/init.d/rcS is not exist"));
		return -1;
	}

	//修改配置文件前，先备份
	memset(cpy_cmd,0,sizeof(cpy_cmd));
	sprintf(cpy_cmd,"%s","cp /etc/init.d/rcS /etc/init.d/cpy_rcS");
	system(cpy_cmd);

	//打开文件
	if((rcS_fp = fopen("/etc/init.d/rcS","r+")) == NULL)
	{
		perror("fopen");
		_log_file_write_("protolog","set_devip",strlen("set_devip"),"/etc/init.d/rcS can't open",strlen("/etc/init.d/rcS can't open"));
		return -1;
	}

	//读取文件
	memset(file_content,0,sizeof(file_content));
	read_ct = fread(file_content,1,sizeof(file_content),rcS_fp);
	if(read_ct <= 0)
	{
		_log_file_write_("protolog","set_devip",strlen("set_devip"),"/etc/init.d/rcS read error",strlen("/etc/init.d/rcS is read error"));
		goto FREE_RESRC;
	}

	debug_printf("read_ct ============ %d\n",read_ct);
	

	//搜索文件中描述ip、掩码、网关的位置
	char *ip_p = NULL,*netmask_p = NULL,*gateway_p = NULL;
	char *content_p = file_content;
	while(1)
	{
		switch(state)
		{
			case state_ip:
				ip_p = strstr(content_p,"ifconfig eth0");
				if(ip_p == NULL)
				{
					_log_file_write_("protolog","set_devip",strlen("set_devip"),"can not found ifconfig eth0",strlen("can not found ifconfig eth0"));
					goto FREE_RESRC;
				}
				DEBUG_PRINTF;
				debug_printf("1ip_p = %s\n",ip_p);
				
				content_p = ip_p;
				state = state_netmask;
				break;
			case state_netmask:
				netmask_p = strstr(content_p,"netmask");
				if(netmask_p == NULL)
				{
					_log_file_write_("protolog","set_devip",strlen("set_devip"),"can not found netmask",strlen("can not found netmask"));
					goto FREE_RESRC;
				}

				if(netmask_p - ip_p > 30)
				{
					ip_p += 1;
					state = state_ip;
				}

				content_p = netmask_p;
				state = state_gw;
				break;
			case state_gw:
				gateway_p = strstr(content_p,"gw");
				if(gateway_p == NULL)
				{
					_log_file_write_("protolog","set_devip",strlen("set_devip"),"can not found gw",strlen("can not found gw"));
					goto FREE_RESRC;
				}

				if(gateway_p - netmask_p > 70)
				{
					ip_p += 1;
					state = state_ip;
				}

				int len = 0;
				char data[200];
				len = gateway_p - ip_p;
				memset(data,0,sizeof(data));
				memcpy(data,ip_p,len);
				debug_printf("data = %s\n",data);	

				flag_ok = 1;
				break;
			defualt:
				break;
		}

		if(flag_ok)
			break;
	}
	
	char *row_p = NULL;
	row_p = strchr(ip_p,'\n');
	if(row_p == NULL)
	{
		_log_file_write_("protolog","set_devip",strlen("set_devip"),"can not found \\n 1",strlen("can not found \\n 1"));
		goto FREE_RESRC;
	}
	row_p = strchr(row_p+1,'\n');
	if(row_p == NULL)
	{
		_log_file_write_("protolog","set_devip",strlen("set_devip"),"can not found \\n 2",strlen("can not found \\n 2"));
		goto FREE_RESRC;
	}


	char setip[200];
	memset(setip,0,sizeof(setip));
	sprintf(setip,"ifconfig eth0 %s netmask %s up >/dev/null 2>&1\n/sbin/route add default gw %s",ip,mask,gw);
	debug_printf("setip = %s,strlen(setip) = %d\n",setip,strlen(setip));

	int head_half_len = ip_p - file_content;
	int tmp_len = row_p - file_content;
	int tail_half_len = read_ct - tmp_len;
	fseek(rcS_fp,0,SEEK_SET);
	fwrite(file_content,1,head_half_len,rcS_fp);
	fwrite(setip,1,strlen(setip),rcS_fp);
	fwrite(row_p,1,tail_half_len,rcS_fp);
	fflush(rcS_fp);
	fclose(rcS_fp);

	_log_file_write_("protolog","set_devip",strlen("set_devip"),"set net ok",strlen("set net ok"));
	return 0;

	FREE_RESRC:
		fclose(rcS_fp);
		return -1;
}
#else 
int set_devip(uint8_t *IP,uint8_t *netmask,uint8_t *gateway)
{
	FILE *IPF = NULL;
	char fcontent[256];
	memset(fcontent,0,sizeof(fcontent));
	sprintf(fcontent,"#!/bin/sh\n\nifconfig eth0 %s netmask %s up >/dev/null 2>&1\n/sbin/route add default gw %s\n",IP,netmask,gateway);
	IPF = fopen("/home/LEDscr/ipconfig.sh","wb+");
	if(IPF == NULL)
		return -1;
	fwrite(fcontent,1,strlen(fcontent),IPF);
	fflush(IPF);
	fclose(IPF);
}

#endif










