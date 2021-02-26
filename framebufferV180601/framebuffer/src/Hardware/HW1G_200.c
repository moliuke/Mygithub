/**************************************************************************************
˵��:(���-200����ʾ�ŵ�����)
	1�����ֶ���ʾ��ÿ��256���ؿ�ȡ�����:��һ����Ļ��320*32����ôҪ�Ѹ���Ļ�ֳ�����:
	   ��һ��:256*32���ڶ���:64*32,�ڶ���������Ч����ֻ��64�ĳ��ȣ�ʵ��������pci104ˢ���ݵ�
	   ʱ��Ҳ�ǰ���256������ˢ��
	   
	2��ÿ��ˢ���ݵ�ʱ�򣬲��ǰ�������RGBA��RGBA��RGBA....ÿ3������4���ֽ����һ����������
	   ���ǰ���256��R��256��G��256��B(-200ֻ��RGB,û��A)

	3��������������ʽˢ��һ�ε����ݺ󣬻�Ҫ��
***************************************************************************************/



#include <sys/io.h>

#include "HW1G_200.h"
#include "Data_pool.h"

#define outp(a, b) outb(b, a)
#define inp(a) inb(a)

static pthread_once_t init_create = PTHREAD_ONCE_INIT; 

static uint8_t *HW2G200BUF = NULL;
static unsigned int SectionDataAddress = 0x6000;
static uint16_t SetctionCNT = 0;
static uint32_t ByteTotal = 0;
static uint32_t section = 0;
static uint32_t Swidth,Sheight;
static uint8_t Sement = 0;
static void Memallocate(void)
{	
	uint32_t MallocSize;
	DP_GetScreenSize(&Swidth,&Sheight);

	if(Swidth <= 256) Sement = 1;
	else if(Swidth <= 512) Sement = 2;
	else if(Swidth <= 768) Sement = 3;
	else{};
	
	MallocSize = Sement * 256 * Sheight * 5;

	HW2G200BUF = (uint8_t *)malloc(MallocSize);
	if(HW2G200BUF == NULL)
	{
		perror("HW2G200BUF allocate fail");
		exit(1);
	}

	memset(HW2G200BUF,0x00,MallocSize);
	
	SetctionCNT = Sement * 4;//(ByteTotal % 8192 == 0) ? (ByteTotal / 8192) : (ByteTotal / 8192 + 1);
}

static inline void HW2G200_Memallocate(void)
{
	pthread_once(&init_create,Memallocate);
}

/////////////////////////////////////////////////////////////////
//����Ĳ������������
//////////////////////////////////////////////////////////////////
#if 0
static int HW2G200_SementDisplay(uint8_t Sement)
{
	uint8_t Section_start = Sement * 5;
	uint8_t ii = 0;
	uint32_t aa = 0;
	uint32_t bytecount = 0,S2count = 0;
	for(ii = Section_start ; ii < Section_start + 5 ; ii++)
	{
		if(ii < Section_start + 4)
		{
			outp(0x9FFC,ii & 0x0f);
			for(aa = 0 ; aa < 8192 ;aa ++)
			{
				outp(SectionDataAddress + aa, HW2G200BUF[Sement*(256 * Sheight * 3 + 4096) + bytecount]);
				bytecount++;
			}
		}
		else		
		{
			outp(0x9FFC,ii & 0x0f);
			for(aa = 0 ; aa < 4096 ;aa ++)
			{
				outp(SectionDataAddress + aa, 0x00);
				//S2count++;
			}
		}
	}
}
#else
static int HW2G200_SementDisplay(uint8_t Sement)
{
	uint8_t Section_start = Sement * 5;
	uint8_t ii = 0;
	uint32_t aa = 0;
	uint32_t bytecount = 0,S2count = 0;
	for(ii = Section_start ; ii < Section_start + 5 ; ii++)
	{
		if(ii < Section_start + 4)
		{
			outp(0x9FFC,ii & 0x0f);
			for(aa = 0 ; aa < 8192 ;aa ++)
			{
				outp(SectionDataAddress + aa, HW2G200BUF[Sement*(256 * Sheight * 3 + 4096) + bytecount]);
				bytecount++;
			}
		}
		else		
		{
			outp(0x9FFC,ii & 0x0f);
			for(aa = 0 ; aa < 4096 ;aa ++)
			{
				outp(SectionDataAddress + aa, 0x00);
				//S2count++;
			}
		}
	}
}
#endif


void HW2G200_Arm_CacheTranslate(uint8_t *intput,uint8_t *output)
{
	uint8_t S = 0;
	uint16_t h,w;
	uint16_t LSW = Swidth - (Sement - 1) * 256;//last sement width;
	
	for(h = 0 ; h < Sheight ; h ++)
	{
		for(S = 0 ; S < Sement ; S++)
		{
			if(S == Sement - 1)
			{
				for(w = 0 ; w < LSW ; w ++)
				{
					output[S * 256 * Sheight * 3 + (h * 3 + 0) * 256 + w] = intput[S * 256 * 4 + (h * Swidth + w) * SCREEN_BPP + 2];
					output[S * 256 * Sheight * 3 + (h * 3 + 1) * 256 + w] = intput[S * 256 * 4 + (h * Swidth + w) * SCREEN_BPP + 1];
					output[S * 256 * Sheight * 3 + (h * 3 + 2) * 256 + w] = intput[S * 256 * 4 + (h * Swidth + w) * SCREEN_BPP + 0];
					
				}
			}
			else
			{
				for(w = 0 ; w < 256 ; w ++)
				{
					output[S * 256 * Sheight * 3 + (h * 3 + 0) * 256 + w] = intput[S * 256 * 4 + (h * Swidth + w) * SCREEN_BPP + 2];
					output[S * 256 * Sheight * 3 + (h * 3 + 1) * 256 + w] = intput[S * 256 * 4 + (h * Swidth + w) * SCREEN_BPP + 1];
					output[S * 256 * Sheight * 3 + (h * 3 + 2) * 256 + w] = intput[S * 256 * 4 + (h * Swidth + w) * SCREEN_BPP + 0];
				}
			}
			
		}
	}
}



int HW2G200_Arm_Display(uint8_t *DataAddr)
{

	int flag = 0;
	int BreakFlag = 0;
	uint16_t h,w;
	HW2G200_Memallocate();
	HW2G200_Arm_CacheTranslate(DataAddr,HW2G200BUF);
	
	uint32_t bytecount = 0;
	uint32_t S2count = 0;
	uint32_t aa,ii;

	iopl(3);
	//��ʼ���
	outp(0x9FF3,0xAC);
	//�ֶ���ʾ��ÿ��256
	for(ii = 0 ; ii < Sement ; ii++)
		HW2G200_SementDisplay(ii);
	// �������
	outp(0x9FF3,0x53);

	
}



/////////////////////////////////////////////////////////////////
//����Ĳ���������ż�
//////////////////////////////////////////////////////////////////




#if 0
#define _0XFF	0xff
int HW2G200_Display(uint8_t *DataAddr)
{

	int flag = 0;
	int BreakFlag = 0;
	uint16_t h,w;
	HW2G200_Memallocate();
	//HW2G200_CacheTranslate(DataAddr,HW2G200BUF);
	
	uint32_t bytecount = 0;
	uint32_t S2count = 0;
	uint32_t aa,ii;

	iopl(3);
	//��ʼ���
	outp(0x9FF3,0xAC);
	//�ֶ���ʾ��ÿ��256
	for(ii = 0 ; ii < 7 ; ii++)
	{
		outp(0x9FFC,ii & 0x0f);

		if(ii < 6)
			for(aa = 0 ; aa < 8192 ;aa ++)
			{
				outp(SectionDataAddress + aa, _0XFF);
				bytecount++;
			}
		else
			for(aa = 0 ; aa < 7168 ;aa ++)
			{
				outp(SectionDataAddress + aa, _0XFF);
				bytecount++;
			}
	}
	// �������
	outp(0x9FF3,0x53);

	
}
#else

//����256��ȵĶ������żܣ��˺�����ֻ����߶�Ϊ32�����512���ڵ��żܡ��żֳܷ�����������
void HW2G200_Door_CacheTranslate(uint8_t *intput,uint8_t *output)
{
	uint8_t S = 0;
	uint16_t h,w;

	uint16_t LSW = Swidth - (Sement - 1) * 256;//last sement width;

	for(h = 0 ; h < Sheight ; h ++)
	{
		for(w = 0 ; w < 256 ; w++)
		{
			output[(h * 3 + 0) * 256 + w] = intput[(h * Swidth + w) * SCREEN_BPP + 2];
			output[(h * 3 + 1) * 256 + w] = intput[(h * Swidth + w) * SCREEN_BPP + 1];
			output[(h * 3 + 2) * 256 + w] = intput[(h * Swidth + w) * SCREEN_BPP + 0];
		}
	}

	//�����֮����256*32 + 16*256,��֪�������
	for(h = 0 ; h < Sheight + 16 ; h ++)
	{
		for(w = 0 ; w < 256 ; w++)
		{
			output[256 * Sheight * 3 + h * 256 + w] = 0x00;
		}
	}
	

	
	for(h = 0 ; h < Sheight ; h ++)
	{
		for(w = 0 ; w < LSW ; w++)
		{
			output[256 * Sheight * 3 + 256 * (Sheight + 16) + (h * 3 + 0) * 256 + w] = intput[256 * 4 + (h * Swidth + w) * SCREEN_BPP + 2];
			output[256 * Sheight * 3 + 256 * (Sheight + 16) + (h * 3 + 1) * 256 + w] = intput[256 * 4 + (h * Swidth + w) * SCREEN_BPP + 1];
			output[256 * Sheight * 3 + 256 * (Sheight + 16) + (h * 3 + 2) * 256 + w] = intput[256 * 4 + (h * Swidth + w) * SCREEN_BPP + 0];
		}
	}

	ByteTotal = 256 * 32 * 3 + 256 * 32 + 256 * 16 + 256 * 32 * 3;
	section = (ByteTotal % 8192 == 0) ? (ByteTotal / 8192) : (ByteTotal / 8192 + 1);
	
}



int HW2G200_Door_Display(uint8_t *DataAddr)
{

	int BreakFlag = 0;
	uint16_t h,w;
	HW2G200_Memallocate();
	HW2G200_Door_CacheTranslate(DataAddr,HW2G200BUF);
	
	uint32_t bytecount = 0;
	uint32_t S2count = 0;
	uint32_t aa,ii;
	int flag = 0;

	//exit(1);
	iopl(3);
	//��ʼ���
	outp(0x9FF3,0xAC);
	//�ֶ���ʾ��ÿ��256
	for(ii = 0 ; ii < section ; ii++)
	{
		outp(0x9FFC,ii & 0x0f);
		for(aa = 0 ; aa < 8192 ;aa ++)
		{
			outp(SectionDataAddress + aa, HW2G200BUF[bytecount]);
			bytecount++;
			if(bytecount > ByteTotal)
			{
				flag = 1;
				break;
			}
		}
		if(flag)
			break;

	}
	// �������
	outp(0x9FF3,0x53);

	
}


#endif


int HW2G200_Display(uint8_t *DataAddr)
{
	uint16_t scrType = SCRTYPE_ARM;
	DP_GetScrType(&scrType);
	if(scrType == SCRTYPE_ARM)
		HW2G200_Arm_Display(DataAddr);
	else
		HW2G200_Door_Display(DataAddr);
}


