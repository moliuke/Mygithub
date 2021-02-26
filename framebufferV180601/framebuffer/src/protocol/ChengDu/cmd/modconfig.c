#include <stdio.h>
#include <string.h>
#include "../../../include/config.h"
#include "../../../include/conf.h"

#define PORT  2929
#define CLSCONF			"/home/LEDscr/config/cls.conf"
#define CPY_CLSCONF		"/home/LEDscr/config/cpy_cls.conf"
#define	_CLSCONF		"/home/LEDscr/config/_cls.conf"

int main(void)
{
	//1、其他参数保持不变，修改端口号为2929，启动默认播放列表为001.lst，亮度模式自动亮度
	char KeyVal[16];
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(CLSCONF,"netport","port",KeyVal);
	if(strncmp(KeyVal,"2929",4) != 0)
		conf_file_write(CLSCONF,"netport","port","2929");
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(CLSCONF,"playlist","list",KeyVal);
	if(strncmp(KeyVal,"001.lst",7) != 0)
		conf_file_write(CLSCONF,"playlist","list","001.lst");

	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(CLSCONF,"brightmode","mode",KeyVal);
	if(strncmp(KeyVal,"30",2) != 0)
		conf_file_write(CLSCONF,"brightmode","mode","30");

	//2、将修改后的配置文件备份一个
	system("cp /home/LEDscr/config/cls.conf /home/LEDscr/config/cpy_cls.conf");

	//3、制作一个恢复ip的配置文件
	system("cp /home/LEDscr/config/cls.conf /home/LEDscr/config/_cls.conf");
	conf_file_write(_CLSCONF,"netport","ip","192.168.1.11");
	conf_file_write(_CLSCONF,"netport","netmask","255.255.255.0");
	conf_file_write(_CLSCONF,"netport","gateway","192.168.1.1");
	conf_file_write(_CLSCONF,"netport","port","5168");

	//4、将屏幕大小对应的一些默认的png图片拷贝到png目录下
	memset(KeyVal,0,sizeof(KeyVal));
	conf_file_read(CLSCONF,"screen","scr_width",KeyVal);
	if(strncmp(KeyVal,"272",3) == 0)
	{
		printf("haha\n");
		system("cp ./png/272/*.png /home/LEDscr/image/png");
		system("cp ./png/272/001.lst /home/LEDscr/list/");
	}
	else
	{
		system("cp ./png/480/*.png /home/LEDscr/image/png");
		system("cp ./png/480/001.lst /home/LEDscr/list/");
	}
	
	return 0;
}
