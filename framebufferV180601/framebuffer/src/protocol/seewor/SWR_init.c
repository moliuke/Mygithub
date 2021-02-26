#include "SWR_init.h"
#include "SWR_protocol.h"
//#include "general/general.h"
#include "../Defmsg.h"
#include "SWR_display.h"
#include "SWR_net.h"
#include "SWR_charparse.h"
#include "../PTC_init.h"


char *SWRGetProtocolStr(void)
{
	DEBUG_PRINTF;
	return "XK";
}

char *SWRGetProjectStr(void)
{
	return "GENERAL";
}

static int PlaylistItemDecoder(ContentList *head,char *itemContent,uint8_t ItemOder)
{
	return itemDecoder(head,itemContent,ItemOder);
}
#if 0
void ProtocolRelation(void *arg1,void *arg2)
{
	uint32_t Swidth,Sheight;
	//初始化协议接口
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
	//version = swr_version;
	//1、给png解析开辟一块跟屏幕一样大的缓存 
	DP_GetScreenSize(&Swidth,&Sheight);
	SWR_PNGMEMmalloc(Swidth * Swidth * 3);
	_ProtocolRelation(arg1,arg2);
} 
#endif
void ProtocolRelationDestroy(void)
{
	//_ProtocolRelationDestroy();
}

