#ifndef __SWR_PRIVATEINTERF_H
#define __SWR_PRIVATEINTERF_H
#include <stdio.h>
#include "debug.h"
#include "config.h"
#include "display.h"
#include "content.h"


#define SWR_RAND0				0
#define SWR_DIRECTLY			1
#define SWR_SPREAD_UP			2
#define SWR_SPREAD_DOWN			3
#define SWR_SPREAD_LEFT			4
#define SWR_SPREAD_WRIGHT		5
#define SWR_MOVE_UP				6
#define SWR_MOVE_DOWN			7
#define SWR_MOVE_LEFT			8
#define SWR_MOVE_WRIGHT			9
#define SWR_SHUT_UPDOWN			10
#define SWR_SPREAD_UPDOWN		11
#define SWR_SHUT_LEFTRIGHT		12
#define SWR_SPREAD_LEFTRIGHT	13
#define SWR_WIND_HORIZONTAL		14
#define SWR_WIND_WERTICAL		15
#define SWR_OPPOSITE_LEFTRIGHT	16
#define SWR_OPPOSITE_UPDOWN		17
#define SWR_RAND18				18
#define SWR_RUNHOST_LEFT		19
#define SWR_RUNHOST_UP			20

int SWR_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire);
void SWR_DefaultLst(void);
#endif

