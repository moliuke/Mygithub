#include "ZC_init.h"
#include "ZC_net.h"
#include "ZC_display.h"
#include "ZC_protocol.h"
#include "../PTC_init.h"
#include "../Defmsg.h"


char *AHGetProtocolStr(void)
{
	return "AnHuiBiaoZhun";
}

char *AHGetProjectStr(void)
{
	return "GENERAL";
}


#if 0
void ProtocolRelation(void *arg1,void *arg2)
{
	defmsgdispaly = Defmsg_display;
	netSendback = ZC_NetSendback;
	recv_process = ZC_recv_process;
	dspdefaultlst = ZC_DefaultLst;
	EffectOption = ZC_EffectOption;
	protocol = ZC_protocolProcessor;

	protocolstr = AHGetProtocolStr;
	projectstr = AHGetProjectStr;
} 


void ProtocolRelationDestroy(void)
{

}
#endif

