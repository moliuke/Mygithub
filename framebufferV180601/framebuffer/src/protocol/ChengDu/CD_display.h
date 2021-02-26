#ifndef __CD_DISPLAY_H
#define __CD_DISPLAY_H
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "display.h"
#include "content.h"
#include "../../module/mtimer.h"
#include "../../Hardware/Data_pool.h"

#define 	CD_CLEAN_SCREEN			0
#define 	CD_DIRECTLY				1
#define 	CD_MOVE_UP				2
#define 	CD_MOVE_DOWN			3
#define 	CD_MOVE_LEFT			4
#define 	CD_MOVE_RIGHT			5
#define 	CD_WIND_HORIZONTAL		6
#define 	CD_WIND_WERTICAL		7
#define 	CD_SHUT_UPDOWN			8
#define 	CD_SPREAD_UPDOWN		9
#define 	CD_SHUT_LEFTWRIGHT		10
#define 	CD_SPREAD_LETFWRIGHT	11
#define 	CD_SHUT_CENTER			12
#define 	CD_SPREAD_CENTER		13
#define 	CD_MOSAIC_DOWN			14
#define 	CD_MOSAIC_RIGHT			15
#define 	CD_FADE_IN				16
#define 	CD_FADE_OUT				17
#define 	CD_GLINT_CHAR_AWAY		18
#define 	CD_GLINT_CHAR_STAY		19
#define 	CD_GLINT_AREA_RECOVER	20
#define 	CD_GLINT_AREA_BLACK		21

void CD_DefaultLst(void);
int CD_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire);


#endif

