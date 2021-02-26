#include <png.h> 

#include "SWR_charparse.h"
#include "SWR_init.h"
#include "../../Hardware/Data_pool.h"
#include "../../module/PNG/img_png.h"


static ContentList XKdspLst;
static uint8_t *RGBbuf = NULL;
int SWR_PNGMEMmalloc(uint32_t mallocSize)
{
	debug_printf("mallocSize = %d\n",mallocSize);
	RGBbuf = (uint8_t *)malloc(mallocSize);
	if(RGBbuf == NULL)
	{
		perror("CD_PNGMEMmalloc malloc fail");
		return -1;
	}
	return 0;
}

static int PlaylistItemDecoder(ContentList *head,char *itemContent,uint8_t ItemOder)
{
	return itemDecoder(head,itemContent,ItemOder);
}


static int SWR_PNGprocess(DSPNode_t *DSPNode,uint8_t *DSPbuf)
{
	uint16_t PNGwidth,PNGheight;
	uint32_t Swidth,Sheight;
	char filepwd[64];
	if(DSPNode == NULL || DSPbuf == NULL)
		return -1;
	
	DP_GetScreenSize(&Swidth,&Sheight);
	if(DSPNode->Cx >= Swidth || DSPNode->Cy >= Sheight)
		return -1;
	
	if(DSPNode->type != DSPTYPE_PNG || DSPNode->XKCellPng == NULL)
		return -1;

	XKCellPngimg *XKCellPng = DSPNode->XKCellPng;
	memset(filepwd,0,sizeof(filepwd));
	sprintf(filepwd,"%s/%s",image_dir,XKCellPng->pngImage);
	debug_printf("filepwd = %s\n",filepwd);

	if(access(filepwd,F_OK) < 0)
		return -1;
	
	//初始化PNG各项参数
	if(PNG_init(filepwd) < 0)
		return -1;
	PNG_GetPngSize(&PNGwidth,&PNGheight);

	DSPNode->width = (DSPNode->Cx + PNGwidth > Swidth) ? (Swidth - DSPNode->Cx) : PNGwidth;
	DSPNode->height = (DSPNode->Cy + PNGheight > Sheight) ? (Sheight - DSPNode->Cy) : PNGheight;

	//从PNG获取RGB数据
	if(PNG_GetRGB(RGBbuf,DSPNode->width,DSPNode->height) < 0)
	{
		PNG_destroy();
		return -1;
	}
	DEBUG_PRINTF;
	//销毁png占用的资源
	PNG_destroy();

	uint16_t h,w;
	uint32_t wtmp = 0,rtmp = 0;
	uint32_t WRpos = 0,RDpos = 0;;

	//将RGB数据填充显示缓存区
	for(h = 0 ; h < DSPNode->height ; h++)
	{
		wtmp = (DSPNode->Cy + h) * Swidth * SCREEN_BPP;
		rtmp = DSPNode->width * h * 3;
		for(w = 0 ; w < DSPNode->width ; w++)
		{
			WRpos = wtmp + (DSPNode->Cx + w) * SCREEN_BPP;
			RDpos = rtmp + w * 3;
			DSPbuf[WRpos + 0] = RGBbuf[RDpos + 0];
			DSPbuf[WRpos + 1] = RGBbuf[RDpos + 1];
			DSPbuf[WRpos + 2] = RGBbuf[RDpos + 2];
		}
	}

	return 0;
		
}



//获取要显示的字符的总数(按字节计算)
int SWR_GetChineseStr(char *itemStr,char *ChineseStr)
{
	int strCounter = 0;
	char *ItemStrP = itemStr;
	char *ChineStrP = ChineseStr;
	
	while(*ItemStrP != 0)
	{
		//换行符
		if(*ItemStrP == 0xd && *(ItemStrP+1) == 0x0a)
			break;

		//显示的字符
		if(*ItemStrP != '\\')
		{		
			//debug_printf("1: %x \n",(uint8_t)*ItemStrP);
			*(ChineStrP++) = *ItemStrP;
			strCounter = strCounter + 1;
			ItemStrP++;
			continue;
		}

		//排除\N符号
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



void SWR_DefInitcontentnode(XKDisplayItem *head)
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
	memset(head->itemconent,0,sizeof(head->itemconent));
}

void SWR_INITGifDefVals(XKCellAnimate * pCellAnimate)
{
    pCellAnimate->cx = 0;
	pCellAnimate->cy = 0;
    pCellAnimate->nCellOrder = 0;
    pCellAnimate->nFormatType = 0;
    pCellAnimate->nPlayTime = 1;
	strcpy(pCellAnimate->strAnimate,"");
}

static void SWR_INITPngDefVals(XKCellPngimg *XKCellPng)
{
	XKCellPng->cx = 0; 
	XKCellPng->cy = 0;
	XKCellPng->LineSeq = 0;
	XKCellPng->ScreenSeq = 0;
	XKCellPng->type = DSPTYPE_PNG;
	XKCellPng->pNext = NULL;
	strcpy(XKCellPng->pngImage,"");
}

void SWR_INITStrDefVals(XKCellString * pCellString)
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
    pCellString->nFontSize 			= 16;
    pCellString->nSpace 			= 0;
	pCellString->nLineSpace			= 0;
    pCellString->nCellOrder = 0;
}


//默认图片信息
void SWR_INITImgDefVals(XKCellImage * pCellImag)
{
	pCellImag->cx					= 0;
	pCellImag->cy					= 0;
	strcpy(pCellImag->strImage,"");
}

void SWR_INITDSPNodeDefVals(DSPNode_t *DSPNode)
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

DSPNode_t DSPNODE[NODESIZE];
XKDisplayItem SXKDisplay[SXKSIZE];



int SWR_PLstIntemDecode(ContentList *head,char *itemContent,uint8_t ItemOder)
{
	uint8_t i = 0;
	int ChineseCounter = 0;
	int Cx = 0,Cy = 0;
	char ChineseStr[256];
	char *imagepwd = NULL;
	char ImagePWD[64]; 
	char *itemData = NULL;
	int DSPnode_cur = 0,DSPnode_pre = 0;
	uint8_t BMPtype = 0;
	uint8_t imgname[4];
    int nCellOrder = 0;
	
	XKCellString 	XKCellStr;
	XKCellImage		XKCellIma;
	XKCellAnimate 	XKCellAni;
	XKCellPngimg	XKCellPng;
	if(ItemOder > SXKSIZE)
		return 0;

	//初始化表示一幕的结构体
	SWR_DefInitcontentnode(&SXKDisplay[ItemOder]);

	//将表示存放一个一个信息的DSPNODE清零
	memset(DSPNODE,0x00,sizeof(DSPNode_t) * 24);
	for(i = 0 ; i < 24 ; i++)
		SWR_INITDSPNodeDefVals(&DSPNODE[i]);

	//一条Item中有可能包含GIF、IMG、STR,将将暂存这些信息的结构清零并初始化
	memset(&XKCellAni,0,sizeof(XKCellAnimate));
	memset(&XKCellStr,0,sizeof(XKCellString));
	memset(&XKCellIma,0,sizeof(XKCellImage));
	memset(&XKCellPng,0,sizeof(XKCellPngimg));
	SWR_INITStrDefVals(&XKCellStr);
	SWR_INITImgDefVals(&XKCellIma);
	SWR_INITGifDefVals(&XKCellAni);
	SWR_INITPngDefVals(&XKCellPng);

	//进入Item的解析
	char *str_p = strchr(itemContent,'=');
	if(str_p == NULL)
		return -1;

	//幕信息的初始化，每一幕入屏、停留时间、出屏动作
	itemData = str_p + 1;
	uint16_t datalen = strlen(itemData);
	
	sscanf(itemData,"%d,%d,%d,%d,%d,%s",&SXKDisplay[ItemOder].nDelayTime,&SXKDisplay[ItemOder].nEffectIn,&SXKDisplay[ItemOder].nEffectShow,&SXKDisplay[ItemOder].nEffectOut,&SXKDisplay[ItemOder].nMoveSpeed,&SXKDisplay[ItemOder].itemconent);
	strncpy(SXKDisplay[ItemOder].itemconent,itemData,datalen);
	debug_printf("=====%d,%d,%d,%d,%d,%s\n",SXKDisplay[ItemOder].nDelayTime,SXKDisplay[ItemOder].nEffectIn,SXKDisplay[ItemOder].nEffectShow,SXKDisplay[ItemOder].nEffectOut,SXKDisplay[ItemOder].nMoveSpeed,SXKDisplay[ItemOder].itemconent);
	//printf("#itemData = %s\n",SXKDisplay[ItemOder].itemconent);
	SXKDisplay[ItemOder].nDelayTime = 1000 * 1000 * SXKDisplay[ItemOder].nDelayTime;//停留时间，单位1/100秒

	str_p = strchr(itemContent,'\\'); 
	if(str_p == NULL) 
		return -1;
	itemData = str_p;

	//详细信息的解析
	while(*itemData != 0)
	{
		if(*itemData != '\\')
			break;

		itemData += 1;

		switch(*itemData)
		{
			case 'C':
				Cx = (itemData[1]-0x30)*100 + (itemData[2]-0x30)*10 + itemData[3]-0x30;
				Cy = (itemData[4]-0x30)*100 + (itemData[5]-0x30)*10 + itemData[6]-0x30;
				itemData += 7;
				debug_printf("Cx = %d,Cy = %d\n",Cx,Cy);
				break;

			case 'I':
				XKCellIma.cx	= Cx;
				XKCellIma.cy	= Cx;
				memcpy(imgname,itemData + 1,3);
				imgname[3] = '\0';
				Dir_LetterBtoL(imgname);
				DEBUG_PRINTF;
				//memset(imagepwd,0,sizeof(imagepwd));

				memset(ImagePWD,0,sizeof(ImagePWD));
				sprintf(ImagePWD,"%s/%s.bmp",image_dir,imgname);
				debug_printf("ImagePWD = %s\n",ImagePWD);
				
				memset(XKCellIma.strImage,0x00,sizeof(XKCellIma.strImage));

				//图片优先从image/目录下寻找
				if(access(ImagePWD,F_OK) == 0)
				{
					sprintf(XKCellIma.strImage,"%s.bmp",imgname);
				}
				
				//上面的默认路径找不到时调用系统内置图片--->image/32或者image/48
				else
				{
					uint32_t Swidth,Sheight;
					DP_GetScreenSize(&Swidth,&Sheight);
					if(Sheight < 48)
						BMPtype = 32;
					else
						BMPtype = 48;
					sprintf(XKCellIma.strImage,"%d/%s.bmp",BMPtype,imgname);
				}
				
				XKCellIma.nCellOrder = ItemOder;
				DSPNODE[DSPnode_cur].Cx = Cx;
				DSPNODE[DSPnode_cur].Cy = Cy;
				DSPNODE[DSPnode_cur].width = BMPtype;
				DSPNODE[DSPnode_cur].height = DSPNODE[DSPnode_cur].width;
				DSPNODE[DSPnode_cur].type = DSPTYPE_IMG;

				XKCellImage *DSPimageNode = (XKCellImage *)malloc(sizeof(XKCellImage));
				memset(DSPimageNode,0x00,sizeof(XKCellImage));
				memcpy(DSPimageNode,&XKCellIma,sizeof(XKCellImage));
				DSPNODE[DSPnode_cur].XKCellIma = DSPimageNode;
				DSPnode_cur += 1;
				
				itemData += 4;
				break;

			case 'A':
				XKCellIma.cx	= Cx;
				XKCellIma.cy	= Cx;
				memcpy(imgname,itemData + 1,3);
				imgname[3] = '\0';
				//Dir_LetterBtoL(imgname);
				DEBUG_PRINTF;
				memset(ImagePWD,0,sizeof(ImagePWD));
				
				memset(XKCellIma.strImage,0x00,sizeof(XKCellIma.strImage));
				if(imgname[0] == '0')
				{
					sprintf(ImagePWD,"%s/32/%s.bmp",image_dir,imgname);
						
					
					if(access(ImagePWD,F_OK) == 0)
					{
						sprintf(XKCellIma.strImage,"32/%s.bmp",imgname);
					}
					//printf("ImagePWD = %s\n",XKCellIma.strImage);
				}
				else if(imgname[0] == '1')
				{
					imgname[0] = '0'; 
					sprintf(ImagePWD,"%s/48/%s.bmp",image_dir,imgname);
					if(access(ImagePWD,F_OK) == 0)
					{
						sprintf(XKCellIma.strImage,"48/%s.bmp",imgname);
					}
					//printf("ImagePWD = %s\n",XKCellIma.strImage);
				}				

				XKCellIma.nCellOrder = ItemOder;
				DSPNODE[DSPnode_cur].Cx = Cx;
				DSPNODE[DSPnode_cur].Cy = Cy;
				DSPNODE[DSPnode_cur].width = BMPtype;
				DSPNODE[DSPnode_cur].height = DSPNODE[DSPnode_cur].width;
				DSPNODE[DSPnode_cur].type = DSPTYPE_IMG;

				XKCellImage *DSPimageNodeBMP = (XKCellImage *)malloc(sizeof(XKCellImage));
				memset(DSPimageNodeBMP,0x00,sizeof(XKCellImage));
				memcpy(DSPimageNodeBMP,&XKCellIma,sizeof(XKCellImage));
				DSPNODE[DSPnode_cur].XKCellIma = DSPimageNodeBMP;
				DSPnode_cur += 1;				
				itemData += 4;				
				break;
				

			case 'G':
				memcpy(imgname,itemData + 1,3);
				imgname[3] = '\0';
				Dir_LetterBtoL(imgname);
				memset(XKCellAni.strAnimate,0x00,sizeof(XKCellAni.strAnimate));
				sprintf(XKCellAni.strAnimate,"gif/%s.gif",imgname);
				XKCellAni.strAnimate[11] = '\0';
				XKCellAni.nCellOrder = ItemOder;
				debug_printf("#XKCellAni.strAnimate = %s\n",XKCellAni.strAnimate);

				//DSPNODE[DSPnode_cur].width = BMPtype;
				//DSPNODE[DSPnode_cur].height = DSPNODE[DSPnode_cur].width;
				DSPNODE[DSPnode_cur].type = DSPTYPE_GIF;
				DSPNODE[DSPnode_cur].Cx = Cx;
				DSPNODE[DSPnode_cur].Cy = Cy;
				debug_printf("Cx = %d,Cy = %d\n",Cx,Cy);
				XKCellAnimate *DSPGIFNode = (XKCellAnimate *)malloc(sizeof(XKCellAnimate));
				memset(DSPGIFNode,0x00,sizeof(XKCellAnimate));
				memcpy(DSPGIFNode,&XKCellAni,sizeof(XKCellAnimate));
				DSPNODE[DSPnode_cur].XKCellAni = DSPGIFNode;
				DSPnode_cur += 1;
				itemData += 4;
				break;
			case 'P':
				memcpy(imgname,itemData + 1,3);
				imgname[3] = '\0';
				Dir_LetterBtoL(imgname);
			
				memset(XKCellPng.pngImage,0x00,sizeof(XKCellPng.pngImage));
				sprintf(XKCellPng.pngImage,"png/%s.png",imgname);
				XKCellPng.pngImage[11] = '\0';
				XKCellPng.nCellOrder = ItemOder;
			
				DSPNODE[DSPnode_cur].Cx = Cx;
				DSPNODE[DSPnode_cur].Cy = Cy;
				DSPNODE[DSPnode_cur].DSPfunc = SWR_PNGprocess;
				DSPNODE[DSPnode_cur].type = DSPTYPE_PNG;
				XKCellPngimg *DSPpngNode = (XKCellPngimg *)malloc(sizeof(XKCellPngimg));
				memset(DSPpngNode,0x00,sizeof(XKCellPngimg));
				memcpy(DSPpngNode,&XKCellPng,sizeof(XKCellPngimg));
			
				DSPNODE[DSPnode_cur].XKCellPng = DSPpngNode;
				DSPnode_cur += 1;
				itemData += 4;
				break;

			case 'O':
				XKCellAni.nFormatType = 0;
				memcpy(XKCellAni.strAnimate,itemData+1,3);
				XKCellAni.strAnimate[3]='\0';
				XKCellAni.nFormatType=1;
				XKCellAni.nPlayTime = (itemData[4]-0x30)*10 + itemData[5]-0x30;
				//AddItemAnimate(pItem,pCellAnimate);
				itemData +=6;
				break;

			case 'Y':
				itemData +=2;
				break;

			case 'T':
				if(itemData[1] == 't') 
				{
					XKCellStr.nForeColor[0] = 255;
					XKCellStr.nForeColor[1] = 0;
					XKCellStr.nForeColor[2] = 0;
					itemData += 2;
				}
				else
				{
					//瀛椾綋棰滆壊
					XKCellStr.nForeColor[0] = (itemData[1]-0x30)*100 + (itemData[2]-0x30)*10 + itemData[3]-0x30;
					XKCellStr.nForeColor[1] = (itemData[4]-0x30)*100 + (itemData[5]-0x30)*10 + itemData[6]-0x30;
					XKCellStr.nForeColor[2] = (itemData[7]-0x30)*100 + (itemData[8]-0x30)*10 + itemData[9]-0x30;
					itemData +=13;
				}
				break;

			case 'B':
				if(itemData[1] == 't') // 榛樿
				{
					XKCellStr.nBkColor[0] = 255;
					XKCellStr.nBkColor[1] = 0;
					XKCellStr.nBkColor[2] = 0;
					itemData += 2;
				}
				else
				{
					XKCellStr.nBkColor[0] = (itemData[1]-0x30)*100 + (itemData[2]-0x30)*10 + itemData[3]-0x30;
					XKCellStr.nBkColor[1] = (itemData[4]-0x30)*100 + (itemData[5]-0x30)*10 + itemData[6]-0x30;
					XKCellStr.nBkColor[2] = (itemData[7]-0x30)*100 + (itemData[8]-0x30)*10 + itemData[9]-0x30;
					itemData +=13;
				}
				break;

			case 'S':
				if(itemData[1] == 't') // 榛樿
				{
					XKCellStr.nShadowColor[0] = 255;
					XKCellStr.nShadowColor[1] = 0;
					XKCellStr.nShadowColor[2] = 0;
					itemData += 2;
				}
				else
				{									
					XKCellStr.nShadowColor[0] = (itemData[1]-0x30)*100 + (itemData[2]-0x30)*10 + itemData[3]-0x30;
					XKCellStr.nShadowColor[1] = (itemData[4]-0x30)*100 + (itemData[5]-0x30)*10 + itemData[6]-0x30;
					XKCellStr.nShadowColor[2] = (itemData[7]-0x30)*100 + (itemData[8]-0x30)*10 + itemData[9]-0x30;
					itemData +=13; 		   
				}
				break;

			case 'M':
				XKCellStr.nSpace = (itemData[1]-0x30)*10 + itemData[2]-0x30;
				itemData +=3;
				break;

			case 'F':
				if(itemData[1] == 's' || itemData[1] == 'h' || itemData[1] == 'f' ||itemData[1] == 'k')
				{
				   XKCellStr.nFontType = itemData[1];
				   XKCellStr.nFontSize = (itemData[2]-0x30)*10 + itemData[3]-0x30;
				}
				itemData += 4;
				break;

			case 'N':
				itemData--;

			case 'U':
				//printf("itemData = %s\n",itemData);
				ChineseCounter = SWR_GetChineseStr(itemData+1,XKCellStr.strContent);
				debug_printf("ChineseCounter = %d\n",ChineseCounter);
				XKCellStr.strContent[ChineseCounter] = '\0';
				XKCellStr.strContentLen = ChineseCounter;
				XKCellStr.nCellOrder = ItemOder;

				XKCellString *DSPStrNode = (XKCellString *)malloc(sizeof(XKCellString));
				memset(DSPStrNode,0x00,sizeof(XKCellString));
				memcpy(DSPStrNode,&XKCellStr,sizeof  (XKCellString));

				DSPNODE[DSPnode_cur].Cx = Cx;
				DSPNODE[DSPnode_cur].Cy = Cy;
				DSPNODE[DSPnode_cur].type = DSPTYPE_STR;
				DSPNODE[DSPnode_cur].XKCellStr = DSPStrNode;
				debug_printf("itemData = %s\n",itemData);
				itemData += ChineseCounter + 1;

				//printf("2itemData = %s\n",itemData);
				DSPnode_cur += 1;
				break;
			}
	}

	//SXKDisplay是表示一幕，DSPNODE表示一幕下的一个信息，一幕有可能包含很多条信息
	//下面是将所有信息插入到SXKDisplay中暂存
	for(i = 0 ; i < DSPnode_cur ; i++) 
		AddItemDSPNode(&SXKDisplay[ItemOder],&DSPNODE[i]);
	
	return 0;
}




int XKDSPnode_cur = 0;
int SWR_Lstparsing(ContentList *head,const char *plist)
{
	int i = 0;
	char *s = NULL;
	FILE *stream;
	char strcontent[2048];
	int  itemCount = 0;
	int  CutInTime = 0;
	int  itemOrder = 0;
	int  parse_finish = 0;
	int  no_content = 0;
	int	 lstStatus = SWR_LSTHEAD;
	debug_printf("parsing the playlist!plist = %s\n",plist);
	
	stream = fopen(plist, "r");
	if(stream == NULL)
	{
		DEBUG_PRINTF;
		return -1;
	}

	//for(i = 0 ; i < 48 ; i++)
	memset(SXKDisplay,0,sizeof(XKDisplayItem) * 48);

	fseek(stream, 0, SEEK_SET);
	DEBUG_PRINTF;
	while(1)
	{
		s = fgets(strcontent, 2048, stream);
		if(s == NULL)
			break;

		debug_printf("strItem = %s\n",strcontent);
		switch(lstStatus)
		{
			case SWR_LSTHEAD:
				if(strncmp(strcontent,"[LIST]",6) != 0)
					goto FREE_RESCR;
				lstStatus = SWR_LSTCOUNT;
				break;
				
			case SWR_LSTCOUNT:
				if(strncmp(strcontent,"ItemCount",9) != 0)
				{
					goto FREE_RESCR;
				}
				char *p = strstr(strcontent,"=");
				if(p == NULL)
					goto FREE_RESCR;
				
				itemCount = atoi(p+1);
				head->itemcount = itemCount;				
				lstStatus = SWR_LSTPARSE;
				debug_printf("itemCount = %d\n",itemCount);
				break;

			case SWR_LSTPARSE:
				debug_printf("XKDSPnode_cur = %d\n",XKDSPnode_cur); 			
				//DecodeItemString(head,strcontent);
				DEBUG_PRINTF;
				PlaylistItemDecoder(head,strcontent,XKDSPnode_cur);
				DEBUG_PRINTF;
				XKDSPnode_cur++;
				if(XKDSPnode_cur >= SXKSIZE)
				{
					parse_finish = 1;
					break;
				}
				debug_printf("XKDSPnode_cur = %d\n",XKDSPnode_cur);
				lstStatus = SWR_LSTPARSE;
				break;
				
			default:
				break;
		}	

		if(parse_finish)
			break;
	
	}
	pthread_mutex_lock(&content_lock);
	debug_printf("##XKDSPnode_cur = %d\n",XKDSPnode_cur);
	ClearContent(head);
	for(i = 0 ; i < XKDSPnode_cur ; i++)
		AddDisplayItem(head,&SXKDisplay[i]);
	head->itemcount = XKDSPnode_cur;
	head->refresh = LST_REFLASH;
	pthread_mutex_unlock(&content_lock);
	XKDSPnode_cur = 0;
	fclose(stream);
	DEBUG_PRINTF;
	return CutInTime;

	FREE_RESCR:
		fclose(stream);
		return -1;
}



int SWR_itemparsing(ContentList *head,char *itemstr)
{

	memset(&SXKDisplay[0],0,sizeof(XKDisplayItem));

	debug_printf("itemOrder = %d\n",0); 			
	//SWR_PLstIntemDecode(head,itemstr,0);
	PlaylistItemDecoder(head,itemstr,0);
	
	pthread_mutex_lock(&content_lock);
	ClearContent(head);
	AddDisplayItem(head,&SXKDisplay[0]);
	head->itemcount = 0;
	head->refresh = LST_REFLASH;
	pthread_mutex_unlock(&content_lock);

	return 0;

}



