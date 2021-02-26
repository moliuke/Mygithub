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
	//初始化系统启动默认的播放列表
	defmsgdispaly = Defmsg_display;
	dspdefaultlst = CD_DefaultLst; 
	EffectOption = CD_EffectOption;
	protocol = CD_protocol_processor;
	//初始化协议特定的网络接收与应答接口
	netSendback = CD_NetSendback;
	recv_process = CD_recv_process;

	protocolstr = CDGetProtocolStr;
	projectstr = CDGetProjectStr;
	
	//1、给png解析开辟一块跟屏幕一样大的缓存 
	DP_GetScreenSize(&Swidth,&Sheight);
	CD_PNGMEMmalloc(Swidth * Swidth * 3);
	//2、设置网络异常时的默认播放列表
	CD_SetDefList(Swidth,Sheight);
	//注册定时器
	CD_timerInit();
	
} 


void ProtocolRelationDestroy(void)
{
	
}
#endif

