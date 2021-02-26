#ifndef __DATA_POOL_H
#define __DATA_POOL_H

#include <stdio.h>
#include "../include/debug.h"
#include "../include/config.h"

extern uint8_t Lbrightflag;  //�����쳣��־λ��0Ϊ������1Ϊ�쳣

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
//���������հ汾ƫ�ƣ�ȥ�������غ�ͨ��
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
	uint8_t COMdatabits;		//����λ��
	uint8_t COMstopbits;		//ֹͣλ��
	uint8_t COMflowctl;			//������
	uint8_t COMparity;			//��żУ��
	uint32_t COMbaudrate;		//������
}COMserial_t;


typedef struct __RXcard
{
	uint8_t Temp;
	uint8_t Volt1;
	uint8_t Volt2;
	uint8_t Volt3;
	uint8_t Volt4;

	uint8_t Door;

//������տ��汾�� 2020.3.16	
	uint8_t Mver;
    uint8_t Sver;

#if 1
//�������ص��죬��ʱ����
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
	//�¶�
	uint8_t TMPstatus;			//���¶�״̬
	uint8_t TMPvals;			//���¶�ֵ
	
	//��Դ
	uint8_t PWstatus;			//��Դ��״̬
	uint8_t PWvals;				//��Դ��ֵ

	//�ſ���
	uint8_t Dstatus;
	uint8_t Dvals;

	//���ػ�����
	uint8_t PIXstatus;			//��Ļ���ص�״̬
	uint8_t PIXmode;            //���ؼ������ 00��ʾȡ��ǰ��ʾ����01��ʾ������Ϣ02��ʾ���տ�ԭ
	uint8_t *PIXdata;			//��������ֵ
	uint32_t PIXvals;			//��Ļ���ػ�����
	uint32_t PIXdataLen;		//���������ܳ�����λ:Byte
	uint8_t PIXcolor;           //��Ļ��ʾ��ɫ
	
	//����
	uint8_t LSstatus;			//������״̬
	uint8_t LSvals;				//������ֵ

	//������
	uint8_t THDstatus;			//��������״̬
	uint8_t THDvals;			//��������ֵ

	//RX������
	uint8_t RXnum;
	RXcard_t RXcard[24];
	uint8_t LSensA;
	uint8_t LSensB;

	//����ͨ��
	uint8_t CHNstatus;
	uint8_t CHNvals;
	

	//����
	uint8_t FANstatus;
	uint8_t FANvals;

	//����ֵ
	uint8_t SMKstatus;			//����״̬
	uint16_t SMKvals;			//����ֵ

	//�����¶�
	uint8_t Tempstatus;			//����״̬
	uint16_t Tempvals;			//����ֵ

	//ʪ��
	uint8_t HUMstatus;			//ʪ��״̬
	uint16_t HUMvals;			//ʪ��ֵ

	//������״̬
	uint8_t YLstatus;			
	uint8_t YLvals;			

	//������
	uint8_t CTRLstatus;			//��������״̬
	uint8_t CTRLvals;			//��������ֵ

	//�����������
	uint8_t SofStatus;
	uint8_t Sofvals;

	//Ӳ������
	uint8_t HardStatus;
	uint8_t Hardvals;

	//���ϵͳ����
	uint8_t SysStatus;
	uint8_t Sysvals;

	//ͨ�Ź���
	uint8_t CommuStatus;
	uint8_t Commuvals;

	//��ʾģ��
	uint8_t MODstatus;			//��ʾģ���״̬
	uint8_t MODvals;			//��ʾģ���ֵ���������

	//��ʾģ���Դ
	uint8_t MODPstatus;			//ģ���Դ��״̬
	uint8_t MODPvals;			//ģ���Դ��ֵ

	//��ǰ��ʾģʽ������
	uint8_t BRTmode;			//����ģʽ
	uint8_t BRTmax;				//���ȵ����ֵ
	uint8_t BRTmin;				//���ȵ���Сֵ
	uint8_t BRTvals;			//���ȵ�ֵ

	//��Ļ״̬��������Ҫ���޸�
	uint8_t ScrStatus;			//ԭ����ָ��Ļ����״̬�����Ϊ����3������״̬�����أ�����״̬��ģʽ
	uint8_t ScrSwitch;

	//����ʱ��
	uint8_t BOOTtime[24];		//����ʱ��

	//��ǰ�����б�
	char 		CPlist[12];			//��ǰ�����б�
	uint16_t 	listLen;

	//��ǰ���ŵ����ݵ���Ϣ
	char 	 CPcontent[256];		//��ǰ��ʾ������
	uint8_t  CPfont;
	uint8_t  CPsize;
	uint32_t CPdelayTime;		//��ǰ��ʾ���ݵ�ͣ��ʱ��,��λus
	uint32_t CPmoveSpeed;		//��ǰ��ʾ���ݵĽ����ٶ�
	uint16_t CPinform;			//��ǰ��ʾ���ݵĳ��ַ�ʽ
	uint16_t CPcontentLen;		//��ǰ��ʾ���ݵ��ܳ���
	uint32_t CPitemOder;		//��ǰ��ʾ�����ڲ����б��е�λ��
	//��ǰ���ŵ����ݵ�һĻ��Ϣ
	char itemcontent[512];

	//ȫ������ɫ��������ɫ��ͬʱΪ����ģʽ
	uint8_t TestMode;
	uint8_t TestFlag;

	/**********************************MODBUS Э�����е��ֶ�****************************************/
	//��Сͨ�ż��
	uint32_t IntervTime;
	uint8_t  VertConnect;

	//�Զ��Լ��趨ʱ��
	uint8_t SThour;
	uint8_t STmin;
	uint8_t STsec;
	uint8_t STunit;
	uint8_t STcycle;

	//�豸�Լ�
	uint8_t CheckSelf;	//�����Լ��־��1��ʾ������0��ʾ��λ
	uint8_t Dspunit;	//�Լ����ʾ��Ԫ���

	uint16_t ALLScreen;
	uint8_t  Modgroup[200];


	uint8_t LBstate;	//�Ƿ�ʹ�ܹ��
	uint8_t LBsement;	//�������
	

	//ϵͳ����������

	//��Ļ��С����
	uint16_t SCRwidth;			//������ȣ���λ����
	uint16_t SCRheight;			//�����߶ȣ���λ����

	//ģ���С����
	uint16_t Modwidth;
	uint16_t Modheight;

	//�����С����
	uint32_t BOXnum;				//����ĸ���
	uint32_t BOXwidth;			//����Ŀ�ȣ���λ����
	uint32_t BOXheight;			//����ĸ߶ȣ���λ����


	//������ʾ�ĺ��������������ƫ��
	uint16_t Xoffset;
	uint16_t Yoffset;

	//������Ļ���ż��͵Ļ��������͵�
	uint16_t scrType;

	//���÷��Ϳ�����
	uint16_t cardType;

	//��̬��ʾ���ƶ��ٶ�(��λʱ���ƶ������ص���)
	uint16_t mvspeed;

	//��Ļ�ֱ��ʵ�����
	uint32_t RESwidth;			//��Ļ�ֱ��ʵĿ��
	uint32_t RESheight;			//��Ļ�ֱ��ʵĸ߶�
	uint32_t RESbits;			//��Ļ�ֱ��ʵ�λ��


	//����˿ڵ�����
	uint32_t NETprot;			//�˿�
	uint8_t NETip[24];			//ip
	uint8_t NETmask[24];		//����
	uint8_t NETgw[24];			//����

	//��������
	COMserial_t COMx[COM_NUM];		//Ĭ�ϴ���
	
	
//TX/RX��ʾ���� ��ʱʹ������洢��������Ҫ����
	uint8_t prm_msg[256];
	uint32_t prm_len;
	
	//���뷢�Ϳ��汾
	uint8_t TX_Mver;
	uint8_t TX_Sver;
	//���빤�ػ�APP�汾
	uint8_t APP_Mver;
	uint8_t APP_Sver;
	//���빤�ػ���ذ汾
	uint8_t MIR_Mver;
	uint8_t MIR_Sver;
	

	//����TX��RX������ģʽ(�൱BootLoader)
	uint8_t TX_upgrade_mode;
	uint8_t RX_upgrade_mode; 

	//����TX��RX��������־
	uint8_t TX_upgrade_flag;
	uint8_t RX_upgrade_flag;

	//����TX��RX��������ַ��00,06,0C,12��
	uint8_t TX_boot_address;
	uint8_t RX_boot_address;


	//����TX��RX��������ַ��00,06,0C,12��00��FAC��������ģʽ�µģ�������Ҫ�޸�FAC�ĳ���
	uint8_t TX_upgrade_address;
	uint8_t RX_upgrade_address;

    //Э��
    uint8_t Pctl;		//һ��Э��
	uint8_t pctl;		//����Э��

	//������ȵȼ�
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
	
	//��������
	uint8_t fonttype;
	uint8_t fontsize;
	char	charstr[256];
	uint8_t strLen;

	//ͼƬ����
	uint8_t maptype;
	char 	mapName[8];

	//����
	char aniName[8];

	//�������
	char lbName[16];
}DSPContent_t;

extern DSPContent_t CURDSPContent;


int DP_GetSysDataAndStatus(uint8_t PID,uint8_t *status,uint8_t *vals);

int DP_TempHumSmogModuleParsing(uint8_t *DataFromTHS,uint8_t len);

void DP_SetCurPlayContent(char *Content,uint16_t len,uint8_t Font,uint8_t Size,uint16_t inform,uint32_t inSpeed,uint32_t stayTime,uint8_t Order);
int DP_GetCurPlayContent(uint8_t *Content,uint16_t *Len,uint8_t *Font,uint8_t *Size,uint8_t *Inform,uint32_t *inSpeed,uint32_t *stayTime,uint8_t *Order);

//��ȡitem��Ϣ add by mo 2020.10.13
void DP_SetCurPlayItemContent(char *Content,uint16_t len);
int DP_GetCurPlayItemContent(char *Content,uint16_t *len);

void DP_ClearCurPlayContent(void);
void DP_DATAPoolPrintf(void);

//�¼�
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


