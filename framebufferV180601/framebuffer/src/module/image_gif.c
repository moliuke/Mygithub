#include <stdio.h>

#include "./gif_lib.h"
#include "../include/config.h"
#include "../include/debug.h"
#include "../include/Dev_framebuffer.h"
#include "../include/display.h"


#include "image_gif.h"


//it meas this block is an extension block
#define GIF_CONTROL_EXT_CODE 0xf9
#define GIF_CONTROL_EXT_SIZE 0x04


static GifFileType *GifFile = NULL;
static GifRowType *ScreenBuffer = NULL;
static ColorMapObject *ColorMap = NULL;
static int trans_color = -1;

void gif_printfErrCode(int errcode)
{
	debug_printf("GIF ERROR: %s\n",GifErrorString(errcode));
}



static int GIF_RGBA_decode(GIFstruct_t *GIFstruct)  
{  
	int h,w;
	GifRowType GifRow;  
	static GifColorType *ColorMapEntry;  
	uint8_t *buffer_cur = NULL,*buffer_pre;	

	int Swidth,SHeight;
	uint8_t *writeAddr = NULL;
	uint32_t ch_line_bytes = GIFstruct->chwidth * 4;
	uint32_t pos_h,pos_x,pos_pixels;

	Swidth  = GIFstruct->GifFile->SWidth;
	SHeight = GIFstruct->GifFile->SHeight;

	if((GIFstruct->cx + Swidth > GIFstruct->chwidth) || (GIFstruct->cy + SHeight > GIFstruct->chheight))
	{
		GIFstruct->ctwidth = GIFstruct->chwidth - GIFstruct->cx;
		GIFstruct->ctheight = GIFstruct->chheight - GIFstruct->cy;
	}
	else
	{
		GIFstruct->ctwidth = Swidth;
		GIFstruct->ctheight = SHeight;
	}

	GIFstruct->ctwidth  = (Swidth < GIFstruct->chwidth) ? Swidth : (GIFstruct->chwidth - GIFstruct->cx);
	GIFstruct->ctheight = (SHeight < GIFstruct->chheight) ? SHeight : (GIFstruct->chheight - GIFstruct->cy);

	debug_printf("Swidth = %d,SHeight = %d,GIFstruct->chwidth = %d,GIFstruct->chheight = %d,GIFstruct->cx = %d,GIFstruct->cy = %d\n",
		Swidth,SHeight,GIFstruct->chwidth,GIFstruct->chheight,GIFstruct->cx,GIFstruct->cy);
	debug_printf("GIFstruct->ctwidth = %d,GIFstruct->ctheight = %d\n",GIFstruct->ctwidth,GIFstruct->ctheight);
#if 1
	while((writeAddr = GIFstruct->writeAddr(GIFstruct->arg)) == NULL)
	{
		usleep(1000);
		continue;
	}
#else
	//while(writeAddr = )
#endif


	
	GIFstruct->cur_frame = writeAddr;
	for (h = 0; h < GIFstruct->ctheight; h++)
	{  
		GifRow = GIFstruct->ScreenBuffer[h]; 
		pos_h = (h + GIFstruct->cy) * ch_line_bytes;
		
		for (w = 0; w < GIFstruct->ctwidth; w++) 
		{  		
			pos_pixels = pos_h + (w+GIFstruct->cx) * 4;
			if( GIFstruct->trans_color != -1 && GIFstruct->trans_color == GifRow[w] )
			{  
				GIFstruct->cur_frame[pos_pixels + 0] = GIFstruct->pre_frame[pos_pixels + 0];
				GIFstruct->cur_frame[pos_pixels + 1] = GIFstruct->pre_frame[pos_pixels + 1];
				GIFstruct->cur_frame[pos_pixels + 2] = GIFstruct->pre_frame[pos_pixels + 2];
				GIFstruct->cur_frame[pos_pixels + 3] = GIFstruct->pre_frame[pos_pixels + 3];
				continue;  
			}
			//debug_printf("GIFstruct->ctwidth = %d\n",GIFstruct->ctwidth);
			ColorMapEntry = &GIFstruct->ColorMap->Colors[GifRow[w]];  
			GIFstruct->cur_frame[pos_pixels + 0] = ColorMapEntry->Blue;
			GIFstruct->cur_frame[pos_pixels + 1] = ColorMapEntry->Green;
			GIFstruct->cur_frame[pos_pixels + 2] = ColorMapEntry->Red;
			GIFstruct->cur_frame[pos_pixels + 3] = 0xff;
		} 
	}
	GIFstruct->pre_frame = GIFstruct->cur_frame;
	return 0;
}  



static int GIF_open(GIFstruct_t *GIFstruct) 
{
	int gif_error = -1;
	debug_printf("GIFstruct->filename = %s\n",GIFstruct->filename);
	GIFstruct->GifFile = DGifOpenFileName(GIFstruct->filename,&gif_error);
	if(GIFstruct->GifFile == NULL)
	{
		debug_printf("GIF ERROR : %s\n",GifErrorString(gif_error));
		return GIFerr_GIF_FAIL;
	}
	GIFstruct->Fwidth  = GIFstruct->GifFile->SWidth;
	GIFstruct->Fheight = GIFstruct->GifFile->SHeight;
	
	return GIFerr_GIF_OK;
}

GIFerror_t static GIF_close(GIFstruct_t *GIFstruct)
{
	int gif_err = -1;
	DGifCloseFile(GIFstruct->GifFile,&gif_err);
	//gif_printfErrCode(gif_err);
	return GIFerr_GIF_OK;
}


GIFerror_t static GIF_Sinit(GIFstruct_t *GIFstruct)
{
	int Size;
	int i;
	
	if(GIFstruct->GifFile == NULL)
		return GIFerr_GIF_FAIL;

	//GIFstruct->ScreenBuffer是个指针数组，gif图片有多少行就有多少个指针
	GIFstruct->ScreenBuffer = (GifRowType *)malloc(GIFstruct->GifFile->SHeight * sizeof(GifRowType *));

	//计算gif图片每行所需的字节数
	Size = GIFstruct->GifFile->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/  
	//给第一行分配内存，根据行总字节数来分配
	if ((GIFstruct->ScreenBuffer[0] = (GifRowType) malloc(Size)) == NULL) /* First row. */	
		debug_printf("Failed to allocate memory required, aborted."); 

	//对第一行数据全部设置成背景色
	for (i = 0; i < GIFstruct->GifFile->SWidth; i++)  /* Set its color to BackGround. */  
		GIFstruct->ScreenBuffer[0][i] = GIFstruct->GifFile->SBackGroundColor;	

	//对第二行以上所有行指针分配内存(注意:是一行一行的分配内存，不是整张图片)，并将行数初始化成跟第一行一样的背景色
	for (i = 1; i < GIFstruct->GifFile->SHeight; i++) 
	{  
		/* Allocate the other rows, and set their color to background too: */  
		if ((GIFstruct->ScreenBuffer[i] = (GifRowType) malloc(Size)) == NULL)  
			debug_printf("Failed to allocate memory required, aborted.");  

		memcpy(GIFstruct->ScreenBuffer[i], GIFstruct->ScreenBuffer[0], Size);  
	}

	return GIFerr_GIF_OK;
}



GIFerror_t static GIF_ResFree(GIFstruct_t *GIFstruct)
{
	uint16_t  i = 0;
	for(i = 0 ; i < GIFstruct->GifFile->SHeight ; i ++)
	{
		free(GIFstruct->ScreenBuffer[i]);
		GIFstruct->ScreenBuffer[i] = NULL;
	}
	free(GIFstruct->ScreenBuffer);
	GIFstruct->ScreenBuffer = NULL;
	GIF_close(GIFstruct);
}




static GIFerror_t GIF_init(GIFstruct_t *GIFstruct)
{
	int gif_error = -1;
	int Size;
	int i;

	debug_printf("GIFstruct->filename = %s\n",GIFstruct->filename);
	if(GIF_open(GIFstruct) < 0)
		return GIFerr_GIF_OK;

	return GIF_Sinit(GIFstruct);
}


static GIFerror_t GIF_getFrameInfo(GIFstruct_t *GIFstruct)
{
	int i = 0;
	GifWord Row,Col,Width,Height;
	do
	{
		if (DGifGetRecordType(GIFstruct->GifFile, &(GIFstruct->RecordType)) == GIF_ERROR)
		{  
			perror("#DGifGetRecordType");
			return GIFerr_GET_TYPE_FAIL;
		} 

		if(GIFstruct->RecordType == TERMINATE_RECORD_TYPE)
		{
			return GIFerr_TERMINATE;
		}

		switch(GIFstruct->RecordType)
		{
			//图像数据块
			case IMAGE_DESC_RECORD_TYPE:
				debug_printf("IMAGE_DESC_RECORD_TYPE============\n");
				if (DGifGetImageDesc(GIFstruct->GifFile) == GIF_ERROR) 
				{  
					return GIFerr_GET_FRAME_FAIL;	
				}  
				Row = GIFstruct->GifFile->Image.Top; /* Image Position relative to Screen. */  
				Col = GIFstruct->GifFile->Image.Left;  
				Width = GIFstruct->GifFile->Image.Width;  
				Height = GIFstruct->GifFile->Image.Height;	
				//rgb_buffer = (uint8_t *)malloc(GifFile->SWidth * GifFile->SHeight * 4);
				//debug_printf("GifFile->SWidth = %d,GifFile->SHeight = %d,GifFile->Image.Width = %d\n",GIFstruct->GifFile->SWidth,GIFstruct->GifFile->SHeight,GIFstruct->GifFile->Image.Width);
				//debug_printf("Col = %d,Row = %d,Width = %d,Height = %d\n",Col, Row, Width, Height); 
				
				if (GIFstruct->GifFile->Image.Left + GIFstruct->GifFile->Image.Width > GIFstruct->GifFile->SWidth ||  
				GIFstruct->GifFile->Image.Top + GIFstruct->GifFile->Image.Height > GIFstruct->GifFile->SHeight)
				{  
					//debug_printf("Image is not confined to screen dimension, aborted.\n");	
					return GIFerr_FRAME_OVERFLOW; 
				} 

				if (GIFstruct->GifFile->Image.Interlace) 
				{  
#if 0
					/* Need to perform 4 passes on the images: */  
					for (Count = i = 0; i < 4; i++)  
					for (j = Row + InterlacedOffset[i]; j < Row + Height; j += InterlacedJumps[i]) 
					{  
						//GifQprintf("\b\b\b\b%-4d", Count++);	
						if (DGifGetLine(GifFile, &ScreenBuffer[j][Col], Width) == GIF_ERROR) 
						{  
							//PrintGifError();	
							//exit(EXIT_FAILURE); 
							return -1;
						}  
					} 
#endif
				}  
				else 
				{  
					//获取gif的行数据，一行一行的保存在GIFstruct->ScreenBuffer中
					for (i = 0; i < Height; i++)
					{  
						if (DGifGetLine(GIFstruct->GifFile, &GIFstruct->ScreenBuffer[Row++][Col], Width) == GIF_ERROR) 
						{  
							return GIFerr_GET_FRAME_DATA_FAIL;
						} 
					}  
				}  
				
				/* Get the color map */  
				GIFstruct->ColorMap = (GIFstruct->GifFile->Image.ColorMap ? GIFstruct->GifFile->Image.ColorMap : GIFstruct->GifFile->SColorMap);  
				if (GIFstruct->ColorMap == NULL) 
				{  
					//debug_printf("Gif Image does not have a colormap\n");	
					return GIFerr_GET_NO_COLOR_MAP;
				}
				break;

			case EXTENSION_RECORD_TYPE:
				debug_printf("EXTENSION_RECORD_TYPE------------------\n");
				if(DGifGetExtension(GIFstruct->GifFile,&GIFstruct->Extcode,&GIFstruct->extension)== GIF_ERROR)
				{
					//debug_printf("get the extension code fail!\n");
					return GIFerr_GET_FRAME_EXTENT_FAIL;
				}
				
				if( GIFstruct->Extcode == GIF_CONTROL_EXT_CODE && GIFstruct->extension[0] == GIF_CONTROL_EXT_SIZE) 
				{  
					int delay = 0;
					delay = (GIFstruct->extension[3] << 8 | GIFstruct->extension[2]) * 10;	
					//debug_printf("delay = %d\n",delay);
					/* Can sleep here */ 
					//usleep(delay);
				}  
			
				/* handle transparent color */	
				if( (GIFstruct->extension[1] & 1) == 1 )
				{	
					//debug_printf("extension[1] = %d,extension[4] = %d\n",GIFstruct->extension[1],GIFstruct->extension[4]);
					GIFstruct->trans_color = GIFstruct->extension[4];  
				}  
				else
				{
					DEBUG_PRINTF;
					GIFstruct->trans_color = -1;
				}
#if 1
				while(GIFstruct->extension!=NULL)
				{
					usleep(10);
					if(DGifGetExtensionNext(GIFstruct->GifFile,&GIFstruct->extension) == GIF_ERROR)
					break;
				}
#endif
				break;

			default:
				break;
		}
	}while(GIFstruct->RecordType != IMAGE_DESC_RECORD_TYPE);
	if(GIFstruct->RecordType == IMAGE_DESC_RECORD_TYPE)
	{
		return GIFerr_GET_FRAME_OK;
	}
}



static GIFerror_t GIF_frameDecoder(GIFstruct_t *GIFstruct)
{
	int h,w;
	GifRowType GifRow;  
	static GifColorType *ColorMapEntry;  
	uint8_t *buffer_cur = NULL,*buffer_pre;	

	int Swidth,SHeight;
	uint8_t *writeAddr = NULL;
	uint32_t ch_line_bytes = GIFstruct->chwidth * SCREEN_BPP;
	uint32_t pos_h,pos_x,pos_pixels;
	for (h = 0; h < GIFstruct->ctheight; h++)
	{  
		GifRow = GIFstruct->ScreenBuffer[h]; 
		pos_h = (h + GIFstruct->cy) * ch_line_bytes;
		for (w = 0; w < GIFstruct->ctwidth; w++) 
		{  		
			pos_pixels = pos_h + (w+GIFstruct->cx) * SCREEN_BPP;
			if( GIFstruct->trans_color != -1 && GIFstruct->trans_color == GifRow[w] )
			{  
				GIFstruct->cur_frame[pos_pixels + 0] = GIFstruct->pre_frame[pos_pixels + 0];
				GIFstruct->cur_frame[pos_pixels + 1] = GIFstruct->pre_frame[pos_pixels + 1];
				GIFstruct->cur_frame[pos_pixels + 2] = GIFstruct->pre_frame[pos_pixels + 2];
				continue;  
			}
			ColorMapEntry = &GIFstruct->ColorMap->Colors[GifRow[w]];  
			GIFstruct->cur_frame[pos_pixels + 0] = ColorMapEntry->Blue;
			GIFstruct->cur_frame[pos_pixels + 1] = ColorMapEntry->Green;
			GIFstruct->cur_frame[pos_pixels + 2] = ColorMapEntry->Red;
		} 
	}

	return 0;
}






GIFerror_t GIF_decoder(GIFstruct_t *GIFstruct)
{
	int i = 0;
	GifWord Row,Col,Width,Height;
	while(1)
	{
		DEBUG_PRINTF;
		if (DGifGetRecordType(GIFstruct->GifFile, &(GIFstruct->RecordType)) == GIF_ERROR)
		{  
			DEBUG_PRINTF;
			perror("DGifGetRecordType");
			return -1; 
		} 

		if(GIFstruct->RecordType == TERMINATE_RECORD_TYPE)
			return 0;

		switch(GIFstruct->RecordType)
		{
			case IMAGE_DESC_RECORD_TYPE:
				debug_printf("IMAGE_DESC_RECORD_TYPE============\n");
				if (DGifGetImageDesc(GIFstruct->GifFile) == GIF_ERROR) 
				{  
					return -1;	
				}  
				DEBUG_PRINTF;
				Row = GIFstruct->GifFile->Image.Top; /* Image Position relative to Screen. */  
				Col = GIFstruct->GifFile->Image.Left;  
				Width = GIFstruct->GifFile->Image.Width;  
				Height = GIFstruct->GifFile->Image.Height;	
				DEBUG_PRINTF;
				//rgb_buffer = (uint8_t *)malloc(GifFile->SWidth * GifFile->SHeight * 4);
				debug_printf("GifFile->SWidth = %d,GifFile->SHeight = %d,GifFile->Image.Width = %d\n",GIFstruct->GifFile->SWidth,GIFstruct->GifFile->SHeight,GIFstruct->GifFile->Image.Width);
				debug_printf("Col = %d,Row = %d,Width = %d,Height = %d\n",Col, Row, Width, Height); 
				
				if (GIFstruct->GifFile->Image.Left + GIFstruct->GifFile->Image.Width > GIFstruct->GifFile->SWidth ||  
				GIFstruct->GifFile->Image.Top + GIFstruct->GifFile->Image.Height > GIFstruct->GifFile->SHeight)
				{  
					debug_printf("Image is not confined to screen dimension, aborted.\n");  
					return -1; 
				} 

				DEBUG_PRINTF;
				if (GIFstruct->GifFile->Image.Interlace) 
				{  
#if 0
					/* Need to perform 4 passes on the images: */  
					for (Count = i = 0; i < 4; i++)  
					for (j = Row + InterlacedOffset[i]; j < Row + Height; j += InterlacedJumps[i]) 
					{  
						//GifQprintf("\b\b\b\b%-4d", Count++);	
						if (DGifGetLine(GifFile, &ScreenBuffer[j][Col], Width) == GIF_ERROR) 
						{  
							//PrintGifError();	
							//exit(EXIT_FAILURE); 
							return -1;
						}  
					} 
#endif
				}  
				else 
				{  
					DEBUG_PRINTF;
					for (i = 0; i < Height; i++)
					{  
						//DEBUG_PRINTF;
						//debug_printf("Row = %d,Height = %d\n",Row,Height);
						if (DGifGetLine(GIFstruct->GifFile, &GIFstruct->ScreenBuffer[Row++][Col], Width) == GIF_ERROR) 
						{  
							DEBUG_PRINTF;
							//PrintGifError();	
							return -1 ;
						} 
						
					}  

				}  
				DEBUG_PRINTF;
				/* Get the color map */  
				GIFstruct->ColorMap = (GIFstruct->GifFile->Image.ColorMap ? GIFstruct->GifFile->Image.ColorMap : GIFstruct->GifFile->SColorMap);  
				if (GIFstruct->ColorMap == NULL) 
				{  
					debug_printf("Gif Image does not have a colormap\n");	
					return -1;
				}
				DEBUG_PRINTF;
				if(GIF_RGBA_decode(GIFstruct) < 0)
				{
					debug_printf("read is busy\n");
				}
				DEBUG_PRINTF;				
				//DumpScreen2RGBA(GIFbuffer->des_buffer,400,128,3,i,ScreenBuffer, GifFile->SWidth, GifFile->SHeight,100,0);	
				
				break;

			case EXTENSION_RECORD_TYPE:
				debug_printf("EXTENSION_RECORD_TYPE------------------\n");
				if(DGifGetExtension(GIFstruct->GifFile,&GIFstruct->Extcode,&GIFstruct->extension)== GIF_ERROR)
				{
					debug_printf("get the extension code fail!\n");
					return -1;
				}
				
				if( GIFstruct->Extcode == GIF_CONTROL_EXT_CODE && GIFstruct->extension[0] == GIF_CONTROL_EXT_SIZE) 
				{  
					int delay = 0;
					delay = (GIFstruct->extension[3] << 8 | GIFstruct->extension[2]) * 10;  
					debug_printf("delay = %d\n",delay);
					/* Can sleep here */ 
					//usleep(delay);
				}  
			
				/* handle transparent color */	
				if( (GIFstruct->extension[1] & 1) == 1 )
				{	
					debug_printf("extension[1] = %d,extension[4] = %d\n",GIFstruct->extension[1],GIFstruct->extension[4]);
					GIFstruct->trans_color = GIFstruct->extension[4];  
				}  
				else
				{
					DEBUG_PRINTF;
					GIFstruct->trans_color = -1;
				}
#if 1
				while(GIFstruct->extension!=NULL)
				{
					usleep(100);
					DEBUG_PRINTF;
					if(DGifGetExtensionNext(GIFstruct->GifFile,&GIFstruct->extension) == GIF_ERROR)
					break;
				}
#endif
				break;

			default:
				break;
		}




		
	}
}


void GIF_INITstruct(GIFstruct_t *GIFstruct,char *gifpath,uint16_t cx,uint16_t cy,uint16_t c_width,uint16_t c_height,uint8_t *cache)
{
	GIFstruct->cx 			= cx;
	GIFstruct->cy 			= cy;
	GIFstruct->chwidth  	= c_width;
	GIFstruct->chheight 	= c_height;
	//将当前帧缓存指针与上一帧缓存指针同指一块缓存中
	GIFstruct->cur_frame	= cache;
	GIFstruct->pre_frame	= cache;
	debug_printf("gifpath = %s,%d\n",gifpath,strlen(gifpath));
	memset(GIFstruct->filename,0,sizeof(GIFstruct->filename));
	memcpy(GIFstruct->filename,gifpath,strlen(gifpath));
	
	GIFstruct->GIF_init 	= GIF_init;
	GIFstruct->GIF_free		= GIF_ResFree;
	GIFstruct->GIF_close	= GIF_close;

	GIFstruct->frameInfo	= GIF_getFrameInfo;
	GIFstruct->frameDecoder = GIF_frameDecoder;
	
}

















#if 0


void m_rend_gif_decodecolormap(unsigned char *cmb,unsigned char *rgbb,ColorMapObject *cm,int s,int l, int transparency)
{
	 
    GifColorType *cmentry;
    int i;
    for(i=0;i<l;i++)
    {
		cmentry=&cm->Colors[cmb[i]];
		*(rgbb++)=cmentry->Red;
		*(rgbb++)=cmentry->Green;
		*(rgbb++)=cmentry->Blue;
		*(rgbb++)=0x00;
    }
}


/* Thanks goes here to Mauro Meneghin, who implemented interlaced GIF files support */
int syl_gif_load(char *name,unsigned char *buffer, int x,int y)
{
	int in_nextrow[4]={8,8,4,2};   //interlaced jump to the row current+in_nextrow
	int in_beginrow[4]={0,4,2,1};  //begin pass j from that row number
	int transparency=-1;  //-1 means not transparency present
	int px,py,i,ibxs;
	int j;
	char *fbptr;
	char *lb;
	char *slb;
	char * alpha=NULL;
	GifFileType *gft;
	GifByteType *extension;
	int extcode;
	GifRecordType rt;
	ColorMapObject *cmap;
	int cmaps;
	int err = -1;
	gft=DGifOpenFileName(name,&err);
	if(gft==NULL)
	{
		//printf("err5\n"); 
		//gflush;
		debug_printf("open the gif file failed!\n");
	} //////////

	
	do
	{
		if(DGifGetRecordType(gft,&rt) == GIF_ERROR)
		{
			debug_printf("get the gif block type fail!\n");
			exit(1);
		}
		switch(rt)
		{
			case IMAGE_DESC_RECORD_TYPE:
				if(DGifGetImageDesc(gft)==GIF_ERROR)
				{
					debug_printf("get the fig image message fail\n");
					exit(1);
				}
				
				px=gft->Image.Width;
				py=gft->Image.Height;
				lb=(char*)malloc(px*4);
				slb=(char*) malloc(px);
				
				//  printf("reading...\n");
				if(lb!=NULL && slb!=NULL)
				{
					unsigned char *alphaptr = NULL;

					cmap=(gft->Image.ColorMap ? gft->Image.ColorMap : gft->SColorMap);
					cmaps=cmap->ColorCount;
					ibxs=ibxs*3;
					fbptr=(char*)buffer;

					if(transparency != -1)
					{
						alphaptr = malloc(px * py);
						//	*alpha = alphaptr;
					}

					if(!(gft->Image.Interlace))
					{
						DEBUG_PRINTF;
						for(i=0;i<py;i++,fbptr+=px*4)
						{
							int j;
							if(DGifGetLine(gft,(GifPixelType*)slb,px)==GIF_ERROR)
							{
								debug_printf("1get the gif line message fail\n");
							}
							m_rend_gif_decodecolormap((unsigned char*)slb,(unsigned char*)lb,cmap,cmaps,px,transparency);
							memcpy(fbptr,lb,px*4);
							if(alphaptr)
							{
								for(j = 0; j<px; j++)
								{
									*(alphaptr++) = (((unsigned char*) slb)[j] == transparency) ? 0x00 : 0xff;
								}
		
							}
						}
						//fb_frame_display(0,0,fbptr,32,3000);
						usleep(1000*1000);
					}
					else
					{
						unsigned char * aptr = NULL;

						for(j=0;j<4;j++)
						{
							int k;
							if(alphaptr)
							aptr = alphaptr + (in_beginrow[j] * px);

							fbptr=(char*)buffer + (in_beginrow[j] * px * 3);
							for(i = in_beginrow[j]; i<py; i += in_nextrow[j], fbptr += px * 3 * in_nextrow[j], aptr += px * in_nextrow[j])
							{
								if(DGifGetLine(gft,(GifPixelType*)slb,px)==GIF_ERROR)
								{
									debug_printf("2get the gif line message fail\n");
								}
								m_rend_gif_decodecolormap((unsigned char*)slb,(unsigned char*)lb,cmap,cmaps,px,transparency);
								memcpy(fbptr,lb,px*3);
								if(alphaptr)
								for(k = 0; k<px; k++) aptr[k] = (((unsigned char*) slb)[k] == transparency) ? 0x00 : 0xff;
							}
						}
					}
				}

				
				if(lb) free(lb);
				if(slb) free(slb);
				break;

			
			case EXTENSION_RECORD_TYPE:
				if(DGifGetExtension(gft,&extcode,&extension)==GIF_ERROR)
				{
					debug_printf("get gif file extension failed\n");
					exit(1);
				}
				if(extcode==0xf9) //look image transparency in graph ctr extension
				{
					if(extension[1] & 1)
					{
						transparency = extension[4];
					}
					//		    tran_off=(int)*extension;
					//		    transparency=(int)*(extension+tran_off);
					//			printf("transparency: %d\n", transparency);
				}
				while(extension!=NULL)
				if(DGifGetExtensionNext(gft,&extension) == GIF_ERROR)return 0;
				break;
				
			default:
				break;
		}
	}

	while( rt!= TERMINATE_RECORD_TYPE );
	DGifCloseFile(gft,&err);
	return(1);
}

#endif
