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


//根据给定颜色、字号、坐标显示给定文字
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


//显示状态信息
static void DisplayDevStatus(void)
{
	uint8_t PowerStatus = 0x31;	//电源状态
	uint8_t CommutStatus = 0x31; //通信状态
	uint8_t LightStatus = 0x31,LightVals = 0; //光敏状态
	uint8_t ThanderStatus = 0x31,ThanderVals = 0; //防雷状态
	uint8_t SystemStatus = 0x31; //系统状态
	
	DisplayStrMsg(statusmsg,'g',24,90,25);
	DisplayStrMsg(commuStatus,'g',24,20,74);
	DisplayStrMsg("正常",'g',24,80,74);
	
	DisplayStrMsg(powerStatus,'g',24,190,74);
	DisplayStrMsg("正常",'g',24,250,74);

	
	DisplayStrMsg(lightStatus,'g',24,20,113);
	DP_GetSysDataAndStatus(PID_LIGHT_SENSITIVE,&LightStatus,&LightVals);
	if(LightStatus == NORMAL)
		DisplayStrMsg("正常",'g',24,80,113);
	else
		DisplayStrMsg("异常",'r',24,80,113);


	DisplayStrMsg(thandStatus,'g',24,190,113);
	DP_GetSysDataAndStatus(PID_THANDER,&ThanderStatus,&ThanderVals);
	if(ThanderStatus == NORMAL)
		DisplayStrMsg("正常",'g',24,250,113);
	else
		DisplayStrMsg("异常",'r',24,250,113);

	DisplayStrMsg(systemStatus,'g',24,20,152);
	if(PowerStatus == ABNORMAL || CommutStatus == ABNORMAL || 
	   LightStatus == ABNORMAL || ThanderStatus == ABNORMAL)
		DisplayStrMsg("异常",'r',24,80,152);
	else
		DisplayStrMsg("正常",'g',24,80,152);
	
}



//每隔5秒钟刷新一次时间,同时60s刷新一次设备状态
static char curTime[6] = "10:30";
static int reflsh_devstatus_time = 0;
static void *hb_timer_action(void *arg)
{
	
	TXTstruct_t TXTstruct;
	char timestr[24];
	uint8_t Len;
	Pcolor_t Bcolor,Fcolor;

	//设备状态每60s更新一次
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



//画显示框框
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


//显示按键图标及按键说明文字
static void DisplayButton(void)
{
	//三个按键，图标暂时都使用BTcurtime的图标
	_DisplayButton(&BTcurtime,440,280);
	DisplayStrMsg("当前时间",'g',24,500,26);
	_DisplayButton(&BTcurtime,440,340);
	DisplayStrMsg("状态信息",'g',24,500,90);
	_DisplayButton(&BTcurtime,440,400);
}


//按键图标切换
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
//初始化按键图标以及按键的坐标范围，为了简化工作量，坐标计算方法如下:
//1、首先串口数据的坐标数据格式如下(例子)
//	0x81 0xa 0x3b 0xa 0x6a
// x = 0xa << 8 | 0x3b , y = 0xa << 8 | 0x6a
//最多取5组数据的平均值，不足5组则有多少组就取多少组数据的平均值
static void _ButtonInit(Button_t *button,uint16_t cx_l,uint16_t cx_r,uint16_t cy_d,uint16_t cy_u)
{
	//初始化按键图标
	memset(button->upbmp,0,sizeof(button->upbmp));
	memset(button->downbmp,0,sizeof(button->downbmp));
	memcpy(button->upbmp,BUTTON_BMP_UP,strlen(BUTTON_BMP_UP));
	memcpy(button->downbmp,BUTTON_BMP_DOWN,strlen(BUTTON_BMP_DOWN));

	//初始化按键上下左右坐标范围
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





//清掉显示框内的显示内容
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


//从串口接收的大长串的数据最多取5组数据的平均值作为触摸屏按下的坐标
static void GetCoor(uint8_t *RXbuf,uint16_t Len,uint16_t *CX,uint16_t *CY)
{
	
	uint8_t times = 0;
	uint16_t DataLen = Len;
	uint16_t sumcx = 0,sumcy = 0;
	uint8_t *ptr = RXbuf;
	while(DataLen)
	{
		//寻找0x81或者0x80开头的数据组
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



//判断特定按键是否被按下
bool isButtonDown(Button_t *button,uint16_t CX,uint16_t CY)
{
	return ((CX >= button->CX_l && CX <= button->CX_r) && (CY >= button->CY_d && CY <= button->CY_u));
}


//线程循环
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

	//初始化两个按键(当前时间、设备状态)所用的图标与所在的位置坐标
	ButtonInit();
	//画框框，把各种状态信息框在框框内部
	DrawBox(100,100);
	//默认显示设备状态
	DisplayDevStatus();
	//默认显示按键图标
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

		//串口读取到的是一串坐标数据，一串坐标数据会包含多个坐标信息
		//原因是手触摸触摸屏时会有所抖动，所以产生多个坐标数据
		Len = uart_recv(xCOM3, RXbuf,1024);
	#if 0
		int i = 0;
		for(i = 0 ; i < Len ; i++)
			printf("0x%x ",RXbuf[i]);
		printf("len = %d\n",Len);
	#endif

		//计算坐标值，取多个坐标值得平均值
		GetCoor(RXbuf,Len,&CX,&CY);

		//判断当前时间的按键是否被按下，通过上面接收到的被按下的区域与设备状态按键
		//规定的区域是否相符可以判定
		if(isButtonDown(&BTcurtime,CX,CY))
		{
			//显示当前时间标记
			time_display_flag = 1;
			//不显示设备状态标记
			statusmsg_display_flag = 0;
			//标记当前时间按键被按下
			BTcurtime.state = BUTTON_DOWN;
			//显示当前时间按键被按下的图标
			ButtonStatusSwitch(&BTcurtime,440,280);
			//对左边显示当前时间或者设备状态的区域清屏
			ClearLCDarea();
			//在显示当前时间区域框内显示文字"当前时间:"以及相应的当前时间值
			DisplayStrMsg(currenttime,'g',32,100,30);
			DisplayStrMsg(curTime,'g',48,100,100);
			usleep(500000);
			//切换当前时间按键图标
			BTcurtime.state = BUTTON_UP;
			ButtonStatusSwitch(&BTcurtime,440,280);
			continue;
		}

		//判断设备状态按键是否按下，与当前时间按键同理
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
	//设置线程属性为分离状态
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
