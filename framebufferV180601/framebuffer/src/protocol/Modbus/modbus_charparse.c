#include <sys/stat.h>

#include "modbus_charparse.h"
#include "config.h"
#include "content.h"
#include "debug.h"
#include "modbus_display.h"
#include "modbus_lightband.h"

#include "../../include/Dev_framebuffer.h"
#include "modbus_lightband.h"
#include "../../include/bitmap.h"
#include "content.h"
#include "modbus_config.h"

REALTime_t REALTime;
int MSwidth = 0,MSheight = 0;

static int LBstate = LBSTATE_CLOSE;
static int LBsement = 0;

#define MAXNODE		384
#define MAXSCRN		96
static XKDisplayItem MXKDisplay[MAXSCRN];
static DSPNode_t DSPNODE[MAXNODE];

static int align = ALIGN_UPDOWN_LEFTRIGHT;
//标记当前节点应该插入到DSPNODE数组中的那个位置
static int CurPos = 0;
//标记当前行在横坐标上布局到了什么位置
static int CurXpos = 0;
//标记当前屏在纵坐标上布局到了什么位置
static int CurYpos = 0;
//标记当前行所占的最大高度，同一行不同的文字或者不通的图片在高度上有可能不一样，按照高度最大的来计算当前行的高度
static int CurLineHeight = 0;	
//标记当前第几屏信息
static int Scene = 0;	
//标记当前行剩余的宽度
static int CurLineRemainWidth = 0;
//标记当前屏剩余的高度
static int CurScrRemainHeight = 0;

static int LBANDFlag = 0;

//存储光带每个段的颜色，最大64个段。
//第0个数据表示第0段什么颜色,第1个数据表示第一段什么颜色
//其中0表示黑色,即光带地图的背景色，1表示红色，2表示绿色，3表示黄色，7表示白色
uint8_t RoadSement[64]=
{
	//地图默认为绿色
	0xff,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
	0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,
};    

				
void Mdbs_InitScrSize(int width,int height)
{
	MSwidth 	= width;
	MSheight 	= height;
}


void Mdbs_LBandArgInit(uint8_t state,uint8_t sement)
{
	LBstate = state;
	LBsement = sement;
}

bool isLBopen(void)
{
	return (LBstate == LBSTATE_OPEN);
}


//说明:
//Road.dat 文件里面的数据其实是描述了底图Road.bmp上每个像素属于第几段
//RoadSement中存放的是对应段的颜色，用户可以随时设置里面的数据来确定每个段要显示什么颜色
int Mdbs_LightBandProcess(int Cx,int Cy,int width,int height,uint8_t *ToWriteBuf)
{
	static int DecodeFlag = 0;
	uint8_t readbuf[128];
	FILE *RDS_Fp = NULL;
	FILE *BMP_Fp = NULL;
	
	unsigned long lHeight;	// 图片高度
	unsigned long lWidth;	// 图片宽度
	int nTmp,nTmp1;
	int nRealBytes;
	int SLbytes = MSwidth * 4;

	BYTE BmpBuffer[2304];  // 一行 768
	char BmpDat[320];// 诱导图配置文件

	DEBUG_PRINTF;
	RDS_Fp = fopen("/home/LEDscr/rds/Road.dat","r+");
	if(RDS_Fp == NULL)
		return -1;

	DEBUG_PRINTF;
	if((BMP_Fp = fopen("/home/LEDscr/image/Road.bmp", "r+"))==NULL)
	{
		perror("BMP_Fp fopen");
		fclose(RDS_Fp);
		return -1;
	}

	DEBUG_PRINTF;

	uint8_t i = 0;
	
	BitMapFile_t bmpfile;
	fread(&(bmpfile.bfHeader), sizeof(BitMapFileHeader_t), 1, BMP_Fp); 
	fread(&(bmpfile.biInfo.bmiHeader), sizeof(BitMapInfoHeader_t), 1, BMP_Fp); 
	// 高度
	lWidth	= bmpfile.biInfo.bmiHeader.biWidth; 
	lHeight = bmpfile.biInfo.bmiHeader.biHeight;
	
	// 对齐边界
	nTmp = (int)lWidth%2;
	nRealBytes = (int)lWidth/2+nTmp;
	nTmp1 = nRealBytes%4;
	
	if(nTmp1>0)
		nTmp = 1;	
	DEBUG_PRINTF;
	int ii = 0,jj = 0,aaa = 0;
	uint8_t RGB_R,RGB_G,RGB_B;

	uint32_t bmppicth = ((lWidth*24 + 31) >> 5) << 2; 
	for(jj = lHeight-1;jj >= 0;jj--)
	{
		fread(BmpBuffer,bmppicth,1,BMP_Fp);

		//读取Road.dat数据，一行一行的读
		fseek(RDS_Fp,lWidth*3*(lHeight-(jj+1)),SEEK_SET);	   //文本中 直观的 1字节+空格 实质为3字节
		for(aaa=0;aaa<lWidth;aaa++)   //取图片宽度的数据
		{
			fscanf(RDS_Fp,"%x",&BmpDat[aaa]);	//逐行获取数据
		}

		
		for(ii = 0 ; ii < width ; ii++)
		{
			//根据从Road.dat文件读出的当前行每个像素点所在那个段确定颜色
			switch(RoadSement[BmpDat[ii]])   ////获得单像素 需要的颜色值
			{
				case 0:  //黑
					RGB_R = 0x00;
					RGB_G = 0x00;
					RGB_B = 0x00;
					break;
				case 1:  //红
					RGB_R = 0xff;
					RGB_G = 0x00;
					RGB_B = 0x00;
					break;
				case 2:  //绿
					RGB_R = 0x00;
					RGB_G = 0xff;
					RGB_B = 0x00;
					break;
				case 3:  //黄
					RGB_R = 0xff;
					RGB_G = 0xff;
					RGB_B = 0x00;
					break;
				case 7:	// 白
					RGB_R = 0xff;
					RGB_G = 0xff;
					RGB_B = 0xff;
					break;
				default:
					RGB_R = BmpBuffer[ii * 3 + BYTE_RGB_R];
					RGB_G = BmpBuffer[ii * 3 + BYTE_RGB_G];
					RGB_B = BmpBuffer[ii * 3 + BYTE_RGB_B];
			}
			ToWriteBuf[SLbytes * (jj + Cy) + (ii + Cx) * 4 + BYTE_RGB_B] = RGB_B;
			ToWriteBuf[SLbytes * (jj + Cy) + (ii + Cx) * 4 + BYTE_RGB_G] = RGB_G;
			ToWriteBuf[SLbytes * (jj + Cy) + (ii + Cx) * 4 + BYTE_RGB_R] = RGB_R;
	
		}

		
	}
	fclose(RDS_Fp);
	fclose(BMP_Fp);

	//设置光带的优先级最高，在解帧的时候一旦检测到光带优先级最高，所有待显示的
	//内容将被忽略过而直接找到光带显示
	//LBANDFlag表示有光带待显示，Scene <= MAXSCRN表示只有当显示的幕数小于最大幕数容量时才能确保光带一定存在带显示链表中
	if(LBANDFlag && Scene <= MAXSCRN)
		SetDSPriorit(DSPTYPE_LBD,PRIORITYN);
	
	return 0;
}


int Mdbs_GetLBandRealTimeData(uint8_t *RTdata,uint16_t *Len)
{
	int i = 0;
	int group = 0;
	uint8_t sementdate = 0;
	RTdata[0] = REALTime.lbtrbmsg;
	RTdata[1] = REALTime.lbdspstate;
	RTdata[2] = REALTime.lbswcode;
	RTdata[3] = REALTime.lbhwcode;
	
	group = (LBsement % 2 == 0) ? (LBsement / 2) : (LBsement / 2 + 1);
	for(i = 0 ; i < group ; i++)
		RTdata[i + 4] = REALTime.lbsement[2 * i] << 4 | REALTime.lbsement[2 * i + 1];

	*Len = group + 4;
	
	return 0;
}

int Mdbs_SetLBandSement(uint8_t StartUnit,uint8_t unitCount,uint8_t data)
{
	int i = 0;

	if(LBstate == LBSTATE_CLOSE)
		return -1;
	for(i = StartUnit ; i < StartUnit + unitCount ; i++)
	{
		if(i + 1 > 64)
			break;
		
		RoadSement[i + 1] = data;
		REALTime.lbsement[i + 1] = data;
	}
	
	SetDSPriorit(DSPTYPE_LBD,PRIORITY0);
	for(i = 0 ; i < 64 ; i++)
		debug_printf("%02x ",RoadSement[i]);
	debug_printf("\n");

	return 0;
}




int Mdbs_GetTxtRealTimeData(uint8_t *RTdata,uint16_t *Len)
{
	//协议上第一个字节本来应该是故障信息的，不知道为什么加上这个故障信息后上位机解析出来的
	//数据就错乱了，而将这个状态信息去掉之后，就可以解析正确了
	//RTdata[0] = REALTime.trbmsg;
	RTdata[0] = REALTime.dspstate;
	RTdata[1] = REALTime.swcode;
	RTdata[2] = REALTime.hwcode;
	RTdata[3] = REALTime.ineffect;
	if(REALTime.ctrway == CTRLWAY_WHOLE)
	{
		RTdata[4] = REALTime.intvtime;
		RTdata[5] = REALTime.font;
		RTdata[6] = REALTime.fontsize;
		RTdata[7] = REALTime.bmpnum;
		RTdata[8] = REALTime.bmptype;
		debug_printf("REALTime.strLen = %d\n",REALTime.strLen);
		memcpy(RTdata + 9 , REALTime.strmsg,REALTime.strLen);
		*Len = 9 + REALTime.strLen;
	}
	else
	{
		memcpy(RTdata + 4 , REALTime.strmsg,REALTime.strLen);
		*Len = 4 + REALTime.strLen;
	}


	return 0;
}

int Mdbs_SetTxtRealTimeData(uint8_t *content,uint16_t len)
{
	uint8_t *data = content;
	memset(&REALTime,0,sizeof(REALTime));
	REALTime.ctrway = data[0];
	REALTime.trbmsg = 0;
	REALTime.swcode = 0;
	REALTime.hwcode = 0;
	if(REALTime.ctrway == CTRLWAY_WHOLE)
	{
		REALTime.dspstate = 1;
		REALTime.ineffect = data[2];;
		REALTime.intvtime = data[3];;
		REALTime.font = data[4];
		REALTime.fontsize = data[5];
		REALTime.bmpnum = data[6];
		REALTime.bmptype = data[7];
		REALTime.strLen = len - 8;
		data += 8;
	}
	else
	{
		REALTime.dspstate = 8;
		REALTime.ineffect = 1;
		REALTime.intvtime = 0xff;
		REALTime.font = 0xff;
		REALTime.fontsize = 0xff;
		REALTime.bmpnum = 0xff;
		REALTime.bmptype = 0xff;
		REALTime.strLen = len - 2;
		data += 2;
	}
	debug_printf("len = %d,REALTime.strLen = %d\n",len,REALTime.strLen);
	memcpy(REALTime.strmsg, data ,REALTime.strLen);

	return 0;
}

int Mdbs_SetLBRealTimeData(uint8_t lbstate)
{
	int i = 0;
	if(LBstate == LBSTATE_CLOSE)
		return -1;
	
	REALTime.lbdspstate = lbstate;
	
	REALTime.lbtrbmsg = 0;
	REALTime.lbswcode = 0;
	REALTime.lbhwcode = 0;
	REALTime.lbsemenLen = LBsement;
	
	for(i = 0 ; i < REALTime.lbsemenLen; i++)
		REALTime.lbsement[i] = 0x02;
}




void Mdbs_DefaultLst(void)
{
	struct stat modbusFile;
	uint16_t readSize = 0;
	FILE *PLST = NULL;
	uint8_t plstContent[1024];
	if(access(PLAYLST,F_OK) < 0)
		return;
	stat(PLAYLST,&modbusFile);
	if(modbusFile.st_size < 8)
		return;
	PLST = fopen(PLAYLST,"r+");
	if(PLST == NULL)
		return;
	fseek(PLST,0,SEEK_SET);
	readSize = fread(plstContent,1,1024,PLST);
	plstContent[readSize] = '\0';
	fclose(PLST);
	Mdbs_charparse(&content,plstContent,readSize);
	return;
	
}





static uint8_t FontSelect(uint8_t FontOption)
{
	uint8_t Fontselect;
	switch(FontOption)
	{
		case 0:Fontselect  = 'h';break;
		case 1:Fontselect  = 'k';break;
		case 2:Fontselect  = 's';break;
		case 3:Fontselect  = 'f';break;
		default:Fontselect = 'k';break;
	}
	return Fontselect;
}

static uint8_t FontSizeSelect(uint8_t SizeOption)
{
	uint8_t FontSize = 0;
	switch(SizeOption)
	{
		case 0:FontSize = 32;break;
		case 1:FontSize = 16;break;
		case 2:FontSize = 24;break;
		case 3:FontSize = 32;break;
		case 4:FontSize = 48;break;
		case 5:FontSize = 64;break;
		default:FontSize = 32;break;
	}
	return FontSize;
}

static uint8_t AlignSelect(uint8_t AlignOption)
{
	uint8_t Align = 0;
	switch(AlignOption)
	{
		case 0x30:Align = ALIGN_UP;break;
		case 0x31:Align = ALIGN_MINDLE_UPDOWN;break;
		case 0x32:Align = ALIGN_DOWN;break;
		case 0x33:Align = ALIGN_LEFT;break;
		case 0x34:Align = ALIGN_MINDLE_LEFTRIGHT;break;
		case 0x35:Align = ALIGN_RIGHT;break;
		default:Align = ALIGN_UP;break;
	}
	return Align;
}


static uint8_t ImageSizeSelect(uint8_t ImageOption)
{
	uint8_t ImageSize = 0;
	switch(ImageOption)
	{
		case 0:ImageSize = 24;break;
		case 1:ImageSize = 32;break;
		case 2:ImageSize = 48;break;
		case 3:ImageSize = 64;break;
		default:ImageSize = 48;break;
	}
	return ImageSize;
}





static int Mdbs_GetChineseStr(char *itemStr,char *ChineseStr,uint8_t *charCNT)
{
	uint8_t charCT = 0;
	int strCounter = 0;
	char *ItemStrP = itemStr;
	char *ChineStrP = ChineseStr;
	while(*ItemStrP != 0)
	{
		if(*ItemStrP != 0x1B)
		{
			if((uint8_t)(*ItemStrP) < 0xa0)
			{
				charCT++;
			}
			
			*(ChineStrP++) = *ItemStrP;
			strCounter = strCounter + 1;
			ItemStrP++;
			continue;
		}	
		break;
	}
	*charCNT = charCT;
	MDBS_charparse_printf("strCounter = %d\n",strCounter);
	return strCounter;
}



static void Mdbs_INITDSPNodeDefVals(DSPNode_t *DSPNode)
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
}

static void _Mdbs_INITStrDefVals(XKCellString * pCellString)
{
	//默认在原点坐标显示
	pCellString->cx					= 0;
	pCellString->cy					= 0;
	//无闪烁
	pCellString->flash				= 0;
	//默认字体颜色是黄色
    pCellString->nForeColor[0] 		= 0x00;
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


static void ColorSelect(XKCellString * pCellString,uint8_t ColorOption)
{
	switch(ColorOption)
	{
		case COLOR_R:
			pCellString->nForeColor[0]		= 0xFF;
			pCellString->nForeColor[1]		= 0x00;
			pCellString->nForeColor[2]		= 0x00;
			break;
		case COLOR_G:
			pCellString->nForeColor[0]		= 0x00;
			pCellString->nForeColor[1]		= 0xFF;
			pCellString->nForeColor[2]		= 0x00;
			break;
		case COLOR_B:
			pCellString->nForeColor[0]	= 0x00;
			pCellString->nForeColor[1]	= 0x00;
			pCellString->nForeColor[2]	= 0xFF;
			break;
		case COLOR_Y:
			pCellString->nForeColor[0]	= 0xFF;
			pCellString->nForeColor[1]	= 0xFF;
			pCellString->nForeColor[2]	= 0x00;
			break;
		default:
			pCellString->nForeColor[0]	= 0xFF;
			pCellString->nForeColor[1]	= 0xFF;
			pCellString->nForeColor[2]	= 0x00;
			break;
	}
}

static void Mdbs_INITStrDefVals(XKCellString * pCellString,uint8_t Font,uint8_t FontSize,uint8_t color)
{
	//默认在原点坐标显示
	pCellString->cx					= 0;
	pCellString->cy					= 0;
	//无闪烁
	pCellString->flash				= 0;
	//ColorSelect(pCellString,color);
	pCellString->nForeColor[0]		= 0xff;
	pCellString->nForeColor[1]		= 0xff;
	pCellString->nForeColor[2]		= 0xff;
	//默认无字符
    strcpy(pCellString->strContent," ");

	//默认宋体、大小16、无间距、无行距
    pCellString->nFontType 			= Font;
    pCellString->nFontSize 			= FontSize;
    pCellString->nSpace 			= 0;
	pCellString->nLineSpace			= 0;
    pCellString->nCellOrder = 0;
}



void ContentListPrintf(ContentList *head)
{
	XKDisplayItem *XKDisplay = head->head;
	XKCellString *XKCellStr = NULL;
	XKCellImage *XKCellIma = NULL;

	DSPNode_t *DSPNode = NULL;

	if(head == &ScreenCache)
	{
		MDBS_CHARPARSEPRINTF;
	}
	
	MDBS_CHARPARSEPRINTF;
	while(XKDisplay != NULL)
	{
		mdbs_display_printf("XKDisplay->nDelayTime = %d\n",XKDisplay->nDelayTime);
		MDBS_CHARPARSEPRINTF;
		//XKCellStr = XKDisplay->pCellString_head;
		//XKCellIma = XKDisplay->pCellImage_head;
		DSPNode = XKDisplay->DSPNode_head;
		MDBS_charparse_printf("1:XKCellStr = 0x%x\n",(uint32_t)XKCellStr);
		while(DSPNode != NULL)
		{
			MDBS_CHARPARSEPRINTF;
			
			//XKCellStringPrintf(XKCellStr);
			DSPNodePrintf(DSPNode);
			//XKCellStr = XKCellStr->pNext;
			DSPNode = DSPNode->pNext;
		}
		MDBS_CHARPARSEPRINTF;

#if 0
		while(XKCellIma != NULL)
		{
			XKCellIma = XKCellIma->pNext;
		}
#endif
		XKDisplay = XKDisplay->pNext;
	}
}



static void Mdbs_DefInitcontentnode(XKDisplayItem *XKDisplay)
{
	Initcontentnode(XKDisplay);
	XKDisplay->align = ALIGN_MINDLE_UPDOWN;
	XKDisplay->nDelayTime = 2 * 1000 * 1000;
}


static XKCellString XKCellStrCache[10];
static XKCellImage  PCellImagCache[10];




static int Mdbs_wholeCtrl(uint8_t *content)
{
	DEBUG_PRINTF;
	XKDisplayItem 	*XKDisplay = NULL;
	XKCellString 	*XKCellStr = NULL;
	XKCellImage		*XKCellIma = NULL;
	DSPNode_t 		*DSPNode = NULL;

	MDBS_CHARPARSEPRINTF;
	if(content == NULL)
		return -1;
	
	XKDisplay = (XKDisplayItem *)malloc(sizeof(XKDisplayItem));
	XKCellStr = (XKCellString *)malloc(sizeof(XKCellString));
	XKCellIma = (XKCellImage *)malloc(sizeof(XKCellImage));
	DSPNode   = (DSPNode_t *)malloc(sizeof(DSPNode_t));
	
	MDBS_CHARPARSEPRINTF;
	memset(XKDisplay,0x00,sizeof(XKDisplayItem));
	memset(XKCellStr,0x00,sizeof(XKCellString));
	memset(XKCellIma,0x00,sizeof(XKCellImage));
	memset(DSPNode,0x00,sizeof(DSPNode_t));
	
	MDBS_CHARPARSEPRINTF;
	XKDisplay->nEffectIn		= content[0];
	XKDisplay->nDelayTime		= content[1] * 1000 * 1000;
	MDBS_CHARPARSEPRINTF;
	Mdbs_INITStrDefVals(XKCellStr,'s',32,COLOR_Y);
	XKCellStr->nFontType		= FontSelect(content[2]);
	XKCellStr->nFontSize		= FontSizeSelect(content[3]);
	memcpy(XKCellStr->strContent,content + 6,6);
	MDBS_CHARPARSEPRINTF;
	debug_printf("XKCellStr->nFontSize = %d\n",XKCellStr->nFontSize);
	DSPNode->height = XKCellStr->nFontSize;
	debug_printf("DSPNode->height = %d\n",DSPNode->height);
	MDBS_CHARPARSEPRINTF;
	DSPNode->width = 6 * 16;
	MDBS_CHARPARSEPRINTF;
	DSPNode->Sseq = 0;
	DSPNode->Lseq = 0;
	MDBS_CHARPARSEPRINTF;
	DSPNode->type = DSPTYPE_STR;
	DSPNode->XKCellStr = XKCellStr;
	MDBS_CHARPARSEPRINTF;
	AddItemDSPNode(XKDisplay,DSPNode);
	
	AddDisplayItem(&ContentCache,XKDisplay);
	MDBS_CHARPARSEPRINTF;
	free(XKCellStr);
	free(XKCellIma);
	free(XKDisplay);
	return 0;
}



int Mdbs_WritePlayLst(uint8_t *Content,uint16_t len)
{
	FILE *FP = NULL;
	FP = fopen(PLAYLST,"wb+");
	if(FP == NULL)
		return -1;
	fseek(FP,0,SEEK_SET);
	fwrite(Content,1,len,FP);
	fflush(FP);
	DEBUG_PRINTF;
	fclose(FP);
	return 0;
}



static int LineSpace(uint8_t DSPnodePos,uint8_t LineHeight)
{
	if(DSPnodePos < 1)
		return 0;

	if(DSPnodePos - 1 > 0)
		LineSpace(DSPnodePos - 1,LineHeight);
	
	debug_printf("%d,%d,%d,%d\n",DSPNODE[DSPnodePos - 1].Lseq,DSPNODE[DSPnodePos].Lseq,DSPNODE[DSPnodePos - 1].height,DSPNODE[DSPnodePos].height);
	if(DSPNODE[DSPnodePos - 1].Lseq != DSPNODE[DSPnodePos].Lseq || DSPNODE[DSPnodePos - 1].height != DSPNODE[DSPnodePos].height)
	{
		return -1;
	}
	
	DSPNODE[DSPnodePos - 1].height += LineHeight;
	return 0;
}




static void ChangLineInit(int LineHeight)
{
	DSPNode_t DSPNode;
	
	DSPNode.type = DSPTYPE_CGL;
	memcpy(&DSPNODE[CurPos],&DSPNode,sizeof(DSPNode_t));
	CurPos += 1;
	//换行后，横坐标回到原点
	CurXpos = 0;
	//换行后纵坐标向下移动当前行的最大文字的高度(同一行可能有不同大小不同字体的文字)
	CurYpos += LineHeight;
	//换行后横向剩余长度回到最大值
	CurLineRemainWidth = MSwidth;
	//换行后纵向剩余高度减掉当前行最大文字的高度
	CurScrRemainHeight -= CurLineHeight;
}

static void ChangScreenInit(void)
{
	DSPNode_t DSPNode;
	
	DSPNode.type = DSPTYPE_CGS;
	memcpy(&DSPNODE[CurPos],&DSPNode,sizeof(DSPNode_t));
	CurPos += 1;
	//换幕后，横坐标纵坐标都回到原点
	CurXpos = 0;
	CurYpos = 0;
	//换幕后当前行的最大高度归0；
	CurLineHeight = 0;
	//换幕后当前行剩余宽度与剩余高度都回归最大的状态
	CurLineRemainWidth = MSwidth;
	CurScrRemainHeight = MSheight;
}


static int InsertNode(DSPNode_t *DSPNode)
{
	DSPNode->Cx = CurXpos;
	DSPNode->Cy = CurYpos;
	DSPNode->height = (CurScrRemainHeight > DSPNode->height) ? DSPNode->height : CurScrRemainHeight;
	DSPNode->width = (CurLineRemainWidth > DSPNode->width) ? DSPNode->width : CurLineRemainWidth;
	memcpy(&DSPNODE[CurPos],DSPNode,sizeof(DSPNode_t));
	CurPos += 1;
	
	CurLineHeight = (CurLineHeight < DSPNode->height) ? DSPNode->height : CurLineHeight;
	CurXpos += DSPNode->width;
	CurLineRemainWidth -= DSPNode->width;
	return 0;
}


//对显示信息自动换行、自动换屏的处理
static int ChangeLineAndScreen(DSPNode_t *dspode)
{
	int i = 0;
	int Bytes = 0;
	uint16_t counter = 0;
	debug_printf("CurLineRemainWidth = %d\n"
				 "CurScrRemainHeight = %d\n"
				 "dspode->width = %d\n"
				 "dspode->height = %d\n",CurLineRemainWidth,CurScrRemainHeight,dspode->width,dspode->height);

	//一旦发现横向剩余宽度或者纵向剩余高度为0就重新换行或者换屏
	if(CurLineRemainWidth == 0)
	{
		ChangLineInit(CurLineHeight);
	}
	if(CurScrRemainHeight == 0)
	{
		ChangScreenInit();
	}

	//图片单独处理，对于图片，行剩余宽度或者屏幕剩余高度能显示多少就显示多少，不会递归
	if(dspode->type == DSPTYPE_IMG || dspode->type == DSPTYPE_LBD || dspode->type == DSPTYPE_PNG)
		return InsertNode(dspode);

	//下面是文字信息
	
	
	//显示节点的高度大于屏幕剩余高度则要换幕
	if(CurScrRemainHeight < dspode->height)
	{
		//文字信息高度不够显示先换幕
		ChangScreenInit();
		return ChangeLineAndScreen(dspode);
	}
	
	//程序运行到这里说明屏幕高度足够，宽度也足够
	if(CurLineRemainWidth >= dspode->width)
		return InsertNode(dspode);

	debug_printf("CurLineRemainWidth = %d,dspode->width = %d\n",CurLineRemainWidth,dspode->width);
	//当前行的剩余空间不足显示新节点的全部字符串，就要分成两段
	Bytes = CurLineRemainWidth / (dspode->height / 2);
	
	//如果当前行剩余可显示宽度不足以显示一个文字则直接换行
	if(Bytes <= 0)
	{
		//先换行再递归
		DEBUG_PRINTF;
		ChangLineInit(CurLineHeight);
		return ChangeLineAndScreen(dspode);
	}
	
	//检查分割处是汉字还是字母，如果分割出刚好是一个汉字就需要将分割出往前移动一个字节
	for(i = 0 ; i <= Bytes - 1 ; i++)
	{
		if(dspode->XKCellStr->strContent[i] < 0x80)
			break;
		counter += 1;
	}
	//汉字由两个字节表示，每个字节都大于0x80，如若大于0x80的字节个数非偶数，说明被分割处是一个汉字
	if(counter % 2 != 0)
		Bytes -= 1;
	else
		Bytes = Bytes;

	DEBUG_PRINTF;
	
	DSPNode_t DSPNode;
	char string[1024];
	memset(string,0,sizeof(string));
	memcpy(string,dspode->XKCellStr->strContent,sizeof(string));

	//直接把整个dspode->XKCellStr拷贝过来是为了继承他的所有属性
	XKCellString *XKCellStr = (XKCellString *)malloc(sizeof(XKCellString));
	memset(XKCellStr,0x00,sizeof(XKCellString));
	memcpy(XKCellStr,dspode->XKCellStr,sizeof(XKCellString));

	//将字符串清零并拷贝指定长度的字符串过来
	memset(XKCellStr->strContent,0x00,sizeof(XKCellStr->strContent));
	memcpy(XKCellStr->strContent,string,Bytes);
	XKCellStr->strContentLen = Bytes;
	DSPNode.XKCellStr = XKCellStr;
	DSPNode.type = DSPTYPE_STR;
	DSPNode.width = Bytes * (dspode->height / 2);
	DSPNode.height = dspode->height;
	DSPNode.time = dspode->time;
	DSPNode.effIn = dspode->effIn;
	DSPNode.effOut = dspode->effOut;
	InsertNode(&DSPNode);
	
	//换行
	DEBUG_PRINTF;
	debug_printf("Bytes = %d\n",Bytes);
	ChangLineInit(CurLineHeight);

	//以下是被换到下一行的内容
	int RemaindLen = dspode->XKCellStr->strContentLen - Bytes;
	memset(dspode->XKCellStr->strContent,0x00,1024);
	memcpy(dspode->XKCellStr->strContent,string + Bytes,RemaindLen);
	dspode->XKCellStr->strContentLen -= Bytes;
	dspode->width = RemaindLen * (dspode->XKCellStr->nFontSize / 2);
	//递归
	return ChangeLineAndScreen(dspode);
	
}



//把当前解析的出的每一个文字串、图片、光带、强制换行、强制换屏等统统当成一个DSPNode_t类型的节点插入到DSPNODE数组中
//在插入几点时，按照每个节点的显示元素在屏幕上从左到右，从上到下的相对位置来插入节点。举个例子说，第一个节点文字串
//先按照在屏幕上的原点来显示，如果没有强制换行，那么第二个节点文字串也接入第一个文字串，如果屏幕宽度不够完整的显示
//第二个文字串则自动换到下一行，当屏幕高度不够显示当前的文字串时则自动换刀下一幕显示。也就是说这个过程要先计算好每个
//节点在屏幕上显示的相对位置坐标，在插入到DSPNODE
static int msgprocess(DSPNode_t *dspode)
{
	//节点总数超数拒绝加入
	if(CurPos >= MAXNODE)
		return 0;
	
	//强制换行
	if(dspode->type == DSPTYPE_CGL)
	{
		ChangLineInit(dspode->height);
		return 0;
	}
		
	//强制换幕
	if(dspode->type == DSPTYPE_CGS)
	{
		ChangScreenInit();
		return 0;
	}

	//自动换行自动换幕
	return ChangeLineAndScreen(dspode);
}



static void ArgumentInit(void)
{
	CurPos = 0;
	CurXpos = 0;
	CurYpos = 0;
	Scene = 0;
	CurLineHeight = 0;
	CurLineRemainWidth = MSwidth;
	CurScrRemainHeight = MSheight;
}


//解析显示信息
static int Mdbs_CharCtrl(uint8_t *content,uint16_t len)
{
	DEBUG_PRINTF;
	int i = 0;

	char imgpwd[48];
	int imgWidth = 0,imgHeight = 0;
	
	uint8_t BMPtype = 0;
	//uint8_t CtrlWay = content[0];
	uint8_t CtrlWay = 0;
	uint16_t imagenum;
	uint16_t contentLen = len;
	uint32_t delayTime = 5;  //默认停留时间
	uint8_t bmpNum = 0,bmpType = 0;
	int effIn = MDBS_DIRECTLY,effOut = 0;
	int LBandFlag = LBSTATE_CLOSE;
	DSPNode_t DSPNode;
	
	//标记是否已经解析出任何一个信息节点
	int GetNodeFlag = 0;	//
	int ChineseCounter = 0;
	uint8_t fontSize = 0,font = 0;
	int Cx = 0,Cy = 0;
	XKCellString 	XKCellStr;
	XKCellImage		XKCellIma;
	LIGHTBand_t     LIGHTBand;
	uint8_t *ContentPos = NULL;

	//光带标志位清零
	LBANDFlag = 0;
	memset(MXKDisplay,0,sizeof(XKDisplayItem) * MAXSCRN);
	
	ArgumentInit();
	Mdbs_SetTxtRealTimeData(content,len);

#if 0 
	printf("list content is :\n");
	for(i = 0; i < len; i++)
	{
		printf("%02X ",content[i]);
	}
	printf("\n");
#endif
//修改
	if((content[2] < 0x09) && (content[4] < 0x09) && (content[5] < 0x09))
	{
		CtrlWay = CTRLWAY_WHOLE;
	}
	else
	{
		CtrlWay = CTRLWAY_ESCAPE;
	}



	//整体控制方式的参数保存
	if(CtrlWay == CTRLWAY_WHOLE)
	{
		effIn 		= content[2];
		delayTime 	= content[3];
		font 		= content[4];
		fontSize	= content[5];
		bmpNum		= content[6];
		bmpType		= content[7];
		ContentPos  = content + 8;
	}
	else
	{
		ContentPos  = content + 2;
	}
	
	memset(DSPNODE,0x00,sizeof(DSPNode_t) * MAXNODE);
	for(i = 0 ; i < 24 ; i++)
		Mdbs_INITDSPNodeDefVals(&DSPNODE[i]);

	InitContentlist(&ContentCache);

	memset(&XKCellStr,0x00,sizeof(XKCellString));
	memset(&XKCellIma,0x00,sizeof(XKCellImage));
	memset(&LIGHTBand,0x00,sizeof(LIGHTBand_t));
	
	Mdbs_INITStrDefVals(&XKCellStr,'h',24,COLOR_Y);

	//此处是发一幕黑屏的，当列表中无内容时显示一幕黑屏的
	if(*ContentPos == 0x00 && *(ContentPos + 1) == 0x00)
	{
		XKCellString *DSPStrNode = (XKCellString *)malloc(sizeof(XKCellString));
		memset(DSPStrNode,0x00,sizeof(XKCellString));
		memcpy(DSPStrNode,&XKCellStr,sizeof(XKCellString));
		DSPNode.type = DSPTYPE_STR;
		DSPNode.width = XKCellStr.nFontSize >> 1;
		DSPNode.height = XKCellStr.nFontSize;
		DSPNode.XKCellStr = DSPStrNode;
		msgprocess(&DSPNode);
		DSPNode.type = DSPTYPE_CGL;
		msgprocess(&DSPNode);
		DSPNode.type = DSPTYPE_CGS;
		msgprocess(&DSPNode);
		return 0;		
	}

	//根据转义字符解析显示信息
	while(*ContentPos != 0x00)
	{
		if(*ContentPos != 0x1B)
		{
			uint8_t charCNT = 0;
			GetNodeFlag = 1;
			memset(XKCellStr.strContent,0x00,sizeof(XKCellStr.strContent));
			ChineseCounter = Mdbs_GetChineseStr(ContentPos,XKCellStr.strContent,&charCNT);
			XKCellStr.strContent[ChineseCounter] = '\0';
			XKCellStr.strContentLen = ChineseCounter;
			XKCellStr.nCellOrder++;
			memset(&DSPNode,0,sizeof(DSPNode));

			XKCellString *NXKCellStr = (XKCellString *)malloc(sizeof(XKCellString));
			memset(NXKCellStr,0,sizeof(XKCellString));
			memcpy(NXKCellStr,&XKCellStr,sizeof(XKCellString));
			debug_printf("-------------------------------------------\n");

			for(i = 0 ; i < ChineseCounter ; i++)
				debug_printf("%02x ",XKCellStr.strContent[i]);
			debug_printf("\n");
			
			DSPNode.XKCellStr = NXKCellStr;
			DSPNode.type = DSPTYPE_STR;
			DSPNode.time = delayTime;
			DSPNode.effIn = effIn;
			DSPNode.width = ChineseCounter * (XKCellStr.nFontSize / 2);
			DSPNode.height = XKCellStr.nFontSize;
			debug_printf("DSPNode.height = %d\n",DSPNode.height);
			msgprocess(&DSPNode);
			debug_printf("curpos = %d\n",CurPos);
			DEBUG_PRINTF;
			ContentPos += ChineseCounter;
			
			continue;
		}


		ContentPos += 1;

		switch(*ContentPos)
		{
			//换行::::::
			//换行这个地方，在解析得出文字、图片、光带、动画任何一个节点之前换行符通通忽略掉
			//而在解析得出文字、图片等等之后的换行符不能忽略，如果连续出现两个换行符以上，则
			//第二个换行符后按照换行之前的字符大小所占的高度空出一个空行，该空行算作前一行文字所占有
			//即如果前一行文字的大小为32号，第二个换行符前的文字字号是24号,那么该行文字将占有32 + 24 = 56
			MDBS_charparse_printf("*ContentPos = 0x%x\n",*ContentPos);
			case 0x0A:
				ContentPos += 1;
				//GetNodeFlag == 0 表示还没有解析出文字或者图片或者光带，遇到换行符直接忽略掉掉
				if(GetNodeFlag == 0)	
					break;

				DSPNode.height = XKCellStr.nFontSize;
				DSPNode.type = DSPTYPE_CGL;
				msgprocess(&DSPNode);
				break;

			//换屏
			case 0x0D:
				DSPNode.type = DSPTYPE_CGS;
				msgprocess(&DSPNode);
				ContentPos += 1;
				debug_printf("\n\n=======================huan ping ===========\n");
				break;

			//字符颜色
			case 0x20://红
				XKCellStr.nForeColor[0] = 0xff;
				XKCellStr.nForeColor[1] = 0x00;
				XKCellStr.nForeColor[2] = 0x00;
				XKCellStr.Color = COLOR_R;
				ContentPos += 1;
				break;
			case 0x21://绿
				XKCellStr.nForeColor[0] = 0x00;
				XKCellStr.nForeColor[1] = 0xff;
				XKCellStr.nForeColor[2] = 0x00;
				XKCellStr.Color = COLOR_G;
				ContentPos += 1;
				break;
			case 0x22://橙
				XKCellStr.nForeColor[0] = 0xff;
				XKCellStr.nForeColor[1] = 0xff;
				XKCellStr.nForeColor[2] = 0x00;
				XKCellStr.Color = COLOR_Y;
				ContentPos += 1;
				break;

			//对齐方式
			case 0x30://上
			case 0x31://中(上下)
			case 0x32://下
			case 0x33://左
			case 0x34://中(左右)
			case 0x35://右
				align = AlignSelect(ContentPos[0]);
				ContentPos += 1;
				break;

			//点阵屏图片编码及类型
			case 0x36:
				if(ContentPos[1] == 0x30)
				{
					ContentPos += 3;
					break;
				}
				if(CtrlWay == CTRLWAY_ESCAPE)
				{
					imagenum = ContentPos[1] - 0x30;
					BMPtype = ImageSizeSelect(ContentPos[2] - 0x30);
				}
				else
				{
					imagenum = bmpNum;
					BMPtype = ImageSizeSelect(bmpType);
				}
				memset(XKCellIma.strImage,0x00,sizeof(XKCellIma.strImage));
				
				//自动适应当前文字的大小调用相应大小的图片
				if(XKCellStr.nFontSize == 16 ||XKCellStr.nFontSize == 24 || XKCellStr.nFontSize == 32 || 
				   XKCellStr.nFontSize == 48 || XKCellStr.nFontSize == 64)
				{
					DEBUG_PRINTF;
					sprintf(XKCellIma.strImage,"%d/%03x.bmp",BMPtype,imagenum);
					XKCellIma.strImage[11] = '\0';
					XKCellIma.type = XKCellStr.nFontSize;
					debug_printf("XKCellIma.strImage = %s\n",XKCellIma.strImage);
					//exit(1);
				}
				else
				{
					DEBUG_PRINTF;
					sprintf(XKCellIma.strImage,"%03x.bmp",imagenum);
					XKCellIma.strImage[7] = '\0';
				}
				DEBUG_PRINTF;
				//char imgpwd[48];
				memset(imgpwd,0,sizeof(imgpwd));
				sprintf(imgpwd,"%s/%s",image_dir,XKCellIma.strImage);
				debug_printf("imgpwd = %s\n",imgpwd);
				if(GetImageSize(imgpwd,&imgWidth,&imgHeight) < 0)
				{
					ContentPos += 3;
					break;
				}
				debug_printf("imgWidth = %d,imgHeight = %d\n",imgWidth,imgHeight);
				DSPNode.width = imgWidth;
				DSPNode.height = imgHeight;
				DSPNode.type = DSPTYPE_IMG;
				DSPNode.time = delayTime;
				DSPNode.effIn = effIn;

				XKCellImage *DSPimageNode = (XKCellImage *)malloc(sizeof(XKCellImage));
				memset(DSPimageNode,0x00,sizeof(XKCellImage));
				memcpy(DSPimageNode,&XKCellIma,sizeof(XKCellImage));
				DSPNode.XKCellIma = DSPimageNode;
				msgprocess(&DSPNode);
				ContentPos += 3;
				break;
				

			//出字方式
			case 0x37:
				if(CtrlWay == CTRLWAY_ESCAPE)
					effIn = ContentPos[1] - 0x30;
				ContentPos += 2;
				break;

			//间隔时间,s转化成us
			case 0x38:
				if(CtrlWay == CTRLWAY_ESCAPE)
					delayTime = (ContentPos[1] - 0x30) * 100 + (ContentPos[2] - 0x30) * 10 + (ContentPos[3] - 0x30);
				ContentPos += 4;
				debug_printf("delayTime = %d\n",delayTime);
				break;

			//字体
			case 0x39:
				if(CtrlWay == CTRLWAY_ESCAPE)
					XKCellStr.nFontType = FontSelect(ContentPos[1] - 0x30);
				else 
					XKCellStr.nFontType = FontSelect(font);
				ContentPos += 2;
				break;
				
			//字体大小
			case 0x3A:
				if(CtrlWay == CTRLWAY_ESCAPE)
					XKCellStr.nFontSize = FontSizeSelect(ContentPos[1] - 0x30);
				else
					XKCellStr.nFontSize = FontSizeSelect(fontSize);
				ContentPos += 2;
				break;
				
			//光带信息
			case 0x3B:
				LIGHTBand.LBnum = ContentPos[1];			
				ContentPos += 2; 

				//配置文件里面配置开启光带时才显示光带
				if(LBstate == LBSTATE_CLOSE)
					break;
				
				if(*ContentPos != 0x1B || *(ContentPos+1) != 0x3D)
				{
					LIGHTBand_t *NEWLIGHTBand = (LIGHTBand_t *)malloc(sizeof(LIGHTBand_t));
					LIGHTBand.LBandFunc = Mdbs_LightBandProcess;
					memcpy(NEWLIGHTBand,&LIGHTBand,sizeof(LIGHTBand_t));
					
					DSPNode.width = MSwidth;
					DSPNode.height = MSheight;
					DSPNode.type = DSPTYPE_LBD;
					DSPNode.time = delayTime;
					DSPNode.LIGHTBand = NEWLIGHTBand;
					msgprocess(&DSPNode);
					LBANDFlag = 1;
				}
				break;

			//文字坐标
			case 0x3C:
				Cx = (ContentPos[1] - 0x30) * 100 + (ContentPos[2] - 0x30) * 10 + (ContentPos[3] - 0x30);
				Cy = (ContentPos[4] - 0x30) * 100 + (ContentPos[5] - 0x30) * 10 + (ContentPos[6] - 0x30);
				ContentPos += 7;
				break;

			//光带坐标
			case 0x3D:
				LIGHTBand.Cx = (ContentPos[1] - 0x30) * 100 + (ContentPos[2] - 0x30) * 10 + (ContentPos[3] - 0x30);				
				LIGHTBand.Cy = (ContentPos[4] - 0x30) * 100 + (ContentPos[5] - 0x30) * 10 + (ContentPos[6] - 0x30);
				MDBS_charparse_printf("%d,%d\n",Cx,Cy);
				ContentPos += 7; 
				
				//配置文件里面配置开启光带时才显示光带
				if(LBstate == LBSTATE_CLOSE)
					break;
				
				if(*ContentPos != 0x1B || *(ContentPos+1) != 0x3B)
				{
					LIGHTBand_t *NEWLIGHTBand = (LIGHTBand_t *)malloc(sizeof(LIGHTBand_t));
					LIGHTBand.LBandFunc = Mdbs_LightBandProcess;
					memcpy(NEWLIGHTBand,&LIGHTBand,sizeof(LIGHTBand_t));
					
					DSPNode.width = MSwidth;
					DSPNode.height = MSheight;
					DSPNode.type = DSPTYPE_LBD;
					DSPNode.time = delayTime;
					DSPNode.LIGHTBand = NEWLIGHTBand;
					msgprocess(&DSPNode);
					LBANDFlag = 1;
				}
				break;
				
			//出字速度
			case 0x3E:
				break;

			//文字颜色
			case 0x3F:
				XKCellStr.nForeColor[0] = (ContentPos[1] - 0x30) * 100 + (ContentPos[2] - 0x30) * 10 + (ContentPos[3] - 0x30);
				XKCellStr.nForeColor[1] = (ContentPos[4] - 0x30) * 100 + (ContentPos[5] - 0x30) * 10 + (ContentPos[6] - 0x30);
				XKCellStr.nForeColor[2] = (ContentPos[7] - 0x30) * 100 + (ContentPos[8] - 0x30) * 10 + (ContentPos[9] - 0x30);
				ContentPos += 13;
				break;

			case 0x40:
				imagenum = (ContentPos[1] - 0x30) * 100 + (ContentPos[2] - 0x30) * 10 + (ContentPos[3] - 0x30);
				uint8_t imgtype = 0;
				imgtype = (ContentPos[4] - 0x30) * 100 + (ContentPos[5] - 0x30) * 10 + (ContentPos[6] - 0x30);
				//其实目前只实现bmp格式的显示
				if(imgtype != 1)
				{
					ContentPos += 7;
					break;
				}
				
				switch(imgtype)
				{
					case 1:sprintf(XKCellIma.strImage,"bmp/%03d.bmp",imagenum);DSPNode.type = DSPTYPE_IMG;break;
					case 2:sprintf(XKCellIma.strImage,"jpg/%03d.jpg",imagenum);DSPNode.type = DSPTYPE_JPG;break;
					case 3:sprintf(XKCellIma.strImage,"gif/%03d.gif",imagenum);DSPNode.type = DSPTYPE_GIF;break;
				}

				memset(imgpwd,0,sizeof(imgpwd));
				sprintf(imgpwd,"%s/%s",image_dir,XKCellIma.strImage);
				debug_printf("imgpwd = %s\n",imgpwd);
				if(GetImageSize(imgpwd,&imgWidth,&imgHeight) < 0)
				{
					ContentPos += 7;
					break;
				}
				//exit(1);
				debug_printf("imgWidth = %d,imgHeight = %d\n",imgWidth,imgHeight);
				DSPNode.width = imgWidth;
				DSPNode.height = imgHeight;
				DSPNode.time = delayTime;
				DSPNode.effIn = effIn;

				XKCellImage *DSPimage = (XKCellImage *)malloc(sizeof(XKCellImage));
				memset(DSPimage,0x00,sizeof(XKCellImage));
				memcpy(DSPimage,&XKCellIma,sizeof(XKCellImage));
				DSPNode.XKCellIma = DSPimage;
				msgprocess(&DSPNode);
				ContentPos += 7;
				break;

			case 0x41:
				break;
			default:
				break;
		}
	}

	//设置光带实时数据
	Mdbs_SetLBRealTimeData(LBstate);

	DSPNode.type = DSPTYPE_CGL;
	msgprocess(&DSPNode);
	DSPNode.type = DSPTYPE_CGS;
	msgprocess(&DSPNode);
}



//计算每个显示节点的精确坐标，函数msgprocess在将节点插入DSPNODE数组的时候已经将所有节点在每一屏上按照从左到右
//从上到下布局好相对位置，这个函数再根据对齐方式的要求精确计算出每个节点在屏幕上显示的横纵坐标
static int CaculateCoordinate(void)
{
	int LineMaxHeight = 0;
	int LineMaxWidth  = 0;
	
	int i = 0;
	int j = 0;
	int Line = 0;
	DSPNode_t *DSPNode = NULL;
	
	Scene = 0;
	Mdbs_DefInitcontentnode(&MXKDisplay[Scene]);
	for(i = 0 ; i < CurPos ; i++)
	{
		debug_printf("DSPNODE[%d].type = %d\n",i,DSPNODE[i].type);
		//所有节点不得超过最大节点数，所有幕数不得超过最大幕数
		if(i >= MAXNODE || Scene >= MAXSCRN)
			break;
		//遇到换幕标记结点，分别计算每个节点的坐标，由于每个节点的相对坐标都已经计算好(在处理换行换幕的时候已经计算好相对位置)
		//此处只需要对每个节点向右平移向下平移一定的像素点即可
		if(DSPNODE[i].type == DSPTYPE_CGS)
		{
			DSPNode = MXKDisplay[Scene].DSPNode_head;
			MXKDisplay[Scene].nDelayTime = MXKDisplay[Scene].nDelayTime * 1000 * 1000;
			while(DSPNode != NULL)
			{
				//根据对齐方式计算每一个显示节点的横纵坐标
				switch(align)
				{
					//上对齐，这里的上对齐显示置顶，左右居中
					case ALIGN_UP:
						DSPNode->Cx += (MSwidth - MXKDisplay[Scene].LineMst[DSPNode->Lseq].LineWidth) / 2;
						break;
					//下对齐,这里的下对齐显示置底，左右居中
					case ALIGN_DOWN:
						DSPNode->Cx += (MSwidth - MXKDisplay[Scene].LineMst[DSPNode->Lseq].LineWidth) / 2;
						DSPNode->Cy += (MSheight - MXKDisplay[Scene].ScrMaxHeight) + (MXKDisplay[Scene].LineMst[DSPNode->Lseq].LineHeight - DSPNode->height);
						break;
					//左对齐,这里的左对齐是显示顶左，上下居中
					case ALIGN_LEFT:
						DSPNode->Cy += (MSheight - MXKDisplay[Scene].ScrMaxHeight) / 2 + (MXKDisplay[Scene].LineMst[DSPNode->Lseq].LineHeight - DSPNode->height);
						break;
					//右对齐，这里的右对齐是显示顶右，上下居中
					case ALIGN_RIGHT:
						DSPNode->Cx += (MSwidth - MXKDisplay[Scene].LineMst[DSPNode->Lseq].LineWidth);
						DSPNode->Cy += (MSheight - MXKDisplay[Scene].ScrMaxHeight) / 2 + (MXKDisplay[Scene].LineMst[DSPNode->Lseq].LineHeight - DSPNode->height);
						break;
					//左右对齐、上下对齐、默认情况都是上下左右都居中
					case ALIGN_MINDLE_LEFTRIGHT:
					case ALIGN_MINDLE_UPDOWN:
					default:
						DSPNode->Cx += (MSwidth - MXKDisplay[Scene].LineMst[DSPNode->Lseq].LineWidth) / 2;
						DSPNode->Cy += (MSheight - MXKDisplay[Scene].ScrMaxHeight) / 2 + (MXKDisplay[Scene].LineMst[DSPNode->Lseq].LineHeight - DSPNode->height);
						break;
				}
				DSPNode = DSPNode->pNext;
			}
			Scene += 1;
			Mdbs_DefInitcontentnode(&MXKDisplay[Scene]);
			Line = 0;
			LineMaxWidth = 0;
			LineMaxHeight = 0;
			continue;
		}
		
		if(DSPNODE[i].type == DSPTYPE_CGL)
		{
			LineMaxWidth = 0;
			LineMaxHeight = 0;
			Line += 1;
			continue;
		}
		
		//标记该节点的行序与幕序
		DSPNODE[i].Lseq = Line;
		DSPNODE[i].Sseq = Scene;
		//获取行最大宽度与行最大高度
		LineMaxWidth += DSPNODE[i].width;
		LineMaxHeight = (LineMaxHeight > DSPNODE[i].height) ? LineMaxHeight : DSPNODE[i].height;
		MXKDisplay[Scene].LineMst[Line].LineWidth = LineMaxWidth;
		MXKDisplay[Scene].LineMst[Line].LineHeight= LineMaxHeight;
		//获取一幕信息整体最大宽度与最大高度，最大高度等于最后一行的纵坐标+最后一行的最大高度
		MXKDisplay[Scene].ScrMaxWidth = LineMaxWidth;
		MXKDisplay[Scene].ScrMaxHeight = DSPNODE[i].Cy + LineMaxHeight;
		MXKDisplay[Scene].nDelayTime = DSPNODE[i].time;
		MXKDisplay[Scene].nEffectIn = DSPNODE[i].effIn;
		AddItemDSPNode(&MXKDisplay[Scene],&DSPNODE[i]);
	}
}


int Mdbs_charparse(ContentList *head,uint8_t *Centent,uint16_t Len)
{
	uint8_t *Contentp = NULL;
	uint8_t CtrlWay;
	CtrlWay 	= Centent[0];

	//解析显示信息
	Mdbs_CharCtrl(Centent,Len);

	//计算每个显示信息的坐标
	CaculateCoordinate();

	//按幕信息将每一幕加入到待显示的列表中
	XKDisplayInsert(&MXKDisplay,Scene);

	return 0;
}

















