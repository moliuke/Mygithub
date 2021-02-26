#ifndef __JX_DISPLAY_H
#define __JX_DISPLAY_H
#include <stdio.h>
#include "debug.h"
#include "common.h"
#include "config.h"

#define 	JX_CLEAN_SCREEN			0
#define 	JX_DIRECTLY				1
#define 	JX_MOVE_UP					2
#define 	JX_MOVE_DOWN				3
#define 	JX_MOVE_LEFT				4
#define 	JX_MOVE_RIGHT				5

int JX_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire);
void JX_DefaultLst(void);
#endif

