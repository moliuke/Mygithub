#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bitmap.h"
#include "debug.h"
#include "display.h"


BMPHeader header;
BMPInfoHeader info_header;
RGBQuad rgb_quad[256];

BitMapFile_t bmpfile;



void bitmap_fileheader_printf(BitMapFileHeader_t *bmfheader)
{
	debug_printf(
		"\n\n============================file_header=======================\n"
		"bfType:            0x%x\n"
		"bfSize:            %d\n"
		"bfOffBits:         %d\n"
		"===================================================================\n\n",
		bmfheader->bfType,
		bmfheader->bfSize,
		bmfheader->bfOffBits);
}


void bitmap_infoheader_printf(BitMapInfoHeader_t *bmiheader)
{
	debug_printf(
		"\n\n=============================info_header======================\n"
		"biSize:            %d\n"
		"biWidth:           %d\n"
		"biHeight:          %d\n"
		"biPlanes:          %d\n"
		"biBitCount:        %d\n"
		"biCompression:     %d\n"
		"biSizeImage:       %d\n"
		"biXPelsPerMeter:   %d\n"
		"biYPelsPerMeter:   %d\n"
		"biClrUsed:         %d\n"
		"biClrImportant:    %d\n"
		"==================================================================\n\n",
		bmiheader->biSize,
		bmiheader->biWidth,
		bmiheader->biHeight,
		bmiheader->biPlanes,
		bmiheader->biBitCount,
		bmiheader->biCompression,
		bmiheader->biSizeImage,
		bmiheader->biXPelsPerMeter,
		bmiheader->biYPelsPerMeter,
		bmiheader->biClrUsed,
		bmiheader->biClrImportant);
}

int IMG_decoder(IMGstruct_t *IMGstruct) 
{
	FILE *fp;
	BYTE BmpBuffer[2560];  // 一行 768
	uint32_t WritePos_y = 0,WritePos_x = 0;
	uint32_t lHeight;  // 图片高度
	uint32_t lWidth;   // 图片宽度

	int nTmp,nTmp1;
	int nRealBytes;
	debug_printf("=IMGstruct->filename = %s\n",IMGstruct->filename);
	DEBUG_PRINTF;
	// 获得文件名
	//char strImageFile[30]="/home/LEDscr/image/000.bmp";
	//strcat(strImageFile,strImage);
	//strcat(strImageFile,".bmp");
	if((fp = fopen(IMGstruct->filename, "r+")) == NULL)
	{
	    //XKLcdDrawString("没有指定图形文件");
//		XKErrorCode = ENOIMAGE;
		DEBUG_PRINTF;
		return -1;
	}
	//debug_printf("%d,%d\n",sizeof(BMPHeader),sizeof(BitMapFileHeader_t));
	// 读取图片参数	
	memset(&header,0,sizeof(BMPHeader));
	memset(&info_header,0,sizeof(BMPInfoHeader));
	fread(&header,sizeof(BMPHeader),1,fp);
	fread(&info_header,sizeof(BMPInfoHeader),1,fp);
	//debug_printf("",header.);
	// 判断是否是bmp文件
	//debug_printf("info_header.biWidth = %d,info_header.biHeight = %d\n",info_header.biWidth,info_header.biHeight);
	debug_printf("header.bfType = 0x%x,header.bfSize = 0x%x,sizeof(BMPHeader) = %d\n",header.bfType,header.bfSize,sizeof(header));
	if(header.bfType!=0x4d42)  
	{
	    //XKLcdDrawString("图片格式错误");
		return -1;
	}
	// 判断图片是否压缩的
	if(info_header.biCompression!=0)
	{
		debug_printf("info_header.biCompression = %d\n",info_header.biCompression);
	    //XKLcdDrawString("系统不支持压缩格式");
		return -1;
	}
	// 高度
	lWidth  = info_header.biWidth;
	lHeight = info_header.biHeight;
	debug_printf("lWidth = %d,lHeight = %d\n",lWidth,lHeight);


	IMGstruct->bits 	   = info_header.biBitCount; 
	IMGstruct->bmpwidth    = info_header.biWidth; 
	IMGstruct->bmpheight   = info_header.biHeight;
	IMGstruct->bmpSize	   = info_header.biSizeImage;

	IMGstruct->ctwidth	= (IMGstruct->bmpwidth + IMGstruct->cx < IMGstruct->chwidth) ? 
					   IMGstruct->bmpwidth : (IMGstruct->chwidth - IMGstruct->cx); 
	IMGstruct->ctheight = (IMGstruct->bmpheight + IMGstruct->cy < IMGstruct->chheight) ?
					   IMGstruct->bmpheight : (IMGstruct->chheight - IMGstruct->cy);

	//实际图像中每行占用的字节数，这里要求必须是4的倍数
	uint32_t bmppicth = (((IMGstruct->bmpwidth)*(IMGstruct->bits) + 31) >> 5) << 2; 

	//由于图像有可能大于屏幕宽度或者高度，那么就需要将偏移到适当的位置在读取文件，bmp图像是倒置的
	//偏移到bmpoffset再读取，即从图像尾部往前偏移bmpoffset再读取数据(忽略超出屏幕的高度)，以保证图片的前面的部分能正常显示
	uint32_t bmpoffset = (lHeight - IMGstruct->ctheight) * bmppicth;
	
	int ii = 0,jj = 0;
	
	// 读取点阵数据
	switch(info_header.biBitCount)
	{

		//单色图，2中颜色:黑和白。一个像素只用1bit表示，所以从图片数据区读出来的每一个字节可以表示
		//8个像素点的颜色索引值
		case 1:
			for (ii = 0 ; ii < 2 ; ii++)
			{
				fread(&rgb_quad[ii],sizeof(RGBQuad),1,fp);
			}
			fseek(fp,header.bfOffBits + bmpoffset,SEEK_SET);

			for(jj = IMGstruct->ctheight-1;jj >= 0;jj--)
			{
				WritePos_y = (jj + IMGstruct->cy) * IMGstruct->chwidth * SCREEN_BPP;
				fread(BmpBuffer,bmppicth,1,fp);
				for(ii = 0; ii < IMGstruct->ctwidth ; ii++)
				{
					WritePos_x = WritePos_y + (ii+IMGstruct->cx) * SCREEN_BPP;
					uint8_t quad = (BmpBuffer[ii / 8] >> (7 - ii % 8)) & 0x01;
					IMGstruct->cache[WritePos_x + BGR_B] = rgb_quad[quad].blue;
					IMGstruct->cache[WritePos_x + BGR_G] = rgb_quad[quad].green;
					IMGstruct->cache[WritePos_x + BGR_R] = rgb_quad[quad].red;
				}
			}
			break;

		//4位图有16中颜色，用4bit表示，而从图片数据区读出来的数据都是一个一个字节的，所以一个字节
		//就表示两个像素点，高4位表示前一个像素点，低4位表示后一个像素点，他们的值就表示在调色板中
		//的颜色索引值
		case 4:
			for (ii = 0 ; ii < 16 ; ii++)
			{
				fread(&rgb_quad[ii],sizeof(RGBQuad),1,fp);
			}
			
			fseek(fp,header.bfOffBits + bmpoffset,SEEK_SET);
			debug_printf("lHeight - IMGstruct->ctheight = %d\n",lHeight - IMGstruct->ctheight);
			for(jj = IMGstruct->ctheight-1;jj >= 0;jj--)
			{
				
				WritePos_y = (jj + IMGstruct->cy) * IMGstruct->chwidth * SCREEN_BPP;
				fread(BmpBuffer,bmppicth,1,fp);
				for(ii = 0; ii < IMGstruct->ctwidth ; ii++)
				{
					uint8_t H4bit = (BmpBuffer[ii/2]>>4)&0x0f;
					uint8_t L4bit = (BmpBuffer[ii/2])&0x0f;
					//4位深的bmp即每个像素点占据4bit，所以出现下面的一个字节分成高4位跟低4位表示两个像素点的情况
					WritePos_x = WritePos_y + (ii+IMGstruct->cx) * SCREEN_BPP;
					if(ii % 2 == 0)
					{
						IMGstruct->cache[WritePos_x + BGR_B] = rgb_quad[H4bit].blue;
						IMGstruct->cache[WritePos_x + BGR_G] = rgb_quad[H4bit].green;
						IMGstruct->cache[WritePos_x + BGR_R] = rgb_quad[H4bit].red;
					}
					else
					{
						IMGstruct->cache[WritePos_x + BGR_B] = rgb_quad[L4bit].blue;
						IMGstruct->cache[WritePos_x + BGR_G] = rgb_quad[L4bit].green;
						IMGstruct->cache[WritePos_x + BGR_R] = rgb_quad[L4bit].red;
					}
				}
			}
			break;

		//8位图，低于或者等于8位的图片都需要用到调色板，8位图有256个颜色，用一个字节表示一个颜色，该字节就是
		//在调色板中的颜色索引值
		case 8:
			DEBUG_PRINTF;
			for (ii = 0 ; ii < 256 ; ii++)
			{
				fread(&rgb_quad[ii],sizeof(RGBQuad),1,fp);
			}
			fseek(fp,header.bfOffBits + bmpoffset,SEEK_SET);
			
			for(jj = IMGstruct->ctheight-1;jj >= 0;jj--)
			{
				WritePos_y = (jj + IMGstruct->cy) * IMGstruct->chwidth * SCREEN_BPP;
				fread(BmpBuffer,bmppicth,1,fp);
				for(ii = 0; ii < IMGstruct->ctwidth ; ii++)
				{
					WritePos_x = WritePos_y + (ii+IMGstruct->cx) * SCREEN_BPP;
					IMGstruct->cache[WritePos_x + BGR_B] = rgb_quad[BmpBuffer[ii]].blue;
					IMGstruct->cache[WritePos_x + BGR_G] = rgb_quad[BmpBuffer[ii]].green;
					IMGstruct->cache[WritePos_x + BGR_R] = rgb_quad[BmpBuffer[ii]].red;
					
				}
			}
			break;

		//16位图，用两个字节表示一个像素点，16位图在rgb分量上有555跟565之分，这里只讨论555的情况，555的情况两个字节的顺序
		//高字节在前低字节在后，而且高字节的最高位为0，两个字节合并成一个16位数据后对rgb数据的分量顺序是r两个字节合并成一个
		//16位数据后对rgb数据的分量顺序是r、g、b
		case 16:
			fseek(fp,header.bfOffBits + bmpoffset,SEEK_SET);
			for(jj = IMGstruct->ctheight-1;jj >= 0;jj--)
			{
				WritePos_y = (jj + IMGstruct->cy) * IMGstruct->chwidth * SCREEN_BPP;
				fread(BmpBuffer,bmppicth,1,fp);
				for(ii = 0; ii < IMGstruct->ctwidth ; ii++)
				{
					WritePos_x = WritePos_y + (ii+IMGstruct->cx) * SCREEN_BPP;
					IMGstruct->cache[WritePos_x + BGR_B] = (BmpBuffer[2*ii + 0] & 0x1f) << 3;
					IMGstruct->cache[WritePos_x + BGR_G] = ((BmpBuffer[2*ii + 1] << 6) & 0xc0) | BmpBuffer[2*ii + 0] & 0xe0 >> 2;
					IMGstruct->cache[WritePos_x + BGR_R] = (BmpBuffer[2*ii + 1] << 1) & 0xf8;
				}
			}
			break;

		//24位真彩色，图片数据区按照BGR的顺序排列
		case 24:
			DEBUG_PRINTF;
			fseek(fp,header.bfOffBits + bmpoffset,SEEK_SET);
			debug_printf("IMGstruct->cx = %d,IMGstruct->cy = %d\n",IMGstruct->cx,IMGstruct->cy);
			for(jj = IMGstruct->ctheight-1;jj >= 0;jj--)
			{
				WritePos_y = (jj + IMGstruct->cy) * IMGstruct->chwidth * SCREEN_BPP;
				fread(BmpBuffer,bmppicth,1,fp);
				for(ii = 0; ii < IMGstruct->ctwidth ; ii++)
				{
					WritePos_x = WritePos_y + (ii+IMGstruct->cx) * SCREEN_BPP;
					IMGstruct->cache[WritePos_x + BGR_B] = BmpBuffer[3*ii+BGR_B];//pEachLinBuf[w*BytePerPix+0]; 
					IMGstruct->cache[WritePos_x + BGR_G] = BmpBuffer[3*ii+BGR_G];//pEachLinBuf[w*BytePerPix+1]; 
					IMGstruct->cache[WritePos_x + BGR_R] = BmpBuffer[3*ii+BGR_R];//pEachLinBuf[w*BytePerPix+2]; 
				}
			}
			DEBUG_PRINTF;
			break;

		//32位跟24位一样，只是32位比24位多了一个表示透明的字节，这里不填充这个字节
		case 32:
			DEBUG_PRINTF;
			fseek(fp,header.bfOffBits + bmpoffset,SEEK_SET);
			for(jj = IMGstruct->ctheight-1;jj >= 0;jj--)
			{
				WritePos_y = (jj + IMGstruct->cy) * IMGstruct->chwidth * SCREEN_BPP;
				fread(BmpBuffer,bmppicth,1,fp);
				for(ii = 0; ii < IMGstruct->ctwidth ; ii++)
				{
					WritePos_x = WritePos_y + (ii+IMGstruct->cx) * SCREEN_BPP;
					IMGstruct->cache[WritePos_x + BGR_B] = BmpBuffer[4*ii+BGR_B];//pEachLinBuf[w*BytePerPix+0]; 
					IMGstruct->cache[WritePos_x + BGR_G] = BmpBuffer[4*ii+BGR_G];//pEachLinBuf[w*BytePerPix+1]; 
					IMGstruct->cache[WritePos_x + BGR_R] = BmpBuffer[4*ii+BGR_R];//pEachLinBuf[w*BytePerPix+2]; 
				}
			}
			break;
	}
	
	fclose(fp);

	return 0;
}




int GetImageSize(char *imgname,int *imgWidth,int *imgHeight)
{
	FILE *imgfp = NULL;
	char imgMsg[28];

	if(access(imgname,F_OK) < 0)
		return -1;
	
	//读取图片的头部信息
	imgfp = fopen(imgname,"r+");
	if(imgfp == NULL)
	{
		return -1;
	}
	fread(imgMsg,1,26,imgfp);
	fclose(imgfp);
	
	//根据头部信息得到图片的整体宽度跟高度
	*imgWidth = (uint8_t)imgMsg[21] << 24 | (uint8_t)imgMsg[20] << 16 | (uint8_t)imgMsg[19] << 8 | (uint8_t)imgMsg[18];
	*imgHeight = (uint8_t)imgMsg[25] << 24 | (uint8_t)imgMsg[24] << 16 | (uint8_t)imgMsg[23] << 8 | (uint8_t)imgMsg[22];
	
	return 0;
}



	
int IMG_INITstruct(IMGstruct_t *IMGstruct,uint16_t cx,uint16_t cy,uint16_t c_width,uint16_t c_height,uint8_t *cache)
{
	IMGstruct->cx = cx;
	IMGstruct->cy = cy;
	IMGstruct->chwidth = c_width;
	IMGstruct->chheight= c_height;

	IMGstruct->cache = cache;
	
	IMGstruct->IMG_decoder = IMG_decoder;


	return 0;
}

 








