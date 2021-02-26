#include <stdio.h>
#include <stdbool.h>


#include "debug.h"
#include "config.h"
#include "common.h"
#include "display.h"
#include "Dev_tcpserver.h"
#include "ZH_charparse.h"
#include "ZH_display.h"
#include "../../include/content.h"
#include "../../Hardware/Data_pool.h"

int ZH_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	switch(option)
	{
		case ZH_CLEAN_SCREEN:

			
		case ZH_DIRECTLY:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
		default:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
	}
	return 0;
}

void ZH_DefaultLst(void)
{
	uint8_t Len;
	char PlayLstPath[64];
	DEBUG_PRINTF;
	memset(PlayLstPath,0,sizeof(PlayLstPath));
	sprintf(PlayLstPath,"%s/%s",list_dir_1,"play.lst");
	if(ZH_PLst_parsing(&content,PlayLstPath) < 0)
	{
		debug_printf("ZH_DSPDefaultLst: CAN NOT DECODE THE PLAY LST\n");
	}
	DEBUG_PRINTF;
	Len = strlen("play.lst");
	//DP_CurPlayList(OPS_MODE_SET,"play.lst",&Len);
	DP_SetCurPlayList("play.lst",Len);
	//PPL_SETCurDspLst("play.lst",8);
}



