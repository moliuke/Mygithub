#include "malaysia_charparse.h"

#include "content.h"
#include "../../../cache.h"
#include "../SWR_charparse.h"
#include "../../../Hardware/Data_pool.h"


static uint32_t Swidth,Sheight;

#if 1
int malaysia_PLstIntemDecode(ContentList *head,char *itemContent,uint8_t ItemOder)
{
	uint8_t i = 0;
	int charCounter = 0;
	int Cx = 0,Cy = 0;
	char ChineseStr[256];
	char *imagepwd = NULL;
	char ImagePWD[64];
	char *itemData = NULL;
	uint8_t BMPtype = 0;
	uint8_t imgname[4];
    int nCellOrder = 0;
	int DSPnode_cur = 0;
	int IMGflag = 0;
	char ContentStr[1024];
	int composeWidth = 0;
	XKCellString 	XKCellStr;
	XKCellImage		XKCellIma;

	if(ItemOder > SXKSIZE)
		return 0;

	DSPnode_cur = 0;
	DP_GetScreenSize(&Swidth,&Sheight);


	//初始化表示一幕的结构体
	SWR_DefInitcontentnode(&SXKDisplay[XKDSPnode_cur]);

	//将表示存放一个一个信息的DSPNODE清零并初始化
	memset(DSPNODE,0x00,sizeof(DSPNode_t) * 24);
	for(i = 0 ; i < 24 ; i++)
		SWR_INITDSPNodeDefVals(&DSPNODE[i]);

	//一条Item中有可能包含IMG、STR,将将暂存这些信息的结构清零并初始化
	memset(&XKCellStr,0,sizeof(XKCellString));
	memset(&XKCellIma,0,sizeof(XKCellImage));
	SWR_INITStrDefVals(&XKCellStr);
	SWR_INITImgDefVals(&XKCellIma);

	//进入Item的解析
	char *str_p = strchr(itemContent,'=');
	if(str_p == NULL)
		return -1;

	//幕信息的初始化，每一幕入屏、停留时间、出屏动作
	itemData = str_p + 1;
	debug_printf("#itemData = %s\n",itemData);
	sscanf(itemData,"%d,%d,%d,%d,%d",&SXKDisplay[XKDSPnode_cur].nDelayTime,&SXKDisplay[XKDSPnode_cur].nEffectIn,&SXKDisplay[XKDSPnode_cur].nEffectShow,&SXKDisplay[XKDSPnode_cur].nEffectOut,&SXKDisplay[XKDSPnode_cur].nMoveSpeed);
	debug_printf("%d,%d,%d,%d,%d\n",SXKDisplay[XKDSPnode_cur].nDelayTime,SXKDisplay[XKDSPnode_cur].nEffectIn,SXKDisplay[XKDSPnode_cur].nEffectShow,SXKDisplay[XKDSPnode_cur].nEffectOut,SXKDisplay[XKDSPnode_cur].nMoveSpeed);

	//SXKDisplay[ItemOder].nEffectIn  = 1;//立即显示
	//SXKDisplay[XKDSPnode_cur].nEffectOut = 1;//立即显示
	SXKDisplay[XKDSPnode_cur].nDelayTime = 1000 * 1000 * SXKDisplay[XKDSPnode_cur].nDelayTime;//停留时间，单位1/100秒

	//检测后面当前item有没有实际的内容
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
				itemData += 7;
				break;

			case 'I':
				//有图片显示标志，后面解码文字信息与自动排版将会根据是否存在图片来排版
				IMGflag = 1;
				
				memcpy(imgname,itemData + 1,3);
				imgname[3] = '\0';
				Dir_LetterBtoL(imgname);

				memset(ImagePWD,0,sizeof(ImagePWD));
				sprintf(ImagePWD,"%s/%s.bmp",image_dir,imgname);
				debug_printf("ImagePWD = %s\n",ImagePWD);
				
				memset(XKCellIma.strImage,0x00,sizeof(XKCellIma.strImage));

				//图片优先从image/目录下寻找
				if(access(ImagePWD,F_OK) == 0)
				{
					sprintf(XKCellIma.strImage,"%s.bmp",imgname);
					DEBUG_PRINTF;
				}
				
				//上面的默认路径找不到时调用系统内置图片--->image/32或者image/48
				else
				{
					if(Sheight < 48)
						BMPtype = 32;
					else
						BMPtype = 48;
					sprintf(XKCellIma.strImage,"%d/%s.bmp",BMPtype,imgname);
				}

				debug_printf("XKCellIma.strImage = %s\n",XKCellIma.strImage);

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
				itemData += 4;
				break;

			case 'O':
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
				memset(ContentStr,0,sizeof(ContentStr));
				charCounter = SWR_GetChineseStr(itemData+1,ContentStr);

				//此为马来西亚项目，排除掉中文信息
				if((uint8_t)ContentStr[0] >= 0xa0)
				{
					itemData += charCounter + 1;
					break;
				}
				
				int remaindCount = charCounter;
				int ContentOffset = 0;
				char *remaindStr = NULL;
				char *newLine = NULL;
				int lineWidth = 0;
				int readCount = 0;
				int newLineFlag = 0;
				
				//根据是否有图片要显示确定排版宽度，有图片则预留48宽度给图片，在右边
				composeWidth = (IMGflag) ? Swidth - 44 : Swidth;
				
				while(remaindCount)
				{
					XKCellString *DSPStrNode = NULL;
					
					remaindStr = ContentStr + charCounter - remaindCount;
					//检测换行符，有换行符则跳过
					if(*(remaindStr) == '\\' && *(remaindStr + 1) == 'N')
					{
						remaindStr += 2;
						remaindCount -= 2;
						continue;
					}
						
					//检查是否有换行符,有换行符则预设定最多读到换行符处，否则将读完一整串字符
					if((newLine = strstr(remaindStr,"\\N")) != NULL)
					{
						readCount = newLine - remaindStr;
						lineWidth = readCount * (XKCellStr.nFontSize / 2);
					}
					else
					{
						readCount = remaindCount;
						lineWidth = readCount * (XKCellStr.nFontSize / 2);
					}


					//根据上面计算出来的预读取的内容的大小，与屏幕最大宽度对比，如果大于屏幕最大宽
					//则按计算出屏幕能容纳的当前字号的字符的个数，否则按上面计算的将要读取的字符个数读取
					if(lineWidth >= composeWidth)
					{
						readCount = composeWidth / (XKCellStr.nFontSize / 2);
						DSPNODE[DSPnode_cur].width = readCount * (XKCellStr.nFontSize / 2);
					}
					else
					{
						DSPNODE[DSPnode_cur].width = lineWidth;
					}

					//读数据并填入指定的存储缓存中
					memcpy(XKCellStr.strContent,remaindStr,readCount);
					XKCellStr.strContent[readCount] = '\0';
					XKCellStr.strContentLen = readCount;
					
					DSPStrNode = (XKCellString *)malloc(sizeof(XKCellString));
					memset(DSPStrNode,0x00,sizeof(XKCellString));
					memcpy(DSPStrNode,&XKCellStr,sizeof  (XKCellString));
					
					DSPNODE[DSPnode_cur].height = XKCellStr.nFontSize;
					DSPNODE[DSPnode_cur].type = DSPTYPE_STR;
					DSPNODE[DSPnode_cur].XKCellStr = DSPStrNode;
					debug_printf("str %d : %s\n",DSPnode_cur,DSPNODE[DSPnode_cur].XKCellStr->strContent);
					
					DSPnode_cur += 1;
					remaindCount -= readCount;
					
				}

				itemData += charCounter + 1;
				break;
		}
	}


	//此函数每被调用一次就是在执行一条ITEM的解码，正常情况下一条ITEM就是一幕信息，但考虑到用户的使用各种情况
	//有可能用户输入的内容一幕显示不完就要切换下到下一幕显示，这就有可能出现一条ITEM有几幕的情况。上面在处理
	//时已经做好了自动换行与被动换行的处理，每一个DSPNODE就是一行信息，下面的代码就是将这些行信息重新组装成，
	//当一幕装不完时则增加一幕信息
	int curHeight = 0;
	int XKDSPnum = XKDSPnode_cur;
	int imgNum = 0;
	int imgflag = 0;
	int delayTime = SXKDisplay[XKDSPnum].nDelayTime;
	debug_printf("---DSPnode_cur = %d\n",DSPnode_cur);

	for(i = 0 ; i < DSPnode_cur ; i++)
	{
		//只对文字信息组装
		if(DSPNODE[i].type != DSPTYPE_STR)
		{
			imgflag = 1;
			imgNum = i;
			AddItemDSPNode(&SXKDisplay[XKDSPnum],&DSPNODE[i]);
			continue;
		}
#if 1

		//当一幕信息在也装不下下一条ITEM时，创建另一幕信息，属性继承上一幕信息的属性
		if(SXKDisplay[XKDSPnum].ScrMaxHeight + DSPNODE[i].XKCellStr->nFontSize > Sheight)
		{
			SXKDisplay[XKDSPnum].nDelayTime = delayTime / 2;
			memcpy(&SXKDisplay[XKDSPnum + 1],&SXKDisplay[XKDSPnum],sizeof(XKDisplayItem));
			SXKDisplay[XKDSPnum + 1].DSPNode_head = NULL;
			SXKDisplay[XKDSPnum + 1].DSPNode_tail = NULL;
			if(imgflag)
				AddItemDSPNode(&SXKDisplay[XKDSPnum + 1],&DSPNODE[imgNum]);
			
			//继承上一幕信息的属性
			XKDSPnum += 1;
			SXKDisplay[XKDSPnum].StrLines = 0;
			SXKDisplay[XKDSPnum].ScrMaxWidth = 0;
			SXKDisplay[XKDSPnum].ScrMaxHeight = 0;
		}

		//下面三行的作用是计算当前幕所有信息的行数、横向最大宽度、纵向最大高度
		SXKDisplay[XKDSPnum].StrLines += 1;
		SXKDisplay[XKDSPnum].ScrMaxWidth = (SXKDisplay[XKDSPnum].ScrMaxWidth > DSPNODE[i].width ) ? SXKDisplay[XKDSPnum].ScrMaxWidth : DSPNODE[i].width;
		SXKDisplay[XKDSPnum].ScrMaxHeight += DSPNODE[i].XKCellStr->nFontSize;
		debug_printf("SXKDisplay[%d].ScrMaxWidth = %d,SXKDisplay[%d].ScrMaxHeight = %d\n",XKDSPnum,SXKDisplay[XKDSPnum].ScrMaxWidth,XKDSPnum,SXKDisplay[XKDSPnum].ScrMaxHeight);
#endif
		//将ITEM压入幕信息结构体
		AddItemDSPNode(&SXKDisplay[XKDSPnum],&DSPNODE[i]);
			
	}

	//下面的代码的作用针对每一幕信息计算每一行的坐标，要求每一幕信息整体上下左右居中
	for(i = XKDSPnode_cur ; i <= XKDSPnum ; i++)
	{
		int curHeight = 0;
		DSPNode_t *DSPNode = NULL;
		int YSpace = 0;

		if(i != XKDSPnum)
			SXKDisplay[i].nDelayTime = delayTime / 2;
		else
			SXKDisplay[i].nDelayTime = delayTime;

		if(SXKDisplay[i].StrLines == 3)
		{
			YSpace = (Sheight - SXKDisplay[i].ScrMaxHeight) / 2;
			curHeight = 0;
		}
		else 
		{
			YSpace = (Sheight - SXKDisplay[i].ScrMaxHeight) / (SXKDisplay[i].StrLines + 1);
			curHeight = YSpace;
		}
		DSPNode = SXKDisplay[i].DSPNode_head;
		while(DSPNode != NULL)
		{
			if(DSPNode->type == DSPTYPE_STR)
			{
				DSPNode->Cx = (composeWidth - DSPNode->width) / 2;
				DSPNode->Cy = curHeight;
				curHeight = DSPNode->Cy + DSPNode->height + YSpace;
			}
			else
			{
				char filename[64];
				char imgMsg[28];
				int imgWidth = 0,ingHeight = 0;
				memset(imgMsg,0,sizeof(imgMsg));
				memset(filename,0,sizeof(filename));
				sprintf(filename,"%s/%s",image_dir,DSPNode->XKCellIma->strImage);

				//读取图片的头部信息
				FILE *fp = fopen(filename,"r+");
				if(fp == NULL)
				{
					DSPNode = DSPNode->pNext;
					continue;
				}
				fread(imgMsg,1,26,fp);
				fclose(fp);

				//根据头部信息得到图片的整体宽度跟高度
				imgWidth = (uint8_t)imgMsg[21] << 24 | (uint8_t)imgMsg[20] << 16 | (uint8_t)imgMsg[19] << 8 | (uint8_t)imgMsg[18];
				ingHeight = (uint8_t)imgMsg[25] << 24 | (uint8_t)imgMsg[24] << 16 | (uint8_t)imgMsg[23] << 8 | (uint8_t)imgMsg[22];

				//图片整体宽度跟高度大于44个像素被认为播放信息是一整张图片184*48
				if(imgWidth > 44 || ingHeight > 44)
				{
					DSPNode->Cx = 0;
					DSPNode->Cy = 0;
					//memset(DSPNode->XKCellIma->strImage,0,sizeof(DSPNode->XKCellIma->strImage));
					DSPNode = DSPNode->pNext;
					continue;
				}

				//否则图片宽高在44个像素点内被认为是一张小图片，横纵坐标要排版
				DSPNode->Cx = composeWidth + (44 - imgWidth) / 2;
				DSPNode->Cy = 0 + (44 - ingHeight) / 2;
			}
			DSPNode = DSPNode->pNext;
		}
		YSpace = 0;
	}

	XKDSPnode_cur = XKDSPnum;
	return 0;
}

#endif

