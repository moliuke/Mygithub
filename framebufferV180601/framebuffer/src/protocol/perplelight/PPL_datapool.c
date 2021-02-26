#include "PPL_datapool.h"
#include "debug.h"
#include "config.h"

//紫光协议数据交换池
typedef struct __DATApool
{
	char		curlist[8];		//当前的播放列表
	uint16_t 	listLen;
	
	char 		curdsplay[512];	//当前显示的文字
	uint16_t 	contentLen;
	uint16_t 	inform;			//出字方式
	uint32_t 	inSpeed;		//出字速度
	uint32_t 	stayTime;		//停留时间

	uint8_t 	brightMode;		//亮度模式
	uint8_t 	bright_R;		//亮度值
	uint8_t 	bright_G;
	uint8_t 	bright_B;
	
}DATApool_t;

static DATApool_t PLL_DATA_pool;


#if 0
//设置当前显示文字的内容、长度、出字方式、出字速度、停留时间
void PPL_SETCurDspContent(char *Content,uint16_t len,uint16_t inform,uint32_t inSpeed,uint32_t stayTime)
{
	memcpy(PLL_DATA_pool.curdsplay,Content,len);
	PLL_DATA_pool.contentLen = len;
	
	PLL_DATA_pool.inform	= inform;
	PLL_DATA_pool.inSpeed	= inSpeed;
	PLL_DATA_pool.stayTime	= stayTime * 100;
	
}

//获取当前显示文字的内容、长度、出字方式、出字速度、停留时间
void PPL_GETCurDspContent(char *Content,uint16_t *len,uint16_t *inform,uint32_t *inSpeed,uint32_t *stayTime)
{
	DEBUG_PRINTF;
	memcpy(Content,PLL_DATA_pool.curdsplay,PLL_DATA_pool.contentLen);
	DEBUG_PRINTF;
	*len 		= PLL_DATA_pool.contentLen;
	DEBUG_PRINTF;
	*inform 	= PLL_DATA_pool.inform;
	DEBUG_PRINTF;
	*inSpeed 	= PLL_DATA_pool.inSpeed;
	DEBUG_PRINTF;
	*stayTime 	= PLL_DATA_pool.stayTime;
	DEBUG_PRINTF;
}



//设置当前显示列表
void PPL_SETCurDspLst(char *list,uint8_t len)
{
	memcpy(PLL_DATA_pool.curlist,list,len);
	PLL_DATA_pool.listLen = len;
}
//获取当前显示列表
void PPL_GETCurDspLst(char *list,uint8_t *len)
{
	memcpy(list,PLL_DATA_pool.curlist,PLL_DATA_pool.listLen);
	*len = PLL_DATA_pool.listLen;
}


//设置当前亮度
void PPL_SETCurBright(uint8_t RGB_R,uint8_t RGB_G,uint8_t RGB_B)
{
	PLL_DATA_pool.bright_R	= RGB_R;
	PLL_DATA_pool.bright_G	= RGB_G;
	PLL_DATA_pool.bright_B	= RGB_B;
	debug_printf("PLL_DATA_pool.bright_R = %d,PLL_DATA_pool.bright_G = %d,PLL_DATA_pool.bright_B = %d\n",PLL_DATA_pool.bright_R,PLL_DATA_pool.bright_G,PLL_DATA_pool.bright_B);
}
//获取当前亮度
void PPL_GETCurBright(uint8_t *RGB_R,uint8_t *RGB_G,uint8_t *RGB_B)
{
	*RGB_R = PLL_DATA_pool.bright_R;
	*RGB_G = PLL_DATA_pool.bright_G;
	*RGB_B = PLL_DATA_pool.bright_B;
}



void PPL_SETBrightMode(uint8_t mode)
{
	PLL_DATA_pool.brightMode = mode;
}

void PPL_GETBrightMode(uint8_t *mode)
{
	*mode = PLL_DATA_pool.brightMode;
}


#endif




