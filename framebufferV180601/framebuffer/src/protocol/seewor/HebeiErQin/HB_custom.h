#ifndef _HB_COSTOM_H
#define _HB_COSTOM_H

#include <stdio.h>
#include "debug.h"
#include "config.h"
#include "Dev_serial.h"


#define currenttime		"当前时间"
#define statusmsg		"状态信息"
#define commuStatus		"通信:"
#define powerStatus		"电源:"
#define lightStatus		"光敏:"
#define thandStatus		"防雷:"
#define systemStatus	"系统:"

#define BUTTON_UP		1
#define BUTTON_DOWN		0

#define BT_TIME_UP		image_dir"/sys/anjian1.bmp"
#define BT_TIME_DOWN	image_dir"/sys/anjian2.bmp"

typedef struct _button
{
	uint16_t CX_l;	//left
	uint16_t CX_r;	//right
	uint16_t CY_u;	//up
	uint16_t CY_d;	//down
	
	uint8_t state;
	char 	upbmp[64];
	char 	downbmp[64];
}Button_t;

//void _ProtocolRelationDestroy(void);
int HB_pthread_create(void);
void cacheMalloc(void);
char *HBerqinProject(void);
void hb_timer_init(void);


#endif

