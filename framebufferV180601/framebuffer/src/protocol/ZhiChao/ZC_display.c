#include "ZC_display.h"
#include <stdio.h>
#include "ZC_Lstparse.h"
#include "debug.h"
#include "display.h"
#include "content.h"
#include "../../module/mtimer.h"
#include "../../Hardware/Data_pool.h"



int ZC_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	switch(option)
	{
		case ZC_MOVE_UP:
			//*type = EFFECT_MOVE;
			//*dire = DIRECTION_UP;
			//break;
		case ZC_MOVE_DOWN:
			//*type = EFFECT_MOVE;
			//*dire = DIRECTION_DOWN;
			//break;
		case ZC_MOVE_LEFT:
			//*type = EFFECT_MOVE;
			//*dire = DIRECTION_LEFT;
			//break;
		case ZC_MOVE_RIGHT:
			//*type = EFFECT_MOVE;
			//*dire = DIRECTION_RIGHT;
			//break;
		case ZC_WIND_HORIZONTAL:
			//*type = EFFECT_WINSHADE;
			//*dire = DIRECTION_LFRI;
			//break;
		case ZC_WIND_WERTICAL:
			//*type = EFFECT_WINSHADE;
			//*dire = DIRECTION_UPDW;
			//break;
		case ZC_DIRECTLY:
			//*type = EFFECT_DIRECT;
			//*dire = DIRECTION_NO;
			//break;
		case ZC_SPREAD_LETFWRIGHT:
			//*type = EFFECT_SPREAD;
			//*dire = DIRECTION_LFRI;
			//break;
		case ZC_SPREAD_UPDOWN:
			//*type = EFFECT_SPREAD;
			//*dire = DIRECTION_UPDW;
			//break;
		case ZC_SPREAD_CENTER:
			//*type = EFFECT_SPREAD;
			//*dire = DIRECTION_CENTER;
			//break;


			
		case ZC_CLEAN_SCREEN:
		case ZC_SHUT_UPDOWN:
		case ZC_SHUT_LEFTWRIGHT:
		case ZC_SHUT_CENTER:
		case ZC_MOSAIC_DOWN:
		case ZC_MOSAIC_RIGHT:
		case ZC_FADE_IN:
		case ZC_FADE_OUT:
		case ZC_GLINT_CHAR_AWAY:
		case ZC_GLINT_CHAR_STAY:
		case ZC_GLINT_AREA_RECOVER:
		case ZC_GLINT_AREA_BLACK:
		default:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
	}
	
	return 0;
}


void ZC_DefaultLst(void)
{
	uint8_t len;
	char list[12]; 
	char playlist[48];
	
	memset(list,0,sizeof(list));
	memset(playlist,0,sizeof(playlist));
	//DP_CurPlayList(OPS_MODE_GET,list,&len);
	DP_GetCurPlayList(list,&len);
	
	sprintf(playlist,"%s/%s",list_dir_1,list);
	debug_printf("playlist = %s\n",playlist);

	DEBUG_PRINTF;
	//DecodePlayList2(&content,playlist);
	ZC_Lstparsing(&content,playlist);
	len = strlen(list);	
	DEBUG_PRINTF;
}








stimer_t ZCTimer; 
uint8_t list[8];
uint8_t CurPlst[48];
uint8_t Len;

#define 	ZC_TIMER_ID		12

static void *ZC_TimerAction(void *arg)
{
	//mtimer_unregister(ZC_TIMER_ID);
	stimer_t *ZCTimer = (stimer_t *)arg;
	debug_printf("time out,ZCTimer->id = %d,ZCTimer->ref_vals = %d,ZCTimer->counter = %d\n",ZCTimer->id,ZCTimer->ref_vals,ZCTimer->counter);
	DEBUG_PRINTF;
	ZC_Lstparsing(&content,CurPlst);

	Len = strlen(list);
	//DP_CurPlayList(OPS_MODE_SET,list,&Len);
	DP_SetCurPlayList(list,Len);
	DEBUG_PRINTF;
	ZCTimer->logout = 1;
}



//定时器的初始化
static int ZC_TimerInit(int stayTime)
{
	stimer_t *arg = (stimer_t *)malloc(sizeof(stimer_t));
	memset(arg,0,sizeof(stimer_t));
	memcpy(arg,&ZCTimer,sizeof(ZCTimer));
	debug_printf("start time : %d\n",stayTime);
	ZCTimer.id = ZC_TIMER_ID;
	ZCTimer.counter = 0;
	ZCTimer.ref_vals = stayTime;
	ZCTimer.function	= ZC_TimerAction;
	//ZCTimer.arg = (void *)&arg;
	ZCTimer.logout = 0;		//值为1时定时器将被注销掉
	mtimer_register(&ZCTimer);
	return 0;
}


int ZC_SetCutInDisplay(int CutInTime)
{
	//uint8_t list[8];
	//uint8_t Len;

	//读取当前显示的播放列表
	memset(list,0,sizeof(list));
	memset(CurPlst,0,sizeof(CurPlst));
	//DP_CurPlayList(OPS_MODE_GET,list,&Len);
	DP_GetCurPlayList(list,&Len);

	//保存播放列表到全局变量中，由定时器时间到调用
	sprintf(CurPlst,"%s/%s",list_dir_1,list);
	debug_printf("time out will play list : %s\n",CurPlst);

	//定时CutInTime秒后重新播放上一个播放列表
	ZC_TimerInit(CutInTime);
	return 0;
}

