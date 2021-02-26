#ifndef __MYERROR_H
#define __MYERROR_H

typedef enum __err
{
	ERR_OK 	  = 0,				//成功
	ERR_ERROR,					//错误
	ERR_FAIL,					//失败
	ERR_BUSY,					//在忙
	ERR_TIMEOUT,				//超时
	ERR_NOTEXIST,				//不存在
	ERR_ISEMPTY,
	ERR_MOLLOC_FAILED,			//动态分配内存失败
	ERR_OPEN_FAILED,			//打开失败
	ERR_STRING_NOTEQU,			//字符串不一致
	ERR_FILE_NOTEXIST,			//文件不存在
	ERR_USER_NOTEXIST,			//用户不存在
	ERR_SOCKET_SHUTDOWN,		//socket关闭
	ERR_UNKNOWN					//不明错误
}err_t;


#endif

