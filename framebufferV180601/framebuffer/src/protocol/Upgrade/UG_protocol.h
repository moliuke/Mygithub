#ifndef __UG_PROTOCOL_H
#define __UG_PROTOCOL_H

#include "../../include/config.h"
#include "../../include/mylist.h"
//#include "UG_Lstparse.h"
#include "../../include/common.h"

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

#define shift(A,N)				(A + (N))
#define UG_START_BYTE_POS		0
#define UG_CMDID_BYTE_POS		1
#define UG_DEVID_BYTE_POS		3


#define UG_DATAS_BYTE_POS		5
#define UG_PARITY_BYTE_POS(N)	shift(UG_DATAS_BYTE_POS,N)
#define UG_END_BYTE_POS(N)		shift(UG_DATAS_BYTE_POS,N+2)


//--------------------------------------------------------------�����б� --------------------------------------------------------//
#define UG_GET_VERSION    			0x3633   /*��ѯ�汾�ţ�֡���͡�63�����ݶ�ά��ģʽ��ʹ��*/


#define UG_FILE_TX					0x3039   /*ȡ�ɱ���Ϣ��־�ĵ�ǰ��ʾ����*/
#define UG_FILE_RX					0x3130  /*��ɱ���Ϣ��־�����ļ�*/


//��ѯ�豸��Ϣ
#define UG_COMMUNICATION_STATE      0x3030
//#define UG_TX_DETAIL_STATE			0x3530

#define UG_RESET_TX					0x3533
#define UG_RESET_RX        			0x3534

#define UG_SET_TESTMODE				0x3536  /*���ò���״̬��֡���͡�56����*/
#define SET_DISPLAY_PARAMETER       0x3537 /*����TX/RX��ʾ����*/
#define UG_SAVE_PARAMETER        	0x3538 /*�޸ĺͱ���TX/RX��ʾ����*/
#define GET_DISPLAY_PARAMETER		0x3542/* ��ѯTX/RX��ʾ������֡���͡�5B����*/

#define SET_UPGRADE_PARAMETER       0x3539  /*����TX/RX�������ò�����֡���͡�59��)*/
#define GET_UPGRADE_PARAMETER 		0x3541	/*��ѯTX/RX�������ò�����֡���͡�5A����*/

#define UG_SET_MODE                 0x3930  /*��TX/RX���������ļ��ı�־*/
#define GET_UPGRADE_STATE           0x3933	/*��λ����ѯTX/RX�ļ��Ƿ�����ɵı�־*/


#define UG_BRIGHT_AUTO				0x30  //�Զ�ģʽ
#define UG_BRIGHT_HAND				0x31  //�ֶ�ģʽ

#define UG_BRIGHT_RANK				32		//���ȵȼ�32��
#define UG_BRIGHT_MAX				31		//�������ֵ
#define UG_BRIGHT_MIN				0		//������Сֵ

//#define UG_SCREEN_ON				0x31	//��������
//#define UG_SCREEN_OFF				0x32	//�����ص�
//#define UG_SCREEN_CHECK				0x30	//��ѯ��������״̬

#define 	UGLIST	list_dir_1"/play.lst"


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
}UG_PROTOCOLStruct_t;

typedef enum
{
	UG_ERR_OK		= 0,
	UG_ERR_FAILED,
	UG_ERR_ARGUMENT,
	UG_ERR_BUSY,
	UG_ERR_NOMEM
}UGError_t;

void DecodeHexCMD(char *CmdString,unsigned int nCmdlength);                //  cmd decode
int UG_protocolProcessor(user_t *user,uint8_t *PTCdata,uint32_t *PTClenth);

#endif
