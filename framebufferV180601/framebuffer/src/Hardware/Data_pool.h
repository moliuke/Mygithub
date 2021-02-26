#ifndef __DATA_POOL_H
#define __DATA_POOL_H

#include <stdio.h>
#include "../include/debug.h"
#include "../include/config.h"

extern uint8_t Lbrightflag;  //光敏异常标志位，0为正常，1为异常

//#define BRIGHT_AUTO			0x30
//#define BRIGHT_HAND			0x31


#define OPS_MODE_GET		0x01
#define OPS_MODE_SET		0x02

#define COM_NUM				4

#define PID_LIGHT_SENSITIVE		0x01
#define PID_THANDER				0x04
#define PID_SMOG				0x05
#define PID_HUMIDITY			0x06
#define PID_POWER				0x07
#define PID_CTROLLER			0x08
#define PID_TOTAL_TEMP			0x09
#define PID_BOX_TEMP			0x0a
#define PID_MODEL				0x0b
#define PID_MODEL_POWER			0x0c
#define PID_PIXELS				0x0d
#define PID_CHANNEL				0x0e
#define PID_FAN					0x0f
#define PID_DOOR				0x10
#define PID_SYSCHECK			0x11
#define PID_TEMP				0x12


#define RX_PID_TEMP				0x01
#define RX_PID_VOLT1			0x02
#define RX_PID_VOLT2			0x03
#define RX_PID_VOLT3			0x04
#define RX_PID_VOLT4			0x05
#define RX_PID_DOOR				0x06
#define RX_PID_BADPIXH			0x07
#define RX_PID_BASPIXL			0x08
#define RX_PID_CHNANEL			0x09
#define RX_PID_LSENSA			0x0A
#define RX_PID_LSENSB			0x0B



#define TEMP_POS 	(0+17)
#define VOL1_POS	(1+17)
#define VOL2_POS	(2+17)
#define VOL3_POS	(3+17)
#define VOL4_POS	(4+17)
#define DOOR_POS	(5+17)
//这里加入接收版本偏移，去掉了像素和通道
#define MVER_POS	(6+17) 
#define SVER_POS	(7+17) 


#if 0
#define PBAD1_POS	8
#define PBAD2_POS	9
#define CBAD_POS	10
#endif

#define RXNUM_POS		1
#define LIGHT1_POS		2
#define LIGHT2_POS		3

#define LIGHT_VATAL     4
#define THANDER_POS     5

#define TX_MVER			15
#define TX_SVER			16



typedef struct __com
{
	uint8_t COMdatabits;		//串口位数
	uint8_t COMstopbits;		//停止位数
	uint8_t COMflowctl;			//流控制
	uint8_t COMparity;			//奇偶校验
	uint32_t COMbaudrate;		//波特率
}COMserial_t;


typedef struct __RXcard
{
	uint8_t Temp;
	uint8_t Volt1;
	uint8_t Volt2;
	uint8_t Volt3;
	uint8_t Volt4;

	uint8_t Door;

//加入接收卡版本号 2020.3.16	
	uint8_t Mver;
    uint8_t Sver;

#if 1
//关于像素点点检，暂时不用
	uint8_t reserd1;

	uint8_t reserd2;
	uint8_t BadPH;
	uint8_t BadPL;
	uint8_t Channel;
	uint8_t reserd3;
#endif
}RXcard_t;

typedef struct __pool_
{
	//温度
	uint8_t TMPstatus;			//总温度状态
	uint8_t TMPvals;			//总温度值
	
	//电源
	uint8_t PWstatus;			//电源的状态
	uint8_t PWvals;				//电源的值

	//门开关
	uint8_t Dstatus;
	uint8_t Dvals;

	//像素坏点数
	uint8_t PIXstatus;			//屏幕像素的状态
	uint8_t PIXmode;            //像素检测类型 00表示取当前显示内容01表示坏点信息02表示接收卡原
	uint8_t *PIXdata;			//像素所有值
	uint32_t PIXvals;			//屏幕像素坏点数
	uint32_t PIXdataLen;		//像素数据总长，单位:Byte
	uint8_t PIXcolor;           //屏幕显示颜色
	
	//光敏
	uint8_t LSstatus;			//光敏的状态
	uint8_t LSvals;				//光敏的值

	//防雷器
	uint8_t THDstatus;			//防雷器的状态
	uint8_t THDvals;			//防雷器的值

	//RX卡参数
	uint8_t RXnum;
	RXcard_t RXcard[24];
	uint8_t LSensA;
	uint8_t LSensB;

	//驱动通道
	uint8_t CHNstatus;
	uint8_t CHNvals;
	

	//风扇
	uint8_t FANstatus;
	uint8_t FANvals;

	//烟雾值
	uint8_t SMKstatus;			//烟雾状态
	uint16_t SMKvals;			//烟雾值

	//环境温度
	uint8_t Tempstatus;			//烟雾状态
	uint16_t Tempvals;			//烟雾值

	//湿度
	uint8_t HUMstatus;			//湿度状态
	uint16_t HUMvals;			//湿度值

	//黄闪灯状态
	uint8_t YLstatus;			
	uint8_t YLvals;			

	//控制器
	uint8_t CTRLstatus;			//控制器的状态
	uint8_t CTRLvals;			//控制器的值

	//控制软件故障
	uint8_t SofStatus;
	uint8_t Sofvals;

	//硬件故障
	uint8_t HardStatus;
	uint8_t Hardvals;

	//检测系统故障
	uint8_t SysStatus;
	uint8_t Sysvals;

	//通信故障
	uint8_t CommuStatus;
	uint8_t Commuvals;

	//显示模组
	uint8_t MODstatus;			//显示模组的状态
	uint8_t MODvals;			//显示模组的值，意义待定

	//显示模组电源
	uint8_t MODPstatus;			//模组电源的状态
	uint8_t MODPvals;			//模组电源的值

	//当前显示模式与亮度
	uint8_t BRTmode;			//亮度模式
	uint8_t BRTmax;				//亮度的最大值
	uint8_t BRTmin;				//亮度的最小值
	uint8_t BRTvals;			//亮度的值

	//屏幕状态，这里需要再修改
	uint8_t ScrStatus;			//原本是指屏幕开关状态这里改为保存3种数据状态屏开关，测试状态，模式
	uint8_t ScrSwitch;

	//开机时间
	uint8_t BOOTtime[24];		//开机时间

	//当前播放列表
	char 		CPlist[12];			//当前播放列表
	uint16_t 	listLen;

	//当前播放的内容的信息
	char 	 CPcontent[256];		//当前显示的内容
	uint8_t  CPfont;
	uint8_t  CPsize;
	uint32_t CPdelayTime;		//当前显示内容的停留时间,单位us
	uint32_t CPmoveSpeed;		//当前显示内容的进出速度
	uint16_t CPinform;			//当前显示内容的出字方式
	uint16_t CPcontentLen;		//当前显示内容的总长度
	uint32_t CPitemOder;		//当前显示内容在播放列表中的位置
	//当前播放的内容的一幕信息
	char itemcontent[512];

	//全屏背景色与字体颜色相同时为测试模式
	uint8_t TestMode;
	uint8_t TestFlag;

	/**********************************MODBUS 协议特有的字段****************************************/
	//最小通信间隔
	uint32_t IntervTime;
	uint8_t  VertConnect;

	//自动自检设定时间
	uint8_t SThour;
	uint8_t STmin;
	uint8_t STsec;
	uint8_t STunit;
	uint8_t STcycle;

	//设备自检
	uint8_t CheckSelf;	//启动自检标志，1表示启动，0表示复位
	uint8_t Dspunit;	//自检的显示单元编号

	uint16_t ALLScreen;
	uint8_t  Modgroup[200];


	uint8_t LBstate;	//是否使能光带
	uint8_t LBsement;	//光带段数
	

	//系统参数的配置

	//屏幕大小配置
	uint16_t SCRwidth;			//整屏宽度，单位像素
	uint16_t SCRheight;			//整屏高度，单位像素

	//模组大小配置
	uint16_t Modwidth;
	uint16_t Modheight;

	//箱体大小配置
	uint32_t BOXnum;				//箱体的个数
	uint32_t BOXwidth;			//箱体的宽度，单位像素
	uint32_t BOXheight;			//箱体的高度，单位像素


	//配置显示的横向与纵向的坐标偏移
	uint16_t Xoffset;
	uint16_t Yoffset;

	//配置屏幕是门架型的还是悬臂型的
	uint16_t scrType;

	//配置发送卡类型
	uint16_t cardType;

	//动态显示的移动速度(单位时间移动的像素点数)
	uint16_t mvspeed;

	//屏幕分辨率的配置
	uint32_t RESwidth;			//屏幕分辨率的宽度
	uint32_t RESheight;			//屏幕分辨率的高度
	uint32_t RESbits;			//屏幕分辨率的位数


	//网络端口的配置
	uint32_t NETprot;			//端口
	uint8_t NETip[24];			//ip
	uint8_t NETmask[24];		//掩码
	uint8_t NETgw[24];			//网关

	//串口配置
	COMserial_t COMx[COM_NUM];		//默认串口
	
	
//TX/RX显示参数 暂时使用数组存储，后面需要分类
	uint8_t prm_msg[256];
	uint32_t prm_len;
	
	//加入发送卡版本
	uint8_t TX_Mver;
	uint8_t TX_Sver;
	//加入工控机APP版本
	uint8_t APP_Mver;
	uint8_t APP_Sver;
	//加入工控机监控版本
	uint8_t MIR_Mver;
	uint8_t MIR_Sver;
	

	//加入TX和RX的升级模式(相当BootLoader)
	uint8_t TX_upgrade_mode;
	uint8_t RX_upgrade_mode; 

	//加入TX和RX的升级标志
	uint8_t TX_upgrade_flag;
	uint8_t RX_upgrade_flag;

	//加入TX和RX的启动地址（00,06,0C,12）
	uint8_t TX_boot_address;
	uint8_t RX_boot_address;


	//加入TX和RX的升级地址（00,06,0C,12）00是FAC（出厂）模式下的，尽量不要修改FAC的程序
	uint8_t TX_upgrade_address;
	uint8_t RX_upgrade_address;

    //协议
    uint8_t Pctl;		//一级协议
	uint8_t pctl;		//二级协议

	//最高亮度等级
	uint8_t MaxBright;
	uint8_t MinBright;
	
}DATAPool_t;


typedef struct _curplay
{
	uint8_t type;
	uint8_t staytime;
	uint8_t inform;
	uint8_t outform;
	uint32_t speed;
	
	//文字内容
	uint8_t fonttype;
	uint8_t fontsize;
	char	charstr[256];
	uint8_t strLen;

	//图片内容
	uint8_t maptype;
	char 	mapName[8];

	//动画
	char aniName[8];

	//光带内容
	char lbName[16];
}DSPContent_t;

extern DSPContent_t CURDSPContent;


int DP_GetSysDataAndStatus(uint8_t PID,uint8_t *status,uint8_t *vals);

int DP_TempHumSmogModuleParsing(uint8_t *DataFromTHS,uint8_t len);

void DP_SetCurPlayContent(char *Content,uint16_t len,uint8_t Font,uint8_t Size,uint16_t inform,uint32_t inSpeed,uint32_t stayTime,uint8_t Order);
int DP_GetCurPlayContent(uint8_t *Content,uint16_t *Len,uint8_t *Font,uint8_t *Size,uint8_t *Inform,uint32_t *inSpeed,uint32_t *stayTime,uint8_t *Order);

//获取item信息 add by mo 2020.10.13
void DP_SetCurPlayItemContent(char *Content,uint16_t len);
int DP_GetCurPlayItemContent(char *Content,uint16_t *len);

void DP_ClearCurPlayContent(void);
void DP_DATAPoolPrintf(void);

//新加
void DP_SetProcotol(uint8_t Flag);
void DP_Set_Procotol(uint8_t Flag);
int DP_GetProcotol(uint8_t *Flag);
int DP_Get_Procotol(uint8_t *Flag);
int DP_Get_APPVersion(uint8_t *Pdata);
int DP_Get_MonitorVersion(uint8_t *Pdata);
int DP_Get_TXVersion(uint8_t *Pdata);
int DP_Get_RXVersion(uint8_t *Pdata);
int DP_Set_APPVersion(void);
int DP_Set_MonitorVersion(void);
void DP_Set_TX_Mode(uint8_t state);
int DP_Get_TX_Mode(uint8_t *state);
void DP_Set_RX_Mode(uint8_t state);
int DP_Get_RX_Mode(uint8_t *state);
void DP_Set_TX_Flag(uint8_t state);
int DP_Get_TX_Flag(uint8_t *state);
void DP_Set_RX_Flag(uint8_t state);
int DP_Get_RX_Flag(uint8_t *state);
void DP_Set_TX_Boot_Address(uint8_t state);
int DP_Get_TX_Boot_Address(uint8_t *state);
void DP_Set_RX_Boot_Address(uint8_t state);
int DP_Get_RX_Boot_Address(uint8_t *state);
void DP_Set_TX_Upgrade_Address(uint8_t state);
int DP_Get_TX_Upgrade_Address(uint8_t *state);
void DP_Set_RX_Upgrade_Address(uint8_t state);
int DP_Get_RX_Upgrade_Address(uint8_t *state);
int DP_Set_Display_Parameter(uint8_t *data,int len);
int DP_Get_Display_Parameter(uint8_t *data,uint32_t *len);
int DP_RXTX_ConfigureParsing(uint8_t *Data,uint16_t Len);
int DP_RXcardDataParsing(uint8_t *DataFromTXcard,uint16_t Len);
int DP_PixelsDataParsing(uint8_t *DataFromTXcard,uint32_t Len);
int DP_GetRXCardData(uint8_t PID,uint8_t CardNum,uint8_t *vals);
int DP_GetPixelsData(uint8_t *PixelsData,uint32_t *Len);
int DP_GetPixelsStatus(uint8_t *Status,uint32_t *BadPixels);
int DP_GetPixelsMode(uint8_t *PixMode);
int DP_SetPixelsMode(uint8_t PixMode);

int DP_RXTX_Parametermsg(uint8_t *DataFromTXcard,uint16_t len);

void GetDSPContent(DSPContent_t *DSPContent);

void DP_SetScreenStatus(uint8_t Status);
int DP_GetScreenStatus(uint8_t *Status);



void DP_SetLSData(uint8_t LSdataA,uint8_t LSdataB);
void DP_GetLSData(uint8_t *LSdataA,uint8_t *LSdataB);

void DP_SetSerialArg(uint8_t comx,uint32_t bdrate,uint8_t databits,uint8_t stopbits,uint8_t fctrl,uint8_t parity);
int DP_GetSerialArg(uint8_t comx,uint32_t *bdrate,uint8_t *databits,uint8_t *stopbits,uint8_t *fctrl,uint8_t *parity);


void DP_SetNetArg(uint8_t *IP,uint32_t port,uint8_t *netmask,uint8_t *gateway);
int DP_GetNetArg(uint8_t *IP,uint32_t *port,uint8_t *netmask,uint8_t *gateway);

void DP_SetScrResolution(uint16_t res_x,uint16_t res_y,uint16_t res_bits);
int DP_GetScrResolution(uint16_t *res_x,uint16_t *res_y,uint16_t *res_bits);

void DP_SetScreenSize(uint32_t width,uint32_t height);
int DP_GetScreenSize(uint32_t *width,uint32_t *height);

void DP_SetBoxSize(uint16_t width,uint16_t height,uint32_t num);
int DP_GetBoxSize(uint16_t *width,uint16_t *height,uint32_t *num);

void DP_SetModSize(uint16_t Modwidth,uint16_t Modheight);
void DP_GetModSize(uint16_t *Modwidth,uint16_t *Modheight);


void DP_SetCurPlayList(uint8_t *Plist,uint8_t Len);
int DP_GetCurPlayList(uint8_t *Plist,uint8_t *Len);


void DP_SaveBrightVals(uint8_t Bright);
int DP_ReadBrightVals(uint8_t *Bright);
void DP_SetBrightMode(uint8_t Bmode);
int DP_GetBrightMode(uint8_t *Bmode);
void DP_SetBrightRange(uint8_t Bmax,uint8_t Bmin);
int DP_GetBrightRange(uint8_t *Bmax,uint8_t *Bmin);

void DP_SetBootupTime(uint8_t *time,uint8_t Len);
int DP_GetBootupTime(uint8_t *time,uint8_t *Len);


void DP_SetTestMode(uint8_t mode);
void DP_GetTestMode(uint8_t *mode);

void DP_GetTemp(uint16_t *temp);
void DP_SetTemp(uint16_t temp);
void DP_GetHum(uint16_t *Hum);
void DP_SetHum(uint16_t Hum);
void DP_GetYLight(uint8_t *status);
void DP_SetYLight(uint8_t status);


void DP_SetTestFlag(uint8_t TestFlag);
void DP_GetTestFlag(uint8_t *TestFlag);

void DP_SetIntervTime(uint32_t IntervTime);
void DP_GetIntervTime(uint32_t *IntervTime);

void DP_SetVertConnect(uint8_t VertConnect);
void DP_GetVertConnect(uint8_t *VertConnect);

void InitDSPContent(void);
int SetDSPContent(DSPContent_t *DSPContent);

void DP_SetAutoCheckTime(uint8_t SThour,uint8_t STmin,uint8_t STsec);
void DP_GetAutoCheckTime(uint8_t *SThour,uint8_t *STmin,uint8_t *STsec);
void DP_SetAutoCheckUnit(uint8_t STunit,uint8_t STcycle);
void DP_GetAutoCheckUnit(uint8_t *STunit,uint8_t *STcycle);

void DP_SetCheckSelf(uint8_t CheckSelf,uint8_t dspunit);
void DP_GetCheckSelf(uint8_t *CheckSelf,uint8_t *dspunit);
void DP_SetCheckSelfTrouble(uint16_t ALLScreen,uint8_t *Modgroup,uint8_t Modnum);
void DP_GetCheckSelfTrouble(uint16_t *ALLScreen,uint8_t *Modgroup,uint8_t Modnum);


void DP_SetLBandArg(uint8_t state,uint8_t sement);
void DP_GetLBandArg(uint8_t *state,uint8_t *sement);


int DP_SetOffset(uint16_t Xoffset,uint16_t Yoffset);
void DP_GetOffset(uint16_t *Xoffset,uint16_t *Yoffset);
void DP_SetScrType(uint16_t scrType);
void DP_GetScrType(uint16_t *scrType);
void DP_SetCardType(uint16_t cardType);
void DP_GetCardType(uint16_t *cardType);
void DP_GetMvspeed(uint16_t *speed);
void DP_SetMvspeed(uint16_t speed);

int DP_GetPixelsColor(uint8_t *PixColor);
int DP_Get_TXRXVersion(uint8_t *Pdata);

int DP_Get_ALLRXVersion(uint8_t *Pdata,uint8_t *num);

#endif


