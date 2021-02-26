#include "config.h"
#include "character.h"
#include "Dev_framebuffer.h"
#include "specialstr.h"




uint8_t *TXTCache = NULL;




//��������Ŀ¼�µ��ļ�����ȷ���ļ��������ļ�����ͷ��ĸ"HZK"���������û��Ŀ����������ļ�
//���Ŀ�ͷ����ĸ�п�����"HZk",Ҳ������"hzk"�ȵ�
//asc���ַ��ļ���ͬ��
//���ֿ⿬��32��:"HZK32K"���û����ļ����п�����"HZK32k"������������"HZK32"
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
file			:�ֿ��ļ���
charaterbuf	:����ʾ���ַ�
fontype		:����
fontsize		:�����С
w_space		:�ּ��
l_space		:�м��
forcolor		:������ɫ
bakcolor		:��䱳��ɫ
ct_attr		:������ɣ���������ԣ���������Ĵ�С������ȵ�
*/
int TXT_decoder(TXTstruct_t *TXTstruct)
{
	uint8_t  High8bit,Low8bit; 

	uint8_t  BYTEbits = 0;		//
	uint8_t  LSTBYTEbits = 0;	//һ������һ��ռ�ݵ����ص���Ҫ�ü����ֽڱ�ʾ�������ʾ���һ���ֽ�ǰ����λ��Ч
	uint32_t offset = 0x00;
	uint8_t  pDspBuf[512] 		= {0x00};	//���ĵ�������
	uint8_t  AscBuffer[256] 	= {0x00};    // ascii��������
	uint16_t row_cnt = 0,col_cnt = 0; 
	uint16_t i=0,j=0;
	uint16_t pIndex = 0x00;				//ָ����ǰ��ȡ���ַ���λ��
	uint16_t Xpos = 0,Ypos = 0;			//Xpos���ַ�Ϊ��λ�����д��ʲôλ�ã�Ypos����Ϊ��λ�����д���ڼ�����
	uint16_t x = 0x00,y=0x00;				//������Ļ����
	
	uint16_t chinese_number = 0;					//�����������ָ���
	uint16_t ascii_number = 0;					//������ĸ����
	
	uint16_t chinese_font_size = 0;				//����һ����������ռ�ö����ֽ�
	uint16_t ascii_font_size = 0;				//����һ��ascii�ַ�ռ�ö����ֽ�

	uint16_t char_lines 		= 0;				//ÿ���ַ�ռ�õ�����(��������ĸ)
	uint16_t char_line_pixel = 0;				//�ַ�ÿ��ռ�õ����ص���
	uint16_t char_line_Bytes = 0;				//�ַ�ÿ��ռ�õ��ֽ���


	uint16_t chinese_lines 		= TXTstruct->fontSize;		//��������ռ�õ�����
	uint16_t chinese_line_pixel 	= TXTstruct->fontSize;		//��������ÿ��ռ�õ����ص���
	uint16_t chinese_line_Bytes 	= chinese_line_pixel / 8;	//�������ֵ���ռ�õ��ֽ���

	uint16_t ascii_lines 		= TXTstruct->fontSize;		//ascii�ַ�ÿ���ַ�ռ�õ�����
	uint16_t ascii_line_pixel 	= 0;			//ascii�ַ�ÿ���ַ�ÿ��ռ�õ����ص���
	uint16_t ascii_line_Bytes 	= 0;			//ascii�ַ�ÿ���ַ�ÿ��ռ�õ��ֽ���
	
	uint8_t *char_tmp = NULL;					//�����ں���
	uint8_t linefeed = 0;						//���б��
	uint8_t ii = 0, jj = 0;

	uint32_t x_max = 0,y_max = 0;				//�ֱ��Ǻ�����������д�������λ��
	int16_t char_num = 0;						//����ж��ٸ��ַ�
	int16_t char_pos_flag = 0;
	
	int ret = -1;

	//TXTstruct->Lspace = 10;

	debug_printf("TXTstruct->Wspace = %d\n",TXTstruct->Wspace);

	chinese_font_size = TXTstruct->fontSize * TXTstruct->fontSize / 8;
	if(TXT_getFontFile(TXTstruct) < 0)
		return -1;
	if(TXT_openFontFile(TXTstruct) < 0)
		return -1;

	//�����ܴ�С����ÿ�����ص�ռ���ֽ���
	int pixelsCount = TXTstruct->size / TXTstruct->BPP;
	//int block_height = TXTstruct->fontSize + TXTstruct->Lspace;				//��λ�����ص�
	int block_height = TXTstruct->fontSize + TXTstruct->Lspace;				//��λ�����ص�
	int block_width = pixelsCount / block_height;
	//��ʼ������������������дλ��ֵ
	char_debug_printf("block_width = %d\n",block_width);
	x_max = 0;
	y_max = 0;
	//��ʼ�������ַ�������һ��������Ļ����д���ֻ���
	int k = 0;
	uint8_t *charaterbuf = TXTstruct->content;

	while(*charaterbuf != 0)
	{
		//debug_printf("*charaterbuf = 0x%x\n",*charaterbuf);
		if(*charaterbuf == '\n' || *charaterbuf == '\r' || *charaterbuf == '\0')
		{
			break;
		}
		//�ж��Ƿ��л��з�
		if(*charaterbuf == '\\')
		{
			if(*(charaterbuf+1)=='N' || *(charaterbuf+1) == 'n')
			{
			#if 0
				//�˴�x_max����˻���֮ǰ��ǰ�е����λ�ã������һ�е����λ�ô�
				//��x_max������ôx_max��ֵȡ�ϴ���Ǹ�
				if(x_max < Xpos)
					x_max = Xpos;

				//���к󣬺�����ص�ԭ�㣬�����������ƶ�һ���ַ��Ĵ�С
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
		

		//�����Ŀ���Ǳ�֤���������һ����֮�󣬺��滹�ܷ�һ��'\0'
		if(Xpos + 2 * TXTstruct->fontSize + TXTstruct->Wspace > block_width)
			break;
		
		//�����µ�ǰ��ȡ���ַ��ĵ�ַ���������.
		//��ĸֻ��һ���ֽڣ��������������ֽڣ�char_tmp
		//�����жϵ�ǰ�ַ�����ĸ�Ŀ��˻��Ǻ��ֵĿ���
		char_tmp = charaterbuf;


		//��ĸ
		if(*charaterbuf< 0x80 )
		{
			// ��ȡasc���󵽻���
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
				ascii_font_size = 48;//һ����ĸռ��48���ֽڱ�ʾ:24�У�ÿ��2���ֽ�
				ascii_line_pixel = 12;
				ascii_line_Bytes = 2;
				//BYTEbits = 8;
				LSTBYTEbits = 8;
			}
			else if(TXTstruct->fontSize == 40)
			{
				//byAscChar = byAscChar - 0x20;
				ascii_font_size = 120;//һ����ĸռ��120���ֽ�:40�У�ÿ��3���ֽ�
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


			char_lines 		= ascii_lines;		//����ռ������
			char_line_pixel = ascii_line_pixel;	//����
			char_line_Bytes = ascii_line_Bytes;

			
		}
		//����
		else
		{
			High8bit = *(charaterbuf);		// ȡ��8λ����	
			charaterbuf++;
			Low8bit =*(charaterbuf);		//	ȡ��8λ���� 

			if(High8bit < 0xA1)
			{
				DEBUG_PRINTF;
				SpecialStrProcess(pDspBuf,TXTstruct->fontSize,High8bit,Low8bit);
				DEBUG_PRINTF;
			}
			else
			{
				//(High8bit-161l)*94 : �������*ÿ������94����
				//Low8bit-161		   : ��������λ��,�ں��ֱ������ÿ����λ����0xA0����ʾ������λ�ֱ�ռ��һ���ֽ�
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
		//ÿ���ֺ���chinese_lines������,ÿ�к���chinese_line_pixel�����ص�
		//ÿ�к���chinese_line_Bytes���ֽڣ�ÿ���ֽں���8bits		
		for(row_cnt = 0 ; row_cnt < char_lines ; row_cnt ++)
		{
			for(col_cnt = 0 ; col_cnt < char_line_Bytes ; col_cnt ++)
			{
				//���һ���ֽڵ���Чλ��
				BYTEbits = (col_cnt == char_line_Bytes - 1) ? LSTBYTEbits : 8;
				
				for( i = 0 ; i < BYTEbits ; i ++)
				{
					x = Xpos+j;//i��������ÿ���ֽڵĵ� �� bit��j��������ÿ���ַ�һ���еĵڼ������ص�
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

		
		//�����ַ��ĸ���,��������λ�����Ų��һ����λ��(ÿ����ռ�õ�������+�ּ�϶)
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
		//������������ƫ��һ�����ֵĴ�С
		Xpos += char_line_pixel; 
		//Ȼ��ʼ�������ҷ�char_lines�У�ÿ��w_space�����صļ�϶����ɫ
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
		//�����ƫ��w_space(���ּ�϶)������ǰƫ��w_space + char_line_pixel(���������һ��)
		//��ÿ�������·�������l_space(�м�϶)��ÿ��w_space + char_line_pixel�ı���ɫ
		Xpos += TXTstruct->Wspace;
		Xpos -= (TXTstruct->Wspace + char_line_pixel);
		//������������ƫ��һ�����ֵĴ�С
		Ypos += TXTstruct->fontSize;
		//��ʼ�������·��ı���ɫ
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

		//���������·��ı���ɫ�󣬺�������Ҫ�ص�ƫ�Ƶ���һ�����ֵ���ʼλ��
		Xpos += TXTstruct->Wspace + char_line_pixel;
		Ypos -= TXTstruct->fontSize;

		//ָ��ƫ�Ƶ���һ���ַ�
		charaterbuf++;
		//�ַ�����λ�ù�0���ȴ�����һ���ַ�
		pIndex = 0x00;
		
		
	}
	//�򿪵��ļ��ǵùر�
	fclose(TXTstruct->fpFont);
	fclose(TXTstruct->fpAsc);
	TXTstruct->ch_flag[char_pos_flag] = '\0';
	Xpos += TXTstruct->fontSize + TXTstruct->Wspace;

	//������ʾ����������Ҫ�ĺ������������ߴ�
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




