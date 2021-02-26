#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "PTC_init.h"
#include "../update.h"
#include "Defmsg.h"

//����
#include "JinXiao/JX_init.h"
#include "JinXiao/JX_net.h"
#include "JinXiao/JX_display.h"
#include "JinXiao/JX_protocol.h"

//�Կ�
#include "seewor/SWR_init.h"
#include "seewor/SWR_protocol.h"
#include "seewor/SWR_display.h"
#include "seewor/SWR_net.h"
#include "seewor/SWR_charparse.h"

//�ɶ�
#include "ChengDu/CD_init.h"
#include "ChengDu/CD_timer.h"
//#include "config.h"
#include "ChengDu/CD_net.h"
#include "ChengDu/CD_display.h"
#include "ChengDu/CD_protocol.h"

//modbus
#include "Modbus/modbus_init.h"
#include "Modbus/modbus_config.h"
#include "Modbus/modbus_protocol.h"
#include "Modbus/modbus_charparse.h"
#include "Modbus/modbus_display.h"
#include "Modbus/modbus_net.h"

//����
#include "XiaMen/XM_init.h"
#include "XiaMen/XM_net.h"
#include "XiaMen/XM_display.h"
#include "XiaMen/XM_protocol.h"


//־��
#include "ZhiChao/ZC_init.h"
#include "ZhiChao/ZC_net.h"
#include "ZhiChao/ZC_display.h"
#include "ZhiChao/ZC_protocol.h"


//�к�־��
#include "ZhongHaiZC/ZH_display.h"
#include "ZhongHaiZC/ZH_init.h"
#include "ZhongHaiZC/ZH_net.h"
#include "ZhongHaiZC/ZH_charparse.h"
#include "ZhongHaiZC/ZH_protocol.h"


//perplelight�Ϲ�
#include "perplelight/PPL_init.h"
#include "perplelight/PPL_net.h"
#include "perplelight/PPL_display.h"
#include "perplelight/PPL_protocol.h"


//upgrade����
#include "Upgrade/UG_init.h"
#include "Upgrade/UG_net.h"
#include "Upgrade/UG_protocol.h"


//�Կ���ض���Э��
#include "seewor/BoZhou/BZ_custom.h"
//#include "seewor/cmd/"
//#include "seewor/FuZhou/"
#include "seewor/general/general.h"
#include "seewor/HeAo/HA_custom.h"
#include "seewor/HebeiErQin/HB_custom.h"
#include "seewor/Malaysia/malaysia_charparse.h"
#include "seewor/Malaysia/malaysia_custom.h"
#include "seewor/ZhuHaiProj/ZhuHai.h"
#include "seewor/FuZhou/FZ_custom.h"
#include "seewor/liandong/liandong_custom.h"



#define MEMSIZE		(24 * 1024 + 64)

uint8_t *FREEdata = NULL;

void COM_PROTOCOLMemoryMalloc(void)
{
	FREEdata = (uint8_t *)malloc(MEMSIZE);
	if(FREEdata == NULL)
	{
		perror("MemoryMalloc malloc");
		debug_printf("allocate memory failed\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"parsing_output malloc failed",strlen("parsing_output malloc failed"));
	}
	memset(FREEdata,0,MEMSIZE);
}

static void appversion(void)
{
	FILE *fp = NULL;
	DEBUG_PRINTF;
	char version[64];
	memset(version,0,sizeof(version));
	DEBUG_PRINTF;
	sprintf(version,"SWR_CMS_%s_LINUX_%s_V%d.%d.%d.%d\n",GetProtocolStr(),GetProjectStr(),MAJOR,MINOR,REVISION1,REVISION2);
	debug_printf("version = %s\n",version);
	if(access("/home/LEDscr/version/",F_OK) < 0)
		return;
	DEBUG_PRINTF;
	fp = fopen("/home/LEDscr/version/app.v","w+");
	if(fp < 0)
	{
	 	return;
	}
	DEBUG_PRINTF;
	char Rversion[64];
	memset(Rversion,0,sizeof(Rversion));
	fread(Rversion,1,sizeof(Rversion),fp);

	if(strncmp(Rversion,version,strlen(version)) == 0)
	{
		fclose(fp);
		return;
	}
	fwrite(version,1,strlen(version),fp);
	fflush(fp);
	fclose(fp);
}




static int UpdateDefInterf(uint8_t *data,uint16_t Len)
{
	return 0;
}

void protocolRelatInit(void *arg1,void *arg2)
{
	updateInterf = UpdateDefInterf;
	PixTimerReg = PixelsTimerUnreg;
	version = appversion;
	COM_PROTOCOLMemoryMalloc();

	uint8_t PROTOCOL;
	DP_GetProcotol(&PROTOCOL);
	uint32_t Swidth,Sheight;

	switch(PROTOCOL)
	{
		
		case PERPLELIGHT:
		defmsgdispaly = Defmsg_display;
		netSendback = PPL_NetSendback;
		recv_process = PPL_recv_process;
		protocol = PPL_protocolProcessor;
		dspdefaultlst = PPL_DefaultLst;
		EffectOption = PPL_EffectOption;

		protocolstr = PPLGetProtocolStr;
		projectstr = PPLGetProjectStr;	
		break;

		case JINXIAO:
		protocolstr = JXGetProtocolStr;
		projectstr = JXRGetProjectStr;
		defmsgdispaly = Defmsg_display;
		netSendback = JX_NetSendback;
		recv_process = JX_recv_process;
		protocol = JX_protocolProcessor;
		dspdefaultlst = JX_DefaultLst;
		EffectOption = JX_EffectOption;
		break;			
			
		case CHENGDU:
		//��ʼ��ϵͳ����Ĭ�ϵĲ����б�
		defmsgdispaly = Defmsg_display;
		dspdefaultlst = CD_DefaultLst; 
		EffectOption = CD_EffectOption;
		protocol = CD_protocol_processor;
		//��ʼ��Э���ض������������Ӧ��ӿ�
		netSendback = CD_NetSendback;
		recv_process = CD_recv_process;
		protocolstr = CDGetProtocolStr;
		projectstr = CDGetProjectStr;
		//1����png��������һ�����Ļһ����Ļ��� 
		DP_GetScreenSize(&Swidth,&Sheight);
		CD_PNGMEMmalloc(Swidth * Swidth * 3);
		//2�����������쳣ʱ��Ĭ�ϲ����б�
		CD_SetDefList(Swidth,Sheight);
		//ע�ᶨʱ��
		CD_timerInit();
		break;

		case MODBUS:
		//int lbstate = 0,lbsement = 0;
		Mdbs_Init();
		DP_GetScreenSize(&MSwidth,&MSheight);
		//DP_GetLBandArg(&lbstate,&lbsement);
		Mdbs_timerInit();
		defmsgdispaly = Defmsg_display;
		netSendback = mdbs_NetSendback;
		recv_process = mdbs_recv_process;
		dspdefaultlst = Mdbs_DefaultLst;
		EffectOption = Mdbs_EffectOption;
		protocol = modbs_protocolProcessor;
		protocolstr = MdbsGetProtocolStr;
		projectstr = MdbsGetProjectStr;
		break;
		
		case SEEWOR:
		//uint32_t Swidth,Sheight;
		//��ʼ��Э��ӿ�
		PROTOCOLInterfInit();
		defmsgdispaly = Defmsg_display;
		dspdefaultlst = SWR_DefaultLst;
		EffectOption = SWR_EffectOption;
		protocol = swr_protocolProcessor;
		netSendback = swr_NetSendback;
		recv_process = swr_recv_process;
		itemDecoder = SWR_PLstIntemDecode;
		protocolstr = SWRGetProtocolStr;
		//��ʱ�ɼ����ص�״̬�Ľӿ�
		PixTimerReg = PixelsTimerReg;
		
		//version = swr_version;
		//1����png��������һ�����Ļһ����Ļ��� 
		DP_GetScreenSize(&Swidth,&Sheight);
		SWR_PNGMEMmalloc(Swidth * Swidth * 3);
		DP_Get_Procotol(&PROTOCOL);
		switch(PROTOCOL)
		{
			case BOZHOU:
			projectstr = BoZhouProject;
			PROTOCOLStruct.FileFrameRX2K 		= ZC_FrameRxFrmUpper2K;
			PROTOCOLStruct.FileFrameTX2K		= ZC_FrameTxToUpper2K;
			break;
			
			case FUZHOU:
			projectstr = FuZhouProject;
			//����ʱ�������ĽӿڶԽӵ��Կ�Э�����չ�ӿ���
			PROTOCOLStruct.extendcmd = FZ_extendcmd;
			//������ȡ�豸�Ķ�ʱ����ƽʱ��
			GetTimingConfig();
			//��ʼ����ʱ��
			FZTimer_init();			
			break;
			
			case GENERAL:
			projectstr = SWRGetProjectStr;
			SW_pthread_create();
			break;

			case HEAO:
			itemDecoder = HA_PLstIntemDecode;
			projectstr = HeAoProject;
			break;
			
			case HEBEIERQIN:
			projectstr = HBerqinProject;
			cacheMalloc();
			hb_timer_init();
			HB_pthread_create();
			break;

			case MALAYSIA:
			projectstr = MalaysiaProject;
			//���Կ�Э�鴦��ӿ���Ԥ����һ����չ�����
			//������������Ŀ�У���չ�ӿڵ����þ��������˶�ȡ��ʪ�������û����Ƶ�״̬
			PROTOCOLStruct.extendcmd = MalayExtendInterf;
			//�ڸ����̵߳�dev_dataprocessor��Ԥ������չ�Ľӿ�:UpdateExtendInterf
			UpdateTempAndHum();
			//���û������б��𻵻��߲����б��Ƿ��ַ����³��򲻶��˳�
			//ϵͳ������ͨ����ĺ�����ʾĬ�ϵ���Ϣ
			defmsgdispaly = malay_defmsgdisplay;
			//��ʱ�ɼ����ص�״̬�Ľӿ�
			PixTimerReg = PixelsTimerReg;
			//�������ǽ��������б�ӿ�
			itemDecoder = malaysia_PLstIntemDecode;
			
			//��ʼ������ʪ��ģ��ͨ�����õĴ���
			//xCOM3_init();
			//usleep(20*1000);
			//�����ϴζϵ�ǰ�Ļ�����״̬���û�����
			SetYLightSate();
			//��ʼ����ʱ��
			//MalayTimerInit();
			break;
			
			case ZHUHAIPROJ:
			//��ʼ����ʱ������Ҫ���ڶ�ʱ3��������λ�����κ�ͨ�����ƽ������ʾĬ�ϵ���Ϣ
			//��ʾĬ�ϵ���Ϣ�����ǹ�ƽ�������ļ�ȷ��
			ZHUHAI_timerInit();
			//���õ�ǰ�Ĳ����б�
			PROTOCOLStruct.SetCurPlayLst 		= ZhuHai_SetPresetPlayLst;
			//��ȡͨ��״̬
			PROTOCOLStruct.GetCommunitStatus	= ZhuHai_GetCommunitStatus;
			projectstr = ZhuHaiProject;			
			break;
			case LIANDONG:
			
			projectstr = liandongProject;	
			test_pthread_create();
			break;
#if 0       //��ʱ��֪��ʲô����
			case CMD:
			
			break;
#endif
			default:
			projectstr = SWRGetProjectStr;	
			break;
		}
		#if 0
		_ProtocolRelation(arg1,arg2);//��������Э��ӿڣ���Ҫ����Ҫ
		#endif
		break;	
		
		case XIAMEN:
		protocolstr = XMGetProtocolStr;
		projectstr = XMRGetProjectStr;
		defmsgdispaly = Defmsg_display;
		netSendback = XM_NetSendback;
		recv_process = XM_recv_process;
		protocol = XM_protocolProcessor;
		dspdefaultlst = XM_DefaultLst;
		EffectOption = XM_EffectOption;
		break;	

		case ZHICHAO:
		defmsgdispaly = Defmsg_display;
		netSendback = ZC_NetSendback;
		recv_process = ZC_recv_process;
		dspdefaultlst = ZC_DefaultLst;
		EffectOption = ZC_EffectOption;
		protocol = ZC_protocolProcessor;
		protocolstr = AHGetProtocolStr;
		projectstr = AHGetProjectStr;
		break;
		
		case ZHONGHAIZC:
		defmsgdispaly = Defmsg_display;
		EffectOption = ZH_EffectOption;
		dspdefaultlst = ZH_DefaultLst;
		netSendback = ZH_NetSendback;
		recv_process = ZH_recv_process;
		protocol = ZH_protocolProcessor;
		protocolstr = ZHZCGetProtocolStr;
		projectstr = ZHZCGetProjectStr;

		break;

		case UPGRADE:
		netSendback = UG_NetSendback;
		recv_process = UG_recv_process;
		protocol = UG_protocolProcessor;
		protocolstr = UGGetProtocolStr;
		projectstr = UGGetProjectStr;

		break;
		
		default:
		//��ʼ��Э��ӿ�
		PROTOCOLInterfInit();
		defmsgdispaly = Defmsg_display;
		dspdefaultlst = SWR_DefaultLst;
		EffectOption = SWR_EffectOption;
		protocol = swr_protocolProcessor;
		netSendback = swr_NetSendback;
		recv_process = swr_recv_process;
		itemDecoder = SWR_PLstIntemDecode;
		protocolstr = SWRGetProtocolStr;
		projectstr = SWRGetProjectStr;
		//1����png��������һ�����Ļһ����Ļ��� 
		DP_GetScreenSize(&Swidth,&Sheight);
		SWR_PNGMEMmalloc(Swidth * Swidth * 3);
		break;
	}

	

	
	//ProtocolRelation(arg1,arg2);
}

void protocolRelatInitDestroy(void)
{
	ProtocolRelationDestroy();
}

void DefmsgDisplay(void)
{
	defmsgdispaly();
}

void DefaultLstDisplay(void)
{
	dspdefaultlst();
}

void PixTimerRegister(void)
{
	PixTimerReg();
}

int recv_task_process(void * arg)
{
	return recv_process(arg);
}

int NetSendback(user_t *user,uint8_t *tx_buf,int tx_len)
{
	return netSendback(user,tx_buf,tx_len);
}


int protocol_processor(user_t *user,uint8_t *input,uint32_t *inputlen)
{
	return protocol(user,input,inputlen);
}

int SetEffectOption(uint8_t option,uint8_t *type,uint8_t *dire)
{
	return EffectOption(option,type,dire);
}

int UpdateExtendInterf(uint8_t *data,uint16_t Len)
{
	return updateInterf(data,Len);
}

void SetVersion(void)
{
	version();
}

char *GetProtocolStr(void)
{
	return protocolstr();
}

char *GetProjectStr(void)
{
	return projectstr();
}




