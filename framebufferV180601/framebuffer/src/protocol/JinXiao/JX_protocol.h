#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include "config.h"
#include "mylist.h"
#include "JX_Lstparse.h"
#include "../../include/common.h"

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

#define shift(A,N)				(A + (N))
#define JX_START_BYTE_POS		0
#define JX_DEVID_BYTE_POS		1
#define JX_DATAS_BYTE_POS		3
#define JX_PARITY_BYTE_POS(N)	shift(JX_DATAS_BYTE_POS,N)
#define JX_END_BYTE_POS(N)		shift(JX_DATAS_BYTE_POS,N+2)

#if 0
// ϵͳ״̬
typedef enum {nXKTest,nXKDisplay,nXKCommunicate,nXKDebug}XKCmsStatus;  //  ����״̬ 

typedef enum {XKEffectInit, XKEffectIn, XKEffectShow,XKEffectOut}XKDisplayStatus;  //  ��ʾ״̬

typedef enum {XKCom1Port,XKCom2Port,XKCom3Port,XKCom4Port,XKSocketPort}XKCommunicatePort; 

typedef enum { XKMsgNorm, XKMsgAlarm, XKMsgDebug, XKMsgTest, XKMsgDeviceinfo}XKMessageType; 
            
typedef enum { DataNorm, DataBin, DataHex } DataType;    


extern XKDisplayStatus DisplayStatus;
#endif

//--------------------------------------------------------------�����б� --------------------------------------------------------//
#define JX_SET_BRIGHT				0x3033   //��������ģʽ������ֵ

#define JX_FILE_TX					0x3039   /*ȡ�ɱ���Ϣ��־�ĵ�ǰ��ʾ����*/
#define JX_FILE_RX					0x3130  /*��ɱ���Ϣ��־�����ļ�*/

#define JX_CUR_DSP					0x3937  /*ȡ�ɱ���Ϣ��־�ĵ�ǰ��ʾ����*/
#define JX_SET_DSP					0x3938  /*ʹ�ɱ���Ϣ��־��ʾԤ�õĲ��ű�*/
#define JX_GET_BRIGHT				0x3036  /*���ÿɱ���Ϣ��־�����ȵ��ڷ�ʽ����ʾ����*/

#define JX_CHECK_TROUBLE			0x3031   /*ȡ�ɱ���Ϣ��־�ĵ�ǰ����*/

#define JX_BRIGHT_AUTO				0x30  //�Զ�ģʽ
#define JX_BRIGHT_HAND				0x31  //�ֶ�ģʽ

#define JX_BRIGHT_RANK				32		//���ȵȼ�32��
#define JX_BRIGHT_MAX				31		//�������ֵ
#define JX_BRIGHT_MIN				0		//������Сֵ

//#define JX_SCREEN_ON				0x31	//��������
//#define JX_SCREEN_OFF				0x32	//�����ص�
//#define JX_SCREEN_CHECK				0x30	//��ѯ��������״̬

#define 	JXLIST	list_dir_1"/play.lst"


typedef struct
{
	user_t		*user;
	uint8_t 	startByte;
	uint16_t 	DEVID;
	uint16_t 	CMDID;
	uint8_t 	*data;
	uint32_t 	length;  //��Ч���ݳ���
	uint16_t 	parity;
	uint8_t 	endByte;
}JX_PROTOCOLStruct_t;

typedef enum
{
	JX_ERR_OK		= 0,
	JX_ERR_FAILED,
	JX_ERR_ARGUMENT,
	JX_ERR_BUSY,
	JX_ERR_NOMEM
}JXError_t;

void DecodeHexCMD(char *CmdString,unsigned int nCmdlength);                //  cmd decode
int JX_protocolProcessor(user_t *user,uint8_t *PTCdata,uint32_t *PTClenth);

#endif
