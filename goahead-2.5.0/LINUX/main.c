/*
 * main.c -- Main program for the GoAhead WebServer (LINUX version)
 *
 * Copyright (c) GoAhead Software Inc., 1995-2010. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 */

/******************************** Description *********************************/

/*
 *	Main program for for the GoAhead WebServer.
 */

/********************************* Includes ***********************************/

#include	"../uemf.h"
#include	"../wsIntrn.h"
#include	<signal.h>
#include	<unistd.h> 
#include	<sys/types.h>
#include	"../webs.h"
#include <netdb.h>  
#include <net/if.h>  
#include <arpa/inet.h>  
#include <sys/ioctl.h>  
#include <sys/types.h>  
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>  

/*版本格式 : 版本号*/
#define MAJOR		1	//主版本号
#define MINOR		1	//次版本号
#define REVISION	4	//修订版本号
//修复端口大于10000出现段错误问题

#ifdef WEBS_SSL_SUPPORT
#include	"../websSSL.h"
#endif

#ifdef USER_MANAGEMENT_SUPPORT
#include	"../um.h"
void	formDefineUserMgmt(void);
#endif


#define IPCONFIG 		"/home/LEDscr/ipconfig.sh"
#define PTCCONFIG	    "/home/LEDscr/config/protocol.conf"

#define CLSCONFIG 		"/home/LEDscr/config/cls.conf"
#define USRCONFIG 		"/www/password"
#define FUNCONFIG 		"/home/LEDscr/config/check.conf"
#define BRIGHTCONFIG 	"/home/LEDscr/config/autobright.conf"
/*********************************** Locals ***********************************/
/*
 *	Change configuration here
 */

//static char_t		*rootWeb = T("/opt/www");			/* Root web directory */
static char_t		*rootWeb = T("/www");
static char_t		*demoWeb = T("/var/www/html");		/* Root web directory */
static char_t		*password = T("");				/* Security password */
static int			port = WEBS_DEFAULT_PORT;		/* Server port */
static int			retries = 5;					/* Server port retries */
static int			finished = 0;					/* Finished flag */

/****************************** Forward Declarations **************************/

static int 	initWebs(int demo);
//static int	aspTest(int eid, webs_t wp, int argc, char_t **argv);
//static void formTest(webs_t wp, char_t *path, char_t *query);
static void formUploadFileTest(webs_t wp, char_t *path, char_t *query); // add by gyr 2011.10.15

static int  websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
				int arg, char_t *url, char_t *path, char_t *query);
static void	sigintHandler(int);
#ifdef B_STATS
static void printMemStats(int handle, char_t *fmt, ...);
static void memLeaks();
#endif

/*****************************************************************************
 * 函 数 名  : GetFileList
 * 负 责 人  : QQ
 * 创建日期  : 2020年1月11日
 * 函数功能  : 获取文件列表跟日志有关
 * 输入参数  : int eid        id
               webs_t wp      d
               int argc       d
               char_t **argv  d
 * 输出参数  : 无
 * 返 回 值  : static
 * 调用关系  : 
 * 其    它  : 

*****************************************************************************/
static int GetFileList(int eid, webs_t wp, int argc, char_t **argv)
{
	websWrite(wp, T("<tr><td><a href='%s'>%s</a>&nbsp; &nbsp;&nbsp;&nbsp;&nbsp;<a href='%s' download='%s'>download</a></td></tr>"),argv[0],argv[0],argv[0],argv[0]);

	return websWrite(wp, T(" "));
}


char *GetSubStr(char *szRxBuff,int nRxDataLen,int nOffset,const char * szCutFlag,int nDataOrder)
{
	static char RetBuff[512] = {0};
	unsigned short i=0;
	unsigned char nCurOrder = 0;
	unsigned char nSavNum = 0;

	memset(RetBuff,0,sizeof(RetBuff));
	for(i=0;i<nRxDataLen-nOffset;i++)
	{
		if((nCurOrder == nDataOrder)&&
			(nSavNum<(sizeof(RetBuff)-1))&&
			szRxBuff[nOffset+i] != szCutFlag[0])
		{
			RetBuff[nSavNum] = szRxBuff[nOffset+i];
			nSavNum++;
		}
		if(szRxBuff[nOffset+i]==szCutFlag[0])
		{
			nCurOrder++;
		}
		if(nCurOrder == (nDataOrder+1))
		{
			break;
		}
	}
	return RetBuff;
}

//这里加入读写配置文件
/******************************************************************************************************/
#if 1
#define KEYVALLEN 256

/*   删除左边的空格   */  
char * l_trim(char * szOutput, const char *szInput)  
{  
    assert(szInput != NULL);  
    assert(szOutput != NULL);  
    assert(szOutput != szInput);  
    for   (NULL; *szInput != '\0' && isspace(*szInput); ++szInput){  
        ;  
    }  
    return strcpy(szOutput, szInput);  
}  
  
/*   删除右边的空格   */  
char *r_trim(char *szOutput, const char *szInput)  
{  
    char *p = NULL;  
    assert(szInput != NULL);  
    assert(szOutput != NULL);  
    assert(szOutput != szInput);  
    strcpy(szOutput, szInput);  
    for(p = szOutput + strlen(szOutput) - 1; p >= szOutput && isspace(*p); --p){  
        ;  
    }  
    *(++p) = '\0';  
    return szOutput;  
}  
  
/*   删除两边的空格   */  
char * a_trim(char * szOutput, const char * szInput)  
{  
    char *p = NULL;  
    assert(szInput != NULL);  //这里暂时注释
    assert(szOutput != NULL);  
    l_trim(szOutput, szInput);  
    for   (p = szOutput + strlen(szOutput) - 1;p >= szOutput && isspace(*p); --p){  
        ;  
    }  
    *(++p) = '\0';  
    return szOutput;  
}  
  
  
int conf_file_read(char *profile, char *field, char *KeyName, char *KeyVal )  
{  
    char appname[32],keyname[32];  
    char *buf,*c;  
    char buf_i[KEYVALLEN], buf_o[KEYVALLEN];  
    FILE *fp;  
    int found=0; /* 1 AppName 2 KeyName */ 
	if(access(profile,F_OK) < 0)
	{
		return -1;
	}
    if( (fp=fopen( profile,"r" ))==NULL )
	{  
        //debug_printf( "openfile [%s] error [%s]\n",profile,strerror(errno) );  
        return(-1);  
    }
    memset( appname, 0, sizeof(appname) );  
    sprintf( appname,"[%s]", field );  
	
    fseek( fp, 0, SEEK_SET );  
    while( !feof(fp) && fgets( buf_i, KEYVALLEN, fp )!=NULL )
	{  
		//没读取到一行内容，首先把左边的空格去掉
		a_trim(buf_o,buf_i);
        if( strlen(buf_o) <= 0 )  
            continue; 
        buf = buf_o;  
  		//在这里搜索[字段名]
        if( found == 0 )
		{  
            if( buf[0] != '['  || strncmp(buf,appname,strlen(appname))!=0)
			{  
                continue;  
            }
			else
			{
				found = 1;
				continue;
			}
        } 
		//上面搜索到[字段名]后首先要检查有没有注释行
		//注释行可用#,//,
        if( (buf[0] == '#') || buf[0] == '/')
		{  
            continue;  
        } 

		//如果搜索到相应的[字段名]，潜在的注释行也都忽略了，后面却又冒出
		//一个字段[某字段]，说明当前字段下内容为空，退出
		if ( buf[0] == '[' )
        {  
        	//debug_printf("%s can not found in profile [ %s ]\n",KeyName,profile);
			fclose(fp);
            return -1;  
        }

		//接下来每一行就应该是键值对的内容


		//'='都找不到，只能继续找下一行
        if( (c = (char*)strchr(buf, '=')) == NULL )  
            continue;  

		//这里终于找到'=','='即键值对
        memset( keyname, 0, sizeof(keyname) );  

        sscanf( buf, "%[^=|^ |^\t]", keyname );

		//找到'='还不一定匹配
        if( strcmp(keyname, KeyName) != 0 )
        {
			continue;
		}
        sscanf( ++c, "%[^\n]", KeyVal ); 
        char *KeyVal_o = (char *)malloc(strlen(KeyVal) + 1);
		if(KeyVal_o == NULL)
		{
			//debug_printf("no enonght memory");
			fclose(fp);
			return -2;
		}
        memset(KeyVal_o, 0, strlen(KeyVal) + 1);  
        a_trim(KeyVal_o, KeyVal);
        if( strlen(KeyVal_o) > 0 )  
            strcpy(KeyVal, KeyVal_o); 

		if(KeyVal_o != NULL)
	        free(KeyVal_o);  
        KeyVal_o = NULL;
		found = 2;//标记成功找到键值对
        break;  

    }  
    fclose( fp );
    if( found == 2 ) 
    {
		return 0;  
	}
    else 
    {
        return -1;  
	}
} 


int conf_file_write(const char *profile,char *field,char* KeyName,char* KeyVal)  
{  

	// 存储读取一行的数据  
    int i = 0;  
	char *buf;
    int found=0; /* 1 AppName 2 KeyName */  
	int modify = 0;
    char buf_i[KEYVALLEN], buf_o[KEYVALLEN]; 
    char appname[32],keyname[32];  
    char lineBuff[256];

    memset(lineBuff,0,256);  
    FILE* fp  = fopen(profile,"r");  
    if(fp == NULL)  
    {
        perror("conf_file_write open fail:");  
		return -1;
	}

	//配置文件长度	
    fseek(fp,0,SEEK_END);  
    int configLen = ftell(fp); 
    fseek(fp,0,SEEK_SET);  
	//原文件长度 + 修改字符长度
    int configBufferLen = strlen(KeyVal);  
    char sumBuf[configBufferLen + configLen];  
    memset(sumBuf,0,sizeof(configBufferLen + configLen)); 
    memset( appname, 0, sizeof(appname) );  
    sprintf( appname,"[%s]", field );  
    while( !feof(fp) && fgets( buf_i, KEYVALLEN, fp )!=NULL )
    {  
    	//一旦把相应位置修改后，后面的行直接存入缓存，待写入新文件
		if(modify)
		{
			strcat(sumBuf,buf_i);
			continue;
		}


		//每读出一行就要先删掉前后空格符，无内容直接拷如缓存
		a_trim(buf_o,buf_i);
		if( strlen(buf_o) <= 0 )
		{
			strcat(sumBuf,buf_i);
			continue; 
		}

        buf = buf_o; 
		//搜索要修改的字段[字段名]，不管搜没搜到都要拷如缓存
        if( found == 0 )
		{  
            if( buf[0] != '['  || strncmp(buf,appname,strlen(appname))!=0)
			{  
				strcat(sumBuf,buf_i);
                continue;  
            }
			else
			{
				found = 1;
				strcat(sumBuf,buf_i);
				continue;
			}
        } 

		//上面搜索到[字段名]后首先要检查有没有注释行
		//注释行可用#,//,
        if( (buf[0] == '#') || buf[0] == '/')
		{  
			strcat(sumBuf,buf_i);
            continue;  
        } 

		//如果搜索到相应的[字段名]，潜在的注释行也都忽略了，后面却又冒出
		//一个字段[某字段]，说明当前字段下内容为空，退出
		if ( buf[0] == '[' )
        {  
        	//debug_printf("%s can not found in profile [ %s ]\n",KeyName,profile);
			fclose(fp);
            return -1;  
        }

		

		 // 没有匹配行  
        char* linePos = NULL;  
        linePos = strstr(buf,"=");
        if(linePos == NULL)  
        {  
            strcat(sumBuf,buf_i);  
            continue;  
        }  


		//找到'='，将'='左边的字符单独拷贝出来
        int lineNum = linePos - buf;  
        char lineName[lineNum + 1];
        char lineName_o[lineNum + 1];
        memset(lineName,0,sizeof(lineName)); 
        memset(lineName_o,0,sizeof(lineName_o)); 
        strncpy(lineName,buf,lineNum);
		a_trim(lineName_o,lineName);
		
		//debug_printf("lineName_o = %s\n",lineName_o);
		
        //"=" 前匹配
        //debug_printf("KeyName = %s,strlen = %d,lineName_o = %s,strlen = %d\n",KeyName,strlen(KeyName),lineName_o,strlen(lineName_o));
        if(strcmp(KeyName,lineName_o) == 0)  
        {  
            strcat(sumBuf,KeyName);  
            strcat(sumBuf," = ");  
            strcat(sumBuf,KeyVal);  
            strcat(sumBuf,"\n"); 
			modify = 1;
        }  
        else  
        {  
            strcat(sumBuf,buf_i);  
        }  
    }  
    fclose(fp); 
    remove(profile);  
    FILE* f = fopen(profile,"w+");  
    fputs(sumBuf,f);  
    fclose(f);
}  
/********************************************************************************************/
#endif
#if 0
static int GetUsrMsg(int eid, webs_t wp, int argc, char_t **argv)
{
	char ValTemp[48];
	char usrname[24];
	char password[24];
	char_t *name;
	memset(ValTemp,0,sizeof(ValTemp));
	memset(usrname,0,sizeof(usrname));
	memset(password,0,sizeof(password));
	conf_file_read(USRCONFIG,"username","user",usrname);
	conf_file_read(USRCONFIG,"password","password",password);
	if (ejArgs(argc, argv, T("%s"), &name) < 1) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}
	if(!strcmp(name,T("name")))
	{
		return websWrite(wp, T("%s"),usrname);
	}

	if(!strcmp(name,T("pass")))
	{
		return websWrite(wp, T("%s"),password);
	}

}
#endif
static int GetCheck(int eid, webs_t wp, int argc, char_t **argv)
{
	char ValTemp[4];
	char check[4];
	char reset[4];
	char pix[4];
	char playcontent[4];
	char ping[4];
	char_t *name;

	memset(ValTemp,0,sizeof(ValTemp));

	memset(check,0,sizeof(check));
	memset(reset,0,sizeof(reset));
	memset(pix,0,sizeof(pix));
	memset(playcontent,0,sizeof(playcontent));
	memset(ping,0,sizeof(ping));

	conf_file_read(FUNCONFIG,"check","check",check);
	//memcpy(ip,ValTemp,strlen(ValTemp));
	
	conf_file_read(FUNCONFIG,"reset","reset",reset);
	conf_file_read(FUNCONFIG,"pix","pixcheck",pix);
	conf_file_read(FUNCONFIG,"playcontent","playcontent",playcontent);
	conf_file_read(FUNCONFIG,"ping","ping",ping);
	if (ejArgs(argc, argv, T("%s"), &name) < 1) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	if(!strcmp(name,T("check")))
	{
		//printf("ip is %s\n",ValTemp);
		return websWrite(wp, T("%s"),check);
	}
	if(!strcmp(name,T("reset")))
	{
		//printf("netmask is %s\n",netmask);
		return websWrite(wp, T("%s"),reset);
	}

	if(!strcmp(name,T("pix")))
	{
		//printf("gateway is %s\n",gateway);
		return websWrite(wp, T("%s"),pix);
	}
	if(!strcmp(name,T("mode")))
	{
		//printf("port is %s\n",port);
		return websWrite(wp, T("%s"),playcontent);
	}	
	if(!strcmp(name,T("ping")))
	{
		//printf("port is %s\n",port);
		return websWrite(wp, T("%s"),ping);
	}	
}
static int GetBrightRange(int eid, webs_t wp, int argc, char_t **argv)
{
	char MaxBright[4];
	char MinBright[4];
	char_t *name;

	memset(MaxBright,0,sizeof(MaxBright));
	memset(MinBright,0,sizeof(MinBright));

	conf_file_read(FUNCONFIG,"bright","MaxBright",MaxBright);
	conf_file_read(FUNCONFIG,"bright","MinBright",MinBright);
	if (ejArgs(argc, argv, T("%s"), &name) < 1) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	if(!strcmp(name,T("MaxBright")))
	{
		//printf("prototcol is %s\n",protocol);
		return websWrite(wp, T("%s"),MaxBright);
	}
	if(!strcmp(name,T("MinBright")))
	{
		//printf("swr_protocol is %s\n",swr_protocol);
		return websWrite(wp, T("%s"),MinBright);
	}

}

static int GetBright(int eid, webs_t wp, int argc, char_t **argv)
{
	char daytime[4];
	char night[4];
	char_t *name;

	memset(daytime,0,sizeof(daytime));
	memset(night,0,sizeof(night));

	conf_file_read(BRIGHTCONFIG,"daytime","brightvlaue",daytime);
	conf_file_read(BRIGHTCONFIG,"night","brightvlaue",night);
	if (ejArgs(argc, argv, T("%s"), &name) < 1) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	if(!strcmp(name,T("daytime")))
	{
		//printf("prototcol is %s\n",protocol);
		return websWrite(wp, T("%s"),daytime);
	}
	if(!strcmp(name,T("night")))
	{
		//printf("swr_protocol is %s\n",swr_protocol);
		return websWrite(wp, T("%s"),night);
	}
}


static int GetConfig(int eid, webs_t wp, int argc, char_t **argv)
{
	char ValTemp[48];
	char ip[24];
	char netmask[24];
	char gateway[24];
	char port[8];
	char_t *name;

	memset(ValTemp,0,sizeof(ValTemp));

	memset(ip,0,sizeof(ip));
	memset(netmask,0,sizeof(netmask));
	memset(gateway,0,sizeof(gateway));
	memset(port,0,sizeof(port));

	conf_file_read(CLSCONFIG,"netport","ip",ValTemp);
	//memcpy(ip,ValTemp,strlen(ValTemp));
	
	conf_file_read(CLSCONFIG,"netport","netmask",netmask);
	conf_file_read(CLSCONFIG,"netport","gateway",gateway);
	conf_file_read(CLSCONFIG,"netport","port",port);
	if (ejArgs(argc, argv, T("%s"), &name) < 1) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	if(!strcmp(name,T("ip")))
	{
		//printf("ip is %s\n",ValTemp);
		return websWrite(wp, T("%s"),ValTemp);
	}
	if(!strcmp(name,T("netmask")))
	{
		//printf("netmask is %s\n",netmask);
		return websWrite(wp, T("%s"),netmask);
	}

	if(!strcmp(name,T("gateway")))
	{
		//printf("gateway is %s\n",gateway);
		return websWrite(wp, T("%s"),gateway);
	}
	if(!strcmp(name,T("port")))
	{
		//printf("port is %s\n",port);
		return websWrite(wp, T("%s"),port);
	}		
}


static int GetProtocol(int eid, webs_t wp, int argc, char_t **argv)
{
	char protocol[16];
	char swr_protocol[16];
	char_t *name;

	memset(protocol,0,sizeof(protocol));
	memset(swr_protocol,0,sizeof(swr_protocol));

	conf_file_read(PTCCONFIG,"protocol","protocol",protocol);
	conf_file_read(PTCCONFIG,"protocol","swr_protocol",swr_protocol);
	if (ejArgs(argc, argv, T("%s"), &name) < 1) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}

	if(!strcmp(name,T("protocol")))
	{
		//printf("prototcol is %s\n",protocol);
		return websWrite(wp, T("%s"),protocol);
	}
	if(!strcmp(name,T("swr_protocol")))
	{
		//printf("swr_protocol is %s\n",swr_protocol);
		return websWrite(wp, T("%s"),swr_protocol);
	}
}

static void formLogin(webs_t wp, char_t *path, char_t *query)
{
#if 0
	char buff[1024];
	char_t *Val;
	char username[24];
	char password[24];
	memset(username,0,sizeof(username));
	memset(password,0,sizeof(password));
	char *ConfigArr[]={
	"name",
	"pass",
	};
	int i,nLen=0;
	for(i=0;i<sizeof(ConfigArr)/sizeof(ConfigArr[0]);i++)
	{
		Val = websGetVar(wp, T(ConfigArr[i]), T("")); 
	}
#endif
}

static void formIP(webs_t wp, char_t *path, char_t *query)
{
	//FILE *fp = NULL;
	char buff[1024];
	char_t	*Val;
	char ip[24];
	char netmask[24];
	char gateway[24];
	char port[8];
	char Wcontent[256];

	memset(Wcontent,0,sizeof(Wcontent));
	memset(ip,0,sizeof(ip));
	memset(netmask,0,sizeof(netmask));
	memset(gateway,0,sizeof(gateway));
	memset(port,0,sizeof(port));
	char *ConfigArr[]=
	{
	"ip",
	"netmask",
	"gateway",
	"port",
	};
	int i;
	int nLen = 0;


	for(i=0;i<sizeof(ConfigArr)/sizeof(ConfigArr[0]);i++)
	{
		
		Val = websGetVar(wp, T(ConfigArr[i]), T("")); 

		if(i==0)
		{
			strcpy(ip,Val);
		}
		if(i==1)
		{
			strcpy(netmask,Val);
		}
		if(i==2)
		{
			strcpy(gateway,Val);
		}
		if(i==3)
		{
			strcpy(port,Val);
		}
		//nLen += sprintf(&buff[nLen],"%s=%s&",ConfigArr[i],Val);
	}
	int fd = open(IPCONFIG,O_RDWR | O_CREAT,0744);
	//printf("ip is %s netmask is %s gateway is %s port is %s\n",ip,netmask,gateway,port);
	//printf("%s %d buff:%s \n",__func__,__LINE__,buff);
	if(fd >= 0)
	{

		//这里修改两个文件
		conf_file_write(CLSCONFIG,"netport","ip",ip);
		conf_file_write(CLSCONFIG,"netport","netmask",netmask);
		conf_file_write(CLSCONFIG,"netport","gateway",gateway);
		conf_file_write(CLSCONFIG,"netport","port",port);

		sprintf(Wcontent,"#!/bin/sh\n\nifconfig eth0 %s netmask %s up >/dev/null 2>&1\n/sbin/route add default gw %s\n",ip,netmask,gateway);

		lseek(fd,0,SEEK_SET);
		write(fd,Wcontent,strlen(Wcontent));		
		close(fd);
		
	}
	websRedirect(wp, "ip.asp");

	system("reboot");
}

static void formCheck(webs_t wp, char_t *path, char_t *query)
{
	char buff[1024];
	char_t	*Val;
	char check[4];
	char reset[4];
	char pix[4];
	char playcontent[4];
	char ping[4];
	memset(buff,0,sizeof(buff));
	memset(check,0,sizeof(check));
	memset(reset,0,sizeof(reset));
	memset(pix,0,sizeof(pix));
	memset(playcontent,0,sizeof(playcontent));
	memset(ping,0,sizeof(ping));
	
	char *ConfigArr[]=
	{
	"mode",
	"check",
	"reset",
	"pix",
	"ping",
	};
	int i;
	int nLen = 0;

	for(i=0;i<sizeof(ConfigArr)/sizeof(ConfigArr[0]);i++)
	{
		
		Val = websGetVar(wp, T(ConfigArr[i]), T("")); 

		if(i==0)
		{
			strcpy(playcontent,Val);
		}
		if(i==1)
		{
			strcpy(check,Val);
		}
		if(i==2)
		{
			strcpy(reset,Val);
		}
		if(i==3)
		{
			strcpy(pix,Val);
		}
		if(i==4)
		{
			strcpy(ping,Val);
		}
		//nLen += sprintf(&buff[nLen],"%s=%s&",ConfigArr[i],Val);
	}
	int fd = open(FUNCONFIG,O_RDWR | O_CREAT,0744);
	if(fd >= 0)
	{

		//这里修改两个文件
		conf_file_write(FUNCONFIG,"playcontent","playcontent",playcontent);
		conf_file_write(FUNCONFIG,"check","check",check);
		conf_file_write(FUNCONFIG,"reset","reset",reset);
		conf_file_write(FUNCONFIG,"pix","pixcheck",pix);
		conf_file_write(FUNCONFIG,"ping","ping",ping);
	}

	//system("reset && killall ledscreen");
	websRedirect(wp, "config.asp");
}


static void formTime(webs_t wp, char_t *path, char_t *query)
{
	int i;
	char_t *Val = NULL;
	char time[64];
	char year[8];
	char mon[4];
	char day[4];
	char hour[4];
	char min[4];
	char sec[4];
	memset(time,0,sizeof(time));
	memset(year,0,sizeof(year));
	memset(mon,0,sizeof(mon));
	memset(day,0,sizeof(day));
	memset(hour,0,sizeof(hour));
	memset(min,0,sizeof(min));
	memset(sec,0,sizeof(sec));
	char *ConfigArr[]=
	{
	"year",
	"mon",
	"day",
	"hour",
	"min",
	"sec",
	};
	for(i=0;i<sizeof(ConfigArr)/sizeof(ConfigArr[0]);i++)
	{
		
		Val = websGetVar(wp, T(ConfigArr[i]), T("")); 
		printf("%s ",Val);
		if(i==0)
		{
			strncpy(year,Val,4);
			year[4] = '\0';
		}
		if(i==1)
		{
			strcpy(mon,Val);
		}
		if(i==2)
		{
			strcpy(day,Val);
		}
		if(i==3)
		{
			strcpy(hour,Val);
		}
		if(i==4)
		{
			strcpy(min,Val);
		}
		if(i==5)
		{
			strcpy(sec,Val);
		}
	}
	
	sprintf(time,"date -s \"%s-%s-%s %s:%s:%s\" > /home/LEDscr/config/date.txt",year,mon,day,hour,min,sec);
	//printf("%s\n",time);
	system(time);
	websRedirect(wp, "config.asp");
}

static void formReboot(webs_t wp, char_t *path, char_t *query)
{	

	system("reset && killall ledscreen");
	websRedirect(wp, "config.asp");
}

static void formBright(webs_t wp, char_t *path, char_t *query)
{
	//char buff[1024];
	char_t	*Val = NULL;
	char daytime[4];
	char night[4];

	memset(daytime,0,sizeof(daytime));
	memset(night,0,sizeof(night));
	
	char *ConfigArr[]=
	{
	"daytime",
	"night",
	};
	int i;
	int nLen = 0;

	for(i=0;i<sizeof(ConfigArr)/sizeof(ConfigArr[0]);i++)
	{
		
		Val = websGetVar(wp, T(ConfigArr[i]), T("")); 

		if(i==0)
		{
			strcpy(daytime,Val);
		}
		if(i==1)
		{
			strcpy(night,Val);
		}

		//nLen += sprintf(&buff[nLen],"%s=%s&",ConfigArr[i],Val);
	}
	int fd = open(BRIGHTCONFIG,O_RDWR | O_CREAT,0744);
	if(fd >= 0)
	{
		//这里修改两个文件
		conf_file_write(BRIGHTCONFIG,"daytime","brightvlaue",daytime);
		conf_file_write(BRIGHTCONFIG,"night","brightvlaue",night);
	}

	
	websRedirect(wp, "config.asp");
}
static void formBrightRange(webs_t wp, char_t *path, char_t *query)
{
	//char buff[1024];
	char_t	*Val = NULL;
	char MaxBright[4];
	char MinBright[4];

	memset(MaxBright,0,sizeof(MaxBright));
	memset(MinBright,0,sizeof(MinBright));
	
	char *ConfigArr[]=
	{
	"MaxBright",
	"MinBright",
	};
	int i;
	int nLen = 0;

	for(i=0;i<sizeof(ConfigArr)/sizeof(ConfigArr[0]);i++)
	{
		
		Val = websGetVar(wp, T(ConfigArr[i]), T("")); 

		if(i==0)
		{
			strcpy(MaxBright,Val);
		}
		if(i==1)
		{
			strcpy(MinBright,Val);
		}

		//nLen += sprintf(&buff[nLen],"%s=%s&",ConfigArr[i],Val);
	}
	int fd = open(FUNCONFIG,O_RDWR | O_CREAT,0744);
	if(fd >= 0)
	{
		//这里修改两个文件
		conf_file_write(FUNCONFIG,"bright","MaxBright",MaxBright);
		conf_file_write(FUNCONFIG,"bright","MinBright",MinBright);
	}

	
	websRedirect(wp, "config.asp");
}




static void formProtocol(webs_t wp, char_t *path, char_t *query)
{
	char buff[1024];
	char_t	*Val;
	char protocol[16];
	char swr_protocol[16];
	char Wcontent[256];

	memset(Wcontent,0,sizeof(Wcontent));
	memset(protocol,0,sizeof(protocol));
	memset(swr_protocol,0,sizeof(swr_protocol));
	char *ConfigArr[]=
	{
	"protocol",
	"swr_protocol",
	};
	int i;
	int nLen = 0;


	for(i=0;i<sizeof(ConfigArr)/sizeof(ConfigArr[0]);i++)
	{
		
		Val = websGetVar(wp, T(ConfigArr[i]), T("")); 

		if(i==0)
		{
			strcpy(protocol,Val);
		}
		if(i==1)
		{
			strcpy(swr_protocol,Val);
		}
		//nLen += sprintf(&buff[nLen],"%s=%s&",ConfigArr[i],Val);
	}
	int fd = open(PTCCONFIG,O_RDWR | O_CREAT,0744);
	if(fd >= 0)
	{

		//这里修改两个文件
		conf_file_write(PTCCONFIG,"protocol","protocol",protocol);
		conf_file_write(PTCCONFIG,"protocol","swr_protocol",swr_protocol);
		//add 播放类表的切换
		if(strncmp(protocol,"jinxiao",7) == 0)
		{
			system("cp /home/LEDscr/protocol/jinxiao/play.lst /home/LEDscr/list/");
			conf_file_write(CLSCONFIG,"playlist","list","play.lst");
			//printf("protocol is %s\n",protocol);
		}

		else if(strncmp(protocol,"seewor",6) == 0)
		{
			if(strncmp(swr_protocol,"bozhou",6) == 0)
			{
				system("cp /home/LEDscr/protocol/seewor/bozhou/000.xkl /home/LEDscr/list/");
				conf_file_write(CLSCONFIG,"playlist","list","000.xkl");
				//printf("swr_protocol is %s\n",swr_protocol);
			}
			else if(strncmp(swr_protocol,"fuzhou",6) == 0)
			{
				system("cp /home/LEDscr/protocol/seewor/fuzhou/000.xkl /home/LEDscr/list/");
				conf_file_write(CLSCONFIG,"playlist","list","000.xkl");
				//printf("swr_protocol is %s\n",swr_protocol);
			}
			else if(strncmp(swr_protocol,"heao",4) == 0)
			{
				system("cp /home/LEDscr/protocol/seewor/heao/000.xkl /home/LEDscr/list/");
				conf_file_write(CLSCONFIG,"playlist","list","000.xkl");
				//printf("swr_protocol is %s\n",swr_protocol);
			}
			else if(strncmp(swr_protocol,"hebeierqin",10) == 0)
			{
				system("cp /home/LEDscr/protocol/seewor/hebeierqin/000.xkl /home/LEDscr/list/");
				conf_file_write(CLSCONFIG,"playlist","list","000.xkl");
				//printf("swr_protocol is %s\n",swr_protocol);
			}
			else if(strncmp(swr_protocol,"malaysia",8) == 0)
			{
				system("cp /home/LEDscr/protocol/seewor/malaysia/000.xkl /home/LEDscr/list/");
				conf_file_write(CLSCONFIG,"playlist","list","000.xkl");				
				//printf("swr_protocol is %s\n",swr_protocol);
			}
			else if(strncmp(swr_protocol,"zhuhaiproj",10) == 0)
			{
				system("cp /home/LEDscr/protocol/seewor/zhuhaiproj/000.xkl /home/LEDscr/list/");
				conf_file_write(CLSCONFIG,"playlist","list","000.xkl");					
				//printf("swr_protocol is %s\n",swr_protocol);
			}
			else
			{
				system("cp /home/LEDscr/protocol/seewor/000.xkl /home/LEDscr/list/");
				conf_file_write(CLSCONFIG,"playlist","list","000.xkl");
				//printf("swr_protocol is %s\n",swr_protocol);
			}				
		}
		else if(strncmp(protocol,"chengdu",7) == 0)
		{
			system("cp /home/LEDscr/protocol/chengdu/001.lst /home/LEDscr/list/");
			conf_file_write(CLSCONFIG,"playlist","list","001.lst");
			//printf("protocol is %s\n",protocol);
		}

		else if(strncmp(protocol,"xiamen",6) == 0)
		{
			system("cp /home/LEDscr/protocol/xiamen/play.lst /home/LEDscr/list/");
			conf_file_write(CLSCONFIG,"playlist","list","play.lst");
			//printf("protocol is %s\n",protocol);
		}
		else if(strncmp(protocol,"zhichao",7) == 0)
		{
			system("cp /home/LEDscr/protocol/zhichao/play.lst /home/LEDscr/list/");
			conf_file_write(CLSCONFIG,"playlist","list","play.lst");
			//printf("protocol is %s\n",protocol);
		}
		else if(strncmp(protocol,"zhonghaizc",10) == 0)
		{
			system("cp /home/LEDscr/protocol/zhonghaizc/play.lst /home/LEDscr/list/");
			conf_file_write(CLSCONFIG,"playlist","list","play.lst");
			//printf("protocol is %s\n",protocol);
		}
		else if(strncmp(protocol,"perplelight",11) == 0)
		{
			system("cp /home/LEDscr/protocol/perplelight/play.lst /home/LEDscr/list/");
			conf_file_write(CLSCONFIG,"playlist","list","play.lst");
			//printf("protocol is %s\n",protocol);
		}
		else if(strncmp(protocol,"modbus",6) == 0)
		{
			system("cp /home/LEDscr/protocol/modbus/modbus.lst /home/LEDscr/list/");
			conf_file_write(CLSCONFIG,"playlist","list","modbus.lst");
			//printf("protocol is %s\n",protocol);
		}

		//升级相关
		else if(strncmp(protocol,"upgrade",7) == 0)
		{
			//printf("protocol is %s\n",protocol);
		}

		else
		{
			system("cp /home/LEDscr/protocol/seewor/000.xkl /home/LEDscr/list/");
			conf_file_write(CLSCONFIG,"playlist","list","000.xkl");
			//printf("protocol is %s\n",protocol);
		}			
	}
	websRedirect(wp, "protocol.asp");
	system("reset && killall ledscreen");	
	//system("reboot");
}



/*********************************** Code *************************************/
/*
 *	Main -- entry point from LINUX
 */
static void websversion(void)
{
	FILE *fp = NULL;
	char version[24];
	memset(version,0,sizeof(version));
	sprintf(version,"webs_V%d.%d.%d\n",MAJOR,MINOR,REVISION);
	if(access("/home/LEDscr/version/",F_OK) < 0)
		return;
	fp = fopen("/home/LEDscr/version/web.v","w+");
	if(fp < 0)
	{
	 	return;
	}
	char Rversion[24];
	memset(Rversion,0,sizeof(Rversion));
	fread(Rversion,1,sizeof(Rversion),fp);
	if(strncmp(Rversion,version,strlen(version)) == 0)
	{
		fclose(fp);
		return;
	}
	fwrite(version,1,strlen(version),fp);
	fflush(fp);
	fclose(fp);
}

int main(int argc, char** argv)
{
	int i, demo = 0;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-demo") == 0) {
			demo++;
		}
	}
	websversion();

	
/*
 *	Initialize the memory allocator. Allow use of malloc and start 
 *	with a 60K heap.  For each page request approx 8KB is allocated.
 *	60KB allows for several concurrent page requests.  If more space
 *	is required, malloc will be used for the overflow.
 */
	bopen(NULL, (60 * 1024), B_USE_MALLOC);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, sigintHandler);
	signal(SIGTERM, sigintHandler);

/*
 *	Initialize the web server
 初始化用户管理部分，打开 web 服务器，注册 URL 处理函数。
 用户管理部分在 um.c 中实现， 
 Web 服务器的初始化是在 default.c 和 webs.c 中实现
 url 处理函数在 handler.c 中实现
 */
	if (initWebs(demo) < 0) {
		return -1;
	}

#ifdef WEBS_SSL_SUPPORT
	websSSLOpen();
/*	websRequireSSL("/"); */	/* Require all files be served via https */
#endif

/*
 *	Basic event loop. SocketReady returns true when a socket is ready for
 *	service. SocketSelect will block until an event occurs. SocketProcess
 *	will actually do the servicing.
 主循环
 socketReady()函数检查是否有准备好的 sock 事件
 socketSelect()函数首先把各个 sock 感兴趣的事件 （sp->handlerMask）注册给三个集合（读， 写，例外），
 然后调用 select 系统调用，然后更新各个 sock 的 sp->currentEvents，表示各个 sock 的当 前状态。
 这两个函数在sockGen.c中实现，他们主要操作的数据是socket_t变量 socketList中的handlerMask 和 currentEvents，
 socketList 在 sock.c 中定义并主要由该文件中的 socketAlloc，socketFree 和 socketPtr 三个函数维护。
 该函数处理具体的 sock 事件 
 1，调用 socketReady(sid)对 socketList[sid]进行检查，看是否有 sock 事件 
 2，如果有 sock 事件，则调用 socketDoEvent()函数，对事件进行处理
 */
	finished = 0;
	while (!finished) {
		if (socketReady(-1) || socketSelect(-1, 1000)) {
			socketProcess(-1);
		}
		websCgiCleanup();
		emfSchedProcess();
	}

#ifdef WEBS_SSL_SUPPORT
	websSSLClose();
#endif

#ifdef USER_MANAGEMENT_SUPPORT
	umClose();
#endif

/*
 *	Close the socket module, report memory leaks and close the memory allocator
 */
	websCloseServer();
	socketClose();
#ifdef B_STATS
	memLeaks();
#endif
	bclose();
	return 0;
}

/*
 *	Exit cleanly on interrupt
 */
static void sigintHandler(int unused)
{
	finished = 1;
}

/******************************************************************************/
/*
 *	Initialize the web server.
 */
	int get_local_ip(const char *eth_inf, char *ip)  
	{  
			#define MAC_SIZE    18  
			#define IP_SIZE     16
			int sd;  
	 		struct sockaddr_in sin;  
	 		struct ifreq ifr;  
	 	
	 		sd = socket(AF_INET, SOCK_DGRAM, 0);	
	 		if (-1 == sd)  
	 		{  
	 				//printf("socket error: %s\n", strerror(errno));	
	 				return -1;				
	 		}  
	 	
	 		strncpy(ifr.ifr_name, eth_inf, IFNAMSIZ);  
	 		ifr.ifr_name[IFNAMSIZ - 1] = 0;  
	 			
	 		// if error: No such device  
	 		if (ioctl(sd, SIOCGIFADDR, &ifr) < 0)  
	 		{  
	 				//printf("ioctl error: %s\n", strerror(errno));  
	 				close(sd);	
	 				return -1;	
	 		}  
	 	
	 		memcpy(&sin, &ifr.ifr_addr, sizeof(sin));  
	 		snprintf(ip, IP_SIZE, "%s", inet_ntoa(sin.sin_addr));  
	 	
	 		close(sd);	
	 		return 0;  
	 }

static int initWebs(int demo)
{
	struct hostent	*hp;
	struct in_addr	intaddr;
	char			host[128], dir[128], webdir[128];
	char			*cp;
	char_t			wbuf[128];
	char ip[128]={0};

/*
 *	Initialize the socket subsystem
 */
	socketOpen();

#ifdef USER_MANAGEMENT_SUPPORT
/*
 *	Initialize the User Management database
 */
	umOpen();
	umRestore(T("umconfig.txt"));
#endif

/*
 *	Define the local Ip address, host name, default home page and the 
 *	root web directory.
 */
	/*if (gethostname(host, sizeof(host)) < 0) {
		error(E_L, E_LOG, T("Can't get hostname"));
		return -1;
	}
	if ((hp = gethostbyname(host)) == NULL) {
		error(E_L, E_LOG, T("Can't get host address"));
		return -1;
	}
	memcpy((char *) &intaddr, (char *) hp->h_addr_list[0],
		(size_t) hp->h_length);
		*/
		do
		{
			get_local_ip("eth0", ip);
			//get_local_ip("ens33", ip);
			sleep(1);
		}
		while(ip[0] == 0);
		
		intaddr.s_addr = inet_addr(ip);

/*
 *	Set ../web as the root web. Modify this to suit your needs
 *	A "-demo" option to the command line will set a webdemo root
 */
	/*getcwd(dir, sizeof(dir)); 
	if ((cp = strrchr(dir, '/'))) {
		*cp = '\0';
	}
	if (demo) {
		sprintf(webdir, "%s/%s", dir, demoWeb);
	} else {
		sprintf(webdir, "%s/%s", dir, rootWeb);
	}*/
	sprintf(webdir, "%s", rootWeb);
	//printf("%s %d %s \n",__func__,__LINE__,webdir);
/*
 *	Configure the web server options before opening the web server
 */
	websSetDefaultDir(webdir);
	cp = inet_ntoa(intaddr);
	ascToUni(wbuf, cp, min(strlen(cp) + 1, sizeof(wbuf)));
	websSetIpaddr(wbuf);
	//printf("%s %d %s \n",__func__,__LINE__,wbuf);
	ascToUni(wbuf, host, min(strlen(host) + 1, sizeof(wbuf)));
	websSetHost(wbuf);
/*
 *	Configure the web server options before opening the web server
 */
	websSetDefaultPage(T("default.asp"));
	websSetPassword(password);

/* 
 *	Open the web server on the given port. If that port is taken, try
 *	the next sequential port for up to "retries" attempts.
 */
	websOpenServer(port, retries);

/*
 * 	First create the URL handlers. Note: handlers are called in sorted order
 *	with the longest path handler examined first. Here we define the security 
 *	handler, forms handler and the default web page handler.
 */
	websUrlHandlerDefine(T(""), NULL, 0, websSecurityHandler, WEBS_HANDLER_FIRST);
	websUrlHandlerDefine(T("/goform"), NULL, 0, websFormHandler, 0);
	websUrlHandlerDefine(T("/cgi-bin"), NULL, 0, websCgiHandler, 0);
	websUrlHandlerDefine(T(""), NULL, 0, websDefaultHandler, WEBS_HANDLER_LAST); 

/*
 *	Now define two test procedures. Replace these with your application
 *	relevant ASP script procedures and form functions.
 */
	//websAspDefine(T("aspTest"), aspTest);
	websAspDefine(T("GetFileList"), GetFileList);
	websAspDefine(T("GetConfig"), GetConfig);
	//websAspDefine(T("GetUsrMsg"), GetUsrMsg);
	websAspDefine(T("GetCheck"),GetCheck);
	websAspDefine(T("GetProtocol"),GetProtocol);
	//add by mo 20201224
	websAspDefine(T("GetBright"),GetBright);
	websAspDefine(T("GetBrightRange"),GetBrightRange);
	websFormDefine(T("formLogin"),formLogin);                   //add by mo 2020.6.29
	websFormDefine(T("formUploadFileTest"), formUploadFileTest);// add by mo 2020.5.15
	//websFormDefine(T("formTest"), formTest);
	websFormDefine(T("formIP"), formIP);
	websFormDefine(T("formProtocol"),formProtocol);
	websFormDefine(T("formCheck"),formCheck);//add by mo 2020.12.23
	websFormDefine(T("formBright"),formBright);
	websFormDefine(T("formBrightRange"),formBrightRange);
	websFormDefine(T("formTime"),formTime);
	websFormDefine(T("formReboot"),formReboot);
/*
 *	Create the Form handlers for the User Management pages
 */
#ifdef USER_MANAGEMENT_SUPPORT
	formDefineUserMgmt();
#endif

/*
 *	Create a handler for the default home page
 */
	websUrlHandlerDefine(T("/"), NULL, 0, websHomePageHandler, 0); 
	return 0;
}

/******************************************************************************/
/*
 *	Test Javascript binding for ASP. This will be invoked when "aspTest" is
 *	embedded in an ASP page. See web/asp.asp for usage. Set browser to 
 *	"localhost/asp.asp" to test.
 */
char nametest[64];
char addrtest[64];
static int aspTest(int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*name, *address;

	/*if (ejArgs(argc, argv, T("%s %s"), &name, &address) < 2) {
		websError(wp, 400, T("Insufficient args\n"));
		return -1;
	}*/
	//printf("%s %d %s %s \n",__func__,__LINE__,nametest,addrtest);
	if(0 == strcmp(argv[0],"name"))
	{
		return websWrite(wp, T("%s"), nametest);
	}
	else if(0 == strcmp(argv[0],"address"))
	{
		return websWrite(wp, T("%s"), addrtest);
	}
	return websWrite(wp, T("%s,%s"), nametest,addrtest);
}

/******************************************************************************/
/*
 * for test html upload file to web server
 * add by gyr 2011.10.15
 */

static void formUploadFileTest(webs_t wp, char_t *path, char_t *query)
{
    FILE *       fp;
    char_t *     fn;
    char_t *     bn = NULL;
    int          locWrite;
    int          numLeft;
    int          numWrite;
	char_t filepath[128]; //升级包存放路径
	memset(path,0,sizeof(filepath));
	//printf("\n...................formUploadFileTest...................\n\n");

    a_assert(websValid(wp));
    websHeader(wp);

    fn = websGetVar(wp, T("filename"), T(""));
    if (fn != NULL && *fn != '\0') {
        if ((int)(bn = gstrrchr(fn, '/') + 1) == 1) {
            if ((int)(bn = gstrrchr(fn, '\\') + 1) == 1) {
                bn = fn;
            }
        }
    }
	//这里只要是ledscreen开头的都重命名为ledscreen
	if(strncmp(fn,"ledscreen",9) == 0)
	{
		strcpy(filepath,"/home/LEDscr/upgrade/ledscreen");
	}
	else if(strncmp(fn,"webs",4) == 0)
	{
		strcpy(filepath,"/home/LEDscr/upgrade/webs");
	}
	else if(strncmp(fn,"bootup",6) == 0)
	{
		strcpy(filepath,"/home/LEDscr/upgrade/bootup");
	}
	else if(strncmp(fn,"rx",2) == 0 || strncmp(fn,"RX",2) == 0)
	{
		strcpy(filepath,"/home/LEDscr/upgrade/RX_CARD.bin");
	}
	else if(strncmp(fn,"tx",2) == 0 || strncmp(fn,"TX",2) == 0)
	{
		strcpy(filepath,"/home/LEDscr/upgrade/TX_CARD.bin");
	}
	else if(strncmp(fn,"cls.conf",8) == 0)
	{
		strcpy(filepath,"/home/LEDscr/config/cls.conf");
	}
	else
	{
		websWrite(wp, T("File is not upgrade-file!<br>"));
		return;
	}
	
   websWrite(wp, T("Filename = %s<br>Size = %d bytes<br>"), bn, wp->lenPostData);

    if ((fp = fopen(filepath, "w+b")) == NULL) {
        websWrite(wp, T("File open failed!<br>"));
    } else {
        locWrite = 0;
        numLeft = wp->lenPostData;
        while (numLeft > 0) {
            numWrite = fwrite(&(wp->postData[locWrite]), sizeof(*(wp->postData)), numLeft, fp);
            if (numWrite < numLeft) {
                websWrite(wp, T("File write failed.<br>  ferror=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes<br>"), ferror(fp), locWrite, numLeft, numWrite, wp->lenPostData);
            break;
            }
            locWrite += numWrite;
            numLeft -= numWrite;
        }

        if (numLeft == 0) {
            if (fclose(fp) != 0) {
                websWrite(wp, T("File close failed.<br>  errno=%d locWrite=%d numLeft=%d numWrite=%d Size=%d bytes<br>"), errno, locWrite, numLeft, numWrite, wp->lenPostData);
            } else {
                websWrite(wp, T("File Size Written = %d bytes<br>"), wp->lenPostData);
				//加入升级
				#if 1
				if(strncmp(fn,"ledscreen",9) == 0)
				{
					system("rm /bin/ledscreen");
					
					system("cp /home/LEDscr/upgrade/ledscreen /bin/ledscreen");

					system("cp /home/LEDscr/upgrade/ledscreen /home/LEDscr/ledscreen");

					system("chmod 774 /bin/ledscreen /home/LEDscr/ledscreen");
					usleep(10000);
					system("reset && killall ledscreen");	
					
				}
				else if(strncmp(fn,"bootup",6) == 0)
				{
					system("rm /home/LEDscr/bootup");
					
					system("cp /home/LEDscr/upgrade/bootup /home/LEDscr/");

					system("cp /home/LEDscr/upgrade/bootup /home/LEDscr/boot/");

					system("chmod 774 /home/LEDscr/bootup /home/LEDscr/boot/bootup");
					usleep(10000);
					//system("reboot");
					
				}
				else if(strncmp(fn,"webs",4) == 0)
				{
					system("rm /home/LEDscr/webs");
					system("cp /home/LEDscr/upgrade/webs /home/LEDscr/");
					system("chmod 774 /home/LEDscr/webs");
					usleep(10000);
					//system("reboot");
				}
				#endif
            }
        } else {
          websWrite(wp, T("numLeft=%d locWrite=%d Size=%d bytes<br>"), numLeft, locWrite, wp->lenPostData);

		}
    }
//change by mo 20201224
   // websFooter(wp);
    //websDone(wp, 200);
    websRedirect(wp, "upload.asp");
	
}

/******************************************************************************/

/*
 *	Home page handler
 */

static int websHomePageHandler(webs_t wp, char_t *urlPrefix, char_t *webDir,
	int arg, char_t *url, char_t *path, char_t *query)
{
/*
 *	If the empty or "/" URL is invoked, redirect default URLs to the home page
 */
	if (*url == '\0' || gstrcmp(url, T("/")) == 0) {
		websRedirect(wp, WEBS_DEFAULT_HOME);
		return 1;
	}
	return 0;
}

/******************************************************************************/

#ifdef B_STATS
static void memLeaks() 
{
	int		fd;

	if ((fd = gopen(T("leak.txt"), O_CREAT | O_TRUNC | O_WRONLY, 0666)) >= 0) {
		bstats(fd, printMemStats);
		close(fd);
	}
}

/******************************************************************************/
/*
 *	Print memory usage / leaks
 */

static void printMemStats(int handle, char_t *fmt, ...)
{
	va_list		args;
	char_t		buf[256];

	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	write(handle, buf, strlen(buf));
}
#endif

/******************************************************************************/

