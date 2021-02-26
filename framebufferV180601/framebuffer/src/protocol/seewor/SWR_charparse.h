#ifndef __SWR_CHARPARSE_H
#define __SWR_CHARPARSE_H
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "content.h"


#define SXKSIZE		48		//һ�������б��������������Ļ,��48��Item
#define NODESIZE	24		//һ��Item�����������24����Ϣ:���֡�ͼƬ��gif
extern DSPNode_t DSPNODE[NODESIZE];
extern XKDisplayItem SXKDisplay[SXKSIZE];


enum{SWR_LSTHEAD = 0,SWR_LSTCOUNT,SWR_LSTPARSE};
int SWR_Lstparsing(ContentList *head,const char *plist);
int SWR_itemparsing(ContentList *head,char *itemstr);
int SWR_PLstIntemDecode(ContentList *head,char *itemContent,uint8_t ItemOder);


void SWR_INITDSPNodeDefVals(DSPNode_t *DSPNode);
void Dir_LetterBtoL(char *dir);
int SWR_GetChineseStr(char *itemStr,char *ChineseStr);
void SWR_DefInitcontentnode(XKDisplayItem *head);
void SWR_INITGifDefVals(XKCellAnimate * pCellAnimate);
void SWR_INITStrDefVals(XKCellString * pCellString);
void SWR_INITImgDefVals(XKCellImage * pCellImag);
void SWR_INITDSPNodeDefVals(DSPNode_t *DSPNode);
int SWR_PNGMEMmalloc(uint32_t mallocSize);

extern int XKDSPnode_cur;
#endif

