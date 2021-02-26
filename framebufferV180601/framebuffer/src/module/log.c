#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include "mtime.h"
#include "debug.h"
#include "log.h"
#include "config.h"

static f_log* g_log = NULL;  
static pthread_once_t init_create = PTHREAD_ONCE_INIT; 



enum
{
	STATE_YEAR = 0,
	STATE_MONTH,
	STATE_DAY,
	STATE_HOUR,
	STATE_MIN,
	STATE_SEC,
	STATE_CPY
};

enum{OLD_CMP,NEW_CMP};



void log_write(char *logtext,int len)
{
	int ret = 0;
	int fd = 0;
	char writeitem[128];
	uint8_t systime[24];
	uint8_t systimelen;
	
	memset(writeitem,0,sizeof(writeitem));
	get_sys_time(systime,&systimelen);
	sprintf(writeitem,"%s time:%s\n\n",logtext,systime);
	fd = open("/home/LEDscr/sys/daemon.log",O_WRONLY | O_CREAT,0744);
	if(fd < 0)
	{
		perror("open:");
		//debug_printf("fp < NULL\n");
	}
	lseek(fd, 0, SEEK_END);
	ret = write(fd,writeitem,strlen(writeitem));
}







static int log_file_NewerCmp(char *new_cmpfile,char *srcfile)
{
	int ret = 0;
	int state = STATE_YEAR;
	char *_p = NULL;
	uint8_t year_pos,month_pos,day_pos,hour_pos,min_pos,sec_pos;

	if(new_cmpfile == NULL || srcfile == NULL)
	{
		log_debug_printf("[log_file_NewerCmp] : agument error!\n");
		return -1;
	}

	_p = strchr(new_cmpfile,'_');

	//������־�ļ�����־�ļ����й̶��ĸ�ʽ
	if(new_cmpfile != 0 && _p == NULL)
		return -1;
	
	uint8_t filenamelen = _p - new_cmpfile; //ǰ׺����
	
	year_pos = filenamelen + 1;
	month_pos = year_pos + 5;
	day_pos = month_pos + 3;
	hour_pos = day_pos + 3;
	min_pos = hour_pos + 3;
	sec_pos = min_pos + 3;

	while(1)
	{
		switch(state)
		{
			case STATE_YEAR:
				ret = atoi(new_cmpfile + year_pos) - atoi(srcfile + year_pos);
				if(ret == 0)
					state = STATE_MONTH;
				else
					state = STATE_CPY;
				break;
				
			case STATE_MONTH:
				ret = atoi(new_cmpfile + month_pos) - atoi(srcfile + month_pos);
				if(ret == 0)
					state = STATE_DAY;
				else
					state = STATE_CPY;
				break;
			case STATE_DAY:
				ret = atoi(new_cmpfile + day_pos) - atoi(srcfile + day_pos);
				if(ret == 0)
					state = STATE_HOUR;
				else
					state = STATE_CPY;
				break;
			case STATE_HOUR:
				ret = atoi(new_cmpfile + hour_pos) - atoi(srcfile + hour_pos);
				if(ret == 0)
					state = STATE_MIN;
				else
					state = STATE_CPY;
				break;
			case STATE_MIN:
				ret = atoi(new_cmpfile + min_pos) - atoi(srcfile + min_pos);
				if(ret == 0)
					state = STATE_SEC;
				else
					state = STATE_CPY;
				break;
			case STATE_SEC:
				ret = atoi(new_cmpfile + sec_pos) - atoi(srcfile + sec_pos);
				if(ret == 0)
					return 0;
				else
					state = STATE_CPY;
				break;
			case STATE_CPY:
				if(ret > 0)
				{
					return 0;
				}

				if(ret < 0)
				{
					strcpy(new_cmpfile,srcfile);
					log_debug_printf("new_cmpfile = %s\n",new_cmpfile);
					return 0;
				}
			default:
				return -1;
		}
	}
	return 0;
}

static int log_file_olderCmp(char *old_cmpfile,char *srcfile)
{
	int ret = 0;
	int state = STATE_YEAR;
	char *_p = NULL;
	uint8_t year_pos,month_pos,day_pos,hour_pos,min_pos,sec_pos;

	if(old_cmpfile == NULL || srcfile == NULL)
	{
		log_debug_printf("[log_file_NewerCmp] : agument error!\n");
		return -1;
	}

	_p = strchr(old_cmpfile,'_');

	//������־�ļ�����־�ļ����й̶��ĸ�ʽ
	if(old_cmpfile != 0 && _p == NULL)
		return -1;
	
	uint8_t filenamelen = _p - old_cmpfile; //ǰ׺����
	
	year_pos = filenamelen + 1;
	month_pos = year_pos + 5;
	day_pos = month_pos + 3;
	hour_pos = day_pos + 3;
	min_pos = hour_pos + 3;
	sec_pos = min_pos + 3;

	//debug_printf("old_cmpfile = %s,srcname = %s\n",old_cmpfile,srcfile);
	while(1)
	{
		switch(state)
		{
			case STATE_YEAR:
				ret = atoi(old_cmpfile + year_pos) - atoi(srcfile + year_pos);
				if(ret == 0)
					state = STATE_MONTH;
				else
					state = STATE_CPY;
				break;
				
			case STATE_MONTH:
				ret = atoi(old_cmpfile + month_pos) - atoi(srcfile + month_pos);
				if(ret == 0)
					state = STATE_DAY;
				else
					state = STATE_CPY;
				break;
			case STATE_DAY:
				ret = atoi(old_cmpfile + day_pos) - atoi(srcfile + day_pos);
				if(ret == 0)
					state = STATE_HOUR;
				else
					state = STATE_CPY;
				break;
			case STATE_HOUR:
				ret = atoi(old_cmpfile + hour_pos) - atoi(srcfile + hour_pos);
				if(ret == 0)
					state = STATE_MIN;
				else
					state = STATE_CPY;
				break;
			case STATE_MIN:
				ret = atoi(old_cmpfile + min_pos) - atoi(srcfile + min_pos);
				if(ret == 0)
					state = STATE_SEC;
				else
					state = STATE_CPY;
				break;
			case STATE_SEC:
				ret = atoi(old_cmpfile + sec_pos) - atoi(srcfile + sec_pos);
				if(ret == 0)
					return 0;
				else
					state = STATE_CPY;
				break;
			case STATE_CPY:
				if(ret > 0)
				{
					strcpy(old_cmpfile,srcfile);
					//debug_printf("old_cmpfile = %s\n",old_cmpfile);
					return 0;
				}

				if(ret < 0)
				{
					return 0;
				}
			default:
				return -1;
		}
	}
	return 0;
}

static int log_find_NewestFile(char *file_prefix,char *logpath,char *newestFile)
{
    DIR *dir; //����һ�����
    struct dirent *file; //readdir�����ķ���ֵ�ʹ��������ṹ����
    struct stat sb,st_buf;
	int i;
	int file_count = 0;
	
	char curdir[128];
	char filename[128];
	char file_newest[64];

	char  *p = NULL;
	int get_newest_flag = 0;

	memset(file_newest,0,sizeof(file_newest));

	if(!(dir = opendir(logpath)))
	{
		LOG_DEBUG_PRINTF;
    	perror("log_find_NewestFile opendir");
		return -1;
	}

	chdir(logpath);
	LOG_DEBUG_PRINTF;
	while((file = readdir(dir)) != NULL)
	{
		//�ѵ�ǰĿ¼.����һ��Ŀ¼..�������ļ���ȥ����������ѭ������Ŀ¼
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

		//1�����ж��Ƿ�����ͨ�ļ�
		stat(file->d_name, &st_buf);
		if(!S_ISREG(st_buf.st_mode))
			continue;

		//2������ļ��Ƿ�����־�ļ����з���
		p = strchr(file->d_name,'_');
		if(p == NULL)
			continue;

		//3������ļ�ǰ׺���Ƿ��������ǰ׺��һ��
		if(strncmp(file_prefix,file->d_name,p-file->d_name) != 0)
			continue;
		
		//4��ȷ������־�ļ�	
		if(strlen(file_newest) == 0)
			memcpy(file_newest,file->d_name,strlen(file->d_name));
		else
			log_file_NewerCmp(file_newest,file->d_name);
		
		get_newest_flag = 1;
		file_count += 1;

	}

	if(get_newest_flag)
	{
		strncpy(newestFile,file_newest,strlen(file_newest));
	}
	else
		file_count = 0;
	
	return file_count;
}


static int log_find_OldestFile(char *file_prefix,char *logpath,char *oldestFile)
{
    DIR *dir; //����һ�����
    struct dirent *file; //readdir�����ķ���ֵ�ʹ��������ṹ����
    struct stat sb,st_buf;
	int i;
	
	char curdir[128];
	char filename[128];
	char file_oldest[64];

	char *p = NULL;
	int get_oldest_flag = 0;
	int file_count = 0;

	memset(file_oldest,0,sizeof(file_oldest));

	if(!(dir = opendir(logpath)))
	{
		LOG_DEBUG_PRINTF;
    	perror("log_find_OldestFile opendir");
		return -1;
	}

	chdir(logpath);
	while((file = readdir(dir)) != NULL)
	{
		//�ѵ�ǰĿ¼.����һ��Ŀ¼..�������ļ���ȥ����������ѭ������Ŀ¼
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

		//debug_printf("--------------file->d_name = %s----------------\n",file->d_name);
		//1�����ж��Ƿ�����ͨ�ļ�
		stat(file->d_name, &st_buf);
		if(!S_ISREG(st_buf.st_mode))
			continue;

		//2������ļ��Ƿ�����־�ļ����з���
		p = strchr(file->d_name,'_');
		if(p == NULL)
			continue;

		//3������ļ�ǰ׺���Ƿ��������ǰ׺��һ��
		if(strncmp(file_prefix,file->d_name,p-file->d_name) != 0)
			continue;

		//4��ȷ������־�ļ�	
		if(strlen(file_oldest) == 0)
			memcpy(file_oldest,file->d_name,strlen(file->d_name));
		else
			log_file_olderCmp(file_oldest,file->d_name);
		
		get_oldest_flag = 1;
		file_count += 1;

		//debug_printf("now the newest file is : %s\n",file_oldest);
	}

	if(get_oldest_flag)
	{
		strncpy(oldestFile,file_oldest,strlen(file_oldest));
	}
	else
		file_count = 0;
	
	return file_count;
}



int log_get_AllFileSize(char *file_prefix,char *logpath)
{
	int allsize = 0;
    DIR *dir; //����һ�����
    struct dirent *file; //readdir�����ķ���ֵ�ʹ��������ṹ����
    struct stat sb,st_buf;

	char *p = NULL;
	
	if(!(dir = opendir(logpath)))
	{
		LOG_DEBUG_PRINTF;
    	perror("log_get_AllFileSize opendir");
		return -1;
	}

	chdir(logpath);
	while((file = readdir(dir)) != NULL)
	{
		//�ѵ�ǰĿ¼.����һ��Ŀ¼..�������ļ���ȥ����������ѭ������Ŀ¼
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

		//debug_printf("--------------file->d_name = %s----------------\n",file->d_name);
		//1�����ж��Ƿ�����ͨ�ļ�
		stat(file->d_name, &st_buf);
		if(!S_ISREG(st_buf.st_mode))
			continue;

		//2������ļ��Ƿ�����־�ļ����з���
		p = strchr(file->d_name,'_');
		if(p == NULL)
			continue;

		//3������ļ�ǰ׺���Ƿ��������ǰ׺��һ��
		if(strncmp(file_prefix,file->d_name,p-file->d_name) != 0)
			continue;

		//4��ȷ������־�ļ�, ͳ�Ƶ�ǰĿ¼�µ�������־�Ĵ�С�ܺ�	
		allsize += st_buf.st_size;
		//debug_printf("now the newest file is : %s\n",file_oldest);
	}
	return allsize;
}


static int log_getDays(char *file_prefix,char *logpath)
{
    DIR *dir; //����һ�����
    struct dirent *file; //readdir�����ķ���ֵ�ʹ��������ṹ����
    struct stat sb,st_buf;
	struct tm* tm;
	char curdir[128];
	char filename[128];
	char date[20][12];

	char *p = NULL;
	int get_oldest_flag = 0;
	int file_count = 0;
	int n = 0;
	int i = 0,j = 0;
	int flag = 0;
	int days = 0;

	memset(date,0,20 * 12);

	log_debug_printf("lod_dir = %s\n",logpath);
	if(!(dir = opendir(logpath)))
	{
    	perror("log_getDays opendir");
		return -1;
	}
	chdir(logpath);
	while((file = readdir(dir)) != NULL)
	{
		//�ѵ�ǰĿ¼.����һ��Ŀ¼..�������ļ���ȥ����������ѭ������Ŀ¼
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;

		//debug_printf("--------------file->d_name = %s----------------\n",file->d_name);
		//1�����ж��Ƿ�����ͨ�ļ�
		stat(file->d_name, &st_buf);
		if(!S_ISREG(st_buf.st_mode))
			continue;

		//2������ļ��Ƿ�����־�ļ����з���
		p = strchr(file->d_name,'_');
		if(p == NULL)
			continue;

		//3������ļ�ǰ׺���Ƿ��������ǰ׺��һ��
		if(strncmp(file_prefix,file->d_name,p-file->d_name) != 0)
			continue;
		tm = localtime(&st_buf.st_ctime);
		sprintf(date[n++],"%4d/%02d/%02d",tm->tm_year+1900,tm->tm_mon + 1,tm->tm_mday);
	}

	for(i = 0 ; i < n - 1 ; i ++)
	{
		for(j = i + 1 ; j < n ; j ++)
		{
			if(strncmp(date[i],date[j],10) == 0)
			{
				break;
			}
		}

		if(j == n)
			days += 1;
	}

	days += 1;
	log_debug_printf("@@@@@@@@@@@days = %d\n",days);

	
	return days;
}


static int log_find_nameType(f_log *log,char *logpath)
{
    DIR *dir; //����һ�����
    struct dirent *file; //readdir�����ķ���ֵ�ʹ��������ṹ����
    struct stat sb,st_buf;
	int i;
	
	char curdir[128];
	char filename[128];

	if(!(dir = opendir(logpath)))
	{
    	perror("log_find_nameType opendir");
		return -1;
	}

	memset(curdir,0,sizeof(curdir));
	getcwd(curdir,sizeof(curdir));
	log_debug_printf("curdir = %s\n",curdir);

	chdir(logpath);

	//������־Ŀ¼�µ�������Ŀ¼������ÿ����Ŀ¼����һ���ṹ��ÿ���ṹ�¿��ԹҺܶ�����ļ�
	//��Щ���ļ��������ǵľ���İ��������洢����־�ļ���ÿ���ṹ��������һ�������ļ���һ��
	//��ɵ��ļ�
	while((file = readdir(dir)) != NULL)
	{
		//�ѵ�ǰĿ¼.����һ��Ŀ¼..�������ļ���ȥ����������ѭ������Ŀ¼
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;
#if 0
		for(i = 0 ; i < MAX_FILE_COUNT ; i ++)
		{
			if(log->file_count[i] == NULL)
				debug_printf("log->file_count[%d] = NULL\n",i);
		}
#endif		
		stat(file->d_name, &st_buf);
		if (S_ISDIR(st_buf.st_mode))
		{
			for(i = 0 ; i < MAX_FILE_COUNT ; i ++)
			{
				if(log->file_count[i] == NULL)
				{
					log_file_t *new_file_log = (log_file_t *)malloc(sizeof(log_file_t));
					new_file_log->file_count = 0;
					strncpy(new_file_log->filename,file->d_name,strlen(file->d_name));
					log->file_count[i] = new_file_log;
					memset(log->file_count[i]->output_filename,0,sizeof(log->file_count[i]->output_filename));
					memset(log->file_count[i]->oldest_file,0,sizeof(log->file_count[i]->oldest_file));
					log->file_index += 1;
					break;
				}
				else
				{
					//debug_printf("log->file_count[%d]->%s\n",i,log->file_count[i]->filename);
				}
			}
		}
		
	}
	//debug_printf("log->file_index = %d\n",log->file_index);

	chdir(curdir);
	return 0;
}


int trave_dir(f_log *log,char* path, int depth)
{
    DIR *d; //����һ�����
    struct dirent *file; //readdir�����ķ���ֵ�ʹ��������ṹ����
    struct stat sb;

	int i = 0;


    if(!(d = opendir(path)))
    {
    	perror("trave_dir opendir");
        log_debug_printf("error opendir %s!!!\n",path);
        return -1;
    }

	char *p = NULL;
	int cmpvals = 0;
	while((file = readdir(d)) != NULL)
	{
		//�ѵ�ǰĿ¼.����һ��Ŀ¼..�������ļ���ȥ����������ѭ������Ŀ¼
		if(strncmp(file->d_name, ".", 1) == 0)
			continue;
		if(file->d_type == 8)
		{
			for(i = 0 ; i < MAX_FILE_COUNT ; i ++)
			{
				if(log->file_count[i] == NULL)
				{
					char filename[128];
					char *p = strchr(file->d_name,'_');
					log_file_t *new_file_log = (log_file_t *)malloc(sizeof(log_file_t));
					
					strncpy(new_file_log->filename,file->d_name,(p-file->d_name));
					strcpy(new_file_log->output_filename,file->d_name);
					new_file_log->file_count = 1;
					log->file_count[i] = new_file_log;
					break;
				}

				else
				{
					log_debug_printf("log->file_count[%d]->filename = %s,log->file_count[%d]->output_filename = %s,file->d_name = %s\n",i,log->file_count[i]->filename,i,log->file_count[i]->output_filename,file->d_name);
					if(strncmp(log->file_count[i]->filename,file->d_name,strlen(log->file_count[i]->filename)) == 0)
					{
						//log_file_NewerCmp(log->file_count[i],file->d_name);
						break;
					}
					else
					{
						continue;
					}
					
				}
				
			}
			
		}

		
		//strcpy(filename[len++], file->d_name); //������������ļ���
		//�жϸ��ļ��Ƿ���Ŀ¼�����Ƿ������������㣬�����Ҷ���ֻ����������Ŀ¼��̫��Ͳ����ˣ�ʡ���ѳ�̫���ļ�
		if(stat(file->d_name, &sb) >= 0 && S_ISDIR(sb.st_mode) && depth <= 3)
		{
			trave_dir(log,file->d_name, depth + 1);
		}
	}
	closedir(d);
	return 0;
}

  
/********************* 
 
  �ڲ�����ʵ�� 
 
 ********************/  
// ȫ�ֱ��� ��ʼ��  
static void _log_init()  
{  
    int i;  
    f_log *temp_glo=NULL;  
    temp_glo = (f_log*)malloc(sizeof(f_log));  
    if(temp_glo == NULL)  
    {  
        log_debug_printf("Can not malloc f_log!!!\r\n");  
        return ;  
  
    }  
    pthread_mutex_init(&temp_glo->mutex,NULL);  
  
    temp_glo->max_filesize = MAX_FILE_SIZE; // 20MB  
    temp_glo->file_index = 0;  
    for(i=0;i<MAX_FILE_COUNT;i++)  
    {  
        temp_glo->file_count[i] == NULL;  
    }  
    g_log=temp_glo; 

}  
static inline int _lopen(const char* filename)  
{  
    return open(filename,O_CREAT | O_WRONLY | O_APPEND,644);  
}  
  
static inline FILE* _log_open(const char* filename)  
{  
    int fd= _lopen(filename);  
    if(fd<0)  
    {  
    	log_debug_printf("filename = %s\n",filename);
        log_debug_printf("Can not open file !\r\n");  
        return NULL;  
    }  
    FILE* f = fdopen(fd,"a");  
    if(f== NULL)  
    {  
        log_debug_printf("Can open not \r\n");  
        return NULL;  
  
    }  
    return f;  
}  
  
// д��װ  
static int _log_write_unlocked(log_file_t *f,char* log,int len)  
{  
    int remain_len,write_len,temp;  
    remain_len = len;  
    write_len = 0;  
    while(remain_len)  
    {  
        temp=fwrite(log,1,len,f->fp);  
        write_len+=temp;  
        //      debug_printf("fddfdfdffd\r\n");  
        remain_len-=temp;  
  
    }  
    return write_len;  
}  
  
static void _log_roll_file(log_file_t* f)  
{  
  
    fclose(f->fp); 
	log_debug_printf("###f->filename = %s,f->output_filename = %s\n",f->filename,f->output_filename);
    _log_create_filename(f->filename,f->output_filename);

	f->file_count += 1;

	//����һ���ļ�������ļ���������10���ļ������ɵ��ļ�ɾ��
	//�������ҳ���ɵ��ļ�
	if(f->file_count > 10)
	{
		char filepsw[128];
		memset(filepsw,0,sizeof(filepsw));
		sprintf(filepsw,logdir"/%s/%s",f->filename,f->oldest_file);
		remove(filepsw);
		f->file_count -= 1;
		f->days -= 1;
		
		memset(filepsw,0,sizeof(filepsw));
		sprintf(filepsw,logdir"/%s",f->filename);
		log_find_OldestFile(f->filename,filepsw,f->oldest_file);
	}
	
    f->fp = _log_open(f->output_filename);  
    f->filesize = 0;  
//  snprintf(f->filename,LOG_MAX_FILE_NAME,"%s",filename);  
  
  
}  
  
// д��־  
static int _log_write(log_file_t *f,char* log,int len)  
{  
    int write_len=0;  
    if(!f || !log )  
    {  
        return 0;  
    }  

	//���ļ�fдlen���ֽڵ�log��־
    write_len=_log_write_unlocked(f,log,len);  
    if(write_len < len)  
    {  
        log_debug_printf("Can not write log to f!\r\n");  
        return 0;  
    }  
    fflush(f->fp);  
    f->filesize += write_len;


	//�ļ���С�ﵽ����ʱ�����´���һ���µ��ļ�
    if(f->filesize > g_log->max_filesize)  
    {  
    	log_debug_printf("f->output_filename = %s,f->filesize = %d,g_log->max_filesize = %d\n",f->output_filename,f->filesize,g_log->max_filesize);
		LOG_DEBUG_PRINTF;
		//���ļ�д��ָ�����ֵʱ�Զ�����һ���µ��ļ����洢
        _log_roll_file(f);  
  
    }  
  
    return write_len;  
}  
  
char* _log_create_filename(char* filename,char* output_filename)  
{  
    char now_time[22];  
    time_t tm_time = time(NULL);  
    struct tm now;  
    gmtime_r(&tm_time,&now);  
    sprintf(now_time,"%04d_%02d_%02d_%02d_%02d_%02d",(now.tm_year+1900),now.tm_mon+1,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec);  
    snprintf(output_filename,LOG_MAX_OUTPUT_NAME,"%s_%s.txt",filename,now_time);  
    return output_filename;  
  
  
}  


//��ȡϵͳʱ�䣬���ʱ�䷢����ʱ��
static void _log_get_time(time_t tm_time,char* time_str)  
{  
#if 0
    struct tm now;  
    gmtime_r(&tm_time,&now);  
    snprintf(time_str,LOG_TIME_STR_LEN+1,"[%04d_%02d_%02d %02d:%02d:%02d]",(now.tm_year+1900),now.tm_mon+1,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec);  
#else
	time_t timer;
	struct tm* t_tm; 

	char timestr[28];

	time(&timer);	
	t_tm = localtime(&timer);
	snprintf(time_str,LOG_TIME_STR_LEN+1,"[%04d_%02d_%02d %02d:%02d:%02d]", t_tm->tm_year+1900,	 
	t_tm->tm_mon+1, t_tm->tm_mday, t_tm->tm_hour, t_tm->tm_min, t_tm->tm_sec);	
	//sprintf(time_str,"%d",2017);
#endif
  
}  
  
static int __log_get_file_sig(char* file_sig,int file_sig_len,char* output_file_sig)  
{  
    if(!file_sig)  
    {  
        return 0;  
    }  
    snprintf(output_file_sig,file_sig_len+3,"[%s]",file_sig);  
      //debug_printf("output file sig is %s\r\n",output_file_sig);  
    return 1;  
  
} 


static inline int _log_get_sync_time(char *timestr)
{
	int stringlen = 0;
    time_t tm_check = time(NULL);  

	char timestring[28];

	pthread_mutex_lock(&g_log->mutex); 

	//��ȡϵͳʱ�䣬����¼�������ʱ��
	_log_get_time(tm_check,timestring); 

	pthread_mutex_unlock(&g_log->mutex); 

	strncpy(timestr,timestring,strlen(timestring));

	//debug_printf("timestr = %s,strlen(timestr) = %d,strlen(timestring) = %d\n",timestr,strlen(timestr),strlen(timestring));
	return 0;
}

static inline int _log_get_file_sig(char* buf,char* file_sig,int file_sig_len)
{
    char sig_buf[file_sig_len+2];
	int stringlen = 0;
    pthread_mutex_lock(&g_log->mutex); 
	//��ȡ�ֶΣ�����¼�������[****],���������ֽ�
    __log_get_file_sig(file_sig,file_sig_len,sig_buf); 
	
    pthread_mutex_unlock(&g_log->mutex);  
    if(!file_sig)   
    {  
        file_sig_len = 0;
		stringlen = 0;
    }  
    else  
    {  
    	//strcat(buf,"\n");
        strncpy(buf,sig_buf,file_sig_len + 2);  
		stringlen = file_sig_len + 2;
		buf[stringlen] = '\0';
       // strcat(buf,sig_buf);
    }
	
	return stringlen;
}


#if 0

//��ȡ��־��Ϣ��ͷ����Ϣ�������¼�������ʱ�䡢�Զ�����ֶ�
static inline int _log_get_sync_head(char* buf,char* file_sig,int file_sig_len)  
{  
	int stringlen = 0;
    time_t tm_check = time(NULL);  
    char sig_buf[file_sig_len+2];

	char timestring[28];
    if( !buf)  
    {  
        return 0;  
    } 
#if 1
    pthread_mutex_lock(&g_log->mutex); 

	//��ȡϵͳʱ�䣬����¼�������ʱ��
    _log_get_time(tm_check,timestring); 

	//��ȡ�ֶΣ�����¼�������[****],���������ֽ�
    _log_get_file_sig(file_sig,file_sig_len,sig_buf); 
	
    pthread_mutex_unlock(&g_log->mutex);  

	//дʱ�䡢�¼����͵�buf
    strncpy(buf,timestring,LOG_TIME_STR_LEN); 
#else
	pthread_mutex_lock(&g_log->mutex); 

	debug_printf("6g_log->file_count[%d]->filename = %s\n",g_log->file_index - 1,g_log->file_count[g_log->file_index - 1]->filename);
	//��ȡϵͳʱ�䣬����¼�������ʱ��
	_log_get_time(tm_check,g_log->timestring); 

	debug_printf("g_log->last_time_str = %s\n",g_log->timestring);

	//��ȡ�ֶΣ�����¼�������[****],���������ֽ�
	_log_get_file_sig(file_sig,file_sig_len,sig_buf); 
	debug_printf("sig_buf = %s\n",sig_buf);
	pthread_mutex_unlock(&g_log->mutex);  

	//дʱ�䡢�¼����͵�buf
	strncpy(buf,g_log->timestring,LOG_TIME_STR_LEN); 
#endif
    if(!file_sig)   
    {  
        file_sig_len = 0;  
    }  
    else  
    {  
    	//strcat(buf,"\n");
        strncpy(buf+LOG_TIME_STR_LEN + 2,sig_buf,file_sig_len + 2);  
       // strcat(buf,sig_buf);
    }

	stringlen = LOG_TIME_STR_LEN + file_sig_len + 2;
	buf[stringlen] = '\0';

	debug_printf("strlen(buf) = %d,#####buf = %s\n",strlen(buf),buf);
  
    return stringlen;  
  
} 

#endif
  
static void log_close(FILE *fp)
{
	if(fp == NULL)
		return;
	fflush(fp);
	fclose(fp);
}

// д��־ �ڷ�װ �Ƿ�ﵽ�ļ������ֵ  
static int _log_sync_write(log_file_t *f,char* file_sig,int file_sig_len,char* log,int len)  
{  
    int write_len,head_len;
	char file_pwd[128];
	char timestring[28];
	char newest_equ_odest_flag = 0;
    write_len =0;  
	//debug_printf("file_sig_len = %d\n",file_sig_len);
    char buf[file_sig_len+2]; 
	//��ȡ��־��Ϣ��ͷ����Ϣ�������¼�������ʱ�䡢�Զ����ֶ���Ϣ
    //head_len = _log_get_sync_head(buf,file_sig,file_sig_len);

	memset(timestring,0,sizeof(timestring));
	_log_get_sync_time(timestring);
	_log_get_file_sig(buf,file_sig,file_sig_len);
    pthread_mutex_lock(&g_log->mutex);

	memset(file_pwd,0,sizeof(file_pwd));
	sprintf(file_pwd,logdir"/%s/%s",f->filename,f->output_filename);

	//��ǵ�ǰ�����ļ�������ļ��Ƿ���ͬһ���ļ�
	log_debug_printf("f->output_filename = %s,f->oldest_file = %s\n",f->output_filename,f->oldest_file);
	if(strcmp(f->output_filename,f->oldest_file) == 0)
	{
		newest_equ_odest_flag = 1;
		log_debug_printf("================\n");
	}
	
	f->fp = _log_open(file_pwd);
	if(f->fp == NULL)
	{
		pthread_mutex_unlock(&g_log->mutex);
		return -1;
	}
	LOG_DEBUG_PRINTF;
    write_len += _log_write(f,timestring,strlen(timestring));
    write_len += _log_write(f,"\n",1);
    write_len += _log_write(f,buf,file_sig_len+2);
	if(log != NULL)
    	write_len += _log_write(f,log,len);  
    write_len += _log_write(f,"\n\n",2); 
	
	log_close(f->fp);
	LOG_DEBUG_PRINTF;
	//ĳһ����־�ļ��ܴ�С����10Mʱ�Զ�����ɵ�һ���ļ�ɾ��
	log_debug_printf("1f->allsize = %d\n",f->allsize);
	f->allsize += write_len;
	log_debug_printf("2f->allsize = %d\n",f->allsize);
	if(f->allsize > g_log->max_dirsize)
	{
		LOG_DEBUG_PRINTF;
		struct stat st_buf;
		char filepsw[128];
		memset(filepsw,0,sizeof(filepsw));
		sprintf(filepsw,logdir"/%s/%s",f->filename,f->oldest_file);

		stat(filepsw, &st_buf);
		remove(filepsw);
		log_debug_printf("f->allsize = %d,g_log->max_dirsize = %d\n",f->allsize,g_log->max_dirsize);
		f->days -= 1;
		f->file_count -= 1;
		f->allsize -= st_buf.st_size;
		
		LOG_DEBUG_PRINTF;
		//��������ļ�������ļ�����ͬ��һ���ļ�������ɾ������ô��Ҫ���´���һ���ļ�
		//��������������ɵ�ָ��ָ������Ĵ������ļ�
		if(newest_equ_odest_flag)
		{
			LOG_DEBUG_PRINTF;
			memset(f->output_filename,0,sizeof(f->output_filename));
			memset(f->oldest_file,0,sizeof(f->oldest_file));
			
			_log_create_filename(f->filename,f->output_filename);
			
			f->filesize = 0;
			f->allsize = 0;
			f->file_count += 1;

			memcpy(f->oldest_file,f->output_filename,strlen(f->output_filename));
			
		}

		//�������������ͬ���ļ���ɾ������ɵ��ļ���Ҫ�����ҳ���ɵ��ļ�
		else
		{
			LOG_DEBUG_PRINTF;
			char filepsw[128];
			memset(filepsw,0,sizeof(filepsw));
			memset(f->oldest_file,0,sizeof(f->oldest_file));
			sprintf(filepsw,logdir"/%s",f->filename);
			log_find_OldestFile(f->filename,filepsw,f->oldest_file);
		}
	}
	LOG_DEBUG_PRINTF;
    pthread_mutex_unlock(&g_log->mutex);  
    return 1;  
  
}  


 
void  find_file(f_log *g_log,char* filename,log_file_t** created_file)  
{  
    int i;  
    *created_file == NULL; 
	//debug_printf("g_log->file_index = %d\n",g_log->file_index);
	//debug_printf("g_log->file_count[2]->filename = %s\n",g_log->file_count[2]->filename);
    for(i=0;i<g_log->file_index;i++)  
    {  
        if(!strncmp(filename,g_log->file_count[i]->filename,strlen(filename)))  
        {  
            *created_file=g_log->file_count[i];        
  			return;
        }  
    }  
}  
  

typedef struct _log_info_  
{  
    unsigned char name[16]; 		//�ļ�����ǰ׺ 
    unsigned char file_sig[16];  	//�ֶ�
    unsigned char log[256];  		//��־����
    unsigned int  log_len;  		//���ݳ���
    unsigned int  file_sig_len;  	//�ֶγ���
    unsigned int i;  
}log_info;  
  

/************************************************************************
���·�װ�ӿ�
************************************************************************/
int getFileSize(char * strFileName)    
{	
	int ret = 0;
	FILE * fp = fopen(strFileName, "r");   
	LOG_DEBUG_PRINTF;
	log_debug_printf("strFileName = %s\n",strFileName);
	ret = fseek(fp, 0L, SEEK_END); 
	log_debug_printf("ret = %d\n",ret);
	LOG_DEBUG_PRINTF;
	int size = ftell(fp);	
	fclose(fp);  
	LOG_DEBUG_PRINTF;
	return size;   
}	


static void __log_init(void)
{
    int i; 
	int file_count = 0;
	int allsize = 0;
	int days = 0 ;
	char log_dir[128];

    struct stat st_buf;
    f_log *temp_glo=NULL; 
	
    temp_glo = (f_log*)malloc(sizeof(f_log));  
    if(temp_glo == NULL)  
    {  
        log_debug_printf("Can not malloc f_log!!!\r\n");  
        return ;  
  
    }  
    pthread_mutex_init(&temp_glo->mutex,NULL);  
  
    temp_glo->max_filesize = MAX_FILE_SIZE; // 3MB  
    temp_glo->max_dirsize  = MAX_DIR_SIZE;	// 10MB
    temp_glo->file_index = 0;

	//��־ϵͳ�ʼ�����е���־�ļ��ṹ�嶼��ʼ��Ϊ0
	//ÿһ����־�ṹ�����һ����־�ļ�������־Ŀ¼����
	//syslog��userlog��tasklog�ȵȣ�ÿһ��Ŀ¼����ౣ��10�����־
    for(i = 0 ; i < MAX_FILE_COUNT ; i ++)  
    {  
        temp_glo->file_count[i] = NULL;
    }

	//��־ȫ�ֽṹ
    g_log=temp_glo;

	//ϵͳ������������־ϵͳ�Ὣ��־Ŀ¼�µ����й������־ͳһ��������
	//����1����־Ŀ¼������Щ��Ŀ¼��ÿ����Ŀ¼����һ�����־
	//2��ÿ����Ŀ¼�µ���ɵ��ļ������µ��ļ�
	//3��ÿ����Ŀ¼�µ��ļ��ĸ����Լ������ļ��ĵ�ǰ��С


	//1���ҳ���־Ŀ¼�����е���Ŀ¼��ÿ����Ŀ¼����һ����־
	log_find_nameType(g_log,logdir);
	
	log_debug_printf("g_log->file_index =****************** %d\n",g_log->file_index);
	//2���ҳ�ÿһ���͵���־�ļ��������ļ��Լ���ɵ��ļ�
	for(i = 0 ; i < g_log->file_index; i ++)
	{
		
		//�����־��Ŀ¼�Ƿ���ڣ��������򴴽���Ŀ¼
		memset(log_dir,0,sizeof(log_dir));
		sprintf(log_dir,"%s/%s",logdir,g_log->file_count[i]->filename);
		if(access(log_dir,F_OK) < 0)
		{
			mkdir(log_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
			continue;
		}
		allsize = log_get_AllFileSize(g_log->file_count[i]->filename,log_dir);

		log_debug_printf("2allsize============################## = %d\n",allsize);
		//�ҳ���Ŀ¼�µ������ļ�
		memset(g_log->file_count[i]->output_filename,0,sizeof(g_log->file_count[i]->output_filename));
		file_count = log_find_NewestFile(g_log->file_count[i]->filename,log_dir,g_log->file_count[i]->output_filename);
		//�ҳ���Ŀ¼�µ������ļ�
		memset(g_log->file_count[i]->oldest_file,0,sizeof(g_log->file_count[i]->oldest_file));
		log_find_OldestFile(g_log->file_count[i]->filename,log_dir,g_log->file_count[i]->oldest_file);

		log_debug_printf("newest file : %s\noldest file : %s\n",g_log->file_count[i]->output_filename,g_log->file_count[i]->oldest_file);

		days = log_getDays(g_log->file_count[i]->filename,log_dir);
		
		//�ҵ������ļ����ȡ�����ļ����ļ���С
		if(strlen(g_log->file_count[i]->output_filename) > 0)
		{
			strcat(log_dir,"/");
			strcat(log_dir,g_log->file_count[i]->output_filename);
			stat(log_dir, &st_buf);
			g_log->file_count[i]->filesize = st_buf.st_size;
			
		}

		LOG_DEBUG_PRINTF;
		//��Ŀ¼�µ��ļ�����
		g_log->file_count[i]->file_count = file_count;
		g_log->file_count[i]->allsize    = allsize;
		g_log->file_count[i]->days       = days;
		log_debug_printf("##################################################log_dir = %s\n",log_dir);
		
		log_debug_printf("g_log->file_count[i]->allsize = %d\n",g_log->file_count[i]->allsize);
		log_debug_printf("g_log->file_count[i]->file_count = %d\n",g_log->file_count[i]->file_count);
		log_debug_printf("log_dir = %s\nthe newest file size :%d\n",log_dir,(int)st_buf.st_size);
		
	}

}


//һ���Գ�ʼ��ȫ�ֱ���
void log_file_init(void)
{
    pthread_once(&init_create,__log_init);  
}

log_file_t *log_file_create(char *filename)
{
	
    log_file_t *f=NULL;
	char dir[128];
	
	int ret = -1;
	memset(dir,0,sizeof(dir));
	sprintf(dir,"%s/%s",logdir,filename);

    pthread_mutex_lock(&g_log->mutex); 
	//�������ļ��ṹ����Ѱ��ָ���ļ����ҵ������ļ�ָ�룬�����д�ļ����߳���
    find_file(g_log,filename,&f); 
	LOG_DEBUG_PRINTF;
	//���ļ������ṹ�����Ҳ�����Ӧ�ļ��У��򴴽�һ���ļ�
	if(f == NULL)
	{
		if((f = (log_file_t *)malloc(sizeof(log_file_t))) == NULL)
		{
			pthread_mutex_unlock(&g_log->mutex);  
			return NULL;
		}
		log_debug_printf("dir = %s\n",dir);
		ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(ret < 0)
		{
			perror("mkdir");
			pthread_mutex_unlock(&g_log->mutex);
			return NULL;
		}
	    snprintf(f->filename,LOG_MAX_FILE_NAME,"%s",filename);  
	    _log_create_filename(filename,f->output_filename);
		memcpy(f->oldest_file,f->output_filename,strlen(f->output_filename));

		f->allsize = 0;
	    f->filesize = 0;
		f->file_count += 1;		//��ౣ��10�����־�ļ�

		g_log->file_index += 1;
	    g_log->file_count[g_log->file_index - 1]=f;

	}

	//����Ƿ��ҵ���Ӧ���ļ���
    if(!f)  
    {  
        pthread_mutex_unlock(&g_log->mutex);  
        return NULL;  

    }

	//�����ļ����Ƿ�ɶ����ߴ�����,����������������ļ��к�Ȼ��ɾ������?
	//��ô���´���
	if(access(dir,F_OK) < 0)
	{
		ret = mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if(ret < 0)
		{
			perror("mkdir");
			pthread_mutex_unlock(&g_log->mutex);
			return NULL;
		}
	}

	//�ļ��д��ڣ������滹û���ļ���? ���һ�������ļ����ĳ��Ⱦ�֪����û���ļ���
	//ϵͳ�ڳ�ʼ����ʱ����¼��ÿ��Ŀ¼�µ������ļ�������ļ��������û���ļ���
	//��f�ṹ�е�����������ļ��������ᱻ���£����Գ���Ϊ0��ͨ���жϳ����Ƿ�Ϊ0
	//�Ϳ���֪���Ƿ����ļ�
	if(strlen(f->output_filename) <= 0)
	{
		memset(f->output_filename,0,sizeof(f->output_filename));
		memset(f->oldest_file,0,sizeof(f->oldest_file));
	    _log_create_filename(filename,f->output_filename);

		f->allsize = 0;
		f->filesize = 0;
		f->file_count += 1;
		f->days += 1;
		memcpy(f->oldest_file,f->output_filename,strlen(f->output_filename));
		
	}
	//ȡϵͳ�ĵ�ǰ����
    char now_time[22];  
    time_t tm_time = time(NULL);  
    struct tm now; 
	char *p =NULL;
    gmtime_r(&tm_time,&now);  
    sprintf(now_time,"%04d_%02d_%02d",(now.tm_year+1900),now.tm_mon+1,now.tm_mday);
	
	//ȡ��־�����ļ�������
	p = strchr(f->output_filename,'_');
	if(p == NULL)
	{
		pthread_mutex_unlock(&g_log->mutex);  
		return NULL;
	}
	//�Ա�ϵͳ��ǰ�����������ļ������ڣ������ǰϵͳ�����ڱ��ļ�������,
	//�����´�����־�ļ�
	if(strncmp(now_time,p+1,10) != 0)
	{
	    _log_create_filename(filename,f->output_filename); 
		
		f->filesize = 0;
		f->file_count += 1;
		f->days += 1;

		//����10���ļ����п����Ѿ�����10���������ˣ�����10������
		//�����ɵ�һ���ļ�ɾ��,�������ҳ���ɵı�����f->oldest_file��
		if(f->days > 10)
		{
			char filepsw[128];
			memset(filepsw,0,sizeof(filepsw));
			sprintf(filepsw,logdir"/%s/%s",f->filename,f->oldest_file);
			remove(filepsw);
			f->days -= 1;

			memset(filepsw,0,sizeof(filepsw));
			sprintf(filepsw,logdir"/%s",f->filename);
			log_find_OldestFile(f->filename,filepsw,f->oldest_file);
		}
	}
	
    pthread_mutex_unlock(&g_log->mutex); 
	return f;
}

// ����д & ������д 
//filename:		�ļ�����
//file_sig:		�Զ�����ֶΣ��������ڼ�¼��˭�����ĵȵ�
//file_sig_len:	�Զ����ֶγ���
//log:			��־����ϸ����
//len:			��־����ϸ���ݵĳ���
int log_file_write(char *filename,char* file_sig,int file_sig_len,char* log,int len)  
{  
	log_file_t *f=NULL; 
	

	//���ȴ�ϵͳ��ά����������־ϵͳ�ṹ����filename����ָ������־���ͽṹ
	f = log_file_create(filename);
	if(f == NULL)
	{
		return -1;
	}

  	//д��־�ļ�
    _log_sync_write(f,file_sig,file_sig_len,log,len); 
    return 0;  
}  


#if 0

int main()  
{  
 #if 1
    struct timeval start,end;  
    log_info log_test;  
    pthread_t pid[100];  
    memset(&log_test,0x00,sizeof(log_test));  
    strcpy(log_test.name,"ninglog");  
    strcpy(log_test.file_sig,"ning");  
    strcpy(log_test.log,"huang ke ning da ben dan !");  
    //  strcpy(log_test.log,"fdsajltest");  
    log_test.log_len=strlen(log_test.log);  
    log_test.file_sig_len = strlen(log_test.file_sig);  

	
    log_file_t *f=NULL;  
    int i,ret;  
    //f=log_create(log_test.name);  
    memset(&start,0x00,sizeof(start));  
    memset(&end,0x00,sizeof(end)); 

	log_file_init();
	//f = log_file_create(log_test.name);
	DEBUG_PRINTF;
    for(i=0;i<400000;i++)  
    {  
        if(i%10 == 0)  
        {  
            debug_printf("a\r\n");  
    //      f=log_create(log_test.name);  
        }  
		//sleep(1);
		//read_file_init(NULL);
		debug_printf("i = %d\n",i);
        _log_file_write_(log_test.name,log_test.file_sig,log_test.file_sig_len,log_test.log,log_test.log_len);  
    }  
  
   
#else   
  
  
    gettimeofday(&start,NULL);  
    for(i=0;i<3;i++)  
    {  
        //      log_test.i=i;  
        //  debug_printf("i is %d\n",i);  
        ret=pthread_create(&pid[i],NULL,thread_func,(void*)&log_test);  
        pthread_join(pid[i],NULL);  
        if(ret!=0)  
            debug_printf("can not create!\n");  
        usleep(100);  
    }  
    sleep(10);  
    gettimeofday(&end,NULL);  
    debug_printf("time is %d\r\n",(end.tv_sec-start.tv_sec)*1000000+(end.tv_usec-start.tv_usec));  
  
#endif  
  
    return 0;  
  
}  
#endif


