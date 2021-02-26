#include "modbus_display.h"
#include "config.h"
#include "debug.h"
#include "content.h"
#include "display.h"
#include "../../module/mtimer.h"



static uint32_t ScreenW = 0,ScreenH = 0;

void Mdbs_SetScreenSize(uint32_t WIDTH,uint32_t HEIGHT)
{
	ScreenW = WIDTH;
	ScreenH = HEIGHT;
}

ContentList ContentCache = 
{
	.head = NULL,
	.tail = NULL
};

ContentList LineCache = 
{
	.head = NULL,
	.tail = NULL
};

ContentList ScreenCache = 
{
	.head = NULL,
	.tail = NULL
};


void XKCellImagePrintf(XKCellImage *XKCellIma)
{
	mdbs_display_printf("XKCellIma->strImage = %s\n",XKCellIma->strImage);
}

void XKCellStringPrintf(XKCellString *XKCellStr)
{
	mdbs_display_printf(
				 "XKCellStr->flash = %d\n"
				 "XKCellStr->nSpace = %d\n"
				 "XKCellStr->nFontType = %c\n"
				 "XKCellStr->nFontSize = %d\n"
				 "XKCellStr->nForeColor: %d | %d | %d \n"
				 "XKCellStr->nBkColor: %d | %d | %d \n"
				 "XKCellStr->strContent = %s\n",
				 XKCellStr->flash,
				 XKCellStr->nSpace,
				 XKCellStr->nFontType,
				 XKCellStr->nFontSize,
				 XKCellStr->nForeColor[0],XKCellStr->nForeColor[1],XKCellStr->nForeColor[2],
				 XKCellStr->nBkColor[0],  XKCellStr->nBkColor[1],  XKCellStr->nBkColor[2],
				 XKCellStr->strContent
				 );

	uint8_t i = 0;
	mdbs_display_printf("XKCellStr->strContent data : ");
	for(i = 0 ; i < XKCellStr->strContentLen ; i++)
		mdbs_display_printf("0x%x ",XKCellStr->strContent[i]);
	mdbs_display_printf("\n");
}


void DSPNodePrintf(DSPNode_t *DSPNode)
{
	MDBS_DISPLAY_DEBUG;
	mdbs_display_printf("\n\n"
				 "type = %d\n"
				 "Sseq = %d\n"
				 "Lseq = %d\n"
				 "Cx = %d\n"
				 "Cy = %d\n"
				 "width  = %d\n"
				 "height = %d\n"
				 ,
		DSPNode->type,DSPNode->Sseq,DSPNode->Lseq,
		DSPNode->Cx,DSPNode->Cy,DSPNode->width,DSPNode->height);
	switch(DSPNode->type)
	{
		case 0:
			break;
		case 1:
			mdbs_display_printf("img\n");
			XKCellImagePrintf(DSPNode->XKCellIma);break;
		case 2:
			mdbs_display_printf("str\n");
			XKCellStringPrintf(DSPNode->XKCellStr);break;
		default:break;
	}
}



int Mdbs_EffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	switch(option)
	{
		case MDBS_DIRECTLY:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
		case MDBS_MOVE_UP:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_UP;
			break;
		case MDBS_MOVE_DOWN:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_DOWN;
			break;
		case MDBS_MOVE_LEFT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_LEFT;
			break;
		case MDBS_MOVE_RIGHT:
			*type = EFFECT_MOVE;
			*dire = DIRECTION_RIGHT;
			break;
		case MDBS_RUNHOST:
			*type = EFFECT_RUNNINGHOST;
			*dire = DIRECTION_LFRI;
			break;
		case MDBS_FLASH:
			//break;
		default:
			*type = EFFECT_DIRECT;
			*dire = DIRECTION_NO;
			break;
	}
	
	return 0;
}




