#include "content.h"
#include "display.h"
#include "Hardware/Data_pool.h"


dsp_content_t *dsp_content_fb = NULL;
static int Screen_BPP = SCREEN_BPP;

static uint32_t frame_width = 0;
static uint32_t frame_height = 0;

DSPCNT_T DSPcnt;

sem_t sem;

pthread_mutex_t dsp_mutex;


void dsp_frame_size_set(uint32_t width,uint32_t height)
{
	frame_width 	= width;
	frame_height 	= height;
}

dsp_content_t *dsp_content_bufExtendMalloc(void)
{
	int i = 0;
	dsp_content_t *newnode = NULL;
	dsp_content_t *cur_node = NULL,*next_node = NULL;

	cur_node = dsp_content_fb->fb_next;
	next_node = cur_node->fb_next;
	
	while(cur_node->fb_next != NULL)
	{
		cur_node = cur_node->fb_next;
		next_node = cur_node->fb_next;
	}

	if(cur_node->fb_next != NULL)
		return NULL;
	
	newnode = (dsp_content_t *)malloc(sizeof(dsp_content_t));
	if(newnode == NULL)
	{
		perror("extend malloc dsp_content_fb:");
		return NULL;
	}
	memset(newnode,0,sizeof(dsp_content_t));

	newnode->dsp_buffer = (uint8_t *)malloc(frame_width * frame_height * 4);
	if(newnode->dsp_buffer == NULL)
	{
		perror("extend malloc dsp_content_fb->dsp_buffer:");
		free(newnode);
		return NULL;
	}
	memset(newnode->dsp_buffer,0,frame_width * frame_height * 4);
	newnode->fb_next = NULL;

	cur_node->fb_next = newnode;

	return newnode;
}

void dsp_content_bufeExtendFree(void)
{
	int i = 0;
	dsp_content_t *newnode = NULL;
	dsp_content_t *cur_node = NULL,*next_node = NULL;
	if(dsp_content_fb == NULL)
		DSP_DEBBUG_PRINTF;

	cur_node = dsp_content_fb->fb_next->fb_next;

	while(cur_node != NULL)
	{
		next_node = cur_node->fb_next;
		free(cur_node->dsp_buffer);
		free(cur_node);
		cur_node = next_node;
	}
}

void dsp_frame_size_get(uint32_t *width,uint32_t *height)
{
	*width  = frame_width;
	*height = frame_height;
}


 
enum
{
	STATE_START 	= 0,
	STATE_MEM_CHECK,
	STATE_CHAR_CHECK,
	STATE_WRITE_FB,
	STATE_END
};


#define WRITE_DATA		10
#define WRITE_BALANCE	11
#define CLEAR_DATA		20
#define CLEAR_BALANCE	21
#define HORIZONTAL		0
#define VERTICAL		1

static int DSP_runhost_write(int Wx,int Wy,int Rx,int Ry,int width,int height,int Flag)
{
	int h = 0,w = 0;
	//uint8_t *readAddr = CTTcache->cache + CTTcache->CHwidth * CTTcache->CHheight * 4;
	uint8_t *readAddr = CTTcache->cache;
	uint32_t CACHE_width = CTTcache->CHwidth * CTTcache->CHheight * 3 / height;
	int ClineBytes = CACHE_width * 4;
	int SlineBytes = CTTcache->CHwidth * 4;

	int Wypos = Wy * SlineBytes + Wx * 4;
	int Rypos = Ry * ClineBytes + Rx * 4;
	int Wwidth = width * 4;
	char tmpbuf[2560];


	switch(Flag)
	{
		case WRITE_DATA:
			for(h = 0 ; h < height ; h++)
			{
				memcpy(DSPcache->W_Addr+ Wypos,readAddr + Rypos,Wwidth);
				Wypos += SlineBytes;
				Rypos += ClineBytes;
			}
			break;
		case WRITE_BALANCE:
			for(h = 0 ; h < height ; h++)
			{
				memcpy(tmpbuf,readAddr + Rypos,Wwidth);
				Wypos += SlineBytes;
				Rypos += ClineBytes;
			}
			break;
		case CLEAR_DATA:
			for(h = 0 ; h < height ; h++)
			{
				memset(DSPcache->W_Addr+ Wypos,0x00,Wwidth);
				Wypos += SlineBytes;
			}
			break;
		case CLEAR_BALANCE:
			for(h = 0 ; h < height ; h++)
			{
				memset(tmpbuf,0x00,Wwidth);
				Wypos += SlineBytes;
			}
			break;
	}

	return 0;
}

static void DSP_writeData(int Wx,int Wy,int Rx,int Ry,int width,int height,int framecnt,int direction,int Flag)
{
	int h = 0,w = 0;
	int ClineBytes = DSPcache->CHwidth * 4;
	int Wwidth = width * 4;
	int HtmpSize = 0;
	int txpos = 0;
	int WRpos = 0;
	int write_tmp_pos = 0,read_tmp_pos = 0;
	int write_pos = 0 ,read_pos = 0;
	int VtmpSize = Wwidth;
	char tmpbuf[2560];

	int Wypos = Wy * ClineBytes + Wx * 4;
	int Rypos = Ry * ClineBytes + Rx * 4;
	
	if(Flag == WRITE_DATA)
	{
		int Wypos = Wy * ClineBytes + Wx * 4;
		int Rypos = Ry * ClineBytes + Rx * 4;
		for(h = 0 ; h < height ; h++)
		{
			memcpy(DSPcache->W_Addr+ Wypos,CTTcache->cache + Rypos,Wwidth);
			Wypos += ClineBytes;
			Rypos += ClineBytes;
		}

		if(direction == HORIZONTAL)
		{
			HtmpSize = (framecnt - width) * 4;
			for(h = 0 ; h < height ; h++)
				memcpy(tmpbuf,CTTcache->cache,HtmpSize);

		}
		else
		{
			HtmpSize = framecnt * 4;
			for(h = height ; h < framecnt ; h++)
				memcpy(tmpbuf,CTTcache->cache,HtmpSize);
		}
	}

	else
	{
		int Wypos = Wy * ClineBytes + Wx * 4;
		for(h = 0 ; h < height ; h++)
		{
			memset(DSPcache->W_Addr + Wypos,0x00,Wwidth);
			memset(tmpbuf,0x00,HtmpSize);
			Wypos += ClineBytes;
		}
		
		if(direction == HORIZONTAL)
		{
			HtmpSize = (framecnt - width) * 4;
			for(h = 0 ; h < height ; h++)
				memset(tmpbuf,0x00,HtmpSize);

		}
		else
		{
			HtmpSize = framecnt * 4;
			for(h = height ; h < framecnt ; h++)
				memset(tmpbuf,0x00,HtmpSize);
		}
		
	}
}



static inline void DSP_write(uint8_t *desBuffer,uint32_t writePos,uint8_t *srcBuffer,uint32_t readPos)
{
	if(srcBuffer != NULL)
	{
		desBuffer[writePos + BGR_B] = srcBuffer[readPos + BGR_B];
		desBuffer[writePos + BGR_G] = srcBuffer[readPos + BGR_G];
		desBuffer[writePos + BGR_R] = srcBuffer[readPos + BGR_R];
	}
	else
	{
		desBuffer[writePos + BGR_B] = 0x00;
		desBuffer[writePos + BGR_G] = 0x00;
		desBuffer[writePos + BGR_R] = 0x00;
	}
}
void DSP_LstInit(struct list_head *DSPhead)
{
	INIT_LIST_HEAD(DSPhead);
}


int DSP_LstInsert(DISPLAY_T *dspnode,struct list_head *DSPhead)
{
	list_add(&(dspnode->list),DSPhead);
}

int DSP_LstFetch(DISPLAY_T **dspnode,struct list_head *pos,struct list_head *DSPhead)
{
	DISPLAY_T *DSPnode = NULL;
	
	if(list_empty(DSPhead))
		return -1;
	
	DSPnode = list_entry(pos,DISPLAY_T,list);
	*dspnode = DSPnode;
	
	return 0;
}

int DSP_LstEmpty(struct list_head *DSPhead)
{
	if(list_empty(DSPhead))
		return 0;
	
	return 1;
}


void DSP_LstDestroy(DISPLAY_T *DSPLstHead)
{
	DISPLAY_T *dspnode = NULL;
	struct list_head *pos = NULL;
	if(list_empty(&DSPLstHead->list))
		return;
	list_for_each(pos,&DSPLstHead->list)
	{
		dspnode = list_entry(pos,DISPLAY_T,list);
		if(dspnode != NULL)
		{
			free(dspnode);
			dspnode = NULL;	
		}
	}
	
}


static int EFT_fill(DISPLAY_T *dspnode,uint8_t IOmode)
{
	uint8_t *writeAddr = NULL;
	int i = 0,h = 0,w = 0;
	uint32_t write_pos = 0;
	uint32_t write_pos_tmp = 0;
	
	for(i = 0 ; i < DSPcache->blockCNT ; i ++)
	{
		writeAddr = DSPcache->cache + i * DSPcache->CHwidth * DSPcache->CHheight * 4;
		for(h = 0 ; h < dspnode->height ; h ++)
		{
			write_pos_tmp = (h + dspnode->cy) * DSPcache->CHwidth * 4;
			for(w = 0 ; w < dspnode->width ; w ++)
			{
				write_pos = write_pos_tmp + (w + dspnode->cx) * 4;
				if(IOmode == EFF_IN)
					DSP_write(writeAddr,write_pos,CTTcache->cache,write_pos);
				else
					DSP_write(writeAddr,write_pos,NULL,0);
			}
		}
	}

	
	return 0;
}

static int EFT_oppsize_leftright(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;
	
	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);
	
	WXpos 		= dspnode->cx + effect->Fcurls;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + effect->Fcurls;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Ftotal - effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);

	WXpos 		= dspnode->cx + dspnode->width - effect->Fcurls;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + dspnode->width - effect->Fcurls;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);

	WXpos 		= dspnode->cx + effect->Ftotal;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + effect->Ftotal;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Ftotal - effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);
	effect->Forder += 1;
	return 0;
}

static int EFT_oppsize_updown(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + effect->Ftotal - effect->Fcurls;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + effect->Ftotal - effect->Fcurls;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Ftotal - effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + effect->Ftotal;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + effect->Ftotal;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);
	
	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + effect->Ftotal + effect->Fcurls;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + effect->Ftotal + effect->Fcurls;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Ftotal - effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);
	effect->Forder += 1;
	return 0;
}

static int EFT_oppsize_in(DISPLAY_T *dspnode)
{
	switch(dspnode->effectIn.direction)
	{
		case DIRECTION_LFRI:
			EFT_oppsize_leftright(dspnode,EFF_IN);
			
			break;
		case DIRECTION_UPDW:
			EFT_oppsize_updown(dspnode,EFF_IN);
			break;
		default:
			break;
	}
	return 0;
}


static int EFT_oppsize_out(DISPLAY_T *dspnode)
{
	switch(dspnode->effectOut.direction)
	{
		case DIRECTION_LFRI:
			EFT_oppsize_leftright(dspnode,EFF_OUT);
			
			break;
		case DIRECTION_UPDW:
			EFT_oppsize_updown(dspnode,EFF_OUT);
			break;
		default:
			break;
	}
	return 0;
}




static int EFT_windows_leftright(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;
	int b = 0;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;

	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;
	
	for(b = 0 ; b < effect->Blocks ; b++)
	{
		WXpos		= dspnode->cx + b * effect->Psize;
		WYpos		= dspnode->cy;
		RXpos		= dspnode->cx + b * effect->Psize;
		RYpos		= dspnode->cy;
		Xwidth		= effect->Fcurls;
		Yheight 	= dspnode->height;
		DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);

		WXpos		= dspnode->cx + b * effect->Psize + effect->Fcurls;
		WYpos		= dspnode->cy;
		RXpos		= dspnode->cx + b * effect->Psize + effect->Fcurls;
		RYpos		= dspnode->cy;
		Xwidth		= effect->Ftotal - effect->Fcurls;
		Yheight 	= dspnode->height;
		DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);
	}
	effect->Forder++;
	return 0;
	
}
static int EFT_windows_updown(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;
	int b = 0;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;

	
	for(b = 0 ; b < effect->Blocks; b++)
	{
		WXpos		= dspnode->cx;
		WYpos		= dspnode->cy + b * effect->Psize;
		RXpos		= dspnode->cx;
		RYpos		= dspnode->cy + b * effect->Psize;
		Xwidth		= dspnode->width;
		Yheight 	= effect->Psize;
		DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);

		WXpos		= dspnode->cx;
		WYpos		= dspnode->cy + b * effect->Psize + effect->Fcurls;
		RXpos		= dspnode->cx;
		RYpos		= dspnode->cy + b * effect->Psize + effect->Fcurls;
		Xwidth		= dspnode->width;
		Yheight 	= effect->Psize - effect->Fcurls;
		DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);
	}
	
	effect->Forder++;
	return 0;
}


static int EFT_windows_in(DISPLAY_T *dspnode)
{
	switch(dspnode->effectIn.direction)
	{
		case DIRECTION_LFRI:
			EFT_windows_leftright(dspnode,EFF_IN);
			
			break;
		case DIRECTION_UPDW:
			EFT_windows_updown(dspnode,EFF_IN);
			break;
		default:
			break;
	}
}

static int EFT_windows_out(DISPLAY_T *dspnode)
{
	switch(dspnode->effectOut.direction)
	{
		case DIRECTION_LFRI:
			EFT_windows_leftright(dspnode,EFF_OUT);
			break;
		case DIRECTION_UPDW:
			EFT_windows_updown(dspnode,EFF_OUT);
			break;
		default:
			break;
	}
}


static int EFT_direct_in(DISPLAY_T *dspnode)
{
	EFFECT_T *effect = NULL;
	effect = &dspnode->effectIn;
	if(effect->endFlag)
		return 0;
	EFT_fill(dspnode,EFF_IN);
	//DSP_writeData(dspnode->cx,dspnode->cy,dspnode->cx,dspnode->cy,dspnode->width,dspnode->height,dspnode->width,HORIZONTAL,WRITE_DATA);
	effect->endFlag = 1;
	
	return 0;
}



static int EFT_direct_out(DISPLAY_T *dspnode)
{
	EFFECT_T *effect = NULL;
	DEBUG_PRINTF;
	effect = &dspnode->effectOut;
	if(effect->endFlag)
		return 0;

	effect->endFlag = 1;
	return 0;
}



static int EFT_spread_left(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;

	WXpos 		= dspnode->cx + dspnode->width - effect->Fcurls;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + dspnode->width - effect->Fcurls;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width - effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);
	
	effect->Forder += 1;
	return 0;
}

static int EFT_spread_right(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);

	WXpos 		= dspnode->cx + effect->Fcurls;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + effect->Fcurls;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width - effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);
	effect->Forder += 1;
	return 0;
}

static int EFT_spread_up(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;
	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + dspnode->height - effect->Fcurls;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + dspnode->height - effect->Fcurls;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width;
	Yheight 	= dspnode->height - effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);
	
	effect->Forder += 1;
	return 0;
}

static int EFT_spread_down(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + effect->Fcurls;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + effect->Fcurls;
	Xwidth 		= dspnode->width;
	Yheight 	= dspnode->height - effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);
	
	effect->Forder += 1;
	return 0;
}



static int EFT_spread_leftright(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}

	effect->Fcurls = effect->Forder * effect->Fstep;

	WXpos 		= dspnode->cx + effect->Ftotal - effect->Fcurls;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + effect->Ftotal - effect->Fcurls;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Ftotal - effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);

	WXpos 		= dspnode->cx + effect->Ftotal;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + effect->Ftotal;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);

	WXpos 		= dspnode->cx + effect->Ftotal + effect->Fcurls;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + effect->Ftotal + effect->Fcurls;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Ftotal - effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);
	
	effect->Forder += 1;
	return 0;
}

static int EFT_spread_upddown(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}

	effect->Fcurls = effect->Forder * effect->Fstep;
	
	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + effect->Ftotal - effect->Fcurls;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + effect->Ftotal - effect->Fcurls;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Ftotal - effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + effect->Ftotal;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + effect->Ftotal;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + effect->Ftotal + effect->Fcurls;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + effect->Ftotal + effect->Fcurls;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Ftotal - effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);
	effect->Forder += 1;
	return 0;
}


static int EFT_spread_in(DISPLAY_T *dspnode)
{
	switch(dspnode->effectIn.direction)
	{
		case DIRECTION_LEFT:
			EFT_spread_left(dspnode,EFF_IN);
			break;
		case DIRECTION_RIGHT:
			EFT_spread_right(dspnode,EFF_IN);
			break;
		case DIRECTION_UP:
			EFT_spread_up(dspnode,EFF_IN);
			break;
		case DIRECTION_DOWN:
			EFT_spread_down(dspnode,EFF_IN);
			break;
		case DIRECTION_LFRI:
			EFT_spread_leftright(dspnode,EFF_IN);
			break;
		case DIRECTION_UPDW:
			EFT_spread_upddown(dspnode,EFF_IN);
			break;
		default:
			break;
	}
}

static int EFT_spread_out(DISPLAY_T *dspnode)
{
	switch(dspnode->effectOut.direction)
	{
		case DIRECTION_LEFT:
			EFT_spread_left(dspnode,EFF_OUT);
			break;
		case DIRECTION_RIGHT:
			EFT_spread_right(dspnode,EFF_OUT);
			break;
		case DIRECTION_UP:
			EFT_spread_up(dspnode,EFF_OUT);
			break;
		case DIRECTION_DOWN:
			EFT_spread_down(dspnode,EFF_OUT);
			break;
		case DIRECTION_LFRI:
			EFT_spread_leftright(dspnode,EFF_OUT);
			break;
		case DIRECTION_UPDW:
			DSP_DEBBUG_PRINTF;
			EFT_spread_upddown(dspnode,EFF_OUT);
			break;
		default:
			break;
	}
}


static int EFF_move_up(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}

	effect->Fcurls = effect->Forder * effect->Fstep;

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + dspnode->height - effect->Fcurls;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,dspnode->height,VERTICAL,WriteFlag);
	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + effect->Fcurls;
	Xwidth 		= dspnode->width;
	Yheight 	= dspnode->height - effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,dspnode->height,VERTICAL,ClearFlag);
	effect->Forder += 1;
	return 0;
	
}

static int EFF_move_down(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;
	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy + dspnode->height - effect->Fcurls;
	Xwidth 		= dspnode->width;
	Yheight 	= effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,dspnode->height,VERTICAL,WriteFlag);

	effect->Fcurls = effect->Forder * effect->Fstep;
	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy + effect->Fcurls;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width;
	Yheight 	= dspnode->height - effect->Fcurls;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,dspnode->height,VERTICAL,ClearFlag);
	effect->Forder += 1;
	//usleep(100000);
	return 0;
	
}

static int EFF_move_right(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;
	
	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + dspnode->width - effect->Fcurls;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,dspnode->width,HORIZONTAL,WriteFlag);

	WXpos 		= dspnode->cx + effect->Fcurls;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width - effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,dspnode->width,HORIZONTAL,ClearFlag);
	effect->Forder += 1;
	return 0;
}


static int EFF_move_left(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;

	WXpos 		= dspnode->cx + dspnode->width - effect->Fcurls;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx;
	RYpos 		= dspnode->cy;
	Xwidth 		= effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,dspnode->width,HORIZONTAL,WriteFlag);

	WXpos 		= dspnode->cx;
	WYpos 		= dspnode->cy;
	RXpos 		= dspnode->cx + effect->Fcurls;
	RYpos 		= dspnode->cy;
	Xwidth 		= dspnode->width - effect->Fcurls;
	Yheight 	= dspnode->height;
	DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,dspnode->width,HORIZONTAL,ClearFlag);

	effect->Forder += 1;
	return 0;
}







static int EFT_move_in(DISPLAY_T *dspnode)
{
	switch(dspnode->effectIn.direction)
	{
		case DIRECTION_LEFT:
			EFF_move_left(dspnode,EFF_IN);
			break;
		case DIRECTION_RIGHT:
			EFF_move_right(dspnode,EFF_IN);
			break;
		case DIRECTION_UP:
			EFF_move_up(dspnode,EFF_IN);
			break;
		case DIRECTION_DOWN:
			EFF_move_down(dspnode,EFF_IN);
			break;
		default:
			break;
	}
}


static int EFT_move_out(DISPLAY_T *dspnode)
{
	switch(dspnode->effectOut.direction)
	{
		case DIRECTION_LEFT:
			EFF_move_left(dspnode,EFF_OUT);
			break;
		case DIRECTION_RIGHT:
			EFF_move_right(dspnode,EFF_OUT);
			break;
		case DIRECTION_UP:
			EFF_move_up(dspnode,EFF_OUT);
			break;
		case DIRECTION_DOWN:
			EFF_move_down(dspnode,EFF_OUT);
			break;
		default:
			break;
	}
}


static int EFT_stagger_leftright(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;
	int b = 0;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;
	
	for(b = 0 ; b < effect->Blocks ; b ++)
	{
		//从左往右扫描部分
		if(b % 2 == 0)
		{
			WXpos		= dspnode->cx;
			WYpos		= dspnode->cy + b * effect->Psize;
			RXpos		= dspnode->cx;
			RYpos		= dspnode->cy + b * effect->Psize;
			Xwidth		= effect->Fcurls;
			Yheight 	= effect->Psize;
			DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);

			WXpos		= dspnode->cx + effect->Fcurls;
			WYpos		= dspnode->cy + b * effect->Psize;
			RXpos		= dspnode->cx + effect->Fcurls;
			RYpos		= dspnode->cy + b * effect->Psize;
			Xwidth		= dspnode->width - effect->Fcurls;
			Yheight 	= effect->Psize;
			DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);

		}//if(piece_order % 2 == 0)
		else
		{
			WXpos		= dspnode->cx + dspnode->width - effect->Fcurls;
			WYpos		= dspnode->cy + b * effect->Psize;
			RXpos		= dspnode->cx + dspnode->width - effect->Fcurls;
			RYpos		= dspnode->cy + b * effect->Psize;
			Xwidth		= effect->Fcurls;
			Yheight 	= effect->Psize;
			DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,WriteFlag);

			WXpos		= dspnode->cx;
			WYpos		= dspnode->cy + b * effect->Psize;
			RXpos		= dspnode->cx;
			RYpos		= dspnode->cy + b * effect->Psize;
			Xwidth		= dspnode->width - effect->Fcurls;
			Yheight 	= effect->Psize;
			DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,HORIZONTAL,ClearFlag);
		}
	}
	effect->Forder += 1;
	return 0;
}


static int EFT_stagger_updown(DISPLAY_T *dspnode,uint8_t IOmode)
{
	EFFECT_T *effect = NULL;
	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;
	int b = 0;

	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	
	if(effect->endFlag)
		return 0;
	
	if(effect->Forder > effect->Ftotal)
	{
		EFT_fill(dspnode,IOmode);
		effect->endFlag = 1;
		return 0;
	}
	
	effect->Fcurls = effect->Forder * effect->Fstep;

	
	for(b = 0 ; b < effect->Blocks ; b ++)
	{
		//从左往右扫描部分
		if(b % 2 == 0)
		{
			WXpos		= dspnode->cx + b * effect->Psize;
			WYpos		= dspnode->cy;
			RXpos		= dspnode->cx + b * effect->Psize;
			RYpos		= dspnode->cy;
			Xwidth		= effect->Psize;
			Yheight 	= effect->Fcurls;
			DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);

			WXpos		= dspnode->cx + b * effect->Psize;
			WYpos		= dspnode->cy + effect->Fcurls;
			RXpos		= dspnode->cx + b * effect->Psize;
			RYpos		= dspnode->cy + effect->Fcurls;
			Xwidth		= effect->Psize;
			Yheight 	= dspnode->height - effect->Fcurls;
			DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);
		}//if(piece_order % 2 == 0)
		else
		{
			WXpos		= dspnode->cx + b * effect->Psize;
			WYpos		= dspnode->cy + dspnode->height - effect->Fcurls;
			RXpos		= dspnode->cx + b * effect->Psize;
			RYpos		= dspnode->cy + dspnode->height - effect->Fcurls;
			Xwidth		= effect->Psize;
			Yheight 	= effect->Fcurls;
			DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,WriteFlag);

			WXpos		= dspnode->cx + b * effect->Psize;
			WYpos		= dspnode->cy;
			RXpos		= dspnode->cx + b * effect->Psize;
			RYpos		= dspnode->cy;
			Xwidth		= effect->Psize;
			Yheight 	= dspnode->height - effect->Fcurls;
			DSP_writeData(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,effect->Ftotal,VERTICAL,ClearFlag);
		}
	}
	
	effect->Forder += 1;
	return 0;
}

static int EFT_stagger_in(DISPLAY_T *dspnode)
{
	switch(dspnode->effectIn.direction)
	{
		case DIRECTION_LFRI:
			EFT_stagger_leftright(dspnode,EFF_IN);
			break;
		case DIRECTION_UPDW:
			EFT_stagger_updown(dspnode,EFF_IN);
			break;
		default:
			break;
	}
	return 0;
}

static int EFT_stagger_out(DISPLAY_T *dspnode)
{
	switch(dspnode->effectOut.direction)
	{
		case DIRECTION_LFRI:
			EFT_stagger_leftright(dspnode,EFF_OUT);
			
			break;
		case DIRECTION_UPDW:
			EFT_stagger_updown(dspnode,EFF_OUT);
			break;
		default:
			break;
	}
	return 0;
}




enum{RUN_INIT = 0,RUN_STATE_1,RUN_STATE_2,RUN_STATE_3,RUN_STATE_4,RUN_STATE_5,RUN_STATE_6,RUN_STATE_7};
static int EFT_runhost_left(DISPLAY_T *dspnode,uint8_t IOmode)
{
	int h,w;
	static int WRemaind = 0;
	static int EnterRmaind = 0;
	static int RmaindWidth = 0;
	static int SpaceRemaind = 0;
	static int ReadPos = 0;
	static int state = RUN_INIT;
	static int EnterFrame = 0;
	EFFECT_T *effect = NULL;
	effect = &dspnode->effectIn;

	static int leftRemaind = 0,rightRemaind = 0;

	int WXpos = 0,WYpos = 0,RXpos = 0,RYpos = 0;
	int Xwidth = 0,Yheight = 0;
	int WriteFlag = -1,ClearFlag = -1;
	int b = 0;
	int stepos = 0;
	WriteFlag = (IOmode == EFF_IN) ? WRITE_DATA : CLEAR_DATA;
	ClearFlag = (IOmode == EFF_IN) ? CLEAR_DATA : WRITE_DATA;
	effect	  = (IOmode == EFF_IN) ? &dspnode->effectIn : &dspnode->effectOut;	
	if(effect->endFlag)
		return 0;
	
	if(content.refresh == LST_REFLASH)
	{
		effect->endFlag = 1;
		return 0;
	}
	switch(state)
	{
		//初始化各个全局变量
		case RUN_INIT:
			//文字开始从右往左滚动时，文字的左边是黑的状态，EnterRmaind描述的是文字左边
			//黑屏的宽度，随着文字的滚动，黑屏的宽度会逐渐减小
			EnterRmaind = CTTcache->CHwidth;
			//RmaindWidth描述的是文字串的总长度，即文字剩余要滚动的总长度
			RmaindWidth = dspnode->width;
			//前一次滚动完与下一次滚动的开始要间隔4个文字的宽度
			SpaceRemaind = 4 * dspnode->height;
			//从文字串缓存中读取文字的位置，随着文字的滚动，读取的位置会逐渐向后移动
			ReadPos = 0;
			state = RUN_STATE_1;

		//第一阶段，文字刚开始从右边出现向左边滚动，文字的左边都是黑屏的，一直到文字滚动到最左边为止
		case RUN_STATE_1:
			WXpos		= DSPcache->CHwidth - EnterFrame;
			WYpos		= dspnode->cy;
			RXpos		= 0;
			RYpos		= 0;
			Xwidth		= EnterFrame;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_DATA);
			Xwidth		= DSPcache->CHwidth - EnterFrame;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_BALANCE);
			
			WXpos		= 0;
			WYpos		= dspnode->cy;
			RXpos		= 0;
			RYpos		= 0;
			Xwidth		= DSPcache->CHwidth - EnterFrame;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_DATA);
			Xwidth		= EnterFrame;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_BALANCE);
			
			stepos = (EnterFrame + dspnode->effectIn.Fstep < DSPcache->CHwidth) ? dspnode->effectIn.Fstep : (DSPcache->CHwidth - EnterFrame);
			EnterFrame += stepos;
			//EnterFrame = (EnterFrame < DSPcache->CHwidth) ? EnterFrame : DSPcache->CHwidth;
			//当文字滚动到屏幕最左边时，切换到下一个状态。
			if(EnterFrame == DSPcache->CHwidth)
			{
				state = RUN_STATE_2;
			}
			break;

		//第二阶段，从文字滚动到屏幕最左边开始，到文字串的最后一个文字出现在屏幕的最右边结束
		case RUN_STATE_2:
			WXpos		= 0;
			WYpos		= dspnode->cy;
			RXpos		= ReadPos;
			RYpos		= 0;
			Xwidth		= DSPcache->CHwidth;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_DATA);
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_BALANCE);
			stepos = (RmaindWidth > DSPcache->CHwidth + dspnode->effectIn.Fstep) ? dspnode->effectIn.Fstep : (RmaindWidth - DSPcache->CHwidth);
			ReadPos += stepos;
			RmaindWidth -= stepos;
			//RmaindWidth = (RmaindWidth < DSPcache->CHwidth) ? DSPcache->CHwidth : RmaindWidth;
			//当文字串的最后一个文字出现在屏幕的最右边时，切换到下一个状态
			if(RmaindWidth == DSPcache->CHwidth)
				state = RUN_STATE_3;
			break;
		//第三阶段，从文字串的最后一个文字出现在屏幕的最右边开始，此后开始出现一段黑屏，直到黑屏的宽度
		//有4个文字的宽度为止
		case RUN_STATE_3:
			WXpos		= 0;
			WYpos		= dspnode->cy;
			RXpos		= ReadPos;
			RYpos		= 0;
			Xwidth		= RmaindWidth;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_DATA);
			Xwidth		= DSPcache->CHwidth - RmaindWidth;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_BALANCE);
			
			WXpos		= RmaindWidth;
			WYpos		= dspnode->cy;
			RXpos		= ReadPos;
			RYpos		= 0;
			Xwidth		= DSPcache->CHwidth - RmaindWidth;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_DATA);
			Xwidth		= RmaindWidth;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_BALANCE);
			stepos = (RmaindWidth > DSPcache->CHwidth - 4 * dspnode->height + dspnode->effectIn.Fstep) ? dspnode->effectIn.Fstep : (RmaindWidth - DSPcache->CHwidth + 4 * dspnode->height);
			ReadPos += stepos;
			RmaindWidth -= stepos;
			//当屏幕的最右边黑屏的部分有4个文字的宽度时切换到下一个状态
			if(RmaindWidth == DSPcache->CHwidth - 4 * dspnode->height)
				state = RUN_STATE_4;
			break;

		//第四阶段，从屏幕最右边的黑屏部分刚好有4个文字的宽度开始，到该段黑屏的最左边刚好滚动到
		//屏幕的最左边结束。该阶段分成三部分，左边文字，中间间隔4个文字的黑屏，右边是下一轮滚动的文字的开始
		case RUN_STATE_4:
			//左边文字部分
			WXpos		= 0;
			WYpos		= dspnode->cy;
			RXpos		= ReadPos;
			RYpos		= 0;
			Xwidth		= RmaindWidth;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_DATA);

			WXpos		= RmaindWidth;
			WYpos		= dspnode->cy;
			RXpos		= 0;
			RYpos		= 0;
			Xwidth		= 4 * dspnode->height;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_DATA);
			Xwidth		= DSPcache->CHwidth - 4 * dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_BALANCE);
			
			WXpos		= RmaindWidth + 4 * dspnode->height;
			WYpos		= dspnode->cy;
			RXpos		= 0;
			RYpos		= 0;
			Xwidth		= DSPcache->CHwidth - (4 * dspnode->height + RmaindWidth);
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_DATA);
			Xwidth		= 4 * dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_BALANCE);

			stepos = (RmaindWidth > dspnode->effectIn.Fstep) ? dspnode->effectIn.Fstep : RmaindWidth;
			ReadPos += stepos;
			RmaindWidth -= stepos;
			//当中间间隔的4个文字宽度的黑屏的最左边滚动到屏幕的最左边时切换状态
			if(RmaindWidth == 0)
				state = RUN_STATE_5;			
			break;
#if 1	
		//第5阶段，从间隔的4个文字的黑屏的最左边滚动到屏幕的最左边开始，到该黑屏段的最右边滚动到屏幕的最左边结束
		case RUN_STATE_5:
			//黑屏部分
			WXpos		= 0;
			WYpos		= dspnode->cy;
			RXpos		= 0;
			RYpos		= 0;
			Xwidth		= SpaceRemaind;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_DATA);
			Xwidth		= DSPcache->CHwidth - SpaceRemaind;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,CLEAR_BALANCE);
			
			WXpos		= SpaceRemaind;
			WYpos		= dspnode->cy;
			RXpos		= 0;
			RYpos		= 0;
			Xwidth		= DSPcache->CHwidth - SpaceRemaind;
			Yheight 	= dspnode->height;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_DATA);
			Xwidth		= SpaceRemaind;
			DSP_runhost_write(WXpos,WYpos,RXpos,RYpos,Xwidth,Yheight,WRITE_BALANCE);
			stepos = (SpaceRemaind > dspnode->effectIn.Fstep) ? dspnode->effectIn.Fstep : SpaceRemaind;
			SpaceRemaind -= stepos;
			//当黑屏段的最右边滚动到屏幕的最嘴边，也就是下一轮播放的文字的串的最开头的文字已经滚动到屏幕的最左边了，
			//开始切换状态2，让它循环往复的播放
			//debug_printf("SpaceRemaind = %d\n",SpaceRemaind);
			if(SpaceRemaind == 0)
			{
				EnterFrame = 0;
				RmaindWidth = dspnode->width;
				SpaceRemaind = 4 * dspnode->height;
				ReadPos = 0;
				state = RUN_STATE_2;
			}
			break;
		#endif
	}
	//usleep(effect->speed * SLEEPTIME);
}
static int EFT_runhost_in(DISPLAY_T *dspnode)
{
	return EFT_runhost_left(dspnode,EFF_IN);
}

static int EFT_runhost_out(DISPLAY_T *dspnode)
{
	return 0;
}



static int DSP_gifFill(DISPLAY_T *dspnode)
{
	uint8_t *writeAddr = NULL;
	int i = 0,h = 0,w = 0;
	uint32_t write_pos = 0;
	uint32_t write_pos_tmp = 0;
	GIFstruct_t *GIFstruct = dspnode->GIFstruct;
	
	for(i = 0 ; i < DSPcache->blockCNT ; i++)
	{
		writeAddr = DSPcache->cache + i * DSPcache->CHwidth * DSPcache->CHheight * 4;
		for(h = 0 ;h < GIFstruct->ctheight ; h ++)
		{
			write_pos_tmp = (h + GIFstruct->cy) * DSPcache->CHwidth * 4;
			for(w = 0 ; w < GIFstruct->ctwidth ; w ++)
			{
				write_pos = write_pos_tmp + (w + GIFstruct->cx) * 4;
				//debug_printf("write_pos = %d,w = %d\n",write_pos,w);
				DSP_write(writeAddr,write_pos,GIFstruct->pre_frame,write_pos);
			}
		}
	}
}




int GIF_GetFrameInfo(DISPLAY_T *dspnode)
{
	static int framecount = 0;
	uint8_t *WriteAddr = NULL;
	if(dspnode->GIFstruct->frameInfo(dspnode->GIFstruct) == GIFerr_TERMINATE)
	{
		dspnode->GIFstruct->endFlag = 1;
		DSP_gifFill(dspnode);
		return -1;
	}
	return 0;
}

int GIF_DecodeFrame(DISPLAY_T *dspnode)
{
	static int framecount = 0;
	uint8_t *WriteAddr = NULL;
	GIFstruct_t *GIFstruct = dspnode->GIFstruct;
	
	if((DSPcache->W_Pos + 1) % 3 != DSPcache->R_Pos)
		DSPcache->W_Pos = (DSPcache->W_Pos + 1) % 3;
	DSPcache->W_Addr = DSPcache->cache + DSPcache->W_Pos * DSPcache->CHwidth * DSPcache->CHheight * SCREEN_BPP;

	GIFstruct->cur_frame = DSPcache->W_Addr;
	GIFstruct->frameDecoder(GIFstruct);
	GIFstruct->pre_frame = GIFstruct->cur_frame;
	return 0;
}




int GIF_display(DISPLAY_T *dspnode)
{
	uint8_t *WriteAddr = NULL;
	//读取GIF的一帧的原始的数据
	if(dspnode->GIFstruct->frameInfo(dspnode->GIFstruct) == GIFerr_TERMINATE)
	{
		dspnode->effectIn.endFlag= 1;
		DSP_gifFill(dspnode);
		dspnode->GIFstruct->GIF_free(dspnode->GIFstruct);
		return -1;
	}
	//当前帧的写地址，DSPcache->W_Addr的更新会在别处更新
	dspnode->GIFstruct->cur_frame = DSPcache->W_Addr;
	//将上面读取出来的原始的数据解码得到RGB数据
	dspnode->GIFstruct->frameDecoder(dspnode->GIFstruct);
	//将上一帧指向当前帧写地址，当前帧写地址将会被指向下一块缓存
	dspnode->GIFstruct->pre_frame = dspnode->GIFstruct->cur_frame;
	return 0;
}

static int EFT_gif_in(DISPLAY_T *dspnode)
{
	if(dspnode->effectIn.endFlag)
		return 0;
		
	GIF_display(dspnode);
	return 0;
}
static int EFT_gif_out(DISPLAY_T *dspnode)
{
	DEBUG_PRINTF;
	dspnode->effectOut.endFlag= 1;
	return 0;
}

typedef int (*EFFfunct_t)(DISPLAY_T *);
static EFFfunct_t EFFchoice[][2] = 
{
	{EFT_oppsize_in,EFT_oppsize_out},
	{EFT_stagger_in,EFT_stagger_out},
	{EFT_windows_in,EFT_windows_out},
	{EFT_move_in,	EFT_move_out},
	{EFT_spread_in,EFT_spread_out},
	{EFT_direct_in,EFT_direct_out},
	{EFT_gif_in,EFT_gif_out},
	{EFT_runhost_in,EFT_runhost_out},
};



int TXT_setEffect_runhost(CTTsize_t *CTTsize,CACHEstruct_t *CACHEstruct,TXTstruct_t *TXTstruct)
{
	uint16_t line_length_total = 0;	//检测出来的文字串总长度
	uint16_t line_length_rest = 0;
	uint16_t end_flag = 0;
	int w,h;
	uint16_t chr_pos_flag = 0;
	
	int state = STATE_START; 
	int finish_flag = 0;
	//文字高度，跟字体大小一样
	uint32_t TXT_height = TXTstruct->ch_hsize;
	//使用第一块缓存，除以要显示的文字的字体大小，即可得到缓存的最大宽度
	uint32_t CACHE_width = CACHEstruct->CHwidth * CACHEstruct->CHheight * 3 / TXT_height;
	//缓存剩余宽度
	uint32_t CACHE_remaind_width = CACHE_width;
	//文字内容剩余长度
	uint32_t TXT_remaind_width = TXTstruct->ctwidth;
	//对重新组装成一行文字的内容将被写在第2块缓存的
	uint8_t *writeAddr = CACHEstruct->cache;
	uint32_t check_pos = 0;
	uint32_t W_posx = 0,W_posy = 0,R_posx = 0,R_posy = 0;
	uint32_t W_lenth_total = 0;
	uint32_t C_lenth_total = 0;

	uint32_t WritePos = 0;
	uint32_t ReadPos = 0;

	debug_printf("TXT_height = %d,CACHE_width = %d,CACHE_remaind_width = %d,TXT_remaind_width = %d\n",TXT_height,CACHE_width,CACHE_remaind_width,TXT_remaind_width);

	while(1)
	{
		switch(state)
		{
			case STATE_START:
				state = STATE_CHAR_CHECK;
				break;
				
			case STATE_CHAR_CHECK:
				//检测到文字串结束，标记满一行文字
				if(TXTstruct->ch_flag[chr_pos_flag] == '\0')
				{
					end_flag = 1;
					state = STATE_WRITE_FB;
					
					line_length_rest = line_length_total;
					if(line_length_total == 0)
						state = STATE_END;
				}
				if(TXTstruct->ch_flag[chr_pos_flag] == '\n')
				{
					state = STATE_WRITE_FB;
					chr_pos_flag += 1;
					check_pos += TXTstruct->ch_hsize;
				}
				
				//检测到文字属于汉字
				if(TXTstruct->ch_flag[chr_pos_flag] == 'H')
				{
					//如果检测到文字串还不满屏幕的宽度，则继续加上后面的文字
					if(line_length_total + TXTstruct->ch_hsize <= CACHE_remaind_width)
					{
						line_length_total += TXTstruct->ch_hsize;
						check_pos += TXTstruct->ch_hsize;
						chr_pos_flag += 1;
						state = STATE_CHAR_CHECK;
					}
					//否则满屏幕宽度，标记满
					else
					{
						DEBUG_PRINTF;
						state = STATE_WRITE_FB;
						//检索出一行文字，将剩余长度置成文字串总长度；
						line_length_rest = line_length_total;
						
						end_flag = 1;
						//debug_printf("line_length_total = %d\n",line_length_total);
					}
				}
				
				//检测到文字属于字母数字
				if(TXTstruct->ch_flag[chr_pos_flag] == 'C')
				{
					if(line_length_total + TXTstruct->ch_csize <= CACHE_remaind_width)
					{
						line_length_total += TXTstruct->ch_csize;
						check_pos += TXTstruct->ch_hsize;
						chr_pos_flag += 1;
						state = STATE_CHAR_CHECK;
					}
					else
					{
						//debug_printf("3、line_length_total = %d\n",line_length_total);
						state = STATE_WRITE_FB;
						
						end_flag = 1;
					}
				}
				
				break;

			case STATE_WRITE_FB:
				if(writeAddr == NULL)
				{
					debug_printf("No memory can be write!\n");
					return -1;
				}
				//当前写入缓存的横坐标位置
				W_posx = W_lenth_total;
				//当前读文字串的横坐标的位置
				R_posx = C_lenth_total;
				for(W_posy = 0 ; W_posy < TXT_height ; W_posy ++)
				{
					WritePos = W_posy * CACHE_width * 4 + W_posx * 4;
					ReadPos = W_posy * TXTstruct->chwidth * 4 + R_posx * 4;
					memcpy(writeAddr+WritePos,TXTstruct->cache+ReadPos,line_length_total * 4);
				}

				//写位置向后偏移
				W_lenth_total += line_length_total;
				debug_printf("W_lenth_total = %d\n",W_lenth_total);
				//读位置向后偏移
				C_lenth_total += check_pos;
				//缓存宽度减小
				CACHE_remaind_width -= line_length_total;
				//文字串长度减小
				TXT_remaind_width   -= check_pos;
				
				line_length_total = 0;
				check_pos = 0;

				if(end_flag)
				{
					state = STATE_END;
					//fb_frame_display(0,0,node_p->dsp_buffer,32,2000*1000);
				}
				else
				{
					state = STATE_CHAR_CHECK;
				}
				
				
				break;


			case STATE_END:
				CTTsize->CTwidth = W_lenth_total;
				CTTsize->CTheight = TXTstruct->ch_hsize;
				CTTsize->cx = 0;
				CTTsize->cy = 0;
				finish_flag = 1;
				break;
		}
		if(finish_flag)
			break;
	}
	return 0;
}


int TXT_setEffect(CTTsize_t *CTTsize,CACHEstruct_t *CACHEstruct,TXTstruct_t *TXTstruct)
{
	uint16_t write_Ypos = 0,read_Xpos = 0;
	uint16_t read_jump = 0;
	
	uint16_t chr_pos_flag = 0;
	uint16_t line_length_total = 0;	//检测出来的文字串总长度

	uint16_t line_height_write = 0;	//实际写入的文字串长度
	
	uint16_t width_limit  = 0;		//限宽，根据横坐标限制显示的横向宽度
	uint16_t height_limit = 0;		//限高，根据纵坐标限制显示的纵向高度
	
	uint32_t Write_ypos_tmp = 0,Read_ypos_tmp = 0;
	uint32_t des_write_pos,src_write_pos = 0;
	
	uint16_t end_flag = 0;
	int finish_flag = 0;
	int state = STATE_START;

	int SLineBytes = CACHEstruct->CHwidth * Screen_BPP;
	int RLineBytes = TXTstruct->chwidth * Screen_BPP;
	int CLineBytes = 0;
	
	int write_pos = CTTsize->cy * SLineBytes + CTTsize->cx * 4;
	int read_pos = 0;
	int h;
	int DoorNextReadPos = 0;
	int DoorFullLineFlag = 0;	

	uint16_t scrType = 0;
	uint16_t frame_height_remaind_size = 0;	//屏幕剩余可现实高度

	//获取屏幕的类型，门架型还是悬臂型
	DP_GetScrType(&scrType);
	CTTsize->CTheight = 0;
	CTTsize->CTwidth  = 0;
	if((CTTsize->cx > CACHEstruct->CHwidth) || (CTTsize->cy > CACHEstruct->CHheight))
		return -1;
	
	width_limit  = CACHEstruct->CHwidth - CTTsize->cx;
	height_limit = CACHEstruct->CHheight - CTTsize->cy;
	
	while(1)
	{
		switch(state)
		{
			case STATE_START:
				//帧缓存块从原点开始写
				write_Ypos = 0;
				//文字串从原点开始读
				read_Xpos  = 0;
				frame_height_remaind_size = (height_limit <= CACHEstruct->CHheight) ? height_limit : CACHEstruct->CHheight;
				state = STATE_CHAR_CHECK;
				break;
			case STATE_CHAR_CHECK:
				//检测到文字串结束，标记满一行文字
				//debug_printf("chr_pos_flag = %d,ch_map->ct_flag[%d] = %d\n",chr_pos_flag,chr_pos_flag,ch_map->ct_flag[chr_pos_flag]);
				if(TXTstruct->ch_flag[chr_pos_flag] == '\0')
				{
					end_flag = 1;	//标志检测结束
					state = STATE_MEM_CHECK;
					//line_length_rest = line_length_total;
					read_jump = 0;
					//下面的这种情况是排除一上来检测字符就立马检测到结束符，直接退出
					if(line_length_total == 0)
					{
						return 0;
					}
					break;
				}
				if(TXTstruct->ch_flag[chr_pos_flag] == '\n')
				{
					//对于门架型的屏幕，当遇到换行符时，重新初始化读取位置
					if(DoorFullLineFlag)
					{
						DoorNextReadPos += TXTstruct->ch_hsize;
						line_length_total = 0;
						DoorFullLineFlag = 0;
						read_pos = DoorNextReadPos * 4;
						chr_pos_flag += 1;
						break;
					}
					state = STATE_MEM_CHECK;
					chr_pos_flag += 1;
					read_jump = TXTstruct->ch_hsize;
					break;
				}
				
				//检测到文字属于汉字
				if(TXTstruct->ch_flag[chr_pos_flag] == 'H')
				{
					//播放信息在遇到换行符之前，如果显示的宽度超过了门架的宽度，
					//首先是截取能显示的部分出来显示，剩下的不显示，一直找到换行符在重新取显示信息
					if(DoorFullLineFlag)
					{
						DoorNextReadPos += TXTstruct->ch_hsize + TXTstruct->Wspace;
						line_length_total = 0;
						chr_pos_flag += 1;
						continue;
					}
				
					//如果检测到文字串还不满屏幕的宽度，则继续加上后面的文字
					if(line_length_total + TXTstruct->ch_hsize + TXTstruct->Wspace <= width_limit)
					{
						line_length_total += TXTstruct->ch_hsize + TXTstruct->Wspace;
						chr_pos_flag += 1;
						state = STATE_CHAR_CHECK;
					}

					//对门架型的屏，屏幕横向能显示多少就显示多少，不会自动换行
					else if(scrType == SCRTYPE_DOOR)
					{
						chr_pos_flag += 1;
						DoorNextReadPos += line_length_total + TXTstruct->ch_hsize + TXTstruct->Wspace;
						DoorFullLineFlag = 1;
						line_length_total = width_limit;
						state = STATE_MEM_CHECK;
					}
					//否则满屏幕宽度，标记满
					else
					{
						state = STATE_MEM_CHECK;
						
						//检索出一行文字，将剩余长度置成文字串总长度；
						read_jump = 0;
					}
					break;
				}
				
				//检测到文字属于字母数字
				if(TXTstruct->ch_flag[chr_pos_flag] == 'C')
				{
					//播放信息在遇到换行符之前，如果显示的宽度超过了门架的宽度，
					//首先是截取能显示的部分出来显示，剩下的不显示，一直找到换行符在重新取显示信息
					if(DoorFullLineFlag)
					{
						DoorNextReadPos += TXTstruct->ch_csize + TXTstruct->Wspace;
						line_length_total = 0;
						chr_pos_flag += 1;
						continue;
					}

					//屏幕宽度还能继续现实一个字母
					if(line_length_total + TXTstruct->ch_csize + TXTstruct->Wspace <= width_limit)
					{
						line_length_total += TXTstruct->ch_csize + TXTstruct->Wspace;
						chr_pos_flag += 1;
						state = STATE_CHAR_CHECK;
					}

					//对门架型的屏，屏幕横向能显示多少就显示多少，不会自动换行
					else if(scrType == SCRTYPE_DOOR)
					{
						chr_pos_flag += 1;
						DoorNextReadPos += line_length_total + TXTstruct->ch_csize + TXTstruct->Wspace;
						DoorFullLineFlag = 1;
						line_length_total = width_limit;
						state = STATE_MEM_CHECK;
					}
					else
					{
						state = STATE_MEM_CHECK;
						read_jump = 0;
					}
					break;
				}
			case STATE_MEM_CHECK:
				if(CACHEstruct->cache== NULL)
					return -1;
				state = STATE_WRITE_FB;
				break;
			
			case STATE_WRITE_FB:
				//写入之前先对比剩余空间高度与文字高度，取较小的一个值
				line_height_write = (frame_height_remaind_size <= TXTstruct->ctheight) ? frame_height_remaind_size : TXTstruct->ctheight;
				CLineBytes = line_length_total * 4;
				for(h = 0 ; h < line_height_write ; h ++)
				{
					memcpy(CACHEstruct->cache + write_pos,TXTstruct->cache + read_pos,CLineBytes);
					write_pos += SLineBytes;
					read_pos += RLineBytes;
				}
				CTTsize->CTwidth = (CTTsize->CTwidth > line_length_total) ? 
					CTTsize->CTwidth : line_length_total;
				CTTsize->CTheight += line_height_write;
				
				frame_height_remaind_size -= line_height_write;

				//写满一帧缓存块,写满一帧缓存后不再继续写!!!
				if(frame_height_remaind_size == 0 || end_flag)
					return 0;
				
				write_Ypos += line_height_write;
				read_Xpos += (line_length_total + read_jump) * 4;

				write_pos = (CTTsize->cy + write_Ypos) * SLineBytes + CTTsize->cx * 4;
				read_pos = read_Xpos;

				line_length_total = 0;
				state = STATE_CHAR_CHECK;
				break;
		}
	}

	return 0;
}



//static void WIND_INITStruct(WINStruct_t *WINStruct,uint16_t width,uint16_t height,uint8_t Psize)
static void WIND_INITStruct(EFFECT_T *effect,DISPLAY_T *dspnode)
{
	//WINStruct->width	= width;
	//WINStruct->height	= height;
	effect->Psize	= 16;
	effect->Rflag	= 0;

	if(effect->direction == DIRECTION_LFRI)
	{
		if(dspnode->width % effect->Psize != 0)
		{
			effect->Blocks	= dspnode->width / effect->Psize + 1;
			effect->RMsize	= dspnode->width % effect->Psize;
			effect->Rflag	= 1;
		}
		else
		{
			effect->Blocks	 = dspnode->width / effect->Psize;
			effect->RMsize	= 0;
		}
	}
	if(effect->direction == DIRECTION_UPDW)
	{
		if(dspnode->height % effect->Psize != 0)
		{
			effect->Blocks	= dspnode->height / effect->Psize + 1;
			effect->RMsize	= dspnode->height % effect->Psize;
			effect->Rflag	= 1;
		}
		else
		{
			effect->Blocks	 = dspnode->height / effect->Psize;
			effect->RMsize	= 0;
		}
	}


	effect->Forder = 0;
	effect->Fstep = 1;
	effect->Ftotal = effect->Psize;
}

static void OPPSIZE_INITStruct(EFFECT_T *effect,DISPLAY_T *dspnode)
{
	effect->Fstep = 1;
	effect->Forder = 1;

	if(effect->direction== DIRECTION_LFRI)
		effect->Ftotal = (dspnode->width % 2 == 0) ? (dspnode->width / 2) : (dspnode->width / 2 + 1);
	if(effect->direction == DIRECTION_UPDW)
	{
		DEBUG_PRINTF;
		effect->Ftotal = (dspnode->height % 2 == 0) ? (dspnode->height / 2) : (dspnode->height / 2 + 1);
	}
}

static void DIRE_INITStruct(EFFECT_T *effect,DISPLAY_T *dspnode)
{

}

static void SPREAD_INITStruct(EFFECT_T *effect,DISPLAY_T *dspnode)
{
	effect->Fstep	= 1;
	effect->Forder  = 1;
	effect->Fstep = 1;

	if(effect->direction == DIRECTION_LEFT || effect->direction == DIRECTION_RIGHT)
	{
		effect->Ftotal = (dspnode->width % effect->Fstep == 0) ? (dspnode->width / effect->Fstep) : (dspnode->width / effect->Fstep + 1);		//共拆解成frames_total帧
		effect->Flast  = dspnode->width;
	}
	
	if(effect->direction == DIRECTION_UP || effect->direction == DIRECTION_DOWN)
	{
		effect->Ftotal	= (dspnode->height % effect->Fstep == 0) ? (dspnode->height / effect->Fstep) : (dspnode->height / effect->Fstep + 1);		//共拆解成frames_total帧
		effect->Flast	= dspnode->height;
	}
	if(effect->direction == DIRECTION_LFRI)
	{
		effect->Ftotal	= ((dspnode->width / 2) % effect->Fstep == 0) ? ((dspnode->width / 2) / effect->Fstep) : ((dspnode->width / 2) / effect->Fstep + 1);
		effect->Flast	= (dspnode->width / 2);
	}
	
	if(effect->direction == DIRECTION_UPDW)
	{
		effect->Ftotal	= ((dspnode->height / 2) % effect->Fstep == 0) ? ((dspnode->height / 2) / effect->Fstep) : ((dspnode->height / 2) / effect->Fstep + 1); 	//共拆解成frames_total帧
		effect->Flast	= (dspnode->height / 2);
		
	}
	
}


static void STAGGER_INITStruct(EFFECT_T *effect,DISPLAY_T *dspnode)
{
	effect->Fstep = 1;
	effect->Psize = 16;
	if(effect->direction == DIRECTION_LFRI)
	{
		effect->Blocks = (dspnode->height % effect->Psize == 0) ? (dspnode->height / effect->Psize) : (dspnode->height / effect->Psize + 1);
		effect->Ftotal = (dspnode->width % effect->Fstep == 0) ? (dspnode->width / effect->Fstep) : (dspnode->width / effect->Fstep + 1);
		effect->Flast	= dspnode->width;
	}
	if(effect->direction == DIRECTION_UPDW)
	{
		effect->Blocks = (dspnode->width % effect->Psize == 0) ? (dspnode->width / effect->Psize) : (dspnode->width / effect->Psize + 1);
		effect->Ftotal = (dspnode->height % effect->Fstep == 0) ? (dspnode->height / effect->Fstep) : (dspnode->height / effect->Fstep + 1);
		effect->Flast	= dspnode->height;
	}
	effect->Forder = 0;
}



static void MOVE_INITStruct(EFFECT_T *effect,DISPLAY_T *dspnode)
{
	uint16_t frame_count = 0;
	uint16_t cardType = TRANSCARD_TXRX;
	DP_GetCardType(&cardType);
	effect->Fstep	= (cardType == TRANSCARD_200) ? 4 : 1;
	effect->Forder  = 1;
	effect->Fflag	= 0;
	if(effect->direction == DIRECTION_LEFT || effect->direction == DIRECTION_RIGHT)
	{
		if(dspnode->width % effect->Fstep)
		{
			effect->Fflag  = 1;
			effect->Ftotal = dspnode->width / effect->Fstep + 1;
		}
		else
		{
			effect->Fflag  = 0;
			effect->Ftotal = dspnode->width / effect->Fstep;
		}
	}
	if(effect->direction == DIRECTION_UP || effect->direction == DIRECTION_DOWN)
	{
		if(dspnode->height % effect->Fstep != 0)
		{
			effect->Fflag  = 1;
			effect->Ftotal = dspnode->height / effect->Fstep + 1;
		}
		else
		{
			effect->Fflag  = 0;
			effect->Ftotal = dspnode->height / effect->Fstep;
		}
	}
}

static void RUNHOST_INITStruct(EFFECT_T *effect,DISPLAY_T *dspnode)
{
	uint16_t mvspeed = 1;
	uint16_t cardType = TRANSCARD_TXRX;
	DP_GetCardType(&cardType);
	DP_GetMvspeed(&mvspeed);
	effect->Fstep	= (cardType == TRANSCARD_200) ? mvspeed : 1;
	effect->Rlong = dspnode->width;
	effect->Forder  = 1;
	effect->Fflag	= 0;
	effect->Rpos = 0;
	effect->Rpos1 = 0;
	if(effect->Rlong % effect->Fstep != 0)
	{
		effect->Ftotal = effect->Rlong / effect->Fstep + 1;
	}

	else
	{
		effect->Ftotal = effect->Rlong / effect->Fstep;
	}
	
}


static int GIF_INITStruct(EFFECT_T *effect,DISPLAY_T *dspnode)
{
	effect->endFlag = 0;
	return 0;
}



static int EffectInit(DISPLAY_T *dspnode)
{
	switch(dspnode->effectIn.number)
	{
		case 0:
			OPPSIZE_INITStruct(&dspnode->effectIn,dspnode);
			break;
		case 1:
			STAGGER_INITStruct(&dspnode->effectIn,dspnode);
			break;
		case 2:
			WIND_INITStruct(&dspnode->effectIn,dspnode);
			break;
		case 3:
			MOVE_INITStruct(&dspnode->effectIn,dspnode);
			break;
		case 4:
			SPREAD_INITStruct(&dspnode->effectIn,dspnode);
			break;
		case 5:
			DIRE_INITStruct(&dspnode->effectIn,dspnode);
			break;
		case 6:
			GIF_INITStruct(&dspnode->effectIn,dspnode);
			break;
		case 7:
			RUNHOST_INITStruct(&dspnode->effectIn,dspnode);
			break;
		default:
			break;
	}
	switch(dspnode->effectOut.number)
	{
		case 0:
			OPPSIZE_INITStruct(&dspnode->effectOut,dspnode);
			break;
		case 1:
			STAGGER_INITStruct(&dspnode->effectOut,dspnode);
			break;
		case 2:
			WIND_INITStruct(&dspnode->effectOut,dspnode);
			break;
		case 3:
			MOVE_INITStruct(&dspnode->effectOut,dspnode);
			break;
		case 4:
			SPREAD_INITStruct(&dspnode->effectOut,dspnode);
			break;
		case 5:
			DIRE_INITStruct(&dspnode->effectOut,dspnode);
			break;
		case 6:
			GIF_INITStruct(&dspnode->effectOut,dspnode);
			break;
		case 7:
			RUNHOST_INITStruct(&dspnode->effectOut,dspnode);
			break;
		default:
			break;
	}
	return 0;
}


void DSP_INITstruct(DISPLAY_T *dspnode,uint32_t cx,uint32_t cy,uint16_t CTwidth,uint16_t CTheight,
	uint8_t type,uint8_t in,uint8_t in_dir,uint8_t out,uint8_t out_dir,uint32_t speed)
{
	uint8_t type_in,dire_in,type_out,dire_out;

	dspnode->cx 		= cx;
	dspnode->cy 		= cy;
	dspnode->width		= CTwidth;
	dspnode->height 	= CTheight;
	
	//1:向左，2向右，3向上，4向下，5左右，6上下，7中心
	dspnode->type		= type;
	dspnode->effectIn.number = in;
	dspnode->effectIn.direction = in_dir;
	dspnode->effectIn.speed = speed;
	dspnode->effectIn.endFlag = 0;
	dspnode->effectOut.number = out;
	dspnode->effectOut.direction = out_dir;
	dspnode->effectOut.speed = speed;
	dspnode->effectOut.endFlag = 0;
	
	debug_printf("in = %d,in_dir = %d,out = %d,out_dir = %d\n",in,in_dir,out,out_dir);
	if(dspnode->type == TYPE_GIF)
	{
		dspnode->effectIn.number = 6;
		dspnode->effectOut.number = 6;
	}
	
	if(dspnode->type == TYPE_LBD)
	{
		dspnode->effectIn.number = 5;
		dspnode->effectOut.number = 5;
	}
	if(dspnode->effectIn.number == 7)
	{
		dspnode->cx = 0;
		dspnode->cy = (frame_height - dspnode->height) >> 1;
	}
	//if(dspnode->type == TYPE_GIF)
	EffectInit(dspnode);
	debug_printf("1dspnode->width = %d,dspnode->height = %d\n",dspnode->width,dspnode->height);
	dspnode->display_in  = EFFchoice[dspnode->effectIn.number][EFF_IN];
	dspnode->display_out = EFFchoice[dspnode->effectOut.number][EFF_OUT];
}



int DSPStructInit(DISPLAY_T *DSPhead)
{
	int pos = 0;
	struct list_head *DspLstPos = DSPhead->list.next;
	DISPLAY_T *playnode = NULL;
	memset(&DSPcnt,0,sizeof(DSPCNT_T));
	
	while(DspLstPos != &DSPhead->list)
	{
		playnode = list_entry(DspLstPos,DISPLAY_T,list);

		debug_printf("3playnode->width = %d,playnode->height = %d,playnode->type = %d\n",playnode->width,playnode->height,playnode->type);
		//跑马灯滚屏
		if(playnode->effectIn.number == 7)
		{
			DEBUG_PRINTF;
			DSPcnt.DSPStruct[pos].cx		= 0;
			DSPcnt.DSPStruct[pos].cy		= (frame_height - playnode->height) >> 1;
			DSPcnt.DSPStruct[pos].width 	= frame_width;
			DSPcnt.DSPStruct[pos].height	= playnode->height;
		}
		else
		{
			DEBUG_PRINTF;
			DSPcnt.DSPStruct[pos].cx		= playnode->cx;
			DSPcnt.DSPStruct[pos].cy		= playnode->cy;
			DSPcnt.DSPStruct[pos].width 	= playnode->width;
			DSPcnt.DSPStruct[pos].height	= playnode->height;
		}
		debug_printf("DSPcnt.dspcnt = %d,DSPcnt.DSPStruct[%d].cx = %d,DSPcnt.DSPStruct[%d].cy = %d,DSPcnt.DSPStruct[%d].width = %d,DSPcnt.DSPStruct[%d].height = %d\n",DSPcnt.dspcnt,pos,DSPcnt.DSPStruct[pos].cx,pos,DSPcnt.DSPStruct[pos].cy,pos,DSPcnt.DSPStruct[pos].width,pos,DSPcnt.DSPStruct[pos].height);
		DspLstPos = DspLstPos->next;
		DSPcnt.dspcnt += 1;
		pos += 1;
	}
	DSPcnt.clrenable = 1;
	return 0;
}


//DSPhead描述的是显示链表的链表头，头结点后面跟着很多个显示元素节点
//每次调用该函数，都要将该链表上所有的显示元素节点遍历一遍，也就是每个节点
//都会被刷到屏幕上一次，就完成了动态显示的一步动作，多次被调用就完成多步动作
//这样就能实现动态显示了
int itemDisplay(DISPLAY_T *DSPhead,uint8_t InOutFlag)
{
	int endFlag = 1;
	int DisplayEnd = 0;		//标记着链表所有显示元素是否都显示完成
	DISPLAY_T *playnode = NULL;
	struct list_head *DspLstPos = NULL;


	int Swidth = frame_width,Sheight = frame_height;
	int count = 0;
	while(!DisplayEnd)
	{
		//取出头元素
		DspLstPos = DSPhead->list.next;
		endFlag = 1;

		while(DspLstPos != &DSPhead->list)
		{	
			//根据链表的内部成员连接点取得整个节点结构的地址
			playnode = list_entry(DspLstPos,DISPLAY_T,list);
			if(playnode->cx >= Swidth || playnode->cy >= Sheight)
			{
				DspLstPos = DspLstPos->next;
				continue;
			}
			
			//根据指定特效，单步显示特效(特效拆分成多步显示完成)
			if(InOutFlag == EFF_IN)
			{
				
				//printf("EFF_IN %d\n",count++);
				//usleep(50*1000);
				usleep(playnode->effectIn.speed * SLEEPTIME);
				playnode->display_in(playnode);
				endFlag &= playnode->effectIn.endFlag;
			}

			//出屏单步特效
			if(InOutFlag == EFF_OUT)
			{
				playnode->display_out(playnode);
				usleep(playnode->effectIn.speed * SLEEPTIME);
				//逐个相与，当每个节点都播放完毕相与结果一定为真
				endFlag &= playnode->effectOut.endFlag;
			}
			DspLstPos = DspLstPos->next;
		}

		//这里其实是判断一下交替显示的3块缓存，写位置是否赶上了读为主
		//是为了避免读写同一块缓存而造成闪屏
		while((DSPcache->W_Pos + 1) % 3 == DSPcache->R_Pos)
		{
			//printf("DSPcache->W_Pos + 1) % 3 == DSPcache->R_Pos \n");
			usleep(10);
		}

		//将写文字偏移到下一块缓存
		DSPcache->W_Pos = (DSPcache->W_Pos + 1) % 3;
		DSPcache->W_Addr = DSPcache->cache + DSPcache->W_Pos * DSPcache->CHwidth * DSPcache->CHheight * SCREEN_BPP;

		//endFlag为真表示链表中所有显示元素都完成了动态显示
		//完成动态显示那么就要跳出该循环了
		if(endFlag)
		{
			DisplayEnd = 1;
			return 0;
		}
	}
	return 0;
}








