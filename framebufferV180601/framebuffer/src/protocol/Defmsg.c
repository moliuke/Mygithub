#include <stdio.h>
#include "content.h"
#include "debug.h"
#include "Defmsg.h"
#include "../Hardware/Data_pool.h"


static DSPNode_t SWRDSPNODE[2];
static XKDisplayItem SWRSXKDisplay;




static void DefmsgInit(XKCellString *Defmsg,char *String,uint8_t Fonttype,uint8_t FontSize)
{
	memcpy(Defmsg->strContent,String,strlen(String));
	Defmsg->strContent[strlen(String)] = '\0';
	Defmsg->strContentLen = 8;
	Defmsg->nFontType = Fonttype;
	Defmsg->nForeColor[0] = 0xff;
	Defmsg->nForeColor[1] = 0x00;
	Defmsg->nForeColor[2] = 0x00;
	Defmsg->nBkColor[0] = 0x00;
	Defmsg->nBkColor[1] = 0x00;
	Defmsg->nBkColor[2] = 0x00;
	Defmsg->nFontSize = FontSize;
}


static int DefmsgParse(uint32_t Swidth,uint32_t Sheight,int direct,uint8_t font)
{
	int Cx = 0,Cy = 0;
	XKCellString *Defstr1 = (XKCellString *)malloc(sizeof(XKCellString));
	XKCellString *Defstr2 = (XKCellString *)malloc(sizeof(XKCellString));

	memset(Defstr1,0,sizeof(Defstr1));
	memset(Defstr2,0,sizeof(Defstr1));

	DefmsgInit(Defstr1,str1,'s',font);
	DefmsgInit(Defstr2,str2,'s',font);	

	memset(&SWRSXKDisplay,0,sizeof(SWRSXKDisplay));
	memset(&SWRDSPNODE[0],0,sizeof(SWRDSPNODE[0]));
	memset(&SWRDSPNODE[1],0,sizeof(SWRDSPNODE[1]));

	SWRDSPNODE[0].type = DSPTYPE_STR;
	SWRDSPNODE[1].type = DSPTYPE_STR;
	
	SWRDSPNODE[0].XKCellStr = Defstr1;
	SWRDSPNODE[1].XKCellStr = Defstr2;
	debug_printf("SWRDSPNODE[0].XKCellStr->strContent = %s\n",SWRDSPNODE[0].XKCellStr->strContent);

	int interval = 0;
	switch(direct)
	{
		case LINE_1:
			if(Swidth == font * 8 )
			{
				SWRDSPNODE[0].Cx = 0;
				SWRDSPNODE[1].Cx = font * 4;
			}
			else
			{
				DEBUG_PRINTF;
				interval = (Swidth - font * 8) / 3;
				SWRDSPNODE[0].Cx = interval; 
				SWRDSPNODE[1].Cx = font * 4 + interval * 2;
				debug_printf("SWRDSPNODE[0].Cx = %d\n",SWRDSPNODE[0].Cx);
			}

			if(Sheight == font)
			{
				SWRDSPNODE[0].Cy = 0;
				SWRDSPNODE[1].Cy = 0;
			}
			else
			{
				interval = (Sheight - font) / 2;
				SWRDSPNODE[0].Cy = interval;
				SWRDSPNODE[1].Cy = interval;
			}
			break;
		case LINE_2:
			if(Swidth == font * 4)
			{
				SWRDSPNODE[0].Cx = 0;
				SWRDSPNODE[1].Cx = 0;
			}
			else
			{
				interval = (Swidth - font * 4) / 2;
				SWRDSPNODE[0].Cx = interval;
				SWRDSPNODE[1].Cx = interval;
			}

			if(Sheight == font * 2)
			{
				SWRDSPNODE[0].Cy = 0;
				SWRDSPNODE[1].Cy = font;
			}
			else
			{
				interval = (Sheight - font * 2) / 3;
				SWRDSPNODE[0].Cy = interval;
				SWRDSPNODE[1].Cy = font + interval * 2;
			}
			break;
		default:
			break;
	}

	SWRSXKDisplay.nDelayTime = 20 * 1000 * 1000;
	SWRSXKDisplay.nEffectIn = 1;
	SWRSXKDisplay.nEffectOut = 1;

	int i = 0;
	for(i = 0 ; i < 2 ; i++) 
		AddItemDSPNode(&SWRSXKDisplay,&SWRDSPNODE[i]);
	return 0;
}


int Defmsg_display(void)
{
	int DIRECT = 1;
	int FONT = 64;
	int FontState = FONT64;
	uint32_t Swidth,Sheight;
	DP_GetScreenSize(&Swidth,&Sheight);

	while(1)
	{
		switch(FontState)
		{
			case FONT64:
				if(Sheight < 64) 
				{
					FontState = FONT48;
					break;
				}

				if(Swidth >= 64 * 8)
				{
					DIRECT = LINE_1;
					FONT = FONT64;
					goto CHAR_PARSE;
				}

				if(Swidth < 64 * 4)
				{
					FontState = FONT48;
					break;
				}

				//高度够64，宽度也够64*4,接下来就要看高度能不能显示两行64的
				if(Sheight < 64 * 2)
				{
					FontState = FONT48;
					break;
				}
				
				DIRECT = LINE_2;
				FONT = FONT64;
				goto CHAR_PARSE;
				
			case FONT48:
				if(Sheight < 48) 
				{
					FontState = FONT32;
					break;
				}

				if(Swidth >= 48 * 8)
				{
					DIRECT = LINE_1;
					FONT = FONT48;
					goto CHAR_PARSE;
				}

				if(Swidth < 48 * 4)
				{
					FontState = FONT32;
					break;
				}

				//高度够64，宽度也够64*4,接下来就要看高度能不能显示两行64的
				if(Sheight < 48 * 2)
				{
					FontState = FONT32;
					break;
				}
				
				DIRECT = LINE_2;
				FONT = FONT48;
				goto CHAR_PARSE;
			case FONT32:
				if(Sheight < 32) 
				{
					FontState = FONT24;
					break;
				}

				if(Swidth >= 32 * 8)
				{
					DIRECT = LINE_1;
					FONT = FONT32;
					goto CHAR_PARSE;
				}

				if(Swidth < 32 * 4)
				{
					FontState = FONT24;
					break;
				}

				//高度够64，宽度也够64*4,接下来就要看高度能不能显示两行64的
				if(Sheight < 32 * 2)
				{
					FontState = FONT24;
					break;
				}
				
				DIRECT = LINE_2;
				FONT = FONT32;
				goto CHAR_PARSE;
			case FONT24:
				if(Sheight < 24) 
				{
					FontState = FONT16;
					break;
				}

				if(Swidth >= 24 * 8)
				{
					DIRECT = LINE_1;
					FONT = FONT24;
					goto CHAR_PARSE;
				}

				if(Swidth < 24 * 4)
				{
					FontState = FONT16;
					break;
				}

				//高度够64，宽度也够64*4,接下来就要看高度能不能显示两行64的
				if(Sheight < 24 * 2)
				{
					FontState = FONT16;
					break;
				}
				
				DIRECT = LINE_2;
				FONT = FONT24;
				goto CHAR_PARSE;
			case FONT16:
				if(Sheight < 16) 
				{
					return -1;
				}

				if(Swidth >= 16 * 8)
				{
					DIRECT = LINE_1;
					FONT = FONT16;
					goto CHAR_PARSE;
				}

				if(Swidth < 16 * 4)
				{
					return -1;
				}

				//高度够64，宽度也够64*4,接下来就要看高度能不能显示两行64的
				if(Sheight < 16 * 2)
				{
					return -1;
				}
				
				DIRECT = LINE_2;
				FONT = FONT16;
				goto CHAR_PARSE;
			default:
				return -1;
		}
	}

	return 0;

	CHAR_PARSE:
		DefmsgParse(Swidth,Sheight,DIRECT,FONT);

		pthread_mutex_lock(&content_lock);
		ClearContent(&content);
		AddDisplayItem(&content,&SWRSXKDisplay);
		content.itemcount = 1;
		content.refresh = LST_REFLASH;
		pthread_mutex_unlock(&content_lock);
		DEBUG_PRINTF;
		return 0;
		
}
