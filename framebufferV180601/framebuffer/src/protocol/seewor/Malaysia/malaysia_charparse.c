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


	//��ʼ����ʾһĻ�Ľṹ��
	SWR_DefInitcontentnode(&SXKDisplay[XKDSPnode_cur]);

	//����ʾ���һ��һ����Ϣ��DSPNODE���㲢��ʼ��
	memset(DSPNODE,0x00,sizeof(DSPNode_t) * 24);
	for(i = 0 ; i < 24 ; i++)
		SWR_INITDSPNodeDefVals(&DSPNODE[i]);

	//һ��Item���п��ܰ���IMG��STR,�����ݴ���Щ��Ϣ�Ľṹ���㲢��ʼ��
	memset(&XKCellStr,0,sizeof(XKCellString));
	memset(&XKCellIma,0,sizeof(XKCellImage));
	SWR_INITStrDefVals(&XKCellStr);
	SWR_INITImgDefVals(&XKCellIma);

	//����Item�Ľ���
	char *str_p = strchr(itemContent,'=');
	if(str_p == NULL)
		return -1;

	//Ļ��Ϣ�ĳ�ʼ����ÿһĻ������ͣ��ʱ�䡢��������
	itemData = str_p + 1;
	debug_printf("#itemData = %s\n",itemData);
	sscanf(itemData,"%d,%d,%d,%d,%d",&SXKDisplay[XKDSPnode_cur].nDelayTime,&SXKDisplay[XKDSPnode_cur].nEffectIn,&SXKDisplay[XKDSPnode_cur].nEffectShow,&SXKDisplay[XKDSPnode_cur].nEffectOut,&SXKDisplay[XKDSPnode_cur].nMoveSpeed);
	debug_printf("%d,%d,%d,%d,%d\n",SXKDisplay[XKDSPnode_cur].nDelayTime,SXKDisplay[XKDSPnode_cur].nEffectIn,SXKDisplay[XKDSPnode_cur].nEffectShow,SXKDisplay[XKDSPnode_cur].nEffectOut,SXKDisplay[XKDSPnode_cur].nMoveSpeed);

	//SXKDisplay[ItemOder].nEffectIn  = 1;//������ʾ
	//SXKDisplay[XKDSPnode_cur].nEffectOut = 1;//������ʾ
	SXKDisplay[XKDSPnode_cur].nDelayTime = 1000 * 1000 * SXKDisplay[XKDSPnode_cur].nDelayTime;//ͣ��ʱ�䣬��λ1/100��

	//�����浱ǰitem��û��ʵ�ʵ�����
	str_p = strchr(itemContent,'\\'); 
	if(str_p == NULL) 
		return -1;
	itemData = str_p;

	//��ϸ��Ϣ�Ľ���
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
				//��ͼƬ��ʾ��־���������������Ϣ���Զ��Ű潫������Ƿ����ͼƬ���Ű�
				IMGflag = 1;
				
				memcpy(imgname,itemData + 1,3);
				imgname[3] = '\0';
				Dir_LetterBtoL(imgname);

				memset(ImagePWD,0,sizeof(ImagePWD));
				sprintf(ImagePWD,"%s/%s.bmp",image_dir,imgname);
				debug_printf("ImagePWD = %s\n",ImagePWD);
				
				memset(XKCellIma.strImage,0x00,sizeof(XKCellIma.strImage));

				//ͼƬ���ȴ�image/Ŀ¼��Ѱ��
				if(access(ImagePWD,F_OK) == 0)
				{
					sprintf(XKCellIma.strImage,"%s.bmp",imgname);
					DEBUG_PRINTF;
				}
				
				//�����Ĭ��·���Ҳ���ʱ����ϵͳ����ͼƬ--->image/32����image/48
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
					//字体颜色
					XKCellStr.nForeColor[0] = (itemData[1]-0x30)*100 + (itemData[2]-0x30)*10 + itemData[3]-0x30;
					XKCellStr.nForeColor[1] = (itemData[4]-0x30)*100 + (itemData[5]-0x30)*10 + itemData[6]-0x30;
					XKCellStr.nForeColor[2] = (itemData[7]-0x30)*100 + (itemData[8]-0x30)*10 + itemData[9]-0x30;
					itemData +=13;
				}
				break;

			case 'B':
				if(itemData[1] == 't') // 默认
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
				if(itemData[1] == 't') // 默认
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

				//��Ϊ����������Ŀ���ų���������Ϣ
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
				
				//�����Ƿ���ͼƬҪ��ʾȷ���Ű��ȣ���ͼƬ��Ԥ��48��ȸ�ͼƬ�����ұ�
				composeWidth = (IMGflag) ? Swidth - 44 : Swidth;
				
				while(remaindCount)
				{
					XKCellString *DSPStrNode = NULL;
					
					remaindStr = ContentStr + charCounter - remaindCount;
					//��⻻�з����л��з�������
					if(*(remaindStr) == '\\' && *(remaindStr + 1) == 'N')
					{
						remaindStr += 2;
						remaindCount -= 2;
						continue;
					}
						
					//����Ƿ��л��з�,�л��з���Ԥ�趨���������з��������򽫶���һ�����ַ�
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


					//����������������Ԥ��ȡ�����ݵĴ�С������Ļ����ȶԱȣ����������Ļ����
					//�򰴼������Ļ�����ɵĵ�ǰ�ֺŵ��ַ��ĸ����������������Ľ�Ҫ��ȡ���ַ�������ȡ
					if(lineWidth >= composeWidth)
					{
						readCount = composeWidth / (XKCellStr.nFontSize / 2);
						DSPNODE[DSPnode_cur].width = readCount * (XKCellStr.nFontSize / 2);
					}
					else
					{
						DSPNODE[DSPnode_cur].width = lineWidth;
					}

					//�����ݲ�����ָ���Ĵ洢������
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


	//�˺���ÿ������һ�ξ�����ִ��һ��ITEM�Ľ��룬���������һ��ITEM����һĻ��Ϣ�������ǵ��û���ʹ�ø������
	//�п����û����������һĻ��ʾ�����Ҫ�л��µ���һĻ��ʾ������п��ܳ���һ��ITEM�м�Ļ������������ڴ���
	//ʱ�Ѿ��������Զ������뱻�����еĴ���ÿһ��DSPNODE����һ����Ϣ������Ĵ�����ǽ���Щ����Ϣ������װ�ɣ�
	//��һĻװ����ʱ������һĻ��Ϣ
	int curHeight = 0;
	int XKDSPnum = XKDSPnode_cur;
	int imgNum = 0;
	int imgflag = 0;
	int delayTime = SXKDisplay[XKDSPnum].nDelayTime;
	debug_printf("---DSPnode_cur = %d\n",DSPnode_cur);

	for(i = 0 ; i < DSPnode_cur ; i++)
	{
		//ֻ��������Ϣ��װ
		if(DSPNODE[i].type != DSPTYPE_STR)
		{
			imgflag = 1;
			imgNum = i;
			AddItemDSPNode(&SXKDisplay[XKDSPnum],&DSPNODE[i]);
			continue;
		}
#if 1

		//��һĻ��Ϣ��Ҳװ������һ��ITEMʱ��������һĻ��Ϣ�����Լ̳���һĻ��Ϣ������
		if(SXKDisplay[XKDSPnum].ScrMaxHeight + DSPNODE[i].XKCellStr->nFontSize > Sheight)
		{
			SXKDisplay[XKDSPnum].nDelayTime = delayTime / 2;
			memcpy(&SXKDisplay[XKDSPnum + 1],&SXKDisplay[XKDSPnum],sizeof(XKDisplayItem));
			SXKDisplay[XKDSPnum + 1].DSPNode_head = NULL;
			SXKDisplay[XKDSPnum + 1].DSPNode_tail = NULL;
			if(imgflag)
				AddItemDSPNode(&SXKDisplay[XKDSPnum + 1],&DSPNODE[imgNum]);
			
			//�̳���һĻ��Ϣ������
			XKDSPnum += 1;
			SXKDisplay[XKDSPnum].StrLines = 0;
			SXKDisplay[XKDSPnum].ScrMaxWidth = 0;
			SXKDisplay[XKDSPnum].ScrMaxHeight = 0;
		}

		//�������е������Ǽ��㵱ǰĻ������Ϣ����������������ȡ��������߶�
		SXKDisplay[XKDSPnum].StrLines += 1;
		SXKDisplay[XKDSPnum].ScrMaxWidth = (SXKDisplay[XKDSPnum].ScrMaxWidth > DSPNODE[i].width ) ? SXKDisplay[XKDSPnum].ScrMaxWidth : DSPNODE[i].width;
		SXKDisplay[XKDSPnum].ScrMaxHeight += DSPNODE[i].XKCellStr->nFontSize;
		debug_printf("SXKDisplay[%d].ScrMaxWidth = %d,SXKDisplay[%d].ScrMaxHeight = %d\n",XKDSPnum,SXKDisplay[XKDSPnum].ScrMaxWidth,XKDSPnum,SXKDisplay[XKDSPnum].ScrMaxHeight);
#endif
		//��ITEMѹ��Ļ��Ϣ�ṹ��
		AddItemDSPNode(&SXKDisplay[XKDSPnum],&DSPNODE[i]);
			
	}

	//����Ĵ�����������ÿһĻ��Ϣ����ÿһ�е����꣬Ҫ��ÿһĻ��Ϣ�����������Ҿ���
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

				//��ȡͼƬ��ͷ����Ϣ
				FILE *fp = fopen(filename,"r+");
				if(fp == NULL)
				{
					DSPNode = DSPNode->pNext;
					continue;
				}
				fread(imgMsg,1,26,fp);
				fclose(fp);

				//����ͷ����Ϣ�õ�ͼƬ�������ȸ��߶�
				imgWidth = (uint8_t)imgMsg[21] << 24 | (uint8_t)imgMsg[20] << 16 | (uint8_t)imgMsg[19] << 8 | (uint8_t)imgMsg[18];
				ingHeight = (uint8_t)imgMsg[25] << 24 | (uint8_t)imgMsg[24] << 16 | (uint8_t)imgMsg[23] << 8 | (uint8_t)imgMsg[22];

				//ͼƬ�����ȸ��߶ȴ���44�����ر���Ϊ������Ϣ��һ����ͼƬ184*48
				if(imgWidth > 44 || ingHeight > 44)
				{
					DSPNode->Cx = 0;
					DSPNode->Cy = 0;
					//memset(DSPNode->XKCellIma->strImage,0,sizeof(DSPNode->XKCellIma->strImage));
					DSPNode = DSPNode->pNext;
					continue;
				}

				//����ͼƬ�����44�����ص��ڱ���Ϊ��һ��СͼƬ����������Ҫ�Ű�
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

