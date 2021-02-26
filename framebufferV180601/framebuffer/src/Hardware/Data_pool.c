#include "Data_pool.h"
#include "../include/config.h"
#include "content.h"
#include "mtime.h"
#include "conf.h"

uint8_t Lbrightflag = 0;  //光敏异常标志位，0为正常，1为异常

static DATAPool_t DATAPool = 
{
	.TMPstatus  = 0x31,
	.TMPvals	= 34,

	.PWstatus	= 0x31,
	.PWvals		= 220,

	.Dstatus	= 0x31,
	.Dvals 		= 0x30,

	.PIXstatus	= 0x31,
	.PIXmode    = 0x00,
	.PIXdata	= NULL,
	.PIXvals	= 0,
	.PIXdataLen	= 0,

	.LSensA 	= 23,
	.LSensB		= 26,

	.LSstatus	= 0x31,
	.LSvals		= 0x20,

	.THDstatus	= 0x31,
	.THDvals	= 0,

	.CHNstatus	= 0x31,
	.CHNvals	= 1,

	.FANstatus	= 0x31,
	.FANvals	= 0,

	.SMKstatus	= 0x31,
	.SMKvals	= 0,

	.SofStatus	= 0x31,
	.Sofvals	= 1,

	.HardStatus	= 0x31,
	.Hardvals	= 1,

	.SysStatus	= 0x31,
	.Sysvals	= 1,

	.Xoffset = 0,	//显示屏的横向与纵向的偏移
	.Yoffset = 0,

	.CommuStatus= 0x31,
	.Commuvals	= 1,

	.ScrSwitch = 1,

	.YLstatus = 0x03,	//不确定状态

	.Tempvals = 0x146,	//32.6摄氏度
	.HUMvals = 0x320,	//80%


	//modbus protocol
	.IntervTime = 600,	//最小间隔时间默认是600s,即10分钟
	.VertConnect = 1,	//虚连接状态默认为连接状态

	//modbus 默认每天12:00自检一次
	.SThour 	= 12,
	.STmin  	= 0,
	.STsec  	= 0,
	.STunit 	= 1,
	.STcycle	= 3,

	//modbus 设备自检，默认不启动自检
	.CheckSelf  = 0,
	.Dspunit    = 1,

	.MaxBright  = 31,
	.MinBright  = 1
};


void DP_DATAPoolPrintf(void)
{
	debug_printf("\n\n\n===================DP_DATAPoolPrintf=====================\n"
				 "SCREEN: %d, %d\n"
				 "RES: %d, %d, %d\n"
				 "BOX: %d, %d %d\n"
				 "COM1: %d, %d, %d, %c, %c\n"
				 "COM2: %d, %d, %d, %c, %c\n"
				 "NET: %s, %d, %s, %s\n"
				 "BRIGHT: 0x%x, %d, %d, %d\n"
				 "PLIST: %s\n"
				 "\n\n\n",
				 DATAPool.SCRwidth,DATAPool.SCRheight,
				 DATAPool.RESwidth,DATAPool.RESheight,DATAPool.RESbits,
				 DATAPool.BOXwidth,DATAPool.BOXheight,DATAPool.BOXnum,
				 DATAPool.COMx[0].COMbaudrate,DATAPool.COMx[0].COMdatabits,DATAPool.COMx[0].COMstopbits,DATAPool.COMx[0].COMflowctl,DATAPool.COMx[0].COMparity,
 				 DATAPool.COMx[1].COMbaudrate,DATAPool.COMx[1].COMdatabits,DATAPool.COMx[1].COMstopbits,DATAPool.COMx[1].COMflowctl,DATAPool.COMx[1].COMparity,
 				 DATAPool.NETip,DATAPool.NETprot,DATAPool.NETmask,DATAPool.NETgw,
 				 DATAPool.BRTmode,DATAPool.BRTvals,DATAPool.BRTmax,DATAPool.BRTmin,
 				 DATAPool.CPlist
		);
}


//add by mo 2021/01/04
//加入显示最小最大亮度值，通过网页配置该值
void DP_SetBrightRealRange(uint8_t max, uint8_t min)
{
	DATAPool.MaxBright = max;
	DATAPool.MinBright = min;
}
void DP_GetBrightRealRange(uint8_t *max, uint8_t *min)
{
	*max = DATAPool.MaxBright;
	*min = DATAPool.MinBright;
}


void DP_SetBootupTime(uint8_t *time,uint8_t Len)
{
	memset(DATAPool.BOOTtime,0,sizeof(DATAPool.BOOTtime));
	memcpy(DATAPool.BOOTtime,time,Len);
}

int DP_GetBootupTime(uint8_t *time,uint8_t *Len)
{
	if(time == NULL || Len == NULL)
		return -1;
	
	memcpy(time,DATAPool.BOOTtime,strlen(DATAPool.BOOTtime));
	*Len = strlen(DATAPool.BOOTtime);
	debug_printf("time = %s\n",time);

	return 0;
}



void DP_SaveBrightVals(uint8_t Bright)
{
	DATAPool.BRTvals 	= Bright;
}

int DP_ReadBrightVals(uint8_t *Bright)
{
	*Bright = DATAPool.BRTvals;
	return 0;
}
void DP_SetBrightMode(uint8_t Bmode)
{
	DATAPool.BRTmode	= Bmode;
}

int DP_GetBrightMode(uint8_t *Bmode)
{
	*Bmode = DATAPool.BRTmode;
	return 0;
}


void DP_SetBrightRange(uint8_t Bmax,uint8_t Bmin)
{
	DATAPool.BRTmax 	= Bmax;
	DATAPool.BRTmin 	= Bmin;
}

int DP_GetBrightRange(uint8_t *Bmax,uint8_t *Bmin)
{
	*Bmax = DATAPool.BRTmax;
	*Bmin = DATAPool.BRTmin;
	return 0;
}


//这里加入几个函数
//加入协议设置和获取
//一级协议
void DP_SetProcotol(uint8_t Flag)
{
	DATAPool.Pctl = Flag;
}
//二级协议
void DP_Set_Procotol(uint8_t Flag)
{
	DATAPool.pctl = Flag;
}
//一级协议
int DP_GetProcotol(uint8_t *Flag)
{
	*Flag = DATAPool.Pctl;
	return 0;
}
//二级协议
int DP_Get_Procotol(uint8_t *Flag)
{
	*Flag = DATAPool.pctl;
	return 0;
}

//获取版本

int DP_Get_APPVersion(uint8_t *Pdata)
{
	Pdata[4] = DATAPool.APP_Mver;
	Pdata[5] = DATAPool.APP_Sver;
}
int DP_Get_MonitorVersion(uint8_t *Pdata)
{
	Pdata[2] = DATAPool.MIR_Mver;
	Pdata[3] = DATAPool.MIR_Sver;
}
int DP_Get_TXVersion(uint8_t *Pdata)
{
	Pdata[6] = DATAPool.TX_Mver;
	Pdata[7] = DATAPool.TX_Sver;
}
int DP_Get_RXVersion(uint8_t *Pdata)
{
	int i = 0;
	Pdata[1] = DATAPool.RXnum;	
	for(i = 0;i < DATAPool.RXnum;i++)
	{
		Pdata[8+i*2] = DATAPool.RXcard[i].Mver;
		Pdata[9+i*2] = DATAPool.RXcard[i].Sver;
	}	
}

//获取接收卡数量和版本号
int DP_Get_ALLRXVersion(uint8_t *Pdata,uint8_t *num)
{
	int i = 0;
	*num = DATAPool.RXnum;	
	for(i = 0;i < DATAPool.RXnum;i++)
	{
		Pdata[i*2] = DATAPool.RXcard[i].Mver;
		Pdata[1+i*2] = DATAPool.RXcard[i].Sver;
		//printf("%02X %02X ",Pdata[i*2],Pdata[1+i*2]);
	}	
	//printf("\n");
}



int DP_Get_TXRXVersion(uint8_t *Pdata)
{
	Pdata[0] = DATAPool.TX_Mver;
	Pdata[1] = DATAPool.TX_Sver;
	Pdata[2] = DATAPool.RXcard[0].Mver;
	Pdata[3] = DATAPool.RXcard[0].Mver;
}

//设置工控机和守护程序的版本号
int DP_Set_APPVersion(void)
{
	uint8_t mver = MAJOR;
	mver &= 0x0F;
	uint8_t sver = MINOR;
	sver &= 0x0F;
	DATAPool.APP_Mver = (mver << 4) | sver;

	mver = REVISION1;
	mver &= 0x0F;
	sver = REVISION2;
	sver &= 0x0F;
	DATAPool.APP_Sver = (mver << 4) | sver;


	debug_printf("APPVersion is %02X %02X\n",DATAPool.APP_Mver,DATAPool.APP_Sver);
	return 0;
}

int DP_Set_MonitorVersion(void)
{
	FILE *fp = NULL;
	int i = 0;
	char content[64];
	char *pdata;
	char *ptmp;
	uint8_t mver;
	uint8_t sver;
	if(access("/home/LEDscr/version/boot.v",F_OK) < 0)
	{
		debug_printf("boot.v is not exist\n");
		return -1;
	}
	if((fp = fopen("/home/LEDscr/version/boot.v","r")) == NULL)
	{
		debug_printf("boot.v opened is failed\n");
		return -1;
	}
	fseek(fp,0,SEEK_SET);
	fread(content,64,1,fp);
	fclose(fp);
    //bootupV1.1.7  这里存在漏洞
	pdata = content;
	ptmp = strstr(pdata,".");
	if(ptmp == NULL)
		return -1;
	pdata = ptmp - 1;
	mver = (unsigned char)atoi(pdata);
	mver &= 0x0F;

	if(mver >= 10)
		pdata = ptmp + 2;
	else
		pdata = ptmp + 1;
	
	ptmp = strstr(pdata,".");
	if(ptmp == NULL)
		return -1;
	sver = (unsigned char)atoi(pdata);
	sver &= 0x0F;

	DATAPool.MIR_Mver = (mver << 4) | (sver);
	
	if(sver >= 10)
		pdata = ptmp + 2;
	else
		pdata = ptmp + 1;

	mver = (unsigned char)atoi(pdata);
	sver &= 0x0F;
	sver = 0x00;
	DATAPool.MIR_Sver = (mver << 4) | (sver);


	debug_printf("MonitorVersion is %02X %02X\n",DATAPool.MIR_Mver,DATAPool.MIR_Sver);
	return 0;	
}





//加入TX和RX的升级模式(相当BootLoader)
void DP_Set_TX_Mode(uint8_t state)
{
	DATAPool.TX_upgrade_mode = state;

}

int DP_Get_TX_Mode(uint8_t *state)
{
	*state = DATAPool.TX_upgrade_mode;
	return 0;
}
void DP_Set_RX_Mode(uint8_t state)
{
	DATAPool.RX_upgrade_mode = state;

}

int DP_Get_RX_Mode(uint8_t *state)
{
	*state = DATAPool.RX_upgrade_mode;
	return 0;
}

//加入TX和RX的升级标志
void DP_Set_TX_Flag(uint8_t state)
{
	DATAPool.TX_upgrade_flag = state;

}

int DP_Get_TX_Flag(uint8_t *state)
{
	*state = DATAPool.TX_upgrade_flag;
	return 0;
}
void DP_Set_RX_Flag(uint8_t state)
{
	DATAPool.RX_upgrade_flag = state;

}

int DP_Get_RX_Flag(uint8_t *state)
{
	*state = DATAPool.RX_upgrade_flag;
	return 0;
}

//加入TX和RX的启动地址（00,06,0C,12）
void DP_Set_TX_Boot_Address(uint8_t state)
{
	DATAPool.TX_boot_address = state;

}

int DP_Get_TX_Boot_Address(uint8_t *state)
{
	*state = DATAPool.TX_boot_address;
	return 0;
}
void DP_Set_RX_Boot_Address(uint8_t state)
{
	DATAPool.RX_boot_address = state;

}

int DP_Get_RX_Boot_Address(uint8_t *state)
{
	*state = DATAPool.RX_boot_address;
	return 0;
}


//加入TX和RX的升级地址（00,06,0C,12）00是FAC（出厂）模式下的，尽量不要修改FAC的程序
void DP_Set_TX_Upgrade_Address(uint8_t state)
{
	DATAPool.TX_upgrade_address = state;

}

int DP_Get_TX_Upgrade_Address(uint8_t *state)
{
	*state = DATAPool.TX_upgrade_address;
	return 0;
}
void DP_Set_RX_Upgrade_Address(uint8_t state)
{
	DATAPool.RX_upgrade_address = state;

}

int DP_Get_RX_Upgrade_Address(uint8_t *state)
{
	*state = DATAPool.RX_upgrade_address;
	return 0;
}

//需要提取整屏宽和高

int DP_Set_Display_Parameter(uint8_t *data,int len)
{
	
	memset(DATAPool.prm_msg,0,sizeof(DATAPool.prm_msg));
	memcpy(DATAPool.prm_msg,data,len);
}
//需要提取整屏宽和高

int DP_Get_Display_Parameter(uint8_t *data,uint32_t *len)
{
	*len = DATAPool.prm_len;
	#if 0
	debug_printf("len is %d\n",*len);
			int i = 0;
		for(;i < DATAPool.prm_len;i++)
		{
			debug_printf("%02X ",DATAPool.prm_msg[i]);
		}
	#endif
	if(DATAPool.prm_msg != NULL && (*len) != 0)
	{
		

		memcpy(data,DATAPool.prm_msg,DATAPool.prm_len);
			
		return 0;
	}
	return -1;
}




//加入发送卡或接收卡配置参数数据解析
int DP_RXTX_ConfigureParsing(uint8_t *Data,uint16_t Len)
{
	
	//加入TX和RX的升级标志
	//加入TX和RX的启动地址（00,06,0C,12）
	//加入TX和RX的升级地址（00,06,0C,12）00是FAC（出厂）模式下的，尽量不要修改FAC的程序
	if(Data[0] == 0x01)
	{

		DATAPool.TX_upgrade_mode = Data[1];
		DATAPool.TX_upgrade_flag = Data[2];
		if(Data[3] == 0x00 || Data[3] == 0x06 || Data[3] == 0x0C || Data[3] == 0x12)
		{
			DATAPool.TX_boot_address = Data[3];
		}
		if(Data[4] == 0x00 || Data[4] == 0x06 || Data[4] == 0x0C || Data[4] == 0x12)
		{
			DATAPool.TX_upgrade_address = Data[4];
		}	

		DATAPool.RX_upgrade_mode = Data[5];
		DATAPool.RX_upgrade_flag = Data[6];
		if(Data[7] == 0x00 || Data[7] == 0x06 || Data[7] == 0x0C || Data[7] == 0x12)
		{
			DATAPool.RX_boot_address = Data[7];
		}
		if(Data[8] == 0x00 || Data[8] == 0x06 || Data[8] == 0x0C || Data[8] == 0x12)
		{
			DATAPool.RX_upgrade_address = Data[8];
		}	

		return 0;

	}
	return -1;
	
}



int DP_RXcardDataParsing(uint8_t *DataFromTXcard,uint16_t Len)
{
	uint8_t i = 0;
	uint8_t Light1,Light2,Thander;
		//光敏状态
	DATAPool.BOXnum = DataFromTXcard[RXNUM_POS];
	DATAPool.RXnum  = DataFromTXcard[RXNUM_POS];
	DATAPool.LSensA = DataFromTXcard[LIGHT1_POS];
	DATAPool.LSensB = DataFromTXcard[LIGHT2_POS];
	if(DATAPool.LSensA > 64 || DATAPool.LSensA < 0 || DATAPool.LSensB > 64 || DATAPool.LSensB < 0)
		DATAPool.LSstatus = 0x30;
	else
		DATAPool.LSstatus = 0x31;
	debug_printf("DATAPool.LSensA = %d,DATAPool.LSensB = %d,DATAPool.LSstatus = 0x%x\n",DATAPool.LSensA,DATAPool.LSensB,DATAPool.LSstatus);

	char timestr[24];
	uint8_t len = 0;
	memset(timestr,0,sizeof(timestr));
	get_sys_time(timestr,&len);
	uint8_t curtime = atoi(timestr+11);
	//printf("curtime is %d\n",curtime);
	char buffer[4];
	memset(buffer,0,sizeof(buffer));
	//获取白天和黑夜设置的亮度
	conf_file_read(AUTOBRIGHT,"daytime","brightvlaue",buffer);
	uint8_t daytimevalue = atoi(buffer);
	conf_file_read(AUTOBRIGHT,"night","brightvlaue",buffer);
	uint8_t nightvalue = atoi(buffer);
	if(DATAPool.LSensA == 0x00 && DATAPool.LSensB == 0x00)
	{
		DEBUG_PRINTF;
		Lbrightflag = 1;
		if(DATAPool.BRTmode == BRIGHT_AUTO)
		{
			//白天两个光敏值都为0时有可能光敏坏了,给一个默认的亮度值
			if(curtime > 6 && curtime < 18)
			{
				DEBUG_PRINTF;
				DATAPool.LSvals = daytimevalue;
				//printf("白天 %d\n",daytimevalue);
				//Set_LEDBright(daytimevalue);
			}
			else
			{
				DATAPool.LSvals = nightvalue;
				//Set_LEDBright(nightvalue);
				//printf("黑夜 %d\n",nightvalue);
			}
		}
	}



	
	else if(DATAPool.LSensA == 0x00 && DATAPool.LSensB != 0x00)
	{
		DATAPool.LSvals = DATAPool.LSensB;
		#if 0
		if(Lbrightflag == 1 && DATAPool.BRTmode == BRIGHT_AUTO)
		{
			Lbrightflag = 0;
			//设置为自动模式
			Set_LEDBright(DATAPool.LSvals);
		}
		#endif
			
	}
	else if(DATAPool.LSensA != 0x00 && DATAPool.LSensB == 0x00)
	{
		DATAPool.LSvals = DATAPool.LSensA;
		#if 0
		if(Lbrightflag == 1 && DATAPool.BRTmode == BRIGHT_AUTO)
		{
			Lbrightflag = 0;
			Set_LEDBright(DATAPool.LSvals);
		}
		#endif
	}
	else
	{
		DATAPool.LSvals = (DATAPool.LSensA + DATAPool.LSensB) >> 1;
		#if 0
		if(Lbrightflag == 1 && DATAPool.BRTmode == BRIGHT_AUTO)
		{
			Lbrightflag = 0;
			Set_LEDBright(DATAPool.LSvals);
		}
		#endif
	}

	
	//当前亮度值
	if(DATAPool.BRTmode == BRIGHT_AUTO && Lbrightflag == 0)
		DATAPool.BRTvals = DataFromTXcard[LIGHT_VATAL];
	else if(DATAPool.BRTmode == BRIGHT_AUTO && Lbrightflag == 1)
	{
		
		DATAPool.BRTvals = DATAPool.LSvals;
		//printf("DATAPool.BRTvals is %d\n",DATAPool.BRTvals);
	}
	if(DATAPool.BRTmode == BRIGHT_AUTO)
	{	
	
		float div = (31) / (float)DATAPool.MaxBright;
		float Abright = DATAPool.LSvals / div;
		int IntBright = (Abright - (uint8_t)Abright > 0.5) ? ((uint8_t)Abright + 1) : ((uint8_t)Abright);
		if(IntBright >= DATAPool.MaxBright)
		{
			IntBright = DATAPool.MaxBright;
			Set_LEDBright(IntBright);
		}
		else if(IntBright <= DATAPool.MinBright)
		{
			IntBright = DATAPool.MinBright;
			Set_LEDBright(IntBright);
		}
		else
		{
			Set_LEDBright(IntBright);
		}
			

	}
		
	//防雷状态	
	DATAPool.THDvals = DataFromTXcard[THANDER_POS];
	#if 1
	DATAPool.THDstatus = (DATAPool.THDvals != 0x00) ? 0x31 : 0x30;
	if(DATAPool.pctl == MALAYSIA)
		DATAPool.THDstatus = 0x31;
	#endif
	//DATAPool.THDstatus =  0x31;
	DATAPool.TX_Mver = DataFromTXcard[TX_MVER];
	DATAPool.TX_Sver = DataFromTXcard[TX_SVER];

//由于tx给的顺序是从最后一张开始上传的	
	for(i = 0 ; i < DATAPool.RXnum; i++)
	{
		DATAPool.RXcard[i].Temp   	= DataFromTXcard[(DATAPool.RXnum - i -1) * 8 + TEMP_POS];
		DATAPool.RXcard[i].Volt1   	= DataFromTXcard[(DATAPool.RXnum - i -1) * 8 + VOL1_POS];
		DATAPool.RXcard[i].Volt2  	= DataFromTXcard[(DATAPool.RXnum - i -1) * 8 + VOL2_POS];
		DATAPool.RXcard[i].Volt3 	= DataFromTXcard[(DATAPool.RXnum - i -1) * 8 + VOL3_POS];
		DATAPool.RXcard[i].Volt4 	= DataFromTXcard[(DATAPool.RXnum - i -1) * 8 + VOL4_POS];
		DATAPool.RXcard[i].Door  	= DataFromTXcard[(DATAPool.RXnum - i -1) * 8 + DOOR_POS];

		DATAPool.RXcard[i].Mver	= DataFromTXcard[(DATAPool.RXnum - i -1) * 8 + MVER_POS];
		DATAPool.RXcard[i].Sver	= DataFromTXcard[(DATAPool.RXnum - i -1) * 8 + SVER_POS];
	}
	//门开关状态
	for(i = 0 ; i < DATAPool.RXnum ; i ++)
	{
		if(DATAPool.RXcard[DATAPool.RXnum - i -1].Door == 0x00)
			break;
	}

	//实际的值0x03表示开门的，0x00表时关着的
	DATAPool.Dstatus = (DATAPool.RXcard[DATAPool.RXnum - i -1].Door == 0x00) ? 0x30 : 0x31;
	DATAPool.Dvals = DATAPool.RXcard[DATAPool.RXnum - i -1].Door;

	//温度状态
	uint8_t TPstatus,TPvals;
	uint16_t Temp = 0;
	for(i = 0 ; i < DATAPool.RXnum ; i ++)
		Temp += DATAPool.RXcard[i].Temp;
	//复位接收卡后，可能出现DATAPool.RXnum = 0的情况，分母为0会导致Floating point exception错误
	if(DATAPool.RXnum != 0)
		TPvals = Temp / DATAPool.RXnum;
	DATAPool.TMPstatus = (TPvals < 0 || TPvals > 80) ? 0x31 : 0x30;
	DATAPool.TMPvals = TPvals;	
}


int DP_RXTX_Parametermsg(uint8_t *DataFromTXcard,uint16_t len)
{
	if(DataFromTXcard[0] == 0x01)
	{
		debug_printf("DataFromTXcard len is %d\n",len);
		memcpy(DATAPool.prm_msg,DataFromTXcard,len);
		DATAPool.prm_len = len;
		int i = 0;
		for(;i < DATAPool.prm_len;i++)
		{
			debug_printf("%02X ",DATAPool.prm_msg[i]);
		}
		debug_printf("\n");
		return 0;
	}
	else
		return -1;

}

int DP_TempHumSmogModuleParsing(uint8_t *DataFromTHS,uint8_t len)
{
	uint8_t SmogStatus,SmogVals;
	uint8_t TempStatus,TempVals;
	uint8_t HumStatus,HumVals;
	SmogStatus 	= DataFromTHS[7];
	TempVals 	= DataFromTHS[11];
	HumVals  	= DataFromTHS[13];
	
	SmogVals    = 0;
	DATAPool.SMKvals = SmogVals;
	DATAPool.SMKstatus = SmogStatus;

	DATAPool.TMPvals = TempVals;
	DATAPool.HUMvals = HumVals;                                                                                             
}

void DP_GetTemp(uint16_t *temp)
{
	*temp = DATAPool.Tempvals;
}

void DP_SetTemp(uint16_t temp)
{
	DATAPool.Tempvals = temp;
}

void DP_GetHum(uint16_t *Hum)
{
	*Hum = DATAPool.HUMvals;
}
void DP_SetHum(uint16_t Hum)
{
	DATAPool.HUMvals = Hum;
}

void DP_SetYLight(uint8_t status)
{
	DATAPool.YLstatus = status;
}

void DP_GetYLight(uint8_t *status)
{
	*status = DATAPool.YLstatus;
}

int DP_GetPixelsMode(uint8_t *PixMode)
{
	*PixMode = DATAPool.PIXmode;
}
int DP_SetPixelsMode(uint8_t PixMode)
{
	DATAPool.PIXmode= PixMode;
}


int DP_GetPixelsColor(uint8_t *PixColor)
{
	*PixColor = DATAPool.PIXcolor;
}




int DP_PixelsDataParsing(uint8_t *PixelsData,uint32_t Len)
{
	uint8_t data;
	uint32_t pixels_badnum = 0;
	uint32_t byte_num = 0,byte_bit = 0;
	uint8_t FrameNum = 0;
	uint8_t pix_val[256] = {
		0x00,0x01,0x01,0x02,0x01,0x02,0x02,0x03,0x01,0x02,0x02,0x03,0x02,0x03,0x03,0x04,
		0x01,0x02,0x02,0x03,0x02,0x03,0x03,0x04,0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,
		0x01,0x02,0x02,0x03,0x02,0x03,0x03,0x04,0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,
		0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,
		0x01,0x02,0x02,0x03,0x02,0x03,0x03,0x04,0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,
		0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,
		0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,
		0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,0x04,0x05,0x05,0x06,0x05,0x06,0x06,0x07,
		0x01,0x02,0x02,0x03,0x02,0x03,0x03,0x04,0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,
		0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,
		0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,
		0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,0x04,0x05,0x05,0x06,0x05,0x06,0x06,0x07,
		0x02,0x03,0x03,0x04,0x03,0x04,0x04,0x05,0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,
		0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,0x04,0x05,0x05,0x06,0x05,0x06,0x06,0x07,
		0x03,0x04,0x04,0x05,0x04,0x05,0x05,0x06,0x04,0x05,0x05,0x06,0x05,0x06,0x06,0x07,
		0x04,0x05,0x05,0x06,0x05,0x06,0x06,0x07,0x05,0x06,0x06,0x07,0x06,0x07,0x07,0x08 					
		};

	if(DATAPool.PIXdata == NULL || PixelsData[0] == 0x00 || Len-2 > 2048)
		return -1;
	//此处排除掉配置文件屏体、箱体大小参数配置不对导致获取的像素数据量大于
	//所开辟的存储像素数据的缓存DATAPool.PIXdata容量而致程序挂掉
	uint8_t tmp = PixelsData[1];
	FrameNum = 0x0F & tmp;
	uint8_t color = (0xF0 & tmp) >> 4;
	DATAPool.PIXcolor = color;
	if((FrameNum*2048 + Len-2)> (DATAPool.SCRwidth * DATAPool.SCRheight)/8)
		return -1;
	
	//PixelsData[Len-1] = 0xff;PixelsData[Len-4] = 0xf0;
	memcpy(DATAPool.PIXdata+(FrameNum*2048),PixelsData+2,Len-2);
	//uint8_t pixmode = 0;
	//DP_GetPixelsMode(&pixmode);
	debug_printf("pixelsbits is %d\n",FrameNum*2048+Len-2);
	uint32_t datalen = 0;
	if(FrameNum*2048+Len-2 == DATAPool.SCRwidth * DATAPool.SCRheight/8)
	{   //printf("tmp %d\n",pix_val[255]);
		datalen = FrameNum*2048+Len-2;
		for(byte_num = 0 ; byte_num < datalen; byte_num ++)
		{
			data = PixelsData[byte_num+2];
			pixels_badnum += pix_val[data];
		}
		
		if(pixels_badnum > 0)
		{
			if(DATAPool.PIXmode == 0x01 && pixels_badnum > 10)
				DATAPool.PIXstatus = 0x30;
			else
				DATAPool.PIXstatus = 0x31;
			DATAPool.PIXvals = pixels_badnum;
		}	
		else
		{
			DATAPool.PIXstatus = 0x31;
			DATAPool.PIXvals = 0;
		}
		debug_printf("DATAPool.PIXstatus is %X DATAPool.PIXvals is %d\n",DATAPool.PIXstatus,DATAPool.PIXvals);
	}
	return 0;	
}



int DP_GetPixelsData(uint8_t *PixelsData,uint32_t *Len)
{
	if(DATAPool.PIXdata == NULL)
		return -1;
	
	memcpy(PixelsData,DATAPool.PIXdata,DATAPool.PIXdataLen);
	*Len = DATAPool.PIXdataLen;

	return 0;
}

int DP_GetPixelsStatus(uint8_t *Status,uint32_t *BadPixels)
{
	if(Status == NULL || BadPixels == NULL)
		return -1;

	*BadPixels = DATAPool.PIXvals;
	*Status = DATAPool.PIXstatus;
	return 0;
}


int DP_GetRXCardData(uint8_t PID,uint8_t CardNum,uint8_t *vals)
{
	switch(PID)
	{
		case RX_PID_TEMP:
			*vals = DATAPool.RXcard[CardNum].Temp;
			break;
		case RX_PID_VOLT1:
			*vals = DATAPool.RXcard[CardNum].Volt1;
			break;
		case RX_PID_VOLT2:
			*vals = DATAPool.RXcard[CardNum].Volt2;
			break;
		case RX_PID_VOLT3:
			*vals = DATAPool.RXcard[CardNum].Volt3;
			break;
		case RX_PID_VOLT4:
			*vals = DATAPool.RXcard[CardNum].Volt4;
			break;
		case RX_PID_DOOR:
			*vals = DATAPool.RXcard[CardNum].Door;
			break;
		case RX_PID_BADPIXH:
			*vals = DATAPool.RXcard[CardNum].BadPH;
			break;
		case RX_PID_BASPIXL:
			*vals = DATAPool.RXcard[CardNum].BadPL;
			break;
		case RX_PID_CHNANEL:
			*vals = DATAPool.RXcard[CardNum].Channel;
			break;
		case RX_PID_LSENSA:
			*vals = DATAPool.LSensA;
			break;
		case RX_PID_LSENSB:
			*vals = DATAPool.LSensB;
			break;
		default:
			break;
			
	}
}


int DP_GetSysDataAndStatus(uint8_t PID,uint8_t *status,uint8_t *vals)
{
	uint8_t i = 0;
	switch(PID)
	{
		case PID_LIGHT_SENSITIVE:
			*status = DATAPool.LSstatus;
			*vals = DATAPool.LSvals;
			break;
		case PID_THANDER:
			*status=DATAPool.THDstatus;
			*vals=DATAPool.THDvals;
			break;
		case PID_SMOG:
			*status=DATAPool.SMKstatus;
			*vals=DATAPool.SMKvals;
			break;
		case PID_HUMIDITY:
			*status=DATAPool.HUMstatus;
			*vals=DATAPool.HUMvals;
			break;
		case PID_POWER:
			*status=DATAPool.PWstatus;
			*vals=DATAPool.PWvals;
			break;
		case PID_CTROLLER:
			*status=DATAPool.CTRLstatus;
			*vals=DATAPool.CTRLvals;
			break;
		case PID_TOTAL_TEMP:
			*status=DATAPool.TMPstatus;
			*vals=DATAPool.TMPvals;
			break;
		case PID_MODEL:
			*status=DATAPool.MODstatus;
			*vals=DATAPool.MODvals;
			break;
		case PID_MODEL_POWER:
			*status=DATAPool.MODPstatus;
			*vals=DATAPool.MODPvals;
			break;
		case PID_PIXELS:
			*status=DATAPool.PIXstatus;
			*vals = DATAPool.PIXvals;
			break;
		case PID_DOOR:
			*status=DATAPool.Dstatus;
			*vals = DATAPool.Dvals;
			break;
		default:
			break;
	}

	return 0;
}


int DP_GetCurPlayContent(uint8_t *Content,uint16_t *Len,uint8_t *Font,uint8_t *Size,uint8_t *Inform,uint32_t *inSpeed,uint32_t *stayTime,uint8_t *Order)
{
	if(Content == NULL || Len == NULL || stayTime == NULL || Inform == NULL || inSpeed == NULL)
		return -1;

	*Len = strlen(DATAPool.CPcontent);
	memcpy(Content,DATAPool.CPcontent,*Len);
	*Font		= DATAPool.CPfont;
	*Size		= DATAPool.CPsize;
	*stayTime   = DATAPool.CPdelayTime;
	*Inform 	= DATAPool.CPinform;
	*inSpeed	= DATAPool.CPmoveSpeed;
	*Len 		= DATAPool.CPcontentLen;
	*Order		= DATAPool.CPitemOder;

	debug_printf("DATAPool.CPcontent = %s,Content = %s\n",DATAPool.CPcontent,Content);
	//printf("DATAPool.CPcontent = %s\n",DATAPool.CPcontent);

	return 0;
}

void DP_SetCurPlayContent(char *Content,uint16_t len,uint8_t Font,uint8_t Size,uint16_t inform,uint32_t inSpeed,uint32_t stayTime,uint8_t Order)
{
	int16_t Len;
	if(Content == NULL || len <= 0)
		return;
	Len = len;
	
	memset(DATAPool.CPcontent,0,sizeof(DATAPool.CPcontent));
	//change by mo 2020/12/7 并没有发现多余的字节
#if 0	
	//减2的原因是后面有两个多余的字节，奇怪
	//文本内容要截掉2个字节，图片或者gif文件的文件名称不要截掉
	if(strstr(Content,".bmp") == NULL && strstr(Content,".gif") == NULL)
		Len -= 2;
#endif
	//有时候这个数是负数的，原因还没查明
	if(Len < 0)
		return;
	memcpy(DATAPool.CPcontent,Content,Len);
	DATAPool.CPfont			= Font;
	DATAPool.CPsize			= Size;
	DATAPool.CPdelayTime	= stayTime;
	DATAPool.CPinform		= inform;
	DATAPool.CPmoveSpeed	= inSpeed;
	DATAPool.CPcontentLen	= Len;
	DATAPool.CPitemOder		= Order;
}

void DP_SetCurPlayItemContent(char *Content,uint16_t len)
{
	int16_t Len;
	if(Content == NULL || len <= 0)
		return;
	Len = len;
	
	memset(DATAPool.itemcontent,0,sizeof(DATAPool.itemcontent));
	memcpy(DATAPool.itemcontent,Content,Len);
}

int DP_GetCurPlayItemContent(char *Content,uint16_t *len)
{
	if(Content == NULL || len == NULL)
		return -1;

	*len = strlen(DATAPool.itemcontent);
	memcpy(Content,DATAPool.itemcontent,*len);
	return 0;
}



void DP_SetCurPlayList(uint8_t *Plist,uint8_t Len)
{
	if(Plist == NULL)
		return;

	memset(DATAPool.CPlist,0,sizeof(DATAPool.CPlist));
	memcpy(DATAPool.CPlist,Plist,Len);
}


int DP_GetCurPlayList(uint8_t *Plist,uint8_t *Len)
{
	if(Plist == NULL || Len == NULL)
		return -1;

	*Len = strlen(DATAPool.CPlist);
	memcpy(Plist,DATAPool.CPlist,*Len);

	return 0;
}



void DP_SetBoxSize(uint16_t width,uint16_t height,uint32_t num)
{
	DATAPool.BOXwidth		= width;
	DATAPool.BOXheight		= height;
	DATAPool.BOXnum 		= num;
}

int DP_GetBoxSize(uint16_t *width,uint16_t *height,uint32_t *num)
{
	if(width == NULL || height == NULL || num == NULL)
		return -1;

	*width	   = DATAPool.BOXwidth;
	*height    = DATAPool.BOXheight;
	*num	   = DATAPool.BOXnum;

	return 0;
}

int DP_SetOffset(uint16_t Xoffset,uint16_t Yoffset)
{
	DATAPool.Xoffset = Xoffset;
	DATAPool.Yoffset = Yoffset;
}

void DP_GetOffset(uint16_t *Xoffset,uint16_t *Yoffset)
{
	*Xoffset = DATAPool.Xoffset;
	*Yoffset = DATAPool.Yoffset;
}

void DP_SetScrType(uint16_t scrType)
{
	DATAPool.scrType = scrType;
}


void DP_GetScrType(uint16_t *scrType)
{
	*scrType = DATAPool.scrType;
}

void DP_SetCardType(uint16_t cardType)
{
	DATAPool.cardType = cardType;
}

void DP_GetCardType(uint16_t *cardType)
{
	*cardType = DATAPool.cardType;
}

void DP_GetMvspeed(uint16_t *speed)
{
	*speed = DATAPool.mvspeed;
}

void DP_SetMvspeed(uint16_t speed)
{
	DATAPool.mvspeed = speed;
}


void DP_SetModSize(uint16_t Modwidth,uint16_t Modheight)
{
	DATAPool.Modwidth  = Modwidth;
	DATAPool.Modheight = Modheight;
}

void DP_GetModSize(uint16_t *Modwidth,uint16_t *Modheight)
{
	*Modwidth  = DATAPool.Modwidth;
	*Modheight = DATAPool.Modheight;
}


void DP_SetLBandArg(uint8_t state,uint8_t sement)
{
	DATAPool.LBstate = state;
	DATAPool.LBsement = sement;
}

void DP_GetLBandArg(uint8_t *state,uint8_t *sement)
{
	*state = DATAPool.LBstate;
	*sement = DATAPool.LBsement;
}




void DP_SetScreenSize(uint32_t width,uint32_t height)
{
	DATAPool.SCRwidth	= width;
	DATAPool.SCRheight	= height;

	//如果开启查询像素点状态的功能则为像素数据开辟内存
#ifdef GET_PIXELS_FUN
	DATAPool.PIXdataLen = DATAPool.SCRwidth * DATAPool.SCRheight / 8;
	DATAPool.PIXdata = (uint8_t *)malloc(DATAPool.PIXdataLen + 64);
	memset(DATAPool.PIXdata,0x00,DATAPool.PIXdataLen + 64);
#else
	DATAPool.PIXdata = NULL;
#endif
}

int DP_GetScreenSize(uint32_t *width,uint32_t *height)
{
	if(width == NULL || height == NULL)
		return -1;

	*width = DATAPool.SCRwidth;
	*height = DATAPool.SCRheight;

	return 0;
}



void DP_SetScrResolution(uint16_t res_x,uint16_t res_y,uint16_t res_bits)
{
	DATAPool.RESwidth	= res_x;
	DATAPool.RESheight	= res_y;
	DATAPool.RESbits	= res_bits;
}

int DP_GetScrResolution(uint16_t *res_x,uint16_t *res_y,uint16_t *res_bits)
{
	if(res_x == NULL || res_y == NULL || res_bits == NULL)
		return -1;

	*res_x		=DATAPool.RESwidth;
	*res_y		=DATAPool.RESheight;
	*res_bits	=DATAPool.RESbits;

	return 0;
}



void DP_SetSerialArg(uint8_t comx,uint32_t bdrate,uint8_t databits,uint8_t stopbits,uint8_t fctrl,uint8_t parity)
{
	DATAPool.COMx[comx].COMbaudrate = bdrate;
	DATAPool.COMx[comx].COMdatabits = databits;
	DATAPool.COMx[comx].COMstopbits = stopbits;
	DATAPool.COMx[comx].COMflowctl	= fctrl;
	DATAPool.COMx[comx].COMparity	= parity;
}

int DP_GetSerialArg(uint8_t comx,uint32_t *bdrate,uint8_t *databits,uint8_t *stopbits,uint8_t *fctrl,uint8_t *parity)
{
	if(bdrate == NULL || databits == NULL || stopbits == NULL || fctrl == NULL ||  parity == NULL)
		return -1;
	
	*bdrate 	= DATAPool.COMx[comx].COMbaudrate ;
	*databits	= DATAPool.COMx[comx].COMdatabits ;
	*stopbits	= DATAPool.COMx[comx].COMstopbits;
	*fctrl		= DATAPool.COMx[comx].COMflowctl;
	*parity 	= DATAPool.COMx[comx].COMparity;

	return 0;
}





void DP_SetNetArg(uint8_t *IP,uint32_t port,uint8_t *netmask,uint8_t *gateway)
{
	if(IP == NULL || netmask == NULL || gateway == NULL)
		return;

	memset(DATAPool.NETip,0,sizeof(DATAPool.NETip));
	memset(DATAPool.NETmask,0,sizeof(DATAPool.NETmask));
	memset(DATAPool.NETgw,0,sizeof(DATAPool.NETgw));
	memcpy(DATAPool.NETip,IP,strlen(IP));
	memcpy(DATAPool.NETmask,netmask,strlen(netmask));
	memcpy(DATAPool.NETgw,gateway,strlen(gateway));
	DATAPool.NETprot = port;
}

int DP_GetNetArg(uint8_t *IP,uint32_t *port,uint8_t *netmask,uint8_t *gateway)
{
	if(IP == NULL || netmask == NULL || gateway == NULL || port == NULL)
		return -1;

	memcpy(IP,DATAPool.NETip,strlen(DATAPool.NETip));
	memcpy(netmask,DATAPool.NETmask,strlen(DATAPool.NETmask));
	memcpy(gateway,DATAPool.NETgw,strlen(DATAPool.NETgw));
	*port = DATAPool.NETprot;

	return 0;
}




void DP_SetScreenStatus(uint8_t Status)
{
	DATAPool.ScrSwitch= Status;
	debug_printf("Status = %d\n",Status);
}

int DP_GetScreenStatus(uint8_t *Status)
{
	*Status = DATAPool.ScrSwitch;
	return DATAPool.ScrSwitch;
}




void DP_SetLSData(uint8_t LSdataA,uint8_t LSdataB)
{
	DATAPool.LSensA = LSdataA;
	DATAPool.LSensB = LSdataB;
}

void DP_GetLSData(uint8_t *LSdataA,uint8_t *LSdataB)
{
	*LSdataA = DATAPool.LSensA;
	*LSdataB = DATAPool.LSensB;
}


void DP_SetTestMode(uint8_t mode)
{
	DATAPool.TestMode = mode;
}

void DP_GetTestMode(uint8_t *mode)
{
	*mode = DATAPool.TestMode;
}

void DP_SetTestFlag(uint8_t TestFlag)
{
	DATAPool.TestFlag = TestFlag;
}

void DP_GetTestFlag(uint8_t *TestFlag)
{
	*TestFlag = DATAPool.TestFlag;
}

void DP_SetIntervTime(uint32_t IntervTime)
{
	DATAPool.IntervTime = IntervTime;
	//debug_printf("DATAPool.IntervTime = %d\n",DATAPool.IntervTime);
}

void DP_GetIntervTime(uint32_t *IntervTime)
{
	*IntervTime = DATAPool.IntervTime;
	//debug_printf("DATAPool.IntervTime = %d\n",DATAPool.IntervTime);
}

void DP_SetVertConnect(uint8_t VertConnect)
{
	DATAPool.VertConnect = VertConnect;
}

void DP_GetVertConnect(uint8_t *VertConnect)
{
	*VertConnect = DATAPool.VertConnect;
}



void DP_SetCheckSelfTrouble(uint16_t ALLScreen,uint8_t *Modgroup,uint8_t Modnum)
{
	uint8_t i = 0;
	
	DATAPool.ALLScreen = ALLScreen;

	for(i = 0 ; i < Modnum ; i++)
		DATAPool.Modgroup[i] = Modgroup[i];
}

void DP_GetCheckSelfTrouble(uint16_t *ALLScreen,uint8_t *Modgroup,uint8_t Modnum)
{
	uint8_t i = 0;
	
	*ALLScreen = DATAPool.ALLScreen;
	for(i = 0 ; i < Modnum ; i++)
		Modgroup[i] = DATAPool.Modgroup[i];
}

void DP_SetCheckSelf(uint8_t CheckSelf,uint8_t dspunit)
{
	DATAPool.CheckSelf = CheckSelf;
	DATAPool.Dspunit   = dspunit;
	//debug_printf("DATAPool.CheckSelf = %d\n",DATAPool.CheckSelf);
}

void DP_GetCheckSelf(uint8_t *CheckSelf,uint8_t *dspunit)
{
	*CheckSelf = DATAPool.CheckSelf;
	*dspunit   = DATAPool.Dspunit;
	//debug_printf("*CheckSelf = %d\n",*CheckSelf);
}

void DP_SetAutoCheckTime(uint8_t SThour,uint8_t STmin,uint8_t STsec)
{
	DATAPool.SThour  = SThour;
	DATAPool.STmin   = STmin;
	DATAPool.STsec   = STsec;
	//debug_printf("DATAPool.SThour = %d,DATAPool.STmin = %d,DATAPool.STsec = %d\n",DATAPool.SThour,DATAPool.STmin,DATAPool.STsec);
}
void DP_GetAutoCheckTime(uint8_t *SThour,uint8_t *STmin,uint8_t *STsec)
{
	*SThour 	= DATAPool.SThour;
	*STmin 		= DATAPool.STmin;
	*STsec 		= DATAPool.STsec;
}


//设置自动自检间隔时间的单位与周期
//STunit:	单位--1每日 2每时 3每分 默认值为1
//STcycle:周期--1日周期 2时周期 3分周期 默认值为1
void DP_SetAutoCheckUnit(uint8_t STunit,uint8_t STcycle)
{
	DATAPool.STunit  = STunit;
	DATAPool.STcycle = STcycle;
}

void DP_GetAutoCheckUnit(uint8_t *STunit,uint8_t *STcycle)
{
	*STunit 	= DATAPool.STunit;
	*STcycle 	= DATAPool.STcycle;
}


DSPContent_t CURDSPContent;

void InitDSPContent(void)
{
	memset(&CURDSPContent,0,sizeof(CURDSPContent));
}
int SetDSPContent(DSPContent_t *DSPContent)
{
	CURDSPContent.speed = DSPContent->speed;
	CURDSPContent.outform = DSPContent->outform;
	CURDSPContent.inform = DSPContent->inform;
	CURDSPContent.staytime = DSPContent->staytime;
	switch(DSPContent->type)
	{
		case DSPTYPE_STR:
			CURDSPContent.fontsize = DSPContent->fontsize;
			CURDSPContent.fonttype = DSPContent->fonttype;
			CURDSPContent.strLen = DSPContent->strLen;
			memset(CURDSPContent.charstr,0,sizeof(CURDSPContent.charstr));
			memcpy(CURDSPContent.charstr,DSPContent->charstr,DSPContent->strLen);
			break;
		case DSPTYPE_IMG:
			CURDSPContent.maptype = DSPContent->maptype;
			memset(CURDSPContent.mapName,0,sizeof(CURDSPContent.mapName));
			memcpy(CURDSPContent.mapName,DSPContent->mapName,strlen(DSPContent->mapName));
			break;
		case DSPTYPE_LBD:
			memset(CURDSPContent.lbName,0,sizeof(CURDSPContent.lbName));
			memcpy(CURDSPContent.lbName,DSPContent->lbName,strlen(DSPContent->lbName));
			break;
	}
}

void GetDSPContent(DSPContent_t *DSPContent)
{
	DSPContent->speed = CURDSPContent.speed;
	DSPContent->outform = CURDSPContent.outform;
	DSPContent->inform = CURDSPContent.inform;
	DSPContent->staytime = CURDSPContent.staytime;
	switch(DSPContent->type)
	{
		case DSPTYPE_STR:
			DSPContent->fontsize = CURDSPContent.fontsize;
			DSPContent->fonttype = CURDSPContent.fonttype;
			strcat(DSPContent->charstr,DSPContent->charstr);
			DSPContent->strLen += CURDSPContent.strLen;
			//memset(DSPContent->charstr,0,sizeof(DSPContent->charstr));
			//memcpy(DSPContent->charstr,CURDSPContent.charstr,CURDSPContent.strLen);
			break;
		case DSPTYPE_IMG:
			DSPContent->maptype = CURDSPContent.maptype;
			memset(DSPContent->mapName,0,sizeof(DSPContent->mapName));
			memcpy(DSPContent->mapName,CURDSPContent.mapName,strlen(CURDSPContent.mapName));
			break;
		case DSPTYPE_LBD:
			memset(DSPContent->lbName,0,sizeof(DSPContent->lbName));
			memcpy(DSPContent->lbName,CURDSPContent.lbName,strlen(CURDSPContent.lbName));
			break;
	}
	
}




