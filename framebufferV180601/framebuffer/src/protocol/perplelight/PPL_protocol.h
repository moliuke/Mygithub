#ifndef __PPL_PROTOCOL_H
#define __PPL_PROTOCOL_H

#include "config.h"
#include "mylist.h"
#include "PPL_Lstparse.h"
#include "../../include/common.h"

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
#define 	PPL_BRIGHT_MIN  2

#define shift(A,N)				(A + (N))
#define PLL_START_BYTE_POS		0
#define PLL_DEVID_BYTE_POS		1
#define PLL_CMDID_BYTE_POS		3
#define PLL_DATAS_BYTE_POS		5
#define PLL_PARITY_BYTE_POS(N)	shift(PLL_DATAS_BYTE_POS,N)
#define PLL_END_BYTE_POS(N)		shift(PLL_DATAS_BYTE_POS,N+2)

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

// 设备总状态
#define CMD_DEVICE_COM               0x3030
#define CMD_DEVICE_TOTAL             0x3031		/*refer the total status of the device*/
#define CMD_DEVICE_POINT             0x3032		/*refer the status of the pixels*/
#define CMD_DEVICE_RESET             0x3033
#define CMD_DEVICE_OPENCLOSE         0x3034
#define CMD_DEVICE_BRIGHTMODE        0x3035		/*refer the brightness of the device*/
#define CMD_DEVICE_SETBRIGHT         0x3036		/*set the brightness of the device*/
#define CMD_DEVICE_DETAIL            0x3037		/*refer the detail status of the device*/
#define CMD_DEVICE_POWERMODE         0x3038
#define CMD_DEVICE_DRIVERS           0x3039    	/*refer the status of the device driver channel*/                                  
// 内容发布
#define CMD_FILE_DOWNLOAD            0x3230
#define CMD_FILE_UPLOAD              0x3231
#define CMD_INFO_DISPLAY             0x3232		/*set the preseting displaying list,which is going to be display at the future*/
#define CMD_INFO_PLAYLIST            0x3233		/*refer the list of the contents for displaying*/
#define CMD_INFO_STRING              0x3234		/*refer the displaying content at present time*/

#define CMD_BIGFILE_DOWNLOAD		 0x3330
#define CMD_BIGFILE_UPLOAD			 0x3331
#define CMD_SET_IPADRESS			 0x3338

// 维护指令
#define CMD_UTIL_SETTIME             0x3430		/*set the current time of the device*/
#define CMD_UTIL_GETTIME             0x3431		/*refer the current time of the device*/
#define CMD_UTIL_LASTTIME            0x3432		/*refer the starting time of de device*/
#define CMD_UTIL_READID              0x3433
#define CMD_UTIL_SETID               0x3434
#define CMD_UTIL_VERCHECK            0x3435

#define PPL_BRIGHT_AUTO		0x30
#define PPL_BRIGHT_HAND		0x31

#define 	PLAY_LIST	list_dir_1"/play.lst"


typedef struct
{
	user_t		*user;
	uint8_t 	startByte;
	uint16_t 	DEVID;
	uint16_t 	CMDID;
	uint8_t 	*data;
	uint32_t 	length;
	uint16_t 	parity;
	uint8_t 	endByte;
}PPL_PROTOCOLStruct_t;

typedef enum
{
	PPL_ERR_OK		= 0,
	PPL_ERR_FAILED,
	PPL_ERR_ARGUMENT,
	PPL_ERR_BUSY,
	PPL_ERR_NOMEM
}PPLError_t;

void DecodeHexCMD(char *CmdString,unsigned int nCmdlength);                //  cmd decode
int PPL_protocolProcessor(user_t *user,uint8_t *PTCdata,uint32_t *PTClenth);

#endif
