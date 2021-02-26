#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include "def_list.h"
#include "../config.h"
#include "../debug.h"
#include "../conf.h"

#define configfile		"/home/LEDscr/config/cls.conf"

#define LISTPATH_DEF	"/home/LEDscr/list/deflist"
#define LISTPATH		"/home/LEDscr/list"

static uint32_t Swidth,Sheight;

static void INITscreenSize(void)
{
	char KeyVal[8];
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(configfile,"screen","scr_width",KeyVal);
	Swidth = atoi(KeyVal);
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(configfile,"screen","scr_height",KeyVal);
	Sheight = atoi(KeyVal);

	debug_printf("Swidth = %d,Sheight = %d\n",Swidth,Sheight);
}

int Set_deflist(void)
{
	DIR *dir; //声明一个句柄
	struct dirent *file; //readdir函数的返回值就存放在这个结构体中
	struct stat sb,st_buf;
	uint32_t width,height;
	char  *p = NULL;
	char cmd[96];

	INITscreenSize();

	if(!(dir = opendir(LISTPATH_DEF)))
	{
		perror("LISTPATH opendir");
		return -1;
	}

	while((file = readdir(dir)) != NULL)
	{
		//把当前目录.，上一级目录..及隐藏文件都去掉，避免死循环遍历目录
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;
		
		width = atoi(file->d_name);
		if(width != Swidth)
			continue;
		
		p = strchr(file->d_name,'_');
		if(p == NULL)
			continue;
		
		height = atoi(p+1);
		if(height != Sheight)
			continue;
		
		memset(cmd,0,sizeof(cmd));
		sprintf(cmd,"cp %s/%s %s/Def.xkl",LISTPATH_DEF,file->d_name,LISTPATH);
		system(cmd);

		return 0;
	}

	return 0;

}



