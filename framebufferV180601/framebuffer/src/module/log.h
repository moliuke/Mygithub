#ifndef __LOG_H#define __LOG_H #ifdef __cpluscplus  extern "C"{  #endif    #include <stdio.h>   #include <stdlib.h>  #include <pthread.h>  #include <sys/epoll.h>  #include <sys/eventfd.h>  #include <fcntl.h>  #include <unistd.h>  #include <string.h>  #include <time.h>  #include <error.h>  //#define LOG_DEBUG#ifdef LOG_DEBUG#define LOG_DEBUG_PRINTF 	DEBUG_PRINTF#define log_debug_printf	debug_printf#else#define LOG_DEBUG_PRINTF 	#define log_debug_printf	#endif  #define LOG_MAX_FILE_NAME   (64)  #define LOG_MAX_OUTPUT_NAME (128) #define LOG_MAX_DATE		(11)  #define LOG_TIME_STR_LEN (21)    #define MAX_FILE_COUNT (20)    #define MAX_FILE_SIZE (1024 * 1024 * 3) #define MAX_DIR_SIZE  (1024 * 1024 * 10)  /****************   每一个文件对于一个这样的结构体   ，不管是多线程写还是单线程    都是对应一个描述文件的结构体  ****************************/    typedef struct _log_file_t_  {      FILE *fp;  //文件指针      unsigned char filename[LOG_MAX_FILE_NAME];//输入文件的名字      unsigned char output_filename[LOG_MAX_OUTPUT_NAME];//输出文件的名字    unsigned char oldest_file[LOG_MAX_OUTPUT_NAME];    unsigned char date[LOG_MAX_DATE];	unsigned int  days;	unsigned int  file_count;		//描述日志保留了多少天    unsigned int  filesize;			//当前正在写文件的大小      unsigned int  allsize;			//该结构下所有文件的大小总和    unsigned int  active_count;		//写文件的线程数  }log_file_t;    typedef struct _log_t  {      pthread_key_t key;      pthread_mutex_t mutex;      unsigned int roll_size;      unsigned int max_filesize;	unsigned int max_dirsize;	unsigned int file_amout;    log_file_t *file_count[MAX_FILE_COUNT];      unsigned char last_time_str[LOG_TIME_STR_LEN+10]; 	char timestring[28];    unsigned int file_index;  }f_log;      char* _log_create_filename(char* filename,char* output_filename);    #ifdef __cpluscplus  }  #endif    
void log_write(char *logtext,int len);void log_file_init(void);int log_file_write(char *filename,char* file_sig,int file_sig_len,char *log,int len);  int systemlog(const char *file,char *keystring,char *keyvals);int write_log (FILE* pFile, const char *format, ...); 

#endif

