#include "XM_init.h"
#include "XM_net.h"
#include "XM_display.h"
#include "XM_protocol.h"
#include "../PTC_init.h"
#include "../Defmsg.h"



char *XMGetProtocolStr(void)
{
	return "XM";
}

char *XMRGetProjectStr(void)
{
	return "XiaMen";
}

#if 0
void ProtocolRelation(void *arg1,void *arg2)
{
	protocolstr = XMGetProtocolStr;
	projectstr = XMRGetProjectStr;
	defmsgdispaly = Defmsg_display;
	netSendback = XM_NetSendback;
	recv_process = XM_recv_process;
	protocol = XM_protocolProcessor;
	dspdefaultlst = XM_DefaultLst;
	EffectOption = XM_EffectOption;
} 



void ProtocolRelationDestroy(void)
{
	
}
#endif

