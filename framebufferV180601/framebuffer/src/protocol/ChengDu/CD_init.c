#include <stdio.h>
#include "CD_init.h"
#include "CD_timer.h"
#include "config.h"
#include "CD_net.h"
#include "CD_display.h"
#include "../Defmsg.h"
#include "CD_protocol.h"


char *CDGetProtocolStr(void)
{
	return "ChengDuV2";
}

char *CDGetProjectStr(void)
{
	return "ChengDu";
}
#if 0
void ProtocolRelation(void *arg1,void *arg2)
{
	uint32_t Swidth,Sheight;
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
	
} 


void ProtocolRelationDestroy(void)
{
	
}
#endif

