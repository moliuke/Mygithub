#include "task.h"
#include "../cache.h"
#include <sys/io.h>
#include "../Hardware/Data_pool.h"
#include "../Hardware/HW1G_200.h"

#define outp(a, b) outb(b, a)
#define inp(a) inb(a)


/**
		��ʾ�̣߳����̴߳��������������ѯ��ȡ����ˢ����Ļ
*/
void *pthread_display_task(void *arg)
{
	uint16_t Xoffset = 0,Yoffset = 0;
	uint8_t *readAddr = NULL;
	uint8_t *ReadAddr = NULL;
	uint16_t cardType = TRANSCARD_TXRX;
	CACHEstruct_t *DSPCACHEstruct = DSPcache;
	DP_GetOffset(&Xoffset,&Yoffset);
	fb_frame_Clear(Xoffset,Yoffset);
	DP_GetCardType(&cardType);
	while(1)
	{
		if((DSPCACHEstruct->R_Pos + 1) % 3 == DSPCACHEstruct->W_Pos)
			continue;
		
		DSPCACHEstruct->R_Pos = (DSPCACHEstruct->R_Pos + 1) % 3;
		ReadAddr = DSPCACHEstruct->cache + DSPCACHEstruct->R_Pos * DSPCACHEstruct->CHwidth * DSPCACHEstruct->CHheight * SCREEN_BPP;
		
		//���ɨ������ʾ
		if(cardType == TRANSCARD_200)
			HW2G200_Display(ReadAddr);
		
		//���TXRX������ʾ
		fb_frame_display(Xoffset,Yoffset,ReadAddr,32,1000);

	}
}


