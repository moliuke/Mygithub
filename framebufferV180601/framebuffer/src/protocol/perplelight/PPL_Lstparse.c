#include <stdio.h>

#include "PPL_Lstparse.h"
#include "debug.h"


#define PPL_SXKSIZE		48		//一个播放列表里面最多限制在幕,即48条Item
#define PPL_NODESIZE		24		//一条Item里面最多容许24个信息:文字、图片、gif
static DSPNode_t PPLDSPNODE[PPL_NODESIZE];
static XKDisplayItem PPLSXKDisplay[PPL_SXKSIZE];


static void PLst_StrMsgPrintf(XKCellString *XKCellStr)
{
	debug_printf("XKCellStr->cx = %d\n"
				 "XKCellStr->cy = %d\n"
				 "XKCellStr->nFontType = %c\n"
				 "XKCellStr->nFontSize = %d\n"
				 "XKCellStr->nForeColor: %d | %d | %d \n"
				 "XKCellStr->strContent = %s\n",
				 XKCellStr->cx,
				 XKCellStr->cy,
				 XKCellStr->nFontType,
				 XKCellStr->nFontSize,
				 XKCellStr->nForeColor[0],XKCellStr->nForeColor[1],XKCellStr->nForeColor[2],
				 XKCellStr->strContent
				 );
}

static void InitContentNodeDefVals(XKDisplayItem *head)
{
	Initcontentnode(head);
	head->nDelayTime = 2;
	head->nEffectIn	 = 0;
}

static void PLst_INITStrDefVals(XKCellString * pCellString)
{
	pCellString->cx					= 0;
	pCellString->cy					= 0;
    pCellString->nForeColor[0] 		= 0xff;
	pCellString->nForeColor[1] 		= 0xff;
	pCellString->nForeColor[2] 		= 0x00;
    pCellString->nBkColor[0] 		= 0x00;
	pCellString->nBkColor[1] 		= 0x00;
	pCellString->nBkColor[2] 		= 0x00;
    pCellString->nShadowColor[0] 	= 0x00;
	pCellString->nShadowColor[1] 	= 0x00;
	pCellString->nShadowColor[2] 	= 0x00;
    strcpy(pCellString->strContent,"");
    pCellString->nFontType 			= 's';
    pCellString->nFontSize 			= 32;
    pCellString->nSpace 			= 0;
	pCellString->nLineSpace			= 0;
    pCellString->nCellOrder = 0;
}

static void PLst_INITImgDefVals(XKCellImage * pCellImag)
{
	pCellImag->cx					= 0;
	pCellImag->cy					= 0;
	strcpy(pCellImag->strImage,"");
}


static int PPL_PLst_GetChineseStr(char *itemStr,char *ChineseStr)
{
	int strCounter = 0;
	char *ItemStrP = itemStr;
	char *ChineStrP = ChineseStr;
	while(*ItemStrP != 0)
	{
		if(*ItemStrP != '\\')
		{
			*(ChineStrP++) = *ItemStrP;
			strCounter = strCounter + 1;
			ItemStrP++;
			continue;
		}
		if(*(ItemStrP + 1) == '\\' || *(ItemStrP + 1) == 'n' || *(ItemStrP + 1) == 'N')
		{
			*(ChineStrP++) = *ItemStrP;
			*(ChineStrP++) = *(ItemStrP + 1);
			strCounter = strCounter + 2;
			ItemStrP += 2;
			
			continue;
		}
		break;
	}
	
	ItemStrP	-= 2;
	strCounter	-= 2;
	debug_printf("strCounter = %d\n",strCounter);
	return strCounter;
}


static void PPL_INITDSPNodeDefVals(DSPNode_t *DSPNode)
{
	DSPNode->type = DSPTYPE_DEF;
	DSPNode->Cx = 0;
	DSPNode->Cy = 0;
	DSPNode->Sseq = 0;
	DSPNode->Lseq = 0;
	DSPNode->width = 0;
	DSPNode->height = 0;
	DSPNode->XKCellIma = NULL;
	DSPNode->XKCellStr = NULL;
	DSPNode->pNext = NULL;
}

static void Dir_LetterBtoL(char *dir)
{
	char *charStr = dir;
	char *charp = charStr;
	while(*charp != '\0')
	{
		if(*charp >= 'A' && *charp <= 'Z')
			*charp = *charp + 0x20;
		charp += 1;
	}
}


static int PLL_intemDecode(ContentList *head,char *itemContent,uint8_t ItemOder)
{
	uint8_t i = 0;
	uint8_t BMPtype = 0;
	int ChineseCounter = 0;
	int Cx = 0,Cy = 0;
	char ChineseStr[256];
	int DSPnode_cur = 0,DSPnode_pre = 0;
	uint8_t imgname[4];
	XKDisplayItem 	XKDisplay;
	XKCellString 	XKCellStr;
	XKCellImage 	pCellImag;
	InitContentNodeDefVals(&PPLSXKDisplay[ItemOder]);
	
	memset(PPLDSPNODE,0x00,sizeof(DSPNode_t) * 24);
	for(i = 0 ; i < 24 ; i++)
		PPL_INITDSPNodeDefVals(&PPLDSPNODE[i]);

	
	memset(&XKCellStr,0,sizeof(XKCellString));
	memset(&pCellImag,0,sizeof(XKCellImage));
	PLst_INITStrDefVals(&XKCellStr);
	PLst_INITImgDefVals(&pCellImag);
	
	char *itemData = NULL;
	
	char *str_p = strchr(itemContent,'=');
	if(str_p == NULL)
		return -1;
	
	PPLSXKDisplay[ItemOder].nEffectIn = 1;//立即显示
	itemData = str_p + 1;
	debug_printf("itemData = %s\n",itemData);
	sscanf(itemData,"%d,%d,%d",&PPLSXKDisplay[ItemOder].nDelayTime,&PPLSXKDisplay[ItemOder].nEffectIn,&PPLSXKDisplay[ItemOder].nMoveSpeed);
	
	debug_printf("XKDisplay->nDelayTime = %d,&XKDisplay->nEffectIn = %d,&XKDisplay->nMoveSpeed = %d\n",
		PPLSXKDisplay[ItemOder].nDelayTime,PPLSXKDisplay[ItemOder].nEffectIn,PPLSXKDisplay[ItemOder].nMoveSpeed);
	PPLSXKDisplay[ItemOder].nEffectOut = 1;//立即显示
	
	//PPLSXKDisplay[ItemOder].nEffectIn = 30;

	if(PPLSXKDisplay[ItemOder].nDelayTime == 0)
		PPLSXKDisplay[ItemOder].nDelayTime = 20 * 1000;
	else
		PPLSXKDisplay[ItemOder].nDelayTime = 1000 * 1000 * PPLSXKDisplay[ItemOder].nDelayTime / 100;
	if(PPLSXKDisplay[ItemOder].nEffectIn != 0 && PPLSXKDisplay[ItemOder].nEffectIn != 1 && PPLSXKDisplay[ItemOder].nEffectIn != 25 && 
		PPLSXKDisplay[ItemOder].nEffectIn != 30)
		PPLSXKDisplay[ItemOder].nDelayTime = PPLSXKDisplay[ItemOder].nDelayTime + 20 * PPLSXKDisplay[ItemOder].nMoveSpeed * 1000;
	
	str_p = strchr(itemContent,'\\');
	if(str_p == NULL)
		return -1;
	
	itemData = str_p;
	while(*itemData != 0)
	{
		debug_printf("*itemData = %c\n",*itemData);
		if(*itemData != '\\')
		{
			ChineseCounter = PPL_PLst_GetChineseStr(itemData,XKCellStr.strContent);
			DEBUG_PRINTF;
			XKCellStr.strContent[ChineseCounter] = '\0';
			XKCellStr.strContentLen = ChineseCounter;
			XKCellStr.nCellOrder = ItemOder;
			DEBUG_PRINTF;
			XKCellString *DSPStrNode = (XKCellString *)malloc(sizeof(XKCellString));
			memset(DSPStrNode,0x00,sizeof(XKCellString));
			memcpy(DSPStrNode,&XKCellStr,sizeof(XKCellString));
			
			PPLDSPNODE[DSPnode_cur].Cx = Cx;
			PPLDSPNODE[DSPnode_cur].Cy = Cy;
			PPLDSPNODE[DSPnode_cur].type = DSPTYPE_STR;
			PPLDSPNODE[DSPnode_cur].XKCellStr = DSPStrNode;
			
			itemData += ChineseCounter;
			DSPnode_cur += 1;
			if(*itemData != '\\')
				break;
			continue;
		}
		
		DEBUG_PRINTF;
		itemData += 1;
		switch(*itemData)
		{
			case 'C':
				Cx = (itemData[1] - 0x30) * 100 + (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
				if(itemData[4] == '-')
				{
					Cy = (itemData[5] - 0x30) * 100 + (itemData[6] - 0x30) * 10 + (itemData[7] - 0x30);
					itemData += 8;
				}
				else
				{
					Cy = (itemData[4] - 0x30) * 100 + (itemData[5] - 0x30) * 10 + (itemData[6] - 0x30);
					itemData += 7;
				}
				debug_printf("Cx = %d,Cy = %d\n",Cx,Cy);
				break;

			case 'B':
				pCellImag.cx	= Cx;
				pCellImag.cy	= Cy;
				memcpy(imgname,itemData + 1,3);
				imgname[3] = '\0';
				Dir_LetterBtoL(imgname);
				
				pCellImag.nCellOrder = ItemOder;
				memset(pCellImag.strImage,0x00,sizeof(pCellImag.strImage));

				
				sprintf(pCellImag.strImage,"%s.bmp",imgname);
				pCellImag.strImage[7] = '\0';

				//DSPNODE[DSPnode_cur].width = BMPtype;
				//DSPNODE[DSPnode_cur].height = DSPNODE[DSPnode_cur].width;
				PPLDSPNODE[DSPnode_cur].type = DSPTYPE_IMG;

				XKCellImage *DSPimageNode = (XKCellImage *)malloc(sizeof(XKCellImage));
				memset(DSPimageNode,0x00,sizeof(XKCellImage));
				memcpy(DSPimageNode,&pCellImag,sizeof(XKCellImage));
				PPLDSPNODE[DSPnode_cur].XKCellIma = DSPimageNode;
				DSPnode_cur += 1;
				
				itemData += 4;
				//debug_printf("pCellImag.strImage = %s\n",pCellImag.strImage);
				//AddItemImage(XKDisplay,pCellImag);
				break;

			case 'c':
				if(itemData[1] == 't')
				{
					XKCellStr.nForeColor[0] = XKCellStr.nBkColor[0];
					XKCellStr.nForeColor[1] = XKCellStr.nBkColor[1];
					XKCellStr.nForeColor[2] = XKCellStr.nBkColor[2];
					itemData += 2;
				}
				else
				{
					XKCellStr.nForeColor[0] = (itemData[1] - 0x30) * 100 + (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
					XKCellStr.nForeColor[1] = (itemData[4] - 0x30) * 100 + (itemData[5] - 0x30) * 10 + (itemData[6] - 0x30);
					XKCellStr.nForeColor[2] = (itemData[7] - 0x30) * 100 + (itemData[8] - 0x30) * 10 + (itemData[9] - 0x30);
					itemData += 13;
					debug_printf("XKCellStr->nForeColor[0] = %d,XKCellStr->nForeColor[1] = %d,XKCellStr->nForeColor[2] = %d\n",XKCellStr.nForeColor[0],XKCellStr.nForeColor[1],XKCellStr.nForeColor[2]);
				}

				break;

			case 'b':
				if(itemData[1] == 't')
				{
					XKCellStr.nBkColor[0] = 0x00;
					XKCellStr.nBkColor[1] = 0x00;
					XKCellStr.nBkColor[2] = 0x00;
					itemData += 2;
				}
				else
				{
					XKCellStr.nBkColor[0] = (itemData[1] - 0x30) * 100 + (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
					XKCellStr.nBkColor[1] = (itemData[4] - 0x30) * 100 + (itemData[5] - 0x30) * 10 + (itemData[6] - 0x30);
					XKCellStr.nBkColor[2] = (itemData[7] - 0x30) * 100 + (itemData[8] - 0x30) * 10 + (itemData[9] - 0x30);
					itemData += 13;
				}
				
				break;

			case 'S':
				XKCellStr.nSpace = (itemData[1] - 0x30) * 10 + (itemData[2] - 0x30);
				itemData += 3;
				break;
			case 's':
				if(itemData[1] == 't')
				{
					XKCellStr.nShadowColor[0] = 0x00;
					XKCellStr.nShadowColor[1] = 0x00;
					XKCellStr.nShadowColor[2] = 0x00;
					itemData += 2;
				}
				else
				{
					XKCellStr.nShadowColor[0] = (itemData[1] - 0x30) * 100 + (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
					XKCellStr.nShadowColor[1] = (itemData[4] - 0x30) * 100 + (itemData[5] - 0x30) * 10 + (itemData[6] - 0x30);
					XKCellStr.nShadowColor[2] = (itemData[7] - 0x30) * 100 + (itemData[8] - 0x30) * 10 + (itemData[9] - 0x30);
					itemData += 13;
				}
				break;

			case 'f':
				DEBUG_PRINTF;
				XKCellStr.cx = Cx;
				XKCellStr.cy = Cy;
				
				XKCellStr.nFontType = itemData[1];
				XKCellStr.nFontSize = (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
				debug_printf("XKCellStr->nFontType = %c,XKCellStr->nFontSize = %d\n",XKCellStr.nFontType,XKCellStr.nFontSize);
				itemData += 6;
				break;
			case 'F':
				debug_printf("Curently is not suport for FLC format flash!\n");
				break;

			case 'n':
				break;

			default:
				break;
				
		}
	}
	//PLst_StrMsgPrintf(XKCellStr);
	//AddDisplayItem(head,XKDisplay);

	//free(XKDisplay);
	//free(XKCellStr);
	//free(pCellImag);
	for(i = 0 ; i < DSPnode_cur ; i++)
		AddItemDSPNode(&PPLSXKDisplay[ItemOder],&PPLDSPNODE[i]);
	
	return 0;
}

int PPL_PLst_parsing(ContentList *head,const char *plist)
{
	uint8_t i = 0;
	int  Snumber = 0;
	int  breakflag = 0;
	char *s = NULL;
	FILE *stream;
	char strcontent[2048];
	int  itemCount = 0;
	int  itemOrder = 0;
	int	 lstStatus = LST_HEAD;
	debug_printf("parsing the playlist!\n");
	
	stream = fopen(plist, "r");
	if(stream == NULL)
	{
		DEBUG_PRINTF;
		return -1;
	}
	memset(PPLSXKDisplay,0,sizeof(XKDisplayItem) * PPL_SXKSIZE);

	while(1)
	{
		s = fgets(strcontent, 2048, stream);
		if(s == NULL)
			break;

		debug_printf("strItem = %s\n",strcontent);
		switch(lstStatus)
		{
			case LST_HEAD:
				if(strncmp(strcontent,"[playlist]",10) != 0)
					return -1;
				lstStatus = LST_COUNT;
				DEBUG_PRINTF;
				break;

			case LST_COUNT:
				if(strncmp(strcontent,"item_no",7) != 0 && strncmp(strcontent,"Item_No",7) != 0)
				{
					debug_printf("strcontent = %s\n",strcontent);
					DEBUG_PRINTF;
					goto FREE_RESCR;
				}
				DEBUG_PRINTF;
				char *p = strstr(strcontent,"=");
				if(p == NULL)
					goto FREE_RESCR;
				
				itemCount = atoi(p+1);
				head->itemcount = itemCount;				
				lstStatus = LST_PARSE;
				debug_printf("itemCount = %d\n",itemCount);
				DEBUG_PRINTF;
				break;

			case LST_PARSE:
				debug_printf("itemOrder = %d\n",itemOrder);				
				//DecodeItemString(head,strcontent);
				PLL_intemDecode(head,strcontent,itemOrder);
				itemOrder++;

				if(itemOrder >= itemCount)
				{
					Snumber = itemOrder;
					breakflag = 1;
					break;
				}
				
				debug_printf("itemoder = %d\n",itemOrder);
				lstStatus = LST_PARSE;
				DEBUG_PRINTF;
				break;
				
			default:
				break;
		}	
		
		if(breakflag)
			break;
		
	}
	
	DEBUG_PRINTF;
	fclose(stream);

	pthread_mutex_lock(&content_lock);
	debug_printf("##itemOrder = %d\n",Snumber);
	ClearContent(head);
	DEBUG_PRINTF;
	for(i = 0 ; i < Snumber ; i++)
		AddDisplayItem(head,&PPLSXKDisplay[i]);
	DEBUG_PRINTF;
	head->itemcount = Snumber;
	head->refresh = LST_REFLASH;
	pthread_mutex_unlock(&content_lock);
	DEBUG_PRINTF;
	return 0;
	
	FREE_RESCR:
		fclose(stream);
		return -1;
}

