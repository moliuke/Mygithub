#include <stdio.h>
#include "../../../include/config.h"
#include "../../../include/conf.h"
#include "../../../include/debug.h"


#define CLS_CONF		"/home/LEDscr/config/cls.conf"
#define CPY_CLS_CONF	"/home/LEDscr/config/cpy_cls.conf"


int main(void)
{
	//����һ���ָ�IP�õ�_cls.conf�Լ�һ������ʹ�õı���cpy_cls.conf,��ֹ
	//cls.conf�������޸Ļ�����ʱ�ָ�
	if(access(config_sh,F_OK) >= 0)
		system(config_sh);
	debug_printf("config_sh = %s\n",config_sh);

	//�޸Ļָ�IP��cls.conf��ip�����ء�����ΪĬ��ֵ
	conf_file_write(_cls,"netport","ip","192.168.1.11");
	conf_file_write(_cls,"netport","netmask","255.255.255.0");
	conf_file_write(_cls,"netport","gateway","192.168.1.1");
	conf_file_write(_cls,"netport","port","5168");
	//conf_file_write(f_cls,"netport","ip","192.168.1.11");
	//conf_file_write(f_cls,"netport","netmask","255.255.255.0");
	//conf_file_write(f_cls,"netport","gateway","192.168.1.1");
	//conf_file_write(f_cls,"netport","port","5168");


	//���ܵ���λ�����͵������ļ���Ҫ��ip�����롢����д��һ���ű��ļ�ipconfig.sh�У���ϵͳ
	//������ͻ��Զ���������ļ�������IP��Ϣ
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
