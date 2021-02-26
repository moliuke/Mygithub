#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<dirent.h>//����ļ���Ϣ
#include<sys/stat.h>//�ж��Ƿ�Ŀ¼
#include "debug.h"

//#include "file_permis.h"


mode_t get_filepemis(char *filename)
{
	mode_t mode,retmode;		//��Ȩ��  
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


//�ж��Ƿ���Ŀ¼
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
/*�ַ���������*/
int endwith(char* s,char c)
{//�����ж��ַ�����β�Ƿ�Ϊ��/��
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
{//�ַ�������
	char* result;
	result=(char*)malloc(strlen(str1)+strlen(str2)+1);//str1�ĳ���+str2�ĳ���+\0;
	if(!result)
	{//����ڴ涯̬����ʧ��
		debug_printf("str_contact\n");
		return NULL;
	}
	
	strcat(result,str1);
	strcat(result,str2);//�ַ���ƴ��
	return result;
}


/*���ƺ���*/
int __copy_file(char* source_path,char *destination_path)
{//�����ļ�
	char buffer[1024];
	FILE *in,*out;//���������ļ������ֱ������ļ��Ķ�ȡ��д��int len;
	if((in=fopen(source_path,"r"))==NULL)
	{//��Դ�ļ����ļ���
		debug_printf("open the src file failed\n");
		return -1;
	}
	
	if((out=fopen(destination_path,"w"))==NULL)
	{//��Ŀ���ļ����ļ���
		debug_printf("open the dest file failed\n");
		fclose(in);
		return -1;
	}
	
	int len;//lenΪfread�������ֽڳ�
	while((len=fread(buffer,1,1024,in))>0)
	{//��Դ�ļ��ж�ȡ���ݲ��ŵ��������У��ڶ�������1Ҳ����д��sizeof(char)
		fwrite(buffer,1,len,out);//��������������д��Ŀ���ļ���
	}
	
	fclose(out);
	fclose(in);
}

/*���ƺ���*/
int copy_file(char* source_path,char *destination_path)
{//�����ļ�
	int in,out;
	mode_t file_mode;
	char buffer[1024];
	//FILE *in,*out;//���������ļ������ֱ������ļ��Ķ�ȡ��д��int len;
	if((in = open(source_path,O_RDONLY)) < 0)
	{//��Դ�ļ����ļ���
		debug_printf("open the src file failed\n");
		return -1;
	}

	file_mode = get_filepemis(source_path);

	if((out = open(destination_path,O_WRONLY | O_CREAT,0777)) < 0)
	{//��Ŀ���ļ����ļ���
		debug_printf("open the dest file failed\n");
		return -1;
	}
	
	int len;//lenΪfread�������ֽڳ�
	while((len=read(in,buffer,1024))>0)
	{//��Դ�ļ��ж�ȡ���ݲ��ŵ��������У��ڶ�������1Ҳ����д��sizeof(char)
		write(out,buffer,len);//��������������д��Ŀ���ļ���
	}
	
	close(out);
	close(in);
	
	set_filepermis(destination_path,file_mode);
	return 0;
}



void copy_folder(char* source_path,char *destination_path)
{//�����ļ���
	if(!opendir(destination_path))
	{
		if (mkdir(destination_path,0777))//��������ھ���mkdir����������
		{
			debug_printf("�����ļ���ʧ�ܣ�");
		}
	}
	char *path;
	path=(char*)malloc(512);//�൱���������Ե�String path=""����C�����µ��ַ��������Լ������С������Ϊpathֱ������512��λ�õĿռ䣬����Ŀ¼��ƴ��
	path=str_contact(path,source_path);//�����䣬�൱��path=source_path
	struct dirent* filename;
	DIR* dp=opendir(path);//��DIRָ��ָ������ļ���
	while(filename=readdir(dp))
	{//����DIRָ��ָ����ļ��У�Ҳ�����ļ����顣
		memset(path,0,sizeof(path));
		path=str_contact(path,source_path);
		//���source_path,destination_path��·���ָ�����β����ôsource_path/,destination_path/ֱ����·������ 
		//����Ҫ��source_path,destination_path���油��·���ָ����ټ��ļ�����˭֪���㴫�ݹ����Ĳ�����f:/a����f:/a/����
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
		//ȡ�ļ����뵱ǰ�ļ���ƴ�ӳ�һ��������·��
		file_source_path=str_contact(file_source_path,filename->d_name);
		file_destination_path=str_contact(file_destination_path,filename->d_name);
		if(is_dir(file_source_path))
		{//�����Ŀ¼
			if(!endwith(file_source_path,'.'))
			{//ͬʱ������.��β����ΪLinux�������ļ��ж���һ��.�ļ�������������һ��Ŀ¼�������޳���������еݹ�Ļ�������޷�����
				copy_folder(file_source_path,file_destination_path);//���еݹ���ã��൱�ڽ�������ļ��н��и��ơ�
			} 
		}
		else
		{
			copy_file(file_source_path,file_destination_path);//�����յ�һ�ļ��ĸ��Ʒ������и��ơ�
			debug_printf("copy %s  to  %s  ok!\n",file_source_path,file_destination_path);
		}
	} 
}


#if 0
/*������*/
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
	
	char* source_path="/home/LEDscr/setting/image/";//argv[1];//ȡ�û�����ĵ�һ������
	char* destination_path="/home/LEDscr/cptest/";//argv[2];//ȡ�û�����ĵڶ�������

	mkdir(destination_path,0777);
#if 0	
	DIR* source=opendir(source_path);
	DIR* destination=opendir(destination_path);
	
	DEBUG_PRINTF;
	if(!source||!destination)
	{
		//�Ե����ļ����п���
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
	copy_folder(source_path,destination_path);//�����ļ��еĿ���
	return 0;
}
#endif

