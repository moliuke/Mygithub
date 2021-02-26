#ifndef __PPL_DISPLAY_H
#define __PPL_DISPLAY_H
#include <stdio.h>
#include "debug.h"
#include "common.h"
#include "config.h"

#define 	PPL_CLEAN_SCREEN			0
#define 	PPL_DIRECTLY				1
#define 	PPL_MOVE_UP					2
#define 	PPL_MOVE_DOWN				3
#define 	PPL_MOVE_LEFT				4
#define 	PPL_MOVE_RIGHT				5
#define 	PPL_WIND_HORIZONTAL			6
#define 	PPL_WIND_WERTICAL			7
#define 	PPL_SHUT_UPDOWN				8
#define 	PPL_SPREAD_UPDOWN			9
#define 	PPL_SHUT_LEFTWRIGHT			10
#define 	PPL_SPREAD_LETFWRIGHT		11
#define 	PPL_SHUT_CENTER				12
#define 	PPL_SPREAD_CENTER			13
#define 	PPL_DISPLAY_SIRCLE			25
#define 	PPL_RUNNING_HOST			30



int PPL_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire);
void PPL_DefaultLst(void);
#endif

