#ifndef __DISPLAY_H
#define __DISPLAY_H 
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "character.h"
#include "bitmap.h"
#include "../cache.h"
#include "mylist.h"
#include "../module/image_gif.h"

//#define DSP_DEBUF
#ifdef DSP_DEBUF
#define DSP_DEBBUG_PRINTF	DEBUG_PRINTF
#define dsp_debug_printf	debug_printf
#else
#define DSP_DEBBUG_PRINTF	
#define dsp_debug_printf	
#endif


#define CONTENT_SIZE	640*480*4


#define BGR_B	0
#define BGR_G	1
#define BGR_R	2
#define BGR_A	3

#define CTBUF_CAN_READ		0
#define CTBUF_CAN_WRITE		1


#define TYPE_PITURE		0
#define TYPE_CHARACTER	1


#define EFFECT_OPPOSITE		0
#define EFFECT_STAGGER		1
#define EFFECT_WINSHADE		2
#define EFFECT_MOVE			3
#define EFFECT_SPREAD		4
#define EFFECT_DIRECT		5
#define EFFECT_UPROLL		6
#define EFFECT_RUNNINGHOST	7
#define EEFECT_DEFAULT		8
#define EEFECT_ALLBLACK		9
#define EEFECT_CIRCLE		10

#define MODE_ENTER			1
#define MODE_EXIT			0

#define DIRECTION_NO		0
#define DIRECTION_LEFT		1
#define DIRECTION_RIGHT		2
#define DIRECTION_UP		3
#define DIRECTION_DOWN		4
#define DIRECTION_LFRI		5
#define DIRECTION_UPDW		6
#define DIRECTION_CENTER	7


typedef struct __effect
{
	uint8_t inType;
	uint8_t inDire;
	uint8_t outType;
	uint8_t outDire;
	uint32_t speed;
}Effect_t;




typedef struct 
{
	uint32_t pos_x;
	uint32_t pos_y;
}coordinate_t;



enum{EFF_IN = 0,EFF_OUT,EFF_GIF};

typedef struct 
{
	coordinate_t start;
	coordinate_t end;
}region_t;




typedef struct dsp_fb
{
	uint16_t 		dsp_width;
	uint16_t 		dsp_height;
	uint8_t  		*dsp_buffer;
	struct dsp_fb 	*fb_next;
	region_t		dsp_area;
}dsp_content_t;








typedef struct _locate
{
	uint16_t 	cx;
	uint16_t 	cy;
	uint16_t 	CTwidth;
	uint16_t 	CTheight;
}CTTsize_t;

typedef struct _effect
{
	uint8_t 	number;
	uint8_t 	direction;
	uint8_t 	endFlag;	//标记出入屏动作完成

	uint8_t 	Fflag;
	uint8_t 	Fstep;		//帧步进多少个像素点
	uint16_t 	Forder; 	//帧顺序，也就是当前显示到第几帧
	uint16_t	Fcurls; 	//frame cur lines,it meases that how many lines 
	uint16_t	Flast;
	uint16_t	Ftotal; 	//完成动态显示共有多少帧
	uint16_t 	Rlong;		
	uint16_t 	Rpos;
	uint16_t 	Rpos1;
	uint16_t 	RMwidth;

	uint8_t 	Psize;		//百叶窗每块大小
	uint8_t 	Blocks; 	//百叶窗可分成几块
	uint8_t 	Wpieces;	//百叶窗科分成几块

	uint8_t 	Rflag;		//if the last piece is not large as the Psize size,then this flag is set.
	uint8_t 	RMsize; 	//the last piece remaind size

	uint32_t 	speed;
	
	

}EFFECT_T;


//////////////////////displayV3/////////////////////////////
typedef struct _Display
{
	char 		filename[64];
	uint8_t 	type;
	uint32_t 	cx;
	uint32_t 	cy;
	uint16_t 	width;
	uint16_t 	height;
	uint32_t 	stoptime;

	GIFstruct_t	*GIFstruct;

	
	EFFECT_T 	effectIn;
	EFFECT_T	effectOut;
	int 		(*display_in)(struct _Display *);
	int 		(*display_out)(struct _Display *);
	int 		(*GIF_display)(struct _Display *);

	struct list_head	list;
}DISPLAY_T;


typedef struct _dspstruct
{
	uint8_t 	effIn;
	uint32_t 	cx;
	uint32_t 	cy;
	uint16_t 	width;
	uint16_t 	height;
}DSPStruct_t;

typedef struct _dspcnt
{
	int 		clrenable;		//1允许清屏，0不允许
	int 		dspcnt;			//计数一屏中共有多少个显示节点
	DSPStruct_t DSPStruct[48];	//显示节点
}DSPCNT_T;


extern DSPCNT_T DSPcnt;
extern pthread_mutex_t dsp_mutex;
extern sem_t sem;

void DSP_LstInit(struct list_head *DSPhead);
int DSP_LstFinish(struct list_head *DSPhead);
int DSP_LstEmpty(struct list_head *DSPhead);
int DSP_LstInsert(DISPLAY_T *dspnode,struct list_head *DSPhead);
int DSP_LstFetch(DISPLAY_T **dspnode,struct list_head *pos,struct list_head *DSPhead);
void DSP_LstDestroy(DISPLAY_T *DSPLstHead);


extern dsp_content_t *dsp_content_fb;



void dsp_frame_size_set(uint32_t width,uint32_t height);
void dsp_frame_size_get(uint32_t *width,uint32_t *height);




dsp_content_t *dsp_content_bufExtendMalloc(void);
void dsp_content_bufeExtendFree(void);

int TXT_setEffect(CTTsize_t *CTTsize,CACHEstruct_t *CACHEstruct,TXTstruct_t *TXTstruct);
int TXT_setEffect_runhost(CTTsize_t *CTTsize,CACHEstruct_t *CACHEstruct,TXTstruct_t *TXTstruct);

int GIF_GetFrameInfo(DISPLAY_T *dspnode);

int GIF_DecodeFrame(DISPLAY_T *dspnode);

void DSP_INITstruct(DISPLAY_T *dspnode,uint32_t cx,uint32_t cy,uint16_t CTwidth,uint16_t CTheight,
	uint8_t type,uint8_t in,uint8_t in_dir,uint8_t out,uint8_t out_dir,uint32_t speed);
int itemDisplay(DISPLAY_T *DSPhead,uint8_t InOutFlag);
int DSPStructInit(DISPLAY_T *DSPhead);
#endif

