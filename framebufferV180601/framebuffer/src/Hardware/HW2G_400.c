#include <stdio.h>
#include <sys/io.h>

#include "HW2G_400.h"
#include "config.h"
#include "debug.h"
#include "mtime.h"
#include "../include/conf.h"
#include "Data_pool.h"



#define outp(a, b) outb(b, a)
#define inp(a) inb(a)


static void HW2G400_getBright(uint8_t *BrightA,uint8_t *BrightB)
{
	uint8_t TempData = 0;
	uint8_t _BrightA,_BrightB;
	iopl(3);
	// ��ȡ����A
	outp(0x9FFA,0x02);
	usleep(100);
	TempData =	0x02 << 3;
	outp(0xC000+TempData,0xff);
	usleep(100);
	_BrightA = (255-inp(0xC000+TempData))*31/255;

	usleep(100);

	// ��ȡ����B
	outp(0x9FFA,0x03);
	usleep(100);
	TempData =	0x03 << 3;
	outp(0xC000+TempData,0xff);
	usleep(100);
	_BrightB = (255-inp(0xC000+TempData))*31/255;

	debug_printf("_BrightA = %d,_BrightB = %d\n",_BrightA,_BrightB);

	//������ֵ
	_BrightA = (_BrightA < 1) ? 1 : _BrightA;
	_BrightA = (_BrightA > 31) ? 31 : _BrightA;

	_BrightB = (_BrightB < 1) ? 1 : _BrightB;
	_BrightB = (_BrightB > 31) ? 31 : _BrightB;

	*BrightB = _BrightB;
	*BrightA = _BrightA;
}


int HW2G400_SetScreenStatus(uint8_t status)
{
	uint8_t ScreenStatus = 0;
	uint8_t BrightV;
	float Fbright = 0.0,Sbright = 0.0;
	uint8_t Bright = 0,Bmax = 0,Bmin = 0;
	iopl(3);
	if(status == LED_STATUS_OFF)
	{
		outp(0x9FAC,0);
		outp(0x9FAD,0);
		outp(0x9FAE,0);
	}
	else
	{
		DP_ReadBrightVals(&BrightV);
		outp(0x9FAC,BrightV);
		outp(0x9FAD,BrightV);
		outp(0x9FAE,BrightV);
	}
	return 0;
		
}


void HW2G400_SETLEDbright(uint8_t Bright)
{	
	iopl(3);
	outp(0x9FAC,Bright);
	outp(0x9FAD,Bright);
	outp(0x9FAE,Bright);
}




//Ϊ��ͳһ��������ֵ������ֵ�ڱ���ʱ
void HW2G400_aotoBright(void)
{
	uint8_t BrightA,BrightB;
	uint8_t BrightMAX,BrightMIN,Bmode;
	uint8_t ENVBright;
	uint8_t BrightMode;
	uint8_t ScrStatus;
	char timestr[24];
	uint8_t len = 0;
	uint8_t curtime = 0;
	float Bright = 0.0,Sbright = 0.0;
	

	//�����Ļʱ����״̬����û��Ҫȥ�����Զ�����
	if(DP_GetScreenStatus(&ScrStatus) == LED_STATUS_OFF)
		return;

	//�ֶ�״̬�������������
	DP_GetBrightMode(&BrightMode);
	if(BrightMode == BRIGHT_HAND)
		return;

	DEBUG_PRINTF;

	//���沿�����Զ�����״̬�µĲ���
	//��ȡ�����Ļ��������ȣ�ɨ�������ȷ�Χ��0-31
	HW2G400_getBright(&BrightA,&BrightB);
	//�������ֵ
	DP_SetLSData(BrightA,BrightB);

	
	//��������ֵ������1ʱȡƽ��ֵ������ڰ��춼С��1����Ϊ��������ֵ����
	//����һ��Ĭ��ֵ20
	if(BrightA > 1 && BrightB > 1)
		ENVBright = (BrightA + BrightB) / 2;
	if(BrightA <= 1 && BrightB > 1)
		ENVBright = BrightB;
	if(BrightA > 1 && BrightB <= 1)
		ENVBright = BrightA;
	if(BrightA <= 1 && BrightB <= 1)
	{
		memset(timestr,0,sizeof(timestr));
		get_sys_time(timestr,&len);
		curtime = atoi(timestr+11);
		ENVBright = (curtime > 6 && curtime < 18) ? 20 : 1;
	}

	//���ݹ���ֵ�趨����
	DEBUG_PRINTF;
	HW2G400_SETLEDbright(ENVBright);

	DP_SaveBrightVals(ENVBright);
}






