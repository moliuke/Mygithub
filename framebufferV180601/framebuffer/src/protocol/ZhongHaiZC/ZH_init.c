#include <stdio.h>

#include "display.h"
#include "ZH_init.h"
#include "ZH_net.h"

#include "ZH_display.h"
#include "content.h"
#include "ZH_charparse.h"
#include "ZH_protocol.h"
#include "../PTC_init.h"
#include "../Defmsg.h"


char *ZHZCGetProtocolStr(void)
{
	DEBUG_PRINTF;
	return "ZHZC";
}

char *ZHZCGetProjectStr(void)
{
	return "ZhongHaiZhiChao";
}

#if 0
void ProtocolRelation(void *arg1,void *arg2)
{
	defmsgdispaly = Defmsg_display;
	EffectOption = ZH_EffectOption;
	dspdefaultlst = ZH_DefaultLst;
	netSendback = ZH_NetSendback;
	recv_process = ZH_recv_process;
	protocol = ZH_protocolProcessor;
	protocolstr = ZHZCGetProtocolStr;
	projectstr = ZHZCGetProjectStr;
} 

void ProtocolRelationDestroy(void)
{

}
#endif
