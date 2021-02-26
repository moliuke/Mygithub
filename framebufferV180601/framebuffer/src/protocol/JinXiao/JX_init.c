#include "JX_init.h"
#include "JX_net.h"
#include "JX_display.h"
#include "JX_protocol.h"
#include "../PTC_init.h"
#include "../Defmsg.h"



char *JXGetProtocolStr(void)
{
	return "JX";
}

char *JXRGetProjectStr(void)
{
	return "JinXiao";
}

#if 0
void ProtocolRelation(void *arg1,void *arg2)
{
	protocolstr = JXGetProtocolStr;
	projectstr = JXRGetProjectStr;
	defmsgdispaly = Defmsg_display;
	netSendback = JX_NetSendback;
	recv_process = JX_recv_process;
	protocol = JX_protocolProcessor;
	dspdefaultlst = JX_DefaultLst;
	EffectOption = JX_EffectOption;
} 




void ProtocolRelationDestroy(void)
{
	
}
#endif



