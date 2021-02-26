#include "ping_task.h"
#include "../update.h"
#include "conf.h" 



#define pingfile 		conf_dir"/ping"
#define pinggateway		conf_dir"/ping.txt"

static char pingstr[24];
#define PING_INTERVAL		60
#define PING_TIMER_ID		12
#define INTERVALTIME 		3600

static int times = 10;
static char gateway[24];


static int PingGateWay(const char *gateWayIp)
{
	int i=0;
	int ret =-1;
	char command[128]={0};

	if(!access(pinggateway,F_OK))
		remove(pinggateway);
	
	snprintf(command,128,"ping %s -c 10 > %s 2>&1",gateWayIp,pinggateway);
	//printf("command = %s\n",command);
	system(command);
	FILE *fp = fopen(pinggateway,"r");
	if(fp==NULL)
		return -1;
	
	fseek(fp,0,SEEK_END);
	int len =  ftell(fp);	
	fseek(fp,0,SEEK_SET);
	char *buffer = (char *)calloc(1,len+1);
	if(buffer==NULL)
	{
		fclose(fp);
		return -1;
	}
	//DEBUG_PRINTF;
	fread(buffer,1,len,fp);
	fclose(fp);
	//remove("ping.txt");
	
	//printf("buffer = %s\n",buffer);
	
	if(strstr(buffer,"bytes from"))
	{
		//printf("\ndconnect gateway\n");
		//printf("%s\n",buffer);
		free(buffer);
		buffer = NULL;
		return 0;
	}	
	free(buffer);
	buffer = NULL;
	//printf("disconnect gateway\n");
	//printf("exit\n");
	return -1;
}



static void Ping10timesPowerUp(void)
{
	while(times > 0)
	{
		if(PingGateWay(gateway) == 0)
		{
			times = 10;
			break;
		}
		times--;
		sleep(1);
		//printf("hello\n");
	}
	
	if(times == 0)
	{
		DEBUG_PRINTF;
		DEBUG_PRINTF_ToFile(__func__,__LINE__);
		//SET_LED_STATE(SLED_OFF);
	}
}






/*
此线程功能是系统启动自动ping网关，且之后每隔1个小时就ping一次网关
原因:有部分客户的网络是经过运营商(移动、联通、电信)的，而运营商出于某种原因开启了
网关的arp功能，网关要求设备(大屏)首先主动与网关有过一次通信，让网关记录该设备(大屏)
的相关的ip信息，这样网关就会持续的检测大屏是否在线，大屏在线则可以通信不在则大屏永远
无法与网关通信。当由于某些原因如断电重启后，如果大屏不主动发起与网关的通信，那么网关
中将没有该大屏的信息，那么中心发给大屏的信息，虽然经过网关，但网关找不到大屏，导致中心
与大屏无法通信。
*/
void *ping_task(void *arg)
{ 
	int ret = -1;
	int num = 0;
	int readsize = 0;
	char wpingnum[24];
	char rpingnum[24];
	char *strnum = NULL;
	FILE *fp = NULL;
	
	memset(gateway,0,sizeof(gateway));
	conf_file_read(ConFigFile,"netport","gateway",gateway);
	Ping10timesPowerUp();

	while(1)
	{
		sleep(INTERVALTIME);
		//TCP_AllUserDel();
		ret = PingGateWay(gateway);

		fp = fopen(pingfile,"r+");
		if(fp == NULL)
			fp = fopen(pingfile,"w+");

		if(fp == NULL)
			continue;

		memset(wpingnum,0,sizeof(wpingnum));
		memset(rpingnum,0,sizeof(rpingnum));
		readsize = fread(rpingnum,1,sizeof(rpingnum),fp);
		if(readsize <= 0)
		{
			sprintf(rpingnum,"ping = 9");
			fseek(fp, 0, SEEK_SET );  
			fwrite(rpingnum,1,strlen(rpingnum),fp);
			fflush(fp);
		}
		
		strnum = strstr(rpingnum,"=");
		if(strnum == NULL)
		{
			fclose(fp);
			continue;
		}

		
		num = atoi(strnum + 2);
		if(ret == 0)
		{
			sprintf(wpingnum,"ping = 9");
			fseek(fp, 0, SEEK_SET );  
			fwrite(wpingnum,1,strlen(wpingnum),fp);
			fflush(fp);
		}	
		else
		{
		//change by mo 20201103 ping 不通重启，改为ping不通10次重启(10个小时)
		#if 0
			if(num > 0)
			{
				sprintf(wpingnum,"ping = %d",num - 1);
				fseek(fp, 0, SEEK_SET );  
				fwrite(wpingnum,1,strlen(wpingnum),fp);
				fflush(fp);
				fclose(fp);
				DEBUG_PRINTF;
				DEBUG_PRINTF_ToFile(__func__,__LINE__);
				SET_LED_STATE(SLED_OFF);
				log_write("Ping GW fail reboot",strlen("Ping GW fail reboot"));
				system("reboot");
				continue;
			}
		#endif
			if(num > 0)
			{
				sprintf(wpingnum,"ping = %d",num - 1);
				fseek(fp, 0, SEEK_SET );  
				fwrite(wpingnum,1,strlen(wpingnum),fp);
				fflush(fp);
				//fclose(fp);
				DEBUG_PRINTF;
				continue;

			}
			else
			{
				sprintf(wpingnum,"ping = 9");
				fseek(fp, 0, SEEK_SET );  
				fwrite(wpingnum,1,strlen(wpingnum),fp);
				fflush(fp);
				fclose(fp);
				DEBUG_PRINTF_ToFile(__func__,__LINE__);
				SET_LED_STATE(SLED_OFF);
				log_write("Ping GW fail reboot",strlen("Ping GW fail reboot"));
				system("reboot");				
			}
		
		}
		fclose(fp);
	}
}
