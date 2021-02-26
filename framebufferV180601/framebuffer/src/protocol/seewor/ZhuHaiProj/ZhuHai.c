#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <dirent.h>
#include "../SWR_protocol.h"
#include "../SWR_charparse.h"

#include "../../../include/content.h"
#include "ZhuHai.h"
#include "../../../include/config.h"
#include "debug.h"
#include "../../../include/conf.h"
#include "../../../module/mtimer.h"
#include "../../../include/content.h"
#include "../../../Hardware/HW3G_RXTX.h"
#include "../../PTC_init.h"

static pthread_once_t init_create = PTHREAD_ONCE_INIT;

static uint32_t SCREEN_status = SCREEN_STATUS_OPEN;
static uint32_t status = STATUS_DISABLE;
static uint32_t TIMEoutAction = TIMEOUT_CLOSELED;

//static DSPNode_t DSPNODE[4];
//static XKDisplayItem SXKDisplay[1];



static stimer_t timer_zhuhai;
#define DefMsg  	"list/def.xkl"
#define DefMsgLen	12	


////////////////��������//////////////////
static void *ZhuHai_TimerAction_DefaultMsg(void *arg);





//�麣�ǻ۽�ͨ��Ŀ�Ķ��ع���
//һ��3������û�з���ͨ��״̬��������ر���Ļ
static inline int ZhuHai_SCRopen(void)
{
	SET_LED_STATE(SLED_ON);
	LEDstateRecord(SLED_ON);
}
static inline int ZhuHai_SCRclose(void)
{
	SET_LED_STATE(SLED_OFF);
	LEDstateRecord(SLED_OFF); 
}


//3 min ��ʱ��Ӧ:���ַ�ʽ��һ�ֹ�����һ����ʾĬ����Ϣ
static void *ZhuHai_TimerAction_CloseLED(void *arg)
{
	uint32_t argument = *(uint32_t *)arg;
	
	debug_printf("zhuhai timer out ---------------------------------------\n\n\n");
	if(SCREEN_status == SCREEN_STATUS_OPEN)
	{
		debug_printf("ZhuHai_TimerAction : LED has been close down!\n");
		ZhuHai_SCRclose();
		SCREEN_status = SCREEN_STATUS_CLOSE;
	}
	mtimer_clear(10);
}




static int ZhuHai_SetDefMsg(char *Defmsg)
{
	return SWR_itemparsing(&content,Defmsg);
}
static void *ZhuHai_TimerAction_DefaultMsg(void *arg)
{
	uint32_t argument = *(uint32_t *)arg;
	
	debug_printf("zhuhai timer out ---------------------------------------\n\n\n");
	ZhuHai_SetDefMsg(DefMSG);
	mtimer_clear(10);
}

static void *ZhuHai_TimerAction(void *arg)
{
	debug_printf("Zhuhai TIMER action\n");
	if(TIMEoutAction == TIMEOUT_CLOSELED)
	{
		debug_printf("action : close led\n");
		ZhuHai_TimerAction_CloseLED(arg);
	}
	else
	{
		debug_printf("action : display defalt msg\n\n");
		ZhuHai_TimerAction_DefaultMsg(arg);
	}
}





static void __ZhuHai_TimerInit(void)
{
	char AbleStatus[12];
	char Timeout[8];
	char TimeoutAction[4];
	uint32_t *arg = NULL;
	
	memset(AbleStatus,0,sizeof(AbleStatus));
	memset(Timeout,0,sizeof(Timeout));
	memset(TimeoutAction,0,sizeof(TimeoutAction));
	conf_file_read(ConFigFile,"scrclose","status",AbleStatus);
	conf_file_read(ConFigFile,"scrclose","timeout",Timeout);
	conf_file_read(ConFigFile,"scrclose","action",TimeoutAction);
	
	debug_printf("1TIMEoutAction = %d,Timeout = %s\n",TIMEoutAction,Timeout);
	
	if(strncmp(AbleStatus,"enable",6) != 0)
		return;
	
	DEBUG_PRINTF;
	debug_printf("zhuhai timer init###########################################\n\n\n\n");
	status = STATUS_ENABLE;
	TIMEoutAction = (strncmp(TimeoutAction,"0",1) == 0) ? TIMEOUT_CLOSELED : TIMEOUT_DSPDEFMSG;

	debug_printf("2TIMEoutAction = %d,Timeout = %s\n",TIMEoutAction,Timeout);

	timer_zhuhai.ref_vals = atoi(Timeout);
	timer_zhuhai.counter  = 0;
	timer_zhuhai.id       = 10;
	timer_zhuhai.function = ZhuHai_TimerAction;
	
	debug_printf("AbleStatus = %s,Timeout = %s,timer_zhuhai.ref_vals = %d\n",AbleStatus,Timeout,timer_zhuhai.ref_vals);
	mtimer_register(&timer_zhuhai);
}

void ZHUHAI_timerInit(void)
{
	pthread_once(&init_create,__ZhuHai_TimerInit);
}

static int __ZhuHai_CommuStatusCloseLED(void)
{
	DEBUG_PRINTF;
	debug_printf("ZhuHai_SCRtatusOps:SCREEN_status = %d\n",SCREEN_status);
	
	DEBUG_PRINTF;
	if(status != STATUS_ENABLE)
		return 0;
	
	mtimer_clear(timer_zhuhai.id);
	
	if(SCREEN_status == SCREEN_STATUS_OPEN)
		return 0;
	
	DEBUG_PRINTF;
	ZhuHai_SCRopen();
	SCREEN_status = SCREEN_STATUS_OPEN;
	return 0;
}

static int __ZhuHai_CommuStatusDspDefMsg(void)
{
	DEBUG_PRINTF;
	debug_printf("ZhuHai_SCRtatusOps:SCREEN_status = %d\n",SCREEN_status);
	
	DEBUG_PRINTF;
	if(status != STATUS_ENABLE)
		return 0;
	
	mtimer_clear(timer_zhuhai.id);
}


static int ZhuHai_CommuniStatusOps(void)
{
	if(TIMEoutAction == TIMEOUT_DSPDEFMSG)
		__ZhuHai_CommuStatusDspDefMsg();
	else
		__ZhuHai_CommuStatusCloseLED();
}
















//�����麣���ϴ���ͼƬ���Ƚ϶࣬��CF�������Ƚ�С����������ֻ�������в����б��а���
//������ͼƬ�������ͼƬȫ��ɾ��

typedef struct __BMPGIFstruct
{
	uint16_t 	BMPcount;
	uint16_t 	GIFcount;
	char 		BMPfile[50][4];
	char 		GIFfile[10][4];
}BMPGIFStruct_t;

static BMPGIFStruct_t BMPGIFStruct;

static void BMPGIFStructPrintf(void)
{
	uint8_t i = 0;
	debug_printf("------------------BMP-----------------\n");
	for(i = 0 ; i < BMPGIFStruct.BMPcount ; i ++)
		debug_printf("BMPGIFStruct.BMPfile[%d] = %s\n",i,BMPGIFStruct.BMPfile[i]);
	debug_printf("\n----------------GIF-----------------\n");
	for(i = 0 ; i < BMPGIFStruct.GIFcount ; i ++)
		debug_printf("BMPGIFStruct.GIFfile[%d] = %s\n",i,BMPGIFStruct.GIFfile[i]);
	debug_printf("\n");
}



static inline void BMPGIFStructInit(void)
{
	memset(&BMPGIFStruct,0,sizeof(BMPGIFStruct));
}

static void GetBMP(char *Item,BMPGIFStruct_t *BMPGIFStruct)
{
	char *p = NULL;
	char *tmpStr = Item;
	while((p = strstr(tmpStr,"\\I")) != NULL)
	{
		tmpStr = p + 5;
		strncpy(BMPGIFStruct->BMPfile[BMPGIFStruct->BMPcount],p+2,3);
		BMPGIFStruct->BMPfile[BMPGIFStruct->BMPcount][4] = '\0';
		BMPGIFStruct->BMPcount++;
		debug_printf("BMPGIFStruct->BMPfile[0] = %s,%d\n",BMPGIFStruct->BMPfile[0],BMPGIFStruct->BMPcount);
	}
}

static void GetGIF(char *Item,BMPGIFStruct_t *BMPGIFStruct)
{
	char *p = NULL;
	char *tmpStr = Item;
	while((p = strstr(tmpStr,"\\G")) != NULL)
	{
		tmpStr = p + 5;
		strncpy(BMPGIFStruct->GIFfile[BMPGIFStruct->GIFcount],p+2,3);
		BMPGIFStruct->GIFfile[BMPGIFStruct->GIFcount][4] = '\0';
		BMPGIFStruct->GIFcount++;
		debug_printf("BMPGIFStruct->GIFfile[0] = %s,%d\n",BMPGIFStruct->GIFfile[0],BMPGIFStruct->GIFcount);
	}
}

static int ZhuHai_GetneededBMP(char *file,BMPGIFStruct_t *BMPGIFStruct)
{
	char *s = NULL;
	char strItem[1024];
	FILE *stream;
	uint8_t status = 0;
	uint8_t ITEMCnt = 0;
	uint8_t ITEMOrder = 0;
	char *p = NULL;
	char *tmpStr = NULL;
	
	
	stream = fopen(file, "r");
	if(stream == NULL)
		return -1;
	fseek(stream, 0, SEEK_SET);
	while(1)
	{
		s = fgets(strItem, 2048, stream);
		if(s == NULL)
			break;

		switch(status)
		{
			case 0:
				if(strncmp(strItem,"ItemCount",9) != 0)
					break;
				
				p = strstr(strItem,"=");
				if(p == NULL)
					goto EXCEPTION;
				
				ITEMCnt = atoi(p+1);
				status = 1;
				break;
			case 1:
				GetBMP(strItem,BMPGIFStruct);
				GetGIF(strItem,BMPGIFStruct);
				ITEMOrder++;
				if(ITEMOrder >= ITEMCnt)
				{
					fclose(stream);
					return 0;
				}
				status = 1;
				break;
			default:
				break;
		}
	}

	EXCEPTION:
		fclose(stream);
		return -1;
}

static int _RMneedlessBMP(char *pwd,BMPGIFStruct_t *BMPGIFStruct)
{
	uint8_t i = 0;
	char RMPath[64];
	struct dirent *fileName = NULL;
	DIR *Dir = opendir(pwd);
	if(Dir == NULL)
	{
		perror("_RMneedlessBMP error");	
		return -1;
	}

	while((fileName = readdir(Dir)))
	{
		if(strcmp(fileName->d_name,".") == 0 || strcmp(fileName->d_name,"..") == 0)
			continue;

		for(i = 0 ; i < BMPGIFStruct->BMPcount ; i ++)
		{
			if(strncmp(fileName->d_name,BMPGIFStruct->BMPfile[i],3) == 0)
				break;
		}
		if(i == BMPGIFStruct->BMPcount)
		{
			debug_printf("removing file [ %s ]\n",fileName->d_name);
			memset(RMPath,0,sizeof(RMPath));
			sprintf(RMPath,"%s/%s",pwd,fileName->d_name);
			remove(RMPath);
		}
	}
	closedir(Dir);
	return 0;
}

static int _RMneedlessGIF(char *pwd,BMPGIFStruct_t *BMPGIFStruct)
{
	uint8_t i = 0;
	char RMPath[64];
	struct dirent *fileName = NULL;
	DIR *Dir = opendir(pwd);
	if(Dir == NULL)
	{
		perror("_RMneedlessGIF error");	
		return -1;
	}

	while((fileName = readdir(Dir)))
	{
		if(strcmp(fileName->d_name,".") == 0 || strcmp(fileName->d_name,"..") == 0)
			continue;

		for(i = 0 ; i < BMPGIFStruct->GIFcount ; i ++)
		{
			if(strncmp(fileName->d_name,BMPGIFStruct->GIFfile[i],3) == 0)
				break;
		}
		if(i == BMPGIFStruct->GIFcount)
		{
			debug_printf("removing file [ %s ]\n",fileName->d_name);
			memset(RMPath,0,sizeof(RMPath));
			sprintf(RMPath,"%s/%s",pwd,fileName->d_name);
			remove(RMPath);
		}
	}
	closedir(Dir);
	return 0;
}

static int ZhuHai_RMneedlessBMPGIF(void)
{
	struct dirent *fileName = NULL;
	DIR *Dir = opendir(list_dir_1);
	char LstPwd[64];
	if(Dir == NULL)
	{
		perror("ZhuHai_RMneedlessBMPGIF error");	
		return -1;
	}

	BMPGIFStructInit();
	while((fileName = readdir(Dir)))
	{
		if(strcmp(fileName->d_name,".") == 0 || strcmp(fileName->d_name,"..") == 0)
			continue;
		debug_printf("checking file [ %s ]\n",fileName->d_name);
		memset(LstPwd,0,sizeof(LstPwd));
		sprintf(LstPwd,"%s/%s",list_dir_1,fileName->d_name);
		debug_printf("LstPwd = %s\n",LstPwd);
		ZhuHai_GetneededBMP(LstPwd,&BMPGIFStruct);
	}
	closedir(Dir);
	
	BMPGIFStructPrintf();
	_RMneedlessBMP(image_dir,&BMPGIFStruct);
	_RMneedlessGIF(gif_dir,&BMPGIFStruct);
	return 0;
}

int ZhuHai_SetPresetPlayLst(Protocl_t *protocol,unsigned int *len)
{
	//Ԥ�ò����б�
	set_preset_playlist(protocol,len);
	//ɾ�����õ�bmp��GIF
	return ZhuHai_RMneedlessBMPGIF();
}


int ZhuHai_GetCommunitStatus(Protocl_t *protocol,unsigned int *len)
{
	//��ȡͨ��״̬
	ZhuHai_CommuniStatusOps();
	return get_communitStatus(protocol,len);
}


char *ZhuHaiProject(void)
{
	return "ZhuHai1";
}
#if 0
int _ProtocolRelation(void *arg1,void *arg2)
{
	//��ʼ����ʱ������Ҫ���ڶ�ʱ3��������λ�����κ�ͨ�����ƽ������ʾĬ�ϵ���Ϣ
	//��ʾĬ�ϵ���Ϣ�����ǹ�ƽ�������ļ�ȷ��
	ZHUHAI_timerInit();
	//���õ�ǰ�Ĳ����б�
	PROTOCOLStruct.SetCurPlayLst 		= ZhuHai_SetPresetPlayLst;
	//��ȡͨ��״̬
	PROTOCOLStruct.GetCommunitStatus	= ZhuHai_GetCommunitStatus;
	projectstr = ZhuHaiProject;

	
}

void _ProtocolRelationDestroy(void)
{

}
#endif

