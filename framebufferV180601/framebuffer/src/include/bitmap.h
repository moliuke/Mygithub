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
ͼƬ��ÿһ�е��ֽ���������4�ı�����
�ڴ洢ͼƬʱ��ͼƬ�������Ѿ��Զ���0
�������ڽ���ʱÿ�����ݱ��밴��4 �ı���
����ȡͼƬ����
*******************************************************/

/***************************************************
λͼ�ļ���Ҫ��Ϊ����4�����֣�

������ ��			��ӦWindows�ṹ�嶨��    ��С��byte�� 

 �ļ���Ϣͷ 		BITMAPFILEHEADER����������14 

λͼ��Ϣͷ  		BITMAPINFOHEADER��������     40 

��ɫ����ɫ�壩RGBQUAD����������������ѡ�� 

 λͼ���ݣ�RGB��ɫ���У� ����������BYTE* ��ͼ�񳤿�ߴ���� 

****************************************************/

 
/***************************************************
1 ��λͼ�ļ�ͷ
****************************************************/
typedef struct
{
	 U16 	bfType;			//ָ���ļ����ͣ�������0x424D�����ַ�����BM��
	 U32 	bfSize;			//ָ���ļ���С��������14���ֽڡ�
	 U16 	bfReserved1;	//Ϊ�����֣����ÿ���
	 U16 	bfReserved2;	//Ϊ�����֣����ÿ���
	 U32 	bfOffBits;		//���ļ�ͷ��ʵ�ʵ�λͼ���ݵ�ƫ���ֽ�������ͼ��ǰ�������ֵĳ���֮��
} BitMapFileHeader_t;


/***************************************************
2��λͼ��Ϣͷ
****************************************************/
typedef struct
{	

	U32 	biSize;			/*ָ������ṹ�ĳ��ȣ�Ϊ40*/

	U32 	biWidth;		/*ָ��ͼ��Ŀ�ȣ���λ������*/
	
	U32 	biHeight;		/*ָ��ͼ��ĸ߶ȣ���λ������*/
	
	U16 	biPlanes;		/*������1�����ÿ���*/
	
	U16 	biBitCount;		/*ָ����ʾ��ɫʱҪ�õ���λ����
							���õ�ֵΪ1(�ڰ׶�ɫͼ),4(16ɫͼ),
							8(256ɫ),16(16bit�߲�ɫ),24(24bit���ɫ)��
							32(32bit��ǿ�����ɫ)*/
							
	U32 	biCompression;	/*ָ��λͼ�Ƿ�ѹ������Ч��ֵΪBI_RGB��
							BI_RLE8��BI_RLE4��BI_BITFIELDS(����һЩ
							Windows����õĳ���)��Ҫ˵�����ǣ�
							Windowsλͼ���Բ���RLE4����RLE8��ѹ��
							��ʽ�����õĲ��ࡣ���ǽ��������
							��ֻ�е�һ�ֲ�ѹ�����������
							biCompressionΪBI_RGB�������*/
							
	U32 	biSizeImage;	/*��ʾͼ��Ĵ�С�����ֽ�Ϊ��λ������
							BI_RGB��ʽʱ��������Ϊ0��*/
							
	U32 	biXPelsPerMeter;/*��ʾˮƽ�ֱ��ʣ�������/�ױ�ʾ*/
	
	U32 	biYPelsPerMeter;/*��ʾ��ֱ�ֱ��ʣ�������/�ױ�ʾ*/
	
	U32 	biClrUsed;		/*��ʾλͼʵ��ʹ�õĲ�ɫ���е���ɫ��
							��������Ϊ0�Ļ�����˵��ʹ�����е�ɫ���*/
							
	U32 	biClrImportant;	/*��ʾ��ͼ����ʾ����ҪӰ�����ɫ��������Ŀ����
	
							����0����ʾ����*/
}BitMapInfoHeader_t;
 


/*********************************************************
3����ɫ��Palette,��Ȼ�������Ƕ���Щ��Ҫ��ɫ��
��λͼ�ļ����Եġ���Щλͼ�������ɫͼ
�ǲ���Ҫ��ɫ��ģ�BITMAPINFOHEADER��ֱ����λ
ͼ���ݡ�
**********************************************************/
typedef struct
{
	 U8 	rgbBlue;		//����ɫ����ɫ����
	 
	 U8 	rgbGreen;		//����ɫ����ɫ����
	 
	 U8 	rgbRed;			//����ɫ�ĺ�ɫ����
	 
	 U8 	rgbReserved;	//����ֵ 
} RGBQUAD_t;




/*********************************************************
λͼ����
**********************************************************/
typedef struct 
{
	char	mapname[64];	//ͼƬ����	
	U8		mapValid;		//ָ��ͼ�񻺴������Ƿ���Ч
	U32 	mapWidth;		//ͼƬ��ȣ���λ����
	U32 	mapHeight;		//ͼƬ�߶ȣ���λ����
	U16 	mapBitCount;	//��ʾͼƬ��ɫ��λ�������24/32
	U32 	mapSizeImage;	//ͼƬ��С����λ�ֽڣ�������0���ֽ�
	U8		*mapdata;		//ͼ�����ݻ�����
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
	

	uint32_t 		cx;			//������ŵ�����
	uint32_t 		cy;			//���߽�������������������ݷ���ʲô����λ��

	uint32_t 		chwidth;	//����Ļ���ߴ磬���߽���������Ĵ�С
	uint32_t 		chheight;

	uint32_t 		ctwidth;
	uint32_t 		ctheight;

	uint8_t 		*cache;
	uint8_t 		*cur_frame;
	uint8_t 		*pre_frame;
	void 			*arg;
	//PCallback_t     writeAddr;	//�ص���������ȡҪд�Ļ���ĵ�ַ

	
	int 			(*IMG_init)(struct imgstruct *);
	int 			(*IMG_decoder)(struct imgstruct *);
	

}IMGstruct_t;




#define BYTE_RGB_B		0
#define BYTE_RGB_G		1
#define BYTE_RGB_R		2
#define BYTE_RGB_A		3


//��SDLģ����ʹ�ã�Ĭ�����ó�640*480,�ȸĳ�����ֵ
#define VIDEO_SCREEN_WIDTH    800
#define VIDEO_SCREEN_HEIGHT   600


void bitmap_fileheader_printf(BitMapFileHeader_t *bmfheader);
void bitmap_infoheader_printf(BitMapInfoHeader_t *bmiheader);


int IMG_INITstruct(IMGstruct_t *IMGstruct,uint16_t cx,uint16_t cy,uint16_t c_width,uint16_t c_height,uint8_t *cache);

int GetImageSize(char *imgname,int *imgWidth,int *imgHeight);










#if 0
typedef struct
{
	 U16 	bfType;			//ָ���ļ����ͣ�������0x424D�����ַ�����BM��
	 U32 	bfSize;			//ָ���ļ���С��������14���ֽڡ�
	 U16 	bfReserved1;	//Ϊ�����֣����ÿ���
	 U16 	bfReserved2;	//Ϊ�����֣����ÿ���
	 U32 	bfOffBits;		//���ļ�ͷ��ʵ�ʵ�λͼ���ݵ�ƫ���ֽ�������ͼ��ǰ�������ֵĳ���֮��
} BitMapFileHeader_t;
#endif


#pragma pack(2)
typedef struct
{
	 U16 	bfType;			//ָ���ļ����ͣ�������0x424D�����ַ�����BM��
	 U32 	bfSize;			//ָ���ļ���С��������14���ֽڡ�
	 U16 	bfReserved1;	//Ϊ�����֣����ÿ���
	 U16 	bfReserved2;	//Ϊ�����֣����ÿ���
	 U32 	bfOffBits;		//���ļ�ͷ��ʵ�ʵ�λͼ���ݵ�ƫ���ֽ�������ͼ��ǰ�������ֵĳ���֮��
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

