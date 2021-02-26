#ifndef __ZH_DISPLAY_H
#define __ZH_DISPLAY_H
#include <stdio.h>
#include "debug.h"
#include "common.h"
#include "config.h"

#define 	ZH_CLEAN_SCREEN			0
#define 	ZH_DIRECTLY				1


int ZH_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire);
void ZH_DefaultLst(void);
#endif

