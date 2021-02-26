#include <stdio.h>
#include <stdbool.h>


#include "debug.h"
#include "config.h"
#include "common.h"
#include "display.h"
#include "Dev_tcpserver.h"
#include "PPL_Lstparse.h"
#include "PPL_datapool.h"
#include "PPL_display.h"
#include "../../include/content.h"
#include "../../Hardware/Data_pool.h"

int PPL_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	switch(option)
	{
		case PPL_CLEAN_SCREEN:
		case PPL_DIRECTLY:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
		case PPL_MOVE_UP:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_UP;
			break;
		case PPL_MOVE_DOWN:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_DOWN;
			break;
		case PPL_MOVE_LEFT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_LEFT;
			break;
		case PPL_MOVE_RIGHT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_RIGHT;
			break;
		case PPL_WIND_HORIZONTAL:
		case PPL_WIND_WERTICAL:
		case PPL_SHUT_UPDOWN:
		case PPL_SPREAD_UPDOWN:
		case PPL_SHUT_LEFTWRIGHT:
		case PPL_SPREAD_LETFWRIGHT:
		case PPL_SHUT_CENTER:
		case PPL_SPREAD_CENTER:
		case PPL_DISPLAY_SIRCLE:
		case PPL_RUNNING_HOST:
			DEBUG_PRINTF;
			//exit(1);
			*type = EFFECT_RUNNINGHOST;
			*dire = DIRECTION_NO;
			break;
		default:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
	}
	return 0;
}

void PPL_DefaultLst(void)
{
	uint8_t Len;
	char PlayLstPath[64];
	DEBUG_PRINTF;
	memset(PlayLstPath,0,sizeof(PlayLstPath));
	sprintf(PlayLstPath,"%s/%s",list_dir_1,"play.lst");
	if(PPL_PLst_parsing(&content,PlayLstPath) < 0)
	{
		debug_printf("PPL_DSPDefaultLst: CAN NOT DECODE THE PLAY LST\n");
	}
	DEBUG_PRINTF;
	Len = strlen("play.lst");
	//DP_CurPlayList(OPS_MODE_SET,"play.lst",&Len);
	DP_SetCurPlayList("play.lst",Len);
	//PPL_SETCurDspLst("play.lst",8);
	DEBUG_PRINTF;
}



