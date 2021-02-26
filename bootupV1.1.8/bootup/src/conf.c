#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <assert.h>  
#include <ctype.h>  
#include <errno.h> 
#include <unistd.h>
#include "conf.h" 
#include "debug.h"
#include "config.h"


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
    assert(szInput != NULL);  
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
        debug_printf( "openfile [%s] error [%s]\n",profile,strerror(errno) );  
        return(-1);  
    }
    memset( appname, 0, sizeof(appname) );  
    sprintf( appname,"[%s]", field );  
	
    fseek( fp, 0, SEEK_SET );  
    while( !feof(fp) && fgets( buf_i, KEYVALLEN, fp )!=NULL )
	{  
		//没读取到一行内容，首先把左边的空格去掉
       // l_trim(buf_o, buf_i); 
		//r_trim(buf_o,buf_o);
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
        	debug_printf("%s can not found in profile [ %s ]\n",KeyName,profile);
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
			debug_printf("no enonght memory");
			fclose(fp);
			return -2;
		}
        memset(KeyVal_o, 0, strlen(KeyVal) + 1);  
        a_trim(KeyVal_o, KeyVal);
        if( strlen(KeyVal_o) > 0 )  
            strcpy(KeyVal, KeyVal_o); 
		
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
        perror("open file\n");  

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
        	debug_printf("%s can not found in profile [ %s ]\n",KeyName,profile);
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
		
		debug_printf("lineName_o = %s\n",lineName_o);
		
        //"=" 前匹配
        debug_printf("KeyName = %s,strlen = %d,lineName_o = %s,strlen = %d\n",KeyName,strlen(KeyName),lineName_o,strlen(lineName_o));
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

#if 0
int main()  
{  
    char ip[16];
	char dev_width[16];
	char dev_height[16];
	char list[16];
	char upper_flowctl[24];
    conf_file_read("./cls.conf", "serial", "upper_flowctl", upper_flowctl);  
    debug_printf("upper_flowctl = %s\n",upper_flowctl);  

	//conf_file_write("./cls.conf","screen","box_width","64");
	
    return 0;  
}  
#endif

