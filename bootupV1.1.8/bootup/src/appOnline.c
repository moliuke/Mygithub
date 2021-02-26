#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include "debug.h"


#define READ_BUF_SIZE 1024

int GetSysIPmsg(void)
{
	
}


//存在返回-1，不存在返回0
int check_APPonline(int get_pid_name(char *app_name))
{
    DIR *dir;
    struct dirent *next;
    int i = 0;

    FILE *fp = NULL;

	char file[64];
    char buffer[128];
    char name[128];
    ///proc中包括当前的进程信息,读取该目录
    dir = opendir("/proc");
    if (!dir)
        return -1;
	
	while ((next = readdir(dir)) != NULL)
	{
		//跳过"."和".."两个文件名
		if ((strcmp(next->d_name, "..") == 0) || (strcmp(next->d_name, ".") == 0))
			   continue;
		//如果文件名不是数字则跳过
		if (!isdigit(*next->d_name))
			   continue;
		//debug_printf("next->d_name = %s\n",next->d_name);
		//判断是否能打开状态文件
		memset(file,0,sizeof(file));
		sprintf(file,"/proc/%s/status",next->d_name);
		if ((fp = fopen(file,"r")) == NULL)
			   continue;
		//debug_printf("open status ok\n");
		//读取状态文件
		//if (fgets(buffer,READ_BUF_SIZE,status) == NULL)
		int readsize = fread(buffer,1,sizeof(buffer),fp);
		if(readsize <= 0)
		{
			   fclose(fp);
			   continue;
		}
		fclose(fp);
		sscanf(buffer,"%*s %s",name);//读取PID对应的程序名，格式为Name:  程序名
  	  	if(!get_pid_name(name))
		{
			closedir(dir);
			#if 1
			memset(name,0,sizeof(name));
			sscanf(buffer,"%*s %*s %*s %s",name);//读取PID的对应的进程状态
			if(name[0] == 'S' || name[0] == 'R')
				return 0;
			else
				return -1;
			#endif
			//debug_printf("ledscreen is exist!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		
		}
	}
	closedir(dir);
	return -1;
}

#if 0
int main(void)
{
	DEBUG_PRINTF;
	judge_pid_exist(get_pid_name);
	return 0;
}
#endif


