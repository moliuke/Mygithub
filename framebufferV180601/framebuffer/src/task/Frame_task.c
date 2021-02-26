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
		{0x02,0x35,0x36,0x30,0x30,0x81,0x65,0x50,0x03}, //��ʾȫ��
		{0x02,0x35,0x36,0x30,0x30,0x82,0x55,0x33,0x03}, //��ʾȫ��
		{0x02,0x35,0x36,0x30,0x30,0x83,0x45,0x12,0x03}, //��ʾȫ��
		{0x02,0x35,0x36,0x30,0x30,0x84,0x35,0xF5,0x03}, //��ʾȫ��
		{0x02,0x35,0x36,0x30,0x30,0x85,0x25,0xD4,0x03}, //��ʾȫ��
		{0x02,0x35,0x36,0x30,0x30,0x86,0x15,0xB7,0x03}, //��ʾȫ��
		{0x02,0x35,0x36,0x30,0x30,0x87,0x05,0x96,0x03}, //��ʾȫ��
		{0x02,0x35,0x36,0x30,0x30,0x00,0xE4,0xF9,0x03}	//��ʾ�˳�����ģʽ
		};

static void CheckIfTestMode(XKCellString *Str)
{
	static int TestStatus = 0;
	uint8_t TestFlag; 
	uint8_t BrightV;
	//uint8_t SendBright[] = {0xD8,0x00,0x02,0x00,0x00,0x00,0x00,0xAA};
	//ȫ��

	//��ʾȫ��
	if(Str->nForeColor[0] == 255 && Str->nBkColor[0] == 255 && Str->nForeColor[1] == 0
	&& Str->nBkColor[1] == 0 && Str->nForeColor[2] == 0 && Str->nBkColor[2] == 0)
	{
		DP_SetTestMode(TEST_MODE);

		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,TestMode[0],9);
		pthread_mutex_unlock(&queue_uart_mutex);
	}
	//��ʾȫ��
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
	//��ɫ
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
//ȫ��
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

//������������ʾԪ��
static int STR_DSPDeal(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	int contentWidth = 0;
	XKCellString *str = DSPNode->XKCellStr;
	CTTsize_t CTTsize;
	Pcolor_t Bcolor,Fcolor;
	charattr_t charattr;
	TXTstruct_t TXTstruct;

	//��ʾ�������곬����Ļ��Χ
	if(DSPNode->Cx >= ScrWith || DSPNode->Cy >= ScrHeight)
		return -1;

	//����Ƿ�������ɫ�뱳��ɫ��ͬһ��ɫ����ɫ��ͬ�򽫲��Ա�־��1
	//detele by mo 20201120
	//CheckIfTestMode(str);
	
	//����һ����ʾԪ�صĽڵ�
	DISPLAY_T *dspnode = (DISPLAY_T *)malloc(sizeof(DISPLAY_T));
	memset(dspnode,0x00,sizeof(DISPLAY_T));
	memset(TXTCache,0,FONT_CACHE_SIZE);

	//�Ը������ַ��������ֿ��н���
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

	//����ʾ���ݳ���С����Ļ����ұ��û�ʹ�ù���ʱ��ǿ��ת��ֱ����ʾ��������ʾ
	contentWidth = strlen(str->strContent) * (str->nFontSize / 2);
	if(Effect->inType == EFFECT_RUNNINGHOST && contentWidth < ScrWith)
	{
		Effect->inType = EFFECT_DIRECT;
		CTTsize.cx = (ScrWith - contentWidth) / 2;
		CTTsize.cy = (ScrHeight - str->nFontSize) / 2;
	}

	//������ƣ���TXT_decoder��������������һ�������֣����뵽CTTcache������Ļ
	//��Сһ���Ļ����У�����ǿ�ƻ��л��߳�����Ļ��Ⱦ��Զ�����
	if(Effect->inType != EFFECT_RUNNINGHOST)
	{
		TXT_setEffect(&CTTsize,CTTcache,&TXTstruct);
		
	}
	//����ƣ���TXT_decoder��������������һ�������֣����뵽CTTcache������Ļ
	//��Сһ���Ļ����У�CTTcache��״�����Ǹ���Ļһ�������Ǹ߶�Ϊstr->nFontSize����Ȳ�ȷ����һ�黺��
	else
		TXT_setEffect_runhost(&CTTsize,CTTcache,&TXTstruct);

	//��䵱ǰ������ʾ���ݵ���Ϣ��������������ʽ��ͣ��ʱ��ȣ�����λ����ȡ��ǰ��ʾ����ʹ�õ�
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
	//������һ���������ϰ汾�ģ��������°汾�ģ����ڵ�����
	DP_SetCurPlayContent(str->strContent,strlen(str->strContent),str->nFontType,str->nFontSize,DSPsrcNode->nEffectIn,DSPsrcNode->nMoveSpeed,DSPsrcNode->nDelayTime,str->nCellOrder);

	//��ʼ��һ����ʾ�ڵ�dspnode��ָ���ýڵ���ʾ��������(ͼƬ�����֡�����)�����꣬��������ʽ���ٶȡ��������г�������ʽ�õ����ĸ������ӿ�
	DSP_INITstruct(dspnode,CTTsize.cx,CTTsize.cy,CTTsize.CTwidth,CTTsize.CTheight,TYPE_STR,Effect->inType,Effect->inDire,Effect->outType,Effect->outDire,Effect->speed);

	//����ʾ�ڵ�ѹ����DSPLstHeadΪ��ͷ������DSPLstHead��������������һĻ��Ϣ
	//ÿһĻ��Ϣ���ܰ����ܶ����ʾ�ڵ�
	DSP_LstInsert(dspnode,DSPLstHead);
}



static int IMG_DSPDeal(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	IMGstruct_t IMGstruct;
	XKCellImage *img = DSPNode->XKCellIma;

	//ͼƬ����ʾ���곬����Ļ�ķ�Χ
	if(DSPNode->Cx >= ScrWith || DSPNode->Cy >= ScrHeight)
		return -1;

	//����һ����ʾԪ�صĽڵ�
	DISPLAY_T *dspnode = (DISPLAY_T *)malloc(sizeof(DISPLAY_T));
	memset(dspnode,0x00,sizeof(DISPLAY_T));
	memset(IMGstruct.filename,0,sizeof(IMGstruct.filename));
	sprintf(IMGstruct.filename,"%s/%s",image_dir,img->strImage);
	debug_printf("IMGstruct->filename = %s\n",IMGstruct.filename);
	debug_printf("ScrWith = %d,ScrHeight = %d\n",ScrWith,ScrHeight);

	//��ʼ��ͼƬ�����ṹ�����߽���������ͼƬ�������ĸ����棬��������Ļ��ʲô����
	//�Լ�����ͼƬ�Ŀ����������Ļ�Ŀ��
	IMG_INITstruct(&IMGstruct,DSPNode->Cx,DSPNode->Cy,ScrWith,ScrHeight,CTTcache->cache);

	//��ʼ����ͼƬ
	if(IMGstruct.IMG_decoder(&IMGstruct) < 0)
	{
		free(dspnode);
		return -1;
	}

	//���õ�ǰ��ʾ������ΪͼƬ����������ʽ��ͣ��ʱ���
	DSPContent_t DSPContent;
	memset(&DSPContent,0,sizeof(DSPContent));
	DSPContent.type = DSPTYPE_IMG;
	DSPContent.inform = DSPsrcNode->nEffectIn;
	DSPContent.speed = DSPsrcNode->nMoveSpeed;
	DSPContent.staytime = DSPsrcNode->nDelayTime;
	DSPContent.maptype = img->type;
	memcpy(DSPContent.mapName,img->strImage,strlen(img->strImage));
	SetDSPContent(&DSPContent);

	//�������һ�����汾��һ��Ҳûɾ�������������itemnode.setcurplaying�����
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

	//��ʼ����ʾԪ�ؽڵ㲢������ʾ������
	DSP_INITstruct(dspnode,DSPNode->Cx,DSPNode->Cy,IMGstruct.ctwidth,IMGstruct.ctheight,TYPE_IMG,Effect->inType,Effect->inDire,Effect->outType,Effect->outDire,Effect->speed);
	DSP_LstInsert(dspnode,DSPLstHead);
}

static int LBD_DSPDeal(XKDisplayItem *DSPsrcNode,DSPNode_t *DSPNode,Effect_t *Effect,struct list_head *DSPLstHead)
{
	LIGHTBand_t *LIGHTBand = DSPNode->LIGHTBand;
	DISPLAY_T *dspnode = (DISPLAY_T *)malloc(sizeof(DISPLAY_T));
	memset(dspnode,0x00,sizeof(DISPLAY_T));

	//�������
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
	//��ʼ�������ʾԪ�ؽڵ㲢������ʾ������
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
	//���GIF�ļ��Ƿ����
	memset(gifpath,0,sizeof(gifpath));
	sprintf(gifpath,"%s/%s",image_dir,ani->strAnimate);
	debug_printf("gifpath = %s\n",gifpath);
	if(access(gifpath,F_OK) < 0)
	{
		return -1;
	}
	//��ʼ��GIF�ṹ������GIF�ļ����������Ǹ������Լ��ڻ����е����꣬������������Ļ�ķ�Χ��
	debug_printf("Effect.inType = %d,Effect.inDire = %d,Effect.outType = %d,Effect.outDire = %d\n",Effect->inType,Effect->inDire,Effect->outType,Effect->outDire);
	GIFstruct = (GIFstruct_t *)malloc(sizeof(GIFstruct_t));
	GIF_INITstruct(GIFstruct,gifpath,DSPNode->Cx,DSPNode->Cy,ScrWith,ScrHeight,CTTcache->cache);

	
	//��ʼ��gifͼƬ������gif����ȡgif���������ΪgifͼƬ�Ľ�������ڴ�
	GIFstruct->GIF_init(GIFstruct);
	debug_printf("GIFstruct->Fwidth = %d\n",GIFstruct->Fwidth);
	if(DSPNode->Cx > ScrWith || DSPNode->Cy > ScrHeight)
	{
		return -1;
	}
	//gif����Ч��ʾ���ݲ��ܳ�����Ļ�ı߿򡣳����򱻺��Ե�
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
	//��ʼ����̬��ʾ(������������������ʾ���ꡢgif��ߵ���Ϣ������Ҫ�ĳ�ʼ��GIFʹ���ĸ���������)
	DSP_INITstruct(dspnode,DSPNode->Cx,DSPNode->Cy,GIFstruct->ctwidth,GIFstruct->ctheight,TYPE_GIF,Effect->inType,Effect->inDire,Effect->outType,Effect->outDire,Effect->speed);
	dspnode->GIFstruct = GIFstruct;
	//��gif�ڵ�ѹ�������У�֡�ֽ��߳̽���Ӹ�������һ��һ���ڵ�ȡ������ʾ
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

	//����PNG
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
	//��ʼ��png��ʾԪ�ؽڵ㲢������ʾ������
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
		//��������Ԫ��
		case DSPTYPE_STR:
			ret = STR_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		//����ͼƬԪ��
		case DSPTYPE_IMG:
			ret = IMG_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		//�������Ԫ��
		case DSPTYPE_LBD:
			ret = LBD_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		//����GIFԪ��
		case DSPTYPE_GIF:
			ret = GIF_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		//����PNGԪ��
		case DSPTYPE_PNG:
			ret = PNG_DSPDeal(DSPsrcNode,DSPNode,Effect,DSPLstHead);
			break;
		default:
			break;
	}
	return ret;
}



//ͣ��ʱ�䣬ÿһĻ��Ϣ������Ҫͣ��һ����ʱ��
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

		//�����ֲ����б����ʱҪ��������ͣ��
		if(content.refresh == LST_REFLASH)
		{
			return 1;
		}
	}
	return 0;
}


static inline void FrameCacheClear(void)
{
	
	//1��������סDisplay_task�����������ˢ���ݵ�framebuffer��
	DSPcache->W_Pos = (DSPcache->R_Pos + 1) % 3;
	//2����������ʾ���涼��0
	memset(DSPcache->cache,0X00,DSPcache->CHwidth * DSPcache->CHheight * SCREEN_BPP*3);
	//3����д����ָ��ָ��W_posָ��Ļ���
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
	//������һ�������б����ܹ��ж���Ļ��Ϣ�����ֻ��һĻ��Ϣ������
	//��������������ɵ���������
	uint8_t itemCount = 0;
	//��ֻ��һĻ��Ϣʱ���������ļ��Ϣ���ñ�����1��ʾ��Ҫ���л�����
	int OneTimeFlag = 0;
	DSPrioritInit();
	DP_GetScreenSize(&ScrWith,&ScrHeight);

	CacheSize = ScrWith * ScrHeight * SCREEN_BPP;

	//syscrashlock,����ļ��������ڣ�˵��ϵͳ���������У�����ֱ����ʾ
	//�ػ�ǰ����ʾ�Ĳ����б�
	if(access(syscrashlock,F_OK) < 0)
	{ 
		DefaultLstDisplay();//DSPDefaultLst();
	}

	//syscrashlock����ļ����Ĵ��ڣ�˵��ϵͳ����������ʾ����ǰ�Ĳ����б�
	//(�п��ܲ����б�����)Ĭ����ʾ�̶���Ϣ"�䰮���� ������ʻ"
	else
	{
		DefmsgDisplay();
		remove(syscrashlock);
	}
	getitem = content.head;
	while(1)
	{
		usleep(5000);

		//�����б��и��£���ȡ�����ͷ��㣬ÿһ���ڵ����һĻ��Ϣ
		//ÿһĻ��Ϣ�����ܻ���������ʾԪ�صĽڵ㣬��ÿһ�о���һ����ʾԪ�ؽڵ�
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

		
		//��ʼ����ʾ����ͷ�����������һĻ��Ϣ��ÿ���ڵ����һ����ʾԪ��
		DSP_INITstruct(&DSPstruct_head,0,0,0,0,0,0,0,0,0,0);
		DSP_LstInit(&DSPstruct_head.list);
		
		//��¼��ǰĻ������Ҫͣ���೤ʱ�䣬���ֻ��1Ļ��Ϣ��Ϊ������Ϊֻˢ��һ��
		//ϵͳ���������ܳ�������Ϣ���ǵ���ʾ���֣�����ָ��ͣ��1���Ӻ��Ҫ����ˢ����Ϣ
		DSPstruct_head.stoptime = (itemCount <= 1) ? (60000000) : itemnode.nDelayTime;
		
		//�����������������ͬ��Э�����ͬ���������������ı�ŵĶ��岻һ������������������
		//�Բ�ͬ��Ž���ͳһ�ı��
		debug_printf("getitem->nEffectIn = %d\n",getitem->nEffectIn);

		//��ȡ��ʾЧ���Ĵ����뷽��
		SetEffectOption(getitem->nEffectIn,&Effect.inType,&Effect.inDire);
		SetEffectOption(getitem->nEffectOut,&Effect.outType,&Effect.outDire);

		//���õ�ǰ��ʾ��Ϣ��Ϊ������λ�����Ի�ȡ��ǰ��ʾ��Ϣ
		//�����õĺ���ָ�룬��Ϊÿ��Э���������÷����ǲ�һ����
		if(itemnode.setcurplaying != NULL)
			itemnode.setcurplaying(itemnode.Curplay);

		//�������������ٶ�
		Effect.speed = getitem->nMoveSpeed;
		
		//�ٶȵȼ���ʱ����20����
		Effect.speed = (Effect.speed > 20) ? 1 : (21 - Effect.speed);

		//itemnode������ȡ������һ���ڵ㣬Ҳ����һĻ��Ϣ��������������ʾԪ�ؽڵ�
		//DSPNode_headָ��ľ�����Щ��ʾԪ�ص�ͷ�ڵ�
		dspNode = itemnode.DSPNode_head;

		//��ʼ����ǰ��ʾ���ݽṹ
		InitDSPContent();
		memset(CTTcache->cache,0,CacheSize);

		//����itemnode������������е���ʾԪ�صĽڵ㣬��һĻ��Ϣ�����п��ܰ���
		//ͼƬ�����������֡�����ȵ�
		pthread_mutex_lock(&content_lock);
		while(content.refresh != LST_FREE && content.refresh != LST_REFLASH && dspNode != NULL)
		{
			memset(&DSPNode,0,sizeof(DSPNode));
			memcpy(&DSPNode,dspNode,sizeof(DSPNode_t));

			//��Թ���ģ�������ȼ���ߣ�����λ�����ù����ɫ��ʱ�򣬷ǹ���Ľڵ㽫�����Ե�
			GetDSPriorit(DSPTYPE_LBD,&priority);
			if(priority == 0 && DSPNode.type != DSPTYPE_LBD)
			{
				dspNode = DSPNode.pNext;
				continue;
			}
			DEBUG_PRINTF;
			//�������֡�ͼƬ��GIF��PNG��BMP
			CreateDspList(&itemnode,&DSPNode,&Effect,&DSPstruct_head.list);
			dspNode = DSPNode.pNext;
		}
		pthread_mutex_unlock(&content_lock);

		
		//��������Ľ������������õ��Ĵ���ʾ��DSPstruct_head����
		if(DSP_LstEmpty(&DSPstruct_head.list) == 0)
			goto GET_NEXTNODE;
		
		//������ݻ���
		FrameCacheClear();
		//��ÿ���ڵ��x��y��width��height�Ȳ�����¼������ʾ�߳����õ�
		DSPStructInit(&DSPstruct_head);
		//��������
		FrameEnter(&DSPstruct_head);
		//ͣ��ʱ��
		FrameStay(&DSPstruct_head);
		//��������
		FrameOut(&DSPstruct_head);
		//������Դ
		DSP_LstDestroy(&DSPstruct_head);
		GET_NEXTNODE:
			getitem = itemnode.pNext;
			DEBUG_PRINTF;

	}
}





