#ifndef  __HW3G_RXTX_H
#define  __HW3G_RXTX_H

#include "config.h"


typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;



#define	DEV_OPS_OPENCOLSE		0x01
#define DEV_OPS_SETTIME			0x02
#define DEV_OPS_SYSRESET		0x03
#define DEV_OPS_SETBRIGHT		0x04
#define DEV_OPS_SETPOWERMODE	0x05
#define DEV_OPS_PLAYLIST		0x06
#define DEV_OPS_READDATA		0x07





typedef struct                    // 单个箱体参数
{
	BYTE nType[8];
	BYTE nData[8];
	BYTE nTemperature;
	BYTE nVoltage[5];
	BYTE bDoorSwitch;
	BYTE bDriver[8];
	BYTE bDriverEx;
	BYTE bSystem;
}XKBoxStatus;




typedef struct                 //   参数范围
{
   unsigned int  Type;
   unsigned int  LowValue;
   unsigned int  HighValue;
   unsigned int  LowAlarm;
   unsigned int  HighAlarm;
}XKValueRange_t;







typedef struct 
{
	BYTE			box_width;
	BYTE			box_height;
	BYTE			box_ndata[8];
	BYTE 			box_channel;
	BYTE 			box_system;
	BYTE 			box_Driver[8];
}boxsmsg_t;

typedef struct
{
	BYTE			BOX_number;
	BYTE  			BOX_bright[2];
	BYTE			BOX_temper;
	BYTE 			BOX_powerVolt;
	BYTE 			BOX_oboVolt;
	boxsmsg_t 		BOX_boxmsg[15];
	
}BOXsmsg_t;

typedef struct 
{
	BYTE			dev_time_year;
	BYTE			dev_time_month;
	BYTE			dev_time_day;
	BYTE			dev_time_hour;
	BYTE			dev_time_min;
	BYTE			dev_time_sec;
}devtime_t;

typedef struct 
{
	BYTE 			status_power;		/*the power status*/
	BYTE 			status_fan;			/*the fan status*/
	BYTE 			status_door;		/*the door status ,open or close*/
	BYTE 			status_system;		/*the system status*/
	BYTE 			status_driver;		/*the driver status*/
	BYTE 			status_pixel;		/*the pixels status*/
	BYTE 			status_lightst;		/*the light sensitive status*/
	BYTE 			status_thunder;		/*the anti-thander status*/
}devstatus_t;

typedef struct 
{
	WORD			scr_width;
	BYTE			scr_height;
	BYTE			src_content;
	BYTE			scr_list[8];
	BYTE			*scr_pixels;
	BYTE			*src_curplaying;
	BYTE			BrightMode;
	BYTE			RedBright;
	BYTE			GreenBright;
	BYTE			BlueBright;
	unsigned long	lBadPointCount;
}screenmsg_t;


typedef struct 
{
	char 			*DEV_name;
	unsigned int    DEV_ID;
	devstatus_t		DEV_stamsg;
	devtime_t		DEV_timmsg;
	BOXsmsg_t		DEV_boxmsg;
	screenmsg_t		DEV_scrmsg;
}DEVStatusmsg_t;

extern DEVStatusmsg_t DEV_statusmsg;

#define PID_PIXELS_STATUS		0x00
#define PID_OTHER_STATUS		0x01
#define PID_CUR_PLAYLIST		0x02
#define PID_LED_BRIGHT			0x03
#define PID_DEV_ID				0x04
#define PID_BOOTUP_TIME			0x05
#define PID_DEV_SIZE			0x06
#define PID_BOX_SIZE			0x07
#define PID_CUR_STRING			0x08
#define PID_TEMP_AND_HUM		0x09
#define PID_SMOKE				0x0a
#define PID_ENV_BRIGHT			0x0b


#define UNION_X0	0
#define UNION_X1	1
#define UNION_X2	2
#define UNION_X3	3
#define UNION_X4	4
#define UNION_X5	5
#define UNION_X6	6
#define UNION_X7	7
#define UNION_X8	8
#define UNION_X9	9
#define UNION_X10	10
#define UNION_X11	11
#define UNION_X12	12
#define UNION_X13	13
#define UNION_X14	14
#define UNION_X15	15
#define UNION_X16	16
#define UNION_X17	17
#define UNION_X18	18
#define UNION_X19	19
#define UNION_X20	20
#define UNION_X		40	


#define FLAG_TEMP 	0
#define FLAG_VOL1	1
#define FLAG_VOL2	2
#define FLAG_VOL3	3
#define FLAG_VOL4	4
#define FLAG_DOOR	6
#define FLAG_PBAD	8
#define FLAG_PBAD1	8
#define FLAG_PBAD2	9
#define FLAG_CBAD	10

#define FLAG_LIGHT		0
#define FLAG_LIGHT1		0
#define FLAG_LIGHT2		1
#define FLAG_THANDER	2

typedef struct 
{
	uint16_t 	PID;
	uint16_t 	len;
	union
	{
		uint8_t 	*byte;
		uint32_t 	word;
	}vals;
}Param_data_t;

extern Param_data_t param_data_pool[30];


//int set_ledBright(uint8_t R_bright,uint8_t G_bright,uint8_t B_bright);





#if 0

#define MODULE_ON	0x01
#define MODULE_OFF	0x00


int set_powerMode(uint8_t powerMode);


//int dev_dataprocessor(unsigned char *data,unsigned int len);
int Set_LEDBright(uint8_t Bright);
int RXTX_SetScreenStatus(uint8_t Status);
uint8_t MapBrightData(uint8_t Bright,uint8_t level);

void RxCardReset(void);
void TxCardReset(void);
void ModulePower(uint8_t state);

#else
int Set_LEDBright(uint8_t Bright);
int RXTX_SetScreenStatus(uint8_t Status);
void Set_ParameterState(uint8_t state);
void Set_DisplayParameter(uint8_t *data, uint32_t *len);
void Get_DisplayParameter();
void Set_UpgradeParameter(uint8_t *data, uint32_t *len);
void Get_UpgradeParameter();
void Get_UpdateDate(void);
void Set_TestState(uint8_t state);
int UpgradeFile_TX_2K(char *filepath,int flag);
void RxCardReset(uint8_t *data);
void TxCardReset(void);
void Set_Pixels_Mode(uint8_t pixmode);

void SetAndGetcmd(uint8_t *data,uint8_t len);

#endif

#endif



