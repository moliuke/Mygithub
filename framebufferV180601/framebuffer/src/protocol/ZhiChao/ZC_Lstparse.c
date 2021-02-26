#include "ZC_Lstparse.h"
#include <stdio.h>
#include "debug.h"
#include "ZC_display.h"


static AHCurplay_t AHCurplay;

static void AHSetCurPlaying(void *arg)
{
	AHCurplay_t *playing = (AHCurplay_t *)arg;
	memcpy(&AHCurplay,playing,sizeof(AHCurplay_t));
	debug_printf("Curplay.stoptime = %d,Curplay.strLen = %d,Curplay.playstr = %s\n",AHCurplay.stoptime,AHCurplay.strLen,AHCurplay.playstr);
}

void AHGetCurPlaying(AHCurplay_t * curplay)
{
	memcpy(curplay,&AHCurplay,sizeof(AHCurplay_t));
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


static void ZC_StrMsgPrintf(XKCellString *XKCellStr)
{
	debug_printf("\n\n"
				 "XKCellStr->cx = %d\n"
				 "XKCellStr->cy = %d\n"
				 "XKCellStr->flash = %d\n"
				 "XKCellStr->nSpace = %d\n"
				 "XKCellStr->nFontType = %c\n"
				 "XKCellStr->nFontSize = %d\n"
				 "XKCellStr->nForeColor: %d | %d | %d \n"
				 "XKCellStr->nBkColor: %d | %d | %d \n"
				 "XKCellStr->strContent = %s\n"
				 "\n\n",
				 XKCellStr->cx,
				 XKCellStr->cy,
				 XKCellStr->flash,
				 XKCellStr->nSpace,
				 XKCellStr->nFontType,
				 XKCellStr->nFontSize,
				 XKCellStr->nForeColor[0],XKCellStr->nForeColor[1],XKCellStr->nForeColor[2],
				 XKCellStr->nBkColor[0],  XKCellStr->nBkColor[1],  XKCellStr->nBkColor[2],
				 XKCellStr->strContent
				 );
}


static void ZC_DefInitcontentnode(XKDisplayItem *head)
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

//�ַ�Ĭ��������Ϣ
static void ZC_INITStrDefVals(XKCellString * pCellString)
{
	//Ĭ����ԭ��������ʾ
	pCellString->cx					= 0;
	pCellString->cy					= 0;
	//����˸
	pCellString->flash				= 0;
	//Ĭ��������ɫ�ǻ�ɫ
    pCellString->nForeColor[0] 		= 0xff;
	pCellString->nForeColor[1] 		= 0xff;
	pCellString->nForeColor[2] 		= 0x00;
	//Ĭ�ϱ�����ɫ�Ǻ�ɫ
    pCellString->nBkColor[0] 		= 0x00;
	pCellString->nBkColor[1] 		= 0x00;
	pCellString->nBkColor[2] 		= 0x00;
	//Ĭ����Ӱ��ɫ�Ǻ�ɫ
    pCellString->nShadowColor[0] 	= 0x00;
	pCellString->nShadowColor[1] 	= 0x00;
	pCellString->nShadowColor[2] 	= 0x00;

	//Ĭ�����ַ�
    strcpy(pCellString->strContent,"");

	//Ĭ�����塢��С16���޼�ࡢ���о�
    pCellString->nFontType 			= 's';
    pCellString->nFontSize 			= 16;
    pCellString->nSpace 			= 0;
	pCellString->nLineSpace			= 0;
    pCellString->nCellOrder = 0;
}

//Ĭ��ͼƬ��Ϣ
static void ZC_INITImgDefVals(XKCellImage * pCellImag)
{
	pCellImag->cx					= 0;
	pCellImag->cy					= 0;
	strcpy(pCellImag->strImage,"");
}





int ZC_GetChineseStr(char *itemStr,char *ChineseStr)
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
	//ItemStrP	-= 2;
	//strCounter	-= 2;
	debug_printf("strCounter = %d\n",strCounter);
	return strCounter;
}


static void ZC_INITDSPNodeDefVals(DSPNode_t *DSPNode)
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

#define ZCSXKSIZE		48		//һ�������б��������������Ļ,��48��Item
#define ZCNODESIZE	24		//һ��Item�����������24����Ϣ:���֡�ͼƬ��gif
static DSPNode_t ZCDSPNODE[ZCNODESIZE];
static XKDisplayItem ZCSXKDisplay[ZCSXKSIZE];


static int PLst_intemDecode(ContentList *head,char *itemContent,uint8_t ItemOder)
{
	int i = 0;
	int DSPnode_cur = 0,DSPnode_pre = 0;
	int ChineseCounter = 0;
	int Cx = 0,Cy = 0;
	char ChineseStr[256];
	char *itemData = NULL;
	uint8_t imgname[4];
	//XKDisplayItem 	*XKDisplay = (XKDisplayItem *)malloc(sizeof(XKDisplayItem));
	XKCellString 	XKCellStr;
	XKCellImage 	pCellImag;

	if(ItemOder > ZCSXKSIZE)
		return 0;
	
	ZC_DefInitcontentnode(&ZCSXKDisplay[ItemOder]);
	//����ʾ���һ��һ����Ϣ��DSPNODE����
	memset(ZCDSPNODE,0x00,sizeof(DSPNode_t) * 24);
	for(i = 0 ; i < 24 ; i++)
		ZC_INITDSPNodeDefVals(&ZCDSPNODE[i]);
	
	memset(&XKCellStr,0,sizeof(XKCellString));
	memset(&pCellImag,0,sizeof(XKCellImage));
	ZC_INITStrDefVals(&XKCellStr);
	ZC_INITImgDefVals(&pCellImag);
	
	debug_printf("PLst_intemDecode : itemContent = %s\n",itemContent);

	char *str_p = strchr(itemContent,'=');
	if(str_p == NULL)
		return -1;

	itemData = str_p + 1;
	debug_printf("itemData = %s\n",itemData);
	sscanf(itemData,"%d,%d,%d",&ZCSXKDisplay[ItemOder].nDelayTime,&ZCSXKDisplay[ItemOder].nEffectIn,&ZCSXKDisplay[ItemOder].nMoveSpeed);
	
	debug_printf("1XKDisplay->nDelayTime = %d,&XKDisplay->nEffectIn = %d,&XKDisplay->nMoveSpeed = %d\n",
		ZCSXKDisplay[ItemOder].nDelayTime,ZCSXKDisplay[ItemOder].nEffectIn,ZCSXKDisplay[ItemOder].nMoveSpeed);

	//���ַ�ʽΪ0ʱ������Ĳ���������Ч
	if(ZCSXKDisplay[ItemOder].nEffectIn == 0)
		return -1;
	
	//XKDisplay->nEffectIn  = 1;//������ʾ
	ZCSXKDisplay[ItemOder].nEffectOut = 1;//������ʾ
	ZCSXKDisplay[ItemOder].nDelayTime = 1000 * 1000 * ZCSXKDisplay[ItemOder].nDelayTime / 100;//ͣ��ʱ�䣬��λ1/100��
	debug_printf("2XKDisplay->nDelayTime = %d,&XKDisplay->nEffectIn = %d,&XKDisplay->nMoveSpeed = %d\n",
		ZCSXKDisplay[ItemOder].nDelayTime,ZCSXKDisplay[ItemOder].nEffectIn,ZCSXKDisplay[ItemOder].nMoveSpeed);

	//XKDisplay->nMoveSpeedÿ����1��ͣ��ʱ�������20ms���˴�Ҫת����us
	if(ZCSXKDisplay[ItemOder].nEffectIn != 0 && ZCSXKDisplay[ItemOder].nEffectIn != 1)
		ZCSXKDisplay[ItemOder].nDelayTime += ZCSXKDisplay[ItemOder].nMoveSpeed * 20 * 1000;
	
	//����������Ҫ��ʾ(��Э��Ҫ��������ʱ����һ����������˴�ֱ�Ӻ��Ե���������)
	str_p = strchr(itemContent,'\\'); 
	if(str_p == NULL) 
		return -1;

	//���õ�ǰĻ��ʾ������
	AHCurplay_t *ThisPlay = (AHCurplay_t *)malloc(sizeof(AHCurplay_t));
	memset(ThisPlay,0,sizeof(AHCurplay_t));
	ThisPlay->order = ItemOder;
	ThisPlay->stoptime = ZCSXKDisplay[ItemOder].nDelayTime / 10000;
	ThisPlay->effectin = ZCSXKDisplay[ItemOder].nEffectIn;
	ThisPlay->strLen = (strlen(str_p) > AHSTRLEN) ? AHSTRLEN : strlen(str_p);
	memcpy(ThisPlay->playstr,str_p,ThisPlay->strLen);
	ZCSXKDisplay[ItemOder].Curplay = (void *)ThisPlay;
	ZCSXKDisplay[ItemOder].setcurplaying = AHSetCurPlaying;
	

	itemData = str_p;
	while(*itemData != 0)
	{
		debug_printf("*itemData = %c\n",*itemData);
		if(*itemData != '\\')
		{
			debug_printf("itemData = %s\n",itemData);
			ChineseCounter = ZC_GetChineseStr(itemData,XKCellStr.strContent);
			debug_printf("ChineseCounter = %d\n",ChineseCounter);
			XKCellStr.strContent[ChineseCounter] = '\0';
			XKCellStr.strContentLen = ChineseCounter;
			XKCellStr.nCellOrder = ItemOder;

			XKCellString *DSPStrNode = (XKCellString *)malloc(sizeof(XKCellString));
			memset(DSPStrNode,0x00,sizeof(XKCellString));
			memcpy(DSPStrNode,&XKCellStr,sizeof  (XKCellString));
			
			ZCDSPNODE[DSPnode_cur].Cx = Cx;
			ZCDSPNODE[DSPnode_cur].Cy = Cy;
			ZCDSPNODE[DSPnode_cur].type = DSPTYPE_STR;
			ZCDSPNODE[DSPnode_cur].XKCellStr = DSPStrNode;
			debug_printf("itemData = %s\n",itemData);
			itemData += ChineseCounter + 1;
			
			debug_printf("itemData = %s\n",itemData);
			DSPnode_cur += 1;
		//	exit(1);
			if(*itemData != '\\')
				break;
			
			continue;
		}
		DEBUG_PRINTF;
		itemData += 1;
		
		switch(*itemData)
		{
			//��ʾ����
			case 'C':
				Cx = (itemData[1] - 0x30) * 100 + (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
				Cy = (itemData[4] - 0x30) * 100 + (itemData[5] - 0x30) * 10 + (itemData[6] - 0x30);
				itemData += 7;
				break;

			//ͼƬ���
			case 'B':
				pCellImag.cx	= Cx;
				pCellImag.cy	= Cx;
				//memcpy(imgname,itemData + 1,3);
				//imgname[3] = '\0';
				//Dir_LetterBtoL(imgname);

				
				
				memcpy(pCellImag.strImage,itemData + 1,3);
				pCellImag.strImage[3] = '\0';
				Dir_LetterBtoL(pCellImag.strImage);
				pCellImag.nCellOrder = ItemOder;
				
				ZCDSPNODE[DSPnode_cur].Cx = Cx;
				ZCDSPNODE[DSPnode_cur].Cy = Cy;
				//ZCDSPNODE[DSPnode_cur].width = BMPtype;
				//ZCDSPNODE[DSPnode_cur].height = DSPNODE[DSPnode_cur].width;
				ZCDSPNODE[DSPnode_cur].type = DSPTYPE_IMG;

				XKCellImage *DSPimageNode = (XKCellImage *)malloc(sizeof(XKCellImage));
				memset(DSPimageNode,0x00,sizeof(XKCellImage));
				memcpy(DSPimageNode,&pCellImag,sizeof(XKCellImage));
				ZCDSPNODE[DSPnode_cur].XKCellIma = DSPimageNode;
				DSPnode_cur += 1;
				
				itemData += 4;


				
				//AddItemImage(XKDisplay,pCellImag);
				//itemData += 4;
				break;

			//�����ֿ�
			case 'f':
				XKCellStr.cx = Cx;
				XKCellStr.cy = Cy;
				XKCellStr.nFontType = itemData[1];
				XKCellStr.nFontSize = (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
				itemData += 6;
				break;

			//�ַ���ɫ
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
				}
				break;

			//�ַ����
			case 'S':
				XKCellStr.nSpace = (itemData[1] - 0x30) * 10 + (itemData[2] - 0x30);
				itemData += 3;
				break;

			//�ַ���Ӱ��ɫ
			case 's':
				if(itemData[1] == 't')
				{
					itemData += 2;
				}
				else
				{
					itemData += 13;
				}
				break;

			//�ַ�������ɫ
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

			//��˸����
			case 'N':
				XKCellStr.flash = itemData[2] - 0x30;
				ZC_StrMsgPrintf(&XKCellStr);
				//AddItemString(XKDisplay,XKCellStr);
				itemData += 3;
				return 0;

			case 'I':
				itemData += 4;
				break;

			case 'F':
				itemData += 6;
				break;
				
			//��ɫ��֯���鱨��ĺ������عܣ���
			case 'y':
				itemData += 1;
				break;

			defualt:
				break;
				
				
		}
		
	}

	for(i = 0 ; i < DSPnode_cur ; i++) 
		AddItemDSPNode(&ZCSXKDisplay[ItemOder],&ZCDSPNODE[i]);
	
	return 0;
}



//���������б�
//����ֵ������:������ʾ����ʧ�ܣ�����0��ʾ�����ɹ�����˵���˲����б��ǲ岥�����б�
int ZC_Lstparsing(ContentList *head,const char *plist)
{
	char *s = NULL;
	FILE *stream;
	char strcontent[2048];
	int  itemCount = 0;
	int  CutInTime = 0;
	int  itemOrder = 0;
	int	 lstStatus = LST_HEAD;
	int  ToReadNextLine = 1;
	debug_printf("parsing the playlist: %s\n",plist);
	
	stream = fopen(plist, "r");
	if(stream == NULL)
	{
		DEBUG_PRINTF;
		return -1;
	}
	memset(ZCSXKDisplay,0,sizeof(XKDisplayItem) * ZCSXKSIZE);
	fseek(stream, 0, SEEK_SET);
	while(1)
	{
		if(ToReadNextLine)
		{
			s = fgets(strcontent, 2048, stream);
			if(s == NULL)
				break;
		}

		debug_printf("strItem = %s\n",strcontent);
		switch(lstStatus)
		{
			case LST_HEAD:
				DEBUG_PRINTF;
				debug_printf("strcontent = %s\n",strcontent);
				if(strncmp(strcontent,"[playlist]",10) != 0)
				{
					DEBUG_PRINTF;
					goto FREE_RESCR;
				}
				lstStatus = LST_TYPE;
				DEBUG_PRINTF;
				break;
			case LST_TYPE:
				DEBUG_PRINTF;
				if(strncmp(strcontent,"time_to_live",12) != 0)
				{
					ToReadNextLine = 0;
					lstStatus = LST_COUNT;
					break;
				}
				DEBUG_PRINTF;
				char *pp = strchr(strcontent,'=');
				if(pp != NULL)
				{
					CutInTime = atoi(pp + 1);
					debug_printf("CutInTime = %d\n",CutInTime);
					if(CutInTime > 0)
						ZC_SetCutInDisplay(CutInTime);
				}
				lstStatus = LST_COUNT;
				break;

			case LST_COUNT:
				ToReadNextLine = 1;
				if(strncmp(strcontent,"item_no",7) != 0 && strncmp(strcontent,"Item_No",7) != 0)
				{
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
				PLst_intemDecode(head,strcontent,itemOrder);
				itemOrder++;

				if(itemOrder >= itemCount)
					break;
				
				debug_printf("itemoder = %d\n",itemOrder);
				lstStatus = LST_PARSE;
				DEBUG_PRINTF;
				break;
				
			default:
				break;
		}		
		
	}

	debug_printf("itemOrder = %d\n",itemOrder);
	//exit(1);
	XKDisplayInsert(ZCSXKDisplay,itemOrder);
	
	DEBUG_PRINTF;
	fclose(stream);
	return CutInTime;
	
	FREE_RESCR:
		fclose(stream);
		DEBUG_PRINTF;
		return -1;
}

