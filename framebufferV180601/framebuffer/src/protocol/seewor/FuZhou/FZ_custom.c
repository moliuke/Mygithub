#include <stdio.h>
#include "conf.h"
#include "../SWR_init.h"
#include "../SWR_protocol.h"
#include "../../../module/mtimer.h"
#include "Dev_serial.h"
#include "../../../update.h"
#include "FZ_custom.h"
#include "mtime.h"
#include "../../../Hardware/Data_pool.h"

#define HOUR24		86400	//一天86400s
#define INTERVAL	600	//10分钟


#define CMD_GET_LEDSWITCH_TIME		 0x3437		/*获取定时开关屏时间*/
#define CMD_SET_LEDSWITCH_TIME		 0x3436		/*设置定时开关屏时间*/
#define CMD_GET_TURNON_LIGHT_PARAM	 0x3439		/*获取开关灯参数*/
#define CMD_SET_TURNON_LIGHT_PARAM	 0x3438		/*设置开关灯参数*/
#define CMD_SET_PARKING_CODE		 0x3532		/*设置车位码*/
#define CMD_SET_LIGHT_BAND			 0x3531		/*设置光带*/


#define TIME_CONFIG		conf_dir"/timing.conf"

static uint8_t LED_close_time[9] = {0x00,0x00,0x3a,0x00,0x00,0x3a,0x00,0x00};
static uint8_t LED_open_time[9] = {0x00,0x00,0x3a,0x00,0x00,0x3a,0x00,0x00};

static uint8_t LEDstatus = SLED_ON;

static int DopenTime = 0,DcloseTime = 0;

//按照顺时针顺序
//关屏时间大于开屏时间
#define CLOSE_MAX_OPEN	1
//关屏时间小于开屏时间
#define CLOSE_MIN_OPEN	0
static int open_close_dir = CLOSE_MAX_OPEN;



/*//////////////////////////////////////////////////////////////////////////
下面的接口对应的命令分别是0x3436,0x3437,0x3438,0x3439,0x3531,0x3532这几条命令显科协议是没有的，所以通过
显科协议预留的扩展接口对接到显科协议中
///////////////////////////////////////////////////////////////////////////*/
static inline void SetSwitchTime(uint8_t *time)
{
	LED_open_time[0] = time[0];
	LED_open_time[1] = time[1];
	LED_open_time[3] = time[2];
	LED_open_time[4] = time[3];
	LED_open_time[6] = time[4];
	LED_open_time[7] = time[5];
	LED_open_time[8] = '\0';
	debug_printf("LED_open_time = %s\n",LED_open_time);
	
	LED_close_time[0] = time[6];
	LED_close_time[1] = time[7];
	LED_close_time[3] = time[8];
	LED_close_time[4] = time[9];
	LED_close_time[6] = time[10];
	LED_close_time[7] = time[11];
	LED_close_time[8] = '\0';
	debug_printf("LED_close_time = %s\n",LED_close_time);

	DcloseTime = atoi(LED_close_time) * 3600 + atoi(LED_close_time + 3) * 60 + atoi(LED_close_time + 6);
	DopenTime = atoi(LED_open_time) * 3600 + atoi(LED_open_time + 3) * 60 + atoi(LED_open_time + 6);


	//确定在顺时针方向上开屏时间与关屏时间哪个大,并且要求两者之间的距离必须有10分钟的距离
	if(DcloseTime > DopenTime)
	{
		open_close_dir = CLOSE_MAX_OPEN;
		if(DopenTime + INTERVAL > DcloseTime)
			DcloseTime = DopenTime + INTERVAL;
	}
	else
	{
		open_close_dir = CLOSE_MIN_OPEN;
		if(DcloseTime + INTERVAL > DopenTime)
			DopenTime = DcloseTime + INTERVAL;
	}
	debug_printf("DopenTime = %d,DcloseTime = %d\n",DopenTime,DcloseTime);
}

static int SetLEDswitchTime(Protocl_t *protocol,unsigned int *len)
{
	int i = 0;
	FILE *fp = NULL;
	uint8_t *data = protocol->protcmsg.data;
	for(i = 0 ; i < 12 ; i ++)
		debug_printf("0x%x ",data[i]);
	debug_printf("\n");
	//memcpy(LED_close_time,data,12);

	SetSwitchTime(data);

	fp = fopen(TIME_CONFIG,"wb+");
	if(fp == NULL)
		goto ERRORDEAL;
	
	fwrite(data,1,12,fp);
	fflush(fp);
	
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;

	fclose(fp);
	
	return 0;

	ERRORDEAL:
		fclose(fp);
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
		*len = 1;
		
}

static int GetLEDswitchTime(Protocl_t *protocol,unsigned int *len)
{
	uint8_t *data = protocol->protcmsg.data;
	
	data[0] = 0x01;
	data[1] = LED_open_time[0];
	data[2] = LED_open_time[1];
	data[3] = LED_open_time[3];
	data[4] = LED_open_time[4];
	data[5] = LED_open_time[6];
	data[6] = LED_open_time[7];
	data[7] = LED_close_time[0];
	data[8] = LED_close_time[1];
	data[9] = LED_close_time[3];
	data[10] = LED_close_time[4];
	data[11] = LED_close_time[6];
	data[12] = LED_close_time[7];

	protocol->protcmsg.length = 13;
	*len = 13;
	debug_printf("GetLEDswitchTime\n");
	return 0;
}




//以下三个接口目前没有这个功能，往里面填数据而已
static int SetLightParam(Protocl_t *protocol,unsigned int *len)
{

	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	return 0;
	
	return 0;
}

static int GetLightParam(Protocl_t *protocol,unsigned int *len)
{
	uint8_t i = 0;
	uint8_t *data = protocol->protcmsg.data;
	data[0] = 0x01;
	data[1] = 0x31;
	data[2] = 0x38;
	data[3] = 0x30;
	data[4] = 0x30;
	data[5] = 0x30;
	data[6] = 0x30;
	data[7] = 0x32;
	data[8] = 0x32;
	data[9] = 0x30;
	data[10] = 0x30;
	data[11] = 0x30;
	data[12] = 0x30;
	protocol->protcmsg.length = 13;
	*len = 13;
}


static int SetParkingCode(Protocl_t *protocol,unsigned int *len)
{
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	return 0;
}

static int SetLightBandParam(Protocl_t *protocol,unsigned int *len)
{
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	return 0;
}


int FZ_extendcmd(Protocl_t *protocol,unsigned int *len)
{
	switch(protocol->protcmsg.head.cmdID)
	{
		case CMD_GET_LEDSWITCH_TIME:
			GetLEDswitchTime(protocol,len);
			break;
		case CMD_SET_LEDSWITCH_TIME:
			SetLEDswitchTime(protocol,len);
			break;
		case CMD_SET_TURNON_LIGHT_PARAM:
			SetLightParam(protocol,len);
			break;
		case CMD_GET_TURNON_LIGHT_PARAM:
			DEBUG_PRINTF;
			GetLightParam(protocol,len);
			break;
		case CMD_SET_PARKING_CODE:
			SetParkingCode(protocol,len);
			break;
		case CMD_SET_LIGHT_BAND:
			SetLightBandParam(protocol,len);
			break;
		default:
			return -1;
	}
}



/*///////////////////////////////////////////////////////
系统启动后初始化定时开关屏时间
//////////////////////////////////////////////////////////*/
int GetTimingConfig(void)
{
	FILE *fp = NULL;
	char timeconfig[13];
	
	fp = fopen(TIME_CONFIG,"r+");
	if(fp == NULL)
		return -1;

	memset(timeconfig,0,sizeof(timeconfig));
	fread(timeconfig,1,12,fp);

	debug_printf("timeconfig = %s\n",timeconfig);
	fclose(fp);
	
	SetSwitchTime(timeconfig);
	
	return 0;
}



/*/////////////////////////////////////////////////
开启一个独立的定时器，当开屏时间到则开屏，关屏时间到则关屏
//////////////////////////////////////////////////////*/
static stimer_t FZTimer;
#define FZTIMER_ID		12
#define COUNTERTIME		300		//5分钟定时器中断一次


static int CloseMaxOpen(int sysTime)
{
	DEBUG_PRINTF;
	int SYStimes = 0;
	debug_printf("sysTime = %d,DopenTime = %d,LEDstatus = %d\n",sysTime,DopenTime,LEDstatus);
	//每次开关灯操作之前都要检查一下当前的屏幕是开的还是关的。
	DP_GetScreenStatus(&LEDstatus);
	//在开屏时间内
	if(sysTime >= DopenTime && sysTime <= DcloseTime)
	{
		//系统时间再开屏时间后的10分钟内
		if(sysTime >= DopenTime && sysTime <= DopenTime + INTERVAL)
		{
			//屏幕原本是关着的就要打开
			if(LEDstatus == SLED_OFF)
			{
				SET_LED_STATE(SLED_ON);
				LEDstateRecord(SLED_ON);
				LEDstatus = SLED_ON;
				return 0;
			}
		}
	}
	//关屏时间，同理开屏
	else
	{
		SYStimes = (sysTime > DcloseTime) ? sysTime : (sysTime % HOUR24 + HOUR24);
		if(SYStimes >= DcloseTime && SYStimes <= DcloseTime + INTERVAL)
		{
			if(LEDstatus == SLED_ON)
			{
				SET_LED_STATE(SLED_OFF);
				LEDstateRecord(SLED_OFF);
				LEDstatus = SLED_OFF;
				return 0;
			}
		}
	}
	return -1;
}


static int CloseMinOpen(int sysTime)
{
	int SYStimes = 0;
	//每次开关灯操作之前都要检查一下当前的屏幕是开的还是关的。
	DP_GetScreenStatus(&LEDstatus);
	if(sysTime >= DcloseTime && sysTime <= DopenTime)
	{
		if(sysTime >= DcloseTime && sysTime <= DcloseTime + INTERVAL)
		{
			if(LEDstatus == SLED_ON)
			{
				DEBUG_PRINTF;
				DEBUG_PRINTF_ToFile(__func__,__LINE__);
				SET_LED_STATE(SLED_OFF);
				LEDstateRecord(SLED_OFF);
				LEDstatus = SLED_OFF;
				DEBUG_PRINTF;
				return 0;
			}
		}
	}
	else
	{
		SYStimes = (sysTime > DopenTime) ? sysTime : (sysTime % HOUR24 + HOUR24);
		if(SYStimes >= DopenTime && SYStimes <= DopenTime + INTERVAL)
		{
			if(LEDstatus == SLED_OFF)
			{
				DEBUG_PRINTF;
				SET_LED_STATE(SLED_ON);
				LEDstateRecord(SLED_ON);
				LEDstatus = SLED_ON;
				DEBUG_PRINTF;
				return 0;
			}
		}
	}
}


static void *FZTimer_action(void *arg)
{
	uint8_t Len;
	uint8_t timerstr[24];
	static int DsysTime = 0;
	get_sys_time(timerstr,&Len);

	DsysTime = atoi(timerstr + 11) * 3600 + atoi(timerstr + 14) * 60 + atoi(timerstr + 17);

	DEBUG_PRINTF;

	//顺时针方向:关屏时间>开屏时间
	if(open_close_dir == CLOSE_MAX_OPEN)
	{
		CloseMaxOpen(DsysTime);
		DEBUG_PRINTF;
	}
	//顺时针方向:开屏时间>关屏时间
	else
	{
		DEBUG_PRINTF;
		CloseMinOpen(DsysTime);
	}

}
void FZTimer_init(void)
{
	FZTimer.counter = 0;
	FZTimer.ref_vals = COUNTERTIME;
	FZTimer.id = FZTIMER_ID;
	FZTimer.function = FZTimer_action;
	
	pthread_mutex_lock(&timerlist_mutex); 
	mtimer_register(&FZTimer);
	pthread_mutex_unlock(&timerlist_mutex); 
}


char *FuZhouProject(void)
{
	return "FuZhou";
}


#if 0

/*//////////////////////////////////////////////////////////////////////////
协议相关的接口的初始化，
///////////////////////////////////////////////////////////////////////////*/
int _ProtocolRelation(void *arg1,void *arg2)
{
	projectstr = FuZhouProject;
	//将定时开关屏的接口对接到显科协议的扩展接口上
	PROTOCOLStruct.extendcmd = FZ_extendcmd;

	//开机获取设备的定时开关平时间
	GetTimingConfig();

	//初始化定时器
	FZTimer_init();
}

void _ProtocolRelationDestroy(void)
{
	
}

#endif