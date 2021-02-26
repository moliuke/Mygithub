#include <stdio.h>

#include "JX_Lstparse.h"
#include "debug.h"


#define JX_SXKSIZE		500		//一个播放列表里面最多限制在幕,即48条Item
#define JX_NODESIZE		24		//一条Item里面最多容许24个信息:文字、图片、gif
static DSPNode_t JXDSPNODE[JX_NODESIZE];
static XKDisplayItem JXSXKDisplay[JX_SXKSIZE];

static Curplay_t Curplay;

static void SetCurPlaying(void *arg)
{
	Curplay_t *playing = (Curplay_t *)arg;
	memcpy(&Curplay,playing,sizeof(Curplay_t));
	debug_printf("Curplay.stoptime = %d,Curplay.strLen = %d,Curplay.playstr = %s\n",Curplay.stoptime,Curplay.strLen,Curplay.playstr);
}

void JX_GetCurPlaying(Curplay_t * curplay)
{
	memcpy(curplay,&Curplay,sizeof(Curplay_t));
}


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
    strcpy(pCellString->strContent," ");
	pCellString->strContent[1] = '\0';
	pCellString->strContentLen = 1;
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


int JX_PLst_GetChineseStr(char *itemStr,char *ChineseStr)
{
	int strCounter = 0;
	char *ItemStrP = itemStr;
	char *ChineStrP = ChineseStr;
	
	while(*ItemStrP != 0)
	{
		if(*ItemStrP == 0xd && *(ItemStrP+1) == 0x0a)
			break;
		
		if(*ItemStrP != '\\')
		{		
			//debug_printf("1: %x \n",(uint8_t)*ItemStrP);
			*(ChineStrP++) = *ItemStrP;
			strCounter = strCounter + 1;
			ItemStrP++;
			continue;
		}

		if(*(ItemStrP + 1) == '\\' && *(ItemStrP + 2) == 'N')
		{
			//debug_printf("2 : %x \n",(uint8_t)*ItemStrP);
			*(ChineStrP++) = *(ItemStrP + 1);
			*(ChineStrP++) = *(ItemStrP + 2);
			strCounter = strCounter + 2;
			ItemStrP += 3;
			continue;
		}
		
		else if(*(ItemStrP + 1) == '\\' || *(ItemStrP + 1) == 'n' || *(ItemStrP + 1) == 'N')
		{
			//debug_printf("3 : %x \n",(uint8_t)*ItemStrP);
			*(ChineStrP++) = *ItemStrP;
			*(ChineStrP++) = *(ItemStrP + 1);
			strCounter = strCounter + 2;
			ItemStrP += 2;
			
			continue;
		}
		
		break;
	}

	ChineseStr[strCounter] = '\0';
	//ItemStrP	-= 2;
	//strCounter	-= 2;
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


//黑屏的一幕，参数为幕序
static int BlackScreen(uint8_t scr_order)
{
	XKCellString *XKCellStr = (XKCellString *)malloc(sizeof(XKCellString));
	memset(XKCellStr,0x00,sizeof(XKCellString));
	PLst_INITStrDefVals(XKCellStr);
	JXDSPNODE[0].Cx = 0;
	JXDSPNODE[0].Cy = 0;
	JXDSPNODE[0].type = DSPTYPE_STR;
	JXDSPNODE[0].XKCellStr = XKCellStr;
	AddItemDSPNode(&JXSXKDisplay[scr_order],&JXDSPNODE[0]);
	
	Curplay_t *ThisPlay = (Curplay_t *)malloc(sizeof(Curplay_t));
	memset(ThisPlay,0,sizeof(Curplay_t));
	JXSXKDisplay[scr_order].nDelayTime = 5000000;
	JXSXKDisplay[scr_order].nEffectIn = 1;
	JXSXKDisplay[scr_order].Curplay = (void *)ThisPlay;
	JXSXKDisplay[scr_order].setcurplaying = SetCurPlaying;
	return 0;
}



static int JX_intemDecode(ContentList *head,char *itemContent,uint8_t ItemOder)
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

	InitContentNodeDefVals(&JXSXKDisplay[ItemOder]);
	
	//memset(JXDSPNODE,0x00,sizeof(DSPNode_t) * 24);
	for(i = 0 ; i < 24 ; i++)
		PPL_INITDSPNodeDefVals(&JXDSPNODE[i]);

	
	memset(&XKCellStr,0,sizeof(XKCellString));
	memset(&pCellImag,0,sizeof(XKCellImage));
	PLst_INITStrDefVals(&XKCellStr);
	PLst_INITImgDefVals(&pCellImag);
	
	char *itemData = NULL;
	
	char *str_p = strchr(itemContent,'=');
	if(str_p == NULL)
		return -1;
	
	itemData = str_p + 1;
	debug_printf("itemData = %s\n",itemData);
	sscanf(itemData,"%d,%d,%d",&JXSXKDisplay[ItemOder].nDelayTime,&JXSXKDisplay[ItemOder].nEffectIn,&JXSXKDisplay[ItemOder].nMoveSpeed);

	//当出字方式为0时黑屏
	if(JXSXKDisplay[ItemOder].nEffectIn == 0)
	{
		BlackScreen(ItemOder);
		debug_printf("ItemOder = %d\n",ItemOder);
		return 0;
	}
	
	debug_printf("XKDisplay->nDelayTime = %d,&XKDisplay->nEffectIn = %d,&XKDisplay->nMoveSpeed = %d\n",
		JXSXKDisplay[ItemOder].nDelayTime,JXSXKDisplay[ItemOder].nEffectIn,JXSXKDisplay[ItemOder].nMoveSpeed);
	JXSXKDisplay[ItemOder].nEffectOut = 1;//立即显示

	if(JXSXKDisplay[ItemOder].nDelayTime == 0)
		JXSXKDisplay[ItemOder].nDelayTime = 20 * 1000;
	else
		JXSXKDisplay[ItemOder].nDelayTime = 1000 * 1000 * JXSXKDisplay[ItemOder].nDelayTime / 100;
	if(JXSXKDisplay[ItemOder].nEffectIn != 0 && JXSXKDisplay[ItemOder].nEffectIn != 1 && JXSXKDisplay[ItemOder].nEffectIn != 25 && 
		JXSXKDisplay[ItemOder].nEffectIn != 30)
		JXSXKDisplay[ItemOder].nDelayTime = JXSXKDisplay[ItemOder].nDelayTime + 20 * JXSXKDisplay[ItemOder].nMoveSpeed * 1000;
	
	str_p = strchr(itemContent,'\\');
	if(str_p == NULL)
		return -1;

	//设置当前幕显示的内容
	Curplay_t *ThisPlay = (Curplay_t *)malloc(sizeof(Curplay_t));
	memset(ThisPlay,0,sizeof(Curplay_t));
	ThisPlay->order = ItemOder;
	ThisPlay->stoptime = JXSXKDisplay[ItemOder].nDelayTime / 10000;
	ThisPlay->effectin = JXSXKDisplay[ItemOder].nEffectIn;
	ThisPlay->strLen = (strlen(str_p) > 1024) ? 1024 : strlen(str_p);
	memcpy(ThisPlay->playstr,str_p,ThisPlay->strLen);
	JXSXKDisplay[ItemOder].Curplay = (void *)ThisPlay;
	JXSXKDisplay[ItemOder].setcurplaying = SetCurPlaying;

	//开始解码
	itemData = str_p;
	while(*itemData != 0)
	{
		DEBUG_PRINTF;
		debug_printf("*itemData = %c\n",*itemData);
		if(*itemData != '\\')
		{
			DEBUG_PRINTF;
			ChineseCounter = JX_PLst_GetChineseStr(itemData,XKCellStr.strContent);
			XKCellStr.strContent[ChineseCounter] = '\0';
			XKCellStr.strContentLen = ChineseCounter;
			XKCellStr.nCellOrder = ItemOder;

			
			XKCellString *DSPStrNode = (XKCellString *)malloc(sizeof(XKCellString));
			memset(DSPStrNode,0x00,sizeof(XKCellString));
			memcpy(DSPStrNode,&XKCellStr,sizeof(XKCellString));

			JXDSPNODE[DSPnode_cur].Cx = Cx;
			JXDSPNODE[DSPnode_cur].Cy = Cy;
			JXDSPNODE[DSPnode_cur].type = DSPTYPE_STR;
			JXDSPNODE[DSPnode_cur].XKCellStr = DSPStrNode;
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
				Cy = (itemData[4] - 0x30) * 100 + (itemData[5] - 0x30) * 10 + (itemData[6] - 0x30);
				itemData += 7;
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

				JXDSPNODE[DSPnode_cur].type = DSPTYPE_IMG;
				XKCellImage *DSPimageNode = (XKCellImage *)malloc(sizeof(XKCellImage));
				memset(DSPimageNode,0x00,sizeof(XKCellImage));
				memcpy(DSPimageNode,&pCellImag,sizeof(XKCellImage));
				JXDSPNODE[DSPnode_cur].XKCellIma = DSPimageNode;
				DSPnode_cur += 1;
				
				itemData += 4;
				break;

			case 'c':
				if(itemData[1] == 't')
				{
					//XKCellStr.nForeColor[0] = XKCellStr.nBkColor[0];
					//XKCellStr.nForeColor[1] = XKCellStr.nBkColor[1];
					//XKCellStr.nForeColor[2] = XKCellStr.nBkColor[2];
					XKCellStr.nForeColor[0] = 255;
					XKCellStr.nForeColor[1] = 255;
					XKCellStr.nForeColor[2] = 0;

					
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
				DEBUG_PRINTF;
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
				DEBUG_PRINTF;
				
				break;

			case 'S':
				XKCellStr.nSpace = (itemData[1] - 0x30) * 10 + (itemData[2] - 0x30);
				itemData += 3;
				debug_printf("XKCellStr.nSpace = %d\n",XKCellStr.nSpace);
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
				//exit(1);
				itemData += 6;
				break;
			case 'F':
				debug_printf("Curently is not suport for FLC format flash!\n");
				break;

			case 'n':
			//	XKCellStr.cy += XKCellStr.nFontSize;
				itemData += 1;
				break;

			default:
				break;
				
		}
	}
	
	for(i = 0 ; i < DSPnode_cur ; i++)
		AddItemDSPNode(&JXSXKDisplay[ItemOder],&JXDSPNODE[i]);
	
	return 0;
}




int JX_PLst_parsing(ContentList *head,const char *plist)
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
	debug_printf("parsing the playlist!\n%s\n",plist);
	
	stream = fopen(plist, "r");
	if(stream == NULL)
	{
		DEBUG_PRINTF;
		return -1;
	}
	//ClearContent(head);
	//head->refresh = FLAG_RESFRESH;
	fseek(stream, 0, SEEK_SET);
	memset(JXSXKDisplay,0,sizeof(XKDisplayItem) * JX_SXKSIZE);
	memset(JXDSPNODE,0,sizeof(DSPNode_t) * JX_NODESIZE);

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
				//当item条数为0时，黑屏,跳出后面的解析
				if(itemCount == 0)
				{
					DEBUG_PRINTF;
					BlackScreen(0);
					Snumber = 1;
					breakflag = 1;
					break;
				}
				
				head->itemcount = itemCount;				
				lstStatus = LST_PARSE;
				debug_printf("itemCount = %d\n",itemCount);
				DEBUG_PRINTF;
				break;

			case LST_PARSE:
				debug_printf("itemOrder = %d\n",itemOrder);				
				JX_intemDecode(head,strcontent,itemOrder);
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
	
	fclose(stream);
	DEBUG_PRINTF;
	//这里应该用错了
	XKDisplayInsert(JXSXKDisplay,Snumber);
	DEBUG_PRINTF;
	return 0;
	
	FREE_RESCR:
		fclose(stream);
		return -1;
		
}

