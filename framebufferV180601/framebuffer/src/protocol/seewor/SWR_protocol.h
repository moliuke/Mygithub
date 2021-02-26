#ifndef __SEEWOR_PROTOCOL_H
#define __SEEWOR_PROTOCOL_H

#include "config.h"
#include "mylist.h"
#include "../../include/common.h"

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

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
#define CMD_SET_WARMING				 0x3130		//警告



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

#define CMD_RST_TX_CARD				 0x3533
#define CMD_RST_RX_CARD				 0x3534
#define CMD_SET_MODULE_POWER		 0x3535
#define CMD_SET_TEST_MODE            0x3536    //add by mo 2020.9.18


#define CMD_GET_SMOK				 0x3531		/*获取烟雾值*/

//加入
#define CMD_SET_MODE                 0x3930     /*设置工控机升级模式*/
#define CMD_SET_PIXMODE              0x3934    /*像素点模式 add by mo 2020.7.7*/
#define CMD_GET_VERSION				 0x3633
#define CMD_GET_DISPLAY_PARAMETER	 0x3542/* 查询TX/RX显示参数（帧类型”5B”）*/


#define shift(A,N)				(A + (N))
#define START_BYTE_POS			0
#define CMDID_BYTE_POS			1
#define DEVID_BYTE_POS			3
#define DATAS_BYTE_POS			5
#define PARITY_BYTE_POS(N)	shift(DATAS_BYTE_POS,N)
#define END_BYTE_POS(N)		shift(DATAS_BYTE_POS,N+2)





typedef struct 
{
	uint16_t 	 cmdID;
	uint16_t     devID;
}head_t;

typedef struct 
{
	head_t 			head;
	uint8_t			*data;
	uint16_t 		parity;
	uint16_t		length;
}protclmsg_t;

typedef struct _PROTOCOLStruct
{
	user_t			*usermsg;
	
	uint8_t 	 	startByte;
	protclmsg_t		protcmsg;
	uint8_t		 	endByte;

	int (*GetCommunitStatus)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetDevTotalStatus)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetDevIP)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetDevDetailStatus)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetDevPixStatus)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetDevBright)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetDevBright)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetCurPlayLst)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetCurPlayLst)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetCurSysTime)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetSysStartTime)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetSysCurTime)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetCurPlayString)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetScreenStatus)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetDevPowerMode)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetSysReset)(struct _PROTOCOLStruct *,unsigned int *);
	int (*getSmok)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetDevDrvStatus)(struct _PROTOCOLStruct *,unsigned int *);

	int (*SetDevTestMode)(struct _PROTOCOLStruct *,unsigned int *); //add by mo 2020.9.18
	int (*FileFrameRX2K)(struct _PROTOCOLStruct *,unsigned int *);
	int (*FileFrameTX2K)(struct _PROTOCOLStruct *,unsigned int *);
	int (*FileFrameRX16K)(struct _PROTOCOLStruct *,unsigned int *);
	int (*FileFrameTX16K)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetRstTxCard)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetRstRxCard)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetModulePower)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetWarming)(struct _PROTOCOLStruct *,unsigned int *);
	int (*SetPixMode)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetVersion)(struct _PROTOCOLStruct *,unsigned int *);
	int (*GetDisplayParameter)(struct _PROTOCOLStruct *,unsigned int *);
	int (*extendcmd)(struct _PROTOCOLStruct *,unsigned int *);
	
}Protocl_t;


typedef struct __PROTOCOLInterf
{
	int	(*SWR_GetCommunitStatus)();
}PROTOCOLInterf_t;


typedef struct __protocol
{
	user_t		*usermsg;
	Protocl_t	*protocolmsg;
}protocol_t;





typedef enum
{
	PTCL_ERR_OK		= 0,
	PTCL_ERR_FAILED,
	PTCL_ERR_ARGUMENT,
	PTCL_ERR_BUSY,
	PTCL_ERR_NOMEM
}PTCL_err_t;

int get_communitStatus(Protocl_t *protocol,unsigned int *len);

void DecodeHexCMD(char *CmdString,unsigned int nCmdlength);                //  cmd decode


int set_preset_playlist(Protocl_t *protocol,unsigned int *len);
int set_netport(Protocl_t *protocol,unsigned int *len);
int FunctonExtend(Protocl_t *protocol,unsigned int *len);
int swr_protocolProcessor(user_t *user,uint8_t *input,uint32_t *inputlen);
void PROTOCOLInterfInit(void);

extern uint8_t *FREEdata;
extern Protocl_t PROTOCOLStruct;

#endif

