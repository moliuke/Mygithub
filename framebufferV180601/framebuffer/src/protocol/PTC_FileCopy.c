#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<dirent.h>//输出文件信息
#include<sys/stat.h>//判断是否目录
#include "debug.h"

//#include "file_permis.h"


mode_t get_filepemis(char *filename)
{
	mode_t mode,retmode;		//总权限  
    struct stat buf;    //lstat();  
	if(lstat(filename, &buf) == -1)	
	{  
		//printf("line %d ", __LINE__);	
		perror("lstat");
		return -1;
	}  
	mode = buf.st_mode; 

	if(mode & S_IRUSR)retmode |= S_IRUSR;
	if(mode & S_IWUSR)retmode |= S_IWUSR;
	if(mode & S_IXUSR)retmode |= S_IXUSR;

	if(mode & S_IRGRP)retmode |= S_IRGRP;
	if(mode & S_IWGRP)retmode |= S_IWGRP;
	if(mode & S_IXGRP)retmode |= S_IXGRP;

	if(mode & S_IROTH)retmode |= S_IROTH;
	if(mode & S_IWOTH)retmode |= S_IWOTH;
	if(mode & S_IXOTH)retmode |= S_IXOTH;

		
	return retmode;
}

void set_filepermis(char *filename,mode_t mode)
{
	chmod(filename,mode);
}


//判断是否是目录
int is_dir(char* path)
{
	struct stat st;
	stat(path,&st);
	
	if(S_ISDIR(st.st_mode))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
/*字符串处理函数*/
int endwith(char* s,char c)
{//用于判断字符串结尾是否为“/”
	if(s[strlen(s)-1]==c)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


char* str_contact(char* str1,char* str2)
{//字符串连接
	char* result;
	result=(char*)malloc(strlen(str1)+strlen(str2)+1);//str1的长度+str2的长度+\0;
	if(!result)
	{//如果内存动态分配失败
		debug_printf("str_contact\n");
		return NULL;
	}
	
	strcat(result,str1);
	strcat(result,str2);//字符串拼接
	return result;
}


/*复制函数*/
int __copy_file(char* source_path,char *destination_path)
{//复制文件
	char buffer[1024];
	FILE *in,*out;//定义两个文件流，分别用于文件的读取和写入int len;
	if((in=fopen(source_path,"r"))==NULL)
	{//打开源文件的文件流
		debug_printf("open the src file failed\n");
		return -1;
	}
	
	if((out=fopen(destination_path,"w"))==NULL)
	{//打开目标文件的文件流
		debug_printf("open the dest file failed\n");
		fclose(in);
		return -1;
	}
	
	int len;//len为fread读到的字节长
	while((len=fread(buffer,1,1024,in))>0)
	{//从源文件中读取数据并放到缓冲区中，第二个参数1也可以写成sizeof(char)
		fwrite(buffer,1,len,out);//将缓冲区的数据写到目标文件中
	}
	
	fclose(out);
	fclose(in);
}

/*复制函数*/
int copy_file(char* source_path,char *destination_path)
{//复制文件
	int in,out;
	mode_t file_mode;
	char buffer[1024];
	//FILE *in,*out;//定义两个文件流，分别用于文件的读取和写入int len;
	if((in = open(source_path,O_RDONLY)) < 0)
	{//打开源文件的文件流
		debug_printf("open the src file failed\n");
		return -1;
	}

	file_mode = get_filepemis(source_path);

	if((out = open(destination_path,O_WRONLY | O_CREAT,0777)) < 0)
	{//打开目标文件的文件流
		debug_printf("open the dest file failed\n");
		return -1;
	}
	
	int len;//len为fread读到的字节长
	while((len=read(in,buffer,1024))>0)
	{//从源文件中读取数据并放到缓冲区中，第二个参数1也可以写成sizeof(char)
		write(out,buffer,len);//将缓冲区的数据写到目标文件中
	}
	
	close(out);
	close(in);
	
	set_filepermis(destination_path,file_mode);
	return 0;
}



void copy_folder(char* source_path,char *destination_path)
{//复制文件夹
	if(!opendir(destination_path))
	{
		if (mkdir(destination_path,0777))//如果不存在就用mkdir函数来创建
		{
			debug_printf("创建文件夹失败！");
		}
	}
	char *path;
	path=(char*)malloc(512);//相当于其它语言的String path=""，纯C环境下的字符串必须自己管理大小，这里为path直接申请512的位置的空间，用于目录的拼接
	path=str_contact(path,source_path);//这三句，相当于path=source_path
	struct dirent* filename;
	DIR* dp=opendir(path);//用DIR指针指向这个文件夹
	while(filename=readdir(dp))
	{//遍历DIR指针指向的文件夹，也就是文件数组。
		memset(path,0,sizeof(path));
		path=str_contact(path,source_path);
		//如果source_path,destination_path以路径分隔符结尾，那么source_path/,destination_path/直接作路径即可 
		//否则要在source_path,destination_path后面补个路径分隔符再加文件名，谁知道你传递过来的参数是f:/a还是f:/a/啊？
		char *file_source_path;
		file_source_path=(char*)malloc(512);
		if(!endwith(source_path,'/'))
		{

			file_source_path=str_contact(file_source_path,source_path);
			file_source_path=str_contact(source_path,"/");

		}
		else
		{
			file_source_path=str_contact(file_source_path,source_path);
		}
		char *file_destination_path;
		file_destination_path=(char*)malloc(512);
		if(!endwith(destination_path,'/'))
		{
			file_destination_path=str_contact(file_destination_path,destination_path);
			file_destination_path=str_contact(destination_path,"/");
		}
		else
		{
			file_destination_path=str_contact(file_destination_path,destination_path);
		}
		//取文件名与当前文件夹拼接成一个完整的路径
		file_source_path=str_contact(file_source_path,filename->d_name);
		file_destination_path=str_contact(file_destination_path,filename->d_name);
		if(is_dir(file_source_path))
		{//如果是目录
			if(!endwith(file_source_path,'.'))
			{//同时并不以.结尾，因为Linux在所有文件夹都有一个.文件夹用于连接上一级目录，必须剔除，否则进行递归的话，后果无法相像
				copy_folder(file_source_path,file_destination_path);//进行递归调用，相当于进入这个文件夹进行复制～
			} 
		}
		else
		{
			copy_file(file_source_path,file_destination_path);//否则按照单一文件的复制方法进行复制。
			debug_printf("copy %s  to  %s  ok!\n",file_source_path,file_destination_path);
		}
	} 
}


#if 0
/*主函数*/
int main(int argc,char *argv[])
{
	DEBUG_PRINTF;
	if(argv[1]==NULL||argv[1]==NULL)
	{
		debug_printf("Please enter two dir\n"
			"the first is src dir and the second one is des dir\n");
		//exit(1);
	}

	DEBUG_PRINTF;
	
	char* source_path="/home/LEDscr/setting/image/";//argv[1];//取用户输入的第一个参数
	char* destination_path="/home/LEDscr/cptest/";//argv[2];//取用户输入的第二个参数

	mkdir(destination_path,0777);
#if 0	
	DIR* source=opendir(source_path);
	DIR* destination=opendir(destination_path);
	
	DEBUG_PRINTF;
	if(!source||!destination)
	{
		//对单个文件进行拷贝
		int len;
		char buf[1024];
		FILE *in,*out;
		in=fopen(argv[1],"r+");
		out=fopen(argv[2],"w+");
		DEBUG_PRINTF;
		while(len=fread(buf,1,sizeof(buf),in))
		{
			fwrite(buf,1,len,out);
		}
		debug_printf("=============copy  %s  to  %s  ok!\n",argv[1],argv[2]);
		return 0;
	}
	DEBUG_PRINTF;
#endif
	copy_folder(source_path,destination_path);//进行文件夹的拷贝
	return 0;
}
#endif

