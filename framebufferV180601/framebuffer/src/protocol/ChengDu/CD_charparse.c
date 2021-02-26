#include "CD_charparse.h"
#include "../../include/Dev_framebuffer.h"
#include <sys/stat.h>
#include <png.h>
#include "aes/aes_interf.h"
#include "../../Hardware/Data_pool.h"
#include "../../module/PNG/img_png.h"


#define CD_SXKSIZE		48		//一个播放列表里面最多限制在幕,即48条Item
#define CD_NODESIZE		24		//一条Item里面最多容许24个信息:文字、图片、gif
static DSPNode_t DSPNODE[CD_NODESIZE];
static XKDisplayItem SXKDisplay[CD_SXKSIZE];

static CDCurplay_t CDCurplay;
static void CDSetCurPlaying(void *arg)
{
	CDCurplay_t *playing = (CDCurplay_t *)arg;
	memcpy(&CDCurplay,playing,sizeof(CDCurplay_t));
	debug_printf("Curplay.stoptime = %d,Curplay.strLen = %d,Curplay.playstr = %s\n",CDCurplay.stoptime,CDCurplay.strLen,CDCurplay.playstr);
}

void CDGetCurPlaying(CDCurplay_t * curplay)
{
	memcpy(curplay,&CDCurplay,sizeof(CDCurplay_t));
}

int CD_FileDecrypt(const char *file,const char *outFile)
{
	int endFlag = 0;
	FILE *stream,*decryFp;
	int size,readSize = 0,writeSize = 0;
	int FileRemaindSize = 0;
	struct stat statbuf;
	uint8_t ReadBuf[4096];
	uint8_t decryptBuf[4096];
	uint16_t decryptLen = 0;
	
	if(access(file,F_OK) < 0)
		return -1;
	
	stat(file,&statbuf);
	FileRemaindSize = statbuf.st_size;

	stream = fopen(file, "r");
	if(stream == NULL)
	{
		return -1;
	}

	
	decryFp = fopen(outFile,"wb+");
	if(decryFp == NULL)
	{
		fclose(stream);
		return -1;
	}

	while(FileRemaindSize)
	{
		endFlag = (FileRemaindSize > 1024) ? 0 : 1;
		readSize = (FileRemaindSize > 1024) ? 1024 : FileRemaindSize;
		size = fread(ReadBuf,1,readSize,stream);
		if(size <= 0)
			break;

		AES_ECB_decrypt(ReadBuf,size,decryptBuf,&decryptLen,endFlag);
		parse_debug_printf("Readsize = %d,decryptLen = %d,endFlag = %d\n",size,decryptLen,endFlag);
		writeSize = fwrite(decryptBuf,1,decryptLen,decryFp);
		fflush(decryFp);
		FileRemaindSize -= size;
	}

	fclose(stream);
	fclose(decryFp);
	return 0;
	
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

static void XKCellImagePrintf(XKCellImage *XKCellIma)
{
	debug_printf("XKCellIma->strImage = %s\n",XKCellIma->strImage);
}


static void XKCellStringPrintf(XKCellString *XKCellStr)
{
	debug_printf(
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
	debug_printf("XKCellStr->strContent data : ");
	for(i = 0 ; i < XKCellStr->strContentLen ; i++)
		debug_printf("0x%x ",XKCellStr->strContent[i]);
	debug_printf("\n");
}
#if 0
static void DSPNodePrintf(DSPNode_t *DSPNode)
{
	debug_printf("\n\n"
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
			debug_printf("img\n");
			XKCellImagePrintf(DSPNode->XKCellIma);break;
		case 2:
			debug_printf("str\n");
			XKCellStringPrintf(DSPNode->XKCellStr);break;
		default:break;
	}
}
#endif

static void CD_StrMsgPrintf(XKCellString *XKCellStr)
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


static void CD_DefInitcontentnode(XKDisplayItem *head)
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
static void CD_INITStrDefVals(XKCellString * pCellString)
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
static void CD_INITImgDefVals(XKCellImage * pCellImag)
{
	pCellImag->cx					= 0;
	pCellImag->cy					= 0;
	strcpy(pCellImag->strImage,"");
}


static int CD_GetChineseStr(char *itemStr,char *ChineseStr)
{
	int strCounter = 0;
	char *ItemStrP = itemStr;
	char *ChineStrP = ChineseStr;
	while(*ItemStrP != 0)
	{
		if(*ItemStrP != '\\')
		{

			if(*ItemStrP == 0xd && *(ItemStrP+1) == 0xa)
				break;
			
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
	
	//ItemStrP	-= 2;
	//strCounter	-= 2;
	parse_debug_printf("strCounter = %d\n",strCounter);
	return strCounter;
}

static void CD_INITDSPNodeDefVals(DSPNode_t *DSPNode)
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

static void CD_INITPngDefVals(XKCellPngimg *XKCellPng)
{
	XKCellPng->cx = 0; 
	XKCellPng->cy = 0;
	XKCellPng->LineSeq = 0;
	XKCellPng->ScreenSeq = 0;
	XKCellPng->type = DSPTYPE_PNG;
	XKCellPng->pNext = NULL;
	strcpy(XKCellPng->pngImage,"");
}

static uint8_t *RGBbuf = NULL;
int CD_PNGMEMmalloc(uint32_t mallocSize)
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

static int CD_PNGprocess(DSPNode_t *DSPNode,uint8_t *DSPbuf)
{
	DEBUG_PRINTF;
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

	if(access(filepwd,F_OK) < 0)
		return -1;
	CD_FileDecrypt(filepwd,"/tmp/play.png");
	
	//初始化PNG各项参数
	if(PNG_init("/tmp/play.png") < 0)
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
	parse_debug_printf("DSPNode->width = %d,DSPNode->height = %d,Swidth = %d\n",DSPNode->width,DSPNode->height,Swidth);

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


static int CD_PLstIntemDecode(ContentList *head,char *itemContent,uint8_t ItemOder)
{
	uint8_t i = 0;
	int ChineseCounter = 0;
	int Cx = 0,Cy = 0;
	char ChineseStr[256];
	char *itemData = NULL;
	int DSPnode_cur = 0,DSPnode_pre = 0;
	uint8_t BMPtype = 0;
	uint8_t imgname[4];
	
	XKCellString 	XKCellStr;
	XKCellImage		XKCellIma;
	LIGHTBand_t     LIGHTBand;
	XKCellPngimg	XKCellPng;
	
	CD_DefInitcontentnode(&SXKDisplay[ItemOder]);
	
	memset(DSPNODE,0x00,sizeof(DSPNode_t) * 24);
	for(i = 0 ; i < 24 ; i++)
		CD_INITDSPNodeDefVals(&DSPNODE[i]);
	
	memset(&XKCellStr,0,sizeof(XKCellString));
	memset(&XKCellIma,0,sizeof(XKCellImage));
	memset(&XKCellPng,0,sizeof(XKCellPngimg));
	CD_INITStrDefVals(&XKCellStr);
	CD_INITImgDefVals(&XKCellIma);
	CD_INITPngDefVals(&XKCellPng);

	char *str_p = strchr(itemContent,'=');
	if(str_p == NULL)
		return -1;

	itemData = str_p + 1;
	parse_debug_printf("itemData = %s\n",itemData);
	sscanf(itemData,"%d,%d,%d",&SXKDisplay[ItemOder].nDelayTime,&SXKDisplay[ItemOder].nEffectIn,&SXKDisplay[ItemOder].nMoveSpeed);
	
	parse_debug_printf("1XKDisplay->nDelayTime = %d,&XKDisplay->nEffectIn = %d,&XKDisplay->nMoveSpeed = %d\n",
		SXKDisplay[ItemOder].nDelayTime,SXKDisplay[ItemOder].nEffectIn,SXKDisplay[ItemOder].nMoveSpeed);

	//出字方式为0时，后面的播放内容无效
	if(SXKDisplay[ItemOder].nEffectIn == 0)
		return -1;
	
	//XKDisplay->nEffectIn  = 1;//立即显示
	SXKDisplay[ItemOder].nEffectOut = 1;//立即显示
	SXKDisplay[ItemOder].nDelayTime = 1000 * 1000 * SXKDisplay[ItemOder].nDelayTime / 100;//停留时间，单位1/100秒
	parse_debug_printf("2XKDisplay->nDelayTime = %d,&XKDisplay->nEffectIn = %d,&XKDisplay->nMoveSpeed = %d\n",
		SXKDisplay[ItemOder].nDelayTime,SXKDisplay[ItemOder].nEffectIn,SXKDisplay[ItemOder].nMoveSpeed);

	//XKDisplay->nMoveSpeed每增加1，停留时间就增加20ms，此处要转化成us
	if(SXKDisplay[ItemOder].nEffectIn != 0 && SXKDisplay[ItemOder].nEffectIn != 1)
		SXKDisplay[ItemOder].nDelayTime += SXKDisplay[ItemOder].nMoveSpeed * 20 * 1000;
	
	//后续无内容要显示(本协议要求无内容时就是一个清屏命令，此处直接忽略掉清屏命令)
	str_p = strchr(itemContent,'\\'); 
	if(str_p == NULL) 
		return -1;

	//设置当前幕显示的内容
	CDCurplay_t *ThisPlay = (CDCurplay_t *)malloc(sizeof(CDCurplay_t));
	memset(ThisPlay,0,sizeof(CDCurplay_t));
	ThisPlay->order = ItemOder;
	ThisPlay->stoptime = SXKDisplay[ItemOder].nDelayTime / 10000;
	ThisPlay->effectin = SXKDisplay[ItemOder].nEffectIn;
	ThisPlay->strLen = (strlen(str_p) > 1024) ? 1024 : strlen(str_p);
	memcpy(ThisPlay->playstr,str_p,ThisPlay->strLen);
	SXKDisplay[ItemOder].Curplay = (void *)ThisPlay;
	SXKDisplay[ItemOder].setcurplaying = CDSetCurPlaying;
	

	itemData = str_p;
	while(*itemData != 0)
	{
		if(*itemData != '\\')
		{
			if(*itemData == '\n')
				break;
			parse_debug_printf("itemData = %s\n",itemData);
			ChineseCounter = CD_GetChineseStr(itemData,XKCellStr.strContent);
			parse_debug_printf("ChineseCounter = %d\n",ChineseCounter);
			XKCellStr.strContent[ChineseCounter] = '\0';
			XKCellStr.strContentLen = ChineseCounter;
			XKCellStr.nCellOrder = ItemOder;
			
			XKCellString *DSPStrNode = (XKCellString *)malloc(sizeof(XKCellString));
			memset(DSPStrNode,0x00,sizeof(XKCellString));
			memcpy(DSPStrNode,&XKCellStr,sizeof(XKCellString));

			DSPNODE[DSPnode_cur].Cx = Cx;
			DSPNODE[DSPnode_cur].Cy = Cy;
			DSPNODE[DSPnode_cur].type = DSPTYPE_STR;
			DSPNODE[DSPnode_cur].XKCellStr = DSPStrNode;
			itemData += ChineseCounter;
			DSPnode_cur += 1;
			DEBUG_PRINTF;
			if(*itemData != '\\')
				break;
			else
				DEBUG_PRINTF;
			continue;
		}
		itemData += 1;
		
		switch(*itemData)
		{
			//显示坐标
			case 'C':
				DEBUG_PRINTF;
				Cx = (itemData[1] - 0x30) * 100 + (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
				Cy = (itemData[4] - 0x30) * 100 + (itemData[5] - 0x30) * 10 + (itemData[6] - 0x30);
				itemData += 7;
				break;

			//图片标号
			case 'B'://临时改成P
				XKCellIma.cx	= Cx;
				XKCellIma.cy	= Cx;
				memcpy(imgname,itemData + 1,3);
				imgname[3] = '\0';
				Dir_LetterBtoL(imgname);
				
				XKCellIma.nCellOrder = ItemOder;

				BMPtype = 48;
				memset(XKCellIma.strImage,0x00,sizeof(XKCellIma.strImage));
				sprintf(XKCellIma.strImage,"%d/%s.bmp",BMPtype,imgname);
				XKCellIma.strImage[10] = '\0';

				DSPNODE[DSPnode_cur].width = BMPtype;
				DSPNODE[DSPnode_cur].height = DSPNODE[DSPnode_cur].width;
				DSPNODE[DSPnode_cur].type = DSPTYPE_IMG;

				XKCellImage *DSPimageNode = (XKCellImage *)malloc(sizeof(XKCellImage));
				memset(DSPimageNode,0x00,sizeof(XKCellImage));
				memcpy(DSPimageNode,&XKCellIma,sizeof(XKCellImage));
				DSPNODE[DSPnode_cur].XKCellIma = DSPimageNode;
				DSPnode_cur += 1;
				
				//AddItemImage(XKDisplay,XKCellIma);
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
				DSPNODE[DSPnode_cur].DSPfunc = CD_PNGprocess;
				DSPNODE[DSPnode_cur].type = DSPTYPE_PNG;
				XKCellPngimg *DSPpngNode = (XKCellPngimg *)malloc(sizeof(XKCellPngimg));
				memset(DSPpngNode,0x00,sizeof(XKCellPngimg));
				memcpy(DSPpngNode,&XKCellPng,sizeof(XKCellPngimg));

				DSPNODE[DSPnode_cur].XKCellPng = DSPpngNode;
				DSPnode_cur += 1;
				itemData += 4;
				break;

			//字体字库
			case 'f':
				DEBUG_PRINTF;
				XKCellStr.cx = Cx;
				XKCellStr.cy = Cy;
				XKCellStr.nFontType = itemData[1];
				XKCellStr.nFontSize = (itemData[2] - 0x30) * 10 + (itemData[3] - 0x30);
				itemData += 6;
				break;

			//字符颜色
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
					itemData += 10;
				}
				break;

			//字符间距
			case 'S':
				XKCellStr.nSpace = (itemData[1] - 0x30) * 10 + (itemData[2] - 0x30);
				itemData += 3;
				break;

			//字符阴影颜色
			case 's':
				if(itemData[1] == 't')
				{
					itemData += 2;
				}
				else
				{
					itemData += 10;
				}
				
				break;

			//字符背景颜色
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
					itemData += 10;
				}
				break;

			//闪烁次数
			case 'N':
				XKCellStr.flash = itemData[2] - 0x30;
				//CD_StrMsgPrintf(XKCellStr);
				//AddItemString(XKDisplay,XKCellStr);
				itemData += 3;
				return 0;

			case 'I':
				itemData += 4;
				break;

			case 'F':
				itemData += 6;
				break;
				
			//黄色组织到情报板的红绿像素管，略
			case 'y':
				itemData += 1;
				break;

			defualt:
				break;
				
				
		}
		
	}
	
	for(i = 0 ; i < DSPnode_cur ; i++)
		AddItemDSPNode(&SXKDisplay[ItemOder],&DSPNODE[i]);

	return 0;
}



static int Blackboad(XKDisplayItem *XKDisplay)
{
	XKCellString XKCellStr;
	DSPNode_t DSPstr;
	memset(&DSPstr,0,sizeof(DSPNode_t));
	memset(&XKCellStr,0,sizeof(XKCellString));

	CD_INITStrDefVals(&XKCellStr);
	memcpy(XKCellStr.strContent,"    ",4);
	XKCellStr.strContent[4] = '\0';
	XKCellStr.strContentLen = 4;
    XKCellStr.nForeColor[0] 		= 0x00;
	XKCellStr.nForeColor[1] 		= 0x00;
	XKCellStr.nForeColor[2] 		= 0x00;

	DSPstr.Cx = 0;
	DSPstr.Cy = 0;
	DSPstr.type = DSPTYPE_STR;

	XKCellString *DSPstring = (XKCellString *)malloc(sizeof(XKCellString));
	memset(DSPstring,0,sizeof(XKCellString));
	memcpy(DSPstring,&XKCellStr,sizeof(XKCellString));

	DSPstr.XKCellStr = DSPstring;
	AddItemDSPNode(XKDisplay,&DSPstr);
	return 0;
}


int CD_Lstparsing(ContentList *head,const char *plist)
{
	uint8_t i = 0;
	char *s = NULL;
	FILE *stream;
	char strcontent[2048];
	int  itemCount = 0;
	int  CutInTime = 0;
	int  itemOrder = 0;
	int	 lstStatus = CD_LSTHEAD;
	int  Snumber = 0;
	int  breakflag = 0;
	debug_printf("parsing the playlist!plist = %s\n",plist);
	
	for(i = 0 ; i < CD_SXKSIZE; i++)
		memset(SXKDisplay,0,sizeof(XKDisplayItem) * CD_SXKSIZE);


	//先对解密的播放列表文件进行解密，解密到/tmp/play.lst，/tmp目录下的文件掉电就消失
	CD_FileDecrypt(plist,"/tmp/play.lst");
	
	stream = fopen("/tmp/play.lst", "r");
	if(stream == NULL)
	{
		return -1;
	}
	fseek(stream, 0, SEEK_SET);

	while(1)
	{
		s = fgets(strcontent, 2048, stream);
		if(s == NULL)
			break;

		parse_debug_printf("strItem = %s\n",strcontent);
		switch(lstStatus)
		{
			case CD_LSTHEAD:
				if(strncmp(strcontent,"[list]",6) != 0)
					goto FREE_RESCR;
				lstStatus = CD_LSTCOUNT;
				break;
				
			case CD_LSTCOUNT:
				if(strncmp(strcontent,"item_no",7) != 0 && strncmp(strcontent,"Item_No",7) != 0)
				{
					goto FREE_RESCR;
				}
				DEBUG_PRINTF;
				char *p = strstr(strcontent,"=");
				if(p == NULL)
					goto FREE_RESCR;
				
				itemCount = atoi(p+1);

				//播放表item条数为0时表示黑屏，下面产生一个黑屏的AddDisplayItem节点
				if(itemCount == 0)
				{
					Blackboad(&SXKDisplay[0]);
					Snumber = 1;
					breakflag = 1;
					break;
				}
				head->itemcount = itemCount;				
				lstStatus = CD_LSTPARSE;
				parse_debug_printf("itemCount = %d\n",itemCount);
				break;

			case CD_LSTPARSE:
				parse_debug_printf("itemOrder = %d,itemCount = %d\n",itemOrder,itemCount);				
				//DecodeItemString(head,strcontent);
				CD_PLstIntemDecode(head,strcontent,itemOrder);
				itemOrder++;
				if(itemOrder >= itemCount)
				{
					Snumber = itemOrder;
					breakflag = 1;
					break;
				}
				parse_debug_printf("itemoder = %d\n",itemOrder);
				lstStatus = CD_LSTPARSE;
				break;
				
			default:
				break;
		}		

		//解析完跳出循环
		if(breakflag)
			break;
		
	}

	fclose(stream);
	
	pthread_mutex_lock(&content_lock);
	parse_debug_printf("##itemOrder = %d\n",Snumber);
	ClearContent(head);
	for(i = 0 ; i < Snumber ; i++)
		AddDisplayItem(head,&SXKDisplay[i]);
	head->itemcount = Snumber;
	head->refresh = LST_REFLASH;
	pthread_mutex_unlock(&content_lock);

	//remove("/tmp/play.lst");
	return 0;
	
	FREE_RESCR:
		fclose(stream);
		return -1;
}

