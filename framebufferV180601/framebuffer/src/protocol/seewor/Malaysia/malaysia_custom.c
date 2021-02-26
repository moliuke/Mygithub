#include "malaysia_custom.h"
#include "content.h"
#include "../SWR_init.h"
#include "../SWR_protocol.h"
#include "../../PTC_init.h"
#include "malaysia_charparse.h"
#include "../../../Hardware/Data_pool.h"


static char str1[] = "Travel Safety";
static char str2[] = "Safety First";

static DSPNode_t MLDSPNODE[2];
static XKDisplayItem MLSXKDisplay;
static inline int GetTempAndHum(void);
void *pthread_uart3_task(void *arg);
int Malaysia_data_process(uint8_t *buf,uint16_t len);

static uint8_t GetTempHum[8] = {0xD8,0x00,0x09,0x01,0x00,0x00,0x00,0xAA};
static uint8_t SetYLight[8] = {0xD8,0x00,0x08,0x00,0x00,0x00,0x00,0xAA};


static void DefmsgInit(XKCellString *Defmsg,char *String,uint8_t Fonttype,uint8_t FontSize)
{
	memcpy(Defmsg->strContent,String,strlen(String));
	Defmsg->strContent[strlen(String)] = '\0';
	Defmsg->strContentLen = 8;
	Defmsg->nFontType = Fonttype;
	Defmsg->nForeColor[0] = 0xff;
	Defmsg->nForeColor[1] = 0x00;
	Defmsg->nForeColor[2] = 0x00;
	Defmsg->nBkColor[0] = 0x00;
	Defmsg->nBkColor[1] = 0x00;
	Defmsg->nBkColor[2] = 0x00;
	Defmsg->nFontSize = FontSize;
}


//���򲻶ϱ���ϵͳ��������ʾĬ�ϵ���Ϣ
int malay_defmsgdisplay(void)
{
	int Cx = 0,Cy = 0;
	XKCellString *Defstr1 = (XKCellString *)malloc(sizeof(XKCellString));
	XKCellString *Defstr2 = (XKCellString *)malloc(sizeof(XKCellString));

	memset(Defstr1,0,sizeof(Defstr1));
	memset(Defstr2,0,sizeof(Defstr1));

	DefmsgInit(Defstr1,str1,'s',24);
	DefmsgInit(Defstr2,str2,'s',24);	

	memset(&MLSXKDisplay,0,sizeof(MLSXKDisplay));
	memset(&MLDSPNODE[0],0,sizeof(MLDSPNODE[0]));
	memset(&MLDSPNODE[1],0,sizeof(MLDSPNODE[1]));

	MLDSPNODE[0].type = DSPTYPE_STR;
	MLDSPNODE[1].type = DSPTYPE_STR;
	
	
	MLDSPNODE[0].XKCellStr = Defstr1;
	MLDSPNODE[1].XKCellStr = Defstr2;

	
	MLDSPNODE[0].Cx = 0;
	MLDSPNODE[0].Cy = 0;
	MLDSPNODE[1].Cx = 0;
	MLDSPNODE[1].Cy = 24;
	
	MLSXKDisplay.nDelayTime = 20 * 1000 * 1000;
	MLSXKDisplay.nEffectIn = 1;
	MLSXKDisplay.nEffectOut = 1;

	int i = 0;
	for(i = 0 ; i < 2 ; i++) 
		AddItemDSPNode(&MLSXKDisplay,&MLDSPNODE[i]);


	pthread_mutex_lock(&content_lock);
	ClearContent(&content);
	AddDisplayItem(&content,&MLSXKDisplay);
	content.itemcount = 1;
	content.refresh = LST_REFLASH;
	pthread_mutex_unlock(&content_lock);
	
	return 0;
}


//��ȡ��ʪ��
static int Malasia_GetTempAndHum(Protocl_t *protocol,unsigned int *len)
{

	//GetTempAndHum();
	uart_send(xCOM3,GetTempHum,8);

	uint16_t temp = 0;
	uint16_t Hum = 0;
	uint8_t *data = protocol->protcmsg.data;
	sleep(1);
	DP_GetTemp(&temp);
	DP_GetHum(&Hum);
	//temp = 0x8146;
	//Hum = 0x840c;
	//printf("temp = 0x%x,Hum = 0x%x\n",temp,Hum);

	data[0] = 0x01;
	
	data[1] = (temp & 0x8000) ? 0x3F : 0x00;
	temp &= 0x7fff;
	
	data[2] = temp / 1000 + 0x30;
	data[3] = temp / 100 % 10 + 0x30;
	data[4] = temp % 100 / 10 + 0x30;
	data[5] = temp % 10 + 0x30;

	data[6] = (Hum & 0x8000) ? 0x3F : 0x00;
	Hum &= 0x7fff;
	
	data[7] = Hum / 1000 + 0x30;
	data[8] = Hum / 100 % 10 + 0x30;
	data[9] = Hum % 100 / 10 + 0x30;
	data[10] = Hum % 10 + 0x30;

	int i = 0;
	for(i = 0 ; i < 11 ; i++)
		debug_printf("0x%x ",data[i]);
	debug_printf("\n");

	protocol->protcmsg.length = 11;
	*len = 11;
	return 0;
}




#define GET_YLIGHT_STATE		0x30	//��ѯ״̬
#define SET_YLIGHT_ALTERNATE	0x31	//���ý�����˸
#define SET_YLIGHT_SAMETIME		0x32	//����ͬʱ��˸
#define SET_YLIGHT_OFF          0x33    //���û�����ر�
static uint8_t YLightOPS[8] = {0xD8,0x00,0x08,0x00,0x00,0x00,0x00,0xAA};
static int Malasia_YLightOperation(Protocl_t *protocol,unsigned int *len)
{
	int fd = 0;
	uint8_t YLight = 0;
	switch(protocol->protcmsg.data[0])
	{
		//��ȡ������״̬
		case GET_YLIGHT_STATE:
			//YLightOPS[3] = 0x00;
			//uart_send(xCOM3,YLightOPS,8);
			DP_GetYLight(&YLight);
			//YLight = 0x02;
			if(YLight == 0x01)
				protocol->protcmsg.data[1] = 0x31;
			else if(YLight == 0x02)
				protocol->protcmsg.data[1] = 0x32;
			else if(YLight == 0x03)
				protocol->protcmsg.data[1] = 0x33;
			else
				goto EXEPTION;
			break;
		//���ý�����
		case SET_YLIGHT_ALTERNATE:
			
			YLightOPS[3] = 0x01;
			DP_SetYLight(YLightOPS[3]);
			uart_send(xCOM3,YLightOPS,8);
			fd = open(conf_dir"/malaysia.conf",O_WRONLY | O_CREAT,0744);
			if(fd > 0)
			{
				write(fd,"1",1);
				close(fd);
				DEBUG_PRINTF;
			}
			protocol->protcmsg.data[1] = 0x31;
			break;
		//����˫��
		case SET_YLIGHT_SAMETIME:
			YLightOPS[3] = 0x02;
			DP_SetYLight(YLightOPS[3]);
			uart_send(xCOM3,YLightOPS,8);

			fd = open(conf_dir"/malaysia.conf",O_WRONLY | O_CREAT,0744);
			if(fd > 0)
			{
				write(fd,"2",1);
				close(fd);
				DEBUG_PRINTF;
			}
			
			protocol->protcmsg.data[1] = 0x32;
			break;
		case SET_YLIGHT_OFF:
			YLightOPS[3] = 0x03;
			DP_SetYLight(YLightOPS[3]);
			uart_send(xCOM3,YLightOPS,8);
			fd = open(conf_dir"/malaysia.conf",O_WRONLY | O_CREAT,0744);
			if(fd > 0)
			{
				write(fd,"3",1);
				close(fd);
				DEBUG_PRINTF;
			}
			protocol->protcmsg.data[1] = 0x33;

			
		default:
			break;
	}
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 2;
	*len = 2;
	return 0;

	EXEPTION:
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.data[1] = 0x33;
		protocol->protcmsg.length = 2;
		*len = 2;
		return -1;		
		
}

int MalayExtendInterf(Protocl_t *protocol,unsigned int *len)
{
	DEBUG_PRINTF;
	switch(protocol->protcmsg.head.cmdID)
	{
		//��ȡ��ʪ��
		case GET_TempAndHum:
			Malasia_GetTempAndHum(protocol,len);
			break;
		//���û�����״̬
		case SET_YLightState:
			Malasia_YLightOperation(protocol,len);
			break;
		default:
			break;
	}
	return 0;
}


#if 0
/*/////////////////////////////////////////////////////////////////////////////
��ʼ�����������ɼ���ʪ������
///////////////////////////////////////////////////////////////////////////////*/
void xCOM3_init(void)
{
	serial_param_set(xCOM3,9600,8,1,'N','N');
	uart_init(xCOM3);
}
#endif
static pthread_t tid_uart3_task;

int UpdateTempAndHum()
{
	int ret = -1;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//�����߳�����Ϊ����״̬
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	
	ret = pthread_create(&tid_uart3_task,&attr,pthread_uart3_task,NULL);
	if(ret!=0)
	{
		perror("pthread_create fail"); 
		return -1;
	}

}
void *pthread_uart3_task(void *arg)
{
	struct timeval time;
	fd_set fd_read;
	int fs_sel = -1;
	uint8_t RXbuf[1024];
	uint16_t Len;
	memset(RXbuf,0,sizeof(RXbuf));
	serial_param_set(xCOM3,9600,8,1,'N','N');
	uart_init(xCOM3);
	int COM3_fd = serial_grup[xCOM3].fd;
	while(1)
	{
		usleep(20*1000);
		FD_ZERO(&fd_read);
		FD_SET(COM3_fd,&fd_read);
		time.tv_sec = 5;
		time.tv_usec = 0;
		

		fs_sel = select(COM3_fd+1,&fd_read,NULL,NULL,&time);
		if(fs_sel < 0)
		{
			continue;
		}
		if(fs_sel == 0)
		{
			continue;
		}

		if(FD_ISSET(COM3_fd,&fd_read) <= 0)
		{
			continue;
		}

		Len = uart_recv(xCOM3, RXbuf,1024);		
		int i = 0;
		#if 0
		printf("recv is : \n");
		for(i=0;i<Len;i++)
		{
			printf("%02X ",RXbuf[i]);
			
		}
		printf("\n");
		#endif
		Malaysia_data_process(RXbuf,Len);


	}
	
	
}
int Malaysia_data_process(uint8_t *buf,uint16_t len)
{
	uint16_t DataLen = len;
	uint8_t *ptr = buf;
	uint8_t *data = NULL;
	while(DataLen)
	{
		if(ptr[0] != 0xD8)
		{
		
			DataLen -= 1;
			if(DataLen == 0)
				return -1;
			ptr += 1;
			continue;
		}
		data = ptr;
		break;
	}
    uint16_t temp = 0;
	uint16_t hum = 0;
	if(data[0] != 0xD8 || data[7] != 0xAA)
		return -1;
	if(data[2] != 0x07 && data[2] != 0x08 && data[2] != 0x09)
		return -1;
	
	//��ʾ���û����Ƶ�״̬��D8 00 07 01 00 00 00 AA����4���ֽ�01��ʾ������˸��02��ʾȫ��һ����˸
	if(data[2] == 0x08) 
	{
		debug_printf("haungshan-------------------------------------data[3] = 0x%x\n",data[3]);
		DP_SetYLight(data[3]); 
	}
	
	//��ʾ��ȡ��ʪ�ȴ�������ֵ����ʪ�ȸ�8λ��ʪ�ȵ�8λ���¶ȸ�8λ���¶ȵ�8λ��
	if(data[2] == 0x09) 
	{
		DEBUG_PRINTF;
		hum = data[3] << 8 | data[4];
		temp = data[5] << 8 | data[6];
		debug_printf("wenshidu==================================temp = 0x%x,hum = 0x%x,data[3] = 0x%x\n",temp,hum,data[3]);
		DP_SetTemp(temp);
		DP_SetHum(hum);
	}
	
	return 0;
}


void SetYLightSate(void)
{
	FILE *fp = NULL;
	int YLvals = 0;
	int ncount = 0;
	char YLchar[1];
	memset(YLchar,0,sizeof(YLchar));
	fp = fopen(conf_dir"/malaysia.conf","r");
	if(fp == NULL)
		return;

	memset(YLchar,0,sizeof(YLchar));
	fseek(fp,0,SEEK_SET);
	if(fread(YLchar,1,1,fp) < 0)
		perror("malaysia fread");
	//printf("YLchar is %s\n",YLchar);
	YLchar[1] = '\0';
	YLvals = atoi(YLchar);
	//YLvals = YLchar[0] - 0x30;
	if(YLvals != 0x01 && YLvals != 0x02 && YLvals != 0x03)
	{
		fclose(fp);
		return;
	}

	YLightOPS[3] = YLvals;
	//printf("YLval is %d\n",YLvals);
	DP_SetYLight(YLvals);
	uart_send(xCOM3,YLightOPS,8);
	fclose(fp);
}



static stimer_t MalayTimer;

#define MALAYTIMER_ID	4

static inline int GetTempAndHum(void)
{
	return uart_send(xCOM3,GetTempHum,8);
}

static inline int GetYLight(void)
{
	return uart_send(xCOM3,SetYLight,8);
}
static void *MalayTimer_Action(void *arg)
{
	GetTempAndHum();
	sleep(2);
	GetYLight();
}


void MalayTimerInit(void)
{
	MalayTimer.id = MALAYTIMER_ID;
	MalayTimer.counter = 0;
	MalayTimer.ref_vals = 3 * 60;//600����
	MalayTimer.function = MalayTimer_Action;
	//TIMER_init(&MalayTimer);
	pthread_mutex_lock(&timerlist_mutex); 
	mtimer_register(&MalayTimer);
	pthread_mutex_unlock(&timerlist_mutex); 
}


char *MalaysiaProject(void)
{
	return "Malaysia";
}

#if 0
int _ProtocolRelation(void *arg1,void *arg2)
{
	//���Կ�Э�鴦��ӿ���Ԥ����һ����չ�����
	//������������Ŀ�У���չ�ӿڵ����þ��������˶�ȡ��ʪ�������û����Ƶ�״̬
	PROTOCOLStruct.extendcmd = MalayExtendInterf;

	//�ڸ����̵߳�dev_dataprocessor��Ԥ������չ�Ľӿ�:UpdateExtendInterf
	updateInterf = UpdateTempAndHum;
	//���û������б��𻵻��߲����б��Ƿ��ַ����³��򲻶��˳�
	//ϵͳ������ͨ����ĺ�����ʾĬ�ϵ���Ϣ
	defmsgdispaly = malay_defmsgdisplay;
	//��ʱ�ɼ����ص�״̬�Ľӿ�
	PixTimerReg = PixelsTimerReg;
	//�������ǽ��������б�ӿ�
	itemDecoder = malaysia_PLstIntemDecode;
	projectstr = MalaysiaProject;
	//�����ϴζϵ�ǰ�Ļ�����״̬���û�����
	SetYLightSate();
	//��ʼ������ʪ��ģ��ͨ�����õĴ���
	xCOM3_init();
	//��ʼ����ʱ��
	MalayTimerInit();
}
void _ProtocolRelationDestroy(void)
{
	
}

#endif

