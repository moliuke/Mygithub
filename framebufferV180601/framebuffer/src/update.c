#include <pthread.h>
#include <sys/io.h>
#include "update.h"
#include "conf.h"
#include "task/task.h"
#include "queue.h"
#include "module/mtimer.h"
#include "Hardware/Data_pool.h"
#include "Hardware/HW2G_400.h"
#include "protocol/PTC_init.h"
#include "protocol/PTC_common.h"
#include "Hardware/HW3G_RXTX.h"
#include "watchdog.h"
#include "config.h"
#include "protocol/seewor/malaysia/malaysia_custom.h"
#define outportb(a, b) outb(b, a)
//#define inportb(a)	   inb(a)


#define STATUS_TIMER_ID		1
#define PIX_TIMER_ID		2
#define CHECK_TIMER_ID      3

#define LEDON		1
#define LEDOFF		0 


#define TIMER_OTHER_STATE 	0
#define TIMER_PIXELS_STAT 	1

extern uint8_t screen_state; // 1��ʾ����״̬��0��ʾ����״̬
extern uint8_t watchdogflag;

uint8_t resetcount = 0;
//���Ϳ���ϸ״̬
void GetUpdateDate(void)
{
	//��ӣ������г�Ա����5��ʱ����������ͷ�������ˣ���ֹ���г�Ա�����϶�
	int queue_num = 0;
	uint8_t update[8] = {0x02,0x35,0x30,0x30,0x30,0x66,0xCF,0x03};
	pthread_mutex_lock(&queue_uart_mutex);
	queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);

	if(queue_num <= 5)
	{
		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,update,8);
		pthread_mutex_unlock(&queue_uart_mutex);
	}
}


//�����͹���ָ��
void SET_LED_STATE(uint8_t state)
{
	//uint8_t state = 0;
	uint8_t screenstate = 0;
	unsigned short parity = 0;
	uint8_t openScreen[9] = {0x02,0x30,0x34,0x30,0x30,0x00,0x00,0x00,0x03};
	uint8_t YLight[8] = {0xD8,0x00,0x08,0x00,0x00,0x00,0x00,0xAA};
	//�������ǻ�����
	uint8_t prot;
	uint8_t ylight;
	DP_Get_Procotol(&prot);
	if(prot == MALAYSIA)
		iopl(3);
	if(state == 1)
	{
		openScreen[5] = 0x01;
		if(prot == MALAYSIA)
		{
			
			DP_GetYLight(&ylight);
			YLight[3] = ylight;
			//iopl(3);
			outportb(0x78, 0xfa);

		}

	}
	if(state == 0)
	{
		openScreen[5] = 0x00;
		if(prot == MALAYSIA)
		{
			//iopl(3);
			outportb(0x78, 0x0a);	
		}


		//usleep(10*1000);
	}
	//DP_SetScreenStatus(openScreen[5]);
	//������ҪУ������
	//У��ֵ
	parity = XKCalculateCRC(openScreen+1,5);
	openScreen[6] = (unsigned char)(parity >> 8);
	openScreen[7] = (unsigned char)(parity);
	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);

	if(queue_num <= 5)
	{
		pthread_mutex_lock(&queue_uart_mutex);
		EnQueue(queuehead,openScreen,9);
		pthread_mutex_unlock(&queue_uart_mutex);
	}
	if(prot == MALAYSIA && state == 1)
	{
		sleep(1);
		uart_send(xCOM3,YLight,8);
	}



	//xCOM2_send(openScreen,9);
}

int LEDstateRecord(uint8_t state)
{
	uint8_t Status;
	DP_GetScreenStatus(&Status);
	debug_printf("get screenStatus = %d\n",Status);
	if(Status == state)
	{
		debug_printf("screenStatus is same");
		return 0;
	}


	DEBUG_PRINTF;
	debug_printf("set screenstate = %d\n",state);
	DP_SetScreenStatus(state);
	if(state == SLED_OFF)
		conf_file_write(ledstatefile,"ledstate","led","OFF");
	else
		conf_file_write(ledstatefile,"ledstate","led","ON");

	return 0;
}
/*****************************************************************************
 * �� �� ��  : ResetTxRxCardMsg
 * �� �� ��  : QQ
 * ��������  : 2020��4��17��
 * ��������  : ��������λ���յ������ļ�ʱ����Ҫ����״̬
 * �������  : void  NULL
 * �������  : ��
 * �� �� ֵ  : 
 * ���ù�ϵ  : 
 * ��    ��  : ע�������Ǵ����Ҫ�޸�
*****************************************************************************/
void ResetTxRxCardMsg(void)
{

	#if 0
	//������Ļ��С
	char KeyVal[48];
	uint8_t SetScreenSize[8] = {0xD8,0x00,0x04,0x01,0x80,0x00,0x20,0xAA};
	uint32_t ScreenWidth,ScreenHeight;
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(ConFigFile,"screen","scr_width",KeyVal);
	ScreenWidth = atoi(KeyVal);
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(ConFigFile,"screen","scr_height",KeyVal);
	ScreenWidth = atoi(KeyVal);
	DP_SetScreenSize(ScreenWidth,ScreenHeight);
	SetScreenSize[3] = (uint8_t)((ScreenWidth & 0xff00) >> 8);
	SetScreenSize[4] = (uint8_t)(ScreenWidth & 0x00ff);
	SetScreenSize[5] = (uint8_t)((ScreenHeight & 0xff00) >> 8);
	SetScreenSize[6] = (uint8_t)(ScreenHeight & 0x00ff);
	//xCOM2_send(SetScreenSize,8);

	//���������С
	uint8_t SetBoxSize[8] = {0xD8,0x00,0x03,0x3F,0x1F,0x06,0x20,0xAA};
	 uint32_t width,height,count;
	 memset(KeyVal,0,sizeof(KeyVal));
	 conf_file_read(ConFigFile,"screen","box_width",KeyVal);
	 width = atoi(KeyVal);
	 memset(KeyVal,0,sizeof(KeyVal));
	 conf_file_read(ConFigFile,"screen","box_height",KeyVal);
	 height = atoi(KeyVal);
	 memset(KeyVal,0,sizeof(KeyVal));
	 conf_file_read(ConFigFile,"screen","box_count",KeyVal);
	 count = atoi(KeyVal);
	 DP_SetBoxSize(width,height,count);
	 SetBoxSize[3] = width - 1;
	 SetBoxSize[4] = height - 1;
	 SetBoxSize[5] = count;
	 //xCOM2_send(SetBoxSize,8);
	 #endif


	 
}


void SetBrightParam(void)
{
	int ret;
	uint8_t i = 0;
	uint8_t testmode = 0;
	uint8_t BrightM = 0;

	uint8_t brightVals = 0;
	uint8_t scrstate = 0;
	unsigned short parity = 0;
	uint8_t brightstate[9] = {0x02,0x30,0x36,0x30,0x30,0x00,0x00,0x00,0x03};
	uint8_t output[16] = {0};
	int outputlen;

	DP_GetBrightMode(&BrightM);
	DP_GetTestMode(&testmode);
	DP_ReadBrightVals(&brightVals);
	debug_printf("BrightM is %02X  testmode is %02X brightVals is %02X",BrightM,testmode,brightVals);

	if(brightVals > 0x1F)
		brightVals = 0x1F;
	
	if(testmode == 1)
		scrstate |= 0x40;
	else
		scrstate &= 0xBF;

	if(BrightM == BRIGHT_AUTO)
		scrstate |= 0x80;
	else
		scrstate &= 0x7F;
	
	if(testmode && BrightM == BRIGHT_HAND)
		brightstate[5] = 0x5F;
	else
		brightstate[5] = brightVals|scrstate;;

	//������ҪУ������
	//У��ֵ
	parity = XKCalculateCRC(brightstate+1,5);
	brightstate[6] = (unsigned char)(parity >> 8);
	brightstate[7] = (unsigned char)(parity);
	check_0x02and0x03(1,brightstate+1,7,output+1,&outputlen);
	output[0] = 0x02;
	output[outputlen+1] = 0x03;
#if 0
	debug_printf("send msg to recv card:\n");
	for(i = 0 ; i < outputlen+2 ; i ++)
	{
		debug_printf("==0x%x\t",output[i]);
	}
#endif
	debug_printf("\n");
	pthread_mutex_lock(&queue_uart_mutex);
	//uart_send(xCOM2,output,outputlen+2);	
	EnQueue(queuehead,output,outputlen+2);
	pthread_mutex_unlock(&queue_uart_mutex);

}

static int openLedTimes = 0;
static void *PixTimer_Action(void *arg)
{
	uint32_t width,height;
	uint8_t FrameNum = 0;
	uint8_t i = 0;
	DP_GetScreenSize(&width,&height);	
	uint32_t PixCount = width*height/8;
	uint8_t pix[4][10] = {   
		{0x02,0x35,0x43,0x30,0x30,0x00,0x1A,0xC9,0x03},
		{0x02,0x35,0x43,0x30,0x30,0x01,0x0A,0xE8,0x03},
		{0x02,0x35,0x43,0x30,0x30,0x1B,0xE7,0x3A,0x8B,0x03},
		{0x02,0x35,0x43,0x30,0x30,0x1B,0xE8,0x2A,0xAA,0x03}
	};
	
	if(PixCount <= 2048)
		FrameNum = 1;
	else if(PixCount > 2048 && PixCount <= 4096)
		FrameNum = 2;
	else if(PixCount > 4096 && PixCount <= 6144)
		FrameNum = 3;
	else 
		FrameNum = 4;

	pthread_mutex_lock(&queue_uart_mutex);
	int queue_num = GetSize(queuehead);  
	pthread_mutex_unlock(&queue_uart_mutex);
	
	if(queue_num <= 5)
	{
		pthread_mutex_lock(&queue_uart_mutex);
		for(i=0; i < FrameNum; i++)
		{
			if(i < 2)
				EnQueue(queuehead,pix[i],9);
			else
				EnQueue(queuehead,pix[i],10);
			usleep(100);
		}
		pthread_mutex_unlock(&queue_uart_mutex);
	}
	stimer_t *timer = (stimer_t *)arg;
}
static void *UpdateTimer_Action(void *arg)
{
	uint16_t cardType = TRANSCARD_TXRX;
	//�˴����Զ���������Ҫ��TXRX������ɨ�����ַ�ʽ�ֿ�
	//��Ϊһ�ַ�ʽ��ֵ��һ�ַ�ʽ��ȻΪ0���������໥����
	DP_GetCardType(&cardType);
	if(cardType == TRANSCARD_TXRX)
	{
		//���TXRX���ӵĸ�������
		GetUpdateDate();
		if(watchdogflag == 1)
			wdt_feed();
	}
	else
	{
		//���ɨ���ĸ�������ֵ(�Զ�����)
		HW2G400_aotoBright();
	}	
}
static void *CheckTimer_Action(void *arg)
{
	uint16_t version = 0;
	uint16_t tmp = 0;
	uint8_t num;
	uint8_t resetflag=0xff;
	int i = 0;
	uint8_t data[128];
	memset(data, 0, sizeof(data));
	DP_Get_ALLRXVersion(data, &num);
	#if 0
	if(num != 28)
	{
		memset(data, 0, sizeof(data));
		sprintf(data,"RX cardsum abnormal %d",num);
		log_write(data,strlen(data));

	}
	#endif
	for(i = 0; i < num; i++)
	{
		version = data[i*2];
		version = version<<8;
		tmp = data[i*2+1];
		version |= tmp;
		if(version > 0x1000 && version < 0x2000)
		{
			if(resetcount == 3)
			{
				resetcount = 0;
				uint8_t startaddr[16] = {0x02,0x35,0x39,0x30,0x30,0x30,0x30,0x06,0x06,0x31,0x30,0x06,0x06,0x81,0xE5,0x03} ;
				uint8_t len = strlen(startaddr);
				SetAndGetcmd(startaddr, len);
			}
			//�������еĽ��տ�
			resetcount ++;
			RxCardReset(&resetflag);
			memset(data, 0, sizeof(data));
			sprintf(data,"RX Reset version%04X cardsum %d problemcard %d",version,num,i+1);
			log_write(data,strlen(data));
			
			break;
		}
	}
	if(i == num)
	{
		resetcount = 0;
	}
	memset(data, 0, sizeof(data));
	DP_Get_TXRXVersion(data);
	version = data[0];
	version = version << 8;
	tmp = data[1];
	version |= tmp;
	if(version > 0x1000 && version < 0x2000)
	{
		//�������еĽ��տ�
		TxCardReset();
		log_write("TX reboot",strlen("TX reboot"));
	}	
}


void PixelsTimerReg(void)
{
	#if 1
	uint8_t PROTOCOL;
	DP_GetProcotol(&PROTOCOL);
	if(PROTOCOL == MALAYSIA)
	{
		stimer_t timer_pixels;
		timer_pixels.id = PIX_TIMER_ID;
		timer_pixels.data = 5;
		timer_pixels.counter = 0;
		timer_pixels.ref_vals = 2; 	//5����
		timer_pixels.function = PixTimer_Action;
		pthread_mutex_lock(&timerlist_mutex); 
		mtimer_register(&timer_pixels);
		pthread_mutex_unlock(&timerlist_mutex); 

	}
	else if(PROTOCOL == SEEWOR)
	{
		stimer_t timer_pixels;
		timer_pixels.id = PIX_TIMER_ID;
		timer_pixels.data = 5;
		timer_pixels.counter = 0;
		timer_pixels.ref_vals = 2; 	//չ����2��
		timer_pixels.function = PixTimer_Action;
		pthread_mutex_lock(&timerlist_mutex); 
		mtimer_register(&timer_pixels);
		pthread_mutex_unlock(&timerlist_mutex); 
	}
	#endif

}

void PixelsTimerUnreg(void)
{

}


static void StatusTimerRegister(void)
{
    stimer_t timer_status;
	timer_status.id = STATUS_TIMER_ID;
	timer_status.data = 4;
	timer_status.ref_vals = 5;//5����
	timer_status.function = UpdateTimer_Action;
	TIMER_init(&timer_status);
	pthread_mutex_lock(&timerlist_mutex); 
	mtimer_register(&timer_status);
	pthread_mutex_unlock(&timerlist_mutex); 
}

//�����տ��汾��
static void CheckTimerRegister(void)
{
	stimer_t timer_status;
	timer_status.id = CHECK_TIMER_ID;
	timer_status.data = 4;
	timer_status.ref_vals = 60;//60����
	timer_status.function = CheckTimer_Action;
	TIMER_init(&timer_status);
	pthread_mutex_lock(&timerlist_mutex); 
	mtimer_register(&timer_status);
	pthread_mutex_unlock(&timerlist_mutex); 
}



void *RoutineMonitor(void *arg)
{
	sleep(1);
	while(1)
	{
		if(screen_state == 0)
		{
			//���TXRX����
			SET_LED_STATE(SLED_ON);
		}
		else if(screen_state == 1)
		{
			SetBrightParam();
			#if 0
			//�������ǻ�����
			uint8_t prot;
			DP_Get_Procotol(&prot);
			if(prot == MALAYSIA)
			{
				usleep(20*1000);
				SetYLightSate();
			}
			#endif
			break;
		}
		sleep(5);
	}
	pthread_exit(NULL);

	

}
void *RoutineReset(void *arg)
{
	time_t cur_time;
	struct tm *ptm;
	uint8_t RXreset=0xff;//��������RX
	while(1)
	{
		sleep(5);
		time(&cur_time);
		ptm=localtime(&cur_time);
		if(ptm->tm_hour == 2 && ptm->tm_min == 29 && ptm->tm_sec >= 50)
		{	
			//printf("start Reset RXTX\n");
			RxCardReset(&RXreset);
			sleep(3);
			TxCardReset();
			sleep(3);
			//����
			SET_LED_STATE(LEDOFF);
		}
	}
	
}

void SetParameter(void)
{
	#if 0
	//add by mo 20201112 ������λrx��tx ��������ļ�
	time_t timep;
	struct tm *p;	
	char tmp[8];
	memset(tmp,0,sizeof(tmp));
	time(&timep);
	p=localtime(&timep); /*ȡ�õ���ʱ��*/
	conf_file_read(CHECKPATH,"reset","reset",tmp);
	if(strncmp(tmp,"ON",2) == 0 && p->tm_hour == 2 && p->tm_min >= 30 && p->tm_min <= 31)
	{
		uint8_t RXreset=0xff;//��������RX
		RxCardReset(&RXreset);
		sleep(3);
		TxCardReset();
		sleep(2);
	}
	#endif
	char tmp[8];
	memset(tmp,0,sizeof(tmp));	
	conf_file_read(CHECKPATH,"pix","pixcheck",tmp);
	if(strncmp(tmp,"ON",2) == 0)
	{
		//�������ص�����
		uint8_t pixmode = 0x01;  //malasia����
		Set_Pixels_Mode(pixmode);
		sleep(1);
	}

	uint8_t ledstate = 0;
	DP_GetScreenStatus(&ledstate);
	debug_printf("ledstate = %d\n",ledstate);
	
	if(ledstate == SLED_ON)
	{
		//���TXRX����
		SET_LED_STATE(SLED_ON);
		//���ɨ��濪��
		HW2G400_SetScreenStatus(LEDON);
		LEDstateRecord(SLED_ON);
		DEBUG_PRINTF;
		debug_printf("SLED_ON===========================================\n");

		//sleep(1);
		//������Ļ����
		//SetBrightParam();

		//����Ļ ����Ϊ�˱�֤�����ɹ�������һ���������Ե��̣߳���⿪���Ƿ�ɹ�
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
		//�ø����Ա�������һ�����߳�
		pthread_t tid;
		pthread_create(&tid,&attr,RoutineMonitor,NULL);
	}


}


void update_timer_register(void)
{
	char tmp[48];
	memset(tmp,0,sizeof(tmp));
	StatusTimerRegister();
	conf_file_read(CHECKPATH,"pix","pixcheck",tmp);
	if(strncmp(tmp,"ON",2) == 0)
	{
		PixTimerRegister();
	}
	//��������տ��汾���쳣 add by mo 20200929
		//�������ļ������Ƿ������Ź�1
	memset(tmp,0,sizeof(tmp));
	conf_file_read(CHECKPATH,"check","check",tmp);
	if(strncmp(tmp,"ON",2) == 0)
	{
		//printf("tmp is %s\n",tmp);
		//���RxTx�Ƿ�������ģʽ����������ģʽ�͸�λ
		CheckTimerRegister();
	}
	conf_file_read(CHECKPATH,"reset","reset",tmp);
	if(strncmp(tmp,"ON",2) == 0)
	{	
		//printf("enter reset RXTX\n");
		//����Ļ ����Ϊ�˱�֤�����ɹ�������һ���������Ե��̣߳���⿪���Ƿ�ɹ�
		pthread_attr_t attr_t;
		pthread_attr_init(&attr_t);
		pthread_attr_setdetachstate(&attr_t,PTHREAD_CREATE_DETACHED);
		//�ø����Ա�������һ�����߳�
		pthread_t tid_time;
		pthread_create(&tid_time,&attr_t,RoutineReset,NULL);
	}	
}


