#include "PPL_init.h"
#include "PPL_net.h"
#include "PPL_display.h"
#include "PPL_protocol.h"
#include "../PTC_init.h"
#include "../Defmsg.h"

char *PPLGetProtocolStr(void)
{
	return "PPL";
}

char *PPLGetProjectStr(void)
{
	return "PerpleLight";
}
#if 0
void ProtocolRelation(void *arg1,void *arg2)
{
	defmsgdispaly = Defmsg_display;
	netSendback = PPL_NetSendback;
	recv_process = PPL_recv_process;
	protocol = PPL_protocolProcessor;
	dspdefaultlst = PPL_DefaultLst;
	EffectOption = PPL_EffectOption;

	protocolstr = PPLGetProtocolStr;
	projectstr = PPLGetProjectStr;
} 



void ProtocolRelationDestroy(void)
{
	
}
#endif

