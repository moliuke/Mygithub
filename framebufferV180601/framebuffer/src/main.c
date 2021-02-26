#include <sys/io.h>

#include "Dev_tcpserver.h"
#include "Dev_serial.h"
#include "queue.h"
#include "content.h"
#include "display.h"
#include "threadpool.h"
#include "conf.h" 
#include "clientlist.h"
#include "mtime.h"

#include "Hardware/Data_pool.h"
#include "protocol/PTC_init.h"
#include "module/mtimer.h"
#include "task/task.h"
#include "watchdog.h"



pthread_mutex_t content_lock;
uint8_t watchdogflag = 0;
void CACHE_INIT(void)
{
	uint32_t dev_width,dev_height;
	
	dsp_frame_size_get(&dev_width,&dev_height);
	CACHE_INIStruct(&DSPcache,3,dev_width,dev_height);
	CACHE_INIStruct(&CTTcache,3,dev_width,dev_height);
	TXT_DecodeMemInit(&TXTCache,FONT_CACHE_SIZE);
}


static int GetCardType(char *cardstr)
{
	//����Ƿ���-400ɨ������-200ɨ���
	if(strncmp(cardstr,"400",3) == 0)
		return TRANSCARD_400;
	if(strncmp(cardstr,"200",3) == 0)
		return TRANSCARD_200;

	//Ĭ��ΪTXRX��
	return TRANSCARD_TXRX;
}


 void ReadConfigFile(void)
 {
	 //uint32_t times = 500000;
	 char KeyVal[48];
	 char conf_file[64];
	 struct stat st_conf,cpy_st_conf;
 
	 memset(conf_file,0x00,sizeof(conf_file));
	 sprintf(conf_file,"%s/cls.conf",conf_dir);
 
	 stat(conf_file,&st_conf);
	 debug_printf("st_conf.st_size = %d\n",(int)st_conf.st_size);
	 //600,��������ȡ�ģ�Ŀǰ���ļ��Ĵ�С����828byte��С��600����Ϊ��������ļ���
	 //����ġ�
	 if(st_conf.st_size <= 600)
	 {
		 uint8_t cpycmd[64];
		 memset(cpycmd,0x00,sizeof(cpycmd));
		 sprintf(cpycmd,"cp %s %s",ConFigFile_CPY,ConFigFile);
		 system(cpycmd);
	 }
 
	 debug_printf("conf_file = %s\n",conf_file);
 
	 //���÷ֱ���
	 char *res_p = NULL;
	 uint16_t res_x = 0,res_y = 0,res_bits = 0;
	 conf_file_read(conf_file,"resolution","resolution",KeyVal);
	 
	 res_p = KeyVal;
	 res_x = atoi(res_p);
	 
	 res_p = strchr(res_p,'*');
	 res_p = res_p + 1;
	 res_y = atoi(res_p);
	 
	 res_p = strchr(res_p,'*');
	 res_p = res_p + 1;
	 res_bits = atoi(res_p);
	 
	 //DP_ScreenReslution(OPS_MODE_SET,&res_x,&res_y,&res_bits);
	 DP_SetScrResolution(res_x,res_y,res_bits);
 
	 
	 //����֡�����Լ�ʵ����ʾ��Ļ�Ĵ�С
	 uint32_t screen_w,screen_h;
	 conf_file_read(conf_file,"screen","scr_height",KeyVal);
	 screen_h = atoi(KeyVal);
	 conf_file_read(conf_file,"screen","scr_width",KeyVal);
	 screen_w = atoi(KeyVal);
	// DP_ScreenSize(OPS_MODE_SET,&screen_w,&screen_h);
	 DP_SetScreenSize(screen_w,screen_h);
 
	 //���������С
	 uint32_t width,height,count;
	 conf_file_read(conf_file,"screen","box_width",KeyVal);
	 width = atoi(KeyVal);
	 conf_file_read(conf_file,"screen","box_height",KeyVal);
	 height = atoi(KeyVal);
	 conf_file_read(conf_file,"screen","box_count",KeyVal);
	 count = atoi(KeyVal);
	// DP_BoxSize(OPS_MODE_SET,&width,&height,&count);
	 DP_SetBoxSize(width,height,count);

	//������Ļ��ʾ�ĺ���ƫ��������ƫ��
	uint16_t Xoffset = 0,Yoffset = 0;
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(conf_file,"offset","xoffset",KeyVal);
	Xoffset = atoi(KeyVal);
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(conf_file,"offset","yoffset",KeyVal);
	Yoffset = atoi(KeyVal);
	DP_SetOffset(Xoffset,Yoffset);
	debug_printf("Xoffset = %d,Yoffset = %d\n",Xoffset,Yoffset);

	//������Ļ���ͣ����ż��͵Ļ��������͵�
	char scrString[8];
	uint16_t scrType = 0;
	memset(scrString,0,sizeof(scrString));
	conf_file_read(conf_file,"scrtype","scrtype",KeyVal);
	scrType = (strncmp(KeyVal,"door",4) == 0) ? SCRTYPE_DOOR : SCRTYPE_ARM;
	DP_SetScrType(scrType);

	//���÷��Ϳ�����:TXRX/400ɨ���/200ɨ���
	char transCard[8];
	uint16_t cardType = TRANSCARD_TXRX;
	memset(transCard,0,sizeof(transCard));
	conf_file_read(conf_file,"cardtype","cardtype",KeyVal);
	cardType = GetCardType(KeyVal);
	DP_SetCardType(cardType);

	//��Ӳ��ʹ��-200ɨ���ʱ��ˢ�����������Ƚϣ�������̬��ʾ������ƻ��������ƶ�
	//ʱ���Եúܳ���������Ҫ������-200�İ�������һ���ٶ�ֵ(�����ƶ��������ص�)
	char mvSpeed[4];
	uint16_t movingSpeed = 1;//Ĭ���ǵ����ƶ�1�����ص�
	memset(mvSpeed,0,sizeof(mvSpeed));
	conf_file_read(conf_file,"mvspeed","movespeed",mvSpeed);
	movingSpeed = (atoi(mvSpeed) > 0) ? atoi(mvSpeed) : 1;
	DP_SetMvspeed(movingSpeed);
	
	
 
	 //��������˿�
	 uint8_t IP[24],mask[24],gw[24];
	 uint32_t port;
	 memset(IP,0,sizeof(IP));
	 memset(mask,0,sizeof(mask));
	 memset(gw,0,sizeof(gw));
	 conf_file_read(conf_file,"netport","ip",KeyVal);
	 memcpy(IP,KeyVal,strlen(KeyVal));

	 //����ģʽΪ6168
	 conf_file_read(CurrentPtcFile,"protocol","protocol",KeyVal);
	 if( strncmp(KeyVal, "upgrade", 7) == 0)
	 {
		 port = 6168;
	 }
	 else
	 {
		 conf_file_read(conf_file,"netport","port",KeyVal);
		 port = atoi(KeyVal);
	 }
	 conf_file_read(conf_file,"netport","netmask",KeyVal);
	 memcpy(mask,KeyVal,strlen(KeyVal));
	 conf_file_read(conf_file,"netport","gateway",KeyVal);
	 memcpy(gw,KeyVal,strlen(KeyVal));
	 //DP_NetArgument(OPS_MODE_SET,IP,&port,mask,gw);
	 DP_SetNetArg(IP,port,mask,gw);
	 
	 //���ô��ڲ���--------------����λ��ͨ�ŵĴ���COM1
	 uint32_t Bdrate;
	 uint8_t DataBits,StopBits,FlowCtrl,Parity;
	 conf_file_read(conf_file,"serial","upper_baudrate",KeyVal);
	 Bdrate = atoi(KeyVal);
	 conf_file_read(conf_file,"serial","upper_databits",KeyVal);
	 DataBits = atoi(KeyVal);
	 conf_file_read(conf_file,"serial","upper_stopbits",KeyVal);
	 StopBits = atoi(KeyVal);
	 conf_file_read(conf_file,"serial","upper_flowctls",KeyVal);
	 FlowCtrl = *KeyVal;
	 conf_file_read(conf_file,"serial","upper_parity",KeyVal);
	 Parity = *KeyVal;
	 //DP_SerialArgument(OPS_MODE_SET,xCOM1,&Bdrate,&DataBits,&StopBits,&FlowCtrl,&Parity);
	 DP_SetSerialArg(xCOM1,Bdrate,DataBits,StopBits,FlowCtrl,Parity);
 	 
#if 0	 
	 //���ô��ڲ���--------------����λ��ͨ�ŵĴ���COM2
	 //write_log("/home/LEDscr/uart.txt","%s\n","\n\n\n==========read from conf file=========");
 
	 conf_file_read(conf_file,"serial","slave_baudrate",KeyVal);
	 Bdrate = atoi(KeyVal);
	 conf_file_read(conf_file,"serial","slave_databits",KeyVal);
	 DataBits = atoi(KeyVal);
	 conf_file_read(conf_file,"serial","slave_stopbits",KeyVal);
	 StopBits = atoi(KeyVal);
	 conf_file_read(conf_file,"serial","slave_flowctls",KeyVal);
	 FlowCtrl = *KeyVal;
	 conf_file_read(conf_file,"serial","slave_parity",KeyVal);
	 Parity = *KeyVal;
	 //DP_SerialArgument(OPS_MODE_SET,xCOM2,&Bdrate,&DataBits,&StopBits,&FlowCtrl,&Parity);
	 DP_SetSerialArg(xCOM2,Bdrate,DataBits,StopBits,FlowCtrl,Parity);

#endif

	//�°汾ͳһ���ã��������޸�����λ���������ù���
	 //DP_SetSerialArg(xCOM2,115200,8,1,'N','N');
	DP_SetSerialArg(xCOM2,115200,8,1,'N','N');
	 //DP_SerialArgument(OPS_MODE_SET,xCOM3,&hwparam->slave_baudrate,&hwparam->slave_databits,&hwparam->slave_stopbits,
	 //  &hwparam->slave_flowctl,&hwparam->slave_parity);


	 conf_file_read(ledstatefile,"ledstate","led",KeyVal);
	 debug_printf("KeyVal = %s\n",KeyVal);
	 if(strncmp(KeyVal,"OFF",3) == 0)
	 	DP_SetScreenStatus(SLED_OFF);
	 else if(strncmp(KeyVal,"ON",2) == 0)
		 DP_SetScreenStatus(SLED_ON);
	 
	 //��������ֵ
	 uint8_t mode = 0,Bright,bright_max = 0,bright_min = 0;
	 conf_file_read(conf_file,"brightmode","mode",KeyVal);
	 debug_printf("KeyVal = %s\n",KeyVal);
	 mode = atoi(KeyVal);
	 mode = (mode == 30) ? 0x30 : 0x31;
	 conf_file_read(conf_file,"brightmode","bright",KeyVal);
	 Bright = atoi(KeyVal);
	 conf_file_read(conf_file,"brightmode","max",KeyVal);
	 bright_max = atoi(KeyVal);
	 conf_file_read(conf_file,"brightmode","min",KeyVal);
	 bright_min = atoi(KeyVal);
	 DP_SetBrightMode(mode); 

	 float div = 0.0,abright = 0.0;
	 div = (bright_max - bright_min + 1) / (float)32;
	 abright = (Bright - bright_min) / div;
	 Bright = (abright - (uint8_t)abright > 0.5) ? ((uint8_t)abright + 1) : ((uint8_t)abright);
     DP_SaveBrightVals(Bright);
	 DP_SetBrightRange(bright_max,bright_min);
 
	 //��ȡ�ϴ��쳣�ػ��������ʾ�Ĳ����б�
	 conf_file_read(conf_file,"playlist","list",KeyVal);
	 debug_printf("KeyVal====%s\n",KeyVal);
	 uint8_t Len = strlen(KeyVal);
	 DP_SetCurPlayList(KeyVal,Len);
	 DP_DATAPoolPrintf();
	//add by mo 20210114

	uint8_t MinBright,MaxBright=0;
	conf_file_read(CHECKPATH,"bright","MaxBright",KeyVal);
	MaxBright = atoi(KeyVal);

 	conf_file_read(CHECKPATH,"bright","MinBright",KeyVal);
	MinBright = atoi(KeyVal);

	DP_SetBrightRealRange(MaxBright,MinBright);
	 //���ð汾��
	 DP_Set_APPVersion();
	 DP_Set_MonitorVersion();


	 //�����ȡЭ��
	 conf_file_read(CurrentPtcFile,"protocol","protocol",KeyVal);
	 debug_printf("KeyVal====%s\n",KeyVal);
	 Len = strlen(KeyVal);
	 uint8_t PcFlag;
	 if(strncmp(KeyVal,"jinxiao",7) == 0)
	 {
		PcFlag = JINXIAO;
		DP_SetProcotol(PcFlag);
	 }
	 else if(strncmp(KeyVal,"seewor",6) == 0)
	 {
		PcFlag = SEEWOR;
		DP_SetProcotol(PcFlag);
		conf_file_read(CurrentPtcFile,"protocol","swr_protocol",KeyVal);
		if(strncmp(KeyVal,"bozhou",6) == 0)
		{
			PcFlag = BOZHOU;
		    DP_Set_Procotol(PcFlag);
		}
		else if(strncmp(KeyVal,"fuzhou",6) == 0)
		{
		   	PcFlag = FUZHOU;
		   	DP_Set_Procotol(PcFlag);
		}
		else if(strncmp(KeyVal,"heao",4) == 0)
		{
		   	PcFlag = HEAO;
		   	DP_Set_Procotol(PcFlag);
		}
		else if(strncmp(KeyVal,"hebeierqin",10) == 0)
		{
		   	PcFlag = HEBEIERQIN;
		   	DP_Set_Procotol(PcFlag);
		}
		else if(strncmp(KeyVal,"malaysia",8) == 0)
		{
		   	PcFlag = MALAYSIA;
		   	DP_Set_Procotol(PcFlag);
		}
		else if(strncmp(KeyVal,"zhuhaiproj",10) == 0)
		{
		   	PcFlag = ZHUHAIPROJ;
		   	DP_Set_Procotol(PcFlag);
		}
		else if(strncmp(KeyVal,"liandong",8) == 0)
		{
			PcFlag = LIANDONG;
			DP_Set_Procotol(PcFlag);
		}
		else
		{
		   	PcFlag = GENERAL;
		   	DP_Set_Procotol(PcFlag);
		}				
	 }
	 else if(strncmp(KeyVal,"chengdu",7) == 0)
	 {
		PcFlag = CHENGDU;
		DP_SetProcotol(PcFlag);
	 }

	 else if(strncmp(KeyVal,"xiamen",6) == 0)
	 {
		 PcFlag = XIAMEN;
		 DP_SetProcotol(PcFlag);

	 }
	 else if(strncmp(KeyVal,"zhichao",7) == 0)
	 {
		 PcFlag = ZHICHAO;
		 DP_SetProcotol(PcFlag);

	 }
	 else if(strncmp(KeyVal,"zhonghaizc",10) == 0)
	 {
		 PcFlag = ZHONGHAIZC;
		 DP_SetProcotol(PcFlag);

	 }
	 else if(strncmp(KeyVal,"perplelight",11) == 0)
	 {
		 PcFlag = PERPLELIGHT;
		 DP_SetProcotol(PcFlag);

	 }
	 else if(strncmp(KeyVal,"modbus",6) == 0)
	 {
		 PcFlag = MODBUS;
		 DP_SetProcotol(PcFlag);
	 }

//�������
	else if(strncmp(KeyVal,"upgrade",7) == 0)
	{
		PcFlag = UPGRADE;
		DP_SetProcotol(PcFlag);
	}
	 
	 else
	 {
		 PcFlag = SEEWOR;
		 DP_SetProcotol(PcFlag);
	 }

	//�������ļ������Ƿ������Ź�1
	conf_file_read(WatchdogFile,"watchdog","watchdog1",KeyVal);
	if(strncmp(KeyVal,"ON",2) == 0)
	{
		wdt_init(15*1000);
		watchdogflag = 1;
	}
 }


 void TIMER_INIT(void)
 {
	 sigset_t sigset; 
	 sigemptyset(&sigset);	
	 sigaddset(&sigset,SIGALRM);
	 pthread_sigmask(SIG_SETMASK,&sigset,NULL); 
 }


 void System_init(void)
 {
	 
	 //��ȡϵͳ����ʱ��
	 uint8_t bootup_time[24];
	 uint8_t timestrlen = 0;
	 memset(bootup_time,0x00,sizeof(bootup_time));
	 get_sys_time(bootup_time,&timestrlen);
	 debug_printf("bootup_time = %s\n",bootup_time);

	 DP_SetBootupTime(bootup_time,timestrlen);
	 
	 //��ʼ���豸֡�����Լ��豸��Ļ�ĳߴ�
	 uint32_t ScreenWidth,ScreenHeight;
	 //DP_ScreenSize(OPS_MODE_GET,&ScreenWidth,&ScreenHeight);
	 DP_GetScreenSize(&ScreenWidth,&ScreenHeight);
	 dsp_frame_size_set(ScreenWidth,ScreenHeight);
	 fb_screen_size_set(ScreenWidth,ScreenHeight);	 
	 fb_screen_init();

	 //��ʼ������
	 uint32_t port;
	 uint8_t IP[24],mask[24],gw[24];
	 memset(IP,0,sizeof(IP));
	 memset(mask,0,sizeof(mask));
	 memset(gw,0,sizeof(gw));
	 DP_GetNetArg(IP,&port,mask,gw);
	 TCP_SetNetArgment(IP,port);
 
	 //���ڳ�ʼ��
	 uint32_t Bdrate;
	 uint8_t DataBits,StopBits,FlowCtrl,Parity;
	 DP_GetSerialArg(xCOM1,&Bdrate,&DataBits,&StopBits,&FlowCtrl,&Parity);
	 serial_param_set(xCOM1,Bdrate,DataBits,StopBits,FlowCtrl,Parity);
	 DP_GetSerialArg(xCOM2,&Bdrate,&DataBits,&StopBits,&FlowCtrl,&Parity);
	 serial_param_set(xCOM2,Bdrate,DataBits,StopBits,FlowCtrl,Parity);
	 
	 uart_init(xCOM1);//����λ��ͨ��
	 uart_init(xCOM2);//����λ��ͨ��
	 //��ʼ����ʾ����
	 CACHE_INIT();
	 //Ca_INITDSPCStruct(8);
	 //��ʼ�������б�ͷ
	 InitContentlist(&content);
	 QueueHead = InitQueue();

	 //add by mo 2020.07.11
 	 queuehead = InitQueue();
	 //��ʼ���û�����
	 user_init();
	  
	 //��ʼ����־
	 log_file_init();
	 
	 //��ʼ����ʱ�� 
	 mtimer_init();
	 //��ʼ���̳߳�
	 pthreadpool = threadpool_create(5,100,12); 
	 debug_printf("pool inited\n");

	// sem_init(&sem, 0, 0);
	 //debug_printf("sem = %d\n",sem);
	 //exit(1);
	 
	 //Э����صĶ���ȫ����������
	 //ProtocolRelation(NULL,NULL);
	 protocolRelatInit(NULL,NULL);
	 //�������߳�
	 TASK_SystemStart();
	 //��ʱ����ʼ��
	 TIMER_INIT();
	 SetVersion();
	 //iopl(3);
	 //outb(0x0f, 0x98);	 
 }
 
 void System_destroy(void)
 {
	 //ProtocolRelationDestroy();
	 protocolRelatInitDestroy();
	 TASK_SystemExit();
	 free(CTTcache);
	 free(DSPcache);
	 free(TXTCache);
 }


 int main(int argc, char **argv)
 {
 	int aa = 90;
	ReadConfigFile();
	System_init();
	System_destroy();
 }


