#ifndef __SWR_CHARPARSE_H
#define __SWR_CHARPARSE_H
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "content.h"


#define SXKSIZE		48		//一个播放列表里面最多限制在幕,即48条Item
#define NODESIZE	24		//一条Item里面最多容许24个信息:文字、图片、gif
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

