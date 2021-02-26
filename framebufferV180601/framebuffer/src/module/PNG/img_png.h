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
	int 	height; /* 尺寸 */
	int 	bit_depth;  /* 位深 */
	int 	color_type;
	int 	channels; //typedef unsigned char png_byte;
	int 	flag;  /* 一个标志，表示是否有alpha通道 */
	png_bytep *row_pointers; //二级指针
	png_structp png_ptr;
	png_infop  info_ptr;
	unsigned char *rgba; /* 图片数组 */
}PNGStruct_t;

int PNG_init(char *PNGfile);
void PNG_GetPngSize(uint16_t *width,uint16_t *height);
int PNG_GetRGB(uint8_t *RGBbyte,uint16_t Getwidth,uint16_t Getheight);
void PNG_destroy(void);

#endif

