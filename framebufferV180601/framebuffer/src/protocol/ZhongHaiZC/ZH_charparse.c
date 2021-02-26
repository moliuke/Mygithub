#include <stdio.h>
#include "ZH_charparse.h"
#include "../../Hardware/Data_pool.h"

static DSPNode_t ZHDSPNODE[8];
static XKDisplayItem ZHSXKDisplay[1];


static void ZH_DefInitcontentnode(XKDisplayItem *head)
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

	head->nDelayTime		= 2;
	head->nMoveSpeed		= 0;
}


static void ZH_INITDSPNodeDefVals(DSPNode_t *DSPNode)
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

static void ZH_INITStrDefVals(XKCellString * pCellString)
{
	//默认在原点坐标显示
	pCellString->cx					= 0;
	pCellString->cy					= 0;
	//无闪烁
	pCellString->flash				= 0;
	//默认字体颜色是黄色
    pCellString->nForeColor[0] 		= 0xff;
	pCellString->nForeColor[1] 		= 0xff;
	pCellString->nForeColor[2] 		= 0x00;
	//默认背景颜色是黑色
    pCellString->nBkColor[0] 		= 0x00;
	pCellString->nBkColor[1] 		= 0x00;
	pCellString->nBkColor[2] 		= 0x00;
	//默认阴影颜色是黑色
    pCellString->nShadowColor[0] 	= 0x00;
	pCellString->nShadowColor[1] 	= 0x00;
	pCellString->nShadowColor[2] 	= 0x00;

	//默认无字符
    strcpy(pCellString->strContent,"");

	//默认宋体、大小16、无间距、无行距
    pCellString->nFontType 			= 's';
    pCellString->nFontSize 			= 32;
    pCellString->nSpace 			= 0;
	pCellString->nLineSpace			= 0;
    pCellString->nCellOrder = 0;
}


static void set_Bcolor(XKCellString *XKCellStr,char Bcolor)
{
	XKCellStr->nBkColor[0] = (Bcolor == 'r' || Bcolor == 'w') ? 0xff : 0x00;
	XKCellStr->nBkColor[1] = (Bcolor == 'g' || Bcolor == 'w') ? 0xff : 0x00;
	XKCellStr->nBkColor[2] = (Bcolor == 'b' || Bcolor == 'w') ? 0xff : 0x00;
}

static void set_Fcolor(XKCellString *XKCellStr,uint8_t r,uint8_t g,uint8_t b)
{
	XKCellStr->nForeColor[0] = r;
	XKCellStr->nForeColor[1] = g;
	XKCellStr->nForeColor[2] = b;
}

static void set_Fontsize(XKCellString *XKCellStr,uint8_t size)
{
	switch(size)
	{
		case 16:XKCellStr->nFontSize = 16;break;
		case 24:XKCellStr->nFontSize = 24;break;
		case 32:XKCellStr->nFontSize = 32;break;
		case 48:XKCellStr->nFontSize = 48;break;
		case 64:XKCellStr->nFontSize = 64;break;
		default:break;
	}
}


//计算能显示字符的数量，汉子占两个字节，ascii占1个字节。超过屏幕宽度的部分忽略不计
static int set_contentLen(uint8_t *content,uint8_t Fontsize,uint16_t Len,uint16_t swidth)
{
	uint8_t charSize = Fontsize / 2;
	uint16_t contentLen = Len;
	//内容长度小于屏宽
	if(Len * charSize <= swidth)
		return Len;

	//最后一个字节属于字母
	contentLen = swidth / charSize;
	if(content[contentLen - 1] < 0xa0)
		return contentLen;

	//最后一个字节非字母但倒数第二个字节却是字母，则将最后一个表示汉字的字节删掉
	if(content[contentLen - 2] < 0xa0)
		return contentLen - 1;

	//最后两个字节均为汉字
	return contentLen;
}

int ZH_Lstparsing(uint8_t *charStr,uint16_t Len,ContentList *head)
{
	int i = 0;
	int Cx,Cy;
	char FontSize;
	//这里有的怪，为啥要这么弄
	//char FontSize = (charStr[12] == 24) ? 24 : 16;
	if(charStr[12] == 16 || charStr[12] == 24 || charStr[12] == 32 || charStr[12] == 48)
		FontSize = charStr[12];
	else
		FontSize = 16;
	int Swidth = 0,Sheight = 0;
	XKCellString 	XKCellStr;

	uint32_t Aswidth,Asheight;
	DP_GetScreenSize(&Aswidth,&Asheight);

#if 1
	debug_printf("printf in ZH_Lstparsing\n");
	for(i = 0 ; i < Len ; i++)
		debug_printf("%x ",charStr[i]);
	debug_printf("\n");
#endif	
	
	memset(&ZHSXKDisplay[0],0,sizeof(XKDisplayItem));
	ZH_DefInitcontentnode(&ZHSXKDisplay[0]);
	memset(ZHDSPNODE,0x00,sizeof(DSPNode_t) * 8);
	for(i = 0 ; i < 8 ; i++)
		ZH_INITDSPNodeDefVals(&ZHDSPNODE[i]);

	memset(&XKCellStr,0,sizeof(XKCellString));
	ZH_INITStrDefVals(&XKCellStr);
	uint16_t contentLen = Len - 17;
	Swidth = charStr[5] << 8 | charStr[4];
	Sheight = charStr[7] << 8 | charStr[6];

	

	debug_printf("Aswidth = %d,Swidth = %d\n",Aswidth,Swidth);

	set_Bcolor(&XKCellStr,'h');
	set_Fcolor(&XKCellStr,charStr[14],charStr[15],charStr[16]);
	set_Fontsize(&XKCellStr,FontSize);

	if(charStr[8] = 0xCB && charStr[9] == 0xCE)
		XKCellStr.nFontType = 's';
	if(charStr[8] = 0xBA && charStr[9] == 0xDA)
		XKCellStr.nFontType = 'h';
	if(charStr[8] = 0xBF && charStr[9] == 0xAC)
		XKCellStr.nFontType = 'k';

	Cx = charStr[1] << 8 | charStr[0];
	Cy = charStr[3] << 8 | charStr[2];
	

	debug_printf("Cx = %d,Cy = %d\n",Cx,Cy);
	//exit(1);
	
	ZHSXKDisplay[0].nMoveSpeed = 100;
	ZHSXKDisplay[0].nEffectOut = 1;//立即显示
	ZHSXKDisplay[0].nEffectIn= 1;//立即显示
	ZHSXKDisplay[0].nDelayTime = 1000 * 1000 * 10;//停留时间，10s
	

	char *p = NULL;
	char *dspstr = charStr + 17;
	char *dspstr_p = dspstr;
	int remainLen = contentLen;
	int realLen = 0;
	int curNode = 0;
	int CurCy = 0;
	int totalHeight = 0;
	while(remainLen > 0)
	{
		debug_printf("Len = %d,contentLen = %d,remainLen = %d\n",Len,contentLen,remainLen);
		p = strstr(dspstr_p,"\\n");
		if(p != NULL)
		{
			DEBUG_PRINTF;
			contentLen = p - dspstr_p;
			realLen = contentLen;
		}
		else
		{
			DEBUG_PRINTF;
			contentLen = remainLen;
			realLen = remainLen;
		}
#if 1
		for(i = 0 ; i < contentLen ; i++)
			debug_printf("|%x ",(uint8_t)dspstr_p[i]);
		debug_printf("\n");
#endif
		remainLen -= contentLen;

		//取显示内容,内容长度超过屏幕宽度将被截掉
		contentLen = set_contentLen(dspstr_p,FontSize,contentLen,Aswidth);
		memcpy(XKCellStr.strContent,dspstr_p,contentLen);
		XKCellStr.strContentLen = contentLen;
		XKCellStr.strContent[XKCellStr.strContentLen] = '\0';
		XKCellString *DSPStrNode = (XKCellString *)malloc(sizeof(XKCellString));
		memset(DSPStrNode,0x00,sizeof(XKCellString));
		memcpy(DSPStrNode,&XKCellStr,sizeof(XKCellString));
		ZHDSPNODE[curNode].width = (FontSize / 2) * contentLen;
		ZHDSPNODE[curNode].height = FontSize;

		//左右居中
		ZHDSPNODE[curNode].Cx = (Aswidth - ZHDSPNODE[curNode].width) / 2;
		ZHDSPNODE[curNode].Cy = Cy + CurCy;
		
		ZHDSPNODE[curNode].type = DSPTYPE_STR;
		ZHDSPNODE[curNode].XKCellStr = DSPStrNode;
		AddItemDSPNode(&ZHSXKDisplay[0],&ZHDSPNODE[curNode]);
		totalHeight += FontSize;
		CurCy += FontSize;
		dspstr_p 	+= (realLen + 2);
		if(p != NULL)
			remainLen 	-= 2;
		curNode 	+= 1;


		//上下居中
		int yoffset = (Asheight >= totalHeight) ? ((Asheight - totalHeight) / 2) : 0;
		debug_printf("yoffset is %d real height is %d scream is %d\n",yoffset,Asheight,totalHeight);
		for(i = 0 ; i < curNode ; i++)
			ZHDSPNODE[curNode].Cy += yoffset;
	
	}
	pthread_mutex_lock(&content_lock);
	ClearContent(head);
	AddDisplayItem(head,&ZHSXKDisplay[0]);
	head->itemcount = 1;
	head->refresh = LST_REFLASH;
	pthread_mutex_unlock(&content_lock);
	//exit(1);
	return 0;
}

int ZH_PLst_parsing(ContentList *head,const char *plist)
{
	uint8_t i = 0;
	char strcontent[1028];
	char lenbuf[4];
	int strlength = 0;
	FILE *stream;
	char *s;
	//int num;
	stream = fopen(plist,"r");
	if(stream == NULL)
	{		
		DEBUG_PRINTF;
		return -1;	
	}

	fseek(stream,0,SEEK_SET);
	memset(ZHSXKDisplay,0,sizeof(ZHSXKDisplay));
	memset(ZHDSPNODE,0,sizeof(DSPNode_t)*8);
	memset(strcontent,0,sizeof(strcontent));
	memset(lenbuf,0,sizeof(lenbuf));
	s = fgets(strcontent,1028,stream);

	memcpy(lenbuf,strcontent,3);
	lenbuf[3] = '\0';
	debug_printf("lenbuf is %s\n",lenbuf);

	strlength = atoi(lenbuf);
	if(s == NULL)
	{
		fclose(stream);
		return -1;
	}
	
	debug_printf("strlen is %d  strITem = %s\n",strlength,strcontent+20);

	ZH_Lstparsing(strcontent+3,strlength,head);

	fclose(stream);
	//DEBUG_PRINTF;
	//XKDisplayInsert(&ZHSXKDisplay,1);
	return 0;
}




