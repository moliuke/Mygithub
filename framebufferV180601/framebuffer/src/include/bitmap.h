#ifndef _LI_BITMAP_H_

#define _LI_BITMAP_H_

 #pragma pack(push, 1)


#include "Dev_framebuffer.h"
/*****Grup for the member @mapValid of the structrure @BitMapData_t****/
#define MAP_VALID		1
#define MAP_INVALID		0



 

 typedef unsigned char  U8;

typedef unsigned short U16;

typedef unsigned int   U32;


/*****************************************************
NOTE:
图片中每一行的字节数必须是4的倍数，
在存储图片时，图片的数据已经自动补0
，所以在解码时每行数据必须按照4 的倍数
来读取图片数据
*******************************************************/

/***************************************************
位图文件主要分为如下4个部分：

块名称 　			对应Windows结构体定义    大小（byte） 

 文件信息头 		BITMAPFILEHEADER　　　　　14 

位图信息头  		BITMAPINFOHEADER　　　　     40 

颜色表（调色板）RGBQUAD　　　　　　（可选） 

 位图数据（RGB颜色阵列） 　　　　　BYTE* 由图像长宽尺寸决定 

****************************************************/

 
/***************************************************
1 、位图文件头
****************************************************/
typedef struct
{
	 U16 	bfType;			//指定文件类型，必须是0x424D，即字符串“BM”
	 U32 	bfSize;			//指定文件大小，包括这14个字节。
	 U16 	bfReserved1;	//为保留字，不用考虑
	 U16 	bfReserved2;	//为保留字，不用考虑
	 U32 	bfOffBits;		//从文件头到实际的位图数据的偏移字节数，即图中前三个部分的长度之和
} BitMapFileHeader_t;


/***************************************************
2、位图信息头
****************************************************/
typedef struct
{	

	U32 	biSize;			/*指定这个结构的长度，为40*/

	U32 	biWidth;		/*指定图象的宽度，单位是象素*/
	
	U32 	biHeight;		/*指定图象的高度，单位是象素*/
	
	U16 	biPlanes;		/*必须是1，不用考虑*/
	
	U16 	biBitCount;		/*指定表示颜色时要用到的位数，
							常用的值为1(黑白二色图),4(16色图),
							8(256色),16(16bit高彩色),24(24bit真彩色)，
							32(32bit增强型真彩色)*/
							
	U32 	biCompression;	/*指定位图是否压缩，有效的值为BI_RGB，
							BI_RLE8，BI_RLE4，BI_BITFIELDS(都是一些
							Windows定义好的常量)。要说明的是，
							Windows位图可以采用RLE4，和RLE8的压缩
							格式，但用的不多。我们今后所讨论
							的只有第一种不压缩的情况，即
							biCompression为BI_RGB的情况。*/
							
	U32 	biSizeImage;	/*表示图象的大小，以字节为单位。当用
							BI_RGB格式时，可设置为0。*/
							
	U32 	biXPelsPerMeter;/*表示水平分辨率，用象素/米表示*/
	
	U32 	biYPelsPerMeter;/*表示垂直分辨率，用象素/米表示*/
	
	U32 	biClrUsed;		/*表示位图实际使用的彩色表中的颜色索
							引数（设为0的话，则说明使用所有调色板项）*/
							
	U32 	biClrImportant;	/*表示对图象显示有重要影响的颜色索引的数目，如
	
							果是0，表示都重*/
}BitMapInfoHeader_t;
 


/*********************************************************
3、调色板Palette,当然，这里是对那些需要调色板
的位图文件而言的。有些位图，如真彩色图
是不需要调色板的，BITMAPINFOHEADER后直接是位
图数据。
**********************************************************/
typedef struct
{
	 U8 	rgbBlue;		//该颜色的蓝色分量
	 
	 U8 	rgbGreen;		//该颜色的绿色分量
	 
	 U8 	rgbRed;			//该颜色的红色分量
	 
	 U8 	rgbReserved;	//保留值 
} RGBQUAD_t;




/*********************************************************
位图数据
**********************************************************/
typedef struct 
{
	char	mapname[64];	//图片名称	
	U8		mapValid;		//指明图像缓存数据是否有效
	U32 	mapWidth;		//图片宽度，单位像素
	U32 	mapHeight;		//图片高度，单位像素
	U16 	mapBitCount;	//表示图片颜色的位数，真彩24/32
	U32 	mapSizeImage;	//图片大小。单位字节，包括补0的字节
	U8		*mapdata;		//图像数据缓存区
}BitMapData_t;
 

typedef struct

{
	 BitMapInfoHeader_t bmiHeader;
	 RGBQUAD_t 			bmiColors[1];
} BitMapInfo_t;

 

 

typedef struct

{
	BitMapFileHeader_t 	bfHeader;
	BitMapInfo_t 		biInfo;
}BitMapFile_t;

 

#pragma pack(pop)



typedef struct imgstruct
{
	FILE 			*IMGfp;
	char 			filename[64];
	uint16_t 		bits;
	uint32_t 		bmpwidth;
	uint32_t 		bmpheight;
	uint32_t 		bmpSize;
	

	uint32_t 		cx;			//解码后存放的坐标
	uint32_t 		cy;			//告诉解码器将解码出来的数据放在什么样的位置

	uint32_t 		chwidth;	//解码的缓存尺寸，告诉解码器缓存的大小
	uint32_t 		chheight;

	uint32_t 		ctwidth;
	uint32_t 		ctheight;

	uint8_t 		*cache;
	uint8_t 		*cur_frame;
	uint8_t 		*pre_frame;
	void 			*arg;
	//PCallback_t     writeAddr;	//回调函数，获取要写的缓存的地址

	
	int 			(*IMG_init)(struct imgstruct *);
	int 			(*IMG_decoder)(struct imgstruct *);
	

}IMGstruct_t;




#define BYTE_RGB_B		0
#define BYTE_RGB_G		1
#define BYTE_RGB_R		2
#define BYTE_RGB_A		3


//仅SDL模拟器使用，默认设置成640*480,先改成其他值
#define VIDEO_SCREEN_WIDTH    800
#define VIDEO_SCREEN_HEIGHT   600


void bitmap_fileheader_printf(BitMapFileHeader_t *bmfheader);
void bitmap_infoheader_printf(BitMapInfoHeader_t *bmiheader);


int IMG_INITstruct(IMGstruct_t *IMGstruct,uint16_t cx,uint16_t cy,uint16_t c_width,uint16_t c_height,uint8_t *cache);

int GetImageSize(char *imgname,int *imgWidth,int *imgHeight);










#if 0
typedef struct
{
	 U16 	bfType;			//指定文件类型，必须是0x424D，即字符串“BM”
	 U32 	bfSize;			//指定文件大小，包括这14个字节。
	 U16 	bfReserved1;	//为保留字，不用考虑
	 U16 	bfReserved2;	//为保留字，不用考虑
	 U32 	bfOffBits;		//从文件头到实际的位图数据的偏移字节数，即图中前三个部分的长度之和
} BitMapFileHeader_t;
#endif


#pragma pack(2)
typedef struct
{
	 U16 	bfType;			//指定文件类型，必须是0x424D，即字符串“BM”
	 U32 	bfSize;			//指定文件大小，包括这14个字节。
	 U16 	bfReserved1;	//为保留字，不用考虑
	 U16 	bfReserved2;	//为保留字，不用考虑
	 U32 	bfOffBits;		//从文件头到实际的位图数据的偏移字节数，即图中前三个部分的长度之和
} BMPHeader;
#pragma pack()


#pragma pack(2)
typedef struct
{
	uint32_t biSize;
	uint32_t biWidth;
	uint32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	uint32_t biXpelsPerMeter;
	uint32_t biYpelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
}BMPInfoHeader;
#pragma pack()

typedef struct
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char rserved;
}RGBQuad;








#endif

