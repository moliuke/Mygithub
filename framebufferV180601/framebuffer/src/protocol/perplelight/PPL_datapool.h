#ifndef __PPL_DATAPOOL_H
#define __PPL_DATAPOOL_H
#include "stdio.h"
#include "config.h"

//设置与读取当前显示内容
void PPL_SETCurDspContent(char *Content,uint16_t len,uint16_t inform,uint32_t inSpeed,uint32_t stayTime);
void PPL_GETCurDspContent(char *Content,uint16_t *len,uint16_t *inform,uint32_t *inSpeed,uint32_t *stayTime);

//设置与读取当前显示列表
void PPL_SETCurDspLst(char *list,uint8_t len);
void PPL_GETCurDspLst(char *list,uint8_t *len);

//设置与读取当前亮度值
void PPL_SETCurBright(uint8_t RGB_R,uint8_t RGB_G,uint8_t RGB_B);
void PPL_GETCurBright(uint8_t *RGB_R,uint8_t *RGB_G,uint8_t *RGB_B);

//设置与读取亮度模式
void PPL_SETBrightMode(uint8_t mode);
void PPL_GETBrightMode(uint8_t *mode);

#endif

