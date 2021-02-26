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
���̹߳�����ϵͳ�����Զ�ping���أ���֮��ÿ��1��Сʱ��pingһ������
ԭ��:�в��ֿͻ��������Ǿ�����Ӫ��(�ƶ�����ͨ������)�ģ�����Ӫ�̳���ĳ��ԭ������
���ص�arp���ܣ�����Ҫ���豸(����)���������������й�һ��ͨ�ţ������ؼ�¼���豸(����)
����ص�ip��Ϣ���������ؾͻ�����ļ������Ƿ����ߣ��������������ͨ�Ų����������Զ
�޷�������ͨ�š�������ĳЩԭ����ϵ�����������������������������ص�ͨ�ţ���ô����
�н�û�иô�������Ϣ����ô���ķ�����������Ϣ����Ȼ�������أ��������Ҳ�����������������
������޷�ͨ�š�
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
		//change by mo 20201103 ping ��ͨ��������Ϊping��ͨ10������(10��Сʱ)
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
