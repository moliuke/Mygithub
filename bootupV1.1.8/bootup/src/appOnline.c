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


//���ڷ���-1�������ڷ���0
int check_APPonline(int get_pid_name(char *app_name))
{
    DIR *dir;
    struct dirent *next;
    int i = 0;

    FILE *fp = NULL;

	char file[64];
    char buffer[128];
    char name[128];
    ///proc�а�����ǰ�Ľ�����Ϣ,��ȡ��Ŀ¼
    dir = opendir("/proc");
    if (!dir)
        return -1;
	
	while ((next = readdir(dir)) != NULL)
	{
		//����"."��".."�����ļ���
		if ((strcmp(next->d_name, "..") == 0) || (strcmp(next->d_name, ".") == 0))
			   continue;
		//����ļ�����������������
		if (!isdigit(*next->d_name))
			   continue;
		//debug_printf("next->d_name = %s\n",next->d_name);
		//�ж��Ƿ��ܴ�״̬�ļ�
		memset(file,0,sizeof(file));
		sprintf(file,"/proc/%s/status",next->d_name);
		if ((fp = fopen(file,"r")) == NULL)
			   continue;
		//debug_printf("open status ok\n");
		//��ȡ״̬�ļ�
		//if (fgets(buffer,READ_BUF_SIZE,status) == NULL)
		int readsize = fread(buffer,1,sizeof(buffer),fp);
		if(readsize <= 0)
		{
			   fclose(fp);
			   continue;
		}
		fclose(fp);
		sscanf(buffer,"%*s %s",name);//��ȡPID��Ӧ�ĳ���������ʽΪName:  ������
  	  	if(!get_pid_name(name))
		{
			closedir(dir);
			#if 1
			memset(name,0,sizeof(name));
			sscanf(buffer,"%*s %*s %*s %s",name);//��ȡPID�Ķ�Ӧ�Ľ���״̬
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


