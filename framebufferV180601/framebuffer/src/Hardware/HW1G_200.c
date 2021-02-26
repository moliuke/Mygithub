/**************************************************************************************
说明:(这个-200的显示优点奇葩)
	1、按分段显示，每段256像素宽度。例如:有一块屏幕是320*32，那么要把该屏幕分成两段:
	   第一段:256*32，第二段:64*32,第二段由于有效数据只有64的长度，实际上在网pci104刷数据的
	   时候也是按照256长度来刷的
	   
	2、每段刷数据的时候，不是按照以往RGBA、RGBA、RGBA....每3个或者4个字节填充一个像素数据
	   而是按照256个R，256个G，256个B(-200只有RGB,没有A)

	3、当按照上述方式刷完一段的数据后，还要在
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
//下面的部分是针对悬臂
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
	//开始标记
	outp(0x9FF3,0xAC);
	//分段显示，每段256
	for(ii = 0 ; ii < Sement ; ii++)
		HW2G200_SementDisplay(ii);
	// 结束标记
	outp(0x9FF3,0x53);

	
}



/////////////////////////////////////////////////////////////////
//下面的部分是针对门架
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
	//开始标记
	outp(0x9FF3,0xAC);
	//分段显示，每段256
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
	// 结束标记
	outp(0x9FF3,0x53);

	
}
#else

//超过256宽度的都视作门架，此函数，只计算高度为32，宽度512以内的门架。门架分成两段来计算
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

	//段与段之间间隔256*32 + 16*256,不知道神情况
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
	//开始标记
	outp(0x9FF3,0xAC);
	//分段显示，每段256
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
	// 结束标记
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


