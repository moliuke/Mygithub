#include "config.h"
#include "character.h"
#include "Dev_framebuffer.h"
#include "specialstr.h"




uint8_t *TXTCache = NULL;




//检索字体目录下的文件的正确的文件名，如文件名开头字母"HZK"，但由于用户的拷贝过来的文件
//名的开头的字母有可能是"HZk",也可能是"hzk"等等
//asc码字符文件名同理
//汉字库楷体32号:"HZK32K"，用户的文件名有可能是"HZK32k"，甚至可能是"HZK32"
static int ch_fontfile_adapter(char *i_dir,char *o_dir,char *fontype,uint8_t fontsize,uint8_t flag)
{
	int ret = -1;

	char *str_font[] = {"HZK","HZk","HzK","hZK","Hzk","hZk","hzK","zhk"};
	char *str_acii[] = {"ASC","ASc","AsC","aSC","Asc","aSc","asC","asc"};
	char str_char[3] = {""};
	char **str = NULL;

	int n = 0,m = 0;
	char strfile[96];

	if(flag == 0)
		str = str_font;
	else
		str = str_acii;


	char fontdir[50];
	char *ch_p = i_dir;
	char *ch_q = i_dir;
	while((ch_q = strchr(ch_q,'/')) != NULL)
	{
		ch_p = ch_q;
		ch_q = ch_q + 1;
	}
	memset(fontdir,0x00,sizeof(fontdir));
	memcpy(fontdir,i_dir,ch_p - i_dir);
	

	if(*fontype >= 65 && *fontype <= 90)
	{
		str_char[1] = *fontype;
		str_char[2] = *fontype + 32;
	}
	else
	{
		str_char[1] = *fontype - 32;
		str_char[2] = *fontype;
	}
	
	for(n = 0 ; n < 8 ; n ++)
	{
		for(m = 0 ; m < 3 ; m ++)
		{
			memset(strfile,0x00,sizeof(strfile));
			sprintf(strfile,"%s/%s%d%c",fontdir,str[n],fontsize,str_char[m]);
			//debug_printf("strfile = %s\n",strfile);
			ret = access(strfile,F_OK);
			if(ret >= 0)
			{
				memcpy(o_dir,strfile,strlen(strfile));
				o_dir[strlen(strfile)] = '\0';
				return 0;
			}
		}
	}

	return -1;
}

static int TXT_getFontFile(TXTstruct_t *TXTstruct)
{
	int ret = -1;
	char chineseFont[48];
	char asciiFont[48];
	
	memset(chineseFont,0,sizeof(chineseFont));
	memset(asciiFont,0,sizeof(asciiFont));
	sprintf(chineseFont,"%s/HZK%d%c",font_dir,TXTstruct->fontSize,TXTstruct->fontType);
	sprintf(asciiFont,"%s/HZK%d%c",font_dir,TXTstruct->fontSize,TXTstruct->fontType);

	ret = ch_fontfile_adapter(chineseFont,TXTstruct->CHINAFile,&TXTstruct->fontType,TXTstruct->fontSize,0);
	if(ret < 0)
	{
		DEBUG_PRINTF;
		return -1;
	}
	
	ret = ch_fontfile_adapter(asciiFont,TXTstruct->ASCIIFile,&TXTstruct->fontType,TXTstruct->fontSize,1);
	if(ret < 0)
	{
		DEBUG_PRINTF;
		return -1;
	}

	debug_printf("chineseFont = %s,asciiFont = %s\n",chineseFont,asciiFont);
	return 0;
}

static int TXT_openFontFile(TXTstruct_t *TXTstruct)
{
	if ((TXTstruct->fpFont = fopen(TXTstruct->CHINAFile,"rb"))==NULL) 
	{
		char_debug_printf("[%s] printf : fopen file <%s> failed!\n",__func__,TXTstruct->CHINAFile);
		return -1;
	}
	if((TXTstruct->fpAsc = fopen(TXTstruct->ASCIIFile,"rb")) == NULL)
	{
		char_debug_printf("[%s] printf : fopen file <%s> failed!\n",__func__,TXTstruct->ASCIIFile);
		fclose(TXTstruct->fpFont);
		return -1;
	}

	debug_printf("TXTstruct->CHINAFile = %s,TXTstruct->ASCIIFile = %s\n",TXTstruct->CHINAFile,TXTstruct->ASCIIFile);

	return 0;
}


/*
file			:字库文件名
charaterbuf	:待显示的字符
fontype		:字体
fontsize		:字体大小
w_space		:字间距
l_space		:行间距
forcolor		:字体颜色
bakcolor		:填充背景色
ct_attr		:解析完成，字体的属性，包括字体的大小，长宽等等
*/
int TXT_decoder(TXTstruct_t *TXTstruct)
{
	uint8_t  High8bit,Low8bit; 

	uint8_t  BYTEbits = 0;		//
	uint8_t  LSTBYTEbits = 0;	//一个文字一行占据的像素点需要用几个字节表示，这里表示最后一个字节前多少位有效
	uint32_t offset = 0x00;
	uint8_t  pDspBuf[512] 		= {0x00};	//中文点阵数据
	uint8_t  AscBuffer[256] 	= {0x00};    // ascii点阵数据
	uint16_t row_cnt = 0,col_cnt = 0; 
	uint16_t i=0,j=0;
	uint16_t pIndex = 0x00;				//指明当前读取的字符的位置
	uint16_t Xpos = 0,Ypos = 0;			//Xpos以字符为单位标记行写到什么位置，Ypos以行为单位标记书写到第几行了
	uint16_t x = 0x00,y=0x00;				//虚拟屏幕坐标
	
	uint16_t chinese_number = 0;					//描述中文文字个数
	uint16_t ascii_number = 0;					//描述字母个数
	
	uint16_t chinese_font_size = 0;				//描述一个中文文字占用多少字节
	uint16_t ascii_font_size = 0;				//描述一个ascii字符占用多少字节

	uint16_t char_lines 		= 0;				//每个字符占用的行数(汉字与字母)
	uint16_t char_line_pixel = 0;				//字符每行占用的像素点数
	uint16_t char_line_Bytes = 0;				//字符每行占用的字节数


	uint16_t chinese_lines 		= TXTstruct->fontSize;		//中文文字占用的行数
	uint16_t chinese_line_pixel 	= TXTstruct->fontSize;		//中文文字每行占用的像素点数
	uint16_t chinese_line_Bytes 	= chinese_line_pixel / 8;	//中文文字单行占用的字节数

	uint16_t ascii_lines 		= TXTstruct->fontSize;		//ascii字符每个字符占用的行数
	uint16_t ascii_line_pixel 	= 0;			//ascii字符每个字符每行占用的像素点数
	uint16_t ascii_line_Bytes 	= 0;			//ascii字符每个字符每行占用的字节数
	
	uint8_t *char_tmp = NULL;					//解析在后面
	uint8_t linefeed = 0;						//换行标记
	uint8_t ii = 0, jj = 0;

	uint32_t x_max = 0,y_max = 0;				//分别标记横向与纵向书写到的最大位置
	int16_t char_num = 0;						//标记有多少个字符
	int16_t char_pos_flag = 0;
	
	int ret = -1;

	//TXTstruct->Lspace = 10;

	debug_printf("TXTstruct->Wspace = %d\n",TXTstruct->Wspace);

	chinese_font_size = TXTstruct->fontSize * TXTstruct->fontSize / 8;
	if(TXT_getFontFile(TXTstruct) < 0)
		return -1;
	if(TXT_openFontFile(TXTstruct) < 0)
		return -1;

	//缓存总大小除以每个像素点占的字节数
	int pixelsCount = TXTstruct->size / TXTstruct->BPP;
	//int block_height = TXTstruct->fontSize + TXTstruct->Lspace;				//单位是像素点
	int block_height = TXTstruct->fontSize + TXTstruct->Lspace;				//单位是像素点
	int block_width = pixelsCount / block_height;
	//初始化横向与纵向的最大书写位置值
	char_debug_printf("block_width = %d\n",block_width);
	x_max = 0;
	y_max = 0;
	//开始对输入字符串按照一个虚拟屏幕来书写布局缓存
	int k = 0;
	uint8_t *charaterbuf = TXTstruct->content;

	while(*charaterbuf != 0)
	{
		//debug_printf("*charaterbuf = 0x%x\n",*charaterbuf);
		if(*charaterbuf == '\n' || *charaterbuf == '\r' || *charaterbuf == '\0')
		{
			break;
		}
		//判断是否含有换行符
		if(*charaterbuf == '\\')
		{
			if(*(charaterbuf+1)=='N' || *(charaterbuf+1) == 'n')
			{
			#if 0
				//此处x_max标记了换行之前当前行的最大位置，如果下一行的最大位置处
				//比x_max还大，那么x_max的值取较大的那个
				if(x_max < Xpos)
					x_max = Xpos;

				//换行后，横坐标回到原点，纵坐标向下移动一个字符的大小
				Xpos = 0;
				Ypos += fontsize + l_space;
			#endif
				TXTstruct->ch_flag[char_pos_flag++] = '\n';
				Xpos += TXTstruct->fontSize;
				//charaterbuf = charaterbuf;
			}
			charaterbuf = charaterbuf+2;
			continue;
		}
		

		//这里的目的是保证在下面填进一个字之后，后面还能放一个'\0'
		if(Xpos + 2 * TXTstruct->fontSize + TXTstruct->Wspace > block_width)
			break;
		
		//保存下当前所取得字符的地址，最后有用.
		//字母只有一个字节，但汉字有两个字节，char_tmp
		//用于判断当前字符是字母的开端还是汉字的开端
		char_tmp = charaterbuf;


		//字母
		if(*charaterbuf< 0x80 )
		{
			// 读取asc点阵到缓冲
			uint8_t byAscChar;

			byAscChar = *charaterbuf;

			if(TXTstruct->fontSize == 12)
			{	//'A'
				byAscChar = byAscChar - 0x20;
				ascii_font_size = 12;
				ascii_line_pixel = 6;
				ascii_line_Bytes = 1;
				//BYTEbits = 6;
				LSTBYTEbits = 6;
			}

			else if(TXTstruct->fontSize == 14)
			{
				byAscChar = byAscChar;
				ascii_font_size = 14;
				ascii_line_pixel = 7;
				ascii_line_Bytes = 1;
				//BYTEbits = 7;
				LSTBYTEbits = 7;
			}
			else if(TXTstruct->fontSize == 24)
			{
				ascii_font_size = 48;//一个字母占用48个字节表示:24行，每行2个字节
				ascii_line_pixel = 12;
				ascii_line_Bytes = 2;
				//BYTEbits = 8;
				LSTBYTEbits = 8;
			}
			else if(TXTstruct->fontSize == 40)
			{
				//byAscChar = byAscChar - 0x20;
				ascii_font_size = 120;//一个字母占用120个字节:40行，每行3个字节
				ascii_line_pixel = 20;
				ascii_line_Bytes = 3;
				LSTBYTEbits = 4;
			}
			else
			{
				//BYTEbits = 8;
				LSTBYTEbits = 8;
				ascii_font_size = chinese_font_size / 2;
				ascii_line_pixel = TXTstruct->fontSize / 2;
				ascii_line_Bytes = ascii_line_pixel / 8;
			}
			
			offset = byAscChar * ascii_font_size;
			fseek(TXTstruct->fpAsc,offset,SEEK_SET);
			fread(pDspBuf,ascii_font_size,1,TXTstruct->fpAsc);


			char_lines 		= ascii_lines;		//字体占的行数
			char_line_pixel = ascii_line_pixel;	//字体
			char_line_Bytes = ascii_line_Bytes;

			
		}
		//汉字
		else
		{
			High8bit = *(charaterbuf);		// 取高8位数据	
			charaterbuf++;
			Low8bit =*(charaterbuf);		//	取低8位数据 

			if(High8bit < 0xA1)
			{
				DEBUG_PRINTF;
				SpecialStrProcess(pDspBuf,TXTstruct->fontSize,High8bit,Low8bit);
				DEBUG_PRINTF;
			}
			else
			{
				//(High8bit-161l)*94 : 跨度区数*每个区有94个字
				//Low8bit-161		   : 所在区的位置,在汉字编码表中每个区位加上0xA0来表示。区和位分别占用一个字节
				offset = ((High8bit-0xA1)*94l+Low8bit-0xA1)*chinese_font_size;
				fseek(TXTstruct->fpFont,offset,SEEK_SET);
				fread(pDspBuf,chinese_font_size,1,TXTstruct->fpFont);
			}
			char_lines		= chinese_lines;
			char_line_pixel = chinese_line_pixel;
			char_line_Bytes = chinese_line_Bytes;
			//BYTEbits = 8;
			LSTBYTEbits = 8;
		}
		//XKSendComString(TX_COM,pDspBuf,s_Font->byteCnt);




		//debug_printf("char_lines = %d,char_line_pixel = %d,char_line_Bytes = %d\n",char_lines,char_line_pixel,char_line_Bytes);
		
		
		++k;
		uint32_t pixels_locate = 0;
		//每个字含有chinese_lines行数据,每行含有chinese_line_pixel个像素点
		//每行含有chinese_line_Bytes个字节，每个字节含有8bits		
		for(row_cnt = 0 ; row_cnt < char_lines ; row_cnt ++)
		{
			for(col_cnt = 0 ; col_cnt < char_line_Bytes ; col_cnt ++)
			{
				//最后一个字节的有效位数
				BYTEbits = (col_cnt == char_line_Bytes - 1) ? LSTBYTEbits : 8;
				
				for( i = 0 ; i < BYTEbits ; i ++)
				{
					x = Xpos+j;//i描述的是每个字节的第 几 bit，j描述的是每个字符一行中的第几个像素点
					y = Ypos+row_cnt;

					pixels_locate = (y * block_width + x) * TXTstruct->BPP;
					if(((pDspBuf[pIndex] >> (7-i))&0x01) == 0x01)
					{
						//debug_printf("@ ");
						TXTstruct->cache[pixels_locate + BYTE_RGB_B] = TXTstruct->Fcolor.blue;
						TXTstruct->cache[pixels_locate + BYTE_RGB_G] = TXTstruct->Fcolor.green;
						TXTstruct->cache[pixels_locate + BYTE_RGB_R] = TXTstruct->Fcolor.red;
					}
					else if(TXTstruct->BackgrdFlag)
					{
						TXTstruct->cache[pixels_locate + BYTE_RGB_B] = TXTstruct->Bcolor.blue;
						TXTstruct->cache[pixels_locate + BYTE_RGB_G] = TXTstruct->Bcolor.green;
						TXTstruct->cache[pixels_locate + BYTE_RGB_R] = TXTstruct->Bcolor.red;
					}
					else
					{
						//debug_printf("  ");
					}
					j++;
					if(j == char_line_pixel)
					{
						//debug_printf("\n");
						break;
					}
				}
				pIndex++;
			}
			//debug_printf("\n");
			j = 0;
		}

		
		//计数字符的个数,并将坐标位置向后挪动一定的位置(每个字占用的行像素+字间隙)
		if(*char_tmp < 0x80)
		{
			TXTstruct->ch_flag[char_pos_flag++] = 'C';
			ascii_number++;
		}

		else
		{
			TXTstruct->ch_flag[char_pos_flag++] = 'H';
			chinese_number++;
		}
		//横坐标先向右偏移一个文字的大小
		Xpos += char_line_pixel; 
		//然后开始描文字右方char_lines行，每行w_space个像素的间隙背景色
		if(TXTstruct->Wspace != 0 && TXTstruct->BackgrdFlag)
		{
			for(ii= 0 ; ii < char_lines ; ii ++ )
			{
				for(jj = 0 ; jj < TXTstruct->Wspace ; jj ++)
				{
					x = Xpos + jj;
					y = Ypos + ii;
					pixels_locate = (y * block_width + x) * TXTstruct->BPP;
					TXTstruct->cache[pixels_locate + BYTE_RGB_B] = TXTstruct->Bcolor.blue;
					TXTstruct->cache[pixels_locate + BYTE_RGB_G] = TXTstruct->Bcolor.green;
					TXTstruct->cache[pixels_locate + BYTE_RGB_R] = TXTstruct->Bcolor.red;
				}
			}

		}
		//先向后偏移w_space(文字间隙)，再向前偏移w_space + char_line_pixel(这样好理解一点)
		//在每个文字下方接着描l_space(行间隙)，每行w_space + char_line_pixel的背景色
		Xpos += TXTstruct->Wspace;
		Xpos -= (TXTstruct->Wspace + char_line_pixel);
		//纵坐标先向下偏移一个文字的大小
		Ypos += TXTstruct->fontSize;
		//开始描文字下方的背景色
		if(TXTstruct->Lspace != 0)
		{
			for(ii= 0 ; ii < TXTstruct->Lspace; ii ++ )
			{
				for(jj = 0 ; jj < char_line_pixel + TXTstruct->Wspace ; jj ++)
				{
					x = Xpos + jj;
					y = Ypos + ii;

					TXTstruct->cache[pixels_locate + BYTE_RGB_B] = TXTstruct->Bcolor.blue;
					TXTstruct->cache[pixels_locate + BYTE_RGB_G] = TXTstruct->Bcolor.green;
					TXTstruct->cache[pixels_locate + BYTE_RGB_R] = TXTstruct->Bcolor.red;
					if(TXTstruct->BPP == 4)
						TXTstruct->cache[pixels_locate + BYTE_RGB_A] = 0x00;
				}
			}

		}

		//描完文字下方的背景色后，横纵坐标要回到偏移到下一个文字的起始位置
		Xpos += TXTstruct->Wspace + char_line_pixel;
		Ypos -= TXTstruct->fontSize;

		//指针偏移到下一个字符
		charaterbuf++;
		//字符数据位置归0，等待读下一个字符
		pIndex = 0x00;
		
		
	}
	//打开的文件记得关闭
	fclose(TXTstruct->fpFont);
	fclose(TXTstruct->fpAsc);
	TXTstruct->ch_flag[char_pos_flag] = '\0';
	Xpos += TXTstruct->fontSize + TXTstruct->Wspace;

	//计算显示给定文字需要的横向跟纵向的最大尺寸
	char_debug_printf("x_max = %d,char_pos_flag = %d\n",x_max,char_pos_flag);
	if(x_max < Xpos)
		x_max = Xpos;

	y_max = Ypos + TXTstruct->fontSize + TXTstruct->Lspace;
	char_debug_printf("Ypos = %d,TXTstruct->fontSize = %d,TXTstruct->Lspace = %d\n",Ypos,TXTstruct->fontSize,TXTstruct->Lspace);

	TXTstruct->ctwidth	= x_max;
	TXTstruct->ctheight	= y_max;
	TXTstruct->chwidth	= block_width;
	TXTstruct->chheight	= y_max;
	TXTstruct->ch_num	= ascii_number + chinese_number;
	TXTstruct->ch_csize	= ascii_line_pixel;
	TXTstruct->ch_hsize	= chinese_line_pixel;
	debug_printf("TXTstruct->ctwidth = %d\n",TXTstruct->ctwidth);
	
	return 0;
}


void TXT_INITstruct(TXTstruct_t *TXTstruct,uint8_t *content,uint16_t len,
	Ftype_t fontType,Fsize_t fontSize,Pcolor_t *Bcolor,Pcolor_t *Fcolor,
	uint8_t Wspace,uint8_t Lspace,uint8_t *cache,uint32_t size,uint8_t BPP)
{
	TXTstruct->fontSize = fontSize;
	TXTstruct->fontType = fontType;
	
	TXTstruct->Fcolor.blue	= Fcolor->blue;
	TXTstruct->Fcolor.green	= Fcolor->green;
	TXTstruct->Fcolor.red	= Fcolor->red;

	TXTstruct->Bcolor.blue	= Bcolor->blue;
	TXTstruct->Bcolor.green	= Bcolor->green;
	TXTstruct->Bcolor.red	= Bcolor->red;
	if(Bcolor->blue || Bcolor->green || Bcolor->red)
		TXTstruct->BackgrdFlag = 1;
	else
		TXTstruct->BackgrdFlag = 0;

	TXTstruct->Lspace		= Lspace;
	char_debug_printf("%d,%d\n",Lspace,TXTstruct->Lspace);
	TXTstruct->Wspace		= Wspace;

	TXTstruct->cache		= cache;
	TXTstruct->BPP			= BPP;
	TXTstruct->size			= size;
	TXTstruct->ctent		= content;
	memcpy(TXTstruct->content,content,len);
	TXTstruct->content[len] = '\0';
	
	TXTstruct->TXT_decoder	= TXT_decoder;
}


void TXT_DecodeMemInit(uint8_t **DecodeCache,uint32_t size)
{
	*DecodeCache = (uint8_t *)malloc(size);
}

void TXT_DecodeMemFree(uint8_t *DecodeCache)
{
	if(DecodeCache != NULL)
		free(DecodeCache);
}




