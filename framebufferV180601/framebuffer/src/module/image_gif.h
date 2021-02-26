#ifndef __IMAGE_GIF_H
#define __IMAGE_GIF_H
#include <stdio.h>
#include "../include/config.h"
#include "gif_lib.h"
#include "display.h"


typedef enum
{
	GIFerr_GIF_OK,
	GIFerr_GIF_FAIL,	
	GIFerr_GET_FRAME_OK,
	GIFerr_TERMINATE,
	GIFerr_GET_TYPE_FAIL,
	GIFerr_GET_FRAME_FAIL,
	GIFerr_GET_FRAME_DATA_FAIL,
	GIFerr_GET_FRAME_EXTENT_FAIL,
	GIFerr_GET_NO_COLOR_MAP,
	GIFerr_FRAME_OVERFLOW,
}GIFerror_t;

typedef uint8_t *(*PCallback_t)(void *);


typedef struct gifstruct
{
	char 			filename[64];
	GifFileType 	*GifFile;
	GifRowType 		*ScreenBuffer;
	ColorMapObject 	*ColorMap;
	GifRecordType	RecordType;
	GifByteType 	*extension;

	uint32_t 		Extcode;
	uint32_t		trans_color;

	uint32_t 		cx;			//������ŵ�����
	uint32_t 		cy;			//���߽�������������������ݷ���ʲô����λ��

	uint32_t 		chwidth;	//����Ļ���ߴ磬���߽���������Ĵ�С
	uint32_t 		chheight;

	uint32_t 		ctwidth;
	uint32_t 		ctheight;

	uint16_t 		Fwidth;		//gif�Ŀ��
	uint16_t 		Fheight;	//gif�ĸ߶�
	
	uint8_t 		*cur_frame;	//��ǰ֡����
	uint8_t 		*pre_frame;	//��һ֡����
	void 			*arg;
	PCallback_t     writeAddr;	//�ص���������ȡҪд�Ļ���ĵ�ַ
	uint8_t 		endFlag;

	
	GIFerror_t 		(*GIF_init)(struct gifstruct *);
	GIFerror_t		(*GIF_free)(struct gifstruct *);
	GIFerror_t		(*GIF_close)(struct gifstruct *);

	GIFerror_t		(*frameInfo)(struct gifstruct *);
	GIFerror_t		(*frameDecoder)(struct gifstruct *);
	
	

}GIFstruct_t;

void GIF_INITstruct(GIFstruct_t *GIFstruct,char *gifpath,uint16_t cx,uint16_t cy,uint16_t c_width,uint16_t c_height,uint8_t *cache);

#endif


