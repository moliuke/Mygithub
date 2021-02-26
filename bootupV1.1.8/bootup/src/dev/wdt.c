#include <stdio.h>
#include <sys/io.h>
#include <termios.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "wdt.h"
#include "../debug.h"


#define outp(a, b) outb(b, a)
#define inp(a) inb(a)


static int wdt_feed_flag = 1;
int wdt_init_flag = 0;


int kbhit(void)
{
	struct timeval tv;
	struct termios old_termios, new_termios;
	int error;
	int count = 0;
	tcgetattr(0, &old_termios);
	new_termios = old_termios;
	new_termios.c_lflag &= ~ICANON;
	new_termios.c_lflag &= ~ECHO;
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 1;
	error = tcsetattr(0, TCSANOW, &new_termios);
	tv.tv_sec = 0;
	tv.tv_usec = 100;
	select(1, NULL, NULL, NULL, &tv);
	error += ioctl(0, FIONREAD, &count);
	error += tcsetattr(0, TCSANOW, &old_termios);
	return error == 0 ? count : -1;
}


static void wdt_start(void)
{
	// Enable watchdog timer
	unsigned char c;
	
	outp(0x22, 0x37);
	c = inp(0x23);
	c |= 0x40;
	outp(0x22, 0x37);
	outp(0x23, c);
	outp(0x22, 0x13); // Lock register
	outp(0x23, 0x00); // Lock config. register
}

static void _wdt_stop(void)
{

	unsigned char c;
	DEBUG_PRINTF;
	iopl(3);
	
	DEBUG_PRINTF;
	outp(0x22, 0x13); // Lock register
	outp(0x23, 0xc5); // Unlock config. register
	outp(0x22, 0x37);
	c = inp(0x23);
	c &= ~(1 << 6);
	outp(0x22, 0x37);
	outp(0x23, c);
	outp(0x22, 0x13); // Lock register
	outp(0x23, 0x00); // Lock config. register
	DEBUG_PRINTF;
}

inline void wdt_stop(void)
{
	DEBUG_PRINTF;
	_wdt_stop();
}


void wdt_init(uint32_t time_ms)
{
	int fd = 0;
	float phz = 1.0 / 32768.0;

	float counter = (time_ms * 1000) / (phz * 1000 * 1000);

	uint32_t lTime = counter;
	//debug_printf("phz = %f,counter = %f,lTime = %d\n",phz,counter,lTime);
	
	iopl(3);
	outp(0x22, 0x13); // Lock register
	outp(0x23, 0xc5); // Unlock config. register

	//set time
	outp(0x22, 0x3b);
	outp(0x23, (lTime >> 16) & 0xff);
	
	outp(0x22, 0x3a);
	outp(0x23, (lTime >> 8) & 0xff);
	outp(0x22, 0x39);
	outp(0x23, (lTime >> 0) & 0xff);
	
	// Reset system
	outp(0x22, 0x38);
	unsigned char c = inp(0x23);
	c &= 0x0f;
	c |= 0xd0; // Reset system. For example, 0x50 to trigger IRQ7
	outp(0x22, 0x38);
	outp(0x23, c);

	//start wdt
	wdt_start();

	fd = open("/home/LEDscr/sys/wdtinit.log",O_WRONLY | O_CREAT,0744);
	if(fd <= 0)
		perror("wdt_init open");
		
}


void wdt_feed(uint8_t flag)
{
	//DEBUG_PRINTF;
	if(flag == WDT_FEED_STOP)
		wdt_feed_flag = 0;

	//DEBUG_PRINTF;
	if(wdt_feed_flag)
	{
		//DEBUG_PRINTF;
		outp(0x22, 0x13); // Unlock register
		outp(0x23, 0xc5);
		outp(0x22, 0x3c);
		unsigned char c = inp(0x23);
		outp(0x22, 0x3c);
		outp(0x23, c | 0x40);//write 1 to reset wdt 
		outp(0x22, 0x13); // Lock register
		outp(0x23, 0x00);
	}
}


#if 0
int main(void)
{
	iopl(3);
	outp(0x22, 0x13); // Lock register
	outp(0x23, 0xc5); // Unlock config. register
	
	// 500 mini-second
	unsigned int lTime = 0x20L * 500L;
	outp(0x22, 0x3b);
	outp(0x23, (lTime >> 16) & 0xff);
	outp(0x22, 0x3a);
	outp(0x23, (lTime >> 8) & 0xff);
	outp(0x22, 0x39);
	outp(0x23, (lTime >> 0) & 0xff);
	
	// Reset system
	outp(0x22, 0x38);
	unsigned char c = inp(0x23);
	c &= 0x0f;
	c |= 0xd0; // Reset system. For example, 0x50 to trigger IRQ7
	outp(0x22, 0x38);
	outp(0x23, c);
	
	// Enable watchdog timer
	outp(0x22, 0x37);
	c = inp(0x23);
	c |= 0x40;
	outp(0x22, 0x37);
	outp(0x23, c);
	outp(0x22, 0x13); // Lock register
	outp(0x23, 0x00); // Lock config. register
	debug_printf("Press any key to stop trigger timer.\n");
	
	//while(!kbhit())
	while(1)
	{
		//sleep(5);
		outp(0x22, 0x13); // Unlock register
		outp(0x23, 0xc5);
		outp(0x22, 0x3c);
		unsigned char c = inp(0x23);
		outp(0x22, 0x3c);
		outp(0x23, c | 0x40);//write 1 to reset wdt 
		outp(0x22, 0x13); // Lock register
		outp(0x23, 0x00);
	}
	
	debug_printf("System will reboot after 500 milli-seconds.\n");
	return 0;
}
#endif



