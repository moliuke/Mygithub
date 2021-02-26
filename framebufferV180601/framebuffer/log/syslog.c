#include "syslog.h"


#define Logdir		"/home/LEDscr/log"

Logmsg_t Derror,Dwarn,Dinfo,Ddebug;

static void getSysTime(char* timeStr)  
{  
	time_t timer;
	struct tm* t_tm; 

	char timestr[28];

	time(&timer);	
	t_tm = localtime(&timer);
	snprintf(timeStr,20,"%04d/%02d/%02d %02d:%02d:%02d", t_tm->tm_year+1900,	 
	t_tm->tm_mon+1, t_tm->tm_mday, t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);	
  
}  



static int LogErrorMsg(char *timeStamp,char *info)
{
	FILE *fp = NULL;
	char logmsg[256];
	char filename[48];
	int year = atoi(timeStamp);
	int month = atoi(timeStamp + 5);
	int day = atoi(timeStamp + 8);
	memset(logmsg,0,sizeof(logmsg));
	DEBUG_PRINTF;
	sprintf(logmsg,"[%s][ERROR][%s][func:%s][line:%d] info:%s\n",timeStamp,__FILE__,__func__,__LINE__,info);

	memset(filename,0,sizeof(filename));
	sprintf(filename,"%s/error/%04d_%02d_%02d.log",Logdir,year,month,day);
	printf("filename = %s\n",filename);
	fp = fopen(filename,"a");
	if(fp == NULL)
		return -1;
	printf("logmsg = %s\n",logmsg);
	fwrite(logmsg,1,strlen(logmsg),fp);
	fflush(fp);
	fclose(fp);

	return 0;
}

static void LogWarnMsg(char *timeStamp,char *info)
{
	
}

static void LogInfoMsg(char *timeStamp,char *info)
{
	
}


static int LogDebugMsg(char *timeStamp,char *info)
{
	FILE *fp = NULL;
	char logmsg[256];
	char filename[48];
	int year = atoi(timeStamp);
	int month = atoi(timeStamp + 5);
	int day = atoi(timeStamp + 8);
	memset(logmsg,0,sizeof(logmsg));
	DEBUG_PRINTF;
	sprintf(logmsg,"[%s][DEBUG][%s][func:%s][line:%d] info:%s\n",timeStamp,__FILE__,__func__,__LINE__,info);

	memset(filename,0,sizeof(filename));
	sprintf(filename,"%s/error/%04d_%02d_%02d.log",Logdir,year,month,day);
	printf("filename = %s\n",filename);
	fp = fopen(filename,"a");
	if(fp == NULL)
		return -1;
	printf("logmsg = %s\n",logmsg);
	fwrite(logmsg,1,strlen(logmsg),fp);
	fflush(fp);
	fclose(fp);

	return 0;
}


static int DeleteFile()
{
	return 0;
}


static int LogDelete(char *dirpwd,Logmsg_t *Logmsg)
{
    DIR *dir; //声明一个句柄
    struct dirent *file; //readdir函数的返回值就存放在这个结构体中
    struct stat sb,st_buf;
	
	if(!(dir = opendir(dirpwd)))
	{
    	perror("log_find_OldestFile opendir");
		return -1;
	}

	while((file = readdir(dir)) != NULL)
	{
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;
		
		stat(file->d_name, &st_buf);
		if(!S_ISREG(st_buf.st_mode))
			continue;

		if()
			
	}
}

void Log_init(void)
{
    DIR *dir; //声明一个句柄
    struct dirent *file; //readdir函数的返回值就存放在这个结构体中
    struct stat sb,st_buf;
	
	char timeStamp[24];
	getSysTime(timeStamp);
	
	if(!(dir = opendir(Logdir)))
	{
    	perror("log_find_OldestFile opendir");
		return;
	}

	while((file = readdir(dir)) != NULL)
	{
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;
		
		stat(file->d_name, &st_buf);
		if(S_ISDIR(st_buf.st_mode))
		{
			char pwd[36];
			memset(pwd,0,sizeof(pwd));
			sprintf(pwd,"%s/%s",Logdir,file->d_name);
			if(strncmp(file->d_name,"error",5) == 0)
				LogDelete(pwd,&Derror);
			if(strncmp(file->d_name,"info",4) == 0)
				LogDelete(pwd,&Dinfo);
			if(strncmp(file->d_name,"warn",4) == 0)
				LogDelete(pwd,&Dwarn);
			if(strncmp(file->d_name,"debug",5) == 0)
				LogDelete(pwd,&Ddebug);
		}
	}
}

void LogMsgWrite(uint8_t Grade,char *info)
{
	char timeStamp[24];
	memset(timeStamp,0,sizeof(timeStamp));
	getSysTime(timeStamp);
	debug_printf("timeStamp = %s\n",timeStamp);

	switch(Grade)
	{
		case ERROR:
			LogErrorMsg(timeStamp,info);
			break;
		case WARN:
			LogWarnMsg(timeStamp,info);
			break;
		case INFO:
			LogInfoMsg(timeStamp,info);
			break;
		case DEBUG:
			LogDebugMsg(timeStamp,info);
			break;
		default:
			break;
	}
}



#if 1
int main(void)
{
	LogMsgWrite(ERROR,"This function is break out");
	return 0;
}
#endif
