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


//--------------------------------------------------------------命令列表 --------------------------------------------------------//
#define UG_GET_VERSION    			0x3633   /*查询版本号（帧类型“63”）暂定维护模式下使用*/


#define UG_FILE_TX					0x3039   /*取可变信息标志的当前显示内容*/
#define UG_FILE_RX					0x3130  /*向可变信息标志上载文件*/


//查询设备信息
#define UG_COMMUNICATION_STATE      0x3030
//#define UG_TX_DETAIL_STATE			0x3530

#define UG_RESET_TX					0x3533
#define UG_RESET_RX        			0x3534

#define UG_SET_TESTMODE				0x3536  /*设置测试状态（帧类型“56”）*/
#define SET_DISPLAY_PARAMETER       0x3537 /*设置TX/RX显示参数*/
#define UG_SAVE_PARAMETER        	0x3538 /*修改和保存TX/RX显示参数*/
#define GET_DISPLAY_PARAMETER		0x3542/* 查询TX/RX显示参数（帧类型”5B”）*/

#define SET_UPGRADE_PARAMETER       0x3539  /*设置TX/RX升级配置参数（帧类型”59”)*/
#define GET_UPGRADE_PARAMETER 		0x3541	/*查询TX/RX升级配置参数（帧类型”5A”）*/

#define UG_SET_MODE                 0x3930  /*给TX/RX发送升级文件的标志*/
#define GET_UPGRADE_STATE           0x3933	/*上位机查询TX/RX文件是否发送完成的标志*/


#define UG_BRIGHT_AUTO				0x30  //自动模式
#define UG_BRIGHT_HAND				0x31  //手动模式

#define UG_BRIGHT_RANK				32		//亮度等级32级
#define UG_BRIGHT_MAX				31		//亮度最大值
#define UG_BRIGHT_MIN				0		//亮度最小值

//#define UG_SCREEN_ON				0x31	//大屏开屏
//#define UG_SCREEN_OFF				0x32	//大屏关电
//#define UG_SCREEN_CHECK				0x30	//查询大屏开关状态

#define 	UGLIST	list_dir_1"/play.lst"


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
