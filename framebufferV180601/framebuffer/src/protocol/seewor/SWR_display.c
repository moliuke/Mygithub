#include "SWR_display.h"
#include "../../Hardware/Data_pool.h"
#include "SWR_charparse.h"


int SWR_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	if(option == SWR_RAND0 || option == SWR_RAND18)
	{
		srand( (unsigned)time( NULL ) ); 
		option = rand()%17+1;
	}
	
	switch(option)
	{
		case SWR_DIRECTLY:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
		case SWR_SPREAD_UP:
			*type = EFFECT_SPREAD;
			*dire = DIRECTION_UP;
			break;
		case SWR_SPREAD_DOWN:
			*type = EFFECT_SPREAD;
			*dire = DIRECTION_DOWN;
			break;
		case SWR_SPREAD_LEFT:
			*type = EFFECT_SPREAD;
			*dire = DIRECTION_LEFT;
			break;
		case SWR_SPREAD_WRIGHT:
			*type = EFFECT_SPREAD;
			*dire = DIRECTION_RIGHT;
			break;
		case SWR_MOVE_UP:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_UP;
			break;
		case SWR_MOVE_DOWN:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_DOWN;
			break;
		case SWR_MOVE_LEFT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_LEFT;
			break;
		case SWR_MOVE_WRIGHT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_RIGHT;
			break;

			
		case SWR_SPREAD_UPDOWN:
			*type = EFFECT_SPREAD;
			*dire = DIRECTION_UPDW;
			break;
		case SWR_SPREAD_LEFTRIGHT:
			*type = EFFECT_SPREAD;
			*dire = DIRECTION_LFRI;
			break;
		case SWR_WIND_HORIZONTAL:
			*type = EFFECT_WINSHADE;
			*dire = DIRECTION_LFRI;
			break;
		case SWR_WIND_WERTICAL:
			*type = EFFECT_WINSHADE;
			*dire = DIRECTION_UPDW;
			break;

			
		case SWR_SHUT_LEFTRIGHT:
			*type = EFFECT_STAGGER;
			*dire = DIRECTION_LFRI;
			break;
		case SWR_SHUT_UPDOWN:
			*type = EFFECT_STAGGER;
			*dire = DIRECTION_UPDW;
			break;
		case SWR_OPPOSITE_LEFTRIGHT:
			*type = EFFECT_OPPOSITE;
			*dire = DIRECTION_LFRI;
			break;
			
		case SWR_OPPOSITE_UPDOWN:
			*type = EFFECT_OPPOSITE;
			*dire = DIRECTION_UPDW;
			break;
		case SWR_RAND0:
		case SWR_RAND18:
			break;
		case SWR_RUNHOST_LEFT:
			*type = EFFECT_RUNNINGHOST;
			*dire = DIRECTION_LFRI;
			break;
		case SWR_RUNHOST_UP:
		default:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
	}
	
	return 0;
}


static int Get_EffectClass(uint8_t effect_number)
{
	int retvals = -1;

	debug_printf("effect_number = %d\n",effect_number);
	int i;
	
	if(effect_number == 0)
	{
		srand( (unsigned)time( NULL ) ); 
		effect_number = rand()%17+1;
		DEBUG_PRINTF;
	}

	if(effect_number == 20)
		retvals = EFFECT_RUNNINGHOST;
		
	if(effect_number == 1)
		retvals = EFFECT_DIRECT;
	if(effect_number == 2 || effect_number == 3  || effect_number == 4 ||
	   effect_number == 5 || effect_number == 11 || effect_number == 13)
	   	retvals = EFFECT_SPREAD;
	if(effect_number == 6 || effect_number == 7 || effect_number == 8 || effect_number == 9)
	  	retvals = EFFECT_MOVE;
	if(effect_number == 10 || effect_number == 12)
		retvals = EFFECT_STAGGER;
	if(effect_number == 14 || effect_number == 15)
		retvals = EFFECT_WINSHADE;
	if(effect_number == 16 || effect_number == 17)
		retvals = EFFECT_OPPOSITE;

	debug_printf("retvals = %d\n",retvals);
	return retvals;
}





void SWR_DefaultLst(void)
{
	uint8_t len;
	char list[12]; 
	char playlist[48];
	memset(list,0,sizeof(list));
	memset(playlist,0,sizeof(playlist));
	//DP_CurPlayList(OPS_MODE_GET,list,&len);
	DP_GetCurPlayList(list,&len);
	sprintf(playlist,"%s/%s",list_dir_1,list);
	debug_printf("#playlist = %s\n",playlist);

	 //DecodePlayList2(&content,playlist);
	SWR_Lstparsing(&content,playlist);
	 DEBUG_PRINTF;
	len = strlen(list);
	//DP_CurPlayList(OPS_MODE_SET,list,&len);
	DP_SetCurPlayList(list,len);
	DEBUG_PRINTF;
}

