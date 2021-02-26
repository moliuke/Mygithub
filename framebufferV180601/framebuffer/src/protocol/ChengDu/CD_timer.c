#include <string.h>
#include "config.h"
#include "CD_timer.h"
#include "CD_charparse.h"
#include "../../module/mtimer.h"
static pthread_once_t init_create = PTHREAD_ONCE_INIT; 
static int CDInterTimeCount = 0;
static stimer_t CDCommunitTimer;
static int Netexecption = 0;
static int ListFlag = EXCEPT_LIST;


#define CDEFLST			list_dir_1"/Def.lst"
#define LST_272_176		list_dir_1"/deflist/272_176.lst"	
#define LST_480_100		list_dir_1"/deflist/480_100.lst"	


void CD_SetDefList(uint32_t Swidth,uint32_t Sheight)
{
	char cmd[256];

	memset(cmd,0,sizeof(cmd));
	if(Swidth == 272 && access(LST_272_176,F_OK) == 0)
		sprintf(cmd,"cp %s %s",LST_272_176,CDEFLST);
	if(Swidth == 480 && access(LST_480_100,F_OK) == 0)
		sprintf(cmd,"cp %s %s",LST_480_100,CDEFLST); 

	system(cmd);
}


void CD_timerOutDefLst(void)
{
	//char Deflst[64];

	//memset(Deflst,0,sizeof(Deflst));
	
	CD_Lstparsing(&content,CDEFLST);
	return;
}

void CD_ClearTimer(void)
{
	DEBUG_PRINTF;
	//最小通信间隔时间清0
	CDInterTimeCount = 0;
	Netexecption = NET_NORMAL;//网络正常
	//printf("网络被设置为正常\n");
	//定时器清0
	mtimer_clear(12);

}	

static void *CD_CommunitTimerACK(void *arg)
{
	char list[64];
	uint8_t Len = 0;
	uint32_t IntervTime = 0;
	stimer_t *MdbsTimer = (stimer_t *)arg;
	
	//每10秒钟中断一次，一次计时10秒
	CDInterTimeCount += 10;
	//memset(Playlst,0,sizeof(Playlst));
	//DP_GetCurPlayList(Playlst,&Len);
	//printf("DP_GetCurPlayList is %s\n",Playlst);
	//每中断一次就要重载一次最小通信间隔时间，因为最小通信间隔时间随时都有可能被重置
	DP_GetIntervTime(&IntervTime);
	//MdbsTimer->ref_vals = IntervTime;
	//IntervTime = 20;

	//计时超过最小通信间隔时间或者超时10分钟
	//printf("InterTimeCount = %d,IntervTime = %d\n\n",CDInterTimeCount,IntervTime);
	if((IntervTime != 0 && CDInterTimeCount >= IntervTime))
	{
		DEBUG_PRINTF;
		Netexecption = NET_EXCEPT;//网络异常
		CDInterTimeCount = 0;
		MdbsTimer->counter = 0;
		//printf("网络异常\n");
	}


	if(Netexecption == NET_NORMAL && ListFlag == EXCEPT_LIST)
	{
		DEBUG_PRINTF;
		//printf("网络正常\n");
		ListFlag = NORMAL_LIST;
		memset(list,0,sizeof(list));
		sprintf(list,"%s/%s",list_dir_1,Playlst);
		//printf("list is %s\n",list);
		CD_Lstparsing(&content,list);
		DP_SetCurPlayList(Playlst,7);
	}

	if(Netexecption == NET_EXCEPT && ListFlag == NORMAL_LIST)
	{
		DEBUG_PRINTF;
		ListFlag = EXCEPT_LIST;
		//printf("displaying default png........................\n\n");
		CD_timerOutDefLst();
		debug_printf("==========================playlst = %s\n",Playlst);
		DP_SetCurPlayList("Def.lst",7);
	}


}


static void __CD_TimerInit(void)
{
	char AbleStatus[12];
	char Timeout[8];
	char TimeoutAction[4];

	CDCommunitTimer.ref_vals = 10;
	CDCommunitTimer.counter  = 0;
	CDCommunitTimer.id       = 12;
	CDCommunitTimer.function = CD_CommunitTimerACK;

	DEBUG_PRINTF;
	mtimer_register(&CDCommunitTimer);

}

void CD_timerInit(void)
{
	pthread_once(&init_create,__CD_TimerInit);
}

