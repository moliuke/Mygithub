#ifndef __MYERROR_H
#define __MYERROR_H

typedef enum __err
{
	ERR_OK 	  = 0,				//�ɹ�
	ERR_ERROR,					//����
	ERR_FAIL,					//ʧ��
	ERR_BUSY,					//��æ
	ERR_TIMEOUT,				//��ʱ
	ERR_NOTEXIST,				//������
	ERR_ISEMPTY,
	ERR_MOLLOC_FAILED,			//��̬�����ڴ�ʧ��
	ERR_OPEN_FAILED,			//��ʧ��
	ERR_STRING_NOTEQU,			//�ַ�����һ��
	ERR_FILE_NOTEXIST,			//�ļ�������
	ERR_USER_NOTEXIST,			//�û�������
	ERR_SOCKET_SHUTDOWN,		//socket�ر�
	ERR_UNKNOWN					//��������
}err_t;


#endif

