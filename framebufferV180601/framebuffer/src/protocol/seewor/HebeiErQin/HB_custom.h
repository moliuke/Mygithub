#ifndef _HB_COSTOM_H
#define _HB_COSTOM_H

#include <stdio.h>
#include "debug.h"
#include "config.h"
#include "Dev_serial.h"


#define currenttime		"��ǰʱ��"
#define statusmsg		"״̬��Ϣ"
#define commuStatus		"ͨ��:"
#define powerStatus		"��Դ:"
#define lightStatus		"����:"
#define thandStatus		"����:"
#define systemStatus	"ϵͳ:"

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

