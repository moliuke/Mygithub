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

/*   ɾ����ߵĿո�   */  
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
  
/*   ɾ���ұߵĿո�   */  
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
  
/*   ɾ�����ߵĿո�   */  
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
		//û��ȡ��һ�����ݣ����Ȱ���ߵĿո�ȥ��
       // l_trim(buf_o, buf_i); 
		//r_trim(buf_o,buf_o);
		a_trim(buf_o,buf_i);
        if( strlen(buf_o) <= 0 )  
            continue; 
        buf = buf_o;  
  		//����������[�ֶ���]
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
		//����������[�ֶ���]������Ҫ�����û��ע����
		//ע���п���#,//,
        if( (buf[0] == '#') || buf[0] == '/')
		{  
            continue;  
        } 

		//�����������Ӧ��[�ֶ���]��Ǳ�ڵ�ע����Ҳ�������ˣ�����ȴ��ð��
		//һ���ֶ�[ĳ�ֶ�]��˵����ǰ�ֶ�������Ϊ�գ��˳�
		if ( buf[0] == '[' )
        {  
        	debug_printf("%s can not found in profile [ %s ]\n",KeyName,profile);
			fclose(fp);
            return -1;  
        }

		//������ÿһ�о�Ӧ���Ǽ�ֵ�Ե�����


		//'='���Ҳ�����ֻ�ܼ�������һ��
        if( (c = (char*)strchr(buf, '=')) == NULL )  
            continue;  

		//���������ҵ�'=','='����ֵ��
        memset( keyname, 0, sizeof(keyname) );  

        sscanf( buf, "%[^=|^ |^\t]", keyname );

		//�ҵ�'='����һ��ƥ��
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
		found = 2;//��ǳɹ��ҵ���ֵ��
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

	// �洢��ȡһ�е�����  
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

	//�����ļ�����	
    fseek(fp,0,SEEK_END);  
    int configLen = ftell(fp); 
    fseek(fp,0,SEEK_SET);  
	//ԭ�ļ����� + �޸��ַ�����
    int configBufferLen = strlen(KeyVal);  
    char sumBuf[configBufferLen + configLen];  
    memset(sumBuf,0,sizeof(configBufferLen + configLen)); 
    memset( appname, 0, sizeof(appname) );  
    sprintf( appname,"[%s]", field );  
    while( !feof(fp) && fgets( buf_i, KEYVALLEN, fp )!=NULL )
    {  
    	//һ������Ӧλ���޸ĺ󣬺������ֱ�Ӵ��뻺�棬��д�����ļ�
		if(modify)
		{
			strcat(sumBuf,buf_i);
			continue;
		}


		//ÿ����һ�о�Ҫ��ɾ��ǰ��ո����������ֱ�ӿ��绺��
		a_trim(buf_o,buf_i);
		if( strlen(buf_o) <= 0 )
		{
			strcat(sumBuf,buf_i);
			continue; 
		}

        buf = buf_o; 
		//����Ҫ�޸ĵ��ֶ�[�ֶ���]��������û�ѵ���Ҫ���绺��
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

		//����������[�ֶ���]������Ҫ�����û��ע����
		//ע���п���#,//,
        if( (buf[0] == '#') || buf[0] == '/')
		{  
			strcat(sumBuf,buf_i);
            continue;  
        } 

		//�����������Ӧ��[�ֶ���]��Ǳ�ڵ�ע����Ҳ�������ˣ�����ȴ��ð��
		//һ���ֶ�[ĳ�ֶ�]��˵����ǰ�ֶ�������Ϊ�գ��˳�
		if ( buf[0] == '[' )
        {  
        	debug_printf("%s can not found in profile [ %s ]\n",KeyName,profile);
			fclose(fp);
            return -1;  
        }

		

		 // û��ƥ����  
        char* linePos = NULL;  
        linePos = strstr(buf,"=");
        if(linePos == NULL)  
        {  
            strcat(sumBuf,buf_i);  
            continue;  
        }  


		//�ҵ�'='����'='��ߵ��ַ�������������
        int lineNum = linePos - buf;  
        char lineName[lineNum + 1];
        char lineName_o[lineNum + 1];
        memset(lineName,0,sizeof(lineName)); 
        memset(lineName_o,0,sizeof(lineName_o)); 
        strncpy(lineName,buf,lineNum);
		a_trim(lineName_o,lineName);
		
		debug_printf("lineName_o = %s\n",lineName_o);
		
        //"=" ǰƥ��
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

