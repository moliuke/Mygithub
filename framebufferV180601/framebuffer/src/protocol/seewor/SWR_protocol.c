
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>

#include "debug.h"
#include "config.h"
#include "conf.h"
#include "mtime.h"
#include "SWR_protocol.h"
#include "content.h"
#include "SWR_charparse.h"

#include "../PTC_common.h"
#include "../../Hardware/HW3G_RXTX.h"
#include "../../Hardware/HW2G_400.h"
#include "../PTC_FileCopy.h"
#include "../PTC_FileRxTx.h"
#include "../../Hardware/Data_pool.h"
#include "../../update.h"

Protocl_t PROTOCOLStruct;


#define BOX_VOLT_MIN 	12
#define BOX_VOLT_MAX	80

time_t LastTimer;





static void recvmsg_printf(unsigned char *input,unsigned int inputlen)
{
	int i = 0;
	debug_printf("===============recv data============\n");
	for(i = 0 ; i < inputlen ; i ++)
	{
		debug_printf("0x%x,",input[i]);
	}
	debug_printf("\n");
}


static void prtclmsg_printf(Protocl_t *protocol)
{
	static int f = 0;
	int i = 0; 
	debug_printf("frame num : %d\n",f++);
	debug_printf("\n\n=================================\n");
	debug_printf(
		"ip:				%s\n"
		"port:				%d\n"
		"fd:				%d\n"
		"startbyte:			0x%x\n"
		"cmdID:				0x%x\n"
		"devID:				0x%x\n",
		protocol->usermsg->ip,
		protocol->usermsg->port,
		protocol->usermsg->fd,
		protocol->startByte,
		protocol->protcmsg.head.cmdID,
		protocol->protcmsg.head.devID);	
	debug_printf("data:				");
	for(i = 0 ; i < protocol->protcmsg.length ; i ++)
	{
		debug_printf("%02x ",protocol->protcmsg.data[i]);
	}

	debug_printf("\n"
		"parity:				0x%x\n"
		"endbyte:			0x%x\n"
		"length:				0x%x\n",
		protocol->protcmsg.parity,
		protocol->endByte,
		protocol->protcmsg.length
		);
	debug_printf("\n=====================================\n\n");
	
}


static int get_deviceStatus(Protocl_t *protocol,unsigned int *len)
{
	uint16_t box_order = 0;
	uint16_t box_number = 0;
	uint8_t BoxWidth,BoxHeight,BoxNum;

	uint16_t SYSstatus = 0;
	uint16_t PStatus = 0;
	uint16_t FANstatus = 0;
	uint16_t door_status = 0,door_bad = 0;
	uint16_t channel_status = 0,channel_bad = 0;
	uint16_t pixels_status = 0,pixels_bad = 0;
	uint16_t thander_status = 0,thander_bad = 0;
	uint16_t light_status = 0,light1_bad = 0,light2_bad = 0;

	DEBUG_PRINTF;

	protocol->protcmsg.data[0] = 0x01;			//执行指令正常
	protocol->protcmsg.data[1] = 0x31;			//通信正常

	//电源电压状态，一般都会正常，否则怎么通信?
	PStatus = 0x31;
	protocol->protcmsg.data[2] = PStatus;	

	//风扇状态，目前没有检测风扇的器件
	FANstatus = 0x31;
	protocol->protcmsg.data[3] = FANstatus;

	//门开关状态检测，门是否关还不确定怎么检测，暂定这么检测吧
	uint8_t DrStatus,DrVals;
	DP_GetSysDataAndStatus(PID_DOOR,&DrStatus,&DrVals);
	protocol->protcmsg.data[4] = DrStatus;//DrStatus;


	//驱动通道状态，channel_bad指明哪个通道坏了
	uint8_t CHNStatus,CHNVals;
	DP_GetSysDataAndStatus(PID_CHANNEL,&CHNStatus,&CHNVals);
	protocol->protcmsg.data[6] = 0x31;//CHNStatus;

	
	//检测像素点状态，pixels_bad指明有多少个像素点坏掉，等于0表明像素点正常
	uint8_t PIXStatus;
	uint32_t BadPixels;
	DP_GetPixelsStatus(&PIXStatus,&BadPixels);
	protocol->protcmsg.data[7] = PIXStatus;

	//检测光敏器件的状态，有两个光敏电阻，如果两个光敏电阻有其中一个的值不在范围内
	//说明坏掉了
	uint8_t LTStatus,LTVals;
	DP_GetSysDataAndStatus(PID_LIGHT_SENSITIVE,&LTStatus,&LTVals);
	protocol->protcmsg.data[8] = LTStatus;


	//检测防雷器的状态，不在范围内说明坏掉了
	uint8_t THDStatus,THDVals;
	DP_GetSysDataAndStatus(PID_THANDER,&THDStatus,&THDVals);
	protocol->protcmsg.data[9] = THDStatus;//THDStatus;


	//检测系统状态，如果电源电压、风扇、驱动通道、像素状态、门开关、光敏、防雷等有其中一项不正常
	//那么说明系统状态不正常。
	#if 0
	if(PStatus   == 0x30 || FANstatus    == 0x30 || DrStatus  == 0x30 
	|| CHNStatus == 0x30 || PIXStatus == 0x30 || LTStatus == 0x30
	|| THDStatus == 0x30)
		SYSstatus = 0x30;
	else
		SYSstatus = 0x31;
	#endif
	SYSstatus = 0x31;
	protocol->protcmsg.data[5] = SYSstatus;

	
	protocol->protcmsg.data[10]= 0x31;
	protocol->protcmsg.data[11]= 0x31;
	protocol->protcmsg.data[12]= 0x31;
	
	*len = 13;
	protocol->protcmsg.length = *len;

	char logmsg[256];
	memset(logmsg,0,sizeof(logmsg));
	sprintf(logmsg,"PStatus:%x FANstatus:%x DrStatus:%x CHNStatus:%x PIXStatus:%x LTStatus:%x THDStatus:%x SYSstatus:%x",PStatus,FANstatus,DrStatus,
		CHNStatus,PIXStatus,LTStatus,THDStatus,SYSstatus);
	_log_file_write_("protolog","get_deviceStatus",strlen("get_deviceStatus"),logmsg,strlen(logmsg));

	char ip_port[24];
	memset(ip_port,0,sizeof(ip_port));
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	_log_file_write_("userlog",ip_port,strlen(ip_port),"cmd:3031,read device total status",strlen("cmd:3031,read device total status"));

	return 0;
	
}



static int get_deviceDetail(Protocl_t *protocol,unsigned int *len)
{
	unsigned int nlength = 0;
	uint16_t box_number = 0;
	uint16_t light1 	= 0,light2 = 0;
	uint16_t temperature = 0,temp_sum = 0;
	uint16_t voltage 	= 0;
	uint16_t thander		= 0;
	
	uint8_t box_volt1 = 0,box_volt1_status = 0;
	uint8_t box_volt2 = 0,box_volt2_status = 0;
	uint8_t box_volt3 = 0,box_volt3_status = 0;
	uint8_t box_volt4 = 0,box_volt4_status = 0;
	uint8_t box_volt5 = 0,box_volt5_status = 0;
	uint8_t box_door_vals = 0,box_door_status = 0;
	uint8_t box_channel_status = 0,box_sys_status = 0;
	uint8_t bad_pixels = 0,bad_channel = 0;
	uint8_t box_order = 0;
	uint8_t box_temperature = 0,box_temperature_status = 0;
	
	
	memset(protocol->protcmsg.data,0,nlength);
	//指令执行情况
	protocol->protcmsg.data[0] = 0x01;
	
	// 箱体个数
	uint16_t BoxWidth,BoxHeight;
	uint32_t BoxNum;
	//DP_BoxSize(OPS_MODE_GET,&BoxWidth,&BoxHeight,&BoxNum);
	DP_GetBoxSize(&BoxWidth,&BoxHeight,&BoxNum);
	protocol->protcmsg.data[1]=(BYTE)(BoxNum / 10 + 0x30);
	protocol->protcmsg.data[2]=(BYTE)(BoxNum % 10 + 0x30);
	
	// 亮度A
	uint8_t LSstatus,LSvalsA,LSvalsB;
	//DP_GetSysDataAndStatus(PID_LSENSA,&LSstatus,&LSvalsA);
	DP_GetRXCardData(RX_PID_LSENSA,0,&LSvalsA);
	protocol->protcmsg.data[3]	=(BYTE)(LSvalsA/100+0x30);
	protocol->protcmsg.data[4]	=(BYTE)(LSvalsA%100)/10+0x30;
	protocol->protcmsg.data[5]	=(BYTE)(LSvalsA%10+0x30);
	
	// 亮度B
	//DP_GetSysDataAndStatus(PID_LSENSB,&LSstatus,&LSvalsB);
	DP_GetRXCardData(RX_PID_LSENSB,0,&LSvalsB);
	protocol->protcmsg.data[6]	=(BYTE)(LSvalsB/100+0x30);
	protocol->protcmsg.data[7]	=(BYTE)(LSvalsB%100)/10+0x30;
	protocol->protcmsg.data[8]	=(BYTE)(LSvalsB%10+0x30);
	
	// 温度
	uint8_t TPStatus,TPvals;
	DP_GetSysDataAndStatus(PID_TOTAL_TEMP,&TPStatus,&TPvals);
	protocol->protcmsg.data[9]	=(BYTE)(TPvals/100+0x30);
	protocol->protcmsg.data[10]	=(BYTE)(TPvals%100)/10+0x30;
	protocol->protcmsg.data[11]	=(BYTE)(TPvals%10+0x30);
	
	//电源电压220v
	voltage = 220;
	protocol->protcmsg.data[12]	=(BYTE)(voltage/100+0x30);
	protocol->protcmsg.data[13]	=(BYTE)(voltage%100)/10+0x30;
	protocol->protcmsg.data[14]	=(BYTE)(voltage%10+0x30);
	
	// 防雷器
	uint8_t THDStatus,THDvals;
	DP_GetSysDataAndStatus(PID_THANDER,&THDStatus,&THDvals);
	protocol->protcmsg.data[15]	=(BYTE)(THDvals/100+0x30);
	protocol->protcmsg.data[16]	=(BYTE)(THDvals%100)/10+0x30;
	protocol->protcmsg.data[17]	=(BYTE)(THDvals%10+0x30);

	for(box_order=0; box_order< BoxNum; box_order++)
	{
		//箱体温度
		DEBUG_PRINTF;
		DP_GetRXCardData(RX_PID_TEMP,box_order,&box_temperature);
		if(box_temperature > 15 && box_temperature < 45)
			box_temperature_status = 0x31;
		else
			box_temperature_status = 0x30;
		protocol->protcmsg.data[18+30*box_order+0] = (BYTE)(box_temperature/100+0x30);
		protocol->protcmsg.data[18+30*box_order+1] = (BYTE)(box_temperature%100)/10+0x30;
		protocol->protcmsg.data[18+30*box_order+2] = (BYTE)(box_temperature%10+0x30);
		
		//箱体电压1的值
		DP_GetRXCardData(RX_PID_VOLT1,box_order,&box_volt1);
		if(box_volt1 > BOX_VOLT_MIN && box_volt1 < BOX_VOLT_MAX)
			box_volt1_status = 0x31;
		else
			box_volt1_status = 0x30;
		protocol->protcmsg.data[21+30*box_order+0] = (BYTE)(box_volt1/100+0x30);
		protocol->protcmsg.data[21+30*box_order+1] = (BYTE)(box_volt1%100)/10+0x30;
		protocol->protcmsg.data[21+30*box_order+2] = (BYTE)(box_volt1%10+0x30);

		//箱体电压2的值		
		DP_GetRXCardData(RX_PID_VOLT2,box_order,&box_volt2);
		if(box_volt2 > BOX_VOLT_MIN && box_volt2 < BOX_VOLT_MAX)
			box_volt2_status = 0x31;
		else
			box_volt2_status = 0x30;
		protocol->protcmsg.data[24+30*box_order+0] = (BYTE)(box_volt2/100+0x30);
		protocol->protcmsg.data[24+30*box_order+1] = (BYTE)(box_volt2%100)/10+0x30;
		protocol->protcmsg.data[24+30*box_order+2] = (BYTE)(box_volt2%10+0x30);

		//箱体电压3的值
		DP_GetRXCardData(RX_PID_VOLT3,box_order,&box_volt3);
		if(box_volt3 > BOX_VOLT_MIN && box_volt3 < BOX_VOLT_MAX)
			box_volt3_status = 0x31;
		else
			box_volt3_status = 0x30;
		protocol->protcmsg.data[27+30*box_order+0] = (BYTE)(box_volt3/100+0x30);
		protocol->protcmsg.data[27+30*box_order+1] = (BYTE)(box_volt3%100)/10+0x30;
		protocol->protcmsg.data[27+30*box_order+2] = (BYTE)(box_volt3%10+0x30);

		//箱体电压4的值
		DP_GetRXCardData(RX_PID_VOLT4,box_order,&box_volt4);
		if(box_volt4 > BOX_VOLT_MIN && box_volt4 < BOX_VOLT_MAX)
			box_volt4_status = 0x31;
		else
			box_volt4_status = 0x30;
		protocol->protcmsg.data[30+30*box_order+0] = (BYTE)(box_volt4/100+0x30);
		protocol->protcmsg.data[30+30*box_order+1] = (BYTE)(box_volt4%100)/10+0x30;
		protocol->protcmsg.data[30+30*box_order+2] = (BYTE)(box_volt4%10+0x30);

		//箱体电压5的值
		box_volt5 = box_volt4;
		if(box_volt5 > BOX_VOLT_MIN && box_volt5 < BOX_VOLT_MAX)
			box_volt5_status = 0x31;
		else
			box_volt5_status = 0x30;
		protocol->protcmsg.data[33+30*box_order+0] = (BYTE)(box_volt4/100+0x30);
		protocol->protcmsg.data[33+30*box_order+1] = (BYTE)(box_volt4%100)/10+0x30;
		protocol->protcmsg.data[33+30*box_order+2] = (BYTE)(box_volt4%10+0x30);

		//箱体门开关状态
		//门开关的值0x03表示门是开着的，即异常(0x30)
		DP_GetRXCardData(RX_PID_DOOR,box_order,&box_door_vals);
		if(box_door_vals == 0x00)
			box_door_status = 0;
		else
			box_door_status = 1;
		protocol->protcmsg.data[36+30*box_order+0] = (BYTE)(box_door_status/100+0x30);
		protocol->protcmsg.data[36+30*box_order+1] = (BYTE)(box_door_status%100)/10+0x30;
		protocol->protcmsg.data[36+30*box_order+2] = (BYTE)(box_door_status%10+0x30);
		//debug_printf("###protocol->protcmsg.data[0] = 0x%x,protocol->protcmsg.data[1] = 0x%x,protocol->protcmsg.data[2] = 0x%x\n",protocol->protcmsg.data[36+30*box_order+0],protocol->protcmsg.data[36+30*box_order+1],protocol->protcmsg.data[36+30*box_order+2]);
		//保留字节
		protocol->protcmsg.data[39+30*box_order+0] = (BYTE)(0x30);
		protocol->protcmsg.data[39+30*box_order+1] = (BYTE)(0x30);
		protocol->protcmsg.data[39+30*box_order+2] = (BYTE)(0x30);


		//通道状态
		uint8_t BadPH,BadPL;
		uint16_t BadPixels;
		DP_GetRXCardData(RX_PID_BADPIXH,box_order,&BadPH);
		DP_GetRXCardData(RX_PID_BASPIXL,box_order,&BadPL);
		DP_GetRXCardData(RX_PID_CHNANEL,box_order,&bad_channel);
		BadPixels = BadPH << 8 | BadPL;
		if(BadPixels == 0 && bad_channel == 0)
			box_channel_status = 1;
		else
			box_channel_status = 0;
		
		protocol->protcmsg.data[42 + 30*box_order+0] = (BYTE)(box_channel_status/100+0x30);
		protocol->protcmsg.data[42 + 30*box_order+1] = (BYTE)(box_channel_status%100)/10+0x30;;
		protocol->protcmsg.data[42 + 30*box_order+2] = (BYTE)(box_channel_status%10+0x30);

		//系统状态  默认正常add by mo 2020.10.30
		#if 0
		if(box_temperature_status == 0x30 || box_volt1_status   == 0x30 || box_volt2_status == 0x30 
		|| box_volt3_status 	  == 0x30 || box_volt4_status   == 0x30 || box_volt5_status == 0x30 
		|| box_door_status 		  == 0x30 || box_channel_status == 0x30)
			box_sys_status = 0;
		else
			box_sys_status = 1;
		#endif
		box_sys_status = 1;
		protocol->protcmsg.data[45 + 30*box_order+0] = (BYTE)(box_sys_status/100+0x30);
		protocol->protcmsg.data[45 + 30*box_order+1] = (BYTE)(box_sys_status%100)/10+0x30;;
		protocol->protcmsg.data[45 + 30*box_order+2] = (BYTE)(box_sys_status%10+0x30);

	}
	
	*len = BoxNum * 30 +18;
	protocol->protcmsg.length = *len;

	char ip_port[24];
	char logmsg[96];
	memset(ip_port,0,sizeof(ip_port));
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x %s",protocol->protcmsg.head.cmdID,"get device detail status");
	debug_printf("strlen(logmsg) = %d\n",strlen(logmsg));
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	return 0;
}


//获取设备像素状态数据
static int get_devicePoint(Protocl_t *protocol,unsigned int *len)
{
	unsigned long TempLength;
	unsigned int nlength;
	unsigned int TempCount;
	unsigned char pixels_flag = 0;
	uint16_t pixels_byte_count = 0;
	uint16_t badpixels = 0;
	uint8_t color = 0;

	//uint16_t dev_width = 0,dev_height = 0;

	//标记是否需要返回坏点
	pixels_flag = protocol->protcmsg.data[0];
	//指令执行情况是否正常
	protocol->protcmsg.data[0]  = 0x01;
	
	uint8_t pixStatus = 0x30;
	uint32_t pixLen = 0 ,BadPixels = 0;
	DP_GetPixelsStatus(&pixStatus,&BadPixels);
	DP_GetPixelsColor(&color);
	//失效点数
	badpixels = BadPixels;

	//这一位改为传颜色
	protocol->protcmsg.data[1]	= color+0x30;


	protocol->protcmsg.data[2]	= BadPixels%100000/10000+0x30;
	protocol->protcmsg.data[3]	= BadPixels%10000/1000+0x30;
	protocol->protcmsg.data[4]	= BadPixels%1000/100+0x30;
	protocol->protcmsg.data[5]	= BadPixels%100/10+0x30;
	protocol->protcmsg.data[6]	= BadPixels%10+0x30;
	DEBUG_PRINTF;
	uint32_t ScreenWidth,ScreenHeight;
	DP_GetScreenSize(&ScreenWidth,&ScreenHeight);
	pixels_byte_count = (ScreenWidth * ScreenHeight) / 8;
	debug_printf("ScreenWidth = %d,ScreenHeight = %d,pixels_count = %d\n",ScreenWidth,ScreenHeight,pixels_byte_count);
//7-9表示屏宽
	protocol->protcmsg.data[7]  = ScreenWidth/100+0x30;
	protocol->protcmsg.data[8]  = ScreenWidth%100/10+0x30;
	protocol->protcmsg.data[9]  = ScreenWidth%10+0x30;
	DEBUG_PRINTF;
	protocol->protcmsg.data[10] = ScreenHeight/100+0x30;
	protocol->protcmsg.data[11] = ScreenHeight%100/10+0x30;
	protocol->protcmsg.data[12] = ScreenHeight%10+0x30;
	DEBUG_PRINTF;

	if(pixels_flag == 0x30)  // 只返回坏点个数
	{
		*len=13;
		DEBUG_PRINTF;
	}
	else
	{
		DP_GetPixelsData(protocol->protcmsg.data + 13,&pixLen);
		*len = 13 + pixLen;
	}
	
	protocol->protcmsg.length = *len;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x %s",protocol->protcmsg.head.cmdID,"read device pixels status");
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	DEBUG_PRINTF;
}

static int get_deviceBright(Protocl_t *protocol,unsigned int *len)
{
	uint8_t status = 0;
	uint8_t led_bright = 0;
	uint8_t light_sensitiv = 0;
	uint8_t mode,Bright;
	uint8_t Bmax = 0,Bmin = 0;
	float div = 0.0,abright = 0.0;

	DEBUG_PRINTF;
	//DP_BrightAndMode(OPS_MODE_GET,&mode,&Bright,&Bmax,&Bmin);
	DP_GetBrightMode(&mode);
	DP_ReadBrightVals(&Bright);
	debug_printf("Bright = %d\n",Bright);
	led_bright = Bright;
	//指令执行情况
	protocol->protcmsg.data[0] = 0x01;                       
	//亮度模式
	protocol->protcmsg.data[1]= mode;
	
	//当前亮度
	DP_GetBrightRange(&Bmax,&Bmin);
	div = (Bmax - Bmin + 1) / (float)31;
	if(Bright >= 31)
		abright = Bmax;
	else
		abright = (Bright) * div;
	
	led_bright = (abright - (uint8_t)abright > 0.5) ? ((uint8_t)abright + 1) : ((uint8_t)abright);
	protocol->protcmsg.data[2] = (BYTE)(led_bright/10)+0x30;
	protocol->protcmsg.data[3] = (BYTE)(led_bright%10)+0x30;
	protocol->protcmsg.data[4] = (BYTE)(led_bright/10)+0x30;
	protocol->protcmsg.data[5] = (BYTE)(led_bright%10)+0x30;
	protocol->protcmsg.data[6] = (BYTE)(led_bright/10)+0x30;
	protocol->protcmsg.data[7] = (BYTE)(led_bright%10)+0x30;


	//发送卡采用的是TXRX还是扫描版
	uint16_t cardType = TRANSCARD_TXRX;
	DP_GetCardType(&cardType);
	if(cardType == TRANSCARD_TXRX)
	{
		DEBUG_PRINTF;

		
		DP_GetSysDataAndStatus(PID_LIGHT_SENSITIVE,&status,&light_sensitiv);
		//light_sensitiv = (light_sensitiv <= Bmin) ? Bmin : light_sensitiv;
		//light_sensitiv = (light_sensitiv >= Bmax) ? Bmax : light_sensitiv;
		//div = (Bmax - Bmin + 1) / (float)31;
		//abright = (light_sensitiv) * div;
		//light_sensitiv = (abright - (uint8_t)abright > 0.5) ? ((uint8_t)abright + 1) : ((uint8_t)abright);
	}
	else
	{
		DEBUG_PRINTF;
		uint8_t LSBrightA,LSBrightB;
		DP_GetLSData(&LSBrightA,&LSBrightB);
		if(LSBrightA > 1 && LSBrightB > 1)
			light_sensitiv = (LSBrightA + LSBrightB) / 2;
		else if(LSBrightA <= 1 && LSBrightB > 1)
			light_sensitiv = LSBrightB;
		else
			light_sensitiv = LSBrightA;
		debug_printf("light_sensitiv = %d\n",light_sensitiv);
	}
	

	//填充数据，上传给上位机
	protocol->protcmsg.data[8]=(BYTE)(light_sensitiv/10)+0x30;
	protocol->protcmsg.data[9]=(BYTE)(light_sensitiv%10)+0x30;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x get bright: mode:%x current bright:%d",
		protocol->protcmsg.head.cmdID,protocol->protcmsg.data[1],led_bright);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	*len = 10 ;
	protocol->protcmsg.length = *len;
	return 0;
}


static int get_playlist(Protocl_t *protocol,unsigned int *len)
{
	uint8_t playlist[48];
	uint8_t lislen = 0;
	char *pch = NULL;
	char *str = NULL;	

	memset(playlist,0x00,sizeof(playlist));
		
	//DP_CurPlayList(OPS_MODE_GET,playlist,&lislen);
	DP_GetCurPlayList(playlist,&lislen);
	
	str = playlist;
	lislen = strlen(str);
	debug_printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^str = %s\n",str);
	
	if(lislen > 7)
	{
		while((pch = strchr(str,'/')) != NULL)
		{
			pch += 1;
			str = pch;
		}
		
		if(pch == NULL && strlen(str) != 0)
			memcpy(playlist,str,strlen(str));
	}
	
	debug_printf("==================================================================playlist = %s,lislen = %d\n",playlist,lislen);
	
	//指令执行情况
	protocol->protcmsg.data[0] = 0x01;

	//当前显示列表
	memcpy(protocol->protcmsg.data+1,playlist,strlen(str));

	*len = 1 + strlen(str);
	protocol->protcmsg.length = *len;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x get playlist: list:%s",protocol->protcmsg.head.cmdID,playlist);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	return 0;
}

static int get_util_time(Protocl_t *protocol,unsigned int *len)
{
	time_t timer;
	struct tm *tblock;

	int year = 0;
	int month = 0;
	int day   = 0;

	int hour = 0,min = 0,sec = 0;

	uint8_t timestr[24];
	uint8_t timestrlen = 0;

	memset(timestr,0x00,sizeof(timestr));
	get_sys_time(timestr,&timestrlen);
	debug_printf("timestr = %s\n",timestr);

	sscanf(timestr,"%4d/%02d/%02d-%02d:%02d:%02d",&year,&month,&day,&hour,&min,&sec);
	
	debug_printf("year = %d,month = %d,day = %d,hour = %d,min = %d,sec = %d\n",
		year,month,day,hour,min,sec);

	//指令执行情况
	protocol->protcmsg.data[0] = 0x01;

	//年
	protocol->protcmsg.data[1]  = (uint8_t)(year / 100 / 10) + 0x30;
	protocol->protcmsg.data[2]  = (uint8_t)(year / 100 % 10) + 0x30;
	protocol->protcmsg.data[3]  = (uint8_t)(year % 100 / 10) + 0x30;
	protocol->protcmsg.data[4]  = (uint8_t)(year % 100 % 10) + 0x30;
	//月
	protocol->protcmsg.data[5]  = (uint8_t)(month / 10) + 0x30 ;
	protocol->protcmsg.data[6]  = (uint8_t)(month % 10) + 0x30 ;
	//日
	protocol->protcmsg.data[7]  = (uint8_t)(day / 10) + 0x30 ;
	protocol->protcmsg.data[8]  = (uint8_t)(day % 10) + 0x30 ;
	//时
	protocol->protcmsg.data[9]  = (uint8_t)(hour / 10) + 0x30 ;
	protocol->protcmsg.data[10] = (uint8_t)(hour % 10) + 0x30 ;
	//分
	protocol->protcmsg.data[11] = (uint8_t)(min / 10) + 0x30 ;
	protocol->protcmsg.data[12] = (uint8_t)(min % 10) + 0x30 ;
	//秒
	protocol->protcmsg.data[13] = (uint8_t)(sec / 10) + 0x30 ;
	protocol->protcmsg.data[14] = (uint8_t)(sec % 10) + 0x30 ;

	*len = 15;
	protocol->protcmsg.length = 15;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x get systime: time:%s",protocol->protcmsg.head.cmdID,timestr);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));

	return 0;
}

static int get_util_lasttime(Protocl_t *protocol,unsigned int *len)
{
	struct tm * StartTime;
	StartTime = localtime(&LastTimer);

	uint8_t timestr[24];
	uint8_t timestrlen = 0;

	int year = 0,month = 0,day = 0;
	int hour = 0,min = 0,sec = 0;

	memset(timestr,0x00,sizeof(timestr));
	//DP_BootUpTime(OPS_MODE_GET,timestr,&timestrlen);
	DP_GetBootupTime(timestr,&timestrlen);

	debug_printf("bootup time is [ %s ]\n",timestr);

	sscanf(timestr,"%4d/%02d/%02d-%02d:%02d:%02d",&year,&month,&day,&hour,&min,&sec);
	
	debug_printf("year = %d,month = %d,day = %d,hour = %d,min = %d,sec = %d\n",
		year,month,day,hour,min,sec);
	
	//年
	protocol->protcmsg.data[1]  = (uint8_t)(year / 100 / 10) + 0x30;
	protocol->protcmsg.data[2]  = (uint8_t)(year / 100 % 10) + 0x30;
	protocol->protcmsg.data[3]  = (uint8_t)(year % 100 / 10) + 0x30;
	protocol->protcmsg.data[4]  = (uint8_t)(year % 100 % 10) + 0x30;
	//月
	protocol->protcmsg.data[5]  = (uint8_t)(month / 10) + 0x30 ;
	protocol->protcmsg.data[6]  = (uint8_t)(month % 10) + 0x30 ;
	//日
	protocol->protcmsg.data[7]  = (uint8_t)(day / 10) + 0x30 ;
	protocol->protcmsg.data[8]  = (uint8_t)(day % 10) + 0x30 ;
	//时
	protocol->protcmsg.data[9]  = (uint8_t)(hour / 10) + 0x30 ;
	protocol->protcmsg.data[10] = (uint8_t)(hour % 10) + 0x30 ;
	//分
	protocol->protcmsg.data[11] = (uint8_t)(min / 10) + 0x30 ;
	protocol->protcmsg.data[12] = (uint8_t)(min % 10) + 0x30 ;
	//秒
	protocol->protcmsg.data[13] = (uint8_t)(sec / 10) + 0x30 ;
	protocol->protcmsg.data[14] = (uint8_t)(sec % 10) + 0x30 ;

	*len = 15;
	protocol->protcmsg.length = 15;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x get boot time: time:%s",protocol->protcmsg.head.cmdID,timestr);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));

	return 0;
}

static int get_util_vercheck(Protocl_t *protocol,unsigned int *len)
{
	/*协议没有此项内容，暂时留空*/
	return 0;
}

static int get_devID(Protocl_t *protocol,unsigned int *len)
{
	/*协议没有此项内容，暂时留空*/
	return 0;
}
static int set_devID(Protocl_t *protocol,unsigned int *len)
{
	/*协议没有此项内容，暂时留空*/
	return 0;
}

static int set_time(Protocl_t *protocol,unsigned int *len)
{
	
	uint8_t timestr[24];
	uint32_t year = 0;
	uint8_t month = 0;
	uint8_t day   = 0;
	uint8_t hour  = 0,min = 0,sec = 0;
	DEBUG_PRINTF;
	memset(timestr,0x00,sizeof(timestr));

	year = ((protocol->protcmsg.data[0] - 0x30)*10 + (protocol->protcmsg.data[1] - 0x30)) * 100 
			+ (protocol->protcmsg.data[2] - 0x30) * 10 + (protocol->protcmsg.data[3] - 0x30);

	month = (protocol->protcmsg.data[4] - 0x30) * 10 + (protocol->protcmsg.data[5] - 0x30);
	day	  = (protocol->protcmsg.data[6] - 0x30) * 10 + (protocol->protcmsg.data[7] - 0x30);
	hour  = (protocol->protcmsg.data[8] - 0x30) * 10 + (protocol->protcmsg.data[9] - 0x30);
	min   = (protocol->protcmsg.data[10] - 0x30) * 10 + (protocol->protcmsg.data[11] - 0x30);
	sec   = (protocol->protcmsg.data[12] - 0x30) * 10 + (protocol->protcmsg.data[13] - 0x30);

	sprintf(timestr,"%4d/%02d/%02d-%02d:%02d:%02d",year,month,day,hour,min,sec);
	
	set_sys_time(timestr,strlen(timestr));
//之前老版本都会出现同步时间错误，这里新版程序修改了	
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = protocol->protcmsg.length;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x set systime: time:%s",protocol->protcmsg.head.cmdID,timestr);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));

	return 0;
}


static int get_curplaying(Protocl_t *protocol,unsigned int *len)
{
	uint16_t ct_len = 0;
	char ct_str[1024];
	uint32_t DelayTime,Speed;
	uint8_t Inform,Order,Font,Size;
	memset(ct_str,0,1024);
	DEBUG_PRINTF;
	//change by mo
	//不带播放信息
	char buffer[4];
	memset(buffer,0,sizeof(buffer));
	conf_file_read(CHECKPATH,"playcontent","playcontent",buffer);
	if(buffer[0] == '3')
	{
		//带播放显示详情的
		DP_GetCurPlayItemContent(ct_str,&ct_len);
	}
	else
	{
		DP_GetCurPlayContent(ct_str,&ct_len,&Font,&Size,&Inform,&Speed,&DelayTime,&Order);
	}

	debug_printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!ct_len = %d\n",ct_len);
	DEBUG_PRINTF;
	protocol->protcmsg.data[0] = 0x01; 
	if(buffer[0] == '1')
	{
		//V1.2protocol read current string
		protocol->protcmsg.data[1] = 0x39;
		protocol->protcmsg.data[2] = 0x2e;
		protocol->protcmsg.data[3] = 0x20;
		memcpy(protocol->protcmsg.data + 4,ct_str,ct_len);
		protocol->protcmsg.length = ct_len + 4;
	}
	else
	{
		memcpy(protocol->protcmsg.data + 1,ct_str,ct_len);
		protocol->protcmsg.length = ct_len + 1;
	}
	
	*len = protocol->protcmsg.length;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x read current playing conten: %s",protocol->protcmsg.head.cmdID,ct_str);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	return 0;
}


static int set_bright(Protocl_t *protocol,unsigned int *len)
{
	uint8_t BrightV;
	uint8_t SBmode = 0,GBmode = 0,SBright = 0,GBright,Bmax,Bmin;
	uint8_t R_bright = 0,G_bright = 0,B_bright = 0;
	
	uint16_t width,height,count;
	//读取要设置的亮度控制模式与亮度
	SBmode = protocol->protcmsg.data[0];
	SBright = (protocol->protcmsg.data[1]-0x30)*10+(protocol->protcmsg.data[2]-0x30);
	BrightV = SBright;
	//设置亮度模式
	DP_SetBrightMode(SBmode);

	//自动亮度时不予设置亮度，但要将配置的亮度模式写入配置文件
	if(SBmode == BRIGHT_AUTO)
	{
		DEBUG_PRINTF;
		conf_file_write(ConFigFile,"brightmode","mode","31");
		goto EXCEPTION;
	}
	
	//对上位机发送的亮度值限定范围
	if(SBright <= 0) SBright = 0;
	//if(SBright >= 31)SBright = 31;
	//保存亮度值,不需要转换(上位机发过来的数据就是在0-31的范围)。
	//但在设置亮度的时候需要先转换在设置
	//DP_SaveBrightVals(SBright);


	//针对扫描版的亮度设置
	HW2G400_SETLEDbright(SBright);
	debug_printf("**SBright = %d\n",SBright);

	//针对TXRX卡的亮度设置
	float div = 0.0,Abright = 0.0;
	uint8_t IntBright = 0;
	DP_GetBrightRange(&Bmax,&Bmin);
	div = (Bmax - Bmin + 1) / (float)31;
	Abright = SBright / div;
	IntBright = (Abright - (uint8_t)Abright > 0.5) ? ((uint8_t)Abright + 1) : ((uint8_t)Abright);
	SBright = IntBright;
	SBright = (SBright <= Bmin) ? Bmin : SBright;
	SBright = (SBright >= Bmax) ? Bmax : SBright;

	//debug_printf("**SBright = %d\n",SBright);
	
	DP_SaveBrightVals(SBright);
	uint8_t MaxBright,MinBright=0;
	DP_GetBrightRealRange(&MaxBright, &MinBright);
	//printf("MaxBright is %d MinBright is %d\n",MaxBright,MinBright);
	div = (Bmax - Bmin + 1) / (float)MaxBright;
	Abright = SBright / div;
	IntBright = (Abright - (uint8_t)Abright > 0.5) ? ((uint8_t)Abright + 1) : ((uint8_t)Abright);
	SBright = IntBright;
	SBright = (SBright <= MinBright) ? MinBright : SBright;
	SBright = (SBright >= MaxBright) ? MaxBright : SBright;
	//printf("bright is %d\n",SBright);
	Set_LEDBright(SBright);
	//将亮度模式与亮度值保存到配置文件中
	if(SBmode == BRIGHT_HAND)
	{
		char brightVals[4];
		memset(brightVals,0,sizeof(brightVals));
		sprintf(brightVals,"%d",BrightV);
		conf_file_write(ConFigFile,"brightmode","mode","30");
		conf_file_write(ConFigFile,"brightmode","bright",brightVals);		
		debug_printf("SBmode = 0x%x,R_bright = 0x%x,G_bright = 0x%x,B_bright = 0x%x\n",SBmode,R_bright,G_bright,B_bright);
	}

	//指令执行情况
	protocol->protcmsg.data[0]= 0x01;
	protocol->protcmsg.data[1]= SBmode;
	//CmsStatus = nXKCommunicate;
	*len=2;
	protocol->protcmsg.length = *len;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x set bright: mode:%x bright:%d",protocol->protcmsg.head.cmdID,SBmode,R_bright);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	
	return 0;

	
	EXCEPTION:
		protocol->protcmsg.data[0]= 0x01;
		protocol->protcmsg.data[1]= SBmode;
		//CmsStatus = nXKCommunicate;
		*len=2;
		protocol->protcmsg.length = *len;
		return 0;

}


static int set_devopenclose(Protocl_t *protocol,unsigned int *len)
{
	uint8_t ScreenStatus = 0;
	uint8_t operat_state = 0;
	uint8_t power 		 = 0;
	DEBUG_PRINTF;
	power 		 = protocol->protcmsg.data[0];
	operat_state = protocol->protcmsg.data[1];
	// 命令执行, 打开设备或者关闭设备
	if(operat_state == 0x31)
	{
		DEBUG_PRINTF;
		SET_LED_STATE(SLED_ON);
		HW2G400_SetScreenStatus(LED_STATUS_ON);
		LEDstateRecord(SLED_ON);
		ScreenStatus = LED_STATUS_ON;
	}
	else
	{
		DEBUG_PRINTF;
		SET_LED_STATE(SLED_OFF);
		LEDstateRecord(SLED_OFF);
		HW2G400_SetScreenStatus(LED_STATUS_OFF);
		ScreenStatus = LED_STATUS_OFF;
	}

	//DP_SetScreenStatus(ScreenStatus);

	
	
	// 组建回传应用数据
	protocol->protcmsg.data[2]= (BYTE)(protocol->protcmsg.data[1]);
	protocol->protcmsg.data[1]= (BYTE)(protocol->protcmsg.data[0]);
	protocol->protcmsg.data[0]= 0x01;

	*len = 3;
	protocol->protcmsg.length = *len;

	char ip_port[24];
	char logmsg[96];
	char devstate[24];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	if(operat_state == 0x31)
		sprintf(devstate,"%s","turn on the screen");
	else
		sprintf(devstate,"%s","turn off the screen");
	sprintf(logmsg,"cmd:%x %s",protocol->protcmsg.head.cmdID,devstate);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));

	return 0;
}


static int set_devpowerMode(Protocl_t *protocol,unsigned int *len)
{
	uint8_t power_mode = 0;

	power_mode = protocol->protcmsg.data[0];
	if(power_mode == 0x31)
	{
		//set_powerMode(0x01);
	}
	else
	{
		//set_powerMode(0x00);
	}
	
	protocol->protcmsg.data[0] = 0x01;
	*len = 1;
	protocol->protcmsg.length = *len;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x set power mode: mode:%x",protocol->protcmsg.head.cmdID,((power_mode == 0x31) ? 0x31 : 0x30));
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	
	return 0;
}

static int set_devReset(Protocl_t *protocol,unsigned int *len)
{
	#if 0
	/*设置设备重新启动*/
	char systemstr[48];
	char cur_workdir[128];
	memset(cur_workdir,0x00,sizeof(cur_workdir));
	//getcwd(cur_workdir,sizeof(cur_workdir));
	sprintf(cur_workdir,"%s",sys_dir);
	chdir(cur_workdir);

	sprintf(systemstr,"sh %s","sh.sh");
	system(systemstr);
	#endif
	int ret = 0;
	protocol->protcmsg.data[0] = 0x01;
	*len = 1;
	protocol->protcmsg.length = *len;
	
	//wdt_feed(WDT_FEED_STOP);
	//wdt_stop();
	DEBUG_PRINTF;
	ret = open(sys_dir"/sys/sys_reboot.lock",O_WRONLY | O_CREAT,0744);
	if(ret < 0)
		perror("set_devReset open");

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x reset the device",protocol->protcmsg.head.cmdID);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	
	return 0;
}

static int get_smok(Protocl_t *protocol,unsigned int *len)
{
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.data[1] = 0x30;
	
	protocol->protcmsg.data[2] = 0x30;
	protocol->protcmsg.data[3] = 0x33;
	protocol->protcmsg.data[4] = 0x30;
	
	*len = 5;
	protocol->protcmsg.length = 5;
	return 0;
}


int get_communitStatus(Protocl_t *protocol,unsigned int *len)
{
	//指令执行情况
	protocol->protcmsg.data[0] = 0x01;
	*len = 1;
	protocol->protcmsg.length = *len;

	char ip_port[24];
	memset(ip_port,0,sizeof(ip_port));
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	_log_file_write_("userlog",ip_port,strlen(ip_port),"cmd:3030,not defined!",strlen("cmd:3030,not defined!"));
	
	return 0;
}


static int get_devDriverStatus(Protocl_t *protocol,unsigned int *len)
{

	uint8_t channel_status = 0,channel_bad = 0;
	uint16_t box_width= 0,box_height = 0,box_number = 0;
	uint16_t CHNnumber = 0;
	uint8_t i = 0,j = 0;

	//获取箱大小与数量
	uint16_t BoxWidth,BoxHeight;
	uint32_t BoxNum;
	//DP_BoxSize(OPS_MODE_GET,&BoxWidth,&BoxHeight,&BoxNum);
	DP_GetBoxSize(&BoxWidth,&BoxHeight,&BoxNum);

	//计算每个箱体有多少个驱动通道,(每个驱动通道有8列像素点)
	//channel_number = box_width / 8;
	//指令执行情况
	protocol->protcmsg.data[0] = 0x01;
	
	for(i = 0 ; i < BoxNum ; i++)
	{
		for(j = 0 ; j < 6 ; j++)
			protocol->protcmsg.data[1 + 6 * i + j] = 0x31;
	}
	
	*len = BoxNum * 6 + 1; 
	protocol->protcmsg.length = *len;

	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x get the driver status",protocol->protcmsg.head.cmdID);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));

	return 0;

}

//测试模式
static int set_devtestmode(Protocl_t *protocol,unsigned int *len)
{
	uint8_t state;
	state = protocol->protcmsg.data[0];
	Set_TestState(state);

	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	return 0;

}


static int ResetTxCard(Protocl_t *protocol,unsigned int *len)
{

	TxCardReset();
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	return 0;
}

static int ResetRxCard(Protocl_t *protocol,unsigned int *len)
{
	uint8_t adrdata = protocol->protcmsg.data[0];
	RxCardReset(&adrdata);
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	return 0;
}


static int SetMudulePowerState(Protocl_t *protocol,unsigned int *len)
{
	
	if(protocol->protcmsg.data[0] == 0x31)
	{
		//ModulePower(MODULE_ON);
	}
	if(protocol->protcmsg.data[0] == 0x30)
	{
		//ModulePower(MODULE_OFF);
	}

	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	return 0;
}

static int GetVersionAll(Protocl_t *protocol,unsigned int *len)
{
	protocol->protcmsg.data[0] = 0x01;
	
	//工控机守护程序版本
	DP_Get_MonitorVersion(protocol->protcmsg.data);

	//工控机版本
	DP_Get_APPVersion(protocol->protcmsg.data);

	//发送卡版本
	DP_Get_TXVersion(protocol->protcmsg.data);

    //接收卡版本	
    DP_Get_RXVersion(protocol->protcmsg.data);

	protocol->protcmsg.length = (protocol->protcmsg.data[1] * 2 + 8);
	*len = protocol->protcmsg.length;
	debug_printf("RXnum is %d ,length is %d\n",protocol->protcmsg.data[1],protocol->protcmsg.length);

	
}

static int SetPixelsMode(Protocl_t *protocol,unsigned int *len)
{

	uint8_t pixmode = 0;
	pixmode = protocol->protcmsg.data[0];
	#if 0
	if(pixmode >= 0x00 && pixmode < 0x04)
	{
		
		Set_Pixels_Mode(pixmode);
		protocol->protcmsg.data[0] = 0x01;
	}
	else
	{
		protocol->protcmsg.data[0] = 0x00;
	}
	#endif
	Set_Pixels_Mode(pixmode);
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = protocol->protcmsg.length;
}



static int GetDisplayParameter(Protocl_t *protocol,unsigned int *len)
{

	Get_DisplayParameter();
	usleep(300*1000);
	int ret;
	ret = DP_Get_Display_Parameter(protocol->protcmsg.data,&(protocol->protcmsg.length));
	//*len = protocol->protcmsg.length;
	if(ret == -1)
	{
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
	}

}

static int ConfigFileOps(void)
{
	//备份一个恢复IP用的_cls.conf以及一个正常使用的备份cpy_cls.conf,防止
	//cls.conf被意外修改或者损坏时恢复
	if(access(config_sh,F_OK) >= 0)
		system(config_sh);

	//修改恢复IP的cls.conf的ip、网关、掩码为默认值
	conf_file_write(_cls,"netport","ip","192.168.1.11");
	conf_file_write(_cls,"netport","netmask","255.255.255.0");
	conf_file_write(_cls,"netport","gateway","192.168.1.1");
	conf_file_write(_cls,"netport","port","5168");
	conf_file_write(f_cls,"netport","ip","192.168.1.11");
	conf_file_write(f_cls,"netport","netmask","255.255.255.0");
	conf_file_write(f_cls,"netport","gateway","192.168.1.1");
	conf_file_write(f_cls,"netport","port","5168");


	//接受到上位机发送的配置文件后，要把ip、掩码、网关写入一个脚本文件ipconfig.sh中，再系统
	//启动后就会自动调用这个文件来配置IP信息
	int fd = -1;
	char Wcontent[256];
	char ip[24],netmask[24],gateway[24];
	memset(ip,0,sizeof(ip));
	memset(netmask,0,sizeof(netmask));
	memset(gateway,0,sizeof(gateway));
	memset(Wcontent,0,sizeof(Wcontent));
	conf_file_read(ConFigFile,"netport","ip",ip);
	conf_file_read(ConFigFile,"netport","netmask",netmask);
	conf_file_read(ConFigFile,"netport","gateway",gateway);
	sprintf(Wcontent,"#!/bin/sh\n\nifconfig eth0 %s netmask %s up >/dev/null 2>&1\n/sbin/route add default gw %s\n",ip,netmask,gateway);
	fd = open(IPCONFIG,O_RDWR | O_CREAT,0744);
	if(fd < 0)
	{
		return -1;
	}
	
	//printf("Wcontent = %s\n",Wcontent);
	
	lseek(fd,0,SEEK_SET);
	write(fd,Wcontent,strlen(Wcontent));
	close(fd);
	//system(IPCONFIG);
	
	return 0;
}



static int File_FrameRxFrmUpper2K(Protocl_t *protocol,unsigned int *len)
{
	int retvals = -1;
	uint16_t CurOffset = 0;
	uint8_t FileNameLen = 0;
	char FileNameLenStr[4];
	char FileName[48];
	char *FrameIdStr = NULL;
	uint32_t FrameId = 0;
	uint32_t FrameLen = 0;
	char *FrameContent = NULL;

	//取文件名长度
	CurOffset += 2;
	memset(FileNameLenStr,0,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,protocol->protcmsg.data + CurOffset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//取文件名
	CurOffset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,protocol->protcmsg.data + CurOffset,FileNameLen);
	FileName[FileNameLen] = '\0';

	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//检查上位机上传过来的文件路径，没有响应文件夹就创建相应的文件夹
	char *p = NULL;
	chdir(sys_dir);
	p = strchr(FileName,'/');
	if(p != NULL)
	{
		DEBUG_PRINTF;
		mkdirs(FileName);
		chdir(sys_dir);
	}

	//帧偏移量
	CurOffset += FileNameLen;
	FrameIdStr = protocol->protcmsg.data + CurOffset;
	FrameId = (FrameIdStr[0] - 0x30) * 1000 + (FrameIdStr[1] - 0x30) * 100 + 
			  (FrameIdStr[2] - 0x30) * 10 + (FrameIdStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",FrameIdStr[0],FrameIdStr[1],FrameIdStr[2],FrameIdStr[3]);
	debug_printf("FrameId = %d\n",FrameId);

	//帧内容长度
	CurOffset += 4;
	FrameLen = protocol->protcmsg.length - CurOffset;

	//帧内容
	FrameContent = protocol->protcmsg.data + CurOffset;

	//创建文件路径
	char FilePwd[64];
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	
	debug_printf("FilePwd = %s,FrameLen = %d\n",FilePwd,FrameLen);

	debug_printf("protocol->usermsg->ip = %s\n",protocol->usermsg->ip);
	//初始化用户所属
	FILEUser_t FileUser;
	memset(&FileUser,0,sizeof(FileUser));
	FRTx_FileUserInit(&FileUser,protocol->usermsg->type,protocol->usermsg->ip,protocol->usermsg->port,protocol->usermsg->uartPort);
	DEBUG_PRINTF;
	//帧数据存文件
	retvals = FRTx_FileFrameRx2K(&FileUser,FilePwd,FrameContent,FrameLen,FrameId);
	if(retvals < 0)
		goto EXCEPTION;

	//接收结束
	if(retvals == 1)
	{
		if(strncmp(FileName,"config/cls.conf",15) == 0)
		{
			ResetTxRxCardMsg();
			debug_printf("FileName = %s\n",FileName);
			ConfigFileOps();
		}
		if(strncmp(FileName,"image/mp0.bmp",13) == 0)
		{
			content.refresh = LST_REFLASH;
			//content.refresh = FLAG_RESFRESH;
		}

		DEBUG_PRINTF;
		chmod(FilePwd,0744);
	}
	
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	return 0;

	EXCEPTION:
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
		return -1;
}


static int File_FrameTxToUpper2K(Protocl_t *protocol,unsigned int *len)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileNameLenStr[4];
	int FileNameLen = 0;
	char FileName[48];
	char FrameIDStr[4];
	uint32_t FrameID = 0;
	char *FrameContent = NULL;
	char FilePwd[64];
	uint32_t FrameLen = 0;
	
	// 获得文件名长度
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,protocol->protcmsg.data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//获取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,protocol->protcmsg.data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//将文件路径中的'\'转化成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//获取文件偏移地址
	Offset += FileNameLen;
	memset(FrameIDStr,0,sizeof(FrameIDStr));
	memcpy(FrameIDStr,protocol->protcmsg.data + Offset,4);
	FrameID = ((uint8_t)FrameIDStr[0] - 0x30) * 1000 + ((uint8_t)FrameIDStr[1] - 0x30) * 100 + ((uint8_t)FrameIDStr[2] - 0x30) * 10 + ((uint8_t)FrameIDStr[3] - 0x30);
	debug_printf("0x%x,0x%x,0x%x,0x%x,%d\n",FrameIDStr[0],FrameIDStr[1],FrameIDStr[2],FrameIDStr[3],FrameID);
	//文件内容在一帧返回帧数据中位置
	Offset += 4;
	FrameContent = protocol->protcmsg.data + Offset + 1;

	//初始化所属用户
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,protocol->usermsg->type,protocol->usermsg->ip,protocol->usermsg->port,protocol->usermsg->uartPort);

	//获取要读取的文件路径
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//开始读文件
	ret = FRTx_FileFrameTx2K(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameID);
	if(ret < 0)
		goto EXCEPTION;

	dir_linuxtowin(FileName);
	//组帧回传
	Offset = 0;
	protocol->protcmsg.data[Offset] = 0x01;
	Offset += 1;
	memcpy(protocol->protcmsg.data + Offset,FileNameLenStr,3);
	Offset += 3;
	memcpy(protocol->protcmsg.data + Offset,FileName,FileNameLen);
	Offset += FileNameLen;
	memcpy(protocol->protcmsg.data + Offset,FrameIDStr,4);
	char *pp = protocol->protcmsg.data + Offset;
	debug_printf("0x%x,0x%x,0x%x,0x%x  0x%x,0x%x,0x%x,0x%x\n",pp[0],pp[1],pp[2],pp[3],FrameIDStr[0],FrameIDStr[1],FrameIDStr[2],FrameIDStr[3]);
	Offset += 4;
	protocol->protcmsg.length = Offset + FrameLen;
	debug_printf("FrameLen = %d,%d\n",FrameLen,protocol->protcmsg.length);

	return 0;


	EXCEPTION:
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
		return -1;
}


static int File_FrameRxFrmUpper16K(Protocl_t *protocol,unsigned int *len)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileName[48];
	char FilePwd[64];
	char FileNameLenStr[4];
	int  FileNameLen;
	char *FrameOffsetStr = NULL;
	int  FrameOffset = 0;
	char *FrameContent = NULL;
	int bmpnumber = 0;
	int	 FrameLen = 0;

	//取文件名长度
	Offset += 2;
	memcpy(FileNameLenStr,protocol->protcmsg.data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,protocol->protcmsg.data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//文件路径'\'转换成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//检查上位机上传过来的文件路径，没有响应文件夹就创建相应的文件夹
	char *p = NULL;
	chdir(sys_dir);
	p = strchr(FileName,'/');
	if(p != NULL)
	{
		DEBUG_PRINTF;
		mkdirs(FileName);
		chdir(sys_dir);
	}
	

	//偏移到文件偏移位置
	Offset += FileNameLen;
	FrameOffsetStr = protocol->protcmsg.data + Offset;
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];

	debug_printf("FrameOffset = %d\n",FrameOffset);
	//偏移到文件内容位置
	Offset += 4;
	FrameContent = protocol->protcmsg.data + Offset;

	//帧长度
	FrameLen = protocol->protcmsg.length - Offset;

	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEUser_t));
	memcpy(FILEuser.ip,protocol->usermsg->ip,strlen(protocol->usermsg->ip));
	FILEuser.port = 5168;
	FILEuser.userType = 0;
	FILEuser.comx = 0;

	//获取要读取的文件路径image/user/100.bmp
	memset(FilePwd,0,sizeof(FilePwd));
	debug_printf("atoi(FileName + 6) = %s\n",FileName + 6);
	if(strstr(FileName,".bmp") != NULL && atoi(FileName + 6) >= 60)
		sprintf(FilePwd,"%s/user/%s",image_dir,FileName + 6);
	else
		sprintf(FilePwd,"%s/%s",sys_dir,FileName);
		
	
	debug_printf("*FilePwd = %s\n",FilePwd);

	ret = FRTx_FileFrameRx16K(&FILEuser,FilePwd,FrameContent,FrameLen,FrameOffset);
	if(ret == 1)
	{
		if(strncmp(FileName,"config/cls.conf",15) == 0)
		{
			//这个操作是多余的
			ResetTxRxCardMsg();
			debug_printf("FileName = %s\n",FileName);
			ConfigFileOps();
		}
		DEBUG_PRINTF;
		chmod(FilePwd,0744);
	}

	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	return 0;
	
}



static int File_FrameTxToUpper16K(Protocl_t *protocol,unsigned int *len)
{
	int ret = -1;
	uint32_t Offset = 0;
	char FileNameLenStr[4];
	int FileNameLen = 0;
	char FileName[48];
	char FrameOffsetStr[4];
	uint32_t FrameOffset = 0;
	char *FrameContent = NULL;
	char FilePwd[64];
	uint32_t FrameLen = 0;
	
	// 获得文件名长度
	Offset += 0;
	memset(FileNameLenStr,0x00,sizeof(FileNameLenStr));
	memcpy(FileNameLenStr,protocol->protcmsg.data + Offset,3);
	FileNameLenStr[3]='\0';
	FileNameLen = atol(FileNameLenStr);

	//获取文件名
	Offset += 3;
	memset(FileName,0x00,sizeof(FileName));
	memcpy(FileName,protocol->protcmsg.data + Offset,FileNameLen);
	FileName[FileNameLen] = '\0';

	//将文件路径中的'\'转化成'/'
	dir_wintolinux(FileName);
	Dir_LetterBtoL(FileName);
	debug_printf("FileName = %s\n",FileName);

	//获取文件偏移地址
	Offset += FileNameLen;
	memset(FrameOffsetStr,0,sizeof(FrameOffsetStr));
	memcpy(FrameOffsetStr,protocol->protcmsg.data + Offset,4);
	FrameOffset = (uint8_t)FrameOffsetStr[0] << 24 | (uint8_t)FrameOffsetStr[1] << 16 | (uint8_t)FrameOffsetStr[2] << 8 | (uint8_t)FrameOffsetStr[3];
	debug_printf("0x%x,0x%x,0x%x,0x%x\n",FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3]);
	//文件内容在一帧返回帧数据中位置
	Offset += 4;
	FrameContent = protocol->protcmsg.data + Offset + 1;

	//初始化所属用户
	FILEUser_t FILEuser;
	memset(&FILEuser,0,sizeof(FILEuser));
	FRTx_FileUserInit(&FILEuser,protocol->usermsg->type,protocol->usermsg->ip,protocol->usermsg->port,protocol->usermsg->uartPort);

	//获取要读取的文件路径
	memset(FilePwd,0,sizeof(FilePwd));
	sprintf(FilePwd,"%s/%s",sys_dir,FileName);
	debug_printf("*FilePwd = %s\n",FilePwd);
	
	//开始读文件
	ret = FRTx_FileFrameTx16K(&FILEuser,FilePwd,FrameContent,&FrameLen,FrameOffset);
	if(ret < 0)
		goto EXCEPTION;

	DEBUG_PRINTF;
	//组帧回传
	Offset = 0;
	protocol->protcmsg.data[Offset] = 0x01;
	Offset += 1;
	memcpy(protocol->protcmsg.data + Offset,FileNameLenStr,3);
	Offset += 3;
	memcpy(protocol->protcmsg.data + Offset,FileName,FileNameLen);
	Offset += FileNameLen;
	memcpy(protocol->protcmsg.data + Offset,FrameOffsetStr,4);
	char *pp = protocol->protcmsg.data + Offset;
	debug_printf("0x%x,0x%x,0x%x,0x%x  0x%x,0x%x,0x%x,0x%x\n",pp[0],pp[1],pp[2],pp[3],FrameOffsetStr[0],FrameOffsetStr[1],FrameOffsetStr[2],FrameOffsetStr[3]);
	Offset += 4;
	protocol->protcmsg.length = Offset + FrameLen;
	debug_printf("FrameLen = %d,%d\n",FrameLen,protocol->protcmsg.length);

	return 0;


	EXCEPTION:
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
		return -1;
}


static int SetWarmingSwitch(Protocl_t *protocol,unsigned int *len)
{
	if(protocol->protcmsg.data[0] == 0x31)
	{
		//SetWarmingState(WARM_ON);
	}
	else if(protocol->protcmsg.data[0] == 0x30)
	{
		//SetWarmingState(WARM_OFF);
	}
	else
	{
		goto EXEPTION;
	}
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	return 0;

	EXEPTION:
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
		*len = 1;
		return -1;
}


int set_preset_playlist(Protocl_t *protocol,unsigned int *len)
{
	int ret = -1;
	uint8_t Len = 0;
	FILE *stream;
	char ListName[64];
	char playlist[8];
	DEBUG_PRINTF;
	memset(playlist,0x00,sizeof(playlist));
	memset(ListName,0x00,sizeof(ListName));

	memcpy(playlist,protocol->protcmsg.data,7);
	playlist[7] = '\0';

	Dir_LetterBtoL(playlist);
	
	sprintf(ListName,"%s/%s",list_dir_1,playlist);
	ListName[strlen(ListName)] = '\0';
	debug_printf("ListName = %s\n",ListName);

	//检查该播放表是否存在
	if(access(ListName,F_OK) < 0)
	{
		debug_printf("The list [%s]file is not exist!\n",ListName);
		goto EXCEPTION;
	}

	SWR_Lstparsing(&content,ListName);
	DEBUG_PRINTF;

	//DP_CurPlayList(OPS_MODE_SET,CurPLst,&Len);
	DP_SetCurPlayList(playlist,7);
	conf_file_write(ConFigFile,"playlist","list",playlist);

	DEBUG_PRINTF;
	protocol->protcmsg.data[0] = 0x01;
	protocol->protcmsg.length = 1;
	*len = 1;
	__debug_printf;
	char ip_port[24];
	char logmsg[96];
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x set playlist: list:%s",protocol->protcmsg.head.cmdID,playlist);
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	__debug_printf;
	return 0;

  	EXCEPTION:
		protocol->protcmsg.data[0] = 0x00;
		protocol->protcmsg.length = 1;
		return 0;
}



static PTCL_err_t struct_to_bytes(Protocl_t *protocol,unsigned char *outputbytes)
{
	unsigned int i = 0;
	uint8_t ret = -1;
	uint32_t outputlen = 0;
	uint32_t offset = 0;
	if(protocol == NULL)
		return PTCL_ERR_ARGUMENT;
	
	outputbytes[START_BYTE_POS] = protocol->startByte;
	outputbytes[CMDID_BYTE_POS + 0] = (unsigned char)(protocol->protcmsg.head.cmdID >> 8);
	outputbytes[CMDID_BYTE_POS + 1] = (unsigned char)(protocol->protcmsg.head.cmdID);
	outputbytes[DEVID_BYTE_POS + 0] = (unsigned char)(protocol->protcmsg.head.devID >> 8);
	outputbytes[DEVID_BYTE_POS + 1] = (unsigned char)(protocol->protcmsg.head.devID);


	for(i = 0 ; i < protocol->protcmsg.length; i ++)
	{
		//debug_printf("0x%x\t",protocol->protcmsg.data[i]);
		outputbytes[DATAS_BYTE_POS + i]	= protocol->protcmsg.data[i];
	}

	outputbytes[PARITY_BYTE_POS(protocol->protcmsg.length) + 0] = (unsigned char)(protocol->protcmsg.parity >> 8);
	outputbytes[PARITY_BYTE_POS(protocol->protcmsg.length) + 1] = (unsigned char)(protocol->protcmsg.parity);
	outputbytes[END_BYTE_POS(protocol->protcmsg.length)] = protocol->endByte;
}

static PTCL_err_t Byte_to_struct(Protocl_t *protocol,unsigned char *Bytestr,unsigned int Bytelen)
{
	if(protocol == NULL || Bytestr == NULL)
		return PTCL_ERR_ARGUMENT;

	protocol->startByte	= Bytestr[0];
	protocol->endByte	= Bytestr[Bytelen - 1];
	protocol->protcmsg.head.cmdID = (Bytestr[1] << 8) | Bytestr[2];
	protocol->protcmsg.head.devID = (Bytestr[3] << 8) | Bytestr[4];
	protocol->protcmsg.parity = Bytestr[Bytelen - 3] << 8 | Bytestr[Bytelen - 2];
	protocol->protcmsg.length	= Bytelen - 8;
	protocol->protcmsg.data = Bytestr + 5;
	return PTCL_ERR_OK;
}





int set_netport(Protocl_t *protocol,unsigned int *len)
{
	unsigned char dev_ip[24] , dev_mask[24] , dev_gw[24] , dev_port[8];
	unsigned char *ip = NULL,*mask = NULL,*gw = NULL,*port = NULL;
	unsigned short _port = 0;

	char conf_file[64];
	memset(conf_file,0x00,sizeof(conf_file));
	sprintf(conf_file,"%s/cls.conf",conf_dir);
	
	ip 	 = protocol->protcmsg.data + 6;
	mask = protocol->protcmsg.data + 10;
	gw   = protocol->protcmsg.data + 14;
	port = protocol->protcmsg.data + 24;

	_port = port[0] << 8 | port[1] << 0;
	if(_port == 0)
		_port = 5168;
	
	memset(dev_ip,0,sizeof(dev_ip));
	memset(dev_mask,0,sizeof(dev_mask));
	memset(dev_gw,0,sizeof(dev_gw));
	memset(dev_port,0,sizeof(dev_port));

	debug_printf("ip[0] = %d\n",ip[0]);
	sprintf(dev_ip,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
	sprintf(dev_mask,"%d.%d.%d.%d",mask[0],mask[1],mask[2],mask[3]);
	sprintf(dev_gw,"%d.%d.%d.%d",gw[0],gw[1],gw[2],gw[3]);
	sprintf(dev_port,"%d",_port);

	debug_printf("dev_ip = %s,dev_mask = %s,dev_gw = %s\n",dev_ip,dev_mask,dev_gw);

	if(access(conf_file,F_OK) < 0)
		goto EXEPTION;

	//1、先将IP、掩码、网关信息写入配置文件，再将配置文件备份
	conf_file_write(conf_file,"netport","ip",dev_ip);
	conf_file_write(conf_file,"netport","netmask",dev_mask);
	conf_file_write(conf_file,"netport","gateway",dev_gw);
	conf_file_write(conf_file,"netport","port",dev_port);

	ConfigFileOps();
	
	//system(config_sh);
	//set_devip(dev_ip,dev_mask,dev_gw);

	

	protocol->protcmsg.data[2] = 0x31;
	protocol->protcmsg.length = 3;
	*len = 3;

	char ip_port[24];
	char logmsg[96];
	memset(ip_port,0,sizeof(ip_port));
	sprintf(ip_port,"%s:%d",protocol->usermsg->ip,protocol->usermsg->port);
	sprintf(logmsg,"cmd:%x setnet:ip: %s netmask:%s gateway:%s port:%s",
		protocol->protcmsg.head.cmdID,dev_ip,dev_mask,dev_gw,dev_port);
	debug_printf("strlen(logmsg) = %d\n",strlen(logmsg));
	_log_file_write_("userlog",ip_port,strlen(ip_port),logmsg,strlen(logmsg));
	return 0;

	EXEPTION:
		protocol->protcmsg.data[2] = 0x31;
		protocol->protcmsg.length = 3;
		*len = 3;
		return -1;
}

int FunctonExtend(Protocl_t *protocol,unsigned int *len)
{
	return 0;
}



void PROTOCOLInterfInit(void)
{
	//默认的显科协议接口
	PROTOCOLStruct.GetCommunitStatus 	= get_communitStatus;
	PROTOCOLStruct.GetDevTotalStatus	= get_deviceStatus;
	PROTOCOLStruct.SetDevIP				= set_netport;
	PROTOCOLStruct.GetDevDetailStatus 	= get_deviceDetail;
	PROTOCOLStruct.GetDevPixStatus		= get_devicePoint;
	PROTOCOLStruct.GetDevBright			= get_deviceBright;
	PROTOCOLStruct.SetDevBright			= set_bright;
	PROTOCOLStruct.GetCurPlayLst		= get_playlist;
	PROTOCOLStruct.SetCurPlayLst		= set_preset_playlist;
	PROTOCOLStruct.GetCurSysTime		= get_util_time;
	PROTOCOLStruct.GetSysStartTime		= get_util_lasttime;
	PROTOCOLStruct.SetSysCurTime		= set_time;
	PROTOCOLStruct.GetCurPlayString		= get_curplaying;
	PROTOCOLStruct.SetScreenStatus		= set_devopenclose;
	PROTOCOLStruct.SetDevPowerMode		= set_devpowerMode;
	PROTOCOLStruct.SetSysReset			= set_devReset;
	PROTOCOLStruct.getSmok				= get_smok;
	//这里需要修改
    PROTOCOLStruct.GetDevDrvStatus		= get_devDriverStatus;
	PROTOCOLStruct.SetDevTestMode       = set_devtestmode;
		
	PROTOCOLStruct.FileFrameRX2K		= File_FrameRxFrmUpper2K;//file_recv;
	PROTOCOLStruct.FileFrameTX2K		= File_FrameTxToUpper2K;
	PROTOCOLStruct.FileFrameRX16K		= File_FrameRxFrmUpper16K;
	PROTOCOLStruct.FileFrameTX16K		= File_FrameTxToUpper16K;
	PROTOCOLStruct.SetWarming			= SetWarmingSwitch;
	PROTOCOLStruct.SetRstTxCard			= ResetTxCard;
	PROTOCOLStruct.SetRstRxCard			= ResetRxCard;
	PROTOCOLStruct.SetModulePower       = SetMudulePowerState;
//加入版本查询
	PROTOCOLStruct.GetVersion           = GetVersionAll;
	PROTOCOLStruct.SetPixMode			= SetPixelsMode;
	PROTOCOLStruct.GetDisplayParameter  = GetDisplayParameter;
	PROTOCOLStruct.extendcmd			= FunctonExtend;	
}


static void Dtos(char *file,uint8_t *data,uint16_t Len)
{
	FILE *fp = NULL;
	int i = 0;
	uint8_t wdata[2048];
	char DD[4];
	uint8_t Hbit = 0;
	uint8_t Lbit = 0;

	memset(wdata,0,sizeof(wdata));
	for(i = 0 ; i < Len; i++)
	{
		Hbit = ((data[i] & 0xf0) >> 4);
		Lbit = (data[i] & 0x0f);
		wdata[3*i + 0] = (Hbit > 9) ? (Hbit + 0x57) : (Hbit + 0x30);
		wdata[3*i + 1] = (Lbit > 9) ? (Lbit + 0x57) : (Lbit + 0x30);
		wdata[3*i + 2] = 0x20;
	}
	wdata[3 * Len + 0] = '\0';
	wdata[3 * Len + 1] = '\n';

	

	fp = fopen(file,"a");
	fwrite(wdata,1,3 * Len + 2,fp);
	fflush(fp);
	fclose(fp);
	debug_printf("Len = %d,wdata = %s\n",Len,wdata);
	
}


//下面的协议是显科的协议
int swr_protocolProcessor(user_t *user,uint8_t *input,uint32_t *inputlen)
{
	//Protocl_t protocol;
	unsigned short parity = 0;
	unsigned char ptcdata[256];
	unsigned int len;
	unsigned int nlength;
	int err = -1;

	unsigned int outlen = 0;

	int output_total_len = 0;

	debug_printf("This data from protocol_processor=============\n");
	#if 1
		int i = 0;
		for(i = 0 ; i < *inputlen ; i++)
			debug_printf("%02x ",input[i]);
		debug_printf("\n\n\n");
	#endif
	debug_printf("1inputlen = %d\n",*inputlen);
/*************************************************************************************/
//每个协议都可以加这条
	uint8_t vindicate[9] = {0x02,0x39,0x30,0x30,0x30,0x30,0x7E,0x18,0x03};
	uint8_t reply[9] = {0x02,0x39,0x30,0x30,0x30,0x01,0x58,0x6A,0x03};
	if(memcmp(input,vindicate,9)==0)
	{ 
		#if 0
		//这里user->type永远都是0
		debug_printf("user is type is %d\n",user->type);
		//回复上位机 
		if(user->type == 0) //网口
			send(user->fd,reply,9,0);
		else if(user->type == 1)
			uart_send(xCOM1,reply,9);
		#endif
		debug_printf("user is type is %d\n",ack_back_table);
		if(ack_back_table == TABLE_NET_PORT)
			send(user->fd,reply,9,0);
		else if(ack_back_table == TABLE_UART_PORT)
			uart_send(xCOM1,reply,9);

		
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
		log_write("Enter to Uprade reboot",strlen("Enter to Uprade reboot"));
		system("killall ledscreen");
		
	}	

/*******************************************************************************************/
	//反转义
	err = prtcl_preparsing(input,*inputlen,FREEdata,&outlen);
	if(err != 0)
	{
		debug_printf("find error when preparsing the recv data\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"prtcl_preparsing failed",strlen("prtcl_preparsing failed"));
		DEBUG_PRINTF;
		goto ERRORDEAL;
	}

	_log_file_write_("testlosg","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"prtcl_preparsing failed",strlen("prtcl_preparsing failed"));

	//校验

	DEBUG_PRINTF;
	err = ParityCheck_CRC16(FREEdata,outlen);
	DEBUG_PRINTF;
	debug_printf("err = %d\n",err);
	if(err != 0)
	{
		DEBUG_PRINTF;
		debug_printf("find error when parity the recv data\n");
		_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),"CRC failed",strlen("CRC failed"));
		DEBUG_PRINTF;
		goto ERRORDEAL;
	}
	DEBUG_PRINTF;
	//给协议信息增加用户信息
	PROTOCOLStruct.usermsg = user;
	//将输入的input的字节序转换成协议结构体protocol
	Byte_to_struct(&PROTOCOLStruct,FREEdata,outlen);
	
 	int ret = 0; 
	int k = 0;
	
	
	debug_printf("1*****************protocol msg: recv *****************\n");
	prtclmsg_printf(&PROTOCOLStruct);
	//下面的可忽略不计
	//if(IsInCMDLIST(PROTOCOLStruct.protcmsg.head.cmdID) == 0) 
	//{
	//	DEBUG_PRINTF;
	//	nlength = 0;
	//	DEBUG_PRINTF;
	//	goto ERRORDEAL;
	//}
	//以下都是命令的处理

	//设置IP命令。临时的
	uint16_t setipcmd = 0;
	setipcmd = PROTOCOLStruct.protcmsg.head.devID;
	if(setipcmd == 0x3338)
		PROTOCOLStruct.protcmsg.head.cmdID = setipcmd;

	char logmsg[12];
	sprintf(logmsg,"cmd : 0x%x",PROTOCOLStruct.protcmsg.head.cmdID);
	_log_file_write_("protolog","prtcl_protocl_parsing",strlen("prtcl_protocl_parsing"),logmsg,strlen(logmsg));
	switch(PROTOCOLStruct.protcmsg.head.cmdID)
	{
		case CMD_DEVICE_COM:
			//do nothing
			DEBUG_PRINTF;
			PROTOCOLStruct.GetCommunitStatus(&PROTOCOLStruct,&len);
			break;
		case CMD_DEVICE_TOTAL:
			PROTOCOLStruct.GetDevTotalStatus(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_SET_IPADRESS:
			DEBUG_PRINTF;
			PROTOCOLStruct.SetDevIP(&PROTOCOLStruct,&len);
			break;
		case CMD_DEVICE_DETAIL:
			PROTOCOLStruct.GetDevDetailStatus(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_DEVICE_POINT:
			DEBUG_PRINTF;
			PROTOCOLStruct.GetDevPixStatus(&PROTOCOLStruct,&len);//ok
			DEBUG_PRINTF;
			break;
		case CMD_DEVICE_BRIGHTMODE:
			PROTOCOLStruct.GetDevBright(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_INFO_PLAYLIST:
			DEBUG_PRINTF;
			PROTOCOLStruct.GetCurPlayLst(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_INFO_DISPLAY:
			DEBUG_PRINTF;
			PROTOCOLStruct.SetCurPlayLst(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_UTIL_GETTIME:
			PROTOCOLStruct.GetCurSysTime(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_UTIL_LASTTIME:
			PROTOCOLStruct.GetSysStartTime(&PROTOCOLStruct,&len);
			break;
			
		//设置超载报警，部分项目用到,目前四川雅康高速用到
		case CMD_SET_WARMING:
			PROTOCOLStruct.SetWarming(&PROTOCOLStruct,&len);
			break;
			

	#if 1
		//一下三项内容在协议中没有，暂时忽略
		case CMD_UTIL_VERCHECK:
			get_util_vercheck(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_UTIL_READID:
			get_devID(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_UTIL_SETID:
			set_devID(&PROTOCOLStruct,&len);//ok
			break;
	#endif

		case CMD_GET_SMOK:
			get_smok(&PROTOCOLStruct,&len);
			break;
	
		case CMD_UTIL_SETTIME:
			PROTOCOLStruct.SetSysCurTime(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_INFO_STRING:
			PROTOCOLStruct.GetCurPlayString(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_DEVICE_SETBRIGHT:
			PROTOCOLStruct.SetDevBright(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_DEVICE_OPENCLOSE:
			PROTOCOLStruct.SetScreenStatus(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_DEVICE_POWERMODE:
			PROTOCOLStruct.SetDevPowerMode(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_DEVICE_RESET:
			PROTOCOLStruct.SetSysReset(&PROTOCOLStruct,&len);
			break;	
		case CMD_DEVICE_DRIVERS:
			DEBUG_PRINTF;
			PROTOCOLStruct.GetDevDrvStatus(&PROTOCOLStruct,&len);//ok
			break;
		case CMD_SET_TEST_MODE:
			PROTOCOLStruct.SetDevTestMode(&PROTOCOLStruct,&len);//ok
			break;
			
		/*文件传输*/
		//从上位机下载文件
		case CMD_FILE_DOWNLOAD:
			DEBUG_PRINTF;
			PROTOCOLStruct.FileFrameRX2K(&PROTOCOLStruct,&len);//ok
			break;
		//将文件上传给上位机
		case CMD_FILE_UPLOAD:
			DEBUG_PRINTF;
			PROTOCOLStruct.FileFrameTX2K(&PROTOCOLStruct,&len);//ok
			DEBUG_PRINTF;
			break;
		//针对大文件，从上位机下载文件
		case CMD_BIGFILE_DOWNLOAD:
			//DEBUG_PRINTF;
			//debug_printf(".......................\n");
			PROTOCOLStruct.FileFrameRX16K(&PROTOCOLStruct,&len);
			break;
		//针对大文件，上传文件到上位机
		case CMD_BIGFILE_UPLOAD:
			//DEBUG_PRINTF;
			PROTOCOLStruct.FileFrameTX16K(&PROTOCOLStruct,&len);
			break;

		//复位接收卡
		case CMD_RST_RX_CARD:
			PROTOCOLStruct.SetRstRxCard(&PROTOCOLStruct,&len);
			break;
		//复位发送卡
		case CMD_RST_TX_CARD:
			PROTOCOLStruct.SetRstTxCard(&PROTOCOLStruct,&len);
			break;

		//打开或者关闭模组电源
		case CMD_SET_MODULE_POWER:
			PROTOCOLStruct.SetModulePower(&PROTOCOLStruct,&len);
			break;

		case CMD_GET_VERSION:
			PROTOCOLStruct.GetVersion(&PROTOCOLStruct,&len);
			break;
		//设置像素点检测模式
		case CMD_SET_PIXMODE:
			PROTOCOLStruct.SetPixMode(&PROTOCOLStruct,&len);
			break;
		//获取显示参数
		case CMD_GET_DISPLAY_PARAMETER:
			PROTOCOLStruct.GetDisplayParameter(&PROTOCOLStruct,&len);
			break;
		//非标准协议扩展接口
		default:
			PROTOCOLStruct.extendcmd(&PROTOCOLStruct,&len);
			break;
			
	}


	struct_to_bytes(&PROTOCOLStruct,FREEdata);
	//校验值
	parity = XKCalculateCRC(FREEdata+1,4+PROTOCOLStruct.protcmsg.length);
	PROTOCOLStruct.protcmsg.parity = parity;
	debug_printf("PROTOCOLStruct.protcmsg.parity = 0x%x\n",PROTOCOLStruct.protcmsg.parity);
	FREEdata[PARITY_BYTE_POS(PROTOCOLStruct.protcmsg.length) + 0] = (unsigned char)(PROTOCOLStruct.protcmsg.parity >> 8);
	FREEdata[PARITY_BYTE_POS(PROTOCOLStruct.protcmsg.length) + 1] = (unsigned char)(PROTOCOLStruct.protcmsg.parity);
	//检测字节序中是否存在0x02、0x03、0x1B，有则进行相应的转换，此处处理办法是将头尾两个字节踢掉在处理
	DEBUG_PRINTF;
	check_0x02and0x03(FLAG_SEND,FREEdata+1,4+PROTOCOLStruct.protcmsg.length+2,input+1,inputlen);
	DEBUG_PRINTF;
	
	output_total_len = *inputlen;
	//输出字节序再加上头尾两个字节
	input[0] 				= 0x02;
	input[output_total_len + 1] 	= 0x03;
	//所以总长度要+2
	output_total_len += 2;
	*inputlen = output_total_len;
	debug_printf("1*****************protocol msg: send *****************\n");
	//prtclmsg_printf(&PROTOCOLStruct);
	PROTOCOLStruct.protcmsg.data = NULL;
	
	DEBUG_PRINTF;
	return 0;
	ERRORDEAL:
		DEBUG_PRINTF;
		debug_printf("memory has been free!\n");
		return -1;
}

