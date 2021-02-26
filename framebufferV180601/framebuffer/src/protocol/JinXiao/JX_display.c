#include <stdio.h>
#include <stdbool.h>


#include "debug.h"
#include "config.h"
#include "common.h"
#include "display.h"
#include "Dev_tcpserver.h"
#include "JX_Lstparse.h"
#include "JX_display.h"
#include "../../include/content.h"
#include "../../Hardware/Data_pool.h"

int JX_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	switch(option)
	{
		case JX_CLEAN_SCREEN:

			
		case JX_DIRECTLY:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
		case JX_MOVE_UP:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_UP;
			break;
		case JX_MOVE_DOWN:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_DOWN;
			break;
		case JX_MOVE_LEFT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_LEFT;
			break;
		case JX_MOVE_RIGHT:
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

void JX_DefaultLst(void)
{
	uint8_t Len;
	char PlayLstPath[64];
	DEBUG_PRINTF;
	memset(PlayLstPath,0,sizeof(PlayLstPath));
	sprintf(PlayLstPath,"%s/%s",list_dir_1,"play.lst");
	if(JX_PLst_parsing(&content,PlayLstPath) < 0)
	{
		debug_printf("JX_DSPDefaultLst: CAN NOT DECODE THE PLAY LST\n");
	}
	DEBUG_PRINTF;
	Len = strlen("play.lst");
	DP_SetCurPlayList("play.lst",Len);
}



