#include "modbus_init.h"
#include "modbus_config.h"
#include "modbus_protocol.h"
#include "modbus_charparse.h"
#include "modbus_display.h"
#include "../PTC_init.h"
#include "modbus_net.h"
#include "../Defmsg.h"
#include "../../Hardware/Data_pool.h"
#include "conf.h"



char *MdbsGetProtocolStr(void)
{
	return "MODBUS";
}

char *MdbsGetProjectStr(void)
{
	return "WuHu";
}

void Mdbs_Init(void)
{
	uint8_t Byte = 0;

	if(access(modbusConfig,F_OK) < 0)
		return;

	//初始化最小间隔时间
	debug_printf("\n\n===============modbus config=================\n");
	char KeyVals[24];
	conf_file_read(modbusConfig,"IntervT","IntervT",KeyVals);
	//debug_printf("KeyVals = %s\n",KeyVals);
	DP_SetIntervTime(atoi(KeyVals));
	uint32_t IntervT;
	DP_GetIntervTime(&IntervT);
	debug_printf("IntervT = %d\n",IntervT);

	//初始化虚连接
	conf_file_read(modbusConfig,"VirtCNT","VirtCNT",KeyVals);
	Byte = (strncmp(KeyVals,"ON",2) == 0) ? 1 : 0;
	debug_printf("bYTE = %d\n",Byte);
	DP_SetVertConnect(Byte);

	//初始化亮度模式、最大亮度值、最小亮度值、以及当前开机的亮度值
	uint8_t Mode,Max,Min,CurVals;
	conf_file_read(modbusConfig,"Bright","mode",KeyVals);
	Mode = (strncmp(KeyVals,"AUTO",4) == 0) ? BRIGHT_AUTO : BRIGHT_HAND;
	DP_SetBrightMode(Mode);
	conf_file_read(modbusConfig,"Bright","max",KeyVals);
	Max = atoi(KeyVals);
	conf_file_read(modbusConfig,"Bright","min",KeyVals);
	Min = atoi(KeyVals);
	DP_SetBrightRange(Max,Min);
	conf_file_read(modbusConfig,"Bright","curvals",KeyVals);
	CurVals = atoi(KeyVals);
	CurVals = (CurVals <= Min) ? Min : CurVals;
	CurVals = (CurVals >= Max) ? Max : CurVals;

	float div = 0.0,fbright = 0.0;
	div = (Max - Min) / (float)32;
	fbright = (CurVals - Min) / div;
	CurVals = (fbright - (uint8_t)fbright > 0.5) ? ((uint8_t)fbright + 1) : ((uint8_t)fbright);
	debug_printf("Mode = %d,Max = %d,Min = %d,CurVals = %d\n",Mode,Max,Min,CurVals);
	DP_SaveBrightVals(CurVals);

	//初始化设备自检时间
	uint8_t hour,min,sec;
	conf_file_read(modbusConfig,"AutoCheckT","AutoCheckT",KeyVals);
	hour = atoi(KeyVals);
	min = atoi(KeyVals+3);
	sec = atoi(KeyVals+7);
	debug_printf("hour = %d,min = %d,sec = %d\n",hour,min,sec);
	DP_SetAutoCheckTime(hour,min,sec);

	//
	uint8_t unit = 0,cycle = 0;
	conf_file_read(modbusConfig,"AutoCheckT","unit",KeyVals);
	if(strncmp(KeyVals,"DAY",3) == 0)unit = 1;
	if(strncmp(KeyVals,"HOUR",4) == 0)unit = 2;
	if(strncmp(KeyVals,"MIN",3) == 0)unit = 3;
	conf_file_read(modbusConfig,"AutoCheckT","cycle",KeyVals);
	cycle = atoi(KeyVals);
	DP_SetAutoCheckUnit(unit,cycle);
	debug_printf("unit = %d,cycle = %d\n",unit,cycle);

	//初始化屏幕的模组的宽高
	uint16_t width = 0,height = 0;
	conf_file_read(modbusConfig,"module","width",KeyVals);
	width = atoi(KeyVals);
	conf_file_read(modbusConfig,"module","height",KeyVals);
	height = atoi(KeyVals);
	DP_SetModSize(width,height);
	debug_printf("width = %d,height = %d\n",width,height);


	//初始化光带
	uint8_t state = 0,sement = 0;
	
	conf_file_read(modbusConfig,"lightband","state",KeyVals);
	state = (strncmp(KeyVals,"enable",6) == 0) ? LBSTATE_OPEN : LBSTATE_CLOSE;
	conf_file_read(modbusConfig,"lightband","sement",KeyVals);
	sement = atoi(KeyVals);
	DP_SetLBandArg(state,sement);
	Mdbs_LBandArgInit(state,sement);
	debug_printf("state = %d,sement = %d\n",state,sement);

	debug_printf("=====================modbus end=========================\n\n");
}

#if 0
void ProtocolRelation(void *arg1,void *arg2)
{
	int lbstate = 0,lbsement = 0;
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
} 


void ProtocolRelationDestroy(void)
{
	
}

#endif

