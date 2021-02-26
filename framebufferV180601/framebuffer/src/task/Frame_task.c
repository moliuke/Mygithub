#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>   
#include <stddef.h>
//#include <png.h>
#include "queue.h"

#include "../Hardware/Data_pool.h"

#include "task.h"
#include "Frame_task.h"
#include "../protocol/Defmsg.h"
#include "../protocol/PTC_init.h"


static int LBandFlag = 0;
static uint32_t ScrWith = 0,ScrHeight = 0; 
#if 0
int DSPFlag = 0;
uint8_t TestMode[8][9] ={
		{0x02,0x35,0x36,0x30,0x30,0x81,0x65,0x50,0x03}, //表示全红
		{0x02,0x35,0x36,0x30,0x30,0x82,0x55,0x33,0x03}, //表示全绿
		{0x02,0x35,0x36,0x30,0x30,0x83,0x45,0x12,0x03}, //表示全蓝
		{0x02,0x35,0x36,0x30,0x30,0x84,0x35,0xF5,0x03}, //表示全黄
		{0x02,0x35,0x36,0x30,0x30,0x85,0x25,0xD4,0x03}, //表示全紫
		{0x02,0x35,0x36,0x30,0x30,0x86,0x15,0xB7,0x03}, //表示全青
		{0x02,0x35,0x36,0x30,0x30,0x87,0x05,0x96,0x03}, //表示全白
		{0x02,0x35,0x36,0x30,0x30,0x00,0xE4,0xF9,0x03}	//表示退出测试模式
		};

static void CheckIfTestMode(XKCellString *Str)
{
	static int TestStatus = 0;
	uint8_t TestFlag; 
	uint8_t BrightV;
	//uint8_t SendBright[] = {0xD8,0x00,0x02,0x00,0x00,0x00,0x00,0xAA};
	//全红

	//表示全红
	if(Str->nForeColor[0] == 255 && Str->nBkColor[0] == 255 && Str->nForeColor[1] == 0
	&& Str->nBkColor[1] == 0 && Str->nForeColor[2] == 0 && Str->nBkColor[2] == 0)
	{
		DP_SetTestMode(TEST_MODE);

		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[0],9);
		pthread_mutex_unlock(&queue_uart_mutex);
	}
	//表示全绿
	else if(Str->nForeColor[0] == 0 && Str->nBkColor[0] == 0 && Str->nForeColor[1] == 255
	&& Str->nBkColor[1] == 255 && Str->nForeColor[2] == 0 && Str->nBkColor[2] == 0)
	{
		DP_SetTestMode(TEST_MODE);

		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[1],9);
		pthread_mutex_unlock(&queue_uart_mutex);		
	}
	
	else if(Str->nForeColor[0] == 0 && Str->nBkColor[0] == 0 && Str->nForeColor[1] == 0
	&& Str->nBkColor[1] == 0 && Str->nForeColor[2] == 255 && Str->nBkColor[2] == 255)
	{
		DP_SetTestMode(TEST_MODE);

		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[2],9);
		pthread_mutex_unlock(&queue_uart_mutex);		
	}
	
	else if(Str->nForeColor[0] == 255 && Str->nBkColor[0] == 255 && Str->nForeColor[1] == 255
	&& Str->nBkColor[1] == 255 && Str->nForeColor[2] == 0 && Str->nBkColor[2] == 0)
	{
		DP_SetTestMode(TEST_MODE);

		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[3],9);
		pthread_mutex_unlock(&queue_uart_mutex);		
	}
	//紫色
	else if(Str->nForeColor[0] == 128 && Str->nBkColor[0] == 128 && Str->nForeColor[1] == 0
	&& Str->nBkColor[1] == 0 && Str->nForeColor[2] == 128 && Str->nBkColor[2] == 128)
	{
		DP_SetTestMode(TEST_MODE);

		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[4],9);
		pthread_mutex_unlock(&queue_uart_mutex);
	}
	else if(Str->nForeColor[0] == 0 && Str->nBkColor[0] == 0 && Str->nForeColor[1] == 128
	&& Str->nBkColor[1] == 128 && Str->nForeColor[2] == 128 && Str->nBkColor[2] == 128)
	{
		DP_SetTestMode(TEST_MODE);

		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[5],9);
		pthread_mutex_unlock(&queue_uart_mutex);
	}
//全白
	else if(Str->nForeColor[0] == 255 && Str->nBkColor[0] == 255 && Str->nForeColor[1] == 255
	&& Str->nBkColor[1] == 255 && Str->nForeColor[2] == 255 && Str->nBkColor[2] == 255)
	{
		DP_SetTestMode(TEST_MODE);
		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[6],9);
		pthread_mutex_unlock(&queue_uart_mutex);		
	}
	else
	{
		uint8_t mode; 
		DP_GetTestMode(&mode);
		if(mode == TEST_MODE)
		{
			pthread_mutex_lock(&queue_uart_mutex);
			EnQueue(queuehead,TestMode[7],9);
			pthread_mutex_unlock(&queue_uart_mutex);	
		}
	}
}
#endif

//解析文字型显示元素
static int STR_DSPDeal(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	int contentWidth = 0;
	XKCellString *str = DSPNode->XKCellStr;
	CTTsize_t CTTsize;
	Pcolor_t Bcolor,Fcolor;
	charattr_t charattr;
	TXTstruct_t TXTstruct;

	//显示内容坐标超出屏幕范围
	if(DSPNode->Cx >= ScrWith || DSPNode->Cy >= ScrHeight)
		return -1;

	//检测是否字体颜色与背景色是同一颜色，颜色相同则将测试标志置1
	//detele by mo 20201120
	//CheckIfTestMode(str);
	
	//开辟一个显示元素的节点
	DISPLAY_T *dspnode = (DISPLAY_T *)malloc(sizeof(DISPLAY_T));
	memset(dspnode,0x00,sizeof(DISPLAY_T));
	memset(TXTCache,0,FONT_CACHE_SIZE);

	//对给定的字符串，到字库中解析
	Bcolor.red		= str->nBkColor[0];
	Bcolor.green	= str->nBkColor[1];
	Bcolor.blue 	= str->nBkColor[2];
	Fcolor.red		= str->nForeColor[0];
	Fcolor.green	= str->nForeColor[1];
	Fcolor.blue 	= str->nForeColor[2];
	TXT_INITstruct(&TXTstruct,str->strContent,strlen(str->strContent),
		str->nFontType,str->nFontSize,&Bcolor,&Fcolor,str->nSpace,0,TXTCache,FONT_CACHE_SIZE,SCREEN_BPP);
	if(TXTstruct.TXT_decoder(&TXTstruct) < 0)
	{
		free(dspnode);
		return -1;			
	}

	CTTsize.cx		= DSPNode->Cx;
	CTTsize.cy		= DSPNode->Cy;

	//当显示内容长度小于屏幕宽度且被用户使用滚屏时，强制转到直接显示并居中显示
	contentWidth = strlen(str->strContent) * (str->nFontSize / 2);
	if(Effect->inType == EFFECT_RUNNINGHOST && contentWidth < ScrWith)
	{
		Effect->inType = EFFECT_DIRECT;
		CTTsize.cx = (ScrWith - contentWidth) / 2;
		CTTsize.cy = (ScrHeight - str->nFontSize) / 2;
	}

	//非跑马灯，将TXT_decoder函数解析出来的一整串文字，填入到CTTcache这块跟屏幕
	//大小一样的缓存中，遇到强制换行或者超出屏幕宽度就自动换行
	if(Effect->inType != EFFECT_RUNNINGHOST)
	{
		TXT_setEffect(&CTTsize,CTTcache,&TXTstruct);
		
	}
	//跑马灯，将TXT_decoder函数解析出来的一整串文字，填入到CTTcache这块跟屏幕
	//大小一样的缓存中，CTTcache形状不再是跟屏幕一样，而是高度为str->nFontSize，宽度不确定的一块缓存
	else
		TXT_setEffect_runhost(&CTTsize,CTTcache,&TXTstruct);

	//填充当前正在显示内容的信息，包括出入屏方式、停留时间等，给上位机获取当前显示内容使用的
	DSPContent_t DSPContent;
	memset(&DSPContent,0,sizeof(DSPContent));
	DSPContent.type = DSPTYPE_STR;
	DSPContent.inform = DSPsrcNode->nEffectIn;
	DSPContent.speed = DSPsrcNode->nMoveSpeed;
	DSPContent.staytime = DSPsrcNode->nDelayTime;
	DSPContent.fonttype = str->nFontType;
	DSPContent.fontsize = str->nFontSize;
	DSPContent.strLen = strlen(str->strContent);
	memcpy(DSPContent.charstr,str->strContent,strlen(str->strContent));
	SetDSPContent(&DSPContent);
	//跟上面一样，这是老版本的，上面是新版本的，还在调试中
	DP_SetCurPlayContent(str->strContent,strlen(str->strContent),str->nFontType,str->nFontSize,DSPsrcNode->nEffectIn,DSPsrcNode->nMoveSpeed,DSPsrcNode->nDelayTime,str->nCellOrder);

	//初始化一个显示节点dspnode，指明该节点显示内容类型(图片、文字、动画)，坐标，出入屏方式、速度、甚至还有出入屏方式该调用哪个动作接口
	DSP_INITstruct(dspnode,CTTsize.cx,CTTsize.cy,CTTsize.CTwidth,CTTsize.CTheight,TYPE_STR,Effect->inType,Effect->inDire,Effect->outType,Effect->outDire,Effect->speed);

	//将显示节点压入以DSPLstHead为表头的链表，DSPLstHead所代表的链表就是一幕信息
	//每一幕信息可能包含很多个显示节点
	DSP_LstInsert(dspnode,DSPLstHead);
}



static int IMG_DSPDeal(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	IMGstruct_t IMGstruct;
	XKCellImage *img = DSPNode->XKCellIma;

	//图片的显示坐标超过屏幕的范围
	if(DSPNode->Cx >= ScrWith || DSPNode->Cy >= ScrHeight)
		return -1;

	//开辟一个显示元素的节点
	DISPLAY_T *dspnode = (DISPLAY_T *)malloc(sizeof(DISPLAY_T));
	memset(dspnode,0x00,sizeof(DISPLAY_T));
	memset(IMGstruct.filename,0,sizeof(IMGstruct.filename));
	sprintf(IMGstruct.filename,"%s/%s",image_dir,img->strImage);
	debug_printf("IMGstruct->filename = %s\n",IMGstruct.filename);
	debug_printf("ScrWith = %d,ScrHeight = %d\n",ScrWith,ScrHeight);

	//初始化图片解析结构，告诉解析函数，图片解析到哪个缓存，解析到屏幕的什么坐标
	//以及解析图片的宽高受限于屏幕的宽高
	IMG_INITstruct(&IMGstruct,DSPNode->Cx,DSPNode->Cy,ScrWith,ScrHeight,CTTcache->cache);

	//开始解析图片
	if(IMGstruct.IMG_decoder(&IMGstruct) < 0)
	{
		free(dspnode);
		return -1;
	}

	//设置当前显示的内容为图片、出入屏方式、停留时间等
	DSPContent_t DSPContent;
	memset(&DSPContent,0,sizeof(DSPContent));
	DSPContent.type = DSPTYPE_IMG;
	DSPContent.inform = DSPsrcNode->nEffectIn;
	DSPContent.speed = DSPsrcNode->nMoveSpeed;
	DSPContent.staytime = DSPsrcNode->nDelayTime;
	DSPContent.maptype = img->type;
	memcpy(DSPContent.mapName,img->strImage,strlen(img->strImage));
	SetDSPContent(&DSPContent);

	//跟上面的一样，版本不一样也没删掉，这里可以有itemnode.setcurplaying代替的
	uint8_t BMPimg[12];
	sprintf(BMPimg,"%s",img->strImage);
	debug_printf("BMPimg = %s\n",BMPimg);
	DP_SetCurPlayContent(BMPimg,strlen(BMPimg),0,0,DSPsrcNode->nEffectIn,DSPsrcNode->nMoveSpeed,DSPsrcNode->nDelayTime,img->nCellOrder);
#if 0
	uint8_t mode; 
	DP_GetTestMode(&mode);
	if(mode == TEST_MODE)
	{
		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[7],9);
		pthread_mutex_unlock(&queue_uart_mutex);	
	}
#endif

	//初始化显示元素节点并插入显示链表中
	DSP_INITstruct(dspnode,DSPNode->Cx,DSPNode->Cy,IMGstruct.ctwidth,IMGstruct.ctheight,TYPE_IMG,Effect->inType,Effect->inDire,Effect->outType,Effect->outDire,Effect->speed);
	DSP_LstInsert(dspnode,DSPLstHead);
}

static int LBD_DSPDeal(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	LIGHTBand_t *LIGHTBand = DSPNode->LIGHTBand;
	DISPLAY_T *dspnode = (DISPLAY_T *)malloc(sizeof(DISPLAY_T));
	memset(dspnode,0x00,sizeof(DISPLAY_T));

	//解析光带
	LIGHTBand->LBandFunc(DSPNode->Cx,DSPNode->Cy,DSPNode->width,DSPNode->height,CTTcache->cache);
	debug_printf("DSPNode->Cx = %d,DSPNode->Cy = %d,DSPNode->width = %d,DSPNode->heig = %d\n",DSPNode->Cx,DSPNode->Cy,DSPNode->width,DSPNode->height);
#if 0
	uint8_t mode; 
	DP_GetTestMode(&mode);
	if(mode == TEST_MODE)
	{
		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[7],9);
		pthread_mutex_unlock(&queue_uart_mutex);	
	}
#endif
	//初始化光带显示元素节点并插入显示链表中
	DSP_INITstruct(dspnode,DSPNode->Cx,DSPNode->Cy,DSPNode->width,DSPNode->height,TYPE_LBD,Effect->inType,Effect->inDire,Effect->outType,Effect->outDire,Effect->speed);
	DSP_LstInsert(dspnode,DSPLstHead);
	LBandFlag = 1;
	return 0;
}


static int GIF_Flags = 0;
DISPLAY_T *GIF_DSPnode = NULL;

static int GIF_DSPDeal(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	XKCellAnimate *ani = DSPNode->XKCellAni;
	char gifpath[64];
	GIFstruct_t *GIFstruct = NULL;
	
	if(DSPNode->Cx > ScrWith || DSPNode->Cy > ScrHeight)
		return -1;
	DISPLAY_T *dspnode = (DISPLAY_T *)malloc(sizeof(DISPLAY_T));
	memset(dspnode,0x00,sizeof(DISPLAY_T));
	//检查GIF文件是否存在
	memset(gifpath,0,sizeof(gifpath));
	sprintf(gifpath,"%s/%s",image_dir,ani->strAnimate);
	debug_printf("gifpath = %s\n",gifpath);
	if(access(gifpath,F_OK) < 0)
	{
		return -1;
	}
	//初始化GIF结构，包括GIF文件，解析到那个缓存以及在缓存中的坐标，并且限制在屏幕的范围内
	debug_printf("Effect.inType = %d,Effect.inDire = %d,Effect.outType = %d,Effect.outDire = %d\n",Effect->inType,Effect->inDire,Effect->outType,Effect->outDire);
	GIFstruct = (GIFstruct_t *)malloc(sizeof(GIFstruct_t));
	GIF_INITstruct(GIFstruct,gifpath,DSPNode->Cx,DSPNode->Cy,ScrWith,ScrHeight,CTTcache->cache);

	
	//初始化gif图片，即打开gif，获取gif各项参数并为gif图片的解码分配内存
	GIFstruct->GIF_init(GIFstruct);
	debug_printf("GIFstruct->Fwidth = %d\n",GIFstruct->Fwidth);
	if(DSPNode->Cx > ScrWith || DSPNode->Cy > ScrHeight)
	{
		return -1;
	}
	//gif的有效显示内容不能超过屏幕的边框。超过则被忽略掉
	GIFstruct->ctwidth = (DSPNode->Cx + GIFstruct->Fwidth > GIFstruct->chwidth) ? 
		(GIFstruct->chwidth - DSPNode->Cx) : (GIFstruct->Fwidth);
	GIFstruct->ctheight = (DSPNode->Cy + GIFstruct->Fheight > GIFstruct->chheight) ? 
		(GIFstruct->chheight - DSPNode->Cy) : (GIFstruct->Fheight);

	memcpy(dspnode->filename,gifpath,strlen(gifpath));
	DP_SetCurPlayContent(ani->strAnimate,strlen(ani->strAnimate),0,0,DSPsrcNode->nEffectIn,DSPsrcNode->nMoveSpeed,DSPsrcNode->nDelayTime,ani->nCellOrder);
#if 0
	uint8_t mode; 
	DP_GetTestMode(&mode);
	if(mode == TEST_MODE)
	{
		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[7],9);
		pthread_mutex_unlock(&queue_uart_mutex);	
	}
#endif
	//初始化动态显示(即入屏出屏动作，显示坐标、gif宽高等信息，最重要的初始化GIF使用哪个函数播放)
	DSP_INITstruct(dspnode,DSPNode->Cx,DSPNode->Cy,GIFstruct->ctwidth,GIFstruct->ctheight,TYPE_GIF,Effect->inType,Effect->inDire,Effect->outType,Effect->outDire,Effect->speed);
	dspnode->GIFstruct = GIFstruct;
	//将gif节点压入链表中，帧分解线程将会从该链表中一个一个节点取下来显示
	DSP_LstInsert(dspnode,DSPLstHead);
	return 0;
}


static int PNG_DSPDeal(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	int ret = -1;
	
	if(DSPNode->Cx >= ScrWith || DSPNode->Cy >= ScrHeight)
		return -1;
	
	DISPLAY_T *dspnode = (DISPLAY_T *)malloc(sizeof(DISPLAY_T));
	memset(dspnode,0x00,sizeof(DISPLAY_T));

	//解析PNG
	ret = DSPNode->DSPfunc(DSPNode,CTTcache->cache); 
	debug_printf("ret = %d\n",ret);
	if(ret < 0)
		return -1;
#if 0
	uint8_t mode; 
	DP_GetTestMode(&mode);
	if(mode == TEST_MODE)
	{
		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[7],9);
		pthread_mutex_unlock(&queue_uart_mutex);	
	}
#endif
	//初始化png显示元素节点并插入显示链表中
	DP_SetCurPlayContent(DSPNode->XKCellPng->pngImage,12,0,0,DSPsrcNode->nEffectIn,DSPsrcNode->nMoveSpeed,DSPsrcNode->nDelayTime * 100 / (1000 * 1000),0);
	DSP_INITstruct(dspnode,DSPNode->Cx,DSPNode->Cy,DSPNode->width,DSPNode->height,TYPE_PNG,Effect->inType,Effect->inDire,Effect->outType,Effect->outDire,Effect->speed);
	DSP_LstInsert(dspnode,DSPLstHead);
	return 0;
}


static int CreateDspList(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	int ret = -1;
	debug_printf("CreateDspList : DSPNode->type = %d\n",DSPNode->type);
	//change by mo
	DP_SetCurPlayItemContent(DSPsrcNode->itemconent,strlen(DSPsrcNode->itemconent));
	switch(DSPNode->type)
	{
		//解析文字元素
		case DSPTYPE_STR:
			ret = STR_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		//解析图片元素
		case DSPTYPE_IMG:
			ret = IMG_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		//解析光带元素
		case DSPTYPE_LBD:
			ret = LBD_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		//解析GIF元素
		case DSPTYPE_GIF:
			ret = GIF_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		//解析PNG元素
		case DSPTYPE_PNG:
			ret = PNG_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		default:
			break;
	}
	return ret;
}



//停留时间，每一幕信息入屏后都要停留一定的时间
static int WaitTime(uint32_t time_us)
{
	uint8_t priority = -1;
	int sleepTime = 10000;
	int RemaindTime = time_us;
	while(RemaindTime > 0)
	{
		GetDSPriorit(DSPTYPE_LBD,&priority);
		if(priority == 0)
			return 0;
		sleepTime = (RemaindTime > 10000) ? 10000 : RemaindTime;
		usleep(sleepTime);
		RemaindTime -= sleepTime;

		//当发现播放列表更新时要立即跳出停留
		if(content.refresh == LST_REFLASH)
		{
			return 1;
		}
	}
	return 0;
}


static inline void FrameCacheClear(void)
{
	
	//1、首先锁住Display_task，不让其继续刷数据到framebuffer中
	DSPcache->W_Pos = (DSPcache->R_Pos + 1) % 3;
	//2、将三个显示缓存都清0
	memset(DSPcache->cache,0X00,DSPcache->CHwidth * DSPcache->CHheight * SCREEN_BPP*3);
	//3、将写缓存指针指向W_pos指向的缓存
	DSPcache->W_Addr = DSPcache->cache + DSPcache->W_Pos * DSPcache->CHwidth * DSPcache->CHheight * SCREEN_BPP;
}

static inline int FrameEnter(DISPLAY_T *Head)
{
	return itemDisplay(Head,EFF_IN);
}

static inline int FrameStay(DISPLAY_T *Head)
{
	return WaitTime(Head->stoptime);
}

static inline int FrameOut(DISPLAY_T *Head)
{
	return itemDisplay(Head,EFF_OUT);
}

void *pthread_Mdbsframebreak_task(void *arg)
{
	uint8_t priority = -1;
	uint8_t i = 0;
	Effect_t Effect;
	XKDisplayItem 	itemnode,*getitem		= NULL;
	DSPNode_t   DSPNode,*dspNode = NULL;
	DISPLAY_T 	DSPstruct_head;
	uint32_t CacheSize = 0;
	//计算在一个播放列表中总共有多少幕信息，如果只有一幕信息将避免
	//不断入屏出屏造成的闪屏现象
	uint8_t itemCount = 0;
	//当只有一幕信息时，播放完该募信息将该变量置1表示不要再切换播放
	int OneTimeFlag = 0;
	DSPrioritInit();
	DP_GetScreenSize(&ScrWith,&ScrHeight);

	CacheSize = ScrWith * ScrHeight * SCREEN_BPP;

	//syscrashlock,这个文件锁不存在，说明系统能正常运行，开机直接显示
	//关机前所显示的播放列表
	if(access(syscrashlock,F_OK) < 0)
	{ 
		DefaultLstDisplay();//DSPDefaultLst();
	}

	//syscrashlock这个文件锁的存在，说明系统不能正常显示开机前的播放列表，
	//(有可能播放列表损坏了)默认显示固定信息"珍爱生命 谨慎驾驶"
	else
	{
		DefmsgDisplay();
		remove(syscrashlock);
	}
	getitem = content.head;
	while(1)
	{
		usleep(5000);

		//播放列表有更新，就取链表的头结点，每一个节点就是一幕信息
		//每一幕信息还可能会包含多个显示元素的节点，如每一行就是一个显示元素节点
		pthread_mutex_lock(&content_lock);
		if(content.refresh == LST_REFLASH)
		{
			getitem = content.head;
			content.refresh = FLAG_NOT_REFRESH;
			itemCount = (content.itemcount == 0) ? 1 : content.itemcount;
			debug_printf("itemCount = %d\n",itemCount);
			OneTimeFlag = 0;
		}
		if(getitem == NULL)
		{
			getitem = content.head;
			pthread_mutex_unlock(&content_lock);
			continue;
		}
		memcpy(&itemnode,getitem,sizeof(XKDisplayItem));
		pthread_mutex_unlock(&content_lock);

		
		//初始化显示链表头，该链表就是一幕信息，每个节点就是一个显示元素
		DSP_INITstruct(&DSPstruct_head,0,0,0,0,0,0,0,0,0,0);
		DSP_LstInit(&DSPstruct_head.list);
		
		//记录当前幕入屏后要停留多长时间，如果只有1幕信息，为避免因为只刷屏一次
		//系统遇到错误跑出错误信息覆盖掉显示文字，这里指定停留1分钟后就要重新刷新信息
		DSPstruct_head.stoptime = (itemCount <= 1) ? (60000000) : itemnode.nDelayTime;
		
		//入屏与出屏动作，不同的协议对相同的入屏出屏动作的编号的定义不一样，所以在这里重新
		//对不同编号进行统一的编号
		debug_printf("getitem->nEffectIn = %d\n",getitem->nEffectIn);

		//获取显示效果的大类与方向
		SetEffectOption(getitem->nEffectIn,&Effect.inType,&Effect.inDire);
		SetEffectOption(getitem->nEffectOut,&Effect.outType,&Effect.outDire);

		//设置当前显示信息，为的是上位机可以获取当前显示信息
		//这里用的函数指针，因为每种协议具体的设置方法是不一样的
		if(itemnode.setcurplaying != NULL)
			itemnode.setcurplaying(itemnode.Curplay);

		//出入屏动作的速度
		Effect.speed = getitem->nMoveSpeed;
		
		//速度等级暂时定义20个级
		Effect.speed = (Effect.speed > 20) ? 1 : (21 - Effect.speed);

		//itemnode是上面取出来的一个节点，也就是一幕信息，里面包含多个显示元素节点
		//DSPNode_head指向的就是这些显示元素的头节点
		dspNode = itemnode.DSPNode_head;

		//初始化当前显示内容结构
		InitDSPContent();
		memset(CTTcache->cache,0,CacheSize);

		//解析itemnode里面包含的所有的显示元素的节点，在一幕信息里面有可能包含
		//图片、动画、文字、光带等等
		pthread_mutex_lock(&content_lock);
		while(content.refresh != LST_FREE && content.refresh != LST_REFLASH && dspNode != NULL)
		{
			memset(&DSPNode,0,sizeof(DSPNode));
			memcpy(&DSPNode,dspNode,sizeof(DSPNode_t));

			//针对光带的，光带优先级最高，当上位机设置光带颜色的时候，非光带的节点将被忽略掉
			GetDSPriorit(DSPTYPE_LBD,&priority);
			if(priority == 0 && DSPNode.type != DSPTYPE_LBD)
			{
				dspNode = DSPNode.pNext;
				continue;
			}
			DEBUG_PRINTF;
			//解析文字、图片、GIF、PNG、BMP
			CreateDspList(&itemnode,&DSPNode,&Effect,&DSPstruct_head.list);
			dspNode = DSPNode.pNext;
		}
		pthread_mutex_unlock(&content_lock);

		
		//经过上面的解析，解析完后得到的待显示的DSPstruct_head链表
		if(DSP_LstEmpty(&DSPstruct_head.list) == 0)
			goto GET_NEXTNODE;
		
		//清除数据缓存
		FrameCacheClear();
		//将每个节点的x，y，width，height等参数记录，在显示线程中用到
		DSPStructInit(&DSPstruct_head);
		//入屏动作
		FrameEnter(&DSPstruct_head);
		//停留时间
		FrameStay(&DSPstruct_head);
		//出屏动作
		FrameOut(&DSPstruct_head);
		//销毁资源
		DSP_LstDestroy(&DSPstruct_head);
		GET_NEXTNODE:
			getitem = itemnode.pNext;
			DEBUG_PRINTF;

	}
}





