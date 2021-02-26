#include "CD_display.h"
#include "CD_charparse.h"
char Playlst[8];

void CD_DefaultLst(void)
{
#if 0
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
		CD_Lstparsing(&content,playlist);
		len = strlen(list); 
#else	
		uint8_t len;
		memset(Playlst,0,sizeof(Playlst));
		DP_GetCurPlayList(Playlst,&len);
		//printf("Playlst is %s\n",Playlst);
		DP_SetCurPlayList("Def.lst",7);
		CD_Lstparsing(&content,"/home/LEDscr/list/Def.lst");
	
#endif

}



int CD_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	switch(option)
	{
		case CD_MOVE_UP:
			//*type = EFFECT_MOVE;
			//*dire = DIRECTION_UP;
			//break;
		case CD_MOVE_DOWN:
			//*type = EFFECT_MOVE;
			//*dire = DIRECTION_DOWN;
			//break;
		case CD_MOVE_LEFT:
			//*type = EFFECT_MOVE;
			//*dire = DIRECTION_LEFT;
			//break;
		case CD_MOVE_RIGHT:
			//*type = EFFECT_MOVE;
			//*dire = DIRECTION_RIGHT;
			//break;
		case CD_WIND_HORIZONTAL:
			//*type = EFFECT_WINSHADE;
			//*dire = DIRECTION_LFRI;
			//break;
		case CD_WIND_WERTICAL:
			//*type = EFFECT_WINSHADE;
			//*dire = DIRECTION_UPDW;
			//break;
		case CD_DIRECTLY:
			//*type = EFFECT_DIRECT;
			//*dire = DIRECTION_NO;
			//break;
		case CD_SPREAD_LETFWRIGHT:
			//*type = EFFECT_SPREAD;
			//*dire = DIRECTION_LFRI;
			//break;
		case CD_SPREAD_UPDOWN:
			//*type = EFFECT_SPREAD;
			//*dire = DIRECTION_UPDW;
			//break;
		case CD_SPREAD_CENTER:
			//*type = EFFECT_SPREAD;
			//*dire = DIRECTION_CENTER;
			//break;


			
		case CD_CLEAN_SCREEN:
		case CD_SHUT_UPDOWN:
		case CD_SHUT_LEFTWRIGHT:
		case CD_SHUT_CENTER:
		case CD_MOSAIC_DOWN:
		case CD_MOSAIC_RIGHT:
		case CD_FADE_IN:
		case CD_FADE_OUT:
		case CD_GLINT_CHAR_AWAY:
		case CD_GLINT_CHAR_STAY:
		case CD_GLINT_AREA_RECOVER:
		case CD_GLINT_AREA_BLACK:
		default:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
	}
	
	return 0;
}


