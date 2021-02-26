#ifndef __ZC_DISPLAY_H
#define __ZC_DISPLAY_H
#include "config.h"


#define 	ZC_CLEAN_SCREEN			0
#define 	ZC_DIRECTLY				1
#define 	ZC_MOVE_UP				2
#define 	ZC_MOVE_DOWN			3
#define 	ZC_MOVE_LEFT			4
#define 	ZC_MOVE_RIGHT			5
#define 	ZC_WIND_HORIZONTAL		6
#define 	ZC_WIND_WERTICAL		7
#define 	ZC_SHUT_UPDOWN			8
#define 	ZC_SPREAD_UPDOWN		9
#define 	ZC_SHUT_LEFTWRIGHT		10
#define 	ZC_SPREAD_LETFWRIGHT	11
#define 	ZC_SHUT_CENTER			12
#define 	ZC_SPREAD_CENTER		13
#define 	ZC_MOSAIC_DOWN			14
#define 	ZC_MOSAIC_RIGHT			15
#define 	ZC_FADE_IN				16
#define 	ZC_FADE_OUT				17
#define 	ZC_GLINT_CHAR_AWAY		18
#define 	ZC_GLINT_CHAR_STAY		19
#define 	ZC_GLINT_AREA_RECOVER	20
#define 	ZC_GLINT_AREA_BLACK		21

int ZC_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire);
void ZC_DefaultLst(void);
int ZC_SetCutInDisplay(int CutInTime);

#endif

