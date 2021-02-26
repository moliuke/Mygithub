#ifndef __XKTYPE_H
#define __XKTYPE_H


#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"

// 常量
#define MAXUNIT 16;


//类型定义
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;

#define PRIORITYN	-1
#define PRIORITY0	0
#define PRIORITY1	1
#define PRIORITY2	2
#define PRIORITY3	3
#define PRIORITY4	4


//---------------------------------------------------------------------- 显示方面---------------------------------------------------//


//行信息
typedef struct 
{
	uint16_t LineWidth;
	uint16_t LineHeight;
}LineMst_t;


#define COLOR_H		0
#define COLOR_R		1
#define COLOR_G		2
#define COLOR_B		3
#define COLOR_Y		4

// 字符串单元
typedef struct String
{

	int ScreenSeq;
	int LineSeq;
	
	int cx;
	int cy;
	int nCellOrder;
	int Color;

	int flash;

	// 布局
	int nSpace;
	int nLineSpace;
	//  字体颜色
	uint8_t nForeColor[3];
	uint8_t nBkColor[3];
	uint8_t nShadowColor[3];
	//  字体类型
	uint8_t nFontSize;
	uint8_t nFontType;			//字体类型
	uint8_t strContent[1024];
	uint16_t strContentLen;
	//
	struct String *pNext;
}XKCellString;

// 图像图形单元
typedef struct Image
{
	int ScreenSeq;
	int LineSeq;

	int cx;
	int cy;
	int type;
	int nCellOrder;
	char strImage[16];
	//
	struct Image *pNext;
}XKCellImage;


typedef struct _PNGCell
{
	int ScreenSeq;
	int LineSeq;
	int cx;
	int cy;
	int type;
	int nCellOrder;
	char pngImage[16];
	struct _PNGCell *pNext;
	
}XKCellPngimg;



// 动画单元
typedef struct Animate
{
	 int cx;
	 int cy;
	 int nCellOrder;
	 int nFormatType;
	 int nPlayTime;
	 char strAnimate[16];
	//
	struct Animate *pNext;
}XKCellAnimate;

// 效果列表
typedef struct Effect
{
	int cx;
	int cy;
    int nCellOrder;
	char strEffect[4];
	//
	struct Effect *pNext;
}XKCellEffect;

// 闪烁单元
typedef struct Twinkle
{
	int cx1;
	int cy1;
	int cx2;
	int cy2;

	// 闪烁参数
	BYTE nTwinkleTime;
	BYTE nTwinkleNum;
    //
	struct Twinkle * pNext;
}XKCellTwinkle;


typedef struct _priority
{
	int8_t lbpri;
	int8_t strpri;
	int8_t gifpri;
	int8_t bmppri;
	int8_t pngpri;
}DSPriorit_t;



#define	 DSPTYPE_DEF	0
#define  DSPTYPE_IMG	1
#define  DSPTYPE_STR	2
#define  DSPTYPE_GIF	3
#define  DSPTYPE_LBD	4
#define  DSPTYPE_PNG	5
#define  DSPTYPE_JPG	6
#define  DSPTYPE_CGL	7
#define  DSPTYPE_CGS	8


typedef struct 
{
	uint16_t Cx;
	uint16_t Cy;
	
	uint8_t  LBnum;
	int (*LBandFunc)(int,int,int,int,uint8_t *);
}LIGHTBand_t;



//显示元素，显示元素只指定了元素的；类型、显示坐标、出入屏方式以及该显示元素的内容
typedef struct Dspnode
{
	uint8_t 		type;	//标记显示类型，图片、文字
	uint8_t 		Sseq;	//标记幕序
	uint8_t 		Lseq;	//标记行号
	uint8_t 		time;	//停留时间
	uint8_t 	 	effIn;	//入屏方式
	uint8_t 		effOut;	//出屏方式
	
	uint16_t 		Cx;		//显示横坐标
	uint16_t 		Cy;		//显示纵坐标
	uint16_t 		width;	//内容宽度
	uint16_t 		height;	//内容高度
	
	XKCellString 	*XKCellStr;	//文字内容
	XKCellImage		*XKCellIma;	//图片内容
	XKCellAnimate   *XKCellAni;	//动画内容
	LIGHTBand_t		*LIGHTBand;	//光带内容
	XKCellPngimg	*XKCellPng;	//png图片内容

	int (*DSPfunc)(struct Dspnode *,uint8_t *);
	struct Dspnode	*pNext; 
}DSPNode_t;


	

// 显示单元 
typedef struct Item
{
	uint8_t  nID;
	uint8_t  align;			//对齐方式
	uint8_t  nItemType;

	// 动画参数
	uint8_t  IsHaveAnimate;
	char strAnimateName[20];

	// 播放参数
	uint32_t nMoveSpeed;
	uint32_t nDelayTime;
	
	int nEffectIn;
	uint8_t nEffectOut;
	uint8_t nEffectShow;

	// 空间布局
	uint8_t nMargin[4];

	//字体最大值
	uint8_t FontMaxSize;

	//一幕信息在显示所有信息时所占的屏幕整体宽度和高度
	uint8_t StrLines;
	uint16_t ScrMaxWidth;
	uint16_t ScrMaxHeight;

	//行信息
	LineMst_t LineMst[16];
	
	// 具体内容
	XKCellTwinkle 	*pCellTwinkle_head,	*pCellTwinkle_tail;
	XKCellString 	*pCellString_head,	*pCellString_tail;
	XKCellImage 	*pCellImage_head,	*pCellImage_tail;
    XKCellAnimate 	*pCellAnimate_head,	*pCellAnimate_tail;
    XKCellEffect 	*pCellEffect_head,	*pCellEffect_tail;


	DSPNode_t		*DSPNode_head,	*DSPNode_tail;

	//当前显示内容
	void			*Curplay;
	void			(*setcurplaying)(void *);
	
	//add by mo 2020.10.14
	//用于存在这一幕原始所有信息
	uint8_t itemconent[512];
	
	struct Item 	*pNext;

}XKDisplayItem;






struct COLORREF
{
  BYTE byRed;
  BYTE byGreen;
  BYTE byBlue;
};


/**grup @refresh flag*/
enum{
	FLAG_NOT_REFRESH = 0,		//播放列表无更新
	FLAG_RESFRESH,				//播放列表有更新
	LST_FREE,					//播放列表为空
	LST_REFLASH					//播放列表有更新
	};
typedef struct
{
	uint32_t 		refresh;//refer to @refresh,indicate wether the playing list has been refresh or not
	uint16_t 		itemcount;
	XKDisplayItem 	*head;	//playing list head pointer
	XKDisplayItem 	*tail;	//playing list tail pointer
}ContentList;



// --------------------------------------------------------------------------------设备方面---------------------------------------------------//
struct XKBoxStatus                   // 单个箱体参数
{
	BYTE nType[8];
	BYTE nData[8];		//单个箱体温度值、电压1、电压2、.....电压5，门开关状态，共7个字节，第8字节保留
	BYTE nTemperature;
	BYTE nVoltage[5];
	BYTE bDoorSwitch;
	BYTE bDriver[8];
	BYTE bDriverEx;		//单个通道的通道状态
	BYTE bSystem;		//单个通道的系统状态
};

struct XKValueRange                 //   参数范围
{
   unsigned int  Type;
   unsigned int  LowValue;
   unsigned int  HighValue;
   unsigned int  LowAlarm;
   unsigned int  HighAlarm;
};

struct XKDeviceAction
{
	BYTE IsPowerSwitchOn;  // 电源开关状态
	BYTE IsFanSwitchOn ;   // 风扇开关状态
	BYTE IsAlarmSwitchOn;  // 报警器开关状态
};


extern ContentList content;
BYTE DecodeItemString(ContentList *head,char *strItemContent);
void ClearContent(ContentList *head);
int XKDisplayInsert(XKDisplayItem *XKDisplay,int scenecnt);


int InitContentlist(ContentList *head);
int DecodePlayList2(ContentList *head,char *strPlayListName);
XKDisplayItem *GetItemHead(ContentList *head);
void Initcontentnode(XKDisplayItem *head);
int AddItemString(XKDisplayItem *head,XKCellString *pCellString);
int AddDisplayItem(ContentList *head,XKDisplayItem * pItem);
int AddItemDSPNode(XKDisplayItem *head,DSPNode_t * DSPNode);

int MXKDisplayInsert(XKDisplayItem *XKDisplay,int scenecnt);

void DSPrioritInit(void);
void SetDSPriorit(uint8_t type,uint8_t priority);
void GetDSPriorit(uint8_t type,uint8_t *priority);
#ifdef __cplusplus
}
#endif

#endif /* _DMP_DOS_Xktypes_H_ */

