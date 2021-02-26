#include <stdio.h>

#include <png.h> 

#include <stdlib.h>

#include <malloc.h>

#include <string.h>

#include "config.h"
#include "img_png.h"
#include "../../include/Dev_framebuffer.h"
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;
#pragma pack(1)

typedef struct tagBITMAPFILEHEADER{
    WORD    bfType;                // the flag of bmp, value is "BM"
    DWORD    bfSize;                // size BMP file ,unit is bytes
    DWORD    bfReserved;            // 0
    DWORD    bfOffBits;            // must be 54
}BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
    DWORD    biSize;                // must be 0x28
    DWORD    biWidth;          //
    DWORD    biHeight;          //
    WORD            biPlanes;          // must be 1
    WORD            biBitCount;            //
    DWORD    biCompression;        //
    DWORD    biSizeImage;      //
    DWORD    biXPelsPerMeter;  //
    DWORD    biYPelsPerMeter;  //
    DWORD    biClrUsed;            //
    DWORD    biClrImportant;        //
}BITMAPINFOHEADER;
#pragma pack(4)
/******************************图片数据*********************************/


/**********************************************************************/

/*

    写入bmp数据为头

    fp:文件指针

    width:图像的宽度

    height:图像的高度

*/
int write_bmp_header(FILE *fp,int width, int height)
{
    BITMAPFILEHEADER  bf;
  BITMAPINFOHEADER  bi;
    //Set BITMAPINFOHEADER
    bi.biSize = 40;
    bi.biWidth = width;
    bi.biHeight = height;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = 0;
    bi.biSizeImage = height*width*3;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
//Set BITMAPFILEHEADER
  bf.bfType = 0x4d42;
  bf.bfSize = 54 + bi.biSizeImage;    
    bf.bfReserved = 0;
  bf.bfOffBits = 54;
  fwrite(&bf, 14, 1, fp); //先写54字节的头部数据
  fwrite(&bi, 40, 1, fp); 
    return 0;
}

/*

    将rgb数组数据写入bmp文件中

    filename:bmp文件名

    out:存在rgb数据的数组 格式为bgr bgr ...

*/

static int write_bmp(const char *filename, PNGStruct_t *out)
{
    FILE *fp;
    int width, height;
    int i, j, count=0, linesize=0;
    unsigned char  * lineData = NULL;
    width = out->width;
    height = out->height;
    linesize = width*3;
    count = height /2;
    if((fp = fopen(filename, "wb+")) == NULL){
        perror("fopen bmp error");
        return -1;
    }
    write_bmp_header(fp, width, height);
  lineData = (unsigned char*)malloc(linesize); 
  if(lineData == NULL){
      perror("malloc lineData error");
      return -1;
  }
    for(i=0; i<count; i++){    
      memcpy(lineData, out->rgba + (i*linesize), linesize);
      memcpy(out->rgba + (i*linesize), out->rgba+ (height - i -1)*linesize, linesize); 
      memcpy(out->rgba+ (height - i -1)*linesize, lineData, linesize);  
  }  
  fwrite(out->rgba, width*height*3, 1, fp); //图片旋转
    free(lineData);
  fclose(fp);  
    return 0;
}

#define PNG_BYTES_TO_CHECK 8

#define HAVE_ALPHA 1

#define NO_ALPHA 0

int check_if_png(char *file_name, FILE **fp)
{
  unsigned char buf[PNG_BYTES_TO_CHECK];
  /* Open the prospective PNG file. */
  if ((*fp = fopen(file_name, "rb")) == NULL)
      return 0;
  /* Read in some of the signature bytes */
  if (fread(buf, 1, PNG_BYTES_TO_CHECK, *fp) != PNG_BYTES_TO_CHECK)
      return 0;
  /* Compare the first PNG_BYTES_TO_CHECK bytes of the signature.
      Return nonzero (true) if they match */
    debug_printf("buf0 =%x buf1=%x buf2=%x buf3=%x\n",buf[0], buf[1], buf[2], buf[3]);
  return(!png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK)); //0错误 非0正确
}
/*

    获取png的数据

    filepath:文件名

    out:存放数据的rgb数组，格式bgr bgr ...

*/

int PNG_decoder(char *PNGfile, PNGStruct_t *PNGStruct)
/* 用于解码png图片 */
{
	FILE *pic_fp;
	int ret = -1;
	/* 初始化各种结构 */
	png_structp png_ptr;
	png_infop  info_ptr;
	
	/*检测是否为png文件*/
	if((ret = check_if_png(PNGfile,&pic_fp)) ==0)
	{
		debug_printf("not png file");
		return -1;
	}
	
	png_ptr  = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	setjmp(png_jmpbuf(png_ptr)); // 这句很重要
	rewind(pic_fp);
	/*开始读文件*/
	png_init_io(png_ptr, pic_fp); //文件指针赋值 
	//png_ptr->io_ptr = (png_voidp)fp;
	// png_voidp io_ptr;          /* ptr to application struct for I/O functions */ 
	// 读文件了
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);
	//#define PNG_TRANSFORM_EXPAND        0x0010    /* read only */
	int color_type,channels; //typedef unsigned char png_byte;
	
	/*获取宽度，高度，位深，颜色类型*/
	channels      = png_get_channels(png_ptr, info_ptr); /*获取通道数*/
	PNGStruct->bit_depth = png_get_bit_depth(png_ptr, info_ptr); /* 获取位深 */
	color_type    = png_get_color_type(png_ptr, info_ptr); /*颜色类型*/
	
	int i,j;
	int size, pos = 0;
	int temp;
	/* row_pointers里边就是rgba数据 */
	png_bytep* row_pointers; //二级指针
	row_pointers = png_get_rows(png_ptr, info_ptr); //获取二维数组的数据
	PNGStruct->width = png_get_image_width(png_ptr, info_ptr);
	PNGStruct->height = png_get_image_height(png_ptr, info_ptr);
	debug_printf("channels=%d depth=%d color_type=%d width=%d height=%d\n",channels, PNGStruct->bit_depth,color_type,PNGStruct->width,PNGStruct->height);
	
	/* 计算图片的总像素点数量 */
	size = PNGStruct->width * PNGStruct->height; 
	if(channels == 4 || color_type == PNG_COLOR_TYPE_RGB_ALPHA) //6
	{/*如果是RGB+alpha通道，或者RGB+其它字节*/ 
		size *= (3*sizeof(unsigned char)); /* 每个像素点占4个字节内存 */
		PNGStruct->flag = HAVE_ALPHA;    /* 标记 */
		PNGStruct->rgba = (unsigned char*) malloc(size);
		if(PNGStruct->rgba == NULL)
		{/* 如果分配内存失败 */
			fclose(pic_fp);
			puts("错误(png):无法分配足够的内存供存储数据!");
			return 1;
		}
		
		temp = (4 * PNGStruct->width);/* 每行有4 * out->width个字节 */
		for(i = 0; i < PNGStruct->height; i++)
		{
			for(j = 0; j < temp; j += 4)
			{/* 一个字节一个字节的赋值 */
				PNGStruct->rgba[pos++] = row_pointers[i][j+2];
				PNGStruct->rgba[pos++] = row_pointers[i][j+1];
				PNGStruct->rgba[pos++] = row_pointers[i][j+0]; 
			}
		}
	}
	
	else if(channels == 3 || color_type == PNG_COLOR_TYPE_RGB)//2
	{/* 如果是RGB通道 */
		size *= (3*sizeof(unsigned char)); /* 每个像素点占3个字节内存 */
		PNGStruct->flag = NO_ALPHA;    /* 标记 */
		PNGStruct->rgba = (unsigned char*) malloc(size);
		debug_printf("malloc\n");
		if(PNGStruct->rgba == NULL)
		{/* 如果分配内存失败 */
			fclose(pic_fp);
			puts("错误(png):无法分配足够的内存供存储数据!");
			return 1;
		}
		temp = (3 * PNGStruct->width);/* 每行有3 * out->width个字节 */
		for(i = 0; i < PNGStruct->height; i++)
		{
			for(j = 0; j < temp; j += 3)
			{/* 一个字节一个字节的赋值 */
				PNGStruct->rgba[pos++] = row_pointers[i][j+2];
				PNGStruct->rgba[pos++] = row_pointers[i][j+1];
				PNGStruct->rgba[pos++] = row_pointers[i][j+0];  
			}
		}
	}
	
	else 
		return 1;
	
	/* 撤销数据占用的内存 */
	png_destroy_read_struct(&png_ptr, &info_ptr, 0); 
	//free(out->rgba);
	return 0;
}




static PNGStruct_t PNGStruct;
int PNG_init(char *PNGfile)
{
	int ret = -1;
	/* 初始化各种结构 */
	png_debug_printf("PNGfile = %s\n",PNGfile);
	
	/*检测是否为png文件*/
	if((ret = check_if_png(PNGfile,&PNGStruct.PNGfp)) ==0)
	{
		debug_printf("not png file\n");
		return -1;
	}
	
	PNGStruct.png_ptr  = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (PNGStruct.png_ptr == NULL)
	{
		DEBUG_PRINTF;
	   fclose(PNGStruct.PNGfp);
	   return (-1);
	}
	
	PNGStruct.info_ptr = png_create_info_struct(PNGStruct.png_ptr);
	if (PNGStruct.info_ptr == NULL)
	{
		DEBUG_PRINTF;
	   fclose(PNGStruct.PNGfp);
	   png_destroy_read_struct(&PNGStruct.png_ptr, 0, 0);
	   return (-1);
	}

	
	if(setjmp(png_jmpbuf(PNGStruct.png_ptr))) // 这句很重要
	{
		png_destroy_read_struct(&PNGStruct.png_ptr, &PNGStruct.info_ptr, 0);
		fclose(PNGStruct.PNGfp);
		DEBUG_PRINTF;
		/* If we get here, we had a problem reading the file */
		return -1;
	}
	rewind(PNGStruct.PNGfp); 
	/*开始读文件*/
	png_init_io(PNGStruct.png_ptr, PNGStruct.PNGfp); //文件指针赋值 
	//png_ptr->io_ptr = (png_voidp)fp;
	// png_voidp io_ptr;          /* ptr to application struct for I/O functions */ 
	// 读文件了
	DEBUG_PRINTF;
	png_set_sig_bytes(PNGStruct.png_ptr,0);
	DEBUG_PRINTF;
	png_read_png(PNGStruct.png_ptr, PNGStruct.info_ptr, PNG_TRANSFORM_EXPAND, 0);
	//#define PNG_TRANSFORM_EXPAND        0x0010    /* read only */
	
	/*获取宽度，高度，位深，颜色类型*/
	PNGStruct.channels      = png_get_channels(PNGStruct.png_ptr, PNGStruct.info_ptr); /*获取通道数*/
	PNGStruct.bit_depth = png_get_bit_depth(PNGStruct.png_ptr, PNGStruct.info_ptr); /* 获取位深 */
	PNGStruct.color_type    = png_get_color_type(PNGStruct.png_ptr, PNGStruct.info_ptr); /*颜色类型*/
		
	/* row_pointers里边就是rgba数据 */
	PNGStruct.row_pointers = png_get_rows(PNGStruct.png_ptr, PNGStruct.info_ptr); //获取二维数组的数据
	PNGStruct.width = png_get_image_width(PNGStruct.png_ptr, PNGStruct.info_ptr);
	PNGStruct.height = png_get_image_height(PNGStruct.png_ptr, PNGStruct.info_ptr);
	png_debug_printf("channels=%d depth=%d color_type=%d width=%d height=%d\n",PNGStruct.channels, PNGStruct.bit_depth,PNGStruct.color_type,PNGStruct.width,PNGStruct.height);

	return 0;
}

void PNG_GetPngSize(uint16_t *width,uint16_t *height)
{
	*width = PNGStruct.width;
	*height = PNGStruct.height;
}


int PNG_GetRGB(uint8_t *RGBbyte,uint16_t Getwidth,uint16_t Getheight)
{
	int i,j;
	int pos = 0;
	int temp;  

	//uint32_t RGBsize = PNGStruct.width * PNGStruct.height * 3;

	//png_debug_printf("RGBsize = %d,size = %d,PNGStruct.width = %d,PNGStruct.height = %d\n",RGBsize,size,PNGStruct.width,PNGStruct.height);
	//if(RGBsize > size)
	//	return -1;

	if(PNGStruct.channels == 4 || PNGStruct.color_type == PNG_COLOR_TYPE_RGB_ALPHA) //6
	{/*如果是RGB+alpha通道，或者RGB+其它字节*/ 
		PNGStruct.flag = HAVE_ALPHA;	 /* 标记 */
		png_debug_printf("PNGStruct.height = %d\n",PNGStruct.height);
		temp = (4 * Getwidth);/* 每行有4 * out->width个字节 */
		for(i = 0; i < Getheight; i++)
		{
			DEBUG_PRINTF;
			for(j = 0; j < temp; j += 4)
			{/* 一个字节一个字节的赋值 */
					RGBbyte[pos++] = PNGStruct.row_pointers[i][j+2];
					RGBbyte[pos++] = PNGStruct.row_pointers[i][j+1];
					RGBbyte[pos++] = PNGStruct.row_pointers[i][j+0]; 
			}
		}
		//fb_frame_display(0,0,0,0,DSPbuf,32,1000);
	}
	else if(PNGStruct.channels == 3 || PNGStruct.color_type == PNG_COLOR_TYPE_RGB)//2
	{/* 如果是RGB通道 */
		PNGStruct.flag = NO_ALPHA;    /* 标记 */
		temp = (3 * Getwidth);/* 每行有3 * out->width个字节 */
		for(i = 0; i < Getheight; i++)
		{
			for(j = 0; j < temp; j += 3)
			{/* 一个字节一个字节的赋值 */
				RGBbyte[pos++] = PNGStruct.row_pointers[i][j+2];
				RGBbyte[pos++] = PNGStruct.row_pointers[i][j+1];
				RGBbyte[pos++] = PNGStruct.row_pointers[i][j+0];	
			}
		}
	}

	else 
		return -1;

	
	//fb_frame_display(0,0,0,0,DSPbuf,32,1000);
	PNG_DEBUG_PRINTF;
	return 0;
}

void PNG_destroy(void)
{
	/* 撤销数据占用的内存 */
	png_destroy_read_struct(&PNGStruct.png_ptr, &PNGStruct.info_ptr, 0); 
	fclose(PNGStruct.PNGfp);
}


int PNG_Display(uint16_t Cx,uint16_t Cy,uint8_t *DSPbuf)
{
	uint16_t width,height;

	
}



/*
    写入数据到png文件
    file_name:写入数据的文件名
    graph:数据的rgb数组 格式为bgr bgr排放
*/
/* 功能：将LCUI_Graph结构中的数据写入至png文件 */
int write_png_file(char *file_name , PNGStruct_t *graph)
{
	int j, i, temp, pos;
	png_byte color_type;
	png_structp png_ptr;
	png_infop info_ptr; 
	png_bytep * row_pointers;
	/* create file */
	FILE *fp = fopen(file_name, "wb");
	if (!fp)
	{
		debug_printf("[write_png_file] File %s could not be opened for writing", file_name);
		return -1;
	}
	/* initialize stuff */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		debug_printf("[write_png_file] png_create_write_struct failed");
		return -1;
	}
	
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		debug_printf("[write_png_file] png_create_info_struct failed");
		return -1;
	}
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		debug_printf("[write_png_file] Error during init_io");
		return -1;
	}
	png_init_io(png_ptr, fp);
	/* write header */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		debug_printf("[write_png_file] Error during writing header");
		return -1;
	}

	
	/* 判断要写入至文件的图片数据是否有透明度，来选择色彩类型 */
	if(graph->flag == HAVE_ALPHA) 
		color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	else 
		color_type = PNG_COLOR_TYPE_RGB;

	
	png_set_IHDR(png_ptr, info_ptr, graph->width, graph->height,
	graph->bit_depth, color_type, PNG_INTERLACE_NONE,
	PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(png_ptr, info_ptr);
	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_debug_printf("[write_png_file] Error during writing bytes");
		return -1;
	}

	
	if(graph->flag == HAVE_ALPHA) 
		temp = (4 * graph->width);
	else 
		temp = (3 * graph->width);

	
	pos = 0;
	row_pointers = (png_bytep*)malloc(graph->height*sizeof(png_bytep));
	for(i = 0; i < graph->height; i++)
	{
		row_pointers[i] = (png_bytep)malloc(sizeof(unsigned char)*temp);
		for(j = 0; j < temp; j += 3)
		{
			// row_pointers[i][j]  = graph->rgba[0][pos]; // red
			// row_pointers[i][j+1] = graph->rgba[1][pos]; // green
			// row_pointers[i][j+2] = graph->rgba[2][pos];  // blue
			row_pointers[i][j+2] = graph->rgba[pos++];
			row_pointers[i][j+1] = graph->rgba[pos++];
			row_pointers[i][j+0] = graph->rgba[pos++];
			//if(graph->flag == HAVE_ALPHA) 
			// row_pointers[i][j+3] = graph->rgba[3][pos]; // alpha
			//++pos;
		}
	}
	
	png_write_image(png_ptr, row_pointers);
	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_debug_printf("[write_png_file] Error during end of write");
		return -1;
	}
	png_write_end(png_ptr, NULL);
	/* cleanup heap allocation */
	for (j=0; j<graph->height; j++)
		free(row_pointers[j]);
	
	free(row_pointers);
	fclose(fp);
	return 0;
}






#if 0

int main(int argc, char *argv[]) //规则图片效果较好

{

    if(argc == 3){ //将png图片转化成bmp图片，argv[1]为png文件名 argv[2]为bmp文件名。

        PNGStruct_t out;

        PNG_decoder(argv[1], &out);

        write_bmp(argv[2], &out);       

        free(out.rgba);

    }

    return 0;

}

#endif


