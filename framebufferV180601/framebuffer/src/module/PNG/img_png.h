#ifndef __IMGPNG_H
#define __IMGPNG_H


//#define PNG_DEBUG

#ifdef PNG_DEBUG
#define PNG_DEBUG_PRINTF	DEBUG_PRINTF
#define png_debug_printf 	debug_printf 
#else
#define PNG_DEBUG_PRINTF	
#define png_debug_printf 	
#endif


typedef struct _pic_data
{
	FILE  	*PNGfp;
	int 	width;
	int 	height; /* �ߴ� */
	int 	bit_depth;  /* λ�� */
	int 	color_type;
	int 	channels; //typedef unsigned char png_byte;
	int 	flag;  /* һ����־����ʾ�Ƿ���alphaͨ�� */
	png_bytep *row_pointers; //����ָ��
	png_structp png_ptr;
	png_infop  info_ptr;
	unsigned char *rgba; /* ͼƬ���� */
}PNGStruct_t;

int PNG_init(char *PNGfile);
void PNG_GetPngSize(uint16_t *width,uint16_t *height);
int PNG_GetRGB(uint8_t *RGBbyte,uint16_t Getwidth,uint16_t Getheight);
void PNG_destroy(void);

#endif

