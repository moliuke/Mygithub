#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "content.h"
#include "debug.h"


#include "character.h"
#include "../Hardware/HW3G_RXTX.h"




//#define CONT_DEBUG

#ifdef CONT_DEBUG
#define CONT_DEBUG_PRINTF	DEBUG_PRINTF
#define cont_debug_printf	debug_printf
#else
#define CONT_DEBUG_PRINTF	
#define cont_debug_printf	
#endif




ContentList content;

static ContentList ct_list;

void Initcontentnode(XKDisplayItem *head)
{
	head->pCellAnimate_head = NULL;
	head->pCellAnimate_tail = NULL;

	head->pCellEffect_head	= NULL;
	head->pCellEffect_tail	= NULL;

	head->pCellImage_head	= NULL;
	head->pCellImage_tail	= NULL;

	head->pCellString_head	= NULL;
	head->pCellString_tail	= NULL;

	head->pCellTwinkle_head	= NULL;
	head->pCellTwinkle_tail	= NULL;

	head->DSPNode_head = NULL;
	head->DSPNode_tail = NULL;
}

void InitCellString(XKCellString * pCellString)
{
    pCellString->nForeColor[0] = 255;
	pCellString->nForeColor[1] = 0;
	pCellString->nForeColor[2] = 0;
    pCellString->nBkColor[0] = 0;
	pCellString->nBkColor[1] = 0;
	pCellString->nBkColor[2] = 0;
    pCellString->nShadowColor[0] = 0;
	pCellString->nShadowColor[1] = 0;
	pCellString->nShadowColor[2] = 0;
    strcpy(pCellString->strContent,"");
    pCellString->nFontType = 's';
    pCellString->nFontSize = 32;
    pCellString->nSpace = 0;
    pCellString->nCellOrder = 0;
}

void InitCellImage(XKCellImage * pCellImage)
{
    pCellImage->cx = 0;
	pCellImage->cy = 0;
    pCellImage->nCellOrder = 0;
	strcpy(pCellImage->strImage,"");
}

void InitCellAnimate(XKCellAnimate * pCellAnimate)
{
    pCellAnimate->cx = 0;
	pCellAnimate->cy = 0;
    pCellAnimate->nCellOrder = 0;
    pCellAnimate->nFormatType = 0;
    pCellAnimate->nPlayTime = 1;
	strcpy(pCellAnimate->strAnimate,"");
}

void InitCellEffect(XKCellEffect * pCellEffect)
{
	pCellEffect->cx = 0;
	pCellEffect->cy = 0;
	pCellEffect->nCellOrder = 0;
	strcpy(pCellEffect->strEffect,"");

}

//twinkle闪烁
void InitCellTwinkle(XKCellTwinkle * pCellTwinkle)
{
	pCellTwinkle->cx1 = 0;
	pCellTwinkle->cy1 = 0;
	pCellTwinkle->cx2 = DEV_statusmsg.DEV_scrmsg.scr_width;
	pCellTwinkle->cy2 = DEV_statusmsg.DEV_scrmsg.scr_height;

	pCellTwinkle->nTwinkleTime = 1;
	pCellTwinkle->nTwinkleNum = 1;
}


int InitContentlist(ContentList *head) 
{
	head->head = NULL;
	head->tail = NULL;
	return 0;
}

int AddItemString(XKDisplayItem *head,XKCellString *pCellString)
{
	XKCellString *pStringNew =(XKCellString *)malloc(sizeof(XKCellString));
	if (pStringNew == NULL || head == NULL)
	{
//	    XKErrorCode = EALLAC; 
		return -1;
	}
	CONT_DEBUG_PRINTF;
	memset(pStringNew,0x00,sizeof(XKCellString));
	
	*pStringNew = *pCellString;
	CONT_DEBUG_PRINTF;
	if(head->pCellString_tail == NULL)
	{
		CONT_DEBUG_PRINTF;
		pStringNew->pNext = NULL;
		head->pCellString_head= head->pCellString_tail = pStringNew;
	}
	else
	{
		CONT_DEBUG_PRINTF;
		head->pCellString_tail->pNext = pStringNew;
		pStringNew->pNext = NULL;
		head->pCellString_tail = pStringNew;
	}
	CONT_DEBUG_PRINTF;
	return 0;
}



int AddItemDSPNode(XKDisplayItem *head,DSPNode_t * DSPNode)
{
	DSPNode_t *DSPNodeNew = (DSPNode_t *)malloc(sizeof(DSPNode_t));
	if(DSPNode == NULL)
		return -1;

	memset(DSPNodeNew,0x00,sizeof(DSPNode_t));
	*DSPNodeNew = *DSPNode;

	if(head->DSPNode_tail == NULL)
	{
		DSPNodeNew->pNext = NULL;
		head->DSPNode_head = head->DSPNode_tail = DSPNodeNew;
	}

	else
	{
		head->DSPNode_tail->pNext = DSPNodeNew;
		DSPNodeNew->pNext = NULL;
		head->DSPNode_tail = DSPNodeNew;
	}
	return 0;	
}

int AddItemImage(XKDisplayItem *head,XKCellImage * pCellImage)
{
	XKCellImage *pImageNew = (XKCellImage *)malloc(sizeof(XKCellImage));
	if (pImageNew ==NULL)
		{
//	    XKErrorCode = EALLAC; 
		return -1;
	}
	memset(pImageNew,0x00,sizeof(XKCellImage));
	*pImageNew = *pCellImage;
	if(head->pCellImage_tail == NULL)
	{
		pImageNew->pNext = NULL;
		head->pCellImage_head = head->pCellImage_tail = pImageNew;
		
	}
	else
	{
		head->pCellImage_tail->pNext = pImageNew;
		pImageNew->pNext = NULL;
		head->pCellImage_tail = pImageNew;
	}
	
	return 0;
}


int AddItemAnimate(XKDisplayItem *head,XKCellAnimate * pCellAnimate) 
{
	XKCellAnimate *pAnimateNew = (XKCellAnimate *)malloc(sizeof(XKCellAnimate));
	if (pAnimateNew ==NULL)
		{
//	    XKErrorCode = EALLAC;
		return -1;
	}
	memset(pAnimateNew,0x00,sizeof(XKCellAnimate));
	
	*pAnimateNew = *pCellAnimate;
	if(head->pCellAnimate_tail == NULL)
	{
		pAnimateNew->pNext = NULL;
		head->pCellAnimate_head = head->pCellAnimate_tail = pAnimateNew;

	}
	else
	{
		head->pCellAnimate_tail->pNext = pAnimateNew;
		pAnimateNew->pNext = NULL;
		head->pCellAnimate_tail = pAnimateNew;
	}
	
	return 0;
}


int AddItemEffect(XKDisplayItem *head,XKCellEffect * pCellEffect)  
{
	XKCellEffect *pEffectNew = (XKCellEffect *)malloc(sizeof(XKCellEffect));
	if (pEffectNew ==NULL)
		{
  //	    XKErrorCode = EALLAC;
		return -1;
	}
	*pEffectNew = *pCellEffect;
	if(head->pCellEffect_tail == NULL)
	{
		pEffectNew->pNext = NULL;
		head->pCellEffect_head = head->pCellEffect_tail = pEffectNew;
		
	}
	else
	{
		head->pCellEffect_tail->pNext = pEffectNew;
		pEffectNew->pNext = NULL;
		head->pCellEffect_tail = pEffectNew;
	}

	return 0;
}

int AddItemTwinkle(XKDisplayItem *head,XKCellTwinkle *pTwinkle)
{
	XKCellTwinkle *pTwinkleNew = (XKCellTwinkle *)malloc(sizeof(XKCellTwinkle));
	
	if (pTwinkleNew == NULL)
		{
	  //  XKErrorCode = EALLAC;
		return -1;
	}

	*pTwinkleNew = *pTwinkle;
	if(head->pCellTwinkle_tail == NULL)
	{
		pTwinkleNew->pNext = NULL;
		head->pCellTwinkle_head = head->pCellTwinkle_tail = pTwinkleNew;
	}
	else
	{
		head->pCellTwinkle_tail->pNext = pTwinkleNew;
		pTwinkleNew->pNext = NULL;
		head->pCellTwinkle_tail = pTwinkleNew;
	}
	
	return 0;
}


int AddDisplayItem(ContentList *head,XKDisplayItem * pItem)
{
	//创建一个新节点
	XKDisplayItem *pItemNew =(XKDisplayItem *)malloc(sizeof(XKDisplayItem));
	if (!pItemNew)
	{
	//    XKErrorCode = EALLAC;
		return -1;
	}

	memset(pItemNew,0x00,sizeof(XKDisplayItem));

	//给新节点赋值
	*pItemNew = *pItem;

	//如果当前列表头尾都是空，则将头尾指针都指向新节点。否则的话将尾指针指向新节点
	if(head->tail == NULL)
	{
		pItemNew->pNext = NULL;
		head->head = head->tail = pItemNew;
	}
	else
	{
		head->tail->pNext = pItemNew;
		pItemNew->pNext = NULL;
		head->tail = pItemNew;
	}
	return 0;
}

void ClearContent(ContentList *head)
{
	XKDisplayItem * pItem = head->head;
	XKCellString * pCellString = NULL;
	XKCellImage * pCellImage = NULL ;
	XKCellTwinkle * pCellTwinkle = NULL;

	DSPNode_t *DSPNode = NULL;

	head->refresh = LST_FREE;
	while(pItem)
	{
		head->head = pItem->pNext;
		pCellString = pItem->pCellString_head;
		while(pCellString)
		{
			pItem->pCellString_head = pCellString->pNext;
			free(pCellString);
			pCellString = pItem->pCellString_head;
		}
		pCellImage = pItem->pCellImage_head;
		while(pCellImage)
		{
			pItem->pCellImage_head = pCellImage->pNext;
			free(pCellImage);
			pCellImage = pItem->pCellImage_head;
		}

		pCellTwinkle = pItem->pCellTwinkle_head;
		while(pCellTwinkle)
		{
			pItem->pCellTwinkle_head = pCellTwinkle->pNext;
			free(pCellTwinkle);
			pCellTwinkle = pItem->pCellTwinkle_head;
		}

		DSPNode = pItem->DSPNode_head;
		while(DSPNode)
		{
			pItem->DSPNode_head = DSPNode->pNext;
			free(DSPNode);
			DSPNode = pItem->DSPNode_head;
		}

		if(pItem->Curplay != NULL)
		{
			free(pItem->Curplay);
			pItem->Curplay = NULL;
		}

		if(pItem != NULL)
			free(pItem);
		pItem = head->head;
		
	}
	head->head = head->tail = NULL;
	head->itemcount = 0;

}

XKDisplayItem *GetItemHead(ContentList *head)
{
  return head->head;
}

int XKDisplayInsert(XKDisplayItem *XKDisplay,int scenecnt)
{
	int i = 0,j;
	pthread_mutex_lock(&content_lock);
	ClearContent(&content);
	for(i = 0 ; i < scenecnt ; i++)
		AddDisplayItem(&content,&XKDisplay[i]);
	content.itemcount = scenecnt;
	content.refresh = LST_REFLASH;
	pthread_mutex_unlock(&content_lock);
	debug_printf("scenecnt = %d\n",scenecnt);
}

DSPriorit_t DSPriorit;
void DSPrioritInit(void)
{
	DSPriorit.bmppri = -1;
	DSPriorit.gifpri = -1;
	DSPriorit.lbpri = -1;
	DSPriorit.pngpri = -1;
	DSPriorit.strpri = -1;
}

void SetDSPriorit(uint8_t type,uint8_t priority)
{
	switch(type)
	{
		case DSPTYPE_GIF:DSPriorit.gifpri = priority;break;
		case DSPTYPE_STR:DSPriorit.strpri = priority;break;
		case DSPTYPE_LBD:DSPriorit.lbpri = priority;break;
		case DSPTYPE_PNG:DSPriorit.pngpri = priority;break;
		default:break;
	}
}
void GetDSPriorit(uint8_t type,uint8_t *priority)
{
	switch(type)
	{
		case DSPTYPE_GIF:*priority = DSPriorit.gifpri;break;
		case DSPTYPE_STR:*priority = DSPriorit.strpri;break;
		case DSPTYPE_LBD:*priority = DSPriorit.lbpri;break;
		case DSPTYPE_PNG:*priority = DSPriorit.pngpri;break;
		default:break;
	}
}

