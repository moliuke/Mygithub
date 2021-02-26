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
	BYTE BmpBuffer[2560];  // һ�� 768
	uint32_t WritePos_y = 0,WritePos_x = 0;
	uint32_t lHeight;  // ͼƬ�߶�
	uint32_t lWidth;   // ͼƬ���

	int nTmp,nTmp1;
	int nRealBytes;
	debug_printf("=IMGstruct->filename = %s\n",IMGstruct->filename);
	DEBUG_PRINTF;
	// ����ļ���
	//char strImageFile[30]="/home/LEDscr/image/000.bmp";
	//strcat(strImageFile,strImage);
	//strcat(strImageFile,".bmp");
	if((fp = fopen(IMGstruct->filename, "r+")) == NULL)
	{
	    //XKLcdDrawString("û��ָ��ͼ���ļ�");
//		XKErrorCode = ENOIMAGE;
		DEBUG_PRINTF;
		return -1;
	}
	//debug_printf("%d,%d\n",sizeof(BMPHeader),sizeof(BitMapFileHeader_t));
	// ��ȡͼƬ����	
	memset(&header,0,sizeof(BMPHeader));
	memset(&info_header,0,sizeof(BMPInfoHeader));
	fread(&header,sizeof(BMPHeader),1,fp);
	fread(&info_header,sizeof(BMPInfoHeader),1,fp);
	//debug_printf("",header.);
	// �ж��Ƿ���bmp�ļ�
	//debug_printf("info_header.biWidth = %d,info_header.biHeight = %d\n",info_header.biWidth,info_header.biHeight);
	debug_printf("header.bfType = 0x%x,header.bfSize = 0x%x,sizeof(BMPHeader) = %d\n",header.bfType,header.bfSize,sizeof(header));
	if(header.bfType!=0x4d42)  
	{
	    //XKLcdDrawString("ͼƬ��ʽ����");
		return -1;
	}
	// �ж�ͼƬ�Ƿ�ѹ����
	if(info_header.biCompression!=0)
	{
		debug_printf("info_header.biCompression = %d\n",info_header.biCompression);
	    //XKLcdDrawString("ϵͳ��֧��ѹ����ʽ");
		return -1;
	}
	// �߶�
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

	//ʵ��ͼ����ÿ��ռ�õ��ֽ���������Ҫ�������4�ı���
	uint32_t bmppicth = (((IMGstruct->bmpwidth)*(IMGstruct->bits) + 31) >> 5) << 2; 

	//����ͼ���п��ܴ�����Ļ��Ȼ��߸߶ȣ���ô����Ҫ��ƫ�Ƶ��ʵ���λ���ڶ�ȡ�ļ���bmpͼ���ǵ��õ�
	//ƫ�Ƶ�bmpoffset�ٶ�ȡ������ͼ��β����ǰƫ��bmpoffset�ٶ�ȡ����(���Գ�����Ļ�ĸ߶�)���Ա�֤ͼƬ��ǰ��Ĳ�����������ʾ
	uint32_t bmpoffset = (lHeight - IMGstruct->ctheight) * bmppicth;
	
	int ii = 0,jj = 0;
	
	// ��ȡ��������
	switch(info_header.biBitCount)
	{

		//��ɫͼ��2����ɫ:�ںͰס�һ������ֻ��1bit��ʾ�����Դ�ͼƬ��������������ÿһ���ֽڿ��Ա�ʾ
		//8�����ص����ɫ����ֵ
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

		//4λͼ��16����ɫ����4bit��ʾ������ͼƬ�����������������ݶ���һ��һ���ֽڵģ�����һ���ֽ�
		//�ͱ�ʾ�������ص㣬��4λ��ʾǰһ�����ص㣬��4λ��ʾ��һ�����ص㣬���ǵ�ֵ�ͱ�ʾ�ڵ�ɫ����
		//����ɫ����ֵ
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
					//4λ���bmp��ÿ�����ص�ռ��4bit�����Գ��������һ���ֽڷֳɸ�4λ����4λ��ʾ�������ص�����
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

		//8λͼ�����ڻ��ߵ���8λ��ͼƬ����Ҫ�õ���ɫ�壬8λͼ��256����ɫ����һ���ֽڱ�ʾһ����ɫ�����ֽھ���
		//�ڵ�ɫ���е���ɫ����ֵ
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

		//16λͼ���������ֽڱ�ʾһ�����ص㣬16λͼ��rgb��������555��565֮�֣�����ֻ����555�������555����������ֽڵ�˳��
		//���ֽ���ǰ���ֽ��ں󣬶��Ҹ��ֽڵ����λΪ0�������ֽںϲ���һ��16λ���ݺ��rgb���ݵķ���˳����r�����ֽںϲ���һ��
		//16λ���ݺ��rgb���ݵķ���˳����r��g��b
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

		//24λ���ɫ��ͼƬ����������BGR��˳������
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

		//32λ��24λһ����ֻ��32λ��24λ����һ����ʾ͸�����ֽڣ����ﲻ�������ֽ�
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
	
	//��ȡͼƬ��ͷ����Ϣ
	imgfp = fopen(imgname,"r+");
	if(imgfp == NULL)
	{
		return -1;
	}
	fread(imgMsg,1,26,imgfp);
	fclose(imgfp);
	
	//����ͷ����Ϣ�õ�ͼƬ�������ȸ��߶�
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

 








