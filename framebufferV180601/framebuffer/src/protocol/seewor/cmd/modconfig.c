#include <stdio.h>
#include <string.h>
#include "../../../include/config.h"
#include "../../../include/conf.h"

#define PORT  5168
#define CLSCONF			"/home/LEDscr/config/cls.conf"
#define CPY_CLSCONF		"/home/LEDscr/config/cpy_cls.conf"
#define	_CLSCONF		"/home/LEDscr/config/_cls.conf"

int main(void)
{
	//1�������������ֲ��䣬�޸Ķ˿ں�Ϊ5168������Ĭ�ϲ����б�Ϊ001.lst������ģʽ�Զ�����
	char KeyVal[16];
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(CLSCONF,"netport","port",KeyVal);
	if(strncmp(KeyVal,"5168",4) != 0)
		conf_file_write(CLSCONF,"netport","port","5168");
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(CLSCONF,"playlist","list",KeyVal);
	if(strncmp(KeyVal,"000.xkl",7) != 0)
		conf_file_write(CLSCONF,"playlist","list","000.xkl");

	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(CLSCONF,"brightmode","mode",KeyVal);
	if(strncmp(KeyVal,"31",2) != 0)
		conf_file_write(CLSCONF,"brightmode","mode","31");

	//2�����޸ĺ�������ļ�����һ��
	system("cp /home/LEDscr/config/cls.conf /home/LEDscr/config/cpy_cls.conf");

	//3������һ���ָ�ip�������ļ�
	system("cp /home/LEDscr/config/cls.conf /home/LEDscr/config/_cls.conf");
	conf_file_write(_CLSCONF,"netport","ip","192.168.1.11");
	conf_file_write(_CLSCONF,"netport","netmask","255.255.255.0");
	conf_file_write(_CLSCONF,"netport","gateway","192.168.1.1");
	conf_file_write(_CLSCONF,"netport","port","5168");
}

