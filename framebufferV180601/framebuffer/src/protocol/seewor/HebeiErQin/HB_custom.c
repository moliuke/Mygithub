#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "../../../Hardware/Data_pool.h"
#include "HB_custom.h"
#include "Dev_framebuffer.h"
#include "bitmap.h"
#include "character.h"
#include "mtime.h"
#include "../../../module/mtimer.h"
#include "../SWR_init.h"


#define LCDarea		screen.frameBuffer

#define CONTENTSIZE		300 * 300 * 3
#define BUTTONSIZE		100 * 50 * 3
#define TXTSIZE			48 * 48 * 5 * 4


#define YPOS			260
#define XPOS			0
#define YBASE_ADDR		YPOS * 640 * 4
#define XBASE_ADDR 		XPOS

#define BUTTON_BMP_UP		image_dir"/sys/anjian1.bmp"
#define BUTTON_BMP_DOWN		image_dir"/sys/anjian2.bmp"


#define NORMAL			0x31
#define ABNORMAL		0x30

static pthread_t tid_hb_task;
static uint8_t *contentBuffer = NULL;
static uint8_t *buttontBuffer = NULL;
static int XBaseAddr = XBASE_ADDR;
static int YBaseAddr = YBASE_ADDR;
static int CharBaseAddr = XBASE_ADDR + 20 * 640 * 4;
static int time_display_flag = 0;
static int statusmsg_display_flag = 1;
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
			screen.frameBuffer[YBASE_ADDR + (y + h) * 640 * 4 + (XBASE_ADDR + x + w) * 4 + 0] = txtparse[h * TXTstruct.chwidth * 4 + w * 4 + 0];
			screen.frameBuffer[YBASE_ADDR + (y + h) * 640 * 4 + (XBASE_ADDR + x + w) * 4 + 1] = txtparse[h * TXTstruct.chwidth * 4 + w * 4 + 1];
			screen.frameBuffer[YBASE_ADDR + (y + h) * 640 * 4 + (XBASE_ADDR + x + w) * 4 + 2] = txtparse[h * TXTstruct.chwidth * 4 + w * 4 + 2];
		}
	}

	
}


//��ʾ״̬��Ϣ
static void DisplayDevStatus(void)
{
	uint8_t PowerStatus = 0x31;	//��Դ״̬
	uint8_t CommutStatus = 0x31; //ͨ��״̬
	uint8_t LightStatus = 0x31,LightVals = 0; //����״̬
	uint8_t ThanderStatus = 0x31,ThanderVals = 0; //����״̬
	uint8_t SystemStatus = 0x31; //ϵͳ״̬
	
	DisplayStrMsg(statusmsg,'g',24,90,25);
	DisplayStrMsg(commuStatus,'g',24,20,74);
	DisplayStrMsg("����",'g',24,80,74);
	
	DisplayStrMsg(powerStatus,'g',24,190,74);
	DisplayStrMsg("����",'g',24,250,74);

	
	DisplayStrMsg(lightStatus,'g',24,20,113);
	DP_GetSysDataAndStatus(PID_LIGHT_SENSITIVE,&LightStatus,&LightVals);
	if(LightStatus == NORMAL)
		DisplayStrMsg("����",'g',24,80,113);
	else
		DisplayStrMsg("�쳣",'r',24,80,113);


	DisplayStrMsg(thandStatus,'g',24,190,113);
	DP_GetSysDataAndStatus(PID_THANDER,&ThanderStatus,&ThanderVals);
	if(ThanderStatus == NORMAL)
		DisplayStrMsg("����",'g',24,250,113);
	else
		DisplayStrMsg("�쳣",'r',24,250,113);

	DisplayStrMsg(systemStatus,'g',24,20,152);
	if(PowerStatus == ABNORMAL || CommutStatus == ABNORMAL || 
	   LightStatus == ABNORMAL || ThanderStatus == ABNORMAL)
		DisplayStrMsg("�쳣",'r',24,80,152);
	else
		DisplayStrMsg("����",'g',24,80,152);
	
}



//ÿ��5����ˢ��һ��ʱ��,ͬʱ60sˢ��һ���豸״̬
static char curTime[6] = "10:30";
static int reflsh_devstatus_time = 0;
static void *hb_timer_action(void *arg)
{
	
	TXTstruct_t TXTstruct;
	char timestr[24];
	uint8_t Len;
	Pcolor_t Bcolor,Fcolor;

	//�豸״̬ÿ60s����һ��
	reflsh_devstatus_time += 5;
	if(reflsh_devstatus_time == 60)
	{
		reflsh_devstatus_time = 0;
		if(statusmsg_display_flag)
			DisplayDevStatus();
	}
	
	get_sys_time(timestr,&Len);
	//printf("timestr = %s\n",timestr);
	if(strncmp(curTime,timestr + 11,5) == 0)
		return NULL;
	
	memcpy(curTime,timestr + 11,5);
	curTime[5] = '\0';

	if(time_display_flag)
		DisplayStrMsg(curTime,'g',48,100,100);
	
}

void hb_timer_init(void)
{
	hb_timer.ref_vals = 5;
	hb_timer.counter  = 0;
	hb_timer.id       = 6;
	hb_timer.function = hb_timer_action;

	DEBUG_PRINTF;
	mtimer_register(&hb_timer);
}


void cacheMalloc(void)
{
	contentBuffer = (uint8_t *)malloc(CONTENTSIZE);
	if(contentBuffer == NULL)
	{
		while(1)
		{
			perror("sorry,no enough memory for content!\n");
			sleep(1);
		}
	}
	buttontBuffer = (uint8_t *)malloc(BUTTONSIZE);
	if(buttontBuffer == NULL)
	{
		while(1)
		{
			perror("sorry,no enough memory for button!\n");
			sleep(1);
		}
	}
	
	memset(contentBuffer,0,sizeof(contentBuffer));
	memset(buttontBuffer,0,sizeof(buttontBuffer));
	
}



//����ʾ���
void DrawBox(uint32_t Boxwidth,uint32_t Boxheight)
{
	int L = 0,C = 0;
	int i = 0,j = 0;
	int WbaseAddr_UP = YBaseAddr;
	int WbaseAddr_DOWN = YBaseAddr + 200 * 640 * 4;
	for(L = 0 ; L < 4 ; L++)
	{
		for(i = 0 ; i < 640 ; i++)
		{
			//screen.frameBuffer[200 * 640 * 4 + 4*i + 0] = 0xff;
			LCDarea[WbaseAddr_UP + L * 640 * 4 + 4*i + 1] = 0xff;
			LCDarea[WbaseAddr_DOWN + L * 640 * 4 + 4*i + 1] = 0xff;
		}
		
	}
	
	for(j = 0 ; j < 200 ; j++)
	{
		for(C = 0 ; C < 4 ; C++)
		{
			LCDarea[WbaseAddr_UP + j * 640 * 4 + C*4 + 1] = 0xff;
			LCDarea[WbaseAddr_UP + 400 * 4 + j * 640 * 4 + C*4 + 1] = 0xff;
			LCDarea[WbaseAddr_UP + 636 * 4 + j * 640 * 4 + C*4 + 1] = 0xff;
		}
	}

}



static Button_t BTcurtime,BTstatus;
static void _DisplayButton(Button_t *button,uint16_t CX,uint16_t CY)
{
	IMGstruct_t IMGstruct;
	memset(&IMGstruct,0,sizeof(IMGstruct));
	memcpy(IMGstruct.filename,button->upbmp,strlen(button->upbmp));
	IMG_INITstruct(&IMGstruct,CX,CY,640,200 + YPOS,screen.frameBuffer);
	if(IMGstruct.IMG_decoder(&IMGstruct) < 0)
	{
		debug_printf("fail to decode the bmp\n");
		return;
	}
}


//��ʾ����ͼ�꼰����˵������
static void DisplayButton(void)
{
	//����������ͼ����ʱ��ʹ��BTcurtime��ͼ��
	_DisplayButton(&BTcurtime,440,280);
	DisplayStrMsg("��ǰʱ��",'g',24,500,26);
	_DisplayButton(&BTcurtime,440,340);
	DisplayStrMsg("״̬��Ϣ",'g',24,500,90);
	_DisplayButton(&BTcurtime,440,400);
}


//����ͼ���л�
static void ButtonStatusSwitch(Button_t *button,uint16_t CX,uint16_t CY)
{
	IMGstruct_t IMGstruct;
	memset(&IMGstruct,0,sizeof(IMGstruct));
	if(button->state == BUTTON_UP)
		memcpy(IMGstruct.filename,button->upbmp,strlen(button->upbmp));

	if(button->state == BUTTON_DOWN)
		memcpy(IMGstruct.filename,button->downbmp,strlen(button->downbmp));

	IMG_INITstruct(&IMGstruct,CX,CY,640,200 + YPOS,screen.frameBuffer);
	if(IMGstruct.IMG_decoder(&IMGstruct) < 0)
	{
		debug_printf("fail to decode the bmp\n");
		return;
	}
}





/*=====================================================================================*/
//��ʼ������ͼ���Լ����������귶Χ��Ϊ�˼򻯹�������������㷽������:
//1�����ȴ������ݵ��������ݸ�ʽ����(����)
//	0x81 0xa 0x3b 0xa 0x6a
// x = 0xa << 8 | 0x3b , y = 0xa << 8 | 0x6a
//���ȡ5�����ݵ�ƽ��ֵ������5�����ж������ȡ���������ݵ�ƽ��ֵ
static void _ButtonInit(Button_t *button,uint16_t cx_l,uint16_t cx_r,uint16_t cy_d,uint16_t cy_u)
{
	//��ʼ������ͼ��
	memset(button->upbmp,0,sizeof(button->upbmp));
	memset(button->downbmp,0,sizeof(button->downbmp));
	memcpy(button->upbmp,BUTTON_BMP_UP,strlen(BUTTON_BMP_UP));
	memcpy(button->downbmp,BUTTON_BMP_DOWN,strlen(BUTTON_BMP_DOWN));

	//��ʼ�����������������귶Χ
	button->CX_l = cx_l;
	button->CX_r = cx_r;
	button->CY_d = cy_d;
	button->CY_u = cy_u;
	button->state = BUTTON_UP;
}
static void ButtonInit(void)
{
	_ButtonInit(&BTcurtime,2350,2820,1650,2070);
	_ButtonInit(&BTstatus,2350,2820,1130,1570);
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


//�Ӵ��ڽ��յĴ󳤴����������ȡ5�����ݵ�ƽ��ֵ��Ϊ���������µ�����
static void GetCoor(uint8_t *RXbuf,uint16_t Len,uint16_t *CX,uint16_t *CY)
{
	
	uint8_t times = 0;
	uint16_t DataLen = Len;
	uint16_t sumcx = 0,sumcy = 0;
	uint8_t *ptr = RXbuf;
	while(DataLen)
	{
		//Ѱ��0x81����0x80��ͷ��������
		if(ptr[0] != 0x81 && ptr[0] != 0x80)
		{
			ptr += 1;
			DataLen -= 1;
			continue;
		}
	#if 0
		int j = 0;
		for(j = 0 ; j < 5 ; j++)
			printf("0x%x ",ptr[j]);
		printf("\n");
	#endif
		sumcx += (ptr[1] << 8 | ptr[2]);
		sumcy += (ptr[3] << 8 | ptr[4]);
		ptr += 5;
		DataLen -= 5;
		
		times += 1;
		if(times == 5)
			break;
	}
	//printf("sumcx = %d,times = %d\n",sumcx,times);

	if(times != 0)
	{
		*CX = sumcx / times;
		*CY = sumcy / times;
	}
	else
	{
		*CX = 0;
		*CY = 0;
	}
}



//�ж��ض������Ƿ񱻰���
bool isButtonDown(Button_t *button,uint16_t CX,uint16_t CY)
{
	return ((CX >= button->CX_l && CX <= button->CX_r) && (CY >= button->CY_d && CY <= button->CY_u));
}


//�߳�ѭ��
void *pthread_hb_task(void *arg)
{
	
	uint16_t Len;
	uint8_t RXbuf[1024];
	struct timeval time;
	fd_set fs_read;
	uint16_t CX,CY;
	int fs_sel = -1;

	serial_param_set(xCOM3,9600,8,1,'N','N');
	uart_init(xCOM3);
	int COM3_fd = serial_grup[xCOM3].fd;

	//��ʼ����������(��ǰʱ�䡢�豸״̬)���õ�ͼ�������ڵ�λ������
	ButtonInit();
	//����򣬰Ѹ���״̬��Ϣ���ڿ���ڲ�
	DrawBox(100,100);
	//Ĭ����ʾ�豸״̬
	DisplayDevStatus();
	//Ĭ����ʾ����ͼ��
	DisplayButton();
	
	while(1)
	{
		usleep(1000 * 1000);
		FD_ZERO(&fs_read);
		FD_SET(COM3_fd,&fs_read);
		time.tv_sec = 5;
		time.tv_usec = 0;
		
		fs_sel = select(COM3_fd+1,&fs_read,NULL,NULL,&time);
		if(fs_sel < 0)
		{
			continue;
		}
		if(fs_sel == 0)
		{
			continue;
		}

		if(FD_ISSET(COM3_fd,&fs_read) <= 0)
		{
			continue;
		}

		//���ڶ�ȡ������һ���������ݣ�һ���������ݻ�������������Ϣ
		//ԭ�����ִ���������ʱ���������������Բ��������������
		Len = uart_recv(xCOM3, RXbuf,1024);
	#if 0
		int i = 0;
		for(i = 0 ; i < Len ; i++)
			printf("0x%x ",RXbuf[i]);
		printf("len = %d\n",Len);
	#endif

		//��������ֵ��ȡ�������ֵ��ƽ��ֵ
		GetCoor(RXbuf,Len,&CX,&CY);

		//�жϵ�ǰʱ��İ����Ƿ񱻰��£�ͨ��������յ��ı����µ��������豸״̬����
		//�涨�������Ƿ���������ж�
		if(isButtonDown(&BTcurtime,CX,CY))
		{
			//��ʾ��ǰʱ����
			time_display_flag = 1;
			//����ʾ�豸״̬���
			statusmsg_display_flag = 0;
			//��ǵ�ǰʱ�䰴��������
			BTcurtime.state = BUTTON_DOWN;
			//��ʾ��ǰʱ�䰴�������µ�ͼ��
			ButtonStatusSwitch(&BTcurtime,440,280);
			//�������ʾ��ǰʱ������豸״̬����������
			ClearLCDarea();
			//����ʾ��ǰʱ�����������ʾ����"��ǰʱ��:"�Լ���Ӧ�ĵ�ǰʱ��ֵ
			DisplayStrMsg(currenttime,'g',32,100,30);
			DisplayStrMsg(curTime,'g',48,100,100);
			usleep(500000);
			//�л���ǰʱ�䰴��ͼ��
			BTcurtime.state = BUTTON_UP;
			ButtonStatusSwitch(&BTcurtime,440,280);
			continue;
		}

		//�ж��豸״̬�����Ƿ��£��뵱ǰʱ�䰴��ͬ��
		if(isButtonDown(&BTstatus,CX,CY))
		{
			time_display_flag = 0;
			statusmsg_display_flag = 1;
			ClearLCDarea();
			BTstatus.state = BUTTON_DOWN;
			ButtonStatusSwitch(&BTstatus,440,340);
			DisplayDevStatus();
			usleep(500000);
			BTstatus.state = BUTTON_UP;
			ButtonStatusSwitch(&BTstatus,440,340);
			continue;
		}

		if(CX >= 400 && CY >= 240)
		{

		}
		
		
	}
	
}


char *HBerqinProject(void)
{
	return "HeBeiErQin";
}

int HB_pthread_create(void)
{
	int ret = -1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//�����߳�����Ϊ����״̬
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	
	ret = pthread_create(&tid_hb_task,&attr,pthread_hb_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}
	return 0;
}

#if 0
int _ProtocolRelation(void *arg1,void *arg2)
{

	projectstr = HBerqinProject;
	cacheMalloc();
	hb_timer_init();
	HB_pthread_create();	
}

void _ProtocolRelationDestroy(void)
{
	pthread_join(tid_hb_task,NULL);
	free(contentBuffer);
	free(buttontBuffer);
}
#endif
