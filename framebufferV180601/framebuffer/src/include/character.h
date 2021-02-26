#ifndef __CHARACTER_H
#define __CHARACTER_H

#include "config.h"

//#define CHAR_DEBUG
#ifdef CHAR_DEBUG
#define CHAR_DEBUG_PRINTF	DEBUG_PRINTF
#define char_debug_printf	debug_printf
#else
#define CHAR_DEBUG_PRINTF	
#define char_debug_printf	
#endif



#define 	FONT_CACHE_SIZE			(1024 * 192 * SCREEN_BPP)
typedef struct 
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
}Pixcolor_t;


typedef struct
{
	uint16_t 	ch_width;
	uint16_t 	ch_height;
	uint8_t  	ch_Bpp;
	uint16_t 	ch_hsize;
	uint16_t 	ch_csize;
	uint16_t 	ch_num;
	uint8_t 	ch_flag[512];
	uint8_t 	*ch_data;
}charattr_t;


typedef struct 
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
}Pcolor_t;

typedef char 			Ftype_t;
typedef unsigned char 	Fsize_t;
typedef char 			Ctent_t;
typedef struct txtstruct
{
	uint8_t			*ctent;			//������������ָ��
	uint8_t	 		content[512];	//�洢������������
	//�������cache��
	uint8_t 		ch_flag[512];	
	uint16_t 		ch_num;
	
	FILE 			*fpFont;
	FILE 			*fpAsc;
	char 			CHINAFile[48];
	char 			ASCIIFile[48];
	
	Ftype_t 		fontType;
	Fsize_t 		fontSize;
	
	uint16_t 		ch_hsize;
	uint16_t 		ch_csize;
	
	Pcolor_t		Fcolor;
	Pcolor_t		Bcolor;
	uint8_t 		BackgrdFlag;

	uint8_t 		Wspace;			//�ּ��
	uint8_t 		Lspace;			//�м��
	
	uint16_t 		cx;				//��ʾ����λ��
	uint16_t 		cy;

	uint32_t 		ctwidth;		//������cache��ռ�ĳ���ߴ�
	uint32_t 		ctheight;		//������cache��ռ�ݵĸ߶�

	uint32_t 		chwidth;		//cache����ĳ���ߴ�
	uint32_t 		chheight;		//cache����ĸ߶ȳߴ�

	uint8_t 		*cache;			//������ֽ������
	uint8_t 		BPP;
	uint32_t 		size;			//�����ֽ���
	

	int 			(*TXT_decoder)(struct txtstruct *);
}TXTstruct_t;

typedef struct 
{
	uint32_t 		r_cx;
	uint32_t 		r_cy;

	uint32_t 		w_cx;
	uint32_t 		w_cy;

	uint32_t 		r_jump;
	
	
}TXTDCstruct_t;


extern uint8_t *TXTCache;

void TXT_DecodeMemFree(uint8_t *DecodeCache);
void TXT_DecodeMemInit(uint8_t **DecodeCache,uint32_t size);
void TXT_INITstruct(TXTstruct_t *TXTstruct,uint8_t *content,uint16_t len,
	Ftype_t fontType,Fsize_t fontSize,Pcolor_t *Bcolor,Pcolor_t *Fcolor,
	uint8_t w_space,uint8_t l_space,uint8_t *cache,uint32_t size,uint8_t BPP);


#endif
