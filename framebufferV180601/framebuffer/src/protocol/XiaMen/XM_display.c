#include <stdio.h>
#include <stdbool.h>


#include "debug.h"
#include "config.h"
#include "common.h"
#include "display.h"
#include "Dev_tcpserver.h"
#include "XM_Lstparse.h"
#include "XM_display.h"
#include "../../include/content.h"
#include "../../Hardware/Data_pool.h"

int XM_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	switch(option)
	{
		case XM_CLEAN_SCREEN:
		case XM_DIRECTLY:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
		case XM_MOVE_UP:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_UP;
			break;
		case XM_MOVE_DOWN:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_DOWN;
			break;
		case XM_MOVE_LEFT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_LEFT;
			break;
		case XM_MOVE_RIGHT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_RIGHT;
			break;
		default:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
	}
	return 0;
}

void XM_DefaultLst(void)
{
	uint8_t Len;
	char PlayLstPath[64];
	DEBUG_PRINTF;
	memset(PlayLstPath,0,sizeof(PlayLstPath));
	sprintf(PlayLstPath,"%s/%s",list_dir_1,"play.lst");
	if(XM_PLst_parsing(&content,PlayLstPath) < 0)
	{
		debug_printf("XM_DSPDefaultLst: CAN NOT DECODE THE PLAY LST\n");
	}
	DEBUG_PRINTF;
	Len = strlen("play.lst");
	//DP_CurPlayList(OPS_MODE_SET,"play.lst",&Len);
	DP_SetCurPlayList("play.lst",Len);
	//PPL_SETCurDspLst("play.lst",8);
}



