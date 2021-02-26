#include "general.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "../../../Hardware/Data_pool.h"
#include "Dev_framebuffer.h"
#include "bitmap.h"
#include "character.h"
#include "mtime.h"
#include "../../../module/mtimer.h"
#include "../SWR_init.h"


#define LINE_RED 2
#define LINE_GREEN 1

#define LCDarea		screen.frameBuffer

#define TXTSIZE			48 * 48 * 5 * 4

#define YPOS			260
#define XPOS			0
#define YBASE_ADDR		YPOS * 640 * 4     //260*640*4
#define XBASE_ADDR 		XPOS               //0

#define NORMAL			0x31
#define ABNORMAL		0x30

static pthread_t tid_sw_task;
static char txtparse[TXTSIZE];

static stimer_t hb_timer;


//���ݸ�����ɫ���ֺš�������ʾ��������
static void DisplayStrMsg(char *str,uint8_t color,uint8_t fontSize,uint16_t x,uint16_t y)
{
	Pcolor_t Bcolor,Fcolor;
	TXTstruct_t TXTstruct;

	memset(txtparse,0,TXTSIZE);
	memset(&Bcolor,0,sizeof(Bcolor));	
	memset(&Fcolor,0,sizeof(Fcolor));	
	Fcolor.red		= (color == 'r') ? 0xff : 0x00;
	Fcolor.green	= (color == 'g') ? 0xff : 0x00;
	Fcolor.blue 	= (color == 'b') ? 0xff : 0x00;
	
	memset(&TXTstruct,0,sizeof(TXTstruct));
	TXT_INITstruct(&TXTstruct,str,strlen(str),
		's',fontSize,&Bcolor,&Fcolor,0,0,txtparse,TXTSIZE,SCREEN_BPP);
	if(TXTstruct.TXT_decoder(&TXTstruct) < 0)
	{
		DEBUG_PRINTF;
		return;
	}

	int w = 0 , h = 0 ; 
	for(h = 0 ; h < TXTstruct.ctheight; h++)
	{
		for(w = 0 ; w < TXTstruct.ctwidth ; w++)
		{
			screen.frameBuffer[(y + h) * 640 * 4 + ( x + w) * 4 + 0] = txtparse[h * TXTstruct.chwidth * 4 + w * 4 + 0];
			screen.frameBuffer[(y + h) * 640 * 4 + (x + w) * 4 + 1] = txtparse[h * TXTstruct.chwidth * 4 + w * 4 + 1];
			screen.frameBuffer[(y + h) * 640 * 4 + (x + w) * 4 + 2] = txtparse[h * TXTstruct.chwidth * 4 + w * 4 + 2];

		}
	}
}


//��ʾ״̬��Ϣ
void DisplayStatus(void)
{
	DisplayStrMsg("��������",'g',16,20,262);
	DisplayStrMsg("��ʾ����",'g',16,126,262);
	DisplayStrMsg("����ģʽ",'g',16,232,262);
	DisplayStrMsg("�ſ���",'g',16,338,262);	
	DisplayStrMsg("������",'g',16,444,262);
	DisplayStrMsg("��������",'g',16,550,262);
	
	DisplayStrMsg("���ػ��汾:",'g',16,4,307);  //
	DisplayStrMsg("TX�汾:",'g',16,4,327);
	DisplayStrMsg("RX�汾:",'g',16,320,327);
	DisplayStrMsg("��ǰʱ��:",'g',16,4,347);
	DisplayStrMsg("IP:",'g',16,4,367);
	DisplayStrMsg("�˿ں�:",'g',16,150,367);
	//add by mo 2020/9/4
	DisplayStrMsg("����:",'g',16,260,367);
	DisplayStrMsg("��������:",'g',16,434,367);
	
	DisplayStrMsg("����",'g',16,18,392);
	DisplayStrMsg("�¶�",'g',16,18,412);
	DisplayStrMsg("��ѹ",'g',16,18,432);
	DisplayStrMsg("����0",'g',16,85,392);
	DisplayStrMsg("����1",'g',16,156,392);
	DisplayStrMsg("����2",'g',16,227,392);
	DisplayStrMsg("����3",'g',16,298,392);
	DisplayStrMsg("����4",'g',16,369,392);
	DisplayStrMsg("����5",'g',16,440,392);
	DisplayStrMsg("����6",'g',16,511,392);
	DisplayStrMsg("����7",'g',16,582,392);

}
void DisplayDevStatus(void)
{
	char buffer[24];
	uint32_t width,height;
	uint8_t Status,Vals;
	
	memset(buffer,0,sizeof(buffer));
	DP_GetSysDataAndStatus(PID_LIGHT_SENSITIVE,&Status,&Vals); //������������״̬
	sprintf(buffer,"%d",Vals);
	DisplayStrMsg(buffer,'b',16,41,282);
	
	DP_ReadBrightVals(&Vals); //����
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",Vals);
	DisplayStrMsg(buffer,'b',16,147,282);
	
	DP_GetBrightMode(&Vals);//����ģʽ
	if(Vals == 0x31)
	{
		DisplayStrMsg("�Զ�",'b',16,249,282);
	}
	else if(Vals == 0x30)
	{
		DisplayStrMsg("�ֶ�",'b',16,249,282);
	}
	
	//�ſ���״̬���
	DP_GetSysDataAndStatus(PID_DOOR,&Status,&Vals);
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",Vals);
	DisplayStrMsg(buffer,'b',16,363,282);

	//����������״̬�����ڷ�Χ��˵��������
	DP_GetSysDataAndStatus(PID_THANDER,&Status,&Vals);
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",Vals);
	DisplayStrMsg(buffer,'b',16,469,282);

	//��������
	DP_GetScreenSize(&width, &height);
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d*%d",width,height);
	DisplayStrMsg(buffer,'b',16,563,282);

	//�汾
	FILE *fp = NULL;
	char Vapp[64];
	memset(Vapp,0,sizeof(Vapp));
	if(!access("/home/LEDscr/version/app.v",F_OK))
	{
		fp = fopen("/home/LEDscr/version/app.v","r");
		fread(Vapp,1,1024,fp);
		fclose(fp);
	}
	DisplayStrMsg(Vapp,'b',16,94,307);	
	memset(Vapp,0,sizeof(Vapp));
	DP_Get_TXRXVersion(Vapp);
	buffer[0] = 'V';
	buffer[1] = (Vapp[0] >> 4) + 0x30;
	buffer[2] = '.';
	buffer[3] = (Vapp[0] & 0x0f) + 0x30;
	buffer[4] = '.';
	buffer[5] = (Vapp[1] >> 4) + 0x30;
	buffer[6] = '.';
	buffer[7] = (Vapp[1] & 0x0f) + 0x30;
	DisplayStrMsg(buffer,'b',16,64,327);

	buffer[0] = 'V';
	buffer[1] = (Vapp[2] >> 4) + 0x30;
	buffer[2] = '.';
	buffer[3] = (Vapp[2] & 0x0f) + 0x30;
	buffer[4] = '.';
	buffer[5] = (Vapp[3] >> 4) + 0x30;
	buffer[6] = '.';
	buffer[7] = (Vapp[3] & 0x0f) + 0x30;
	DisplayStrMsg(buffer,'b',16,380,327);	

	

	
	//ʱ��	
	char timestr[24];
	uint8_t Len;
	get_sys_time(timestr,&Len);
	memset(buffer,0,sizeof(buffer));
	memcpy(buffer,timestr,Len-3);
	//printf("timestr = %s\n",buffer);
	DisplayStrMsg(buffer,'b',16,80,347);
	//������Ϣ
	uint8_t ip[24];
	uint32_t port;
	uint8_t netmask[24];
	uint8_t gateway[24];
	memset(ip,0,sizeof(ip));
	memset(netmask,0,sizeof(netmask));
	memset(gateway,0,sizeof(gateway));
	DP_GetNetArg(ip,&port,netmask,gateway);
	DisplayStrMsg(ip,'b',16,32,367);
	memset(buffer,0,sizeof(buffer));
	sprintf(buffer,"%d",port);
	DisplayStrMsg(buffer,'b',16,210,367);
	DisplayStrMsg(gateway,'b',16,304,367);
	DisplayStrMsg(netmask,'b',16,510,367);
	uint32_t num;
	DP_GetBoxSize(&width, &height, &num);
	//�����¶�
	int i = 0;
	uint8_t tmp0,tmp1,tmp2,tmp3;
	uint8_t volt = RX_PID_VOLT1;
	DP_GetRXCardData(RX_PID_VOLT1,i,&tmp0);
	DP_GetRXCardData(RX_PID_VOLT2,i,&tmp1);
	DP_GetRXCardData(RX_PID_VOLT3,i,&tmp2);
	DP_GetRXCardData(RX_PID_VOLT4,i,&tmp3);
	if(tmp0 != 0)
		volt = RX_PID_VOLT1;
	else if(tmp1 != 0)
		volt = RX_PID_VOLT2;
	else if(tmp2 != 0)
		volt = RX_PID_VOLT3;
	else if(tmp3 != 0)
		volt =RX_PID_VOLT4;
	
	for(i = 0; i < num; i++)
	{
		switch(i)
		{
			case 0:
				//�¶�
				DP_GetRXCardData(RX_PID_TEMP,i,&Vals);	
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,94,412);
				
				//�����ѹ1��ֵ
				DP_GetRXCardData(volt,i,&Vals);
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,94,432);				
			break;
			case 1:
				DP_GetRXCardData(RX_PID_TEMP,i,&Vals);	
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,165,412);
				
				DP_GetRXCardData(volt,i,&Vals);
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,165,432);	
			break;

			case 2:
				DP_GetRXCardData(RX_PID_TEMP,i,&Vals);	
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,236,412);	
				
				DP_GetRXCardData(volt,i,&Vals);
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,236,432);				
			break;
			
			case 3:
				DP_GetRXCardData(RX_PID_TEMP,i,&Vals);	
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,307,412);

				DP_GetRXCardData(volt,i,&Vals);
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,307,432);	
			break;
			
			case 4:
				DP_GetRXCardData(RX_PID_TEMP,i,&Vals);	
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,378,412);
				DP_GetRXCardData(volt,i,&Vals);
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,378,432);	
			break;
			
			case 5:
				DP_GetRXCardData(RX_PID_TEMP,i,&Vals);	
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,449,412);
				DP_GetRXCardData(volt,i,&Vals);
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,449,432);
			break;

			case 6:
				DP_GetRXCardData(RX_PID_TEMP,i,&Vals);	
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,520,412);			
				DP_GetRXCardData(volt,i,&Vals);
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,520,432);			
			break;

			case 7:
				DP_GetRXCardData(RX_PID_TEMP,i,&Vals);	
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,591,412);			
				DP_GetRXCardData(volt,i,&Vals);
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",Vals);
				DisplayStrMsg(buffer,'b',16,591,432);			
			break;
		}

	}

}


//����ʾ���
void DrawTable(uint16_t xpos, uint16_t ypos, uint8_t row, uint8_t column ,uint8_t color, uint8_t line_weight)
{
	int i = 0, j = 0, k = 0;
	int L = 0;
	int TableWidth = 0;
//����
	for(L = 0; L < line_weight; L++)
	{
		for(i = 0; i < 640; i++)
		{
			for(k = 0; k < row+1 ;k++)
			{
				LCDarea[(ypos*640 + k*20*640 + xpos + L*640 + i ) * 4 + color] = 0xff;
			}			
		}
	}
//����
	TableWidth = 640/column;

	for(j = 0 ; j < 20*row ; j++)
	{
		for(L = 0 ; L < line_weight ; L++)
		{
			for(k = 0; k < column+1; k++)
			{
				LCDarea[(ypos*640 +  k*TableWidth + j*640 + L)*4 + color] = 0xff;
			}
		}
	}
}


//�����ʾ���ڵ���ʾ����
static void ClearLCDarea(void)
{
	int contentAreaWidth = 396;
	int contentAreaHeight = 196;
	int H = 160 + 4,W = 0 + 4;

	for(H = 0 ; H < 196 ; H++)
	{
		memset(LCDarea + (264 + H) * 640 * 4 + 4 * 4 ,0,396 * 4);
	}
}
void ClearScreen(uint16_t xpos, uint16_t ypos)
{
	int H = 0, W = 0;
	for(H = ypos; H < 640 ; H++)
	{
		memset(LCDarea +  H * 640 * 4 ,0,640 * 4);
	}
}

//�߳�ѭ��
void *pthread_sw_task(void *arg)
{
	
	//����
	uint32_t width;
	uint32_t height;
	uint8_t count = 0;
	DP_GetScreenSize(&width, &height);
	if(height > 220 || height < 0)
		exit(0);
	ClearScreen(0,0);
	//�������
	DrawTable(0, 260, 2, 6 ,LINE_RED ,2);
	DrawTable(0, 305, 4, 1 ,LINE_RED ,2);
	DrawTable(0, 390, 3, 9 ,LINE_RED ,2);
	//���ע������
	DisplayStatus();

	
	while(1)
	{
		//ClearLCDarea();
		

		//��Ч��ʵʱ����
		count ++;
		if(count == 4)
		{
			ClearScreen(0,260);
			//�������
			DrawTable(0, 260, 2, 6 ,LINE_RED ,2);
			DrawTable(0, 305, 4, 1 ,LINE_RED ,2);
			DrawTable(0, 390, 3, 9 ,LINE_RED ,2);
			//���ע������
			DisplayStatus();
			count = 0;
		}
		usleep(1000*100);
		DisplayDevStatus();
		sleep(30);
	}

}



int SW_pthread_create(void)
{
	int ret = -1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//�����߳�����Ϊ����״̬
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	
	ret = pthread_create(&tid_sw_task,&attr,pthread_sw_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
	return 0;
}

