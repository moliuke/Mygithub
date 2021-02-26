#ifndef __HW2G_400_H
#define __HW2G_400_H
#include <stdio.h>
#include "config.h"
#include "debug.h"


#define LED_BRIGHT_MAX		31
#define LED_BRIGHT_MIN		0

#define LED_BRIGHT_MODE_HAND	0x30
#define LED_BRIGHT_MODE_AOTO	0x31

#define LED_STATUS_ON		1
#define LED_STATUS_OFF		0 

void HW2G400_aotoBright(void);
void HW2G400_SETLEDbright(uint8_t Bright);
int HW2G400_SetScreenStatus(uint8_t status);


#endif


