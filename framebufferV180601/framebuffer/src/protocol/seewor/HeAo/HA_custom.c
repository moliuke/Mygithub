#include "content.h"
#include "../../../cache.h"
#include "../SWR_charparse.h"

#include "HA_custom.h"
#include "../SWR_init.h"
#include "../../PTC_init.h"
#include "../SWR_charparse.h"
#include "../../../Hardware/Data_pool.h"

int HA_PLstIntemDecode(ContentList *head,char *itemContent,uint8_t ItemOder)
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

	//进入Item的解析
	char *str_p = strchr(itemContent,'=');
	if(str_p == NULL)
		return -1;

	//幕信息的初始化，每一幕入屏、停留时间、出屏动作
	itemData = str_p + 1;
	debug_printf("#itemData = %s\n",itemData);
	sscanf(itemData,"%d,%d,%d,%d,%d",&SXKDisplay[ItemOder].nDelayTime,&SXKDisplay[ItemOder].nEffectIn,&SXKDisplay[ItemOder].nEffectShow,&SXKDisplay[ItemOder].nEffectOut,&SXKDisplay[ItemOder].nMoveSpeed);
	debug_printf("=====%d,%d,%d,%d,%d\n",SXKDisplay[ItemOder].nDelayTime,SXKDisplay[ItemOder].nEffectIn,SXKDisplay[ItemOder].nEffectShow,SXKDisplay[ItemOder].nEffectOut,SXKDisplay[ItemOder].nMoveSpeed);

	SXKDisplay[ItemOder].nDelayTime = 1000 * 1000 * (SXKDisplay[ItemOder].nDelayTime / 10);//停留时间，单位1/100秒
	SXKDisplay[ItemOder].nEffectIn  = 1;
	SXKDisplay[ItemOder].nEffectOut = 1;
	
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



char *HeAoProject(void)
{
	return "HeAo";
}

#if 0
int _ProtocolRelation(void *arg1,void *arg2)
{
	itemDecoder = HA_PLstIntemDecode;
	projectstr = HeAoProject;
}
void _ProtocolRelationDestroy(void)
{
	
}
#endif



