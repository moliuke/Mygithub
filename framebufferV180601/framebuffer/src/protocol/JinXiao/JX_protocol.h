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
// 系统状态
typedef enum {nXKTest,nXKDisplay,nXKCommunicate,nXKDebug}XKCmsStatus;  //  运行状态 

typedef enum {XKEffectInit, XKEffectIn, XKEffectShow,XKEffectOut}XKDisplayStatus;  //  显示状态

typedef enum {XKCom1Port,XKCom2Port,XKCom3Port,XKCom4Port,XKSocketPort}XKCommunicatePort; 

typedef enum { XKMsgNorm, XKMsgAlarm, XKMsgDebug, XKMsgTest, XKMsgDeviceinfo}XKMessageType; 
            
typedef enum { DataNorm, DataBin, DataHex } DataType;    


extern XKDisplayStatus DisplayStatus;
#endif

//--------------------------------------------------------------命令列表 --------------------------------------------------------//
#define JX_SET_BRIGHT				0x3033   //设置亮度模式与亮度值

#define JX_FILE_TX					0x3039   /*取可变信息标志的当前显示内容*/
#define JX_FILE_RX					0x3130  /*向可变信息标志上载文件*/

#define JX_CUR_DSP					0x3937  /*取可变信息标志的当前显示内容*/
#define JX_SET_DSP					0x3938  /*使可变信息标志显示预置的播放表*/
#define JX_GET_BRIGHT				0x3036  /*设置可变信息标志的亮度调节方式和显示亮度*/

#define JX_CHECK_TROUBLE			0x3031   /*取可变信息标志的当前故障*/

#define JX_BRIGHT_AUTO				0x30  //自动模式
#define JX_BRIGHT_HAND				0x31  //手动模式

#define JX_BRIGHT_RANK				32		//亮度等级32级
#define JX_BRIGHT_MAX				31		//亮度最大值
#define JX_BRIGHT_MIN				0		//亮度最小值

//#define JX_SCREEN_ON				0x31	//大屏开屏
//#define JX_SCREEN_OFF				0x32	//大屏关电
//#define JX_SCREEN_CHECK				0x30	//查询大屏开关状态

#define 	JXLIST	list_dir_1"/play.lst"


typedef struct
{
	user_t		*user;
	uint8_t 	startByte;
	uint16_t 	DEVID;
	uint16_t 	CMDID;
	uint8_t 	*data;
	uint32_t 	length;  //有效数据长度
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
