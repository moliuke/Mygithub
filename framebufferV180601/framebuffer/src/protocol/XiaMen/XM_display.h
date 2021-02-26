#ifndef __XM_DISPLAY_H
#define __XM_DISPLAY_H
#include <stdio.h>
#include "debug.h"
#include "common.h"
#include "config.h"

#define 	XM_CLEAN_SCREEN			0
#define 	XM_DIRECTLY				1
#define 	XM_MOVE_UP					2
#define 	XM_MOVE_DOWN				3
#define 	XM_MOVE_LEFT				4
#define 	XM_MOVE_RIGHT				5
#define 	XM_WIND_HORIZONTAL			6
#define 	XM_WIND_WERTICAL			7
#define 	XM_SHUT_UPDOWN				8
#define 	XM_SPREAD_UPDOWN			9
#define 	XM_SHUT_LEFTWRIGHT			10
#define 	XM_SPREAD_LETFWRIGHT		11
#define 	XM_SHUT_CENTER				12
#define 	XM_SPREAD_CENTER			13
#define 	XM_DISPLAY_SIRCLE			25
#define 	XM_RUNNING_HOST			30



int XM_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire);
void XM_DefaultLst(void);
#endif

