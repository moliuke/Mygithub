#include <stdio.h>
#include "../../Hardware/Data_pool.h"
#include "content.h"
#include "modbus_protocol.h"
#include "modbus_config.h"
#include "debug.h"
#include "content.h"
#include "modbus_display.h"
#include "../../module/mtimer.h"
#include "modbus_config.h"
#include "../../include/Dev_framebuffer.h"
#include "../../include/bitmap.h"
#include "modbus_lightband.h"
#include "../PTC_FileRxTx.h"
#include "modbus_charparse.h"
#include <sys/socket.h>
#include "../../Hardware/Data_pool.h"
#include "../../Hardware/HW3G_RXTX.h"
#include "../../Hardware/HW2G_400.h"
#include "conf.h"
#include "mtime.h"
#include "../../update.h"


static pthread_once_t init_create = PTHREAD_ONCE_INIT;
static stimer_t CommunitTimer,CheckSelfTimer;
static int InterTimeCount = 0;


//#define TIMEOUT_SCREEN_CLOSE



static void Mdbs_StartCheckSelf(void)
{
	uint16_t AllScreenTrouble = 0;
	uint8_t Modgroup[200] = {0x00};
	DP_SetCheckSelfTrouble(AllScreenTrouble,Modgroup,100);
}

void Mdbs_TimeoutPlayLst(void)
{
	struct stat modbusFile;
	uint16_t readSize = 0;
	FILE *PLST = NULL;
	uint8_t plstContent[1024];
	if(access(MODBUSDEFLST,F_OK) < 0)
		return;
	PLST = fopen(MODBUSDEFLST,"r+");
	if(PLST == NULL)
		return;
	fseek(PLST,0,SEEK_SET);
	readSize = fread(plstContent,1,1024,PLST);
	plstContent[readSize] = '\0';
	fclose(PLST);
	Mdbs_charparse(&content,plstContent,readSize);
	return;
	
}



static void *Mdbs_CommunitTimerACK(void *arg)
{
	uint32_t IntervTime = 0;
	uint8_t ledstate = 0;
	uint8_t vertconnect = 0;
	stimer_t *MdbsTimer = (stimer_t *)arg;
	//每10秒钟中断一次，一次计时10秒
	InterTimeCount += 10;

	//每中断一次就要重载一次最小通信间隔时间，因为最小通信间隔时间随时都有可能被重置
	DP_GetIntervTime(&IntervTime);
	//MdbsTimer->ref_vals = IntervTime;

	//计时超过最小通信间隔时间或者超时3分钟
	MDBS_protoparse_printf("InterTimeCount = %d,IntervTime = %d\n\n",InterTimeCount,IntervTime);
	if(IntervTime != 0 && InterTimeCount >= IntervTime)
	{
		InterTimeCount = IntervTime;
		MdbsTimer->counter = 0;
		//Mdbs_TimeoutPlayLst();
		//针对TXRX开屏
		DP_GetScreenStatus(&ledstate);
		if(ledstate == SLED_ON)
		{
			SET_LED_STATE(SLED_OFF);
			//针对扫描版开屏
			HW2G400_SetScreenStatus(SLED_OFF);
			//printf("SLED_OFF===========================================\n");
			//记录屏幕状态
			DEBUG_PRINTF;
			LEDstateRecord(SLED_OFF);
			DP_SetScreenStatus(SLED_OFF);
		}

	}
	else
	{
		//打开屏幕
		DP_GetScreenStatus(&ledstate);
		debug_printf("ledstate = %d\n",ledstate);
		//exit(1);
		DP_GetVertConnect(&vertconnect);
		if((ledstate == SLED_OFF) && (vertconnect == VIRTUAL_CLOSE))
		{
			//针对TXRX开屏
			SET_LED_STATE(SLED_ON);
			//针对扫描版开屏
			HW2G400_SetScreenStatus(SLED_ON);
			//printf("SLED_ON===========================================\n");
			//记录屏幕状态
			DEBUG_PRINTF;
			LEDstateRecord(SLED_ON);
			DP_SetScreenStatus(SLED_ON);
		}
	}

}

static int Mdbs_GetTimeRerfVals(uint8_t unit,uint8_t cycle)
{
	MDBS_protoparse_printf("unit = %d,cycle = %d\n",unit,cycle);
	switch(cycle)
	{
		case 1:return unit * 24 * 60 * 60;
		case 2:return unit * 60 * 60;
		case 3:return unit * 60;
	}
}


static void __modbus_TimerInit(void)
{
	char AbleStatus[12];
	char Timeout[8];
	char TimeoutAction[4];
	uint32_t *arg = NULL;

	//默认设置最小间隔时间600秒
	//DP_SetIntervTime(10);
	uint32_t Swidth,Sheight;
	DP_GetScreenSize(&Swidth,&Sheight);
	Mdbs_SetScreenSize(Swidth,Sheight);
	
	arg = (uint32_t *)malloc(sizeof(uint32_t));
	*arg = 1;
	
#ifdef TIMEOUT_SCREEN_CLOSE
	uint16_t IntervT = 0;
	DP_GetIntervTime(&IntervT);
	CommunitTimer.ref_vals = IntervT;
#else
	CommunitTimer.ref_vals = 10;
#endif
	CommunitTimer.counter  = 0;
	CommunitTimer.id       = 12;
	CommunitTimer.arg	  = (void *)arg;
	CommunitTimer.function = Mdbs_CommunitTimerACK;
	
	arg = NULL;
	mtimer_register(&CommunitTimer);

}

void Mdbs_timerInit(void)
{
	pthread_once(&init_create,__modbus_TimerInit);
}



static uint16_t MDBSCheckout_CRC(uint8_t *input,uint32_t Len)
{
	unsigned char CRC[2];
	uint16_t checkout = 0;
	uint16_t a,b,tmp,CRC16,V;
	
	CRC16 = 0xffff;							//初始值
	for(a = 0 ; a < Len ; a++)
	{
		tmp = *input & 0x00ff;
		input++;
		CRC16 ^= tmp;
		for(b= 0 ; b < 8 ; b++)
		{
			if(CRC16 & 0x0001)
			{
				CRC16 >>= 1;
				CRC16 ^= 0xa001;
			}
			else
				CRC16 >>= 1;
		}

	}
	tmp = 	CRC16;
	CRC[0] = (uint8_t)(CRC16 & 0x00ff);
	CRC[1] = (uint8_t)(CRC16 >> 8);
	V = CRC[0] << 8 | CRC[1];
	MDBS_protoparse_printf("CRC[0] = 0x%x,CRC[1] = 0x%x,V = 0x%x\n",CRC[0],CRC[1],V);
	return V;
}

static uint16_t MDBSCheckout_LRC(uint8_t *input,uint32_t Len)
{

#if 0
	uint8_t i;
	uint32_t k;
	uint8_t result;
	uint8_t LRCdata[1024];

	for(i = 0 ; i < Len ; i++)
	{
		if(input[i] > 0x40)
			LRCdata[i - 1] = input[i] - 0x41 + 10;
		else
			LRCdata[i - 1] = input[i] - 0x30;

	} 

	k = 0;
	for(i = 0 ; i < Len / 2 ; i++)
	{
		k += (LRCdata[2*i] * 16 + LRCdata[2*i+1]);
	}

	k = k % 256;
	k = 256 - k;
	result = k % 256;

	debug_printf("result = 0x%x\n",result);

	return result;
#else
	uint8_t LRC = 0;
	while(Len--)
		LRC += *input++;
	LRC = ((uint8_t)(-((char)LRC)));
	MDBS_protoparse_printf("LRC = 0x%x\n",LRC);
	return LRC;

#endif
}

static int MDBSCheckOut(uint8_t *input,uint32_t Len)
{
	uint16_t checkout = 0;
	uint8_t LRC;
#ifdef MODBUS_ASCII
	uint8_t subarg1 = 0,subarg2 = 0;
	subarg1 = (input[Len - 2] < 0x3a) ? 0x30 : 0x37;
	subarg2 = (input[Len - 1] < 0x3a) ? 0x30 : 0x37;
	
	LRC = (input[Len - 2] - subarg1) << 4 | input[Len - 1] - subarg2;
	MDBS_protoparse_printf("input[Len - 2] = 0x%x,input[Len - 1] = 0x%x,LRC = 0x%x\n",input[Len - 2],input[Len - 1],LRC);
	if(LRC != MDBSCheckout_LRC(input,Len - 2))
	{
		MDBS_PTCPARSEPRINTF;
		return -1;
	}
#else
	uint16_t CRC;
	MDBS_protoparse_printf("input[Len - 2] = 0x%x,input[Len - 1] = 0x%x\n",input[Len - 2],input[Len - 1]);
	CRC = input[Len - 2] << 8 | input[Len - 1];
	MDBS_protoparse_printf("CRC = 0x%x\n",CRC);
	if(CRC != MDBSCheckout_CRC(input,Len - 2))
	{
		MDBS_PTCPARSEPRINTF;
		return -1;
	}
#endif
	return 0;
}

static int MDBSGetBytes(uint8_t *input,uint32_t Len)
{
	uint32_t i = 0,j = 0;
	uint8_t *output = NULL;
	uint8_t subarg1 = 0,subarg2 = 0;

	if(input == NULL || Len == 0)
		return -1;
#ifdef MODBUS_ASCII
	output = (uint8_t *)malloc(Len * 2);
	for(i = 0,j = 0 ; i < Len - 1 ; i += 2,j += 1)
	{
		MDBS_protoparse_printf("input[i] = 0x%x,input[i+1] = 0x%x\n",input[i],input[i+1]);
		subarg1 = (input[i]   < 0x3a) ? 0x30 : 0x37;
		subarg2 = (input[i+1] < 0x3a) ? 0x30 : 0x37;
		output[j] = (input[i] - subarg1) << 4 | (input[i+1] - subarg2);
	}

	for(i = 0 ; i < j ; i++)
		MDBS_protoparse_printf("0x%x ",output[i]);
	MDBS_protoparse_printf("\n");
	
	memset(input,0,Len);
	memcpy(input,output,j);	
	return j;
#else
	return Len;
#endif
}


static int MDBSPreDealDate(uint8_t *input,uint32_t Len)
{	
	uint16_t CheckRCR = 0;
	int DataLen = -1;
	
	if(MDBSCheckOut(input,Len) < 0)
		return -1;

	MDBS_PTCPARSEPRINTF;
	if((DataLen = MDBSGetBytes(input,Len)) < 0)
		return -1;
	MDBS_PTCPARSEPRINTF;
	return DataLen;
}


//这是心跳包的反应，每来一帧有效帧数据，将对定时器清0并打开屏幕
static void Mdbs_ClearTimer(void)
{
	//最小通信间隔时间清0
	InterTimeCount = 0;

	//定时器清0
	mtimer_clear(12);
}



static void MODBSStructInit(MODBSStruct_t *MODBSStruct,uint8_t *input,uint32_t Len)
{
	MODBSStruct->TCPhead 	= input;
	MODBSStruct->DevAddr 	= input[6];
	MODBSStruct->FunCode 	= input[7];
	MODBSStruct->data	 	= input + 8;
	MODBSStruct->dataLen	= Len - 2 - 6;
	MODBSStruct->CheckOut	= 0x00;//input[Len - 2] << 8 | input[Len - 1];
}

static void MODBSStructPrintf(MODBSStruct_t *MODBSStruct)
{
	int i = 0; 
	debug_printf(
		"DevAddr:				0x%x\n"
		"FunCode:				0x%x\n"
		"dataLen:				%d\n",
		MODBSStruct->DevAddr,
		MODBSStruct->FunCode,
		MODBSStruct->dataLen);
	debug_printf("data: 			");
	for(i = 0 ; i < MODBSStruct->dataLen ; i ++)
	{
		debug_printf("%02x ",MODBSStruct->data[i]);
	}

	debug_printf("\n"
		"CheckOut:				%x\n",
		MODBSStruct->CheckOut
		);
		
}


static void MODBSStructToByte(MODBSStruct_t *MODBSStruct,uint8_t *output,uint32_t *outputLen)
{
	uint16_t i = 0;
	uint16_t CRC;
	uint16_t CRCpos;


#ifndef MODBUS_TCPIP 
	output[0] = MODBSStruct->DevAddr;
	output[1] = MODBSStruct->FunCode;
	CRC = MDBSCheckout_CRC(output,2 + MODBSStruct->dataLen);

	CRCpos = 2 + MODBSStruct->dataLen;
	output[CRCpos + 0] = (uint8_t)((CRC >> 8) & 0x00ff);
	output[CRCpos + 1] = (uint8_t)(CRC & 0x00ff);

	*outputLen = 2 + MODBSStruct->dataLen + 2;
#else
	output[6] = MODBSStruct->DevAddr;
	output[7] = MODBSStruct->FunCode;
	MDBS_protoparse_printf("MODBSStruct->dataLen = %d=============\n",MODBSStruct->dataLen);
	for(i = 0 ; i < MODBSStruct->dataLen ; i++)
		output[2 + 6 + i] = MODBSStruct->data[i];


	//*outputLen = 2 + 8 + MODBSStruct->dataLen;
	*outputLen = 8 + MODBSStruct->dataLen;
	output[4] = (uint8_t)((*outputLen - 6)/256);
	output[5] = (uint8_t)((*outputLen - 6)%256);
	

#endif
}

static int Mdbs_ReadPublicErea(uint16_t StartAddr,uint8_t ReadCNT,uint8_t *ReadData)
{
	uint8_t StartPos = 0;
	uint16_t ReadWord;
	uint8_t ReadByte;
	uint8_t Datapool[48];

	uint8_t i = 0;
	
	StartPos = (StartAddr - 0x0000) * 2; 

	memset(Datapool,0x00,sizeof(Datapool));

//模拟寄存器0 => 设备类型     //128 20
	Datapool[0]= 0x80+0x30;
	Datapool[1]= 0x14+0x30;
//模拟寄存器1 => 软件版本     //215 119
	Datapool[2]= 0xD7+0x30;
	Datapool[3]= 0x77+0x30;
//模拟寄存器2 => 通信协议版本信息 // 0 0
	Datapool[4]= 0x30;
	Datapool[5]= 0x30;
//模拟寄存器3 => 制造商编号 
	Datapool[6]= 0x30;
	Datapool[7]= 0x31;
//模拟寄存器04~11 => 设备序列号  0
	Datapool[8]= 0x30;
	Datapool[9]= (uint8_t)(0x30);			
	Datapool[10]= (uint8_t)(0x30);
	Datapool[11]=(uint8_t)(0x30);
	Datapool[12]=(uint8_t)(0x30);			
	Datapool[13]=(uint8_t)(0x30);
	Datapool[14]=(uint8_t)(0x30);
	Datapool[15]=(uint8_t)(0x30);		
	Datapool[16]=(uint8_t)(0x30);
	Datapool[17]=(uint8_t)(0x30);
	Datapool[18]=(uint8_t)(0x30);		
	Datapool[19]=(uint8_t)(0x30);
	Datapool[20]=(uint8_t)(0x30);
	Datapool[21]=(uint8_t)(0x30);
	Datapool[22]=(uint8_t)(0x30);
	Datapool[23]=(uint8_t)(0x30);	
//模拟寄存器12~15 => 设备名  xianke88
	strncpy(Datapool+24,"xianke88",8);
//模拟寄存器16~23 => 设备描述 swmb66
	strncpy(Datapool+32,"seeworgongkongji",16);
////////////获得需要的 寄存器 的反馈
	
	debug_printf("ReadCNT = %d\n",ReadCNT);
	for(i = 0 ; i < ReadCNT ; i++)
	{
		ReadData[i] = Datapool[StartPos + i];	
		debug_printf("%x ",ReadData[i]);
	}
	debug_printf("\n");

	

}

static int Mdbs_ReadGeneralErea(uint16_t StartAddr,uint8_t ReadCNT,uint8_t *ReadData)
{
	uint8_t StartPos = 0;
	uint16_t ReadWord;
	uint32_t intervtime;
	uint8_t ReadByte;
	uint8_t Datapool[48];

	uint8_t i = 0;
	
	StartPos = (StartAddr - 0x1000) * 2; 

	memset(Datapool,0x00,sizeof(Datapool));

	//寄存器0 => 最小通信间隔
	DP_GetIntervTime(&intervtime);
	Datapool[0] = (uint8_t)((intervtime >> 8) & 0xff);
	Datapool[1] = (uint8_t)(intervtime & 0xff);

	//寄存器1 => 虚拟连接模式
	DP_GetVertConnect(&ReadByte);
	Datapool[2] = 0x00;
	Datapool[3] = ReadByte;

	//寄存器2 => 亮度调节模式
	DP_GetBrightMode(&ReadByte);
	ReadByte = (ReadByte == BRIGHT_AUTO) ? MDBUS_BRIGHT_AUTO : MDBUS_BRIGHT_HAND;
	Datapool[4] = 0x00;
	Datapool[5] = ReadByte;

	//寄存器3 => 暗亮度
	DP_ReadBrightVals(&ReadByte);
	ReadByte = (ReadByte > 2) ? ((ReadByte - 2) / 2) : 0;
	Datapool[6] = 0x00;
	Datapool[7] = ReadByte;

	//寄存器4 => 黑屏/显示
	DP_GetScreenStatus(&ReadByte);
	Datapool[8] = 0x00;
	Datapool[9] = ReadByte;

	uint8_t SThour,STmin,STsec;
	DP_GetAutoCheckTime(&SThour,&STmin,&STsec);
	//寄存器5 => 自检 时/分
	Datapool[10] = (((SThour / 10) << 4) & 0xf0) | ((SThour % 10) & 0x0f);
	Datapool[11] = (((STmin / 10) << 4) & 0xf0) | ((STmin % 10) & 0x0f);

	//寄存器6 => 自检 秒
	Datapool[12] = 0x00;
	Datapool[13] = (((STsec / 10) << 4) & 0xf0) | ((STsec % 10) & 0x0f);

	//寄存器7 => 自检间隔单位/周期
	uint8_t STunit,STcycle;
	DP_GetAutoCheckUnit(&STunit,&STcycle);
	Datapool[14] = STunit;
	Datapool[15] = STcycle;

	//寄存器8 => 空
	Datapool[16]=0x00;
	Datapool[17]=0x00;

	////////////获取系统时间
	uint8_t systime[24];
	uint8_t systimeLen = 0;
	int year,month,day,hour,min,sec;
	memset(systime,0x00,sizeof(systime));
	get_sys_time(systime,&systimeLen);
	sscanf(systime,"%4d/%02d/%02d-%02d:%02d:%02d",&year,&month,&day,&hour,&min,&sec);
	MDBS_protoparse_printf("%d,%d,%d,%d,%d,%d\n",year,month,day,hour,min,sec);
	//寄存器9 => 年
	uint8_t yearB1,yearB2;
	yearB1 = year / 100;
	yearB2 = year % 100;
	Datapool[18] = (uint8_t)((((yearB1 / 10) << 4 ) & 0xf0) | ((yearB1 % 10) & 0x0f));
	Datapool[19] = (uint8_t)((((yearB2 / 10) << 4 ) & 0xf0) | ((yearB2 % 10) & 0x0f));

	//寄存器10 =>月&日	
	Datapool[20] = (uint8_t)((((month / 10) << 4 ) & 0xf0) | ((month % 10) & 0x0f));
	Datapool[21] = (uint8_t)((((day / 10) << 4 ) & 0xf0) | ((day % 10) & 0x0f));

	//寄存器11 =>时&分	
	Datapool[22] = (uint8_t)((((hour / 10) << 4 ) & 0xf0) | ((hour % 10) & 0x0f));
	Datapool[23] = (uint8_t)((((min / 10) << 4 ) & 0xf0) | ((min % 10) & 0x0f));

	//寄存器12 =>秒&空			
	Datapool[24] = (uint8_t)((((sec / 10) << 4 ) & 0xf0) | ((sec % 10) & 0x0f));
	Datapool[25] = 0x00;

	//寄存器13 => 文字型显示单元数量
	Datapool[26]=0x00;
	Datapool[27]=0x01;

	//寄存器14 => 光带显示单元数量
	Datapool[28]=0x00;
	Datapool[29]=0x00;

	//寄存器15 => 固定信息显示单元数量
	Datapool[30]=0x00;
	Datapool[31]=0x00;

	uint32_t Width,Height;
	DP_GetScreenSize(&Width,&Height);
	//模拟寄存器16 => 屏幕像素宽度
	//*(uint16_t *)&Datapool[32]=Width;//(uint8_t)(Width >> 8 & 0x000000ff);
	Datapool[32]=(uint8_t)(Width >> 8 & 0x000000ff);
	Datapool[33]=(uint8_t)(Width & 0x000000ff);
	debug_printf("Datapool[32] = %d,Datapool[33] = %d\n",Datapool[32],Datapool[33]);
	//模拟寄存器17 => 屏幕像素高度
	//*(uint16_t *)&Datapool[34]=Height;//(uint8_t)(Height >> 8 & 0x000000ff);
	Datapool[34]=(uint8_t)(Height >> 8 & 0x000000ff);
	Datapool[35]=(uint8_t)(Height & 0x000000ff);
	debug_printf("Datapool[34] = %d,Datapool[35] = %d\n",Datapool[34],Datapool[35]);
	//exit(1);
	debug_printf("ReadCNT = %d\n",ReadCNT);
	for(i = 0 ; i < ReadCNT ; i++)
	{
		ReadData[i] = Datapool[StartPos + i];	
		debug_printf("%02x ",ReadData[i]);
	}
	debug_printf("\n");

	return 0;
	
}


static int Mdbs_ReadDSPconfErea(uint16_t StartAddr,uint8_t ReadCNT,uint8_t *ReadData)
{
	uint8_t StartPos = 0;
	uint8_t Datapool[48];

	uint8_t i = 0;
	
	StartPos = (StartAddr - 0x1080) * 2; 
	memset(Datapool,0x00,sizeof(Datapool));
	//自检模组数量长-宽个数
	uint16_t Mwidth,Mheight;
	uint32_t Swidth,Sheight;
	DP_GetScreenSize(&Swidth,&Sheight);
	DP_GetModSize(&Mwidth,&Mheight);
	Datapool[0]=(uint8_t)((Mwidth && Swidth % Mwidth == 0) ? (Swidth / Mwidth) : (Swidth / Mwidth + 1));
	Datapool[1]=(uint8_t)((Mheight && Sheight % Mheight == 0) ? (Sheight / Mheight) : (Sheight / Mheight + 1));
	//文字显示单元数量
	uint8_t Nwidth = 0,Nheight = 0;
	uint16_t total = 0;
	Nwidth = Swidth / 16;	//横向能显示的最大的中文个数，按16号字体计算
	Nheight = Sheight / 16;	//纵向能显示的最大的中文行数，按16号字体计算
	total = Nwidth * Nheight;	//能显示的中文的总字数
	Datapool[2]=(uint8_t)((total & 0xff00) >> 8);
	Datapool[3]=(uint8_t)(total & 0x00ff);

	//点阵状状态模组
	Datapool[4]=Datapool[0];
	Datapool[5]=Datapool[1];

	if(StartPos + ReadCNT > 6)
		ReadCNT = 6 - StartPos;

	for(i = 0 ; i < ReadCNT ; i++)
	{
		ReadData[i] = Datapool[StartPos + i];	  
	}
	return 0;
}


static int Mdbs_ReadCheckselfErea(uint16_t StartAddr,uint8_t ReadCNT,uint8_t *ReadData)
{
	uint8_t StartPos = 0;
	uint8_t Datapool[256];

	uint8_t i = 0;
	memset(Datapool,0,sizeof(Datapool));
	StartPos = (StartAddr - 0x1100) * 2;
	//设备自检
	Datapool[0] = 0x00;
	Datapool[1] = 0x01;
	//文字型显示单元编号
	Datapool[2] = 0x00;
	Datapool[3] = 0x00;
	
	uint16_t Bwidth,Bheight;
	uint32_t Bnum;
	DP_GetBoxSize(&Bwidth,&Bheight,&Bnum);
	for(i = 0 ; i < Bnum ; i++)
		Datapool[4 + i] = 0x00;

	
	for(i = 0 ; i < ReadCNT ; i++)
		ReadData[i] = Datapool[StartPos + i];
	
	return 0;

}


static int Mdbs_ReadRealtimeErea(uint16_t StartAddr,uint8_t ReadCNT,uint8_t *ReadData)
{
	uint8_t StartPos = 0;
	uint8_t Datapool[48];
	uint8_t readCnt = ReadCNT;

	uint8_t i = 0;
	
	StartPos = (StartAddr - 0x1900) * 2;
	if(StartPos + readCnt > 50)
		readCnt = 50 - StartPos;

	memset(Datapool,0x00,sizeof(Datapool));
	//故障信息[0:无故障或保留位 1:当前存在故障]
	Datapool[0]=0x00;
	//显示状态[1:当前存在显示信息,工作于标准文字工作模式],更多见pdf文档4.3.1 显示状态值范围
	Datapool[1]=0x01;
	//软件故障号
	Datapool[2]=0x00;
	//硬件故障号
	Datapool[3]=0x00;

#if 0
	uint8_t  Content[128];
	uint8_t Font,Size,Inform,Order;
	uint16_t Len;
	uint32_t inSpeed,stayTime;
	memset(Content,0x00,sizeof(stayTime));
	DP_GetCurPlayContent(Content,&Len,&Font,&Size,&Inform,&inSpeed,&stayTime,&Order);
	MDBS_protoparse_printf("Font = %c,Size = %d,Inform = %d,inSpeed = %d,stayTime = %d,Order = %d\n",Font,Size,Inform,inSpeed,stayTime,Order);
#else
	DSPContent_t DSPContent;
	memset(&DSPContent,0,sizeof(DSPContent));
	DSPContent.type = DSPTYPE_STR;
	GetDSPContent(&DSPContent);
#endif
	//出字方式&间隔时间
	Datapool[4] = DSPContent.inform + 0x30;
	Datapool[5] = DSPContent.staytime / (1000 * 1000);
	
	//字体&大小
	Datapool[6] = DSPContent.fonttype;
	Datapool[7] = DSPContent.fontsize;

	
	memset(&DSPContent,0,sizeof(DSPContent));
	DSPContent.type = DSPTYPE_IMG;
	GetDSPContent(&DSPContent);
	
	//点阵屏图片编码，0:无图片；1-12:诱导图片
	Datapool[8] = atoi(DSPContent.mapName);
	//点阵屏图片类型，0-3
	switch(DSPContent.maptype)
	{
		case 24:Datapool[9] = 0;break;
		case 32:Datapool[9] = 1;break;
		case 48:Datapool[9] = 2;break;
		case 64:Datapool[9] = 3;break;
		default:break;
	}
	
	//第10个字节后面本来应该是当前正在显示的文字信息，现在全部默认为0，XP版的是这样的，我也不懂
	
	
	for(i=0 ; i < ReadCNT ; i++)
	{
		ReadData[i] = Datapool[StartPos + i];	  
	}

	return 0;
	
}


static int Mdbs_ReadExternErea(uint16_t StartAddr,uint8_t ReadCNT,uint8_t *ReadData)
{
	uint8_t StartPos = 0;
	uint8_t Datapool[48];
	uint8_t readCnt = ReadCNT;
	uint8_t i = 0;
	
	StartPos = (StartAddr - 0x3100) * 2;

	if((StartPos+readCnt)>50)
	{
		readCnt = 50-StartPos;
	}

	memset(Datapool,0x00,sizeof(Datapool));
	//设备状态
	Datapool[0]=0x00;
	Datapool[1]=0x00;
	//机箱温度
	Datapool[2]=0x3f;
	Datapool[3]=0x00;
	//Datapool默认已经置0，获得需要的寄存器的反馈
	for(i=0 ; i < readCnt ; i++)
	{
		ReadData[i] = Datapool[StartPos + i];	  
	}

	return 0;
	
}



static uint16_t ModgroupDSPNum = 0;	//设置模组故障编号
static uint16_t ModgroupTRCNum = 0;	//设置模组显示编号
static int Mdbs_ReadPixelsDSP(uint16_t StartAddr,uint8_t ReadCNT,uint8_t *ReadData)
{
	uint8_t StartPos = 0;
	uint8_t Datapool[512];

	uint8_t i = 0;
	
	StartPos = (StartAddr - 0x2000) * 2;

	memset(Datapool,0x00,sizeof(Datapool));

	//显示单元编号
	Datapool[0] = 0x00;
	Datapool[1] = 0x01;

	//Datapool[2] = (uint8_t)((ModgroupDSPNum & 0xff00) >> 8);
	//Datapool[3] = (uint8_t)((ModgroupDSPNum & 0x00ff));

	for(i = 0 ; i < ReadCNT ; i++)
		ReadData[i] = Datapool[StartPos + i];

	
	return 0;
}

static int Mdbs_ReadPixelsTRC(uint16_t StartAddr,uint8_t ReadCNT,uint8_t *ReadData)
{
	uint8_t StartPos = 0;
	uint8_t Datapool[512];

	uint8_t i = 0;
	
	StartPos = (StartAddr - 0x1b00) * 2;

	memset(Datapool,0x00,sizeof(Datapool));

	//显示单元编号
	Datapool[0] = 0x00;
	Datapool[1] = 0x01;

	//Datapool[2] = (uint8_t)((ModgroupTRCNum & 0xff00) >> 8);
	//Datapool[3] = (uint8_t)((ModgroupTRCNum & 0x00ff));

	for(i = 0 ; i < ReadCNT ; i++)
		ReadData[i] = Datapool[StartPos + i];
	return 0;
}




static int MDBS_Func03processor(MODBSStruct_t *MODBSStruct)
{
	uint16_t StartAddr,RegisterCNT;
	uint8_t Datapool[48];
	uint8_t StartPos = 0;
	uint16_t ReadWord;
	uint8_t ReadByte;
	uint8_t ccc = 0;
	uint8_t ReadCNT = 0;
	
	StartAddr   = MODBSStruct->data[0] << 8 | MODBSStruct->data[1];
	RegisterCNT = MODBSStruct->data[2] << 8 | MODBSStruct->data[3];

	ReadCNT = RegisterCNT * 2;
	MDBS_protoparse_printf("0x%x,0x%x\n",MODBSStruct->data[0],MODBSStruct->data[1]);
	MDBS_protoparse_printf("StartAddr = 0x%x,RegisterCNT = %d\n",StartAddr,RegisterCNT);
	
	memset(Datapool,0x00,sizeof(Datapool));

	//读取公共信息区 add 2020.6.2
	if(StartAddr >= 0x0000 && StartAddr <= 0x0018)
	{
		Mdbs_ReadPublicErea(StartAddr,ReadCNT,MODBSStruct->data + 1);
	}
	

	//读取通用功能区
	if(StartAddr >= 0x1000 && StartAddr <= 0x1012)
	{
		Mdbs_ReadGeneralErea(StartAddr,ReadCNT,MODBSStruct->data + 1);
	}

	//读取显示单元配置区
	if(StartAddr>=0x1080 && StartAddr<=0x1083)
	{
		Mdbs_ReadDSPconfErea(StartAddr,ReadCNT,MODBSStruct->data + 1);
		
	}

	//读取自检数据区
	if(StartAddr>=0x1100 && StartAddr <=0x1103)
	{
		DEBUG_PRINTF;
		Mdbs_ReadCheckselfErea(StartAddr,ReadCNT,MODBSStruct->data + 1);
	}

	//读取扩展区
	if(StartAddr >= 0x3100 && StartAddr <= 0x3103)
	{
		Mdbs_ReadExternErea(StartAddr,ReadCNT,MODBSStruct->data + 1);
	}

	//读取实时数据反馈
	if(StartAddr >= 0x4100 && StartAddr <= 0x4109)
	{
		Mdbs_ReadRealtimeErea(StartAddr,ReadCNT,MODBSStruct->data + 1);
	}

	
#if 0
	//读取实时数据区
	if(StartAddr >= 0x1900 && StartAddr <= 0x1909)
	{
		DEBUG_PRINTF;
		Mdbs_ReadRealtimeErea(StartAddr,ReadCNT,MODBSStruct->data + 1);
	}

	//获取点阵状态结果(显示)
	if(StartAddr >= 0x2000 && StartAddr <= 0x3000)
	{
		Mdbs_ReadPixelsDSP(StartAddr,ReadCNT,MODBSStruct->data + 1);
	}

	//获取点阵状态结果(故障)
	if(StartAddr >= 0x1b00 && StartAddr < 0x2000)
	{
		Mdbs_ReadPixelsTRC(StartAddr,ReadCNT,MODBSStruct->data + 1);
	}
#endif

	MODBSStruct->data[0] = ReadCNT;
	MODBSStruct->dataLen = ReadCNT + 1;

	debug_printf("MODBSStruct->dataLen = %d\n",MODBSStruct->dataLen);
	uint8_t i = 0;
	for(i = 0 ; i < MODBSStruct->dataLen ; i++)
		debug_printf("%02x ",MODBSStruct->data[i]);
	debug_printf("\n");

	return 0;
	
}


//显科协议中，自动亮度用0x31表示自动亮度，0x30表示手动亮，为了统一管理
//在modubus协议中也采用这两个数据，只是在上传给上位机时在转换成modbus的
static int Mdbs_SetBright(uint8_t *data)
{
	uint8_t Mode,Bright = 0;
	Mode = (data[1] == 1) ? BRIGHT_HAND : BRIGHT_AUTO;
	char writeval[4];

	debug_printf("%02x %x \n",data[0],data[1]);
	DP_SetBrightMode(Mode);
	if(Mode == BRIGHT_AUTO)
	{
		DEBUG_PRINTF;
		conf_file_write(modbusConfig,"Bright","mode","AUTO");
		return 0;
	}
	DEBUG_PRINTF;
	conf_file_write(modbusConfig,"Bright","mode","HAND");

	
	Bright = data[3];
	if(Bright <= 0)Bright = 0;
	if(Bright >= 31)Bright = 31;
	memset(writeval,0x00,sizeof(writeval));
	sprintf(writeval,"%d",Bright);
	conf_file_write(modbusConfig,"Bright","curvals",writeval);

	DP_SaveBrightVals(Bright);

	//针对扫描版
	HW2G400_SETLEDbright(Bright);

	//针对TXRX板的亮度设置
	uint8_t Bmax,Bmin;
	float div = 0.0,fbright = 0.0;
	DP_GetBrightRange(&Bmax,&Bmin);
	div = (Bmax - Bmin) / (float)32;
	fbright = Bright * div + Bmin;
	Bright = (fbright - (uint8_t)fbright > 0.5) ? ((uint8_t)fbright + 1) : ((uint8_t)fbright);
	Set_LEDBright(Bright);
	
	return 0;
}





static int MDBS_Func06processor(MODBSStruct_t *MODBSStruct)
{
	uint16_t StartAddr = MODBSStruct->data[0] << 8 | MODBSStruct->data[1];
	uint16_t SetData = MODBSStruct->data[2] << 8 | MODBSStruct->data[3];

	uint8_t ReadByte = 0;
	uint8_t IntervTime[8];
	uint8_t VirtCNT[4];
	char Unit[4],Cycle[4];
	switch(StartAddr)
	{
		//设置最小通信间隔时间
		case 0x1000:
			Mdbs_ClearTimer();
			DP_SetIntervTime(SetData);
			sprintf(IntervTime,"%d",SetData);
			conf_file_write(modbusConfig,"IntervT","IntervT",IntervTime);
			//DP_GetIntervTime(&IntervTime);
			//debug_printf("IntervTime = %d\n",IntervTime);
			break;
			
		//设置虚连接的开关,虚连接开--黑屏，虚连接关--恢复显示
		case 0x1001:
		{
			DP_SetVertConnect((uint8_t)SetData);
			if(SetData)
			{
				conf_file_write(modbusConfig,"VirtCNT","VirtCNT","ON");
				//针对TXRX的板子
				SET_LED_STATE(SLED_OFF);
				//针对扫描版
				HW2G400_SetScreenStatus((SLED_OFF));
				//记录屏幕状态
				DEBUG_PRINTF;
				LEDstateRecord(SLED_OFF);
				DP_SetScreenStatus(SLED_OFF);
				//printf("LED_OFF---------------------\n");
			}
				
			else
			{
				conf_file_write(modbusConfig,"VirtCNT","VirtCNT","OFF");
				//针对TXRX的板子
				SET_LED_STATE(SLED_ON);
				//针对扫描版
				HW2G400_SetScreenStatus((SLED_ON));
				//记录屏幕状态
				DEBUG_PRINTF;
				LEDstateRecord(SLED_ON);
				DP_SetScreenStatus(SLED_ON);
				//printf("LED_ON---------------------\n");
			}
			break;
		}

		//仅对自动亮度有效，手动亮度在功能码16中
		case 0x1002:
			DEBUG_PRINTF;
			Mdbs_SetBright(MODBSStruct->data + 2);
			break;
			
		//设置屏幕开关
		case 0x1004:
			DEBUG_PRINTF;
			//SetData = 1:开屏，否则关屏，针对TXRX的板子
			SET_LED_STATE((uint8_t)SetData);
			//针对扫描版
			HW2G400_SetScreenStatus((uint8_t)SetData);
			//记录屏幕状态
			DEBUG_PRINTF;
			LEDstateRecord((uint8_t)SetData);
			DP_SetScreenStatus((uint8_t)SetData);
			break;
		//暂时不用
		case 0x1005:
			break;
		//设定自检时间
		case 0x1007:
			DP_SetAutoCheckUnit(MODBSStruct->data[2],MODBSStruct->data[3]);
			memset(Unit,0x00,sizeof(Unit));
			memset(Cycle,0x00,sizeof(Cycle));
			sprintf(Unit,"%d",MODBSStruct->data[2]);
			sprintf(Cycle,"%d",MODBSStruct->data[3]);
			conf_file_write(modbusConfig,"AutoCheckT","unit",Unit);
			conf_file_write(modbusConfig,"AutoCheckT","cycle",Cycle);
			break;
		case 0x1009:break;
		default:break; 
	}
	MDBS_PTCPARSEPRINTF;
}














static int Mdbs_CheckSelf(uint8_t *Data)
{
	DP_SetCheckSelf(Data[1],Data[3]);
	Mdbs_StartCheckSelf();
	DP_SetCheckSelf(0,Data[3]);
	return 0;
}


static int Mdbs_SetCurTime(uint8_t *Data)
{
	uint8_t timestr[24];
	uint16_t year;
	uint8_t yearB1,yearB2,month,day,hour,min,sec;

	yearB1 = ((Data[0] & 0xf0) >> 4) * 10 + (Data[0] & 0x0f);
	yearB2 = ((Data[1] & 0xf0) >> 4) * 10 + (Data[1] & 0x0f);

	year = yearB1 * 100 + yearB2;
	month = ((Data[2] & 0xf0) >> 4) * 10 + (Data[2] & 0x0f);
	day = ((Data[3] & 0xf0) >> 4) * 10 + (Data[3] & 0x0f);
	hour = ((Data[4] & 0xf0) >> 4) * 10 + (Data[4] & 0x0f);
	min = ((Data[5] & 0xf0) >> 4) * 10 + (Data[5] & 0x0f);
	sec = ((Data[6] & 0xf0) >> 4) * 10 + (Data[6] & 0x0f);

	MDBS_protoparse_printf("yearB1 = %d,yearB2 = %d,year = %d,month = %d,day = %d,hour = %d,min = %d,sec = %d\n",yearB1,yearB2,year,month,day,hour,min,sec);
	sprintf(timestr,"%4d/%02d/%02d-%02d:%02d:%02d",year,month,day,hour,min,sec);
	set_sys_time(timestr,strlen(timestr));
	return 0;
}


static int Mdbs_SetCheckTime(uint8_t *Data)
{
	uint8_t hour,min,sec;
	char autotime[10];
	memset(autotime,0x00,sizeof(autotime));
	
	hour = (Data[0] >> 4) * 10 + (Data[0] & 0x0f);
	min = (Data[1] >> 4) * 10 + (Data[1] & 0x0f);
	sec = (Data[2] >> 4) * 10 + (Data[2] & 0x0f);

	debug_printf("hour = %d,min = %d,sec = %d\n",hour,min,sec);
	DP_SetAutoCheckTime(hour,min,sec);

	sprintf(autotime,"%02d:%02d:%02d",hour,min,sec);
	debug_printf("autotime = %s\n",autotime);
	conf_file_write(modbusConfig,"AutoCheckT","AutoCheckT",autotime);
}



//设置模组编号(故障)
static int Mdbs_SetPixelsTRCStatus(uint8_t *data)
{
	ModgroupTRCNum = data[2] << 8 | data[3];
	return 0;
}

//设置模组编号(显示)
static int Mdbs_SetPixelsDSPStatus(uint8_t *data)
{
	ModgroupDSPNum = data[2] << 8 | data[3];
	return 0;
}









static int Mdbs_LightBand(uint8_t *data)
{
	uint8_t unitCount = 0;
	uint8_t StartUnit = 0;
	
	unitCount = data[6] << 8 | data[7];
	StartUnit = data[4] << 8 | data[5]; 
	uint8_t i = 0;

	//检查光带是否使能，否则啥都不做
	if(!isLBopen())
		return -1;

	//检查光带段数书否非0
	if(!unitCount)
		return -1;

	Mdbs_SetLBandSement(StartUnit,unitCount,data[9]);
}






/*****************************************************************************************************************************
从机地址(1BYTE) | 功能码(1BYTE) | 起始地址(2BYTE) | 设置N个寄存器(2BYTE) | N个寄存器总字节数(1BYTE) | N个寄存器顺序值 | CRC
******************************************************************************************************************************/
static int MDBS_Func10processor(MODBSStruct_t *MODBSStruct)
{
	uint16_t StartAddr,RegisterCNT;
	StartAddr   = MODBSStruct->data[0] << 8 | MODBSStruct->data[1];
	RegisterCNT = MODBSStruct->data[2] << 8 | MODBSStruct->data[3];
	MDBS_protoparse_printf("StartAddr = 0x%x,RegisterCNT = 0x%x\n",StartAddr,RegisterCNT);

	//偏移5是有效数据的开始
	uint8_t *Content = MODBSStruct->data + 5;
	uint16_t Len = RegisterCNT * 2;

	uint8_t H,M,S;
	switch(StartAddr)
	{
		//设置手动亮度
		case SETBRIGHT_0X1002:
			Mdbs_SetBright(Content);
			break;
			
		//设置自检开始时间
		case SETAUTOCHECKTIME_0X1005:
			Mdbs_SetCheckTime(Content);
			break;

		//设定当前时间
		case SETCURTIME_0X1009:
			Mdbs_SetCurTime(Content);
			break;
			
		//启动自检
		case STARTCHECKSELF_0X1100:
			Mdbs_CheckSelf(Content);
			break;

		//响应光带动作
		case SETLIGHTBAND_0X1700:
			Mdbs_LightBand(Content);
			break;

			
		//芜湖项目不需要
#if 0		
		case DISPLAY_0X1500:
			Mdbs_WritePlayLst(Content,Len);
			Mdbs_charparse(&content,Content,Len);
			break;

		case SETPIXELSTRCSTATUS_0X1B00:
			Mdbs_SetPixelsTRCStatus(Content);
			break;
		case SETPIXELSDSPSTATUS_0X2000:
			Mdbs_SetPixelsDSPStatus(Content);
			break;
#endif

		default:
			break;
	}

	MODBSStruct->dataLen = 4;
	MDBS_PTCPARSEPRINTF;
	
}


static void dir_wintolinux(const char *dir)
{	
	char *p = NULL;
	debug_printf("B:dir = %s\n",dir);
	
	while((p = strchr(dir,'\\')) != NULL)
	{
		*p = '/';
		p += 1;
	}
	debug_printf("F:dir = %s\n",dir);
}


static void Dir_LetterBtoL(char *dir)
{
	char *charStr = dir;
	char *charp = charStr;
	while(*charp != '\0')
	{
		if(*charp >= 'A' && *charp <= 'Z')
			*charp = *charp + 0x20;
		charp += 1;
	}
}

static int GetFilePWD(uint8_t option,uint8_t *filename,uint8_t *filepwd)
{
	debug_printf("option = %d\n",option);
	switch(option)
	{
		case 0x31:
			strcat(filename,".bmp");
			dir_wintolinux(filename);
			Dir_LetterBtoL(filename);
			sprintf(filepwd,"%s/%s",bmp_dir,filename);
			break;
		case 0x32:
			strcat(filename,".jpg");
			dir_wintolinux(filename);
			Dir_LetterBtoL(filename);
			sprintf(filepwd,"%s/%s",jpg_dir,filename);
			break;
		case 0x33:
			strcat(filename,".gif");
			dir_wintolinux(filename);
			Dir_LetterBtoL(filename);
			sprintf(filepwd,"%s/%s",gif_dir,filename);
			break;
		default:
			break;
	}

	filepwd[strlen(filepwd)] = '\0';
	debug_printf("GetFilePWD : filepwd = %s\n",filepwd);
	return 0;
}

static int Mdbs_FileRX_2K(MODBSStruct_t *MODBSStruct)
{
	int ret = -1;
	uint8_t i = 0;
	uint8_t filename[8];
	char filePath[64];
	uint32_t FileOffset = 0;
	uint8_t *offset = MODBSStruct->data + 4;
	uint8_t *FileData = NULL;
	uint32_t FrameSize = 0;

	FILEUser_t FileUser;
	
	memset(filename,0,sizeof(filename));
	memcpy(filename,offset,3);
	

	memset(filePath,0,sizeof(filePath));
	//文件名称及后缀
	offset += 3;
	debug_printf("filename = %s\n",filename);
	
	GetFilePWD(*offset,filename,filePath);
	debug_printf("filePath = %s\n",filePath);
	
	//文件偏移
	offset += 1;
	FileOffset = (uint8_t)offset[0] << 24 | (uint8_t)offset[1] << 16 | (uint8_t)offset[2] << 8 | (uint8_t)offset[3];
	debug_printf("offset[0] = 0x%x,offset[1] = 0x%x,offset[2] = 0x%x,offset[3] = 0x%x,FileOffset = %d\n",offset[0],offset[1],offset[2],offset[3],FileOffset);

	//文件帧实际数据大小
	offset += 4;
	FrameSize = (uint8_t)offset[0] << 8 | (uint8_t)offset[1];
	debug_printf("FrameSize =%d\n",FrameSize);
	//文件数据
	offset += 2;
	FileData = offset;

	debug_printf("ZCPTCStruct->user->ip = %s\n",MODBSStruct->user->ip);
	memset(&FileUser,0,sizeof(FILEUser_t));
	FRTx_FileUserInit(&FileUser,MODBSStruct->user->type,MODBSStruct->user->ip,MODBSStruct->user->port,MODBSStruct->user->uartPort);

	if((ret = FRTx_FileFrameRx(&FileUser,filePath,FileData,FrameSize,FileOffset)) < 0)
		goto EXCEPTION;

	chmod(filePath,0744);

	EXCEPTION:
		MODBSStruct->dataLen  = 4;
		DEBUG_PRINTF;
		return -1;
	
}


static int Mdbs_FileTX_2K(MODBSStruct_t *MODBSStruct)
{
	int ret = -1;
	uint8_t i = 0;
	uint8_t filename[8];
	char filePath[64];
	uint32_t FileOffset = 0;
	uint8_t registerCnt[2];

	//有效数据从第四个字节开始
	uint8_t *offset = MODBSStruct->data + 4;
	//0x43功能码的返回值:前两个字节是寄存器的数量，后面是有效数据
	uint8_t *FileData = MODBSStruct->data + 2;
	uint32_t FrameSize = 0;
	uint32_t FrameLen = 0;

	FILEUser_t FileUser;

	for(i = 0 ; i < 16 ; i++)
		debug_printf("0x%x ",MODBSStruct->data[i]);
	debug_printf("\n");
	

	registerCnt[0] = MODBSStruct->data[2];
	registerCnt[1] = MODBSStruct->data[3];
	
	memset(filename,0,sizeof(filename));
	memcpy(filename,offset,3);

	memset(filePath,0,sizeof(filePath));
	//文件名称及后缀
	offset += 3;
	GetFilePWD(*offset,filename,filePath);
	debug_printf("filePath = %s\n",filePath);

	//文件偏移
	offset += 1;
	FileOffset = (uint8_t)offset[0] << 24 | (uint8_t)offset[1] << 16 | (uint8_t)offset[2] << 8 | (uint8_t)offset[3];
	debug_printf("offset[0] = 0x%x,offset[1] = 0x%x,offset[2] = 0x%x,offset[3] = 0x%x,FileOffset = %d\n",offset[0],offset[1],offset[2],offset[3],FileOffset);

	//文件帧实际数据大小
	offset += 4;
	FrameSize = (uint8_t)offset[0] << 8 | (uint8_t)offset[1];
	debug_printf("FrameSize =%d\n",FrameSize);
	//文件数据
	offset += 2;
	//FileData = offset;

	debug_printf("ZCPTCStruct->user->ip = %s\n",MODBSStruct->user->ip);
	memset(&FileUser,0,sizeof(FILEUser_t));
	FRTx_FileUserInit(&FileUser,MODBSStruct->user->type,MODBSStruct->user->ip,MODBSStruct->user->port,MODBSStruct->user->uartPort);
	
	if((ret = FRTx_FileFrameTx(&FileUser,filePath,FileData,&FrameLen,FileOffset)) < 0)
		return -1;

	MODBSStruct->dataLen = FrameLen + 2;
	MODBSStruct->data[0] = registerCnt[0];
	MODBSStruct->data[1] = registerCnt[1];	
	return 0;
}



/*******************************************************************************************
从机地址 | 功能码 | 起始地址 | 设置N个寄存器 | N个寄存器总字节数 | N个寄存器顺序值 | CRC
********************************************************************************************/
static int MDBS_Func41processor(MODBSStruct_t *MODBSStruct)
{
	uint16_t StartAddr,RegisterCNT;
	StartAddr   = MODBSStruct->data[0] << 8 | MODBSStruct->data[1];
	RegisterCNT = MODBSStruct->data[2] << 8 | MODBSStruct->data[3];

	uint8_t *Content = MODBSStruct->data + 4;
	uint16_t Len = RegisterCNT * 2;

	
	if(StartAddr == 0x1500)
	{
		Mdbs_WritePlayLst(Content,Len);
		DEBUG_PRINTF;
		Mdbs_charparse(&content,Content,Len);
	}

	MODBSStruct->dataLen = 4;

	debug_printf("MODBSStruct->dataLen = %d\n",MODBSStruct->dataLen);
	//芜湖不需要
#if 0
	if(StartAddr == 0x6000)
		return Mdbs_FileRX_2K(MODBSStruct);
#endif
}




static int MDBS_GetRealTimeMsg(MODBSStruct_t *MODBSStruct)
{
	
	uint8_t StartPos = 0;
	uint16_t Len = 0;
	uint16_t StartAddr,RegisterCNT;
	uint16_t ReadCNT = 0;
	uint8_t RTdata[1600];

	uint8_t LBstate = LBSTATE_CLOSE;
	uint8_t LBsment = 0;
	uint16_t txtLen = 0,lbLen = 0;
	
	StartAddr   = MODBSStruct->data[0] << 8 | MODBSStruct->data[1];
	RegisterCNT = MODBSStruct->data[2] << 8 | MODBSStruct->data[3];
	ReadCNT = RegisterCNT * 2;

	
	StartPos = (StartAddr - 0x4100) * 2; 

	memset(RTdata,0,sizeof(RTdata));
	Mdbs_GetTxtRealTimeData(RTdata,&txtLen); 
	//memcpy(MODBSStruct->data + 2,RTdata,Len);
	debug_printf("txtLen = %d\n",txtLen);

	//DP_GetLBandArg(&LBstate,&LBsment);
	if(isLBopen())
		Mdbs_GetLBandRealTimeData(RTdata + 1004,&lbLen);

	memcpy(MODBSStruct->data + 2,RTdata + StartPos,ReadCNT);

	debug_printf("ReadCNT = %d\n",ReadCNT);
	MODBSStruct->data[0] = (uint8_t)((RegisterCNT & 0xff00) >> 8);
	MODBSStruct->data[1] = (uint8_t)(RegisterCNT & 0x00ff);
	MODBSStruct->dataLen = ReadCNT + 2;
	return 0;
}



//获取实时故障信息，全部默认为无故障
static int Mdbs_GetRealtimeTrbMsg(MODBSStruct_t *MODBSStruct)
{
	uint8_t StartPos = 0;
	uint16_t Len = 0;
	uint16_t StartAddr,RegisterCNT;
	uint16_t ReadCNT = 0;
	uint8_t RTdata[1600];
	StartAddr   = MODBSStruct->data[0] << 8 | MODBSStruct->data[1];
	RegisterCNT = MODBSStruct->data[2] << 8 | MODBSStruct->data[3];
	ReadCNT = RegisterCNT * 2;

	StartPos = (StartAddr - 0x4100) * 2;
	int i = 0;
	for(i = 0 ; i < ReadCNT ; i++)
		MODBSStruct->data[i + 2] = 0x00;

	MODBSStruct->data[0] = (uint8_t)((RegisterCNT & 0xff00) >> 8);
	MODBSStruct->data[1] = (uint8_t)(RegisterCNT & 0x00ff);
	MODBSStruct->dataLen = ReadCNT + 2;
	
}
static int MDBS_Func43processor(MODBSStruct_t *MODBSStruct)
{
	uint16_t StartAddr   = MODBSStruct->data[0] << 8 | MODBSStruct->data[1];
	debug_printf("StartAddr = %x\n",StartAddr);
	//if(StartAddr == 0x7000)
	//	return Mdbs_FileTX_2K(MODBSStruct);

	if(StartAddr == 0x4100)
		return MDBS_GetRealTimeMsg(MODBSStruct);

	if(StartAddr >= 0x3102 && StartAddr <= 0x3104)
		return Mdbs_GetRealtimeTrbMsg(MODBSStruct);
}


int Mdbs_ProtocolParsing(user_t *user,uint8_t *input,uint32_t *inputlen)
{
	int Len = 0;
	int i = 0;
	uint8_t Virtual = VIRTUAL_CLOSE;
	uint16_t StartAddr = 0;
	MODBSStruct_t MODBSStruct;


#if 0
	DEBUG_PRINTF;
	debug_printf("*inputlen = %d\n",*inputlen);
	if((Len = MDBSPreDealDate(input,*inputlen)) < 0)
		return -1;
	DEBUG_PRINTF;
	debug_printf("Len = %d\n",Len);
#endif

	Mdbs_ClearTimer();
	
	memset(&MODBSStruct,0x00,sizeof(MODBSStruct));
	MODBSStruct.user = user;
	MODBSStructInit(&MODBSStruct,input,*inputlen);
	debug_printf("\n========================RECV msg============================\n");
	MODBSStructPrintf(&MODBSStruct);

	StartAddr = MODBSStruct.data[0] << 8 | MODBSStruct.data[1];
	DP_GetVertConnect(&Virtual);
	//处于虚连接状态下，只有读取通用功能和设置虚连接状态不受限
	MDBS_protoparse_printf("Virtual = %d,StartAddr = 0x%x\n",Virtual,StartAddr);
	if(Virtual == VIRTUAL_OPEN && StartAddr != 0x1000 && StartAddr != 0x1001)
		goto EXECPTION;
	MDBS_PTCPARSEPRINTF;

	debug_printf("MODBSStruct.FunCode = %x============\n",MODBSStruct.FunCode);
	if(MODBSStruct.FunCode != 0x03 && MODBSStruct.FunCode != 0x06 && MODBSStruct.FunCode != 0x10 && 
	   MODBSStruct.FunCode != 0x41 && MODBSStruct.FunCode != 0x43)
	  goto EXECPTION;
	
	switch(MODBSStruct.FunCode)
	{
		case 0x03:
			MDBS_PTCPARSEPRINTF;
			MDBS_Func03processor(&MODBSStruct);
			MDBS_protoparse_printf("MODBSStruct.dataLen = %d\n",MODBSStruct.dataLen);
			break;
		case 0x06:
			MDBS_Func06processor(&MODBSStruct);
			break;
		case 0x10:
			MDBS_Func10processor(&MODBSStruct);
			break;
		case 0x41:
			MDBS_Func41processor(&MODBSStruct);
		case 0x43:
			MDBS_Func43processor(&MODBSStruct);
			break;
		default:
			break;
	}
	


	debug_printf("\n========================ACK msg====MODBSStruct->dataLen = %d========================\n",MODBSStruct.dataLen);
	MODBSStructPrintf(&MODBSStruct);
	MODBSStructToByte(&MODBSStruct,input,inputlen);
	
	return 0;

	EXECPTION:
		*inputlen = 6;
		return 0;
		
}


int modbs_protocolProcessor(user_t *user,uint8_t *input,uint32_t *inputlen)
{
	//DEBUG_PRINTF;
	/*************************************************************************************/
//每个协议都可以加这条
	uint8_t vindicate[9] = {0x02,0x39,0x30,0x30,0x30,0x30,0x7E,0x18,0x03};
	uint8_t reply[9] = {0x02,0x39,0x30,0x30,0x30,0x01,0x58,0x6A,0x03};
	if(memcmp(input,vindicate,9)==0)
	{

		//回复上位机 
		if(user->type == 0) //网口
			send(user->fd,reply,9,0);
		else if(user->type == 1)
			uart_send(0,reply,9);  //xCOM1

		
		char buf_frist[16];
		char buf_second[16];
		char content[256];

		memset(buf_frist,0,sizeof(buf_frist));
		memset(buf_second,0,sizeof(buf_second));
		memset(content,0,sizeof(content));

		conf_file_read(CurrentPtcFile,"protocol","protocol",buf_frist);
		conf_file_read(CurrentPtcFile,"protocol","swr_protocol",buf_second);

		debug_printf("buf_frist is %s  buf_second is %s\n",buf_frist,buf_second);
		sprintf(content,"[protocol]\nprotocol = %s\nswr_protocol = %s\n",buf_frist,buf_second);
		debug_printf("%s\n",content);
		//保存升级之前的协议，用于升级完成后，切换回原来的协议
		FILE *IPF = fopen(RecordPtcFile,"wb+");
		if(IPF == NULL)
			return -1;
		fwrite(content,1,sizeof(content),IPF);
		fflush(IPF);
		fclose(IPF);
		
		conf_file_write(CurrentPtcFile,"protocol","protocol","upgrade");
		conf_file_write(CurrentPtcFile,"protocol","swr_protcol","general");
		system("killall ledscreen");
	}	

/*******************************************************************************************/
	if(input[0] == 0x02 && input[*inputlen - 1] == 0x03)	
	{
		MDBS_PTCPARSEPRINTF;
		//return prtcl_protocl_parsing(user,input,inputlen);
	}
	MDBS_PTCPARSEPRINTF;
	return Mdbs_ProtocolParsing(user,input,inputlen);
}





