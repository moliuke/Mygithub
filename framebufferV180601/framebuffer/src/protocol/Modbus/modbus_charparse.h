#ifndef __MODBUS_CHARPARSE_H
#define __MODBUS_CHARPARSE_H
#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "content.h"
#include "modbus_config.h"


#define PLAYLST			list_dir_1"/modbus.lst"
#define MODBUSDEFLST	list_dir_1"/modbusdef.lst"

//#define MDBS_PARSE_DEBUG
#ifdef MDBS_PARSE_DEBUG
#define MDBS_CHARPARSEPRINTF	DEBUG_PRINTF
#define MDBS_charparse_printf		debug_printf
#else
#define MDBS_CHARPARSEPRINTF	
#define MDBS_charparse_printf	
#endif


#define CTRLWAY_WHOLE		0
#define CTRLWAY_ESCAPE		1

#define FONT_H			0
#define FONT_K			1
#define FONT_S			2
#define FONT_F			3

#define FONTSIZE_FIX	0
#define FONTSIZE_16X16	1
#define FONTSIZE_24X24	2
#define FONTSIZE_32X32	3
#define FONTSIZE_48X48	4
#define FONTSIZE_64X64	5
#define FONTSIZE_24X16	6
#define FONTSIZE_16X24	7
#define FONTSIZE_20X20 	8
#define FONTSIZE_28X28	9
#define FONTSIZE_36X36	10
#define FONTSIZE_40X40	11
#define FONTSIZE_44X44	12

#define LBSTATE_OPEN	1
#define LBSTATE_CLOSE	0



typedef struct 
{
	uint16_t  ItemWidth;	//信息宽度
	uint16_t  ItemHeight;	//信息高度
	uint16_t  ItemCx;		//信息横坐标
	uint16_t  ItemCy;		//信息纵坐标
}ItemMsg_t;

typedef struct 
{
	uint16_t LineWidth; 	//行信息最大宽度
	uint16_t LineHeight;	//行信息最大高度
	ItemMsg_t ItemMsg[10];
}MLineMsg_t;

typedef struct _realtime
{
	//文字实时数据
	uint8_t ctrway;		//控制方式
	uint8_t trbmsg;		//故障信息
	uint8_t dspstate;	//显示状态
	uint8_t swcode;		//软件故障码
	uint8_t hwcode;		//硬件故障码
	uint8_t ineffect;	//出字方式
	uint8_t intvtime;	//时间间隔
	uint8_t font;		//字体
	uint8_t fontsize;	//字体大小
	uint8_t bmpnum;		//点阵图片编码
	uint8_t bmptype;	//点阵图片类型
	uint16_t strLen;

	//光带实时数据
	uint8_t lbtrbmsg;
	uint8_t lbdspstate;
	uint8_t lbswcode;
	uint8_t lbhwcode;
	uint16_t lbsemenLen;		//光带段数
	
	uint8_t strmsg[1024];//文字信息
	uint8_t lbsement[256];	//光带状态
}REALTime_t;


extern REALTime_t REALTime;
int Mdbs_GetLBandRealTimeData(uint8_t *RTdata,uint16_t *Len);
int Mdbs_SetLBandSement(uint8_t StartUnit,uint8_t unitCount,uint8_t data);
int Mdbs_charparse(ContentList *head,uint8_t *Centent,uint16_t Len);
int Mdbs_WritePlayLst(uint8_t *Content,uint16_t len);
void Mdbs_DefaultLst(void);

void Mdbs_InitScrSize(int width,int height);
bool isLBopen(void);
int Mdbs_SetTxtRealTimeData(uint8_t *content,uint16_t len);
int Mdbs_GetTxtRealTimeData(uint8_t *RTdata,uint16_t *Len);
void Mdbs_LBandArgInit(uint8_t state,uint8_t sement);

#endif


