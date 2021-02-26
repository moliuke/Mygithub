#ifndef __ZC_PROTOCOL_H
#define __ZC_PROTOCOL_H
#include <stdio.h>
#include "common.h"



#define 	ZC_BRIGHT_MIN	2
//#define 	MEMSIZE		((16 + 8) * 1024)

#define 	ZC_PLAY_LST		list_dir_1"/play.lst"

#define 	SET_DEV_IP						0x3338
#define 	GET_DETAIL_STATUS				0x3031
#define 	RECV_FILE_FROMUPPER				0x3032
#define 	UPLOAD_FILE_TOUPPER				0x3033
#define 	SET_PLAY_LIST					0x3034
#define 	GET_PLAY_CONTENT				0x3035
#define 	SET_BRIGHT_MODE					0x3036
#define 	SET_BRIGHT_VAL					0x3037
#define 	GET_BRIGHTMODE_AND_BRIGHTVAL	0x3038
#define 	SET_CURRENT_TIME				0x3039
		
#define 	COMPATIBLEMODE					0xffff

//�ļ����շ�����ʵ�����Ѿ����ļ����շ�ָ���ˣ�������������ʱ��Ҫʹ�����ǵ��ļ��շ������������ļ�
//���������ｫ�ԿƵ��շ��ļ���Э����ݵ��γ�Э����
#define 	RECV_FILE_2K					0x3230
#define 	SEND_FILE_2K					0x3231
#define 	RECV_FILE_16K					0x3330
#define 	SEND_FILE_16K					0x3331

//��������γ�Э��û�и�ָ������ڼ�ؽ������賿2:30����ǰҪ�����ص���Ļ���ɼ�ؼ�ؽ���
//����һ������ָ������򣬳�����Ҫ�ȹص���Ļ
#define 	SET_SCREEN_STATUS				0x3034



#define ZC_BRIGHT_RANK		32		//���ȵȼ�32��
#define ZC_BRIGHT_AUTO		0x30	//�Զ�����
#define ZC_BRIGHT_HAND		0x31	//�ֶ�����



#define shift(A,N)					(A + (N))
#define ZC_START_BYTE_POS			0
#define ZC_DEVID_BYTE_POS			1
#define ZC_DATAS_BYTE_POS			3
#define ZC_PARITY_BYTE_POS(N)		shift(ZC_DATAS_BYTE_POS,N)
#define ZC_END_BYTE_POS(N)			shift(ZC_DATAS_BYTE_POS,N+2)

#define COMPAT_START_BYTE			0
#define COMPAT_SWR_CMD_POS			1
#define COMPAT_SWR_DEV_POS			3
#define COMPAT_SWR_DATA_POS			5
#define COMPAT_PARITY_BYTE_POS(N)	shift(COMPAT_SWR_DATA_POS,N)
#define COMPAT_END_BYTE_POS(N)		shift(COMPAT_SWR_DATA_POS,N + 2)


typedef struct __ZCstruct
{
	user_t		*user;
	
	uint8_t 	StartByte;
	uint16_t 	DevAddr;
	uint16_t 	CmdCode;
	uint16_t 	CRC16;
	uint32_t 	DataLen;
	uint8_t 	*Data;
	uint8_t 	EndByte;
}ZCPTCStruct_t;

int ZC_protocolProcessor(user_t *user,uint8_t *input,uint32_t *inputlen);


#endif

