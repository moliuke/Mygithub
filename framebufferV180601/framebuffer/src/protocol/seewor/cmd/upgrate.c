#include <stdio.h>
#include "../../../include/config.h"
#include "../../../include/conf.h"
#include "../../../include/debug.h"


#define CLS_CONF		"/home/LEDscr/config/cls.conf"
#define CPY_CLS_CONF	"/home/LEDscr/config/cpy_cls.conf"


int main(void)
{
	//备份一个恢复IP用的_cls.conf以及一个正常使用的备份cpy_cls.conf,防止
	//cls.conf被意外修改或者损坏时恢复
	if(access(config_sh,F_OK) >= 0)
		system(config_sh);
	debug_printf("config_sh = %s\n",config_sh);

	//修改恢复IP的cls.conf的ip、网关、掩码为默认值
	conf_file_write(_cls,"netport","ip","192.168.1.11");
	conf_file_write(_cls,"netport","netmask","255.255.255.0");
	conf_file_write(_cls,"netport","gateway","192.168.1.1");
	conf_file_write(_cls,"netport","port","5168");
	//conf_file_write(f_cls,"netport","ip","192.168.1.11");
	//conf_file_write(f_cls,"netport","netmask","255.255.255.0");
	//conf_file_write(f_cls,"netport","gateway","192.168.1.1");
	//conf_file_write(f_cls,"netport","port","5168");


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
		exit(1);
		return -1;
	}
	
	debug_printf("Wcontent = %s\n",Wcontent);
	
	lseek(fd,0,SEEK_SET);
	write(fd,Wcontent,strlen(Wcontent));
	close(fd);
	//system(IPCONFIG);
	
	return 0;
}
