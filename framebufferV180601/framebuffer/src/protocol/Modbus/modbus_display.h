#ifndef __MODBUS_DISPLAY_H
#define __MODBUS_DISPLAY_H
#include <stdio.h>
#include "debug.h"
#include "config.h"
#include "content.h"
#include "modbus_config.h"
//#define MDBS_DISPLAYDEBUG

#ifdef MDBS_DISPLAYDEBUG
#define MDBS_DISPLAY_DEBUG  	DEBUG_PRINTF
#define mdbs_display_printf		debug_printf
#else
#define MDBS_DISPLAY_DEBUG  	
#define mdbs_display_printf		
#endif



#define MODBUS_LED_ON	1
#define MODBUS_LED_OFF	0



//对齐方式
#define ALIGN_UP				0
#define ALIGN_MINDLE_UPDOWN		1
#define ALIGN_DOWN				2
#define ALIGN_LEFT				3
#define ALIGN_MINDLE_LEFTRIGHT	4
#define ALIGN_RIGHT				5
#define ALIGN_UPDOWN_LEFTRIGHT	6

//出字方式
#define 	MDBS_DIRECTLY			1
#define 	MDBS_FLASH				2
#define 	MDBS_MOVE_LEFT			3
#define 	MDBS_MOVE_UP			4
#define 	MDBS_MOVE_RIGHT			5
#define 	MDBS_MOVE_DOWN			6
#define 	MDBS_RUNHOST			7


extern ContentList ContentCache;
extern ContentList LineCache; 
extern ContentList ScreenCache;

void XKCellStringPrintf(XKCellString *XKCellStr);
int Mdbs_CoordinateProcess(ContentList *cache);
int Mdbs_SwitchScreenDeal(ContentList *cache);
int Mdbs_SwitchLineDeal(ContentList *cache);


int Mdbs_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire);
void Mdbs_SetScreenSize(uint32_t WIDTH,uint32_t HEIGHT);
void DSPNodePrintf(DSPNode_t *DSPNode);

#endif

