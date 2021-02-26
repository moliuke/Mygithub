#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/shm.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/io.h>
#include "debug.h"
#include "dev/wdt.h"
#include "conf.h"
#include "dev/Dev_serial.h"
#include "user/cpy.h"
#include "user/def_list.h"
#include "mtime.h"
#include "appOnline.h"
#include "dev/gpio.h"

#define MAJOR		1	//主版本号
#define MINOR		1	//次版本号
#define REVISION	8	//修订版本号


#define sys_dir 						"/home/LEDscr"
#define conf_dir 						sys_dir"/config"
#define ConFigFile						conf_dir"/cls.conf"
#define syscrashlock					sys_dir"/crash.lock"


#define TTY_PATH            "/dev/tty"
#define STTY_US             "stty raw -echo -F "
#define STTY_DEF            "stty -raw echo -F "


#define sys_dir 			"/home/LEDscr"
#define conf_dir 			sys_dir"/config"
#define image_dir			sys_dir"/image"
#define font_dir			sys_dir"/FONT"
#define list_dir			sys_dir
#define list_dir_1			sys_dir"/list"
#define setting_dir			sys_dir"/setting"
#define crash_dir			sys_dir"/crash"

#define list_factory_set 	setting_dir"/list"
#define image_factory_set	setting_dir"/image"
#define conf_factory_set	setting_dir"/config"



#define defIP				"/home/LEDscr/config/defIP.sh"
#define confIP				"/home/LEDscr/ipconfig.sh"
#define cpy_rcs				"/etc/init.d/cpy_rcS"
#define rcs					"/etc/init.d/rcS"
#define cpy_cls				conf_dir"/cpy_cls.conf"
#define cls					conf_dir"/cls.conf"
#define _cls				conf_dir"/_cls.conf"
#define factory				sys_dir"/factory"

#define exefile_dir			"/bin"
//#define exefile_dir			"/mnt/linuxVersion/"

#define exefile				"ledscreen"

#define cpy_config_file 	sys_dir

#define FILE_SIZE_MAX_1M		(1024 * 512)


#define BOOTUPWAITIME	16

//static int conect_times = 0;

int flag = 0;
int sig = 0;
int sec = BOOTUPWAITIME; 
char option;


//设置屏幕开关，这里设置成打开屏幕
//static uint8_t closeLED[8] = {0xD8,0x00,0x01,0x00,0x00,0x00,0x00,0xAA};
//复位发送卡、接收卡，复位第一张接收卡会引起后面所有的接收卡都复位，第6字节表示复位发送卡
//static uint8_t RestRX[8] = {0xD8,0x00,0x00,0x01,0x00,0x00,0x00,0xAA};
//static uint8_t RestTX[8] = {0xD8,0x00,0x00,0x00,0x00,0x01,0x00,0xAA};
static char *shm;
static int shmid;


static int Cannot_conect_times = 0;

#if 1
void bootversion(void)
{
	FILE *fp = NULL;
	
	char version[64];
	memset(version,0,sizeof(version));
	sprintf(version,"bootupV%d.%d.%d\n",MAJOR,MINOR,REVISION);

	if(access("/home/LEDscr/version/boot.v",F_OK) < 0)
		return;

	fp = fopen("/home/LEDscr/version/boot.v","w+");
	if(fp < 0)
	{
	 	return;
	}

	char Rversion[64];
	memset(Rversion,0,sizeof(Rversion));
	fread(Rversion,1,sizeof(Rversion),fp);

	if(strncmp(Rversion,version,strlen(version)) == 0)
	{
		fclose(fp);
		return;
	}
	fwrite(version,1,strlen(version),fp);
	fflush(fp);
	fclose(fp);
}
#endif


static int memshareCreat(void)
{
	key_t key;
	if((key = ftok(".", 'z')) < 0)
	{ 
		perror("ftok error");
		return -1;
	}

	// 获取共享内存
	if(((shmid = shmget(key, 16, IPC_CREAT|0666))) == -1)
	{
		perror("shmget error");
		return -1;
	}

	// 连接共享内存
	shm = (char*)shmat(shmid, 0, 0);
	if((int)shm == -1)
	{
		perror("msgget error");
		return -1;
	}
	//debug_printf("shmid = %d\n",shmid);
	memset(shm,0,16);
	memcpy(shm,"initwdt",7);
	shmdt(shm);
	return 0;
}


static int memshareRead(void)
{
	struct shmid_ds buf;
	char wdtstate[16];
	key_t key;
	if((key = ftok(".", 'z')) < 0)
	{ 
		perror("ftok error");
		return -1;
	}



	uint32_t timeout = 15 * 1000 * 1000;
	uint32_t sleeptime = 0;
	while(timeout)
	{
		sleeptime = (timeout > 500000) ? 500000 : timeout;
		usleep(sleeptime);
		timeout -= sleeptime;

		// 获取共享内存
		if((shmid = shmget(key, 16, 0)) == -1)
		{
			perror("shmget error");
			continue;
		}

		//debug_printf("shmid = %d\n",shmid);

		DEBUG_PRINTF;
		// 连接共享内存
		shm = (char*)shmat(shmid, 0, 0);
		if((int)shm == -1)
		{
			perror("msgget error");
			return -1;
		}
		//debug_printf("shm = %s\n",shm);
		if(strncmp(shm,"initwdt",7) == 0)
		{
			wdt_stop();
			shmdt(shm);
			shmctl(shmid, IPC_RMID, &buf);
			printf("stop the wdt ok!\n");
			return 0;
		}
	}
	
	return 0;
}


static int get_char(char *ch);

void kill_app(void)
{
	system("pkill -f ledscreen");
}


void log_write(char *logtext,int len)
{
	int ret = 0;
	int fd = 0;
	char writeitem[128];
	char systime[24];
	uint8_t systimelen;
	
	memset(writeitem,0,sizeof(writeitem));
	get_sys_time(systime,&systimelen);
	sprintf(writeitem,"%s time:%s\n\n",logtext,systime);
	fd = open("/home/LEDscr/sys/daemon.log",O_WRONLY | O_CREAT,0744);
	if(fd < 0)
	{
		perror("open:");
	}
	lseek(fd, 0, SEEK_END);
	ret = write(fd,writeitem,strlen(writeitem));
}




static int get_char(char *ch)
{
    fd_set rfds;
    struct timeval tv;
	int ret = -1;

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    tv.tv_sec = BOOTUPWAITIME;
    tv.tv_usec = 0;

    ret = select(1, &rfds, NULL, NULL, &tv);
	if(ret > 0)
    {
        *ch = getchar(); 
		return 0;
    }
	if(ret == 0)
	{
		return -1;
	}

    return -1;
}




void *help_message(void *arg)
{
	FILE *fp = NULL;
	char Vapp[64],Vboot[24],Vsys[24],ipmsg[256];
	char *strcur = NULL,*strnext = NULL;
	char ip[24],netmask[24],gateway[24],port[8];
	DEBUG_PRINTF;
	memset(Vapp,0,sizeof(Vapp));
	if(!access("/home/LEDscr/version/app.v",F_OK))
	{
		fp = fopen("/home/LEDscr/version/app.v","r");
		fread(Vapp,1,1024,fp);
		fclose(fp);
	}

	memset(Vboot,0,sizeof(Vboot));
	if(!access("/home/LEDscr/version/boot.v",F_OK))
	{
		fp = fopen("/home/LEDscr/version/boot.v","r");
		fread(Vboot,1,1024,fp);
		fclose(fp);
	}

	memset(Vsys,0,sizeof(Vsys));
	if(!access("/home/LEDscr/version/sys.v",F_OK))
	{
		fp = fopen("/home/LEDscr/version/sys.v","r");
		fread(Vsys,1,1024,fp);
		fclose(fp);
	}
	memset(ipmsg,0,sizeof(ipmsg));
	if(!access(cls,F_OK))
	{
		memset(ip,0,sizeof(ip));
		memset(netmask,0,sizeof(netmask));
		memset(gateway,0,sizeof(gateway));
		memset(port,0,sizeof(port));
		
		conf_file_read(cls,"netport","ip",ip);
		conf_file_read(cls,"netport","netmask",netmask);
		conf_file_read(cls,"netport","gateway",gateway);
		conf_file_read(cls,"netport","port",port);
		
	}
	
	DEBUG_PRINTF;
	
	while(1)
	{
		printf(
		"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
		"Version : \n"
		"app : %s"
		"boot : %s"
		"sys : %s",Vapp,Vboot,Vsys);
		if(ip != NULL && netmask != NULL && gateway != NULL)
			printf("ip:%s  netmask:%s  gateway:%s  port:%s\n",ip,netmask,gateway,port);
		printf(
		"<1>bootup application  <2>restore system  <3>enter backstage\n<4>not bootup  <5>restore IP\n"
		"\n16 secs later will boot up the application!\n");
		while(1)
		{
			sec--;
			if(sec < 0)
				sec = 0;
			sleep(1);
			
			if(flag)
			{
				debug_printf("your choice is : %c \n",option);
				sig = 1;
				flag = 0;
				pthread_exit(0);
			}
		}
	}
}

int file_copy(char *destpath,char *srcpath)
{
	char *destination_path = destpath;
	char *source_path = srcpath;

	if(access(source_path,F_OK) < 0)
	{
		debug_printf("Sorry,the backup system is not found curently!\n");
		return -1;
	}

	if(access(destination_path,F_OK) > 0)
		remove(destination_path);

	mkdir(destination_path,0777);
	
	copy_folder(source_path,destination_path);//进行文件夹的拷贝

	return 0;
}



static int wdt_flag = 0;
void enter_Backstage(void)
{
	while(!wdt_flag)
	{
		if(access("/home/LEDscr/sys/wdtinit.log",F_OK) < 0)
		{
			usleep(1000);
			debug_printf("wdtinit.log not exit\n");
			continue;
		}
		
		debug_printf("The wdt has been stop!!\n");
		wdt_flag = 1;
	}
	debug_printf("wdtinit.log remove\n");
	wdt_stop();
	remove("/home/LEDscr/sys/wdtinit.log");
}

void IP_restore(void)
{
	char resIP[64];
	char resconfile[64];
	memset(resconfile,0,sizeof(resconfile));
	memset(resIP,0,sizeof(resIP));
	sprintf(resIP,"cp %s %s",defIP,confIP);
	sprintf(resconfile,"cp %s %s",_cls,cls);
	system(resIP);
	system(resconfile);
	system("reboot");
}

void bootup_application(void)
{
	char cmd_exe[96];
	memset(cmd_exe,0,sizeof(cmd_exe));
	debug_printf("now rebooting the app!\n");
	sprintf(cmd_exe,"reset && %s/%s &",exefile_dir,exefile);
	//debug_printf("cmd_exe = %s\n",cmd_exe);
	system(cmd_exe);
}


void restore_factory_setting(void)
{
	//复制出厂配置文件
	system("./tofactory.sh && reboot");
}


static void __restore_system(void)
{
	chdir(sys_dir);
	debug_printf("		Compressing the bad system,please wait a moment...\n");
	system("tar -czvf crash/crash.tar.gz image/ list/ config/ log/ ledscreen bootup");
	debug_printf("		Compressing the bad system ok!\n");

	debug_printf("		restoring the system...\n");
	restore_factory_setting();
}

void restore_system(void)
{
	//杀掉应用程序
	system("pkill -f ledscreen");
	//恢复系统
	__restore_system();
	log_write("USR_RST_reboot",strlen("USR_RST_reboot"));
	system("reboot");
}


static int chek_time(void)
{
	int fileSize = 0;
	time_t timep;
	struct tm *p;
	struct stat log_buf;
	time(&timep);
	p=localtime(&timep); /*取得当地时间*/
	
	
	if(p->tm_hour == 2 && p->tm_min == 30 && p->tm_sec < 10)
	{ 
		lstat("/home/LEDscr/sys/daemon.log", &log_buf);
		fileSize = log_buf.st_size;
		if(fileSize > FILE_SIZE_MAX_1M)
			remove("/home/LEDscr/sys/daemon.log");
		return 0;
	}

	return -1;
}

static int time_to_resetsystem(void)
{
	return chek_time();
}

void bootup_option(char option)
{
	switch (option)
	{
		case '1':
			//debug_printf("now booting up the application,please wait a moment...\n");
			bootup_application();
			break;
		case '2':
			//debug_printf("		restoring factory settings,please wait a moment...\n");
			restore_system();
			//debug_printf("		restoring factory settings ok.\n");
			//debug_printf("		booting the application...\n");
			sleep(2);
			bootup_application();
			break;
		case '3':
			//debug_printf("enter backstage\n");
			//enter_Backstage();
			memshareRead();
			DEBUG_PRINTF;
			exit(1);
			break;
		case '4':
			//debug_printf("		do not boot up the application\n");
			break;
		case '5':
			IP_restore();
			break;
    } 
}


int wait_appNetOK(void)
{
	int err = -1;
	int wait_3s = 10;
	int flag = -1;
	while(wait_3s--)
	{
		if(access("/home/LEDscr/net.lock",F_OK) < 0)
		{
			debug_printf("net pthread has not setup\n");
			usleep(500*1000);
			continue;
		}
		else
		{
			debug_printf("file exist\n");
			flag = 1;
			break;
		}
	}

	if(flag < 0)
		return -1;
	err = remove("/home/LEDscr/net.lock");
	if(err < 0)
	{
		perror("remove");
	}
	return 0;
}

int check_SYSrebootLock(void)
{
	if(access("/home/LEDscr/sys/sys_reboot.lock",F_OK) < 0)
		return -1;
	else
	{
		remove("/home/LEDscr/sys/sys_reboot.lock");
		return 0;
	}
}


int get_pid_name(char *app_name)
{
	return strncmp(app_name,"ledscreen",strlen("ledscreen"));
}

void NetLockRemove(void)
{
	if(access("/home/LEDscr/net.lock",F_OK))
	{
		remove("/home/LEDscr/net.lock");
	}
	
	if(access("/home/LEDscr/sys/sys_reboot.lock",F_OK))
	{
		remove("/home/LEDscr/sys/sys_reboot.lock");
	}

	if(access("/home/LEDscr/sys/wdtinit.log",F_OK))
	{
		remove("/home/LEDscr/sys/wdtinit.log");
	}
}

int main()
{
	int appexitTimes = 0;
    char ch = 0;
	int ret = -1;
	pid_t child_pid ; 
	int retval;
	int status;
	int fd;
	int err = -1;
	uint32_t Timedstant = 0;
	pthread_t tid_flash_scr;
	bootversion();
	NetLockRemove();
	//serial_param_set(xCOM2,57600,8,1,'N','N');
	//uart_init(xCOM2);//与下位机通信
	debug_printf("excuting bootup application....\n");
	if(check_APPonline(get_pid_name) == 0)
	{
		printf("[ %s ] is running ,exiting bootup application!\n",exefile);
		//发现APP已经在运行，不应该杀掉!!!
		//kill_app();
		exit(0);
	}

	
    system("stty -icanon");
	ret = pthread_create(&tid_flash_scr,NULL,help_message,NULL);
	if(ret != 0)
	{
		perror("pthread_create fail"); 
		//APP还没启动呢，不能杀掉
		//kill_app();
		//exit(1);
	}
    ret = get_char(&ch);
	option = (ret < 0) ? '1' : ch;
	flag = 1;
	while(!sig);

	bootup_option(option);
	
	pthread_join(tid_flash_scr,NULL); 
	
#if 0
	if ((child_pid = fork()) < 0)
	{
		printf("fork error\n");
		//创建子进程失败，先杀掉APP，再推出，此程序推出会引起重启
		//kill_app();
		//exit(1);
	}

	else if (child_pid != 0)
	{
		//父进程退出，不能杀掉APP
		//kill_app();
		exit(0);
	}
#endif
	GPIO_pin_init();
	wdt_init(7000);
	memshareCreat();
	log_write("SYS_reboot enter",strlen("SYS_reboot enter"));
	struct timeval startTime, endTime;
	gettimeofday (&startTime, NULL);
	while(1)
	{
		//DEBUG_PRINTF;
		sleep(3);
		//喂狗，如果 此进程意外退出将会重启系统
		wdt_feed(WDT_FEED);
		//检测是否凌晨2:30,是则重启系统
		if(!time_to_resetsystem())
		{
			log_write("TIM_SYS_reboot",strlen("TIM_SYS_reboot"));
			goto REBOOT_SYSTEM;
		}
		//检测用户是否从上位机重启
		if(check_SYSrebootLock() == 0)
		{
			//休眠1秒钟的目的是等待APP将信息发给上位机，在重启
			sleep(1);
			log_write("USR_SYS_reboot",strlen("USR_SYS_reboot"));
			goto REBOOT_SYSTEM;
		}
		//检测app是否在线，app如果意外退出将会被重启,在120秒钟内如果连续意外退出次数超过20次则重启系统
		//并设置默认显示播放列表
		if(check_APPonline(get_pid_name) < 0)
		{
			appexitTimes++;
			//printf("appexitTimes = %d\n",appexitTimes);
			if(appexitTimes > 20)
			{
				gettimeofday (&endTime, NULL);
				Timedstant = (endTime.tv_sec - startTime.tv_sec) * 1000000 + (endTime.tv_usec - startTime.tv_usec);
				printf("Timedstant = %d\n",Timedstant);
				//3分钟
				if(Timedstant < 360000000)
				{
					//Set_deflist();
					open(syscrashlock,O_RDWR | O_CREAT,0744);
					//conf_file_write(ConFigFile,"playlist","list","Def.lst");
					log_write("ACD_SYS_reboot",strlen("ACD_SYS_reboot"));
					goto REBOOT_SYSTEM;
				}
			} 
			//net_CloseSocket();
			log_write("APP_EXT_reboot",strlen("APP_EXT_reboot"));
			goto REBOOT_APPLICATION;
		}
		else
		{
			//debug_printf("app is online\n");
			appexitTimes = 0;
			gettimeofday (&startTime, NULL);
		}
		//检测是否有人通过短接GPIO引脚来恢复系统
		if(check_manMadeRestore() == 0)
		{
			wdt_stop();
			restore_system();
			sleep(1);
			goto REBOOT_SYSTEM;
		}

		//检测是否硬件恢复IP
		if(check_IPRestore() == 0)
		{
			IP_restore();
			goto REBOOT_SYSTEM;
		}

		continue;

		
	//重启系统
	REBOOT_SYSTEM:
		printf("CLOSE LED & rebooting the system.....\n");
		//uart_send(xCOM2,closeLED,8);
		//sleep(1);
		//uart_send(xCOM2,closeLED,8);
		//sleep(1);
		system("reboot");
		return 0;

	//重启app，等待4秒钟目的等待app将网络初始化好
	REBOOT_APPLICATION:
		//uart_send(xCOM2,closeLED,8);
		//debug_printf("CLOSE LED\n");
		bootup_application();
		
	}
}




